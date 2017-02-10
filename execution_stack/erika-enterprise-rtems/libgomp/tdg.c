/* Copyright (C) 2015, 2017 Barcelona Supercomputing Center (BSC)
							DEI - Universita' di Bologna
   Contributed by:
   Eduardo Quiñones <eduardo.quinones@bsc.es>
   Roberto Vargas <roberto.vargas@bsc.es>
*/

/* This file is part of the GNU OpenMP Library (libgomp).

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

/* This file handles the maintainence of the task dependency graph (TDG).  */


#include <stdio.h>
#include "libgomp.h"
#include <sys/time.h>
#include <time.h>

//#define DEBUG_GOMP_MAP

// Maximum nesting level considered by the static TDG mechanism
#define MAX_NESTING_LOOP 10
#define MAX_NESTING_TDGs 10

// Information included within the application's address space
extern struct gomp_tdg_t *gomp_tdg[]; // Application's TDG structure
extern unsigned short *gomp_tdg_ins[]; // Task instance IDs input dependencies 
extern unsigned short *gomp_tdg_outs[]; // Task instance IDs output dependencies 

extern unsigned gomp_tdg_ntasks[]; // Number of tasks in the TDG
extern unsigned gomp_maxI[]; // Maximum number of iterations of any loop containing #pragma omp tasks
extern unsigned gomp_maxT[]; // Number of tasks constructs (#pragma omp tasks) of the program

extern unsigned gomp_num_tdgs; // Total number of TDGs included in the program

// Defined in task.c
extern void gomp_ready(struct gomp_task *task);

static int nested_loop_stack[MAX_NESTING_LOOP];
static int nls_i=0;

static int nested_tdg_id_stack[MAX_NESTING_TDGs];
static int ntis_i=0;

int curr_tdg=0;

void 
dump_gomp_tdg(void)
{
    int i;
 
    printf("TDG structure: #Nodes= %d, Number of pragma tasks=%d\n", gomp_tdg_ntasks[curr_tdg], gomp_maxT[curr_tdg]);
    for (i = 0; i < gomp_tdg_ntasks[curr_tdg]; ++i)
        printf("\t(@%p) gomp_tdg[%d][%d].id=%lu, cnt=%d\n",&gomp_tdg[curr_tdg][i],curr_tdg,i,gomp_tdg[curr_tdg][i].id,(int)gomp_tdg[curr_tdg][i].cnt);

    printf("Loop stack structure: Max iterations = %d\n",gomp_maxI[curr_tdg]);
    for(i = 0; i < nls_i; i++)
        printf("\tNesting level %d, iteration %d\n",i,nested_loop_stack[i]);
}

void 
gomp_fatal (char *msg, int par)
{
    printf("%s %d\n",msg, par);
    dump_gomp_tdg();
    abort();
}

// Computes the task instance ID to index the TDG
unsigned long 
gomp_task_instance_id(unsigned task_id)
{
    int i;
    unsigned long task_instance_id = 0;

    for(i=nls_i-1;i>=0;i--)
    {
        task_instance_id += nested_loop_stack[i];
        task_instance_id *= gomp_maxI[curr_tdg];
    }
    task_instance_id *= gomp_maxT[curr_tdg];
    task_instance_id += task_id;
    
    return task_instance_id;
}

// Dicotomic search of task instance id within the TDG structure
int 
tdg_index_lookup(unsigned long id)
{
    int low = 0;
    int high = gomp_tdg_ntasks[curr_tdg]-1;
	int mid;
    
    while (low <= high) 
    {
        mid = low + ((high - low) / 2);
        if (id == gomp_tdg[curr_tdg][mid].id)
            return mid;
        else if (id < gomp_tdg[curr_tdg][mid].id)
            high = mid - 1;
        else
            low = mid + 1;
    }
    return -1;
}

int
get_tdg_index(unsigned id)
{
    unsigned long t_id = gomp_task_instance_id(id);
    int tdg_index = tdg_index_lookup(t_id);

    if (tdg_index == -1) 
        gomp_fatal("[gomp_get_tdg_index]: task not found ",t_id);

	return tdg_index;
}

int
gomp_register_new_task_in_tdg(unsigned id)
{
    int i;
    struct gomp_tdg_t *tdg, *dp;
    int tdg_index = get_tdg_index(id);

    tdg = &gomp_tdg[curr_tdg][tdg_index];
    if (tdg->cnt >= 0)
        gomp_fatal("[gomp_register_new_task_in_tdg]: Depend counter (cnt) not initialized ",tdg->id);

	tdg->task = NULL;
    // Check the number of tasks upon which the new task depend
    tdg->cnt =0;
    for (i = 0; i < tdg->nin; ++i) 
    {
        dp = &gomp_tdg[curr_tdg][gomp_tdg_ins[curr_tdg][tdg->offin + i]];
        if (dp->cnt >= 0)
            ++tdg->cnt;
    }
#ifdef DEBUG_GOMP_MAP
    printf("[gomp_register_new_task_in_tdg] - Registering dependencies of Task ID %lu with %d (from %d) unresolved dependencies (tdg_index %d)\n",
           tdg->id,tdg->cnt,tdg->nin,tdg_index);
#endif

	return tdg_index;
}

