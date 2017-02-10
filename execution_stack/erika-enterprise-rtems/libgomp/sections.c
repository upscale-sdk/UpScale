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

/* This file handles the SECTIONS construct.  */

#include "libgomp.h"

ALWAYS_INLINE void
gomp_sections_init (gomp_work_share_t *ws, int count)
{
    ws->end = count + 1;
    ws->next = 1;
    ws->chunk_size = 1;
    ws->incr = 1;
}

ALWAYS_INLINE int
gomp_sections_next (gomp_work_share_t *ws)
{
    long s, e, ret;
    
    if (gomp_iter_dynamic_next (ws, &s, &e))
        ret = s;
    else
        ret = 0;
    
    return ret;
}

ALWAYS_INLINE void
gomp_sections_end_nowait ()
{
    gomp_work_share_end_nowait ();
}

ALWAYS_INLINE void
gomp_sections_end ()
{
  gomp_sections_end_nowait ();

#ifdef TASKING_ENABLED
  gomp_task_scheduler();
#endif

  gomp_hal_barrier(0x0);
}

/**************** APIs **************************/
int
GOMP_sections_start (int count)
{
    int ret = 0;
    gomp_work_share_t *ws;
    
    //NOTE ws return from this function already locked
    if (gomp_work_share_start(&ws))
        gomp_sections_init (ws, count);
    
    if (ws->next != ws->end)
        ret = ws->next++;
    
    gomp_hal_unlock (&ws->lock);
    return ret;
}

void
GOMP_sections_end()
{
    gomp_sections_end();
}

void
GOMP_sections_end_nowait()
{
    gomp_sections_end_nowait();
}
int
GOMP_sections_next()
{
    unsigned int myid = prv_proc_num;
    return gomp_sections_next((gomp_work_share_t *) CURR_WS(myid));
}

void
GOMP_parallel_sections_start (void (*fn) (void *), void *data, unsigned num_threads, unsigned count)
{
    gomp_team_t *new_team;
    
    gomp_team_start (fn, data, num_threads, &new_team);
    #ifdef __OMP_SINGLE_WS__
    gomp_sections_init (new_team->work_share, count);
    #else
    gomp_sections_init (new_team->work_share[prv_proc_num], count);
    #endif
    
    MSlaveBarrier_Release(0x0, new_team->nthreads, new_team->proc_ids);
    
}
