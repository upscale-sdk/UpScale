/*
 * ee_api_k1.h
 *
 *  Created on: Dec 16, 2014
 *      Author: e.guidieri
 */

#ifndef EE_API_K1_H_
#define EE_API_K1_H_

#include "eecfg.h"

#ifdef EE_API_EXTENSION

/* #include "ee_basic_data_structures.h" */
#include "ee_api_types.h"
#include "ee_hal.h"

typedef EE_UREG EE_JOB_ID;
typedef void *  EE_job_param;
typedef void (* EE_job_func ) ( EE_job_param );
typedef uint8_t EE_job_prio;

typedef mOS_vcore_set_t      CoreMask;
typedef EE_JOB_ID            JobType;
typedef JobType *            JobRefType;

typedef EE_job_param  JobTaskParam;
typedef EE_job_func   JobTaskFunc;

#define INVALID_CORE_ID ((EE_CORE_ID)-1)
#define INVALID_JOB_ID ((EE_JOB_ID)-1)

typedef enum EE_wait_cond_tag {
  VALUE_EQ,
  VALUE_NOT_EQ,
  VALUE_LT,
  VALUE_GT,
  VALUE_LT_OR_EQ,
  VALUE_GT_OR_EQ,
} EE_wait_cond;

typedef enum EE_block_policy_tag {
  BLOCK_OS,
  BLOCK_IMMEDIATLY,
  BLOCK_NO
} EE_block_policy;

typedef EE_UREG              ValueType;
typedef ValueType *          ValueTypeRef;
typedef EE_spin_lock         SpinlockObjType;
typedef SpinlockObjType *    SpinlockObjTypeRef;
typedef EE_wait_cond         WaitCondType;
typedef EE_block_policy      BlockPolicyType;

typedef struct EE_blockable_value_tag {
  SpinlockObjType        lock;
  EE_SN           *      blocked_queue;
  ValueType              value;
  WaitCondType           wait_cond;
  ValueType              right_value;
} EE_blockable_value;
typedef EE_blockable_value    BlockableValueType;
typedef BlockableValueType *  BlockableValueTypeRef;


#define OS_SERVICE_ID_K1          OS_SERVICE_ID_DYNAMIC
#define OSServiceId_CreateJob    (OS_SERVICE_ID_K1 + 0U)
#define OSServiceId_ActivateJob  (OS_SERVICE_ID_K1 + 1U)
#define OSServiceId_JoinJob      (OS_SERVICE_ID_K1 + 2U)
#define OSServiceId_SignalValue  (OS_SERVICE_ID_K1 + 3U)

EE_INLINE__ void SpinInitObj ( SpinlockObjTypeRef SpinlockObjRef ) {
  EE_k1_spin_init_lock(SpinlockObjRef);
}

EE_INLINE__ void SpinLockObj ( SpinlockObjTypeRef SpinlockObjRef ) {
  EE_k1_spin_lock(SpinlockObjRef);
}

EE_INLINE__ void SpinUnlockObj ( SpinlockObjTypeRef SpinlockObjRef ) {
  EE_k1_spin_unlock(SpinlockObjRef);
}

EE_INLINE__ StatusType CheckCondition ( BoolTypeRef CondResultRef,
  ValueType Value, WaitCondType WaitCond, ValueType RightValue )
{
  StatusType status_type = E_OK;
  if ( CondResultRef != NULL ) {
    switch ( WaitCond ) {
      case VALUE_EQ:
        *CondResultRef  = (Value == RightValue);
      break;
      case VALUE_NOT_EQ:
        *CondResultRef  = (Value != RightValue);
      break;
      case VALUE_LT:
        *CondResultRef  = (Value < RightValue);
      break;
      case VALUE_GT:
        *CondResultRef  = (Value > RightValue);
      break;
      case VALUE_LT_OR_EQ:
        *CondResultRef  = (Value <= RightValue);
      break;
      case VALUE_GT_OR_EQ:
        *CondResultRef  = (Value >= RightValue);
      break;
      default:
        *CondResultRef = EE_FALSE;
        status_type    = E_OS_NOFUNC;
      break;
    }
  } else {
    status_type = E_OS_PARAM_POINTER;
  }
  return status_type;
}

StatusType CreateJob ( JobRefType JobIdRef, CoreMask JobAttendeeMask,
  TaskPrio JobPrio, JobTaskFunc JobFunc, JobTaskParam JobParam,
  MemSize StackSize );

StatusType ReadyJob ( JobType JobId, CoreMask ActivatedAttendeeMask );

StatusType ActivateJob ( JobType JobId, CoreMask ActivatedAttendeeMask );

StatusType GetJobID ( JobRefType JobIdRef );

StatusType JoinJob ( JobType JobId );

EE_INLINE__ void InitBlockableValue ( BlockableValueTypeRef BlockableValueRef,
  ValueType Value)
{
  if ( BlockableValueRef != NULL ) {
    EE_k1_spin_init_lock(&BlockableValueRef->lock);
    BlockableValueRef->blocked_queue = NULL;
    BlockableValueRef->value         = Value;
    BlockableValueRef->right_value   = Value;
    BlockableValueRef->wait_cond     = VALUE_EQ;
  }
}

StatusType WaitCondition ( BlockableValueTypeRef BlockableValueRef,
  WaitCondType WaitCond, ValueType RightValue, BlockPolicyType BlockPolicy );

StatusType SignalValue (BlockableValueTypeRef BlockableValueRef,
  ValueType Value);

StatusType CommAndSchedule ( void );

#endif /* EE_API_EXTENSION */

#endif /* EE_API_K1_H_ */
