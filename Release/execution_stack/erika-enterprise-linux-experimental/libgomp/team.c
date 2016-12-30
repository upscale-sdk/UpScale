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
   info         parallel thread team implementation */

#include "libgomp.h"

INLINE gomp_team_t *
gomp_new_team()
{
    gomp_team_t * new_team;
    new_team = (gomp_team_t *) shmalloc(sizeof(gomp_team_t));
    return new_team;
}

INLINE void
gomp_free_team(gomp_team_t * team)
{
    shfree(team);
}

/* Safely get-and-decrement at most 'specified' free processors */
INLINE int
gomp_resolve_num_threads (int specified)
{ 
  int nthr;
  
  nthr = GLOBAL_IDLE_CORES + 1;
  
  /* If a number of threads has been specified by the user
   * and it is not bigger than max idle threads use that number
   */
  if (specified && (specified < nthr))
    nthr = specified;
  
  GLOBAL_IDLE_CORES -= (nthr - 1);
  
  return nthr;
}

void
gomp_master_region_start (void *fn, void *data, int specified, gomp_team_t **team)
{
    unsigned int i, myid, local_id_gen, num_threads,
    curr_team_ptr, my_team_ptr;
    gomp_team_t *new_team;
    
    myid = prv_proc_num;
    
    curr_team_ptr = (unsigned int) CURR_TEAM_PTR(0);
    /* Proc 0 calls this... */
    my_team_ptr = curr_team_ptr;
    
    /* Fetch free processor(s) */
    GLOBAL_INFOS_WAIT();
    
    
    num_threads = gomp_resolve_num_threads (specified);
    /* Create the team descriptor for current parreg */
    new_team = gomp_new_team();
    
    new_team->omp_task_f = (void *)(fn);
    new_team->omp_args = data;
    new_team->nthreads = num_threads; // also the master
    
    new_team->team = 0xFFFF;
    
    /* Use the global array to fetch idle cores */
    local_id_gen = 1; // '0' is master
    
    num_threads--; // Decrease NUM_THREADS to account for the master
    new_team->thread_ids[myid] = 0;
    new_team->proc_ids[0] = myid;
    
    // to make SW barrier work
    for(i=1; i<prv_num_procs; i++)
    {
        new_team->proc_ids[local_id_gen] = i;
        new_team->thread_ids[i] = local_id_gen++;
    }
    GLOBAL_INFOS_SIGNAL();
    new_team->level = 0;
    
    new_team->parent = 0x0;
    *((gomp_team_t **) my_team_ptr) = new_team;
    *team = new_team;

}

