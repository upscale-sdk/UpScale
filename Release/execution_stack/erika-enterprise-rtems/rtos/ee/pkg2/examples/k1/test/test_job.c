/*
 * test_job.c
 *
 *  Created on: Jan 22, 2015
 *      Author: e.guidieri
 */

#define ASSERT_LENGTH 70U
#include "ee_internal.h"
#include <stdio.h>
#include <stdarg.h>

#define JOB_TASK_STACK_SIZE 2048

#define ASSERT_LENGTH 70U
EE_TYPEASSERTVALUE EE_assertions[ASSERT_LENGTH];

#ifdef EE_JOB_TEST

#define ASSERT_LENGTH 70U
EE_k1_spinlock     assertions_lock;

int volatile assert_count = EE_ASSERT_NIL;
void test_assert(int test)
{
  register int next_assert;
  EE_k1_spin_lock(&assertions_lock);
  next_assert = (assert_count == EE_ASSERT_NIL) ? 1 : assert_count + 1;
  EE_assert(next_assert, test, EE_ASSERT_NIL);
  assert_count = next_assert;
  EE_k1_spin_unlock(&assertions_lock);
}
EE_UREG dummy_job_param;

#define ENABLE_PRINTF
#define ENABLE_LOCKED_PRINTF
#if defined(ENABLE_PRINTF)
#if defined(ENABLE_LOCKED_PRINTF)
EE_k1_spinlock     print_lock;

#define locked_printf(fmt, ...)       \
  do {                                \
     EE_k1_spin_lock(&print_lock);    \
     printf(fmt, __VA_ARGS__);        \
     EE_k1_spin_unlock(&print_lock);  \
  } while ( 0 )
#else
#define locked_printf printf
#endif /* ENABLE_LOCKED_PRINTF */
#else
#define locked_printf(fmt, ...) ((void)0)
#endif /* ENABLE_PRINTF */

BlockableValueType comm_variables[(EE_K1_CORE_NUMBER - 1U)];
typedef struct {
  SpinlockObjType  lock;
  CoreIdType       core_ids[EE_K1_CORE_NUMBER];
  ValueType        counter;
} core_stamps_t;
core_stamps_t core_stamps;
core_stamps_t core_stamps_end;

void dummy_job ( JobTaskParam param ) {
  EE_TDB EE_CONST * const volatile p_curr = EE_get_curr_core()->p_CCB->p_curr;
  volatile unsigned int tid               = p_curr->tid;
  volatile unsigned int orig_core_id      = p_curr->orig_core_id;

  test_assert( &dummy_job_param == param );

  if ( orig_core_id != (EE_K1_CORE_NUMBER - 1U) ) {
    BlockableValueTypeRef p_blockable_var = &comm_variables[orig_core_id];
    locked_printf(
      "Before First Wait Task[%p]:\ttid:%u orig_core_id:%u curr_core_id:%u\n---\n",
      p_curr, tid, orig_core_id, (unsigned int)EE_get_curr_core_id());
    WaitCondition(p_blockable_var, VALUE_EQ, (orig_core_id + 1U),
      BLOCK_IMMEDIATLY);
    test_assert(p_blockable_var->value == (orig_core_id + 1U));
    locked_printf(
      "After First Wait Task[%p]:\ttid:%u orig_core_id:%u curr_core_id:%u\n---\n",
      p_curr, tid, orig_core_id, (unsigned int)EE_get_curr_core_id());
  }

  SpinLockObj(&core_stamps.lock);
  locked_printf(
    "Before First Signal Task[%p]:\ttid:%u orig_core_id:%u curr_core_id:%u\n---\n",
    p_curr, tid, orig_core_id, (unsigned int)EE_get_curr_core_id());
  core_stamps.core_ids[core_stamps.counter++] = orig_core_id;
  SpinUnlockObj(&core_stamps.lock);

  if ( orig_core_id != EE_K1_MAIN_CORE ) {
    CoreIdType            const to_be_signal_id = (orig_core_id - 1U);
    BlockableValueTypeRef const p_signal_var    = &comm_variables[to_be_signal_id];
    SignalValue(p_signal_var, orig_core_id);
    locked_printf(
      "Before second Wait Task[%p]:\ttid:%u orig_core_id:%u curr_core_id:%u\n---\n",
      p_curr, tid, orig_core_id, (unsigned int)EE_get_curr_core_id());
    WaitCondition(p_signal_var, VALUE_EQ, to_be_signal_id, BLOCK_IMMEDIATLY);
  }

  SpinLockObj(&core_stamps_end.lock);
  locked_printf(
    "After second Wait Task[%p]:\ttid:%u orig_core_id:%u curr_core_id:%u\n---\n",
     p_curr, tid, orig_core_id, (unsigned int)EE_get_curr_core_id());
  core_stamps_end.core_ids[core_stamps_end.counter++] = orig_core_id;
  SpinUnlockObj(&core_stamps_end.lock);

  if ( orig_core_id != (EE_K1_CORE_NUMBER - 1U) ) {
    locked_printf(
      "Before second Signal Task[%p]:\ttid:%u orig_core_id:%u curr_core_id:%u\n---\n",
       p_curr, tid, orig_core_id, (unsigned int)EE_get_curr_core_id());
    SignalValue(&comm_variables[orig_core_id], orig_core_id);
  }
}

int main ( void ) {
  StatusType   status_type = E_OK;
  JobType      jobId;
  ArrayIndex   i;
  EE_STACK_T * sp;

  EE_k1_spin_init_lock(&assertions_lock);
  EE_k1_spin_init_lock(&core_stamps.lock);
  EE_k1_spin_init_lock(&core_stamps_end.lock);
#if defined(ENABLE_LOCKED_PRINTF)
  EE_k1_spin_init_lock(&print_lock);
#endif

  for ( i = 0U; i < (EE_K1_CORE_NUMBER - 1U); ++i ) {
    InitBlockableValue(&comm_variables[i], 0U);
  }
  status_type = CreateJob(&jobId, 0xFFFFU, 1U, dummy_job,
    &dummy_job_param, JOB_TASK_STACK_SIZE );
  test_assert( status_type == E_OK );

  sp = EE_get_SP();
  status_type = ActivateJob(jobId, 0xFFFFU);

  test_assert( sp == EE_get_SP() );
  test_assert( status_type == E_OK );

  status_type = JoinJob(jobId);

  test_assert( sp == EE_get_SP() );

  test_assert( status_type == E_OK );

  {
    EE_k1_rmb();
    for ( i = 0U; i < (EE_K1_CORE_NUMBER - 1U); ++i ) {
      test_assert (core_stamps.core_ids[i] == ((EE_K1_CORE_NUMBER - 1U) - i));
      test_assert (core_stamps_end.core_ids[i] == i);
    }
  }

  EE_assert_range(0, 1, assert_count);

  if ( EE_assert_last() == EE_ASSERT_YES) {
    printf("Job Test -- Passed\n");
  } else {
    printf("Job Test -- FAILED !!!\n");
  }

  /*
  while ( EE_TRUE ) {
    ;
  }
  */
  return 0;
}
#endif /* EE_JOB_TEST */
