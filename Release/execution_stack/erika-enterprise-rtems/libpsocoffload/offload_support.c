/* MPPA headers */
#include <mppa/osconfig.h>
#include <mppaipc.h>
#include "shared_defs.h"

/* CONFIGURATION PARAMETERS */

#define CLUSTER_APP_NAME "erika"

unsigned char * offload_output_buffer[CLUSTER_COUNT][MAX_OFFLOADS_PER_CLUSTER];
mppa_aiocb_t output_data_portal_aiocb[CLUSTER_COUNT][MAX_OFFLOADS_PER_CLUSTER];

void GOMP_init(int device)
{
  int i;
  long long match = -1LL ^ (1LL << device);
  long long dummy = 0;
  if(device == -1) device = 0; //device-var ICV, we always choose cluster 0

  // Kernel referring this device will be executed by the host
  if(device < 0 || device >= CLUSTER_COUNT)
  {
    return;
  }

  // Create sync
  sprintf(ctx[device].io_sync, "/mppa/sync/128:%d", (1+device));
  ctx[device].io_sync_fd = mppa_open(ctx[device].io_sync, O_RDONLY);
  mppa_ioctl(ctx[device].io_sync_fd, MPPA_RX_SET_MATCH, match);

  // Command portal
  sprintf(ctx[device].command_portal, "/mppa/portal/%d:%d", device, (1 + CLUSTER_COUNT + device));
  ctx[device].command_portal_fd = mppa_open(ctx[device].command_portal, O_WRONLY);

  // IO data portals
  for(i=0;i<MAX_OFFLOADS_PER_CLUSTER; ++i)
  {
    sprintf(ctx[device].input_data_portal[i], "/mppa/portal/%d:%d", device, (1 + 2*CLUSTER_COUNT + device*2*MAX_OFFLOADS_PER_CLUSTER + 2*i+0));
    ctx[device].input_data_portal_fd[i] = mppa_open(ctx[device].input_data_portal[i], O_WRONLY);
    sprintf(ctx[device].output_data_portal[i], "/mppa/portal/128:%d", (2*CLUSTER_COUNT+1 + device*2*MAX_OFFLOADS_PER_CLUSTER + 2*i+1));
    ctx[device].output_data_portal_fd[i] = mppa_open(ctx[device].output_data_portal[i], O_RDONLY);
  }

  // Prepare the arguments for the cluster application
  char arg0[4];
  sprintf(arg0, "%d", device);
  const char *argv_cluster[3+MAX_OFFLOADS_PER_CLUSTER*2];
  argv_cluster[0] = arg0;
  argv_cluster[1] = ctx[device].io_sync;
  argv_cluster[2] = ctx[device].command_portal;
  for(i=0; i<MAX_OFFLOADS_PER_CLUSTER; ++i)
  {
    argv_cluster[3 + 2*i+0] = ctx[device].input_data_portal[i];
    argv_cluster[3 + 2*i+1] = ctx[device].output_data_portal[i];
  }

  // Spawn the applications on cluster "device"
  //  - device = cluster id
  //  - app_name = binary to run on compute clusters
  //  - argv_clusters = arguments to pass the cluster application
  mppa_spawn(device, NULL, "erika", argv_cluster, 0);

  // Wait sync
  mppa_read(ctx[device].io_sync_fd, &dummy, sizeof(dummy));;
  mppa_ioctl(ctx[device].io_sync_fd, MPPA_RX_SET_MATCH, match);
}


void *data_refs[CLUSTER_COUNT][MAX_OFFLOADS_PER_CLUSTER];

