/*
 * ee_scheduler_entry_points.c
 *
 *  Created on: 07/dic/2014
 *      Author: AmministratoreErrico
 */

#include "eecfg.h"
#include "ee_hal_internal.h"
#include "ee_scheduler.h"
#include "ee_kernel_types.h"
#include "ee_get_kernel_and_core.h"

void EE_scheduler_task_not_terminated ( void ) {
  EE_CDB * p_CDB;
  EE_TDB * p_terminated;
  EE_hal_disableIRQ();
  p_CDB = EE_lock_and_get_curr_core();
  p_terminated = EE_scheduler_task_terminated(p_CDB);
  mppa_tracepoint(erika,TERMINATE_TASK_ENTER, p_terminated->tid);
  EE_hal_terminate_activation(p_terminated->p_HDB, &EE_scheduler_task_end);
}

void EE_scheduler_task_end ( void ) {
  EE_TDB * p_To;
  EE_TDB * p_From;
  EE_CDB * p_CDB  = EE_get_curr_core();

  p_To = EE_scheduler_task_schedule_next(p_CDB, &p_From);
  if ( p_To == NULL ) {
    /* Idle TASK Terminated: reactivate it */
    p_To = p_From;
    EE_scheduler_task_set_running(p_CDB, p_To);
  }

  EE_hal_activation_terminated(p_From->p_HDB);
  EE_unlock_curr_core();
  mppa_tracepoint(erika,TERMINATE_TASK_EXIT, p_From->tid);
  if ( p_From->task_type != EE_TASK_TYPE_ISR2 ) {
    EE_hal_change_context(p_To->p_HDB, p_To->task_func, p_From->p_HDB);
  } else {
    EE_hal_change_context_from_isr2(p_To->p_HDB, p_To->task_func,
      p_From->p_HDB);
  }
}

