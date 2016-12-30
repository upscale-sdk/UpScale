#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <HAL/hal/hal.h>
#include <mppa/osconfig.h>

#include "shared_defs.h"
#include "ee.h"
#include "comm_layer.c"

/* OMP DATA STRUCTURES */

struct _omp_data_msg {
  char *memory_area;   // Area for libgomp global structures
  int  num_params;
  char *function_args; // Args for the offloaded function (from the host)
};

typedef struct EE_k1_omp_message_tag {
  struct _omp_param *params;
  omp_fn             func;
  unsigned int       slot_id;
} __attribute__ ((aligned(_K1_DCACHE_LINE_SIZE))) EE_k1_omp_message;

extern void GOMP_main (void *args);

static int init_state = 1;
int input_data_portal_fd[MAX_OFFLOADS_PER_CLUSTER];
int output_data_portal_fd[MAX_OFFLOADS_PER_CLUSTER];
unsigned char * current_output_addr[MAX_OFFLOADS_PER_CLUSTER];
unsigned long current_output_size[MAX_OFFLOADS_PER_CLUSTER];


#if   defined BAR_SW
#define _DTYPE  int
#elif defined BAR_SIGWAIT
#define _DTYPE  BlockableValueType
#endif

void gomp_interpret_mask(unsigned int mask, int *p2v_id_map, int *proc_num)
{
  int i;
#ifdef EE_SCHEDULER_GLOBAL
  *proc_num = __builtin_popcount(mask);
#else
  int proc_count = 0;
  for(i=0; i<16; ++i)
  {
    unsigned int bit = mask & 1;
    p2v_id_map[i] = (bit? proc_count++: -1);
    mask >>= 1;
  }
  *proc_num = proc_count;
#endif
}

struct _omp_data_msg msg_params[MAX_OFFLOADS_PER_CLUSTER];
EE_k1_omp_message msg[MAX_OFFLOADS_PER_CLUSTER];


char * memory[MAX_OFFLOADS_PER_CLUSTER];

int command_portal_fd;

int isPendingOffload()
{
  return comm_rx_pending(command_portal_fd);
}


#define CLUSTER_COUNT 16 

/* Communication hook */
void CommunicationHook( void )
{
  // Get arguments
  k1_boot_args_t args;
  get_k1_boot_args(&args);

  static int rank;
  static int io_sync;
  static int command_portal;
  static int input_data_portal[MAX_OFFLOADS_PER_CLUSTER];
  static int output_data_portal[MAX_OFFLOADS_PER_CLUSTER];
  static struct _command cmd;
  static int io_sync_fd /*command_portal_fd,*/ ;
  static JobType job_id[MAX_OFFLOADS_PER_CLUSTER];
  struct _omp_param *params = 0;
  long long mask = (long long)1 << __k1_get_cluster_id();;
  int i;
  char *omp_data;

  // Initialization phase (executed once)
  if(init_state)
  {
    rank = __k1_get_cluster_id();
    io_sync = 1+rank;
    command_portal = io_sync+CLUSTER_COUNT;
    io_sync_fd = comm_sync_tx_open(io_sync, rank, 128);
    command_portal_fd = comm_rx_open(command_portal, 128, rank);
    comm_rx_read(command_portal_fd, &cmd, sizeof(struct _command), 0);
    for(i=0; i<MAX_OFFLOADS_PER_CLUSTER; ++i)
    {
      job_id[i] = -1;
      input_data_portal[i]  = 1 + 2*CLUSTER_COUNT + rank*2*MAX_OFFLOADS_PER_CLUSTER + 2*i; 
      output_data_portal[i] =  input_data_portal[i] + 1; //atoi(args.argv[3+2*i+1]);
      input_data_portal_fd[i] = comm_rx_open(input_data_portal[i], 128, __k1_get_cluster_id());
      output_data_portal_fd[i] = comm_tx_open(output_data_portal[i], __k1_get_cluster_id(), 128);
    }
    comm_sync_tx_write(io_sync_fd);
    init_state = 0;
  }

  // Check for offload commands
  int status = comm_rx_ready(command_portal_fd, sizeof(struct _command));
  if(!status) return;

  // Offload request
  unsigned int slot_id = cmd.slot_id;
  int ndata = cmd.in_data_counter+cmd.inout_data_counter +cmd.out_data_counter;
  unsigned long header_size = sizeof(struct _omp_param )*ndata;
  unsigned long input_size = cmd.in_data_size + cmd.inout_data_size;
  unsigned long total_size = header_size + input_size + cmd.out_data_size;
  omp_data = memalign(8,total_size);
  
  // Read input data
  if(cmd.in_data_counter + cmd.inout_data_counter > 0)
  {
    comm_rx_read(input_data_portal_fd[slot_id], omp_data, header_size+input_size, 0);
    comm_sync_tx_write(io_sync_fd);
    comm_rx_wait(input_data_portal_fd[slot_id]);

    params = (struct _omp_param *) omp_data;
    unsigned long offset = 0;
    for ( i=0; i < ndata; ++i ) {
      params[i].ptr = omp_data+header_size+offset;
      offset += params[i].size;
    }
  }
  else
    params = 0;

  // Status for output copy-back
  current_output_addr[cmd.slot_id] = params[cmd.in_data_counter].ptr;
  current_output_size[cmd.slot_id] = cmd.inout_data_size +cmd.out_data_size;

  // Init environment for OpenMP runtime
  if(!memory[cmd.slot_id])
    memory[cmd.slot_id] = memalign(8, 0x9000);
  memset(memory[cmd.slot_id], 0x0, 0x9000);
#if defined BAR_SW
  memset(memory[cmd.slot_id], 0x0, EE_K1_PE_NUMBER*sizeof(int)*2);
#elif defined BAR_SIGWAIT
  BlockableValueType *barriers = (BlockableValueType *) memory[cmd.slot_id];
  for(i=0; i<EE_K1_CORE_NUMBER*2; ++i)
  {
    InitBlockableValue(&barriers[i], 0U);
  }
#endif
  msg_params[slot_id] = (struct _omp_data_msg){
    .memory_area   = memory[cmd.slot_id],
    .num_params = (int)params,
    .function_args = (char *)params
  };
  msg[slot_id] = (EE_k1_omp_message) {
    .func = omp_functions[cmd.kernel],
    .slot_id = cmd.slot_id,
    .params = (struct _omp_param *)&msg_params[slot_id]
  };

  // Rearm command channel
  comm_rx_rearm(command_portal_fd);


  // Create job
  if ( job_id[cmd.slot_id] == -1 ) {
    CreateJob(&job_id[cmd.slot_id], 0xFFFF, cmd.slot_id+1, (JobTaskFunc) GOMP_main, &msg[slot_id], ERIKA_TASK_STACK_SIZE);
  }

  // Activate job
  int *p2v_id_map = (int *)(memory[cmd.slot_id] + 16*sizeof(_DTYPE)*2);
  int *proc_num   = (int *)(memory[cmd.slot_id] + 16*sizeof(_DTYPE)*2+16*sizeof(int));
  gomp_interpret_mask(cmd.mask, p2v_id_map, proc_num);

  ReadyJob(job_id[cmd.slot_id], cmd.mask);

  // Last sync write to release GOMP_target
  comm_sync_tx_write(io_sync_fd);
}

