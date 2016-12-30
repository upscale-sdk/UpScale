#include "ee_k1_bsp_communication.h"
#include "ee_hal_internal.h"
#include "ee_kernel_k1.h"
#include "ee_kernel.h"
#include <assert.h>

StatusType CreateJob ( JobTypeRef JobIdRef, CoreNumber JobAttendee,
  TaskPrio JobPrio, JobTaskFunc JobFunc, JobTaskParam JobParam,
  MemSize StackSize )
{
  StatusType status_type;
  if ( (JobIdRef == NULL) || (JobFunc == NULL) ) {
    status_type = E_OS_PARAM_POINTER;
  } else
  if ( JobAttendee > EE_K1_PE_NUMBER ) {
    status_type = E_OS_VALUE;
  } else
  if ( StackSize == SYSTEM_STACK ) {
    status_type = E_OS_VALUE;
  } else {
    EE_FREG const flags = EE_hal_begin_nested_primitive();
    if ( KDB_WJ.p_KCB_WJ->job_index < EE_MAX_NUM_JOB ) {
      if ( JobAttendee > 0U ) {
        EE_UREG     i, core_count = 0U;
        EE_BIT      used_cores[EE_K1_PE_NUMBER] = {};

        /* Check if it's possible to create the Job, and lock the cores that
           attends to the job */
        /* Actually Task Core allocation rule is "First found is used" */
        if ( KDB_WJ.kdb.p_KCB->free_TDB_index <
          (EE_TDB_ARRAY_SIZE - JobAttendee) )
        {
          for ( i = 0U; (i < EE_K1_PE_NUMBER) &&
            (core_count < JobAttendee); ++i )
          {
            EE_CDB *    p_CDB    = &KDB_WJ.core_descriptors[i];
            EE_CCB_WL * p_CCB_wl = &KCB_WJ.core_ctrls[i];
            /* Lock the core until the Job creation end */
            __k1_spin_lock( &p_CCB_wl->lock );
            if ( (p_CDB->free_TCB_index < p_CDB->tcb_array_size) &&
              (ee_k1_pools[i].residual_mem >= StackSize) )
            {
              used_cores[i] = EE_TRUE;
              ++core_count;
            } else {
              /* Release the unused core */
              __k1_spin_unlock( &p_CCB_wl->lock );
            }
          }
        }
        /* Check if the job can be created */
        if ( core_count >= JobAttendee) {
          /* Allocate New Job */
          EE_JOB *         p_alloc_job;

          /* lock the kernel to access job counters an to create TASKs */
          EE_lock_kernel();

          (*JobIdRef) = KDB_WJ.p_KCB_WJ->job_index++;
          p_alloc_job = &KDB_WJ.p_KCB_WJ->jobs[(*JobIdRef)];

          /* Fill the Job */
          p_alloc_job->job_func             = JobFunc;
          p_alloc_job->job_param            = JobParam;
          p_alloc_job->job_prio             = JobPrio;

          for ( i = 0U; (i < EE_K1_PE_NUMBER) && (core_count > 0U); ++i ) {
            StatusType create_status;
            EE_TID     task_id;
            if ( used_cores[i] == EE_TRUE ) {
              create_status = EE_create_task (i, &task_id,
                EE_TASK_TYPE_EXTENDED, EE_job_wrapper, JobPrio, JobPrio, 1U,
                StackSize );
              if ( create_status == E_OK ) {
                /* Fill the Job */
                p_alloc_job->task_attendees_id[i] = task_id;
                p_alloc_job->attendee_mask |= EE_BIT(i);
                --core_count;
              } else {
                /* THIS SHALL NEVER HAPPENS !!! */
                assert(false);
              }
            } else {
              p_alloc_job->task_attendees_id[i] = INVALID_TASK_ID;
            }
          }
          /* Set the Job as Terminated */
          p_alloc_job->terminated_task_mask = p_alloc_job->attendee_mask;

          /* Release the Kernel & Commit the changes in memory */
          EE_unlock_kernel();

          /* Release all the used cores */
          for ( i = 0U;
            (i < EE_K1_PE_NUMBER) && (core_count < JobAttendee); ++i )
          {
            if ( used_cores[i] == EE_TRUE ) {
              __k1_spin_unlock(&KCB_WJ.core_ctrls[i].lock);
              ++core_count;
            }
          }
          status_type = E_OK;
        } else {
          status_type = E_OS_LIMIT;
          /* Release the alraedy acquired cores */
          for ( i = 0U; (i < EE_K1_PE_NUMBER) && (core_count > 0U); ++i ) {
            if ( used_cores[i] == EE_TRUE ) {
              __k1_spin_unlock(&KCB_WJ.core_ctrls[i].lock);
              --core_count;
            }
          }
        }
      } else {
        status_type = E_OS_NOFUNC;
      }
    } else {
      status_type = E_OS_LIMIT;
    }
    EE_hal_end_nested_primitive(flags);
  }
  return status_type;
}

