/*
 * ee_api_dynamic.c
 *
 *  Created on: 07/dic/2014
 *      Author: AmministratoreErrico
 */

#include "ee_internal.h"

#if (defined(EE_API_DYNAMIC))

StatusType EE_create_task ( EE_CORE_ID core_id, EE_TID * taskIdRef,
  EE_task_type taskType, EE_task_func taskFunc, EE_task_prio readyPrio,
  EE_task_prio dispatchPrio, EE_task_nact maxNumOfAct, EE_mem_size stackSize )
{
  StatusType ret_val = E_OK;
  EE_KDB * const p_KDB = EE_get_kernel();
  EE_KCB * const p_KCB = p_KDB->p_KCB;

  if ( p_KCB->free_task_index < EE_TASK_ARRAY_SIZE ) {
    EE_TDB * p_TDB;
    /* Prova ad assegnare il TID */
    EE_array_index const tid   = p_KDB->p_KCB->free_task_index;
    /* Ottieni il TDB */
    p_TDB = &p_KDB->tdb_array[tid];

    if ( EE_hal_hdb_init(core_id, &p_TDB->HDB, stackSize) ) {
      EE_TCB * const p_TCB = p_TDB->p_TCB;
      /* Alloca TDB e TCB */
      ++p_KCB->free_task_index;

      /* New Task Id */
      (*taskIdRef) = tid;

      /* Fill TDB */
      p_TDB->tid              = tid;
      p_TDB->task_type        = taskType;
      p_TDB->task_func        = taskFunc;
      p_TDB->dispatch_prio    = dispatchPrio;
      p_TDB->ready_prio       = readyPrio;
      p_TDB->orig_core_id     = core_id;
      p_TDB->max_num_of_act   = maxNumOfAct;

      /* Fill TCB */
      p_TCB->current_prio        = readyPrio;
      p_TCB->current_core_id     = core_id;
      p_TCB->status              = EE_TASK_SUSPENDED;
      p_TCB->residual_activation = maxNumOfAct;

      ret_val = E_OK;
    } else {
      ret_val = E_OS_LIMIT;
    }
  } else {
    ret_val = E_OS_LIMIT;
  }
  return ret_val;
}

StatusType CreateTask( TaskRefType taskIdRef, TaskExecutionType taskType,
  TaskFunc taskFunc, TaskPrio readyPrio, TaskPrio dispatchPrio,
  TaskActivation maxNumOfAct, MemSize stackSize )
{
  StatusType status_type;
  EE_FREG    const flags   = EE_hal_begin_nested_primitive();
  EE_CORE_ID const core_id = EE_get_curr_core_id();
  EE_lock_kernel();
  status_type = EE_create_task ( core_id, taskIdRef, taskType,
    taskFunc, readyPrio, dispatchPrio, maxNumOfAct, stackSize );
  EE_unlock_kernel();
  EE_hal_end_nested_primitive(flags);
  return status_type;
}

StatusType SetISR2Source( TaskType taskId, IRS2SourceId ISR2Id )
{
  StatusType status_type;
  EE_KDB EE_CONST * p_KDB = EE_get_kernel();
  if ( taskId >= p_KDB->p_KCB->free_task_index ) {
     status_type = E_OS_ID;
  } else {
    EE_lock_kernel();
    status_type = EE_hal_set_isr2_source (taskId, ISR2Id,
      p_KDB->tdb_array[taskId].ready_prio );
    EE_unlock_kernel();
  }
  return status_type;
}

#endif /* EE_API_DYNAMIC */