/* GOMP_target */
void GOMP_target(int device, void (*fn) (void *), const void *target_fn, void *data, unsigned int slot_id, unsigned int mask)
{
  int i;
  long long dummy = 0;
  long long match = -1LL ^ (1LL << device);
  int ndata = *((int *)data);
  struct _omp_param *params = (struct _omp_param *)(((int *)data)+1);
  int fn_id = (int) target_fn;
  if(device == -1) device = 0; //device-var ICV, we always choose cluster 0
  data_refs[device][slot_id] = data;

  // Executed by host
  if(device < 0 || device >= CLUSTER_COUNT)
  {
    if(fn) fn(params);
    return;
  }

  // Prepare command
  struct _command cmd = {fn_id, slot_id, mask, 0, 0, 0, 0, 0, 0};
  for(i=0; i<ndata; ++i)
  {
    // IN parameter
    if( params[i].type == 0 )
    {
      cmd.in_data_counter++;
      cmd.in_data_size += params[i].size;
    }
    // OUT parameter
    else if( params[i].type == 1 )
    {
      cmd.out_data_counter++;
      cmd.out_data_size += params[i].size;
    }
    // INOUT parameter
    else if( params[i].type == 2 )
    {
      cmd.inout_data_counter++;
      cmd.inout_data_size += params[i].size;
    }
  }

  // Write command
  mppa_pwrite(ctx[device].command_portal_fd, &cmd, sizeof(struct _command), 0);

  // Configure output channel
  int outputSize = cmd.out_data_size + cmd.inout_data_size ;
  if(outputSize == 0) outputSize = 8;
  //offload_output_buffer[device][slot_id]  = malloc(outputSize);
  posix_memalign(&offload_output_buffer[device][slot_id], 8, outputSize);
  memset(offload_output_buffer[device][slot_id], 0, outputSize);
  //printf("OUTPUT ADDRESS = %x\n", offload_output_buffer[device][slot_id]);
  //printf("*** OUTPUT SIZE = %d\n", outputSize);
  mppa_aiocb_ctor(&output_data_portal_aiocb[device][slot_id], ctx[device].output_data_portal_fd[slot_id], offload_output_buffer[device][slot_id], outputSize);
  //mppa_aiocb_set_pwrite(&output_data_portal_aiocb[device][slot_id],  offload_output_buffer[device][slot_id], outputSize, 0);
  mppa_aiocb_set_trigger(&output_data_portal_aiocb[device][slot_id], 1);
  mppa_aio_read(&output_data_portal_aiocb[device][slot_id]);

  // Write input data
  mppa_ioctl(ctx[device].input_data_portal_fd[slot_id], MPPA_TX_NOTIFY_OFF);
  mppa_read(ctx[device].io_sync_fd, &dummy, sizeof(dummy));
  mppa_ioctl(ctx[device].io_sync_fd, MPPA_RX_SET_MATCH, match);
  //printf("[IO] ready to write...\n");
  mppa_pwrite(ctx[device].input_data_portal_fd[slot_id], params, sizeof(struct _omp_param )*ndata, 0);
  unsigned long offset = sizeof(struct _omp_param )*ndata;
  int counter = 0;
  for(i=0; i<ndata; ++i)
  {
    // IN / INOUT parameter
    if( (params[i].type == 0) || (params[i].type == 2) )
    {
      counter++;
      if(counter == cmd.in_data_counter+cmd.inout_data_counter) mppa_ioctl(ctx[device].input_data_portal_fd[slot_id], MPPA_TX_NOTIFY_ON);
      // printf("[IO] write pre\n");
      mppa_pwrite(ctx[device].input_data_portal_fd[slot_id], params[i].ptr, params[i].size, offset);
      // printf("[IO] write post\n");
      offset += params[i].size;
    }
  }

  // Last sync
  mppa_read(ctx[device].io_sync_fd, &dummy, sizeof(dummy));
  mppa_ioctl(ctx[device].io_sync_fd, MPPA_RX_SET_MATCH, match);
}

void GOMP_target_wait(int device, void *_data, unsigned int slot_id)
{
  void * data = data_refs[device][slot_id];
  int i;
  int ndata = *((int *)data);
  struct _omp_param *params = (struct _omp_param *)(((int *)data)+1);
  unsigned long offset = 0;

  // Wait copy back
  mppa_aio_wait(&output_data_portal_aiocb[device][slot_id]);
  //printf("###################################data = %d\n", ((int *)offload_output_buffer[device][slot_id])[17]);

 // Copy data into final destinations
  for(i=0; i<ndata; ++i)
  {
    // OUT / INOUT parameter
    if( (params[i].type == 2) )
    {
      //printf(" ######################### DATA = %d\n", *((int*)offload_output_buffer[device][slot_id]+offset));
      memcpy(params[i].ptr, offload_output_buffer[device][slot_id]+offset, params[i].size);
      offset += params[i].size;
    }
  }
  for(i=0; i<ndata; ++i)
  {
    if( (params[i].type == 1) )
    {
      //printf("DATA = %f\n", *((double*)(offload_output_buffer[device][slot_id]+offset)));
      memcpy(params[i].ptr, offload_output_buffer[device][slot_id]+offset, params[i].size);
      offset += params[i].size;
    }
  }

  // Free the copy back buffer
  free(offload_output_buffer[device][slot_id]);

  // Free data
  free(data);
  data_refs[device][slot_id] = 0;
}

/* GOMP_deinit */
// TODO this function is not used now
void GOMP_deinit(int device)
{
 int i, j;
 for(i=0; i<CLUSTER_COUNT; ++i)
 for(j=0; j<MAX_OFFLOADS_PER_CLUSTER; ++j)
   if(data_refs[i][j]) free(data_refs[i][j]); 
/*
  long long dummy = 0;
  //long long match = -1LL ^ (1LL << device);
  struct _command cmd = {-1, 0, 0, 0, 0, 0, 0, 0, 0};

  // Deinit
  mppa_read(ctx[device].io_sync_fd, &dummy, sizeof(dummy));
  //mppa_ioctl(ctx[device].io_sync_fd, MPPA_RX_SET_MATCH, match);
  mppa_pwrite(ctx[device].command_portal_fd, &cmd, sizeof(struct _command), 0);
*/
}
