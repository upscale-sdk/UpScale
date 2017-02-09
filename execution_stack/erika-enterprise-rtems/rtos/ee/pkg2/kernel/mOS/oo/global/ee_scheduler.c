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

EE_BOOL EE_schedule_data_priority_insert ( EE_SN ** pp_first,
  EE_SN * p_SN_new, EE_TDB * EE_CONST p_TDB_new )
{
  EE_BOOL              head_changed  = EE_FALSE;
  EE_SN *              p_prev        = NULL;
  EE_SN *              p_curr        = (*pp_first);

  EE_task_prio  const  new_task_prio = p_TDB_new->p_TCB->current_prio;

  /* Save the TDB on the new SN */
  p_SN_new->p_TDB = p_TDB_new;

  /* Traverse the queue until needed */
  while ( (p_curr != NULL) && (new_task_prio <= p_curr->p_TDB->p_TCB->
    current_prio) )
  {
    p_prev = p_curr;
    p_curr = p_curr->p_next;
  };

  if ( p_prev != NULL ) {
    p_prev->p_next = p_SN_new;
  } else {
    (*pp_first)  = p_SN_new;
    head_changed = EE_TRUE;
  }

  p_SN_new->p_next = p_curr;

  return head_changed;
}

static EE_TDB EE_CONST * EE_scheduler_stack_first_rq ( EE_KDB EE_CONST * p_KDB,
  EE_CDB EE_CONST * p_CDB )
{
  EE_KCB * const           p_KCB      = p_KDB->p_KCB;
  EE_CCB * const           p_CCB      = p_CDB->p_CCB;
  EE_SN  * const           p_rq_first = p_KCB->p_rq_first;
  EE_TDB EE_CONST *        p_TDB_act;

  if ( p_rq_first != NULL ) {
    EE_TCB * p_TCB_act;
    p_TDB_act                   = p_rq_first->p_TDB;
    p_TCB_act                   = p_TDB_act->p_TCB;

    /* Extract from ready queue */
    p_KCB->p_rq_first           = p_rq_first->p_next;

    /* Set as current */
    p_rq_first->p_next          = NULL;
    p_CCB->p_curr_sn            = p_rq_first;
    p_CCB->p_curr               = p_TDB_act;

    p_TCB_act->current_core_id  = EE_get_curr_core_id();
    p_TCB_act->status           = EE_TASK_RUNNING;

    /* Adjust actual priority with dispatch priority: if needed */
    if ( p_TCB_act->current_prio < p_TDB_act->dispatch_prio ) {
      p_TCB_act->current_prio = p_TDB_act->dispatch_prio;
    }
  } else {
    p_TDB_act = NULL;
  }
  return p_TDB_act;
}

EE_status_type  EE_scheduler_task_activated ( EE_KDB EE_CONST * const  p_KDB,
    EE_CDB EE_CONST * p_CDB, EE_TDB EE_CONST * p_task_activated,
    EE_BOOL preemption_point )
{
  EE_BOOL                  rq_head_changed;
  EE_status_type           status_type;
  EE_CCB *          const  p_CCB                 = p_CDB->p_CCB;
  EE_KCB EE_CONST * const  p_KCB                 = p_KDB->p_KCB;
  EE_TCB          * const  p_task_activated_tcb  = p_task_activated->p_TCB;

  EE_lock_kernel();


  if ( p_task_activated_tcb->residual_activation > 0U ) {
    if ( p_task_activated_tcb->status == EE_TASK_SUSPENDED ) {
      p_task_activated_tcb->status = EE_TASK_READY;
    }
    --p_task_activated_tcb->residual_activation;
    rq_head_changed = EE_schedule_data_priority_insert (&p_KCB->p_rq_first,
      EE_scheduler_sn_alloc(p_KCB), p_task_activated);

    if ( rq_head_changed ) {
      EE_TDB EE_CONST * p_task_running = p_CCB->p_curr;
      if ( preemption_point && (p_task_running->p_TCB->current_prio <
        p_task_activated_tcb->current_prio) )
      {
        EE_SN * const p_task_running_sn = p_CCB->p_curr_sn;
        p_task_running->p_TCB->status = EE_TASK_READY;

        (void)EE_scheduler_stack_first_rq (p_KDB, p_CDB);

        if ( p_task_running != p_CDB->p_idle_task ) {
          (void)EE_schedule_data_priority_insert ( &p_KCB->p_rq_first,
            p_task_running_sn, p_task_running );

          EE_hal_broadcast_signal ();
        }

        EE_hal_change_context(&p_task_activated->HDB,
          p_task_activated->task_func, &p_task_running->HDB);
      } else {
        EE_unlock_kernel();
        EE_hal_broadcast_signal();
      }
    } else {
      EE_unlock_kernel();
    }
    status_type = E_OK;
  } else {
    EE_unlock_kernel();
    status_type = E_OS_LIMIT;
  }
  return status_type;
}

