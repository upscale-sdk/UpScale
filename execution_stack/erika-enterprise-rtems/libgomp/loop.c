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

/* This file handles the LOOP (FOR/DO) construct.  */

#include "libgomp.h"

ALWAYS_INLINE void
gomp_loop_init(gomp_work_share_t *ws, long start, long end, long incr, long chunk_size)
{
    //printf("Processor ID %d\n", prv_proc_num);
    ws->chunk_size = chunk_size;
    /* Canonicalize loops that have zero iterations to ->next == ->end.  */
    ws->end = ((incr > 0 && start > end) || (incr < 0 && start < end)) ? start : end;
    ws->incr = incr;
    ws->next = start;
}

ALWAYS_INLINE int
gomp_loop_dynamic_next (gomp_work_share_t *ws, long *istart, long *iend)
{
    int ret;
    ret = gomp_iter_dynamic_next (ws, istart, iend);
    return ret;
}

/* The GOMP_loop_end* routines are called after the thread is told that
 *  all loop iterations are complete.  This first version synchronizes
 *  all threads; the nowait version does not.  */

ALWAYS_INLINE void
gomp_loop_end_nowait()
{
    gomp_work_share_end_nowait();
}


ALWAYS_INLINE void
gomp_loop_end()
{
    gomp_work_share_end_nowait();
    gomp_hal_barrier(0x0);
}


/*********************** APIs *****************************/

int
GOMP_loop_dynamic_start(long start, long end, long incr, long chunk_size,
                        long *istart, long *iend)
{
    int chunk, left;
    gomp_work_share_t *ws;
    int ret = 1;

    //NOTE ws return from this function already locked
    if (gomp_work_share_start(&ws))
        gomp_loop_init(ws, start, end, incr, chunk_size);

    start = ws->next;

    if (start == ws->end)
        ret = 0;

    if(ret)
    {
        chunk = chunk_size * incr;

        left = ws->end - start;
        if (incr < 0)
        {
            if (chunk < left)
                chunk = left;
        }
        else
        {
            if (chunk > left)
                chunk = left;
        }

        end = start + chunk;
        ws->next = end;
    }

    *istart = start;
    *iend   = end;

    gomp_hal_unlock (&ws->lock);
    return ret;
}

int
GOMP_loop_dynamic_next (long *istart, long *iend)
{
    int ret;
    unsigned int myid = prv_proc_num;
    ret = gomp_loop_dynamic_next ((gomp_work_share_t *) CURR_WS(myid), istart, iend);

    return ret;
}

void
GOMP_parallel_loop_dynamic_start (void (*fn) (void *), void *data,
                                  unsigned num_threads, long start, long end,
                                  long incr, long chunk_size)
{
    gomp_team_t *new_team;

    gomp_team_start (fn, data, num_threads, &new_team);
    #ifdef __OMP_SINGLE_WS__
    gomp_loop_init(new_team->work_share, start, end, incr, chunk_size);
    #else
    gomp_loop_init(new_team->work_share[prv_proc_num], start, end, incr, chunk_size);
    #endif
    MSlaveBarrier_Release(0x0, new_team->nthreads, new_team->proc_ids);
}

void
GOMP_loop_end()
{
    gomp_loop_end();
}

void
GOMP_loop_end_nowait()
{
    gomp_loop_end_nowait();
}
