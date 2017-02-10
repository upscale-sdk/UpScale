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

/* This file contains routines to manage the work-share queue for a team
   of threads.  */

#include "libgomp.h"

#ifdef __OMP_SINGLE_WS__
#warning You compiled the library with __OMP_SINGLE_WS__ so without support for nowait.
#endif

#define WS_INITED       ( 0xfeeddeadU )
#define WS_NOT_INITED   ( 0x0U )

#define WS_EMBEDDED     ( 0xfeeddeadU )
#define WS_NOT_EMBEDDED ( 0x0U )

/* Allocate a new work share */
ALWAYS_INLINE gomp_work_share_t *
alloc_work_share ()
{
    gomp_work_share_t *res;

    #ifndef __NO_OMP_PREALLOC__
    WSMEM_LOCK_WAIT();
    res = (gomp_work_share_t *) WSMEM_FREE_MEM;
    if((unsigned int) res < (unsigned int)WSMEM_LIMIT)
        WSMEM_FREE_MEM += sizeof(gomp_work_share_t);
    else
    {
        res = WSMEM_FREE_LIST;
        if(res)
            WSMEM_FREE_LIST = res->next_free;
        else
        {
            #ifndef __NO_OMP_MALLOC_ON_PREALLOC__
            res = (gomp_work_share_t *)shmalloc(sizeof(gomp_work_share_t));
            printf("[LIBGOMP] Warning. Dynamic WS allocation needed.");
            #else
            printf("[LIBGOMP] Error. WS MEMORY is out of Memory. Max workshare allowed is", MAX_WS);
            abort();
            #endif
        }
    }
    WSMEM_LOCK_SIGNAL();
    #else
    res = (gomp_work_share_t *)shmalloc(sizeof(gomp_work_share_t));
    #endif
    return res;
}

/* Free a work share struct, put it into current team's free gomp_work_share list.  */
ALWAYS_INLINE void
gomp_free_work_share (gomp_work_share_t *ws)
{
    #ifndef __NO_OMP_PREALLOC__
    if ( ws->embedded == WS_NOT_EMBEDDED )
    {
        WSMEM_LOCK_WAIT();
        ws->next_free = WSMEM_FREE_LIST;
        WSMEM_FREE_LIST = ws;
        WSMEM_LOCK_SIGNAL();
    }
    #endif
}

ALWAYS_INLINE gomp_work_share_t
*gomp_new_work_share()
{
    gomp_work_share_t *new_ws;

    new_ws = alloc_work_share();

    /*Reset the locks */
    gomp_hal_init_lock(&new_ws->lock);
    gomp_hal_init_lock(&new_ws->enter_lock);
    gomp_hal_init_lock(&new_ws->exit_lock);
    new_ws->embedded = WS_NOT_EMBEDDED;
    new_ws->next_ws = NULL;
    new_ws->prev_ws = NULL;

    return new_ws;
}

ALWAYS_INLINE int
gomp_work_share_start (gomp_work_share_t **new_ws)
{
    unsigned int ret = 0;
    unsigned int myid = prv_proc_num;

    #ifdef __OMP_SINGLE_WS__
    /*Single WS */
    *new_ws = (gomp_work_share_t *) CURR_WS(myid);

    gomp_hal_lock(&(*new_ws)->lock); //Acquire the ws lock
    if ((*new_ws)->checkfirst != WS_INITED)
    {
        /* This section is performed only by first thread of next new_ws*/
        (*new_ws)->checkfirst = WS_INITED;

        (*new_ws)->completed = 0;
        ret = 1;
    }

    #else

    /*Multiple WS*/
    gomp_work_share_t *curr_ws = (gomp_work_share_t *) CURR_WS(myid);

    gomp_hal_lock(&curr_ws->lock); //Acquire the curr_ws lock

    *new_ws = curr_ws->next_ws;

    /* Check if new ws is already allocated */
    if(*new_ws == NULL)
    {
        /* This section is performed only by first thread of next ws*/
        *new_ws = gomp_new_work_share();
        (*new_ws)->prev_ws = curr_ws;
        curr_ws->next_ws = *new_ws;
        (*new_ws)->completed = 0;
        ret = 1;
    }

    gomp_hal_lock(&(*new_ws)->lock); //acquire new ws lock
    gomp_hal_unlock(&curr_ws->lock); //release curr ws lock

    CURR_WS(myid) = *new_ws; //update curr ws pointer

    #endif

    return ret;
}

ALWAYS_INLINE void
gomp_work_share_end_nowait ()
{
    unsigned int myid = prv_proc_num;
    gomp_team_t *team = (gomp_team_t *) CURR_TEAM(myid);
    gomp_work_share_t *ws;

    # ifdef __OMP_SINGLE_WS__
    ws = team->work_share;
    # else
    ws = team->work_share[myid];
    # endif

    gomp_hal_lock(&ws->lock);
    ws->completed++;

    if (ws->completed == team->nthreads)
    {
        #ifndef __OMP_SINGLE_WS__
        //NOTE at this point no more threads point to ws->prev_ws because you
        // are sure that everyone point to ws. Thus you can free ws->prev_ws
        if(ws->prev_ws)
            gomp_free_work_share(ws->prev_ws);

        #else
        ws->checkfirst = WS_NOT_INITED;
        #endif
    }

    gomp_hal_unlock(&ws->lock);
}

