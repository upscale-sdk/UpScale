/* Copyright (C) 2010, 2017 DEI - Universita' di Bologna
   Contributed by:
   Alessandro Capotondi <alessandro.capotondi@unibo.it>
   Daniele Cesarini <daniele.cesarini@unibo.it>
   Andrea Marongiu  <a.marongiu@unibo.it>
   Giuseppe Tagliavini <giuseppe.tagliavini@unibo.it>
*/

/* Copyright (C) 2005, 2009 Free Software Foundation, Inc.
   Contributed by Richard Henderson <rth@redhat.com>.

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

/* This file handles the maintainence of threads in response to team
   creation and termination.  */

#include "libgomp.h"

INLINE gomp_team_t *
gomp_new_team()
{
    gomp_team_t * new_team;
    new_team = (gomp_team_t *) shmalloc(sizeof(gomp_team_t));
    //printf("****************************** new team = %d\n", new_team);
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
gomp_master_region_start ( __attribute__((unused)) void *fn,
                           __attribute__((unused)) void *data,
                           __attribute__((unused)) int specified,
                                                   gomp_team_t **team)
{
    uint32_t i, nprocs;
    gomp_team_t *new_team;

    nprocs    = prv_num_procs; //FIXME change it to function get_vnum_procs()
    
    /* Create the team descriptor for current parreg */
    new_team = gomp_new_team();

    new_team->nthreads = nprocs;
    new_team->team = 0xFFU;

    for (i = 0; i < nprocs; ++i)
        new_team->proc_ids[i] = i;
    
    /* Update Team */
    new_team->level  = 0x0U;
    new_team->parent = ( gomp_team_t * ) NULL;
    CURR_TEAM(prv_proc_num) = new_team;

    #ifdef OMP_TEAM_DEBUG
    printf("[%d][%d][gomp_master_region_start] New Team: 0x%08x (%d threads, Level %d), check 0x%08x\n", 0,
           prv_proc_num, new_team, nprocs, new_team->level, CURR_TEAM(prv_proc_num));
    #endif
    *team = new_team;
}

INLINE void
gomp_team_start (void *fn, void *data, int specified, gomp_team_t **team) 
{
    //unsigned long long start1 = usecs();
    uint32_t i, nprocs, pid, local_id_gen, num_threads;
    uint32_t curr_team_ptr, mask;
    gomp_team_t *new_team, *lru_team, *parent_team;
    
    pid = prv_proc_num; //FIXME change it to function get_vproc_id()
        
    /* Fetch free processor(s) */
    GLOBAL_INFOS_WAIT();
    
    num_threads = gomp_resolve_num_threads (specified);
    
    /* Create the team descriptor for current parreg */
    new_team = gomp_new_team();
    //gomp_assert(new_team != (gomp_team_t *) NULL);

    /* Check Level */
    parent_team      = CURR_TEAM(pid);
    new_team->level  = parent_team->level + 1;
    new_team->parent = parent_team;

    new_team->omp_task_f = (void *)(fn);
    new_team->omp_args   = data;

    #ifdef OMP_TEAM_DEBUG
    printf("[%d][%d][gomp_team_start] New Team: 0x%08x (%d threads, Level %d), Fn 0x%08x, Args 0x%08x, parent 0x%08x (Level %d)\n", 0,
           prv_proc_num, new_team, num_threads, new_team->level, new_team->omp_task_f, new_team->omp_args, parent_team, parent_team->level);
    #endif
    
    /* Init team-specific locks */
    gomp_hal_init_lock(&new_team->atomic_lock);
    gomp_hal_init_lock(&new_team->critical_lock);
    
#ifdef TASK_CORE_STATIC_SCHED
	for(i=0;i<MAX_THREADS;i++) {
    	new_team->thread[i].task_q.task_count = 0;
    	gomp_hal_init_lock((omp_lock_t *) &new_team->thread[i].sleep_lock);
	}
#endif
   
    /* Init default work share */  
    gomp_work_share_t *root_ws = &(new_team->root_ws);
    
    /*Reset the locks */
    gomp_hal_init_lock(&root_ws->lock);
    gomp_hal_init_lock(&root_ws->enter_lock);
    gomp_hal_init_lock(&root_ws->exit_lock);
    root_ws->embedded = WS_EMBEDDED;
    root_ws->next_ws = NULL;
    root_ws->prev_ws = NULL;
    
    #ifdef __OMP_SINGLE_WS__
    new_team->work_share = &new_team->root_ws;
    #else
    new_team->work_share[pid] = &new_team->root_ws;
    #endif

    // lru_team = gomp_get_lru_team();
    // if(new_team->level == 0x1U &&
    //    lru_team == new_team   &&
    //    lru_team->nthreads >= num_threads)
    // {
    //     //LRU team hit
    //     new_team->team = __getBitmask(num_threads);
    //     new_team->nthreads = num_threads;
    //     gomp_alloc_thread_pool(new_team->team);

    //     #ifdef OMP_TEAM_DEBUG  
    //     printf("[%d][%d][gomp_team_start] HIT: Level: %d, LRU: 0x%08x (%d threads), NewTeam: 0x%08x (%d threads - bitmask 0x%08x)\n", get_cl_id(),
    //            get_proc_id(), new_team->level, lru_team, lru_team->nthreads, new_team, new_team->nthreads, new_team->team);
    //     #endif
    // }
    // else
    // {
        //LRU team miss
        new_team->team = 0x0U;
        new_team->nthreads = num_threads;
        nprocs = prv_num_procs;

        /* Use the global array to fetch idle cores */
        local_id_gen = 1; // '0' is master
        
        num_threads--; // Decrease NUM_THREADS to account for the master
        new_team->team |= (1 << pid);
        new_team->thread_ids[pid] = 0;
        new_team->proc_ids[0] = pid;
        //unsigned long long end1 = usecs();
        //printf("TEAM ALLOC TIME = %llu\n", (end1-start1)*250);

        //unsigned long long start2 = usecs();
        unsigned int *gtpool = (unsigned int *) (GLOBAL_INFOS_BASE);        
        for( i=1, mask = 2, curr_team_ptr = (uint32_t) CURR_TEAM_PTR(1); /* skip p0 (global master) */
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

                /* How many left? */
                num_threads--;
            } // if
        } // for

        #ifdef OMP_TEAM_DEBUG  
        printf("[%d][%d][gomp_team_start] MISS: Level: %d, LRU: 0x%08x (%d threads), NewTeam: 0x%08x (%d threads - bitmask 0x%08x)\n", get_cl_id(),
               get_proc_id(), new_team->level, lru_team, lru_team->nthreads, new_team, new_team->nthreads, new_team->team);
        #endif
    // }
    GLOBAL_INFOS_SIGNAL();
    
    /* Update the parent team field */
    CURR_TEAM(pid) = new_team;
    *team = new_team;
    //unsigned long long end2 = usecs();
    //printf("THREAD RECRUIT TIME = %llu\n", (end2-start2)*250);


    //unsigned long long start3 = usecs();
#ifdef TASKING_ENABLED
    // unsigned int j, pid, nthreads;
    gomp_thread_t *thread;
    
    new_team->task_q.task_count = 0;
    new_team->pending_threads = 0;
    gomp_hal_init_lock((omp_lock_t *) &new_team->task_lock);
    gomp_hal_init_lock((omp_lock_t *) &new_team->sleep_lock);
    gomp_hal_lock((omp_lock_t *) &new_team->sleep_lock);
    
    gomp_task_t *task, *implicit_task;

    thread = &(new_team->thread[pid]);
    thread->undeferred_task = NULL;
    thread->deferred_task = NULL;
    //thread->tot = 0;

    new_team->tasks = (gomp_task_t *)shmalloc(DEFAULT_MAXPROC*MAX_TASKS_FOR_THREAD*sizeof(gomp_task_t));
    task =  &new_team->tasks[pid*MAX_TASKS_FOR_THREAD];
    thread->free_task_list = task;
    for(i = 0; i < MAX_TASKS_FOR_THREAD; i++)
        task[i].next_free_mem = &task[i+1];
    task[i-1].next_free_mem = NULL;

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
#endif // TASKING_ENABLED
     //unsigned long long end3 = usecs();
     //printf("TASK STUFF TIME = %llu\n", (end3-start3)*250);


}

/* End team and destroy team descriptor */
INLINE void
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
/*
    int i;
    gomp_thread_t *thread;
    unsigned int pid;

    for(i = 0; i < nthreads; i++)
    {
        pid = the_team->proc_ids[i];
        thread = &(the_team->thread[pid]);
        //printf("TIME THREAD %d = %llu\n", pid, thread->tot);
        if(thread->implicit_task)
          task_free(thread->implicit_task);
        shfree(thread->free_task_list);
    }
*/
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
    
    shfree(the_team->tasks);
    gomp_free_team(the_team);
    
} // gomp_team_end
