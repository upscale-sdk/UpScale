#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <HAL/hal/hal.h>
#include <mppaipc.h>
#include <mppa/osconfig.h>

#include "shared_defs.h"
#include "ee.h"

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
int output_data_portal_fd[MAX_OFFLOADS_PER_CLUSTER];
unsigned char * current_output_addr[MAX_OFFLOADS_PER_CLUSTER];
unsigned long current_output_size[MAX_OFFLOADS_PER_CLUSTER];

mppa_aiocb_t command_portal_aiocb[1];

int isPendingOffload()
{
  int status = mppa_aio_error(command_portal_aiocb);
  //printf("COMMAND STATUS = %d \n", status);
  return status==0? 1 :0;
}

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
  for (i=0; i < 16; ++i)
  {
    unsigned int bit = mask & 1;
    p2v_id_map[i] = (bit? proc_count++: -1);
    mask >>= 1;
  }
  *proc_num = proc_count;
#endif
  //printf("############# PROC NUM = %d\n", *proc_num);
  //for(i=0; i<16; ++i)
  //  printf(" vproc[%d] = %d\n", i, p2v_id_map[i]);
}

struct _omp_data_msg msg_params[MAX_OFFLOADS_PER_CLUSTER];
EE_k1_omp_message msg[MAX_OFFLOADS_PER_CLUSTER];


char * memory[MAX_OFFLOADS_PER_CLUSTER];

/* Communication hook */
void CommunicationHook( void )
{
  // Get arguments
  k1_boot_args_t args;
  get_k1_boot_args(&args);
  //printf("START CommunicationHook\n");

  static int rank;
  static char *io_sync;
  static char *command_portal;
  static char *input_data_portal[MAX_OFFLOADS_PER_CLUSTER];
  static char *output_data_portal[MAX_OFFLOADS_PER_CLUSTER];
  static struct _command cmd;
  static int io_sync_fd, command_portal_fd, input_data_portal_fd[MAX_OFFLOADS_PER_CLUSTER];
  static JobType job_id[MAX_OFFLOADS_PER_CLUSTER];
  struct _omp_param *params = 0;
  long long mask = 1ULL << __k1_get_cluster_id();
  int i;
  char *omp_data;

  // Initialization phase (executed once)
  if(init_state)
  {
    rank = atoi(args.argv[0]);
    io_sync = args.argv[1];
    command_portal = args.argv[2];
    io_sync_fd = mppa_open(io_sync, O_WRONLY);
    command_portal_fd = mppa_open(command_portal, O_RDONLY);
    mppa_aiocb_ctor(command_portal_aiocb, command_portal_fd, &cmd, sizeof(struct _command));
    mppa_aio_read(command_portal_aiocb);
    for(i=0; i<MAX_OFFLOADS_PER_CLUSTER; ++i)
    {
      job_id[i] = -1;
      input_data_portal[i]  = args.argv[3+2*i+0];
      output_data_portal[i] = args.argv[3+2*i+1];
      input_data_portal_fd[i] = mppa_open(input_data_portal[i], O_RDONLY);
      output_data_portal_fd[i] = mppa_open(output_data_portal[i], O_WRONLY);
    }
    mppa_write(io_sync_fd, &mask, sizeof(mask));
    init_state = 0;
    //printf("INIT DONE\n");
  }

  // Check for offload commands
  int status = mppa_aio_error(command_portal_aiocb);
  //printf("COMMAND STATUS = %d \n", status);
  if(status != 0) return; // return

  // Offload request
  //printf("******** KERNEL TO EXECUTE = %d (in slot %d)\n", cmd.kernel, cmd.slot_id);
  unsigned int slot_id = cmd.slot_id;
  int ndata = cmd.in_data_counter+cmd.inout_data_counter +cmd.out_data_counter;
  unsigned long header_size = sizeof(struct _omp_param )*ndata;
  unsigned long input_size = cmd.in_data_size + cmd.inout_data_size;
  unsigned long total_size = header_size + input_size + cmd.out_data_size;
  omp_data = malloc(total_size);

  // Read input data
  if(cmd.in_data_counter + cmd.inout_data_counter > 0)
  {
    mppa_aiocb_t input_data_portal_aiocb[1] = { MPPA_AIOCB_INITIALIZER(input_data_portal_fd[slot_id], omp_data, header_size+input_size) };
    mppa_aiocb_set_trigger(input_data_portal_aiocb, 1);
    mppa_aio_read(input_data_portal_aiocb);
    mppa_write(io_sync_fd, &mask, sizeof(mask));
    mppa_aio_wait(input_data_portal_aiocb);

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
  //printf("params[cmd.in_data_counter].ptr = %lu\n", cmd.in_data_counter);
  current_output_size[cmd.slot_id] = cmd.inout_data_size +cmd.out_data_size;

  // Init environment for OpenMP runtime
  if(!memory[cmd.slot_id])
    memory[cmd.slot_id] = memalign(8, 0x900);
  memset(memory[cmd.slot_id], 0x0, 0x900);
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
    .num_params = (int)params, /* EG: XXX: siamo sicuri di questo ??? */
    .function_args = (char *)params
  };
  msg[slot_id] = (EE_k1_omp_message) {
    .func = omp_functions[cmd.kernel],
    .slot_id = cmd.slot_id,
    .params = (struct _omp_param *)&msg_params[slot_id]
  };

  // Rearm command channel
  mppa_aio_return(command_portal_aiocb);
  mppa_aio_read(command_portal_aiocb);

  // Create job
  if ( job_id[cmd.slot_id] == -1 ) {
    CreateJob(&job_id[cmd.slot_id], 0xFFFF, slot_id+1, (JobTaskFunc) GOMP_main, &msg[slot_id], 4*1024U/*ERIKA_TASK_STACK_SIZE*/);
  }

  // Activate job
  int *p2v_id_map = (int *)(memory[cmd.slot_id] + 16*sizeof(_DTYPE)*2);
  int *proc_num   = (int *)(memory[cmd.slot_id] + 16*sizeof(_DTYPE)*2+16*sizeof(int));
  gomp_interpret_mask(cmd.mask, p2v_id_map, proc_num);

  ReadyJob(job_id[cmd.slot_id], cmd.mask);

  // Last sync write to release GOMP_target
  mppa_write(io_sync_fd, &mask, sizeof(mask));

}

