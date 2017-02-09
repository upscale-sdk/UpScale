/* Copyright 2014 DEI - Universita' di Bologna

author    DEI - Universita' di Bologna
          Alessandro Capotondi - alessandro.capotondi@unibo.it
          Giuseppe Tagliavini  - giuseppe.tagliavini@unibo.it
          Andrea Marongiu      - amarongiu@unibo.it
          
info      barrier management */

#include "bar.h"
#include "libgomp_config.h"
#include "appsupport.h"
#include "libgomp.h"

#define OMP_BLOCK_POLICY BLOCK_OS

/* This is the barrier code executed by each SLAVE core */
void
MSGBarrier_SlaveEnter(MSGBarrier b, int myid)
{
#ifdef BARRIER_DEBUG
  printf("MSGBarrier_SlaveEnter SIGNAL master (%d) at %p\n",myid, MASTER_FLAG(myid));
#endif
	SignalValue(MASTER_FLAG(myid), 1U);
#ifdef BARRIER_DEBUG
	printf("MSGBarrier_SlaveEnter WAIT slave (%d) at %p\n", myid,SLAVE_FLAG(myid));
#endif
	WaitCondition(SLAVE_FLAG(myid), VALUE_EQ, 1U, OMP_BLOCK_POLICY);
#ifdef BARRIER_DEBUG
	printf("MSGBarrier_SlaveEnter SIGNAL2 slave (%d) at %p\n",myid,SLAVE_FLAG(myid));
#endif
	SignalValue(SLAVE_FLAG(myid), 0U);
#ifdef BARRIER_DEBUG
	printf("MSGBarrier_SlaveEnter SIGNAL2 slave RESET (%d) at %p\n",myid,SLAVE_FLAG(myid));
#endif
}

/* This is the barrier code executed by the MASTER core to gather SLAVES */
ALWAYS_INLINE void
MSGBarrier_Wait(MSGBarrier b, int num_threads, unsigned int *local_slave_ids)
{
	unsigned int curr_proc_id, i;

	for(i = 1; i <num_threads; i++)
	{
		curr_proc_id = local_slave_ids[i];
#ifdef BARRIER_DEBUG
		printf("MSGBarrier_Wait WAIT master (%d/%d) at %p\n",curr_proc_id,num_threads,MASTER_FLAG(curr_proc_id));
#endif
		WaitCondition(MASTER_FLAG(curr_proc_id), VALUE_EQ, 1U, OMP_BLOCK_POLICY);
#ifdef BARRIER_DEBUG
		printf("MSGBarrier_Wait SIGNAL master (%d/%d) at %p \n",curr_proc_id,num_threads,MASTER_FLAG(curr_proc_id));
#endif
        
		SignalValue(MASTER_FLAG(curr_proc_id), 0U);
	}
}

/* This is the barrier code executed by the MASTER core to gather SLAVES */
ALWAYS_INLINE void
MSGBarrier_Release(MSGBarrier b, int num_threads, unsigned int *local_slave_ids)
{
	unsigned int curr_proc_id, i;

	for(i = 1; i < num_threads; i++)
	{
		curr_proc_id = local_slave_ids[i];
#ifdef BARRIER_DEBUG
    printf("MSGBarrier_Release SIGNAL master (%d/%d) at %p\n", curr_proc_id, num_threads-1,SLAVE_FLAG(curr_proc_id));
#endif
		SignalValue(SLAVE_FLAG(curr_proc_id), 1U);
	}
}

ALWAYS_INLINE void
MSGBarrier_Release_all(MSGBarrier b, int num_threads, unsigned int *local_slave_ids)
{
	unsigned int curr_proc_id, i;

	for(i = 1; i < num_threads; i++)
	{
		curr_proc_id = local_slave_ids[i];
		SignalValue(SLAVE_FLAG(curr_proc_id), 1U);
	}
}

ALWAYS_INLINE void
gomp_hal_barrier(MSGBarrier b)
{
  unsigned int myid, nthreads;
  gomp_team_t *team;
  unsigned int* proc_ids;
  
  myid = prv_proc_num;
  team = (gomp_team_t *) CURR_TEAM(myid);
  
  nthreads = team->nthreads;
  proc_ids = team->proc_ids;
  
  /* We can fetch master core ID looking 
   * at the first position of proc_ids */
  if(myid == proc_ids[0])
  {
    MSGBarrier_Wait(b, nthreads, proc_ids);
    MSGBarrier_Release(b, nthreads, proc_ids);
  }
  else
    MSGBarrier_SlaveEnter(b, myid);
  
}
