/** Application (CLUSTER SIDE) **/

/* MPPA headers */
#include <mppaipc.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "ee.h"
#include "cluster.h"

/* OFFOAD OUTLINED FUNCTIONS */

int INSTRUCTIONS;
#define ITER 1
#define LEVELS 1
#define CHILDREN 2

void __attribute__((optimize("O0"))) do_work() 
{
  unsigned int instructions = INSTRUCTIONS/4;   
  //int a0, a1, a2, a3;
  __asm__ __volatile__  (
    "make $r1 = 0 \n\t ;; \n\t "
    "copy $r2 = %0 \n\t ;; \n\t "
    "loop: \n\t"
    "add $r4 = $r4, 1 \n\t ;; \n\t"
    "add $r1 = $r1, 1 \n\t ;; \n\t"
    "comp.gt $r3 = $r2, $r1 \n\t ;; \n\t"
    "cb.nez $r3, loop \n\t ;; \n\t"
    : //"=X" (start)
    : "r" (instructions)
    :"r1","r2","r3", "r4");
  //printf("ADDR: %x, %x, %x, %x\n", &a0, &a1, &a2, &a3);
  //printf("instructions = %d\n", instructions);
}

struct OUT__3__3653___data 
{
  unsigned int level;
};

static void OUT__1__3653__(void *__out_argv);
static void OUT__2__3653__(void *__out_argv);
static void OUT__3__3653__(void *__out_argv);

void create_level(unsigned int level)
{
  int seed = omp_get_thread_num();
  int value = rand_r(seed);
  printf(">>> (%d) in level = %d\n", value, level);
  do_work();
  if (level == 0) {
    printf(">>> (%d) out level 0\n", value);
    return ;
  }
  unsigned int i;
  for (i = 0; i < CHILDREN; i++) {
    struct OUT__3__3653___data __out_argv3__3653__;
    __out_argv3__3653__.level = level;
    GOMP_task(OUT__3__3653__,&__out_argv3__3653__,0,sizeof(struct OUT__3__3653___data ),4,1,1);
  }
  GOMP_taskwait();
  printf(">>> (%d) out level = %d\n", value, level);
}

void omp_fn4(struct _omp_param *params)
{
  printf("ENTER fn4\n");
  int k;
  unsigned long sum = 0;
  for (k = 0; k < ITER; ++k) {
    INSTRUCTIONS = 10;
    GOMP_parallel_start(OUT__2__3653__,0,16);
    OUT__2__3653__(0);
    GOMP_parallel_end();
  }
  printf("Exit fn4 %lu\n",(sum / ITER));
}

static void OUT__1__3653__(void *__out_argv)
{
  printf("TASK root start \n");
  create_level(LEVELS);
}

static void OUT__2__3653__(void *__out_argv)
{
  printf("START\n");
  if (GOMP_single_start()) {
    printf("SINGLE start \n");
    GOMP_task(OUT__1__3653__,0,0,0,0,1,0);
  }
  GOMP_barrier();
}

static void OUT__3__3653__(void *__out_argv)
{
  unsigned int level = (unsigned int )(((struct OUT__3__3653___data *)__out_argv) -> level);
  unsigned int _p_level = level;
//printf("create_level (%d)\n", level-1);
  create_level((_p_level - 1));
}



/* KERNEL POINTERS */

omp_fn omp_functions[] = { 0, 0, omp_fn4 };


/* CALLBACKS */

#ifdef EE_CONF_LIBGOMP

int main(int argc, char* argv[]) {
  printf("TEST\n");
  return 0;
}

void gomp_runtime_entry( /*TODO */)
{


  while ( 1 ) {
    /* While loop exit  */
  }
}
#endif /* EE_CONF_LIBGOMP */

#if (!defined(LIBMPPA))
/* OFFLOAD HOOK */

extern void GOMP_main (void *args);

