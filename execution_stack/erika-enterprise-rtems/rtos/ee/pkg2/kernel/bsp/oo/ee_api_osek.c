/*
 * ee_osek_api.c
 *
 *  Created on: 07/dic/2014
 *      Author: AmministratoreErrico
 */

#include "eecfg.h"
#include "ee_hal_internal.h"
#include "ee_kernel.h"
#include "ee_get_kernel_and_core.h"
#include <assert.h>

StatusType StartOS ( TaskFunc idleTaskFunc ) {
  StatusType        status_type;
  EE_FREG    const  flags   = EE_hal_begin_nested_primitive();
  EE_CORE_ID const  core_id = EE_lock_and_get_curr_core_id();
  EE_CDB *   const  p_CDB = EE_get_core(core_id);
  EE_CCB *   const  p_CCB = p_CDB->p_CCB;

  EE_lock_kernel();
  EE_hal_init();

  if ( !p_CCB->os_started ) {
    TaskType idleTaskId;
    EE_TDB * p_TDB;

    status_type = EE_create_task(core_id, &idleTaskId, EE_TASK_TYPE_IDLE,
      idleTaskFunc, 0U, 0U, 1U, SYSTEM_STACK);
    EE_unlock_kernel();

    if ( status_type == E_OK ) {
      p_TDB = &EE_get_kernel()->tdb_array[idleTaskId];
      p_TDB->p_TCB->residual_activation = 0U;
      /* Insert the TDB in stacked list */
      EE_scheduler_task_set_running(p_CDB, p_TDB);
      p_CDB->p_CCB->os_started = EE_TRUE;

      EE_unlock_curr_core();

      if ( idleTaskFunc != NULL ) {
        EE_hal_change_context(p_TDB->p_HDB, p_TDB->task_func, NULL);
      } else {
        EE_hal_activation_started(p_TDB->p_HDB);
      }
    } else {
      EE_unlock_curr_core();
      assert(false);
    }
    EE_hal_enableIRQ();
  } else {
    EE_unlock_kernel();
    EE_unlock_curr_core();
    EE_hal_end_nested_primitive(flags);
    status_type = E_OS_ACCESS;
  }
  return status_type;
}

StatusType ActivateTask (TaskType taskId ) {
  StatusType        status_type;
  EE_KDB *   const  p_KDB = EE_get_kernel();
  EE_TDB *   const  p_Act = &p_KDB->tdb_array[taskId];
  EE_FREG    const  flags = EE_hal_begin_nested_primitive();

  if ( taskId >= p_KDB->p_KCB->free_TDB_index ) {
     status_type = E_OS_ID;
  } else
  if ( p_Act->orig_core != EE_get_curr_core_id() ) {
    mppa_tracepoint(erika,ACTIVATE_TASK_ENTER, taskId);
    /* TODO: Handle Remote Activation */
    mppa_tracepoint(erika,ACTIVATE_TASK_EXIT, taskId);

    status_type = E_OK;
  } else
  {
    EE_CDB *       const  p_CDB        = EE_lock_and_get_curr_core();

    if ( p_Act->p_TCB->residual_activation == 0U ) {

      EE_unlock_curr_core();
      status_type = E_OS_LIMIT;
    } else {
      EE_TDB * p_From;

      mppa_tracepoint(erika,ACTIVATE_TASK_ENTER, taskId);

      p_From = &p_KDB->tdb_array[p_CDB->p_CCB->curr_tid];
      if ( EE_scheduler_task_activated(p_CDB, p_Act) &&
        (p_From->task_type != EE_TASK_TYPE_ISR2) )
      {
        (void)EE_scheduler_task_dispatch(p_CDB);

        EE_unlock_curr_core();
        mppa_tracepoint(erika,ACTIVATE_TASK_EXIT, taskId);

        EE_hal_change_context(p_Act->p_HDB, p_Act->task_func,
          p_From->p_HDB );
      } else {
        EE_unlock_curr_core();
        mppa_tracepoint(erika,ACTIVATE_TASK_EXIT, taskId);
      }
      status_type = E_OK;
    }
  }
  EE_hal_end_nested_primitive(flags);
  return status_type;
}

StatusType TerminateTask ( void ) {
  EE_FREG  const flags        = EE_hal_begin_nested_primitive();
  EE_CDB * const p_CDB        = EE_lock_and_get_curr_core();
  EE_TDB * const p_terminated = EE_scheduler_task_terminated(p_CDB);

  mppa_tracepoint(erika,TERMINATE_TASK_ENTER, p_terminated->tid);

  EE_hal_terminate_activation(p_terminated->p_HDB, &EE_scheduler_task_end);
  EE_hal_end_nested_primitive(flags);
  return E_OK; /* <-- FAKE! */
}
