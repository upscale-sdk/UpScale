#include "ee_hal_internal.h"
#include "ee_kernel_k1.h"
#include "ee_kernel.h"
#include <assert.h>

StatusType CreateJob ( JobRefType JobIdRef, CoreMask JobAttendeeMask,
  TaskPrio JobPrio, JobTaskFunc JobFunc, JobTaskParam JobParam,
  MemSize StackSize )
{
  StatusType status_type;
  if ( (JobIdRef == NULL) || (JobFunc == NULL) ) {
    status_type = E_OS_PARAM_POINTER;
  } else {
    EE_UREG  const JobAttendeeCount = __k1_cbs(JobAttendeeMask);
    EE_FREG  const flags            = EE_hal_begin_nested_primitive();

    /* Enter in the critical section to create the Job */
    EE_lock_kernel();

    if ( (KDB_WJ.p_KCB_WJ->job_index < EE_MAX_NUM_JOB) &&
      (ee_k1_pool.residual_mem >=
        (JobAttendeeCount * (StackSize + EE_STACK_GUARD_AREA))) )
    {
      if ( JobAttendeeMask != 0U ) {
        /* Check if it's possible to create the Job, and lock the cores that
           attends to the job */
        /* Actually Task Core allocation rule is "First found is used" */
        if ( KDB_WJ.kdb.p_KCB->free_task_index <
            (EE_TASK_ARRAY_SIZE - JobAttendeeCount) )
        {
          EE_UREG     i;
          /* Allocate New Job */
          EE_JOB *         p_alloc_job;

          (*JobIdRef) = KDB_WJ.p_KCB_WJ->job_index++;
          p_alloc_job = &KDB_WJ.p_KCB_WJ->jobs[(*JobIdRef)];

          /* Fill the Job */
          p_alloc_job->job_func             = JobFunc;
          p_alloc_job->job_param            = JobParam;
          p_alloc_job->job_prio             = JobPrio;
          p_alloc_job->job_id               = (*JobIdRef);
          /* Init the Job Lock */
          EE_k1_spin_init_lock(&p_alloc_job->lock);

          for ( i = 0U; i < EE_K1_CORE_NUMBER; ++i ) {
            /* Fill the Job */
            if ( (JobAttendeeMask & EE_BIT(i)) != 0 ) {
              EE_TID     task_id;
              StatusType const create_status = EE_create_task (i, &task_id,
                EE_TASK_TYPE_EXTENDED, EE_job_wrapper, JobPrio,
                  JobPrio, 1U, StackSize );
              if ( create_status == E_OK ) {
                p_alloc_job->task_attendees_id[i] = task_id;
                p_alloc_job->attendee_mask |= EE_BIT(i);
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
          status_type = E_OK;
        } else {
          status_type = E_OS_LIMIT;
        }
      } else {
        status_type = E_OS_NOFUNC;
      }
    } else {
      status_type = E_OS_LIMIT;
    }

    /* Release the Kernel & Commit the changes in memory */
    EE_unlock_kernel();
    EE_hal_end_nested_primitive(flags);
  }
  return status_type;
}

StatusType ReadyJob ( JobType JobId, CoreMask JobAttendeeMask ) {
  StatusType status_type;
  if ( JobId < KDB_WJ.p_KCB_WJ->job_index ) {
    EE_CORE_ID core_id = EE_get_curr_core_id();
    /* Activate all the JOB TASKs */
    EE_FREG flags = EE_hal_begin_nested_primitive();

    status_type = EE_k1_activate_job(JobId, JobAttendeeMask, core_id
#if (defined(EE_K1_FULL_PREEMPTION))
      , EE_FALSE
#endif /* EE_K1_FULL_PREEMPTION */
    );

    EE_hal_end_nested_primitive(flags);
  } else {
    status_type = E_OS_ID;
  }
  return status_type;
}

StatusType ActivateJob ( JobType JobId, CoreMask JobAttendeeMask ) {
  StatusType status_type;
  if ( JobId < KDB_WJ.p_KCB_WJ->job_index ) {
    EE_CORE_ID core_id = EE_get_curr_core_id();
    /* Activate all the JOB TASKs */
    EE_FREG flags = EE_hal_begin_nested_primitive();

    status_type = EE_k1_activate_job(JobId, JobAttendeeMask, core_id
#if (defined(EE_K1_FULL_PREEMPTION))
      , EE_TRUE
#endif /* EE_K1_FULL_PREEMPTION */
    );

    if ( status_type == E_OK ) {
      EE_k1_optimized_task_preemption_point();
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
    EE_JOB   * const p_JOB   = &KDB_WJ.p_KCB_WJ->jobs[JobId];
    EE_TDB   * const p_curr  = EE_get_curr_task();
    /* Invalidate the Job line */
    /* __k1_dcache_invalidate_mem_area((uintptr_t)p_JOB, sizeof(*p_JOB)); */
    if ( p_curr->p_TCB->current_prio < p_JOB->job_prio) {
      while ( __k1_umem_read32(&p_JOB->attendee_mask) !=
        __k1_umem_read32(&p_JOB->terminated_task_mask) )
      {
#if (!defined(EE_K1_FULL_PREEMPTION))
        EE_k1_optimized_task_preemption_point();
#else
        ; /* Nothing TO DO */
#endif /* !EE_K1_FULL_PREEMPTION */
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

StatusType GetJobID ( JobRefType JobIdRef ) {
  register StatusType ev;

    if ( JobIdRef == NULL ) {
      ev = E_OS_PARAM_POINTER;
    } else {
      EE_CDB EE_CONST * const p_CDB    = EE_get_curr_core();
      EE_TDB EE_CONST * const p_curr   = p_CDB->p_CCB->p_curr;
      EE_JOB          * const p_job    = KCB_WJ.tid_to_job[p_curr->tid];

      if ( p_job != NULL ) {
        (*JobIdRef) = p_job->job_id;
        ev = E_OK;
      } else {
        (*JobIdRef) = INVALID_JOB_ID;
        ev = E_OS_NOFUNC;
      }
    }

    return ev;
}

StatusType CommAndSchedule ( void ) {
#if (defined(EE_HAS_COMM_HOOK))
if ( isPendingOffload() ) {
  if ( EE_k1_spin_trylock(&comm_lock) ) {
    CommunicationHook();
    EE_k1_spin_unlock(&comm_lock);
  }
}
#endif /* EE_HAS_COMM_HOOK */

  EE_k1_optimized_task_preemption_point();

  return E_OK;
}

static EE_status_type EE_k1_block_on_value ( EE_blockable_value *
  blockable_value_ref, EE_wait_cond wait_cond, EE_UREG right_value )
{
  EE_status_type            status_type;
  EE_KDB EE_CONST  *  const p_KDB     = EE_get_kernel();
  EE_CDB EE_CONST  *  const p_CDB     = EE_get_curr_core();
  EE_CCB           *  const p_CCB     = p_CDB->p_CCB;
  EE_TDB EE_CONST  *  const p_curr    = p_CCB->p_curr;
  EE_FREG             const flags     = EE_hal_begin_nested_primitive();

  if ( p_curr->task_type != EE_TASK_TYPE_EXTENDED ) {
    status_type = E_OS_ACCESS;
  } else {
    EE_TDB EE_CONST * p_To;

    __k1_fspinlock_lock(&blockable_value_ref->lock);

    if ( blockable_value_ref->blocked_queue == NULL ) {
      BoolType  cond_result;
      ValueType value = __k1_umem_read32(&blockable_value_ref->value);

      status_type = CheckCondition(&cond_result, value, wait_cond, right_value);

      if ( (status_type == E_OK) && (cond_result == EE_FALSE) ) {
        blockable_value_ref->right_value = right_value;
        blockable_value_ref->wait_cond   = wait_cond;

        /* Prepare to put the TASK inside the BlockableValue queue...
         * It will be really done in EE_scheduler_task_wrapper calling
         * EE_scheduler_task_saving_available */
        p_To = EE_scheduler_task_block_current(p_KDB, p_CDB,
          &blockable_value_ref->blocked_queue);

#if (defined(EE_SCHEDULER_GLOBAL)) && (!defined(EE_SINGLECORE))
        p_CCB->p_lock_to_be_released = &blockable_value_ref->lock;
#else
        EE_k1_spin_unlock( &blockable_value_ref->lock );
#endif

        EE_hal_change_context(&p_To->HDB, p_To->task_func, &p_curr->HDB);
      } else {
        __k1_fspinlock_unlock(&blockable_value_ref->lock);
      }
    } else {
      __k1_fspinlock_unlock(&blockable_value_ref->lock);
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
  } else {
    do {
      value       = __k1_umem_read32(&BlockableValueRef->value);
      status_type = CheckCondition (&cond_result, value, WaitCond, RightValue);

      if ( status_type == E_OK ) {
#if (defined(EE_HAS_COMM_HOOK))
        if ( isPendingOffload() ) {
          if ( EE_k1_spin_trylock(&comm_lock) ) {
            CommunicationHook();
            EE_k1_spin_unlock(&comm_lock);
          }
        }
#endif /* EE_HAS_COMM_HOOK */
        if ( cond_result == EE_FALSE ) {
          switch ( BlockPolicy ) {
            case BLOCK_OS:
            case BLOCK_IMMEDIATLY:
              status_type = EE_k1_block_on_value(BlockableValueRef, WaitCond,
                RightValue);
              cond_result = EE_TRUE;
              break;
             case BLOCK_NO:
               EE_k1_optimized_task_preemption_point();
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

void EE_job_wrapper ( void ) {
  EE_UREG           terminated_mask;
  /* C Single inheritance */
  EE_CDB EE_CONST * const p_CDB    = EE_get_curr_core();
  EE_TDB EE_CONST * const p_curr   = p_CDB->p_CCB->p_curr;
  EE_JOB          * const p_job    = KCB_WJ.tid_to_job[p_curr->tid];

  p_job->job_func(p_job->job_param);

  __k1_fspinlock_lock(&p_job->lock);
  terminated_mask = __k1_umem_read32(&p_job->terminated_task_mask) | EE_BIT(p_curr->orig_core_id);
  __k1_umem_write32(&p_job->terminated_task_mask, terminated_mask );
  KCB_WJ.tid_to_job[p_curr->tid] = NULL;
  __k1_fspinlock_unlock(&p_job->lock);

  TerminateTask();
}