EE_TDB EE_CONST * EE_scheduler_task_block_current ( EE_KDB EE_CONST * p_KDB,
  EE_CDB EE_CONST * p_CDB, EE_SN EE_CONST ** pp_block_queue_head )
{
  EE_KCB EE_CONST *  const  p_KCB         = p_KDB->p_KCB;
  EE_CCB          *  const  p_CCB         = p_CDB->p_CCB;
  EE_TDB EE_CONST *  const  p_curr        = p_CCB->p_curr;
  EE_SN           *  const  p_curr_sn     = p_CCB->p_curr_sn;
  EE_TDB EE_CONST *         p_TDB_next;
  EE_SN           *         p_ready_queue;

  p_curr->p_TCB->status = EE_TASK_WAITING;
  (void)EE_schedule_data_priority_insert (pp_block_queue_head, p_curr_sn,
    p_curr);

  EE_lock_kernel();

  p_ready_queue = p_KCB->p_rq_first;

  /* The task must go into the WAITING state */
  if ( p_ready_queue != NULL ) {
    p_TDB_next = EE_scheduler_stack_first_rq (p_KDB, p_CDB);
  } else {
    p_TDB_next                = p_CDB->p_idle_task;
    p_TDB_next->p_TCB->status = EE_TASK_RUNNING;
    p_CCB->p_curr             = p_TDB_next;
    p_CCB->p_curr_sn          = NULL;
  }

  return p_TDB_next;
}

EE_BOOL  EE_scheduler_task_unblocked ( EE_KDB EE_CONST * p_KDB,
  EE_CDB EE_CONST * p_CDB, EE_SN EE_CONST * p_block_queue_head )
{
  EE_BOOL rq_head_changed;
  EE_BOOL is_preemption                     = EE_FALSE;
  EE_TDB  EE_CONST * const  p_TDB_unblocked = p_block_queue_head->p_TDB;
  p_block_queue_head->p_TDB->p_TCB->status  = EE_TASK_READY;

  EE_lock_kernel();
  rq_head_changed = EE_schedule_data_priority_insert(&p_KDB->p_KCB->p_rq_first,
    p_block_queue_head, p_block_queue_head->p_TDB);
  EE_unlock_kernel();

  if ( rq_head_changed == EE_TRUE ) {
    is_preemption = (p_TDB_unblocked->p_TCB->current_prio >
      p_CDB->p_CCB->p_curr->p_TCB->current_prio);
  }

  if ( is_preemption == EE_FALSE ) {
    EE_hal_broadcast_signal ();
  }
  return is_preemption;
}

EE_TDB * EE_scheduler_task_schedule_next ( EE_KDB EE_CONST * p_KDB,
  EE_CDB EE_CONST * p_CDB )
{
  EE_KCB          *  const p_KCB       = p_KDB->p_KCB;
  EE_SN           *        p_rq_first;
  EE_TDB EE_CONST *        p_TDB_next;

  EE_lock_kernel();

  p_rq_first = p_KCB->p_rq_first;

  if ( p_rq_first != NULL ) {
    p_TDB_next = EE_scheduler_stack_first_rq(p_KDB, p_CDB);
  } else {
    EE_CCB * const p_CCB            = p_CDB->p_CCB;
    p_TDB_next                      = p_CDB->p_idle_task;
    p_TDB_next->p_TCB->status       = EE_TASK_RUNNING;
    p_CCB->p_curr                   = p_TDB_next;
    p_CCB->p_curr_sn                = NULL;
  }

  return p_TDB_next;
}

