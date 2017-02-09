/*
 * ee_basic_scheduler.c
 *
 *  Created on: 06/dic/2014
 *      Author: AmministratoreErrico
 */

#include "eecfg.h"
#include "ee_scheduler.h"
#include "ee_api_types.h"
#include "ee_get_kernel_and_core.h"
#include "ee_kernel.h"
#include <assert.h>

static EE_BOOL EE_schedule_data_priority_insert ( EE_KDB EE_CONST * p_KDB,
  EE_schedule_data p_schedule_data[], EE_array_index * p_free_sd,
  EE_array_index * p_first, EE_TDB * EE_CONST p_TDB_new )
{
  EE_BOOL              head_changed  = EE_FALSE;
  if ( (p_KDB != NULL) && (p_schedule_data != NULL) && (p_first != NULL) &&
    (p_TDB_new != NULL) )
  {
    EE_array_index       curr_index    = (*p_first);
    EE_array_index       prev_index    = INVALID_INDEX;

    /* Traverse the queue until needed */
    if ( p_KDB->tdb_array != NULL ) {
      while ( curr_index != INVALID_INDEX ) {
        EE_schedule_data * const p_sd_tmp = &p_schedule_data[curr_index];
        if ( p_sd_tmp != NULL ) {
          EE_TDB EE_CONST * const p_TDB_tmp = &p_KDB->tdb_array[p_sd_tmp->tid];

          if ( p_TDB_new->ready_prio <= p_TDB_tmp->ready_prio ) {
            prev_index = curr_index;
            curr_index = p_sd_tmp->next;
          } else {
            break;
          }
        } else {
          assert(EE_FALSE);
          break;
        }
      }
    }

    if ( p_free_sd != NULL ) {
      EE_array_index const free_sd_index = (*p_free_sd);
      EE_array_index const next_free_sd  = p_schedule_data[free_sd_index].next;

      p_schedule_data[free_sd_index].tid  = p_TDB_new->tid;
      p_schedule_data[free_sd_index].next = curr_index;

      if ( prev_index == INVALID_INDEX ) {
        (*p_first)    = free_sd_index;
        head_changed  = EE_TRUE;
      } else {
        p_schedule_data[prev_index].next = free_sd_index;
      }
      (*p_free_sd) = next_free_sd;
    }
  }
  return head_changed;
}

static EE_TDB EE_CONST * EE_scheduler_stack_first_rq ( EE_KDB EE_CONST * p_KDB,
  EE_CDB EE_CONST * p_CDB )
{
  EE_TDB EE_CONST * p_TDB_act = NULL;
  if ( (p_KDB != NULL) && (p_KDB->tdb_array) && (p_CDB->sd_array) &&
    (p_CDB != NULL) )
  {
    EE_CCB * const p_CCB = p_CDB->p_CCB;
    if ( p_CCB != NULL ) {
      EE_array_index     const new_stk_index = p_CCB->rq_first;
      EE_schedule_data * const p_sd_to_stk   = &p_CDB->sd_array[new_stk_index];
      if ( (new_stk_index != INVALID_INDEX) && (p_sd_to_stk != NULL) ) {
        p_CCB->rq_first    = p_sd_to_stk->next;
        p_sd_to_stk->next  = p_CCB->stk_first;
        p_CCB->stk_first   = new_stk_index;
        p_CCB->curr_tid    = p_sd_to_stk->tid;
        p_TDB_act          = &p_KDB->tdb_array[p_sd_to_stk->tid];
      }
    }
  }
  return p_TDB_act;
}

EE_BOOL  EE_scheduler_task_activated ( EE_CDB EE_CONST * p_CDB,
    EE_TDB EE_CONST * p_task_activated )
{
  EE_BOOL         rq_head_changed;
  EE_BOOL         need_rescheduling = EE_FALSE;
  EE_KDB EE_CONST * const  p_KDB    = EE_get_kernel();

  if ( (p_KDB != NULL) && (p_CDB != NULL) ) {
    EE_CCB *          const  p_CCB             = p_CDB->p_CCB;

    /* Post-decrement activation to check with max num of activation for status
       switching */
    if ( p_task_activated->p_TCB->residual_activation-- ==
      p_task_activated->max_num_of_act )
    {
      p_task_activated->p_TCB->status = EE_TASK_READY;
    }

    rq_head_changed = EE_schedule_data_priority_insert (p_KDB, p_CDB->sd_array,
      &p_CCB->free_SD_index, &p_CCB->rq_first, p_task_activated);

    if ( rq_head_changed ) {
      EE_TDB EE_CONST * p_task_running = &p_KDB->tdb_array[p_CCB->curr_tid];
      if ( p_task_running->p_TCB->current_prio <
        p_task_activated->p_TCB->current_prio )
      {
        need_rescheduling = EE_TRUE;
      }
    }
  } else {
    assert(EE_FALSE);
  }
  return need_rescheduling;
}

