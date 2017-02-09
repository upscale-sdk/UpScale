/*
 * test_job.c
 *
 *  Created on: Jan 22, 2015
 *      Author: e.guidieri
 */

#include "ee.h"

#ifdef EE_JOB_TEST

#define ASSERT_LENGTH 70U
EE_k1_spinlock     assertions_lock;
EE_TYPEASSERTVALUE EE_assertions[ASSERT_LENGTH];

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

#ifdef GOMP_TEST
#include <stdio.h>
////////////////////////////////////////////////////////////////////////////////

BlockableValueType MasterBar[EE_K1_PE_NUMBER];
BlockableValueType SlaveBar[EE_K1_PE_NUMBER];
/* This is the barrier code executed by each SLAVE core */
static inline void MSGBarrier_SlaveEnter(int myid)
{
	SignalValue(&MasterBar[myid], 1U);
	WaitCondition(&SlaveBar[myid], VALUE_EQ, 1U, BLOCK_IMMEDIATLY);
	SignalValue(&SlaveBar[myid], 0U);
}

/* This is the barrier code executed by the MASTER core to gather SLAVES */
static inline void MSGBarrier_Wait(int num_threads)
{
	unsigned int i;
	for(i = 1; i<num_threads; i++)
	{
		WaitCondition(&MasterBar[i], VALUE_EQ, 1U, BLOCK_IMMEDIATLY);
		SignalValue(&MasterBar[i], 0U);
	}
}

/* This is the barrier code executed by the MASTER core to gather SLAVES */
static inline void MSGBarrier_Release(int num_threads)
{
	unsigned int i;
	for(i = 1; i < num_threads; i++)
	{
	  printf("SIGNAL %d\n", i);
		SignalValue(&SlaveBar[i], 1U);
	}
}


static inline void gomp_hal_barrier()
{
  int myid = GetCoreID();
  if(myid == 0)
  {
    printf("I AM 0!!!\n");
    MSGBarrier_Wait(EE_K1_PE_NUMBER);
    MSGBarrier_Release(EE_K1_PE_NUMBER);
  }
  else {
    MSGBarrier_SlaveEnter(myid);
  }
}

void dummy_job ( JobTaskParam param ) {
  printf("START %d\n", (int)GetCoreID());
  gomp_hal_barrier();
  gomp_hal_barrier();
  gomp_hal_barrier();
  gomp_hal_barrier();
  gomp_hal_barrier();
  gomp_hal_barrier();
  gomp_hal_barrier();
  gomp_hal_barrier();
  gomp_hal_barrier();
  gomp_hal_barrier();
  gomp_hal_barrier();
  gomp_hal_barrier();
  printf("END\n");
}
////////////////////////////////////////////////////////////////////////////////
#endif /* GOMP_TEST */


#ifdef NORMAL_TEST
BlockableValueType comm_variables[(EE_K1_PE_NUMBER - 1U)];
typedef struct {
  SpinlockObjType  lock;
  CoreIdType       core_ids[(EE_K1_PE_NUMBER - 1U)];
  ValueType        counter;
} core_stamps_t;
core_stamps_t core_stamps;
core_stamps_t core_stamps_end;

void dummy_job ( JobTaskParam param ) {
  CoreIdType curr_core_id = GetCoreID();
  test_assert( &dummy_job_param == param );
  if ( curr_core_id != (EE_K1_PE_NUMBER - 1U) ) {
    BlockableValueTypeRef p_blockable_var = &comm_variables[curr_core_id];
    WaitCondition(p_blockable_var, VALUE_EQ, (curr_core_id + 1U),
      BLOCK_IMMEDIATLY);
    test_assert(p_blockable_var->value == (curr_core_id + 1U));
  }
  SpinLockObj(&core_stamps.lock);
  core_stamps.core_ids[core_stamps.counter++] = curr_core_id;
  SpinUnlockObj(&core_stamps.lock);
  if ( curr_core_id != EE_K1_MAIN_CORE ) {
    CoreIdType to_be_signal_id = (curr_core_id - 1U);
    BlockableValueTypeRef p_blockable_var = &comm_variables[to_be_signal_id];
    SignalValue(p_blockable_var, curr_core_id);
    WaitCondition(p_blockable_var, VALUE_EQ, to_be_signal_id, BLOCK_IMMEDIATLY);
  }
  if ( curr_core_id != (EE_K1_PE_NUMBER - 1U) ) {
    SpinLockObj(&core_stamps_end.lock);
    core_stamps_end.core_ids[core_stamps_end.counter++] = curr_core_id;
    SignalValue(&comm_variables[curr_core_id], curr_core_id);
    SpinUnlockObj(&core_stamps_end.lock);
  }
}
#endif /* NORMAL_TEST  */

int main ( void ) {
  StatusType status_type = E_OK;

#ifndef EE_JOB_TEST_FROM_RM
  JobId      jobId;
  status_type = CreateJob(&jobId, EE_K1_PE_NUMBER, 1U, dummy_job,
    &dummy_job_param, 256U );
  test_assert( status_type == E_OK );

  status_type = ActivateJob(jobId, EE_K1_PE_NUMBER);
  test_assert( status_type == E_OK );

  status_type = JoinJob(jobId);
#endif /* !EE_JOB_TEST_FROM_RM */

#ifdef NORMAL_TEST
  do {
    status_type = JoinJob(0U);
  } while ( (status_type != E_OK) ||
    (__k1_umem_read32(&core_stamps_end.counter) == 0));

  {
    ArrayIndex i;
    EE_k1_rmb();
    for ( i = 0U; i < (EE_K1_PE_NUMBER - 1U); ++i ) {
      test_assert (core_stamps.core_ids[i] == ((EE_K1_PE_NUMBER - 1U) - i));
      test_assert (core_stamps_end.core_ids[i] == i);
    }
  }
#endif /* NORMAL_TEST */

  test_assert( status_type == E_OK );

  EE_assert_range(0, 1, assert_count);
  EE_assert_last();


  while ( EE_TRUE ) {
    ;
  }
  return 0;
}
#endif /* EE_JOB_TEST */
