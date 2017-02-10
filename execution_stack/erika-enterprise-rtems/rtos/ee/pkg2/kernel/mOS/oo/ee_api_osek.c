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

static void EE_idle_task_wrapper ( void ) {
  EE_CDB * const  p_CDB  = EE_get_curr_core();
  EE_TDB * const  p_idle_TDB  = p_CDB->p_idle_task;

  while ( p_CDB->p_CCB->os_started ) {
    TaskFunc        idle_task_func  = p_idle_TDB->task_func;
    if ( idle_task_func ) {
      idle_task_func();
    }
  }
  EE_hal_terminate_idle_task(&p_idle_TDB->HDB);
}

StatusType StartOS ( AppModeType Mode ) {
  StatusType           status_type  = E_OK;
  EE_CDB      * const  p_CDB        = EE_get_curr_core();
  EE_CCB      * const  p_CCB        = p_CDB->p_CCB;
  EE_FREG       const  flags        = EE_hal_begin_nested_primitive();

  EE_lock_kernel();

  EE_hal_init();

  if ( !p_CCB->os_started ) {
    EE_TDB * const p_idle_TDB     = p_CDB->p_idle_task;
    TaskFunc const idle_task_func = p_CDB->p_idle_task->task_func;

    /* Insert the TDB in stacked list */
    p_CDB->p_CCB->os_started = EE_TRUE;
    /* Fill CCB */
    p_CCB->p_curr            = p_idle_TDB;

    /* Set Idle loop as started to handle Autostart TASKs */
    EE_hal_activation_started(&p_idle_TDB->HDB);

    /* Handle autostart TASK here */

    EE_unlock_kernel();

    /* Schedule Here */

    if ( idle_task_func != NULL ) {
      EE_hal_start_idle_task ( &p_idle_TDB->HDB, EE_idle_task_wrapper );
      /* Handle Shutdown Here */
    }
    status_type = E_OK;
  } else {
    EE_unlock_kernel();
    status_type = E_OS_ACCESS;
  }

  EE_hal_end_nested_primitive(flags);

  return status_type;
}

StatusType ActivateTask (TaskType taskId ) {
  StatusType                 status_type;
  EE_KDB *            const  p_KDB = EE_get_kernel();
  EE_FREG             const  flags = EE_hal_begin_nested_primitive();

#if (defined(EE_API_DYNAMIC))
  if ( taskId >= p_KDB->p_KCB->free_task_index ) {
     status_type = E_OS_ID;
  } else
#else
  if ( taskId >= p_KDB->tdb_array_size ) {
     status_type = E_OS_ID;
  } else
#endif /* EE_API_DYNAMIC */
  {
    EE_CDB EE_CONST *  const  p_CDB  = EE_get_curr_core();
    EE_CCB          *  const  p_CCB  = p_CDB->p_CCB;
    EE_TDB EE_CONST *  const  p_From = p_CCB->p_curr;
    EE_TDB EE_CONST *  const  p_Act = &p_KDB->tdb_array[taskId];

    if ( p_From->task_type != EE_TASK_TYPE_ISR2 ) {
      status_type = EE_scheduler_task_activated(p_KDB, p_CDB, p_Act, EE_TRUE);
    } else {
      status_type = EE_scheduler_task_activated(p_KDB, p_CDB, p_Act, EE_FALSE);
    }
  }
  EE_hal_end_nested_primitive(flags);
  return status_type;
}

StatusType TerminateTask ( void ) {
  EE_FREG  const flags        = EE_hal_begin_nested_primitive();
  EE_CDB * const p_CDB        = EE_get_curr_core();
  EE_TDB * const p_terminated = EE_scheduler_task_terminated(EE_get_kernel(),
    p_CDB);

  if ( p_terminated != NULL ) {
    mppa_tracepoint(erika,TERMINATE_TASK_ENTER, p_terminated->tid);

    EE_hal_terminate_activation(&p_terminated->HDB, &EE_scheduler_task_end);
  }
  EE_hal_end_nested_primitive(flags);
  return E_OS_CALLEVEL;
}

StatusType ShutdownOS ( StatusType error ) {
  StatusType      status;
  EE_FREG   const flags   = EE_hal_begin_nested_primitive();
  EE_CDB  * const p_CDB   = EE_get_curr_core();

  if ( p_CDB->p_CCB->os_started ) {
    p_CDB->p_CCB->os_started = EE_FALSE;
    /* Used to propagate the error to the ShutdownHook */
    p_CDB->p_CCB->last_error = error;

    EE_hal_terminate_idle_task(&p_CDB->p_idle_task->HDB);
  } else {
    status = E_OS_STATE;
  }

  EE_hal_end_nested_primitive(flags);
  return status;
}

StatusType GetTaskID ( TaskRefType TaskID )
{
  register StatusType ev;
  /* [OS566]: The Operating System API shall check in extended mode all pointer
      argument for NULL pointer and return OS_E_PARAMETER_POINTER
      if such argument is NULL.
      +
      MISRA dictate NULL check for pointers always. */
  if ( TaskID == NULL ) {
    ev = E_OS_PARAM_POINTER;
  } else {
    /* XXX: This SHALL be atomic. */
    *TaskID = EE_get_curr_task()->tid;
    ev = E_OK;
  }

  return ev;
}