void EE_scheduler_task_preemption_point ( EE_KDB EE_CONST * const p_KDB,
  EE_CDB  EE_CONST * p_CDB )
{
  EE_KCB *          const  p_KCB         = p_KDB->p_KCB;
  EE_SN           *        p_rq_first;

  /* Lock the Scheduler */
  EE_lock_kernel();

  p_rq_first      = p_KCB->p_rq_first;

  if ( p_rq_first != NULL ) {
    EE_TCB          * const  p_rq_first_tcb  = p_rq_first->p_TDB->p_TCB;
    EE_CCB *          const  p_CCB           = p_CDB->p_CCB;
    EE_TDB EE_CONST * const  p_curr          = p_CCB->p_curr;
    EE_BOOL           const  is_preemption   = (p_rq_first_tcb->current_prio >
      p_curr->p_TCB->current_prio);

    /* If there'is preemption: schedule */
    if ( is_preemption ) {
      EE_SN           * const  p_curr_sn     = p_CCB->p_curr_sn;

      p_curr->p_TCB->status                  = EE_TASK_READY;

      /* Set the first RQ as current */
      (void)EE_scheduler_stack_first_rq (p_KDB, p_CDB);

      if ( p_curr != p_CDB->p_idle_task ) {
        (void)EE_schedule_data_priority_insert ( &p_KCB->p_rq_first,
          p_curr_sn, p_curr );
        EE_hal_broadcast_signal ();
      }
      /* mppa_tracepoint(erika,ACTIVATE_TASK_EXIT, taskId); */
      /* mppa_tracepoint(erika,ISR_HANDLER_EXIT, ev_src); */
      EE_hal_change_context(&p_rq_first->p_TDB->HDB,
        p_rq_first->p_TDB->task_func, &p_curr->HDB);
    } else {
      EE_unlock_kernel();
    }
  } else {
    EE_unlock_kernel();
  }
}

void EE_scheduler_task_set_running ( EE_KDB EE_CONST * p_KDB,
  EE_CDB  EE_CONST * p_CDB, EE_TDB EE_CONST * p_TDB )
{
  EE_CCB * const          p_CCB           = p_CDB->p_CCB;
  EE_TDB EE_CONST * const p_preempted     = p_CCB->p_curr;
  EE_SN           * const p_preempted_sn  = p_CCB->p_curr_sn;

  p_preempted->p_TCB->status    = EE_TASK_READY;
  p_TDB->p_TCB->status          = EE_TASK_RUNNING;
  p_CCB->p_curr                 = p_TDB;
  p_TDB->p_TCB->current_core_id = EE_get_curr_core_id();

  EE_lock_kernel();
  /* Alloc the SN for the new Running TASK */
  p_CCB->p_curr_sn              = EE_scheduler_sn_alloc(p_KDB->p_KCB);
  p_CCB->p_curr_sn->p_TDB       = p_TDB;

  if ( p_preempted != p_CDB->p_idle_task ) {
    (void)EE_schedule_data_priority_insert ( &p_KDB->p_KCB->p_rq_first,
      p_preempted_sn,  p_preempted );
    EE_hal_broadcast_signal ();
  }
  /* Call the HAL */
  EE_hal_change_context(&p_TDB->HDB, p_TDB->task_func,& p_preempted->HDB);
}

EE_TDB EE_CONST * EE_scheduler_task_terminated ( EE_KDB EE_CONST * p_KDB,
  EE_CDB  EE_CONST * p_CDB )
{
  EE_CCB          * const p_CCB       = p_CDB->p_CCB;
  EE_TDB EE_CONST *       p_TDB_term  = p_CCB->p_curr;

  if ( p_TDB_term != p_CDB->p_idle_task ) {
    EE_lock_kernel();
    EE_task_end(p_TDB_term);
    EE_scheduler_sn_release(p_KDB->p_KCB, p_CDB->p_CCB->p_curr_sn);
    EE_unlock_kernel();
  } else {
    p_TDB_term = NULL;
  }
  return p_TDB_term;
}