ALWAYS_INLINE void
gomp_team_start (void *fn, void *data, int specified, gomp_team_t **team) 
{
    unsigned int i, nprocs, myid, local_id_gen, num_threads,
    curr_team_ptr, my_team_ptr;
    unsigned /*long long*/ int mask;
    gomp_team_t *new_team, *parent_team;
    
    nprocs = prv_num_procs;
    myid = prv_proc_num;
    
    curr_team_ptr = (unsigned int) CURR_TEAM_PTR(0);
    my_team_ptr = (unsigned int) CURR_TEAM_PTR(myid);
        
    /* Fetch free processor(s) */
    GLOBAL_INFOS_WAIT();
    
    num_threads = gomp_resolve_num_threads (specified);
    
    /* Create the team descriptor for current parreg */
    new_team = gomp_new_team();
    new_team->omp_task_f = (void *)(fn);
    new_team->omp_args = data;
    new_team->nthreads = num_threads; // also the master
    
    new_team->team = 0x0;
    
    /* Use the global array to fetch idle cores */
    local_id_gen = 1; // '0' is master
    
    num_threads--; // Decrease NUM_THREADS to account for the master
    new_team->team |= (1 << myid);
    new_team->thread_ids[myid] = 0;
    new_team->proc_ids[0] = myid;
    
    /* Init team-specific locks */
    gomp_hal_init_lock(&new_team->atomic_lock);
    gomp_hal_init_lock(&new_team->critical_lock);
    
    /* Init default work share */  
    gomp_work_share_t *root_ws = (gomp_work_share_t *) gomp_new_work_share();
    
    #ifdef __OMP_SINGLE_WS__
    new_team->work_share = root_ws;
    #else
    new_team->work_share[myid] = root_ws;
    #endif
    
    unsigned int *gtpool = (unsigned int *) (GLOBAL_INFOS_BASE);
    
    for( i=1, mask = 2, curr_team_ptr += 4; /* skip p0 (global master) */
         i<nprocs && num_threads;
         i++, mask <<= 1, curr_team_ptr += 4) 
    {       
        if(!( *gtpool & mask))
        {
            *gtpool |= mask;
            
            new_team->team |= mask;
            
            new_team->proc_ids[local_id_gen] = i;
            
            new_team->thread_ids[i] = local_id_gen++;
            
            /* Update local team structure pointers of all processors of the team */
            *((gomp_team_t **) curr_team_ptr) = new_team;

            #ifndef __OMP_SINGLE_WS__
            new_team->work_share[i] = root_ws;
            #endif
            
            /* How many left? */
            num_threads--;
        } // if
    } // for
    
    GLOBAL_INFOS_SIGNAL();
    
    /* Update the parent team field */
    parent_team = *(gomp_team_t **) my_team_ptr;
    
    new_team->level = parent_team->level + 1;
    
    new_team->parent = parent_team;
    *((gomp_team_t **) my_team_ptr) = new_team;
    *team = new_team;
    
#ifdef TASKING_ENABLED
    unsigned int j, pid, nthreads;
    gomp_thread_t *thread;
    
    new_team->task_q.task_count = 0;
    new_team->pending_threads = 0;
    gomp_hal_init_lock((omp_lock_t *) &new_team->task_lock);
    gomp_hal_init_lock((omp_lock_t *) &new_team->sleep_lock);
    gomp_hal_lock((omp_lock_t *) &new_team->sleep_lock);
    //new_team->task_lock = &new_team->_task_lock;
    //new_team->_task_lock = 0x0U;
    //new_team->sleep_lock = &new_team->_sleep_lock;
    //new_team->_sleep_lock = 0xFFFFFFFFU;
    
    gomp_task_t *task, *implicit_task;
    nthreads = new_team->nthreads;

    unsigned int task_mem_per_thread = sizeof(gomp_task_t)*MAX_TASKS_FOR_THREAD;
    for(i = 0; i < nthreads; i++)
    {
        pid = new_team->proc_ids[i];
        thread = &(new_team->thread[pid]);
        thread->undeferred_task = NULL;
        thread->deferred_task = NULL;

        task = malloc(task_mem_per_thread);
        thread->free_task_list = task;
        for(j = 0; j < MAX_TASKS_FOR_THREAD; j++)
            task[j].next_free_mem = &task[j+1];
        task[j-1].next_free_mem = NULL;

        //thread->mem_lock = &thread->_mem_lock;
        //thread->_mem_lock = 0x0U;
        gomp_hal_init_lock((omp_lock_t *) &thread->mem_lock);

        implicit_task = task_malloc(thread);
        implicit_task->child_waiting_q.task_count = 0;
        implicit_task->child_running_q.task_count = 0;
        implicit_task->kind = GOMP_TASK_IMPLICIT;
        //implicit_task->taskwait_barrier = &implicit_task->_taskwait_barrier;
        //implicit_task->_taskwait_barrier  = 0xFFFFFFFFU;
        gomp_hal_init_lock((omp_lock_t *) &implicit_task->taskwait_barrier);
        gomp_hal_lock((omp_lock_t *) &implicit_task->taskwait_barrier);
        thread->implicit_task = implicit_task;
        thread->curr_task = implicit_task;
    }
#endif // TASKING_ENABLED

}

/* End team and destroy team descriptor */
ALWAYS_INLINE void
gomp_team_end()
{
    unsigned int neg_mask, myid, nthreads;
    gomp_team_t *the_team;
    myid = prv_proc_num;
    
    the_team = (gomp_team_t *) CURR_TEAM(myid);
    
    neg_mask =~ the_team->team;
    
    neg_mask |= (1 << myid);
    nthreads = the_team->nthreads;

#ifdef TASKING_ENABLED
    int i;
    gomp_thread_t *thread;
    unsigned int pid;

    for(i = 0; i < nthreads; i++)
    {
        pid = the_team->proc_ids[i];
        thread = &(the_team->thread[pid]);
        task_free(thread->implicit_task);
        shfree(thread->free_task_list);
    }
#endif
    
    GLOBAL_INFOS_WAIT();
    GLOBAL_IDLE_CORES += (nthreads - 1); // free all but master
    GLOBAL_THREAD_POOL &= neg_mask;
    
    GLOBAL_INFOS_SIGNAL();
    
    /* After this, the current parallel thread will be lost. Free...if we had any for MPARM */
    CURR_TEAM(myid) = the_team->parent;
    
    #ifdef __OMP_SINGLE_WS__
    gomp_free_work_share(the_team->work_share);
    #else
    gomp_free_work_share(the_team->work_share[myid]);
    #endif
    
    gomp_free_team(the_team);
    
} // gomp_team_end