EE_BOOL EE_scheduler_task_current_is_preempted ( EE_CDB  EE_CONST * p_CDB )
{
  EE_KDB * const  p_KDB             = EE_get_kernel();
  EE_CCB * const  p_CCB             = p_CDB->p_CCB;
  EE_BOOL         is_preemption     = EE_FALSE;

  if ( (p_KDB != NULL) && (p_CCB != NULL) &&  (p_CDB->sd_array != NULL) ) {
    EE_TID const rq_first_tid = p_CDB->sd_array[p_CCB->rq_first].tid;
    EE_TID const curr_tid     = p_CCB->curr_tid;
    if ( rq_first_tid != INVALID_TASK_ID ) {
      EE_TDB EE_CONST * const p_rq_head = &p_KDB->tdb_array[rq_first_tid];
      EE_TDB EE_CONST * const p_current = &p_KDB->tdb_array[curr_tid];

      if ( (p_rq_head != NULL) && (p_current != NULL) &&
         (p_current->p_TCB != NULL) )
      {
        is_preemption = (p_rq_head->ready_prio >
          p_current->p_TCB->current_prio);
      }
    }
  } else {
    assert(EE_FALSE);
  }
  return is_preemption;
}

void EE_scheduler_task_set_running ( EE_CDB  EE_CONST * p_CDB,
  EE_TDB EE_CONST * p_TDB )
{
  if (  (p_CDB != NULL) && (p_TDB != NULL) ) {
    EE_CCB * const p_CCB = p_CDB->p_CCB;
    if ( p_CCB != NULL ) {
      EE_array_index     const sd_free_index = p_CCB->free_SD_index;
      EE_schedule_data * const p_sd_free     = &p_CDB->sd_array[sd_free_index];
      EE_TID             const tid            = p_TDB->tid;

      p_TDB->p_TCB->status  = EE_TASK_RUNNING;
      p_CCB->curr_tid       = tid;
      p_CCB->free_SD_index  = p_sd_free->next;
      p_sd_free->next       = p_CCB->stk_first;
      p_CCB->stk_first      = sd_free_index;
      p_sd_free->tid        = tid;
    }
  }
}

EE_TDB EE_CONST * EE_scheduler_task_terminated ( EE_CDB  EE_CONST * p_CDB ) {
  EE_TDB EE_CONST * p_TDB_term = NULL;
  if ( p_CDB != NULL ) {
    EE_KDB EE_CONST * const p_KDB = EE_get_kernel();
    EE_CCB          * const p_CCB = p_CDB->p_CCB;
    if ( (p_KDB != NULL) && (p_KDB->tdb_array != NULL) && (p_CCB != NULL) ) {
      EE_TID const curr_tid = p_CCB->curr_tid;
      p_TDB_term     = &p_KDB->tdb_array[curr_tid];
      EE_task_end(p_TDB_term);
    }
  }
  return p_TDB_term;
}

EE_TDB EE_CONST * EE_scheduler_task_dispatch ( EE_CDB  EE_CONST * p_CDB ) {
  EE_KDB EE_CONST * const p_KDB = EE_get_kernel();
  EE_TDB EE_CONST * p_in_activation = NULL;
  if ( (p_KDB != NULL) && (p_KDB->tdb_array != NULL) && (p_CDB != NULL) )
  {
    EE_CCB * const p_CCB = p_CDB->p_CCB;
    if ( p_CCB != NULL ) {
      EE_TDB EE_CONST * p_old_running = &p_KDB->tdb_array[p_CCB->curr_tid];
      if ( (p_old_running != NULL) && (p_old_running->p_TCB != NULL) ) {
        p_old_running->p_TCB->status = EE_TASK_READY;
      }
    }
    p_in_activation = EE_scheduler_stack_first_rq (p_KDB, p_CDB);
    if ( (p_in_activation != NULL) && (p_in_activation->p_TCB != NULL) ) {
      p_in_activation->p_TCB->current_prio   = p_in_activation->dispatch_prio;
    }
  }
  return p_in_activation;
}

/* !!! hereunder no more  MISRA !!! */

