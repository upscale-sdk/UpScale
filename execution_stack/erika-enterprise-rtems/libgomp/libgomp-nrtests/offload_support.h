/* MPPA headers */

#include <mppa/osconfig.h>
#include <mppaipc.h>

/* CONFIGURATION PARAMETERS */

// total number of clusters
#define CLUSTER_COUNT 16
// cluster application binary name
#define CLUSTER_APP_NAME "erika"

/* OMP DATA STRUCTURES */

// kernel parameter descriptor
struct _omp_param {
  void *ptr;           // pointer to parameter data
  unsigned long size;  // parameter size (in bytes)
  char type;           // 0 = IN, 1 = OUT, 2 = INOUT
};

// outlined data structures
struct _params1 {
  int n;
  struct _omp_param params[1];
};

struct _params2 {
  int n;
  struct _omp_param params[2];
};

struct _params3 {
  int n;
  struct _omp_param params[3];
};

struct _params4 {
  int n;
  struct _omp_param params[4];
};

struct _params5 {
  int n;
  struct _omp_param params[5];
};

// command descriptor
struct _command {
  int kernel; // if >=0, it represents an index in kernel pointers table
  unsigned long in_data_counter;    // number of input parameters
  unsigned long inout_data_counter; // number of inout parameters
  unsigned long out_data_counter;   // number of output parameters
  unsigned long in_data_size;       // total byte size of input data
  unsigned long inout_data_size;    // total byte size of inout data
  unsigned long out_data_size;      // total byte size of output data
};


// Execution context
struct _gomp_context {
  const char *io_sync;
  int io_sync_fd;
  char cluster_sync[20];
  int cluster_sync_fd;
  char command_portal[20];
  int command_portal_fd;
  char input_data_portal[20];
  int input_data_portal_fd;
  char output_data_portal[20];
  int output_data_portal_fd;
};
struct _gomp_context ctx;


/* GOMP_init */
void GOMP_init(int device)
{
  long long match = -1LL ^ (1LL << device);
  long long dummy = 0;
  if(device == -1) device = 0; //device-var ICV, we always choose cluster 0

  // Kernel referring this device will be executed by the host
  if(device < 0 || device >= CLUSTER_COUNT)
  {
    return;
  }

  // Create syncs
  ctx.io_sync = "/mppa/sync/128:1";   // sync. connector address.
  ctx.io_sync_fd = mppa_open(ctx.io_sync, O_RDONLY);
  mppa_ioctl(ctx.io_sync_fd, MPPA_RX_SET_MATCH, match);
  sprintf(ctx.cluster_sync, "/mppa/sync/%d:2", device);
  ctx.cluster_sync_fd = mppa_open(ctx.cluster_sync, O_WRONLY);

  // Command portal
  sprintf(ctx.command_portal, "/mppa/portal/%d:3", device);
  ctx.command_portal_fd = mppa_open(ctx.command_portal, O_WRONLY);

  // Data portals
  sprintf(ctx.input_data_portal, "/mppa/portal/%d:4", device);
  ctx.input_data_portal_fd = mppa_open(ctx.input_data_portal, O_WRONLY);
  //EG: sprintf(ctx.output_data_portal, "/mppa/portal/128:5", device);
  sprintf(ctx.output_data_portal, "/mppa/portal/128:5");
  ctx.output_data_portal_fd = mppa_open(ctx.output_data_portal, O_RDONLY);

  // Prepare the arguments for the cluster application
  char arg0[4];
  sprintf(arg0, "%d", device);
  const char *argv_cluster[6];
  argv_cluster[0] = arg0;
  argv_cluster[1] = ctx.io_sync;
  argv_cluster[2] = ctx.cluster_sync;
  argv_cluster[3] = ctx.command_portal;
  argv_cluster[4] = ctx.input_data_portal;
  argv_cluster[5] = ctx.output_data_portal;

  // Spawn the applications on cluster "device"
  //  - device = cluster id
  //  - app_name = binary to run on compute clusters
  //  - argv_clusters = arguments to pass the cluster application
  mppa_spawn(device, NULL, CLUSTER_APP_NAME, argv_cluster, 0);

  // Wait sync
  mppa_read(ctx.io_sync_fd, &dummy, sizeof(dummy));;
}


/* GOMP_target */
void GOMP_target(int device, void (*fn) (void *), const void *target_fn, char *data)
{
  int i;
  long long dummy = 0;
  long long match = -1LL ^ (1LL << device);
  long long mask = (long long)1 << device;
  int ndata = *((int *)data);
  struct _omp_param *params = (struct _omp_param *)(((int *)data)+1);
  int fn_id = (int) target_fn;
  if(device == -1) device = 0; //device-var ICV, we always choose cluster 0

  // Executed by host
  if(device < 0 || device >= CLUSTER_COUNT)
  {
    fn(params);
    return;
  }

  // Prepare command
  struct _command cmd = {fn_id, 0, 0, 0, 0, 0, 0};
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
  mppa_ioctl(ctx.io_sync_fd, MPPA_RX_SET_MATCH, match);
  mppa_pwrite(ctx.command_portal_fd, &cmd, sizeof(struct _command), 0);

  // Write input data
  mppa_read(ctx.io_sync_fd, &dummy, sizeof(dummy));
  mppa_ioctl(ctx.io_sync_fd, MPPA_RX_SET_MATCH, match);
  mppa_pwrite(ctx.input_data_portal_fd, params, sizeof(struct _omp_param )*ndata, 0);
  unsigned long offset = sizeof(struct _omp_param )*ndata;
  for(i=0; i<ndata; ++i)
  {
    // IN / INOUT parameter
    if( (params[i].type == 0) || (params[i].type == 2) )
    {
      mppa_pwrite(ctx.input_data_portal_fd, params[i].ptr, params[i].size, offset);
      offset += params[i].size;
    }
  }

  // Read output
  for(i=0; i<ndata; ++i)
  {
    // OUT / INOUT parameter
    if( (params[i].type == 1) || (params[i].type == 2) )
    {
      mppa_aiocb_t output_data_portal_aiocb[1] = { MPPA_AIOCB_INITIALIZER(ctx.output_data_portal_fd, params[i].ptr, params[i].size) };
      mppa_aiocb_set_trigger(output_data_portal_aiocb, 1);
      mppa_aio_read(output_data_portal_aiocb);
      mppa_write(ctx.cluster_sync_fd, &mask, sizeof(mask));
      mppa_aio_wait(output_data_portal_aiocb);
    }
  }

}

/* GOMP_deinit */
void GOMP_deinit(int device)
{
  long long dummy = 0;
  long long match = -1LL ^ (1LL << device);
  struct _command cmd = {-1, 0, 0, 0, 0, 0, 0};

  // Deinit
  mppa_read(ctx.io_sync_fd, &dummy, sizeof(dummy));
  mppa_ioctl(ctx.io_sync_fd, MPPA_RX_SET_MATCH, match);
  mppa_pwrite(ctx.command_portal_fd, &cmd, sizeof(struct _command), 0);
}