void
gomp_set_tdg_to_task (struct gomp_task *task, int tdg_index)
{
	if(tdg_index < 0 || tdg_index >= gomp_tdg_ntasks[curr_tdg])
		gomp_fatal("[gomp_set_tdg_to_task]: TDG index out of range ",tdg_index);

    if (gomp_tdg[curr_tdg][tdg_index].cnt < 0)
        gomp_fatal("[gomp_set_tdg_to_task]: Dependencies not registered; task id ",gomp_tdg[curr_tdg][tdg_index].id);

    task->tdg = &gomp_tdg[curr_tdg][tdg_index];
    gomp_tdg[curr_tdg][tdg_index].task = task;

#ifdef DEBUG_GOMP_MAP
    printf("[gomp_set_tdg_to_task] - Task %d registered to TDG index %d\n", task->tdg->id,tdg_index);
#endif
}

int
gomp_get_unresolved_dependencies (int tdg_index)
{
	if(tdg_index < 0 || tdg_index >= gomp_tdg_ntasks[curr_tdg])
		gomp_fatal("[gomp_get_unresolved_dependencies]: TDG index out of range ",tdg_index);

	int n_dep=(int)(gomp_tdg[curr_tdg][tdg_index].cnt);

#ifdef DEBUG_GOMP_MAP
	if(!n_dep)
    	printf("[gomp_get_unresolved_dependencies] - Task id %lu (tdg[%d][%d]) ready to execute\n",gomp_tdg[curr_tdg][tdg_index].id,curr_tdg,tdg_index);
	else
    	printf("[gomp_get_unresolved_dependencies] - Task id %lu (tdg[%d][%d]) with %d dependencies unsolved. Waiting...\n",gomp_tdg[curr_tdg][tdg_index].id,curr_tdg,tdg_index,n_dep);
#endif
	return(n_dep);
}

void
gomp_deltask_dep(struct gomp_task *task)
{
    int i;
    struct gomp_tdg_t *dp, *tdg;

#ifdef DEBUG_GOMP_MAP
    printf("[gomp_deltask_dep] -Task ID %lu finished\n", task->tdg->id);
#endif

    if ((tdg = task->tdg) == NULL) {
        if  (task->kind == GOMP_TASK_IMPLICIT)
            return;
        gomp_fatal("gomp_deltask_dep: task not found",task->tdg->id);
    }
    if (tdg->cnt < 0)
        gomp_fatal("task destroyed when it was not active",tdg->id);

	
    tdg->cnt = -1;
    // Decrementing the counter of tasks with output dependencies
    // Acquiring the team->task_lock is not needed as this function is called from gomp_finalize function, which is already
    // protected with a lock
    for (i = 0; i < tdg->nout; ++i) {
        dp = &gomp_tdg[curr_tdg][gomp_tdg_outs[curr_tdg][tdg->offout + i]];
        if (dp->cnt == 0) 
            gomp_fatal("deltask: descendant was running while it shouldn't",dp->id);
        
        if (dp->cnt > 0) {
            // In case a dependent task has all its input dependencies solved move it to the ready queue
            if (--dp->cnt == 0 && dp->task) {
#ifdef DEBUG_GOMP_MAP
    			printf("[gomp_deltask_end] - Task id %lu ready to execute (cnt %d)\n",dp->task->tdg->id,dp->task->tdg->cnt);
#endif
                gomp_ready(dp->task);
			}
        }
    }
}

/***************** Time tracing support for execution time measure of tasks **********************/
long get_execution_time (void)
{
   struct timeval t;
   gettimeofday(&t,NULL);
   return t.tv_sec*1000000+t.tv_usec;
}

void
GOMP_start_tracepoint_task()
{
  	unsigned int pid = prv_proc_num;
  	gomp_team_t *team = (gomp_team_t *) CURR_TEAM(pid);
  	gomp_thread_t *thread = &team->thread[pid];
  	struct gomp_tdg_t *tdg = thread->curr_task->tdg;

    if (tdg->cnt != 0 || tdg->task_counter) {
		printf("[GOMP_start_tracepoint_task]: In task %d, cnt %d, task_counter %d (*tdg %p)\n",tdg->id,tdg->cnt,tdg->task_counter,tdg);
        gomp_fatal("[GOMP_start_tracepoint_task]: Task already created without starting the counter or counter already intialized",tdg->id);
	}

	tdg->task_counter = get_execution_time();
}

void
GOMP_stop_tracepoint_task()
{
  	unsigned int pid = prv_proc_num;
  	gomp_team_t *team = (gomp_team_t *) CURR_TEAM(pid);
  	gomp_thread_t *thread = &team->thread[pid];
  	gomp_task_t *exe_task = thread->curr_task;
	struct gomp_tdg_t *tdg = exe_task->tdg;

    if (tdg->task_counter == 0)
        gomp_fatal("[GOMP_stop_tracepoint_task]: Counter not initialized",tdg->id);

	long current_time = get_execution_time();
    //gomp_tdg[curr_tdg][tdg_index].task_counter_end = current_time;
    tdg->task_counter = current_time - tdg->task_counter;
}

