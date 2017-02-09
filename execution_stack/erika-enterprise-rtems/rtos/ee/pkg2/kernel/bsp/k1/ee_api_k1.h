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

#include "ee_basic_data_structures.h"
#include "ee_api_types.h"
#include "ee_k1_bsp_communication.h"
#include "ee_hal.h"

typedef EE_UREG EE_JOB_ID;
typedef void *  EE_job_param;
typedef void (* EE_job_func ) ( EE_job_param );
typedef uint8_t EE_job_prio;

typedef EE_UREG       CoreNumber;
typedef EE_JOB_ID     JobType;
typedef JobType *     JobTypeRef;

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
  SpinlockObjType  lock;
  CoreIdType       core_id;
  ArrayIndex       blocked_queue;
  ValueType        value;
  WaitCondType     wait_cond;
  ValueType        right_value;
} EE_blockable_value;
typedef EE_blockable_value    BlockableValueType;
typedef BlockableValueType *  BlockableValueTypeRef;


#define OS_SERVICE_ID_K1          OS_SERVICE_ID_DYNAMIC
#define OSServiceId_CreateJob    (OS_SERVICE_ID_K1 + 0U)
#define OSServiceId_ActivateJob  (OS_SERVICE_ID_K1 + 1U)
#define OSServiceId_JoinJob      (OS_SERVICE_ID_K1 + 2U)
#define OSServiceId_SignalValue  (OS_SERVICE_ID_K1 + 3U)

EE_INLINE__ StatusType SendMessage ( CoreIdType RemoteCoreId, void * Msg,
  void ** AnsRef, MemSize MsgSize )
{
  return EE_k1_send_message(RemoteCoreId, Msg, AnsRef, MsgSize);
}

#if 0
EE_INLINE__ StatusType PostMessage ( CoreIdType RemoteCoreId,
  void * Msg, MemSize MsgSize )
{
  return EE_k1_post_message (RemoteCoreId, Msg, MsgSize);
}
#endif /* 0 */

EE_INLINE__ StatusType WaitMessage ( CoreIdType RemoteCoreId,
  void * Msg, MemSize MsgSize )
{
  return EE_k1_wait_message(RemoteCoreId, Msg, MsgSize);
}

EE_INLINE__ StatusType AckMessage ( CoreIdType RemoteCoreId )
{
  return EE_k1_ack_message(RemoteCoreId);
}

EE_INLINE__ StatusType PostAnsMessage ( CoreIdType RemoteCoreId,
  void * Msg, MemSize MsgSize)
{
  return EE_k1_post_answer_message (RemoteCoreId, Msg, MsgSize);
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
        status_type = E_OS_NOFUNC;
      break;
    }
  } else {
    status_type = E_OS_PARAM_POINTER;
  }
  return status_type;
}

StatusType CreateJob ( JobTypeRef JobIdRef, CoreNumber JobAttendee,
  TaskPrio JobPrio, JobTaskFunc JobFunc, JobTaskParam JobParam,
  MemSize StackSize );

StatusType ActivateJob ( JobType JobId, CoreNumber ActivatedAttendee );

StatusType JoinJob ( JobType JobId );

EE_INLINE__ void InitBlockableValue ( BlockableValueTypeRef BlockableValueRef,
  ValueType Value)
{
  if ( BlockableValueRef != NULL ) {
    BlockableValueRef->core_id       = INVALID_CORE_ID;
    EE_k1_spin_init_lock(&BlockableValueRef->lock);
    BlockableValueRef->blocked_queue = INVALID_INDEX;
    BlockableValueRef->value         = Value;
    BlockableValueRef->right_value   = Value;
    BlockableValueRef->wait_cond     = VALUE_EQ;
  }
}

StatusType WaitCondition ( BlockableValueTypeRef BlockableValueRef,
  WaitCondType WaitCond, ValueType RightValue, BlockPolicyType BlockPolicy );

StatusType SignalValue (BlockableValueTypeRef BlockableValueRef,
  ValueType Value);

#endif /* EE_API_EXTENSION */

#endif /* EE_API_K1_H_ */
