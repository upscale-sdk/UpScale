/** Application (CLUSTER SIDE) **/

/* MPPA headers */
#include <mppaipc.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "ee.h"
#include "cluster.h"

/* OFFOAD OUTLINED FUNCTIONS */

#define SIZE 16

void omp_fn1_old(struct _omp_param *params)
{
  int *a = params[0].ptr;
  int *b = params[1].ptr;
  int *c = params[2].ptr;

  // START USER CODE
  int i, j, k;

  #pragma omp parallel for schedule(static, 2)
  for (i = 0; i < SIZE*SIZE; ++i)
    c[i] = 0;

  #pragma omp parallel for
  for (j = 0; j < SIZE; ++j)
    for (k = 0; k < SIZE; ++k)
      for (i = 0; i < SIZE; ++i)
        c[j * SIZE + i] += a[k * SIZE + i] * b[j * SIZE + k];
  // END USER CODE
}

struct OUT__1__3603___data
{
  void *a_p;
  void *b_p;
  void *c_p;
}
;
void OUT__1__3603__(void *__out_argv);

struct OUT__2__3603___data
{
  void *c_p;
}
;
void OUT__2__3603__(void *__out_argv);

void omp_fn1(struct _omp_param *params)
{
  int *a = params[0].ptr;
  int *b = params[1].ptr;
  int *c = params[2].ptr;
  
// START USER CODE
  int i;
  int j;
  int k;
  struct OUT__2__3603___data __out_argv2__3603__;
  __out_argv2__3603__.c_p = ((void *)(&c));
  printf("a\n");
  GOMP_parallel_start(OUT__2__3603__,&__out_argv2__3603__,16);
  printf("b\n");
  OUT__2__3603__(&__out_argv2__3603__);
  printf("c\n");
  GOMP_parallel_end();

    printf("a1\n");
  struct OUT__1__3603___data __out_argv1__3603__;
  __out_argv1__3603__.c_p = ((void *)(&c));
  __out_argv1__3603__.b_p = ((void *)(&b));
  __out_argv1__3603__.a_p = ((void *)(&a));
    printf("a2\n");
  GOMP_parallel_start(OUT__1__3603__,&__out_argv1__3603__,16);
  OUT__1__3603__(&__out_argv1__3603__);
    printf("a3\n");
  GOMP_parallel_end();
// END USER CODE

}


void OUT__1__3603__(void *__out_argv)
{
  printf("*** INSIDE\n");
  /*
  int **a = (int **)(((struct OUT__1__3603___data *)__out_argv) -> a_p);
  int **b = (int **)(((struct OUT__1__3603___data *)__out_argv) -> b_p);
  int **c = (int **)(((struct OUT__1__3603___data *)__out_argv) -> c_p);
  int _p_i;
  int _p_j;
  int _p_k;
  long p_index_;
  long p_lower_;
  long p_upper_;
  int _p_chunk_size;
  int _p_iter_count = (1 + (SIZE - 1 - 0)) / 1;
  int _p_num_threads = omp_get_num_threads();
  _p_chunk_size = _p_iter_count / _p_num_threads;
  int _p_ck_temp = _p_chunk_size * _p_num_threads != _p_iter_count;
  _p_chunk_size = _p_ck_temp + _p_chunk_size;
  int _p_thread_id = omp_get_thread_num();
  p_lower_ = 0 + _p_chunk_size * _p_thread_id * 1;
  p_upper_ = p_lower_ + _p_chunk_size * 1 + -1;
  p_upper_ = (p_upper_ < SIZE - 1?p_upper_ : SIZE - 1);
  for (p_index_ = p_lower_; p_index_ <= p_upper_; p_index_ += 1) {
    for (_p_k = 0; _p_k < SIZE; ++_p_k)
      for (_p_i = 0; _p_i < SIZE; ++_p_i)
        ( *c)[(p_index_ * SIZE) + _p_i] += (( *a)[(_p_k * SIZE) + _p_i] * ( *b)[(p_index_ * SIZE) + _p_k]);
  }
  */
  //GOMP_barrier();
}