// The instrumentation of the run-time overhead only is valid if tasks are not preempted (e.g. no nested parallelism)
void
GOMP_start_tracepoint_runtime (unsigned int id)
{
    int tdg_index = get_tdg_index(id);
	
    if (gomp_tdg[curr_tdg][tdg_index].cnt >= 0 || gomp_tdg[curr_tdg][tdg_index].runtime_counter) {
		printf("[GOMP_start_tracepoint_runtime]: Static ID %d; unique ID %d; runtime_counter %d\n",id,tdg_index,gomp_tdg[curr_tdg][tdg_index].runtime_counter);
        gomp_fatal("[GOMP_start_tracepoint_runtime]: Task already created without starting the counter or counter already intialized",gomp_tdg[curr_tdg][tdg_index].id);
	}

	gomp_tdg[curr_tdg][tdg_index].runtime_counter = get_execution_time();
}

void
GOMP_stop_tracepoint_runtime (unsigned int id)
{
    int tdg_index = get_tdg_index(id);

    if (gomp_tdg[curr_tdg][tdg_index].runtime_counter == 0)
        gomp_fatal("[GOMP_stop_tracepoint_runtime]: Counter not initialized",gomp_tdg[curr_tdg][tdg_index].id);

	long current_time = get_execution_time();
    gomp_tdg[curr_tdg][tdg_index].runtime_counter = current_time - gomp_tdg[curr_tdg][tdg_index].runtime_counter;
    if(current_time > gomp_tdg[curr_tdg][tdg_index].task_counter_end)
    	gomp_tdg[curr_tdg][tdg_index].runtime_counter -= gomp_tdg[curr_tdg][tdg_index].task_counter;
}

void
GOMP_print_task_counters()
{
	int i;
	for (i = 0; i < gomp_tdg_ntasks[curr_tdg]; ++i) {
        printf("[%d,%d] = R:%d,T:%d\n",curr_tdg,(int)gomp_tdg[curr_tdg][i].id,(int)gomp_tdg[curr_tdg][i].runtime_counter,(int)gomp_tdg[curr_tdg][i].task_counter);
		gomp_tdg[curr_tdg][i].task_counter = 0;
		gomp_tdg[curr_tdg][i].task_counter_end = 0;
		gomp_tdg[curr_tdg][i].runtime_counter = 0;
	}
}

void
GOMP_start_tracepoint_taskpart()
{
}

void
GOMP_stop_tracepoint_taskpart()
{
}

/***************** Control Flow Identification API for task instance id computation **********************/
void
GOMP_push_loop(void)
{
    if (nls_i == MAX_NESTING_LOOP)
        gomp_fatal("nested_loop_stack overflow",nls_i);
    nested_loop_stack[nls_i++]=0;
}

void
GOMP_pop_loop(void)
{
    if (!nls_i)
        gomp_fatal("nested_loop_stack underflow",nls_i);
    nls_i--;
}

void
GOMP_inc_loop(void)
{
    nested_loop_stack[nls_i-1]++;
}

/***************** TDG API for determining the TDG currently executed **********************/
void
GOMP_set_tdg_id(unsigned int new_tdg_id)
{
	if(new_tdg_id > gomp_num_tdgs)
		gomp_fatal("[GOMP_set_tdg_id]: new tdg_id out of range ",new_tdg_id);
	if(ntis_i == MAX_NESTING_TDGs)
		gomp_fatal("[GOMP_set_tdg_id]: nested_tdg_id_stack overflow",ntis_i);
	curr_tdg = nested_tdg_id_stack[ntis_i++] = new_tdg_id;

	// Init TDG counters
	int i;
	for (i = 0; i < gomp_tdg_ntasks[curr_tdg]; ++i) {
		gomp_tdg[curr_tdg][i].task_counter = 0;
		//gomp_tdg[curr_tdg][i].task_counter_end = 0;
		//gomp_tdg[curr_tdg][i].runtime_counter = 0;
	}
	
#ifdef DEBUG_GOMP_MAP
    printf("[GOMP_set_tdg_id] - Setting a new TDG identifier %d [%d]\n", curr_tdg, ntis_i);
#endif
}

void
GOMP_unset_tdg_id(void)
{
    if (!ntis_i)
        gomp_fatal("[GOMP_unset_tdg_id]: nested_tdg_id_stack underflow",ntis_i);
    ntis_i--;

	// Reset counters
	int i;
	for (i = 0; i < gomp_tdg_ntasks[curr_tdg]; ++i) {
		gomp_tdg[curr_tdg][i].task_counter = 0;
		gomp_tdg[curr_tdg][i].task_counter_end = 0;
		gomp_tdg[curr_tdg][i].runtime_counter = 0;
	}

	curr_tdg = nested_tdg_id_stack[ntis_i];

#ifdef DEBUG_GOMP_MAP
    printf("[GOMP_unset_tdg_id] - Setting back the TDG identifier %d\n", curr_tdg);
#endif
}
