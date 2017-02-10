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

/* This file handles the SINGLE construct.  */

#include "libgomp.h"

void *
gomp_single_copy_start(gomp_work_share_t *ws)
{
    GOMP_WARN_NOT_SUPPORTED("#pragma omp single copyprivate");
    return NULL;
}

void
gomp_single_copy_end(gomp_work_share_t *ws, void *data)
{
    GOMP_WARN_NOT_SUPPORTED("#pragma omp single copyprivate");
}

/**************** APIs **************************/

int
GOMP_single_start(void)
{
    int ret = 0;
    unsigned int myid = prv_proc_num;
    
    gomp_team_t *team = (gomp_team_t *) CURR_TEAM(myid);
    gomp_work_share_t *ws;
    
    //NOTE ws return from this function already locked
    ret = gomp_work_share_start(&ws);
    
    /* Faster than a call to gomp_work_share_end_nowait() */
    ws->completed++;
    
    if (ws->completed == team->nthreads)
    {
        ws->checkfirst = WS_NOT_INITED;
        
        #ifndef __OMP_SINGLE_WS__
        gomp_free_work_share(ws->prev_ws);
        #endif
    }
    gomp_hal_unlock(&ws->lock);
    return ret;
}

void *
GOMP_single_copy_start(void)
{
    GOMP_WARN_NOT_SUPPORTED("#pragma omp single copyprivate");
    return NULL;
}

void
GOMP_single_copy_end(void *data)
{
    GOMP_WARN_NOT_SUPPORTED("#pragma omp single copyprivate");
    return;
}
