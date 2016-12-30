/* Copyright 2014 DEI - Universita' di Bologna

author    DEI - Universita' di Bologna
          Alessandro Capotondi - alessandro.capotondi@unibo.it
          Giuseppe Tagliavini  - giuseppe.tagliavini@unibo.it
          Andrea Marongiu      - amarongiu@unibo.it
          
info      barrier management */

#ifndef __BAR_H__
#define __BAR_H__

//#include "libgomp_globals.h"

#if   defined BAR_SW
#define _DTYPE 	int
#elif defined BAR_SIGWAIT
#define _DTYPE 	BlockableValueType
#endif
                            
/* MSG Barrier defined in libgomp_globals.h */           
#define BARRIER_BASE    		(gomp_mem_ptr[get_slot_id()])
#define SLAVE_FLAG(x)       (_DTYPE *) (BARRIER_BASE + x * sizeof(_DTYPE))
#define MASTER_FLAG(x)      (_DTYPE *) (BARRIER_BASE + DEFAULT_MAXPROC*sizeof(_DTYPE) + x * sizeof(_DTYPE))


typedef volatile _DTYPE * MSGBarrier;
//extern void MSGBarrier_SlaveEnter(MSGBarrier b, int myid);
/* if numProcs == NPROCS, we don't need to provide slave_ids */
//extern void MSGBarrier_Wait(MSGBarrier b, int numProcs, unsigned int *local_slave_ids);
//extern void MSGBarrier_Release(MSGBarrier b, int numProcs, unsigned int *local_slave_ids);
//extern void MSGBarrier_Wake(int numProcs, unsigned int *local_slave_ids);

/* MS Barrier */

/* Counter Master-Slave Barrier */
#define MS_BARRIER_TYPE									              MSGBarrier
#define MS_BARRIER_SIZE									              (DEFAULT_MAXPROC * sizeof(_DTYPE) * 2)
#define MSlaveBarrier_Wait(b, numProcs, mask) 			  MSGBarrier_Wait(b, numProcs, mask)
#define MSlaveBarrier_SlaveEnter(b, myid) 				    MSGBarrier_SlaveEnter(b, myid)
#define MSlaveBarrier_Release_all(b, numProcs, mask)  MSGBarrier_Release_all(b, numProcs, mask)
#define MSlaveBarrier_Release(b, numProcs, mask) 		  MSGBarrier_Release(b, numProcs, mask)
#define MSlaveBarrier_Wake(numProcs, mask) 				    MSGBarrier_Wake(numProcs, mask)

//extern void gomp_hal_barrier(MS_BARRIER_TYPE b);
#endif