EE_TDB EE_CONST * EE_scheduler_task_block_current ( EE_CDB EE_CONST * p_CDB,
  EE_array_index * p_block_queue_head )
{
  EE_KDB EE_CONST *  const  p_KDB    = EE_get_kernel();
  EE_CCB          *  const  p_CCB    = p_CDB->p_CCB;
  EE_TDB EE_CONST *  const  p_TDB_blocked = &p_KDB->tdb_array[p_CCB->curr_tid];
  EE_array_index            old_stk  = p_CCB->stk_first;
  EE_schedule_data * const  p_blocked_sd = &p_CDB->sd_array[old_stk];
  EE_array_index     const  next_stk = p_blocked_sd->next;
  EE_array_index     const  rq_first = p_CCB->rq_first;
  EE_TDB EE_CONST *         p_TDB_act;

  /* The Idle Task is a basic TASK: cannot block so even thought I popped one
   * from stacked I'm sure it's still not empty */

  /* Give the node to the API data structure */
  (void)EE_schedule_data_priority_insert (p_KDB, p_CDB->sd_array,
    &old_stk, p_block_queue_head, p_TDB_blocked);

  /* The task must go into the WAITING state */
  p_TDB_blocked->p_TCB->status       = EE_TASK_WAITING;
  p_TDB_blocked->p_TCB->current_prio = p_TDB_blocked->ready_prio;
  p_blocked_sd->next = INVALID_INDEX;

  if ( ( rq_first == INVALID_INDEX ) ||
    (p_KDB->tdb_array[next_stk].p_TCB->current_prio >=
     p_KDB->tdb_array[rq_first].ready_prio) )
  {
    /* Change the current tid */
    p_CCB->stk_first = next_stk;
    p_CCB->curr_tid  = p_CDB->sd_array[next_stk].tid;
    p_TDB_act = &p_KDB->tdb_array[p_CCB->curr_tid];
  } else {
    p_TDB_act = EE_scheduler_stack_first_rq ( p_KDB, p_CDB );
  }
  p_TDB_act->p_TCB->status = EE_TASK_RUNNING;
  return p_TDB_act;
}

EE_BOOL  EE_scheduler_task_unblocked ( EE_CDB EE_CONST * p_CDB,
  EE_array_index * p_block_queue_head )
{
  EE_KDB EE_CONST  * const p_KDB = EE_get_kernel();
  EE_schedule_data * p_unblocked_sd = &p_CDB->sd_array[*p_block_queue_head];
  return EE_schedule_data_priority_insert(p_KDB, p_CDB->sd_array,
    p_block_queue_head, &p_CDB->p_CCB->rq_first,
    &p_KDB->tdb_array[p_unblocked_sd->tid]);
}

EE_TDB * EE_scheduler_task_schedule_next ( EE_CDB EE_CONST * p_CDB,
  EE_TDB ** pp_term_task )
{
  EE_KDB EE_CONST   * const p_KDB          = EE_get_kernel();
  EE_CCB            * const p_CCB          = p_CDB->p_CCB;
  EE_array_index      const free_index     = p_CCB->stk_first;
  EE_schedule_data  * const p_free_sd      = &p_CDB->sd_array[free_index];
  EE_schedule_data  * const p_rq_head_sd   = (p_CCB->rq_first != INVALID_INDEX)?
    &p_CDB->sd_array[p_CCB->rq_first]: NULL;
  EE_array_index      const next_stk       = p_free_sd->next;
  EE_schedule_data  * const p_next_stk_sd  = (next_stk != INVALID_TASK_ID)?
    &p_CDB->sd_array[next_stk]: NULL;
  EE_TDB EE_CONST   * const p_TDB_next_stk = (p_next_stk_sd != NULL)?
    &p_KDB->tdb_array[p_next_stk_sd->tid]: NULL;
  EE_TDB EE_CONST   * p_TDB_act;
  /* Free the first in Stacked Queue and get the new head */

  /* Return the old head if required */
  if ( pp_term_task != NULL ) {
    (*pp_term_task) = &p_KDB->tdb_array[p_free_sd->tid];
  }

  /* Free the node */
  p_CCB->stk_first     = next_stk;
  p_CCB->curr_tid      = (p_next_stk_sd != NULL)? p_next_stk_sd->tid:
    INVALID_TASK_ID; /* <-- Not Needed, just for debug */
  p_free_sd->next      = p_CCB->free_SD_index;
  p_free_sd->tid       = INVALID_TASK_ID; /* <-- Not Needed, just for debug */
  p_CCB->free_SD_index = free_index;

  if ( (p_rq_head_sd != NULL) && ((p_TDB_next_stk == NULL) ||
    (p_TDB_next_stk->p_TCB->current_prio <
       p_KDB->tdb_array[p_rq_head_sd->tid].ready_prio)) )
  {
    /* The head of RQ will be the head of STKQ after the following call */
    p_TDB_act = EE_scheduler_stack_first_rq(p_KDB, p_CDB);
  } else {
    p_CCB->curr_tid = p_TDB_next_stk->tid;
    p_TDB_act = p_TDB_next_stk;
  }
  p_TDB_act->p_TCB->status = EE_TASK_RUNNING;
  return p_TDB_act;
}