StatusType ActivateJob ( JobType JobId, CoreNumber ActivatedAttendee ) {
  StatusType status_type;
  if ( JobId < KDB_WJ.p_KCB_WJ->job_index ) {
    EE_CORE_ID core_id = EE_get_curr_core_id();
    /* Activate all the JOB TASKs */
    EE_FREG flags = EE_hal_begin_nested_primitive();
    if ( core_id == EE_K1_BOOT_CORE ) {
      status_type = EE_k1_rm_activate_job(JobId, ActivatedAttendee, core_id);
    } else {
      status_type = EE_ar_rpc(OSServiceId_ActivateJob, JobId,
        ActivatedAttendee, INVALID_PARAM);
    }
    if ( status_type == E_OK ) {
       EE_KDB EE_CONST * const p_KDB = EE_get_kernel();
       EE_CDB EE_CONST * const p_CDB = EE_lock_and_get_core(core_id);
       if ( EE_scheduler_task_current_is_preempted(p_CDB) ) {
        EE_TDB * const p_current   = &p_KDB->tdb_array[p_CDB->p_CCB->curr_tid];
        EE_TDB * const p_prempting = EE_scheduler_task_dispatch(p_CDB);

        EE_unlock_core(core_id);

        EE_hal_change_context(p_prempting->p_HDB, p_prempting->task_func,
          p_current->p_HDB);
      } else {
        EE_unlock_core(core_id);
      }
    }
    EE_hal_end_nested_primitive(flags);
  } else {
    status_type = E_OS_ID;
  }
  return status_type;
}

StatusType JoinJob ( JobType JobId ) {
  StatusType status_type;
  EE_array_index job_index = __k1_umem_read32(&KDB_WJ.p_KCB_WJ->job_index);
  if ( JobId < job_index ) {
    EE_JOB   * const p_JOB      = &KDB_WJ.p_KCB_WJ->jobs[JobId];
    EE_TDB   * const p_current  = EE_get_curr_task();
    /* Invalidate the Job line */
    __k1_dcache_invalidate_mem_area((uintptr_t)p_JOB, sizeof(*p_JOB));
    if ( p_current->p_TCB->current_prio < p_JOB->job_prio) {
      while ( __k1_umem_read32(&p_JOB->attendee_mask) !=
        __k1_umem_read32(&p_JOB->terminated_task_mask) ) {
        ; /* Nothing TO DO */
      }
      status_type = E_OK;
    } else {
      status_type = E_OS_ACCESS;
    }
  } else {
    status_type = E_OS_ID;
  }
  return status_type;
}

