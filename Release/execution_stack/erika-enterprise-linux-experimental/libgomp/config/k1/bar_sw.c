
#include "bar.h"
#include "libgomp_config.h"
#include "appsupport.h"
#include "libgomp.h"

/* This is the barrier code executed by each SLAVE core */
void
MSGBarrier_SlaveEnter(MSGBarrier b, int myid)
{
	/* Point to my SLAVE flag */
	//_DTYPE *flag = (volatile _DTYPE *) SLAVE_FLAG(myid);
	/* Read start value */
	//volatile _DTYPE g = *flag;
	_DTYPE g = __k1_umem_read32(SLAVE_FLAG(myid));

	/* Notify the master I'm on the barrier */
	//*(MASTER_FLAG(myid)) = 1;
	__k1_umem_write32(MASTER_FLAG(myid),1U);

	while(1)
	{

		//volatile _DTYPE *exit = SLAVE_FLAG(myid);
  	_DTYPE exit = __k1_umem_read32(SLAVE_FLAG(myid));

		if (g == exit)
		{
			continue;
		}
		break;
	}
}

/* This is the barrier code executed by the MASTER core to gather SLAVES */
ALWAYS_INLINE void
MSGBarrier_Wait(MSGBarrier b, int num_threads, unsigned int *local_slave_ids)
{
	unsigned int curr_proc_id, i;

	for(i = 1; i <num_threads; i++)
	{
		curr_proc_id = local_slave_ids[i];
		while( !(__k1_umem_read32(MASTER_FLAG(curr_proc_id))) )
			{ continue; }
		
		/* Reset flag */
		//*(MASTER_FLAG(curr_proc_id)) = 0;
  	__k1_umem_write32(MASTER_FLAG(curr_proc_id),0U);
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
		//volatile _DTYPE *exit = (volatile _DTYPE *) SLAVE_FLAG(curr_proc_id);
		
		/* Increase exit count */
		//(*exit)++;
		_DTYPE tmp = __k1_umem_read32(SLAVE_FLAG(curr_proc_id));
		__k1_umem_write32(SLAVE_FLAG(curr_proc_id),tmp+1);
	}
}

ALWAYS_INLINE void
MSGBarrier_Release_all(MSGBarrier b, int num_threads, unsigned int *local_slave_ids)
{
	unsigned int curr_proc_id, i;

	for(i = 1; i < num_threads; i++)
	{
		curr_proc_id = local_slave_ids[i];
		
		//volatile _DTYPE *exit = (volatile _DTYPE *) SLAVE_FLAG(curr_proc_id);
		
		/* Increase exit count */
		//(*exit)++;
		_DTYPE tmp = __k1_umem_read32(SLAVE_FLAG(curr_proc_id));
		__k1_umem_write32(SLAVE_FLAG(curr_proc_id),tmp+1);
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
