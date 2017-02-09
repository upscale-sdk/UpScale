/* MPPA headers */
#include <mppa/osconfig.h>
#include <malloc.h>
#include "shared_defs.h"

#include "comm_layer.c"

/* CONFIGURATION PARAMETERS */

#define CLUSTER_APP_NAME "erika"

unsigned char * offload_output_buffer[CLUSTER_COUNT][MAX_OFFLOADS_PER_CLUSTER];

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
  ctx[device].io_sync_fd = comm_sync_rx_open(1+device, device, 128);
  sprintf(ctx[device].io_sync, "%d", 1+device);

  // Command portal
  ctx[device].command_portal_fd = comm_tx_open((1 + CLUSTER_COUNT + device), 128, device);
  sprintf(ctx[device].command_portal,  "%d", (1 + CLUSTER_COUNT + device));

  // IO data portals
  for(i=0;i<MAX_OFFLOADS_PER_CLUSTER; ++i)
  {
    ctx[device].input_data_portal_fd[i]  = comm_tx_open((1 + 2*CLUSTER_COUNT + device*2*MAX_OFFLOADS_PER_CLUSTER + 2*i+0), 128, device);
    sprintf(ctx[device].input_data_portal[i], "%d", (1 + 2*CLUSTER_COUNT + device*2*MAX_OFFLOADS_PER_CLUSTER + 2*i+0));
    ctx[device].output_data_portal_fd[i] = comm_rx_open((2*CLUSTER_COUNT+1 + device*2*MAX_OFFLOADS_PER_CLUSTER + 2*i+1), device, 128);
    sprintf(ctx[device].output_data_portal[i], "%d", (2*CLUSTER_COUNT+1 + device*2*MAX_OFFLOADS_PER_CLUSTER + 2*i+1));
  }

  // Prepare the arguments for the cluster application
  char arg0[4];
  sprintf(arg0, "%d", device);
  const char *argv_cluster[3+MAX_OFFLOADS_PER_CLUSTER*2+1];
  memset(argv_cluster, 0x00, sizeof(argv_cluster));
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
  SPAWN(device, "erika", argv_cluster, 0);

  // Wait sync
  comm_sync_rx_wait(ctx[device].io_sync_fd);
}


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


  /// Write command
  comm_tx_write_eot(ctx[device].command_portal_fd, &cmd, sizeof(struct _command), 0);

  // Configure output channel
  int outputSize = cmd.out_data_size + cmd.inout_data_size;
  if(outputSize == 0) outputSize = 8;
#ifdef linux
  offload_output_buffer[device][slot_id] = mppa_noc_buffer_alloc(outputSize);
#else
  posix_memalign(&offload_output_buffer[device][slot_id], 8, outputSize);
#endif
  memset(offload_output_buffer[device][slot_id], 42, outputSize);
  comm_rx_read(ctx[device].output_data_portal_fd[slot_id], offload_output_buffer[device][slot_id], outputSize, 0);

  // Write input data
  comm_sync_rx_wait(ctx[device].io_sync_fd);
  comm_tx_write(ctx[device].input_data_portal_fd[slot_id], params, sizeof(struct _omp_param )*ndata, 0);
  unsigned long offset = sizeof(struct _omp_param )*ndata;
  int counter = 0;
  for(i=0; i<ndata; ++i)
  {
    // IN / INOUT parameter
    if( (params[i].type == 0) || (params[i].type == 2) )
    {
      counter++;
      comm_tx_write(ctx[device].input_data_portal_fd[slot_id], params[i].ptr, params[i].size, offset);
      if(counter == cmd.in_data_counter+cmd.inout_data_counter) comm_tx_eot(ctx[device].input_data_portal_fd[slot_id]);
      offset += params[i].size;
    }
  }

  // Last sync
  comm_sync_rx_wait(ctx[device].io_sync_fd);
}

void GOMP_target_wait(int device, void *data, unsigned int slot_id)
{
  int i;
  int ndata = *((int *)data);
  struct _omp_param *params = (struct _omp_param *)(((int *)data)+1);
  unsigned long offset = 0;

  // Wait copy back
  comm_rx_wait(ctx[device].output_data_portal_fd[slot_id]);

  // Copy data into final destinations
  for(i=0; i<ndata; ++i)
  {
    // OUT / INOUT parameter
    if( (params[i].type == 2) )
    {
      memcpy(params[i].ptr, offload_output_buffer[device][slot_id]+offset, params[i].size);
      offset += params[i].size;
    }
  }
  for(i=0; i<ndata; ++i)
  {
    if( (params[i].type == 1) )
    {
      memcpy(params[i].ptr, offload_output_buffer[device][slot_id]+offset, params[i].size);
      offset += params[i].size;
    }
  }
  // Free the copy back buffer
  free(offload_output_buffer[device][slot_id]);
}

/* GOMP_deinit */
void GOMP_deinit(int device)
{
  long long dummy = 0;
  struct _command cmd = {-1, 0, 0, 0, 0, 0, 0, 0, 0};

  // Deinit
  comm_sync_rx_wait(ctx[device].io_sync_fd);
  comm_tx_write_eot(ctx[device].command_portal_fd, &cmd, sizeof(struct _command), 0);

}
