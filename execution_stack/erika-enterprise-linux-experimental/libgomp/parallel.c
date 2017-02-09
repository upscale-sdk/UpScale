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
   info         #pragma omp parallel implementation */

#include "libgomp.h"

void
GOMP_parallel_start (void *fn, void *data, int num_threads)
{
    /* The thread descriptor for slaves of the newly-created team */
    gomp_team_t *new_team;  
    
    gomp_team_start (fn, data, num_threads, &new_team);

  MSlaveBarrier_Release(0x0, new_team->nthreads, new_team->proc_ids);
}

void
GOMP_parallel_end (void)
{
  unsigned int myid;
  gomp_team_t *the_team;

  myid = prv_proc_num;
  the_team = (gomp_team_t *) CURR_TEAM(myid);

#ifdef TASKING_ENABLED
  gomp_task_scheduler();
#endif
  MSlaveBarrier_Wait(0x0, the_team->nthreads, the_team->proc_ids);

  gomp_team_end();
}

void
GOMP_parallel (void (*fn) (void*), void *data, int num_threads, unsigned int flags)
{
    /* The thread descriptor for slaves of the newly-created team */
    gomp_team_t *new_team;  

    gomp_team_start (fn, data, num_threads, &new_team);

	MSlaveBarrier_Release(0x0, new_team->nthreads, new_team->proc_ids);
    fn(data);

    GOMP_parallel_end();
}

/* The public OpenMP API for thread and team related inquiries.  */

int
omp_get_num_threads()
{
  unsigned int myid = prv_proc_num;
  return CURR_TEAM(myid)->nthreads;
}

int
omp_get_max_threads(void)
{
  return GLOBAL_IDLE_CORES + 1;
}

int
omp_get_thread_num()
{
  unsigned int myid = prv_proc_num;
  return CURR_TEAM(myid)->thread_ids[myid];
}

int
omp_in_parallel()
{
  unsigned int myid = prv_proc_num;
  return CURR_TEAM(myid)->level != 0;
}

