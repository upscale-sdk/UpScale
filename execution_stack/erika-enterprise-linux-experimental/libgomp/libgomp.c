/* Copyright (C) 2005-2014 Free Software Foundation, Inc.
 C ontributed by Richard Henderson <r*th@redhat.com>.

 This file is part of the GNU OpenMP Library (libgomp).

 Libgomp is free software; you can redistribute it and/or modify it
 under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3, or (at your option)
 any later version.

 Libgomp is distributed in the hope that it will be useful, but WITHOUT ANY
 WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 more details.

 Under Section 7 of GPL version 3, you are granted additional
 permissions described in the GCC Runtime Library Exception, version
 3.1, as published by the Free Software Foundation.

 You should have received a copy of the GNU General Public License and
 a copy of the GCC Runtime Library Exception along with this program;
 see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
 <http://www.gnu.org/licenses/>.  */

/* Copyright 2014 DEI - Universita' di Bologna
   author       DEI - Universita' di Bologna
                Alessandro Capotondi - alessandro.capotondi@unibo.it
        Giuseppe Tagliavini - giuseppe.tagliavini@unibo.it
   info         Libgomp main entry point */

#include "libgomp.h"
#include "ee.h"
#include <time.h>
#include <sys/time.h>
//#include <mppaipc.h>
#include "comm_layer.h"

static long long zeroTag;
extern char * memory[MAX_OFFLOADS_PER_CLUSTER];
static char * offloadArgs[MAX_OFFLOADS_PER_CLUSTER];

static void omp_initenv(int, int);
static void omp_SPMD_worker(int, omp_fn, struct _omp_param *, unsigned int slot_id);

extern void mOS_dcache_disable(void);

/* main routine */
void
GOMP_main (void *args)
{

  EE_k1_omp_message *offload = (EE_k1_omp_message *) args;
  omp_fn _app_main = offload->func;
  struct _omp_data_msg *data_msg = (struct _omp_data_msg *)offload->params;
  char *global_memory = data_msg->memory_area;
  unsigned int slot_id = offload->slot_id;
  offloadArgs[slot_id] = args;
  struct _omp_param *_app_args = (struct _omp_param *)data_msg->function_args;
  gomp_mem_ptr[slot_id] = global_memory;

  JobType job_id;
  GetJobID ( &job_id );
  job_2_slot_id[job_id] = slot_id;

  int id    = prv_proc_num;
  int procs = prv_num_procs;

  /* The MASTER executes omp_initenv().. */
  #ifdef LIBGOMP_MAIN_VERBOSE
  printf("[OpenMP] booting thread %d of %d\n",id, procs);
  #endif
  if (id == MASTER_ID)
  {

    #ifdef LIBGOMP_MAIN_VERBOSE
    printf("********* TASKMEM_LOCK_ADDR %x\n", TASKMEM_LOCK_ADDR);
    printf("********* CONTEXTMEM_LOCK_ADDR %x\n", CONTEXTMEM_LOCK_ADDR);

    printf("##### REQUIRED MEMORY = %p \n", CONTEXT_FREE_LIST_ADDR-gomp_mem_ptr[slot_id]);

    printf("gomp_mem_ptr GLOBAL_INFOS_BASE = %x %x\n", gomp_mem_ptr[slot_id], GLOBAL_INFOS_BASE);
    #endif

    #ifdef LIBGOMP_MAIN_HEADER
    printf("\n");
    printf("----------------------------------------------\n");
    printf("        OpenMP runtime START\n");
    printf("----------------------------------------------\n");
    printf("\n");
    printf("Default max procs:\t%d\n", DEFAULT_MAXPROC);
    printf("Memory area:\t%p\n", global_memory);
    #endif
    omp_initenv(procs, id);
  }
  omp_SPMD_worker(id, _app_main, _app_args, slot_id);

  return;
}