static EE_status_type
  EE_block_on_value ( BlockableValueTypeRef BlockableValueRef,
    WaitCondType WaitCond, ValueType RightValue)
{
  EE_status_type            status_type;
  EE_TDB EE_CONST  *        p_current;
  EE_KDB EE_CONST  *  const p_KDB = EE_get_kernel();
  EE_CORE_ID          const core_id   = EE_get_curr_core_id();
  EE_FREG             const flags     = EE_hal_begin_nested_primitive();
  EE_CDB           *  const p_CDB     = EE_get_core(core_id);

  p_current = &p_KDB->tdb_array[p_CDB->p_CCB->curr_tid];

  if ( p_current->task_type != EE_TASK_TYPE_EXTENDED ) {
    status_type = E_OS_ACCESS;
  } else
  if ( core_id == EE_K1_BOOT_CORE ) {
    status_type = E_OS_CORE;
  } else
  {
    EE_TDB * p_To;
    __k1_spin_lock(&BlockableValueRef->lock);

    if ( BlockableValueRef->core_id == INVALID_CORE_ID ) {
      BoolType  cond_result;
      ValueType value = __k1_umem_read32(&BlockableValueRef->value);

      status_type = CheckCondition (&cond_result, value, WaitCond, RightValue);

      if ( (status_type == E_OK) && (cond_result == EE_FALSE) ) {
        BlockableValueRef->core_id     = core_id;
        BlockableValueRef->right_value = RightValue;
        BlockableValueRef->wait_cond   = WaitCond;

        EE_lock_core(core_id);
        p_To = EE_scheduler_task_block_current(p_CDB,
          &BlockableValueRef->blocked_queue);
        EE_unlock_core(core_id);
        __k1_spin_unlock(&BlockableValueRef->lock);

        /* if p_To point to IdleTask Could I go in Idle instead ? */

        EE_hal_change_context(p_To->p_HDB, p_To->task_func, p_current->p_HDB);
      } else {
        __k1_spin_unlock(&BlockableValueRef->lock);
      }
    } else {
      __k1_spin_unlock(&BlockableValueRef->lock);
       status_type = E_OS_ACCESS;
    }
  }
  EE_hal_end_nested_primitive(flags);
  return status_type;
}


StatusType WaitCondition ( BlockableValueTypeRef BlockableValueRef,
  WaitCondType WaitCond, ValueType RightValue, BlockPolicyType BlockPolicy)
{
  StatusType     status_type = E_OK;
  ValueType      value;
  EE_BOOL        cond_result;
  if ( BlockableValueRef == NULL ) {
    status_type = E_OS_PARAM_POINTER;
  }
  else
  {
    do {
      value       = __k1_umem_read32(&BlockableValueRef->value);
      status_type = CheckCondition (&cond_result, value, WaitCond, RightValue);

      if ( status_type == E_OK ) {
         if ( cond_result == EE_FALSE ) {
           switch ( BlockPolicy ) {
             case BLOCK_IMMEDIATLY:
               status_type = EE_block_on_value(BlockableValueRef, WaitCond,
                 RightValue);
               cond_result = EE_TRUE;
               break;
             case BLOCK_OS:
               /* TODO */
               break;
             case BLOCK_NO:
               break;
             default:
               status_type = E_OS_VALUE;
               break;
           }
         }
      } else {
        break;
      }
    } while ( (status_type == E_OK) && (cond_result == EE_FALSE) );
  }
  return status_type;
}

StatusType SignalValue (BlockableValueTypeRef BlockableValueRef,
  ValueType Value)
{
  StatusType     status_type;
  if ( BlockableValueRef == NULL ) {
    status_type = E_OS_PARAM_POINTER;
  } else
  {
    EE_CORE_ID core_id;
    __k1_spin_lock(&BlockableValueRef->lock);
    __k1_umem_write32(&BlockableValueRef->value, Value);
    core_id = __k1_umem_read32(&BlockableValueRef->core_id);
    if ( core_id != INVALID_CORE_ID ) {
      EE_BOOL cond_result;
      EE_wait_cond const wait_cond =
        __k1_umem_read32(&BlockableValueRef->wait_cond);
      EE_UREG      const right_value =
        __k1_umem_read32(&BlockableValueRef->right_value);

      status_type = CheckCondition(&cond_result, Value, wait_cond, right_value);

      if ( (status_type == E_OK) && cond_result ) {
        status_type = EE_ar_rpc(OSServiceId_SignalValue,
          (uintptr_t)BlockableValueRef, INVALID_PARAM, INVALID_PARAM);
      }
    } else {
      status_type = E_OK;
    }
    __k1_spin_unlock(&BlockableValueRef->lock);
  }
  return status_type;
}