void CommunicationHook ( void )
{
    k1_boot_args_t args;
    get_k1_boot_args(&args);

    // Get arguments
    int rank = atoi(args.argv[0]);
    const char *io_sync = args.argv[1];
    const char *cluster_sync = args.argv[2];
    const char *command_portal = args.argv[3];
    const char *input_data_portal = args.argv[4];
    const char *output_data_portal = args.argv[5];

    long long match = -1LL ^ (1LL << rank);
    long long mask = (long long)1 << rank;
    long long dummy = 0;

    int i;
    struct _command cmd;
    int io_sync_fd, cluster_sync_fd, command_portal_fd, input_data_portal_fd, output_data_portal_fd;
    char *omp_data;

    // Sync channels
    io_sync_fd = mppa_open(io_sync, O_WRONLY);
    cluster_sync_fd = mppa_open(cluster_sync, O_RDONLY);
    mppa_ioctl(cluster_sync_fd, MPPA_RX_SET_MATCH, match);

    // Command portal
    command_portal_fd = mppa_open(command_portal, O_RDONLY);
    mppa_aiocb_t command_portal_aiocb[1] =  { MPPA_AIOCB_INITIALIZER(command_portal_fd, &cmd, sizeof(struct _command)) };

    // Data portals
    input_data_portal_fd = mppa_open(input_data_portal, O_RDONLY);
    output_data_portal_fd = mppa_open(output_data_portal, O_WRONLY);

    // Wait for command data
    mppa_aio_read(command_portal_aiocb);
    mppa_write(io_sync_fd, &mask, sizeof(mask));
    mppa_aio_wait(command_portal_aiocb);
    while (cmd.kernel >= 0) // -1 == exit command
    {
      // Data buffer allocation
      int ndata = cmd.in_data_counter+cmd.inout_data_counter +cmd.out_data_counter;
      unsigned long header_size = sizeof(struct _omp_param )*ndata;
      unsigned long input_size = cmd.in_data_size + cmd.inout_data_size;
      //EG: unused unsigned long output_size = cmd.inout_data_size + cmd.out_data_size;
      unsigned long total_size = header_size + input_size + cmd.out_data_size;
      omp_data = malloc(total_size);

      // Read input data (IO -> cluster)
      mppa_aiocb_t input_data_portal_aiocb[1] = { MPPA_AIOCB_INITIALIZER(input_data_portal_fd, omp_data, header_size+input_size) };
      mppa_aiocb_set_trigger(input_data_portal_aiocb, cmd.in_data_counter+cmd.inout_data_counter+1);
      mppa_aio_read(input_data_portal_aiocb);
      mppa_write(io_sync_fd, &mask, sizeof(mask));
      mppa_aio_wait(input_data_portal_aiocb);

      // Update data pointers for cluster side
      struct _omp_param *params = (struct _omp_param *) omp_data;
      unsigned long offset = 0;
      printf("NDATA = %d\n", ndata);
      for ( i=0; i < ndata; ++i ) {
        params[i].ptr = omp_data+header_size+offset;
        offset += params[i].size;
      }

      // Execute selected kernel
      {
        char *memory = memalign(8, 0x9000);
        #if   defined BAR_SW
        memset(memory, 0x0, EE_K1_PE_NUMBER*sizeof(int)*2);
        #elif defined BAR_SIGWAIT
        BlockableValueType *barriers = (BlockableValueType *) memory;
        for(i=0; i<EE_K1_CORE_NUMBER*2; ++i)
        {
          InitBlockableValue(&barriers[i], 0U);
        }
        #endif

        struct _omp_data_msg msg_params = {.memory_area   = memory,
                                           .function_args = (char *)params };
        EE_k1_omp_message msg = {
          .func = omp_functions[cmd.kernel],
          .params = (struct _omp_param *)&msg_params
        };

        JobType job_id;
        CreateJob(&job_id, EE_K1_CORE_NUMBER, 1U, (JobTaskFunc)GOMP_main, &msg, 1024U);
        ActivateJob(job_id, EE_K1_CORE_NUMBER);
        JoinJob(job_id);

        free(memory);
      }

      // Copy back data (cluster -> IO)
      for ( i=0; i<ndata; ++i ) {
        // OUT / INOUT parameter
        if( (params[i].type == 1) || (params[i].type == 2) ) {
          mppa_read(cluster_sync_fd, &dummy, sizeof(dummy));
          mppa_ioctl(cluster_sync_fd, MPPA_RX_SET_MATCH, match);
          mppa_pwrite(output_data_portal_fd, params[i].ptr, params[i].size, 0);
        }
      }

      // Read next command
      mppa_aio_read(command_portal_aiocb);
      mppa_write(io_sync_fd, &mask, sizeof(mask));
      mppa_aio_wait(command_portal_aiocb);

      // Free resources
      free(omp_data);
      printf ("Command Executed\n");
    }
}
#endif /* !LIBMPPA */