/* omp_initenv() - initialize environment & synchronization constructs */
static void
omp_initenv(int nprocs, int pid)
{
  int i;
  gomp_team_t * root_team;

  //shmalloc_init(STATIC_TCDM_SIZE + sizeof(int));

  /* Init preallocated ws pool*/
  WSMEM_FREE_MEM = (void *)((unsigned int) WSMEM_ADDR);
  WSMEM_FREE_LIST = 0;

  //Reset workshare descriptor pool lock
  gomp_hal_init_lock(WSMEM_LOCK);

  GLOBAL_IDLE_CORES = nprocs - 1;
  GLOBAL_THREAD_POOL = (1 << MASTER_ID);
  gomp_hal_init_lock(GLOBAL_LOCK);

  for(i=0; i<nprocs; ++i)
      CURR_TEAM(i) = (gomp_team_t *) NULL;

  #ifdef LIBGOMP_MAIN_VERBOSE
  printf("GLOBAL_IDLE_CORES:\t%d\n", GLOBAL_IDLE_CORES);
  printf("GLOBAL_THREAD_POOL:\t%x\n", GLOBAL_THREAD_POOL);
  for(i=0; i<nprocs; ++i)
      printf("CURR_TEAM(%d):\t%p\t%p\n", i, CURR_TEAM(i), &CURR_TEAM(i));
  #endif

  gomp_master_region_start (NULL, NULL, 1, &root_team);
}

/* parallel execution */

// FIXME is this address likely to map any .code section?
#define OMP_SLAVE_EXIT 0xdeadbeef

extern int input_data_portal_fd[];
extern int output_data_portal_fd[];
extern unsigned char * current_output_addr[];
extern unsigned long current_output_size[];
/* omp_SPMD_worker() - worker threads spin until work provided via GOMP_parallel_start() */
static void
omp_SPMD_worker(int myid, omp_fn _app_main, struct _omp_param *_app_args, unsigned int slot_id)
{
  /* For slaves */
  volatile task_f  * omp_task_f;
  volatile int **omp_args;
  int i, nprocs;

  //_printstrp("omp_SPMD_worker");

  nprocs = prv_num_procs;

  if (myid == MASTER_ID)
  {
    MSlaveBarrier_Wait(0x0, nprocs, (unsigned int *) CURR_TEAM(myid)->proc_ids);

     _app_main(_app_args);

    for(i=1; i<nprocs; i++)
      CURR_TEAM(i) = (gomp_team_t *) OMP_SLAVE_EXIT;

    /* We release slaves inside gomp_parallel_end() */
    MSlaveBarrier_Release(0x0, nprocs, (unsigned int *) CURR_TEAM(myid)->proc_ids);

    // Copy back data
    unsigned char *outputBuffer = current_output_addr[slot_id];
    unsigned int outputSize = current_output_size[slot_id];;
    if(current_output_size[slot_id] == 0) {
      outputBuffer = &zeroTag;
      outputSize = sizeof(zeroTag);
    }
    EE_k1_mb();
    comm_tx_write_eot(output_data_portal_fd[slot_id], outputBuffer, outputSize, 0);

    free(_app_args);
    free(CURR_TEAM(myid));

    #ifdef LIBGOMP_MAIN_HEADER
    printf("\n");
    printf("----------------------------------------------\n");
    printf("        OpenMP runtime STOP\n");
    printf("----------------------------------------------\n");
    printf("\n");
    #endif

  } // MASTER
  else
  {
    MSlaveBarrier_SlaveEnter(0x0, myid);

    while (1){
      /* Exit runtime loop... */
      if ( (volatile unsigned int) CURR_TEAM(myid) ==  OMP_SLAVE_EXIT) {
        // we are done!!
        break;
      }

      /* Have work! */
      else {
        omp_task_f = (void*) (&CURR_TEAM(myid)->omp_task_f);
        omp_args = (void*) (&CURR_TEAM(myid)->omp_args);
        (**omp_task_f)((int) *omp_args);
      } // ! omp_task_f

     #ifdef TASKING_ENABLED
     gomp_task_scheduler();
     #endif

      MSlaveBarrier_SlaveEnter(0x0, myid);
    }
  }

  return;
} // omp_worker

/******************************************************************************/

int
omp_get_num_procs(void)
{
  return prv_num_procs;
}