void OUT__2__3603__(void *__out_argv)
{
  /*
  int **c = (int **)(((struct OUT__2__3603___data *)__out_argv) -> c_p);
  int _p_i;
  long p_index_;
  long p_lower_;
  long p_upper_;
  int _p_chunk_size;
  int _p_iter_count = (1 + ((SIZE * SIZE) - 1 - 0)) / 1;
  int _p_num_threads = omp_get_num_threads();
  _p_chunk_size = _p_iter_count / _p_num_threads;
  int _p_ck_temp = _p_chunk_size * _p_num_threads != _p_iter_count;
  _p_chunk_size = _p_ck_temp + _p_chunk_size;
  int _p_thread_id = omp_get_thread_num();
  p_lower_ = 0 + _p_chunk_size * _p_thread_id * 1;
  p_upper_ = p_lower_ + _p_chunk_size * 1 + -1;
  p_upper_ = (p_upper_ < (SIZE * SIZE) - 1?p_upper_ : (SIZE * SIZE) - 1);
  for (p_index_ = p_lower_; p_index_ <= p_upper_; p_index_ += 1) {
    ( *c)[p_index_] = 0;
  }
  */
  GOMP_barrier();
}




void omp_fn2(struct _omp_param *params)
{
  double *A = params[0].ptr;
  double *Anew = params[1].ptr;

  // START USER CODE
  int i,j;
  int iter_max = 100;
  double tol = 1.0e-6;
  double error = 1.0;
  int iter = 0;

  while(error > tol && iter < iter_max)  {
    error = 0.0;

    #pragma omp parallel for reduction(max: error)
    for(j = 1; j < SIZE-1; ++j) {
      for(i = 1; i < SIZE-1; ++i) {
        Anew[j*SIZE+i] = 0.25 * ( A[j*SIZE+i+1] + A[j*SIZE+i-1]
                            + A[(j-1)*SIZE+i] + A[(j+1)*SIZE+i]);
        error = fmax(error, fabs(Anew[j*SIZE+i] - A[j*SIZE+i]));
      }
    }

    #pragma omp parallel for
    for( j = 1; j < SIZE-1; j++) {
      for( i = 1; i < SIZE-1; i++ ) {
        A[j*SIZE+i] = Anew[j*SIZE+i];
      }
    }

    iter++;
  }
  // END USER CODE
}


#define MAX_N_TASK 2
unsigned int INSTRUCTIONS;

#define ITER 1

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

struct OUT__1__3604___data
{
  int i;
};

void OUT__1__3604__(void *__out_argv);
void OUT__2__3604__(void *__out_argv);
void OUT__3__3604__(void *__out_argv);

void omp_fn3(struct _omp_param *params)
{
  printf("ENTER fn3\n");
  //printf("Alignment of omp_lock_t = %u\n", __alignof (omp_lock_t));
  int k;
  int values[] = {(1), (10), (100), (500), (1000), (5000), (7500), (10000), (15000), (20000), (50000), (100000), (200000), (500000), (1000000), (2000000)};
  unsigned long sum = 0;
  //for (k = 0; k < 1; ++k) {
    INSTRUCTIONS = values[2];
    GOMP_parallel_start(OUT__3__3604__,0,2);
    OUT__3__3604__(0);
    GOMP_parallel_end();
  //}
  printf("Exit fn3 %lu\n",(sum / ITER));
}


void OUT__1__3604__(void *__out_argv)
{
  int i = (int )(((struct OUT__1__3604___data *)__out_argv) -> i);
  int _p_i = i;
  do_work();
  printf(">>> task %d COMPLETED\n", i);
}

void OUT__2__3604__(void *__out_argv)
{
//printf("TASK in single\n");
  int i;
  for (i = 0; i < MAX_N_TASK; i++) {
    struct OUT__1__3604___data __out_argv1__3604__;
    __out_argv1__3604__.i = i;
    printf("before GOMP_task %d\n", i);
    GOMP_task(OUT__1__3604__,&__out_argv1__3604__,0,sizeof(struct OUT__1__3604___data ),4,1,1);
    printf("task %d continuation\n", i);
  }
}

void OUT__3__3604__(void *__out_argv)
{
  if (GOMP_single_start()) {
    GOMP_task(OUT__2__3604__,0,0,0,0,1,1);
//printf("TASK in single END\n");
    printf("SINGLE end\n");
  }
  GOMP_barrier();
}




/* KERNEL POINTERS */

omp_fn omp_functions[] = { omp_fn1, omp_fn2, omp_fn3 };


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
