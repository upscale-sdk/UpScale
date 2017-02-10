/* Copyright (C) 2010, 2017 DEI - Universita' di Bologna
   Contributed by:
   Alessandro Capotondi <alessandro.capotondi@unibo.it>
   Daniele Cesarini <daniele.cesarini@unibo.it>
   Andrea Marongiu  <a.marongiu@unibo.it>
   Giuseppe Tagliavini <giuseppe.tagliavini@unibo.it>
*/

/* This file is part of the GNU OpenMP Library (libgomp).

   Libgomp is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   Libgomp is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
   FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
   more details.

   Under Section 7 of GPL version 3, you are granted additional
   permissions described in the GCC Runtime Library Exception, version
   3.1, as published by the Free Software Foundation.

   You should have received a copy of the GNU General Public License and
   a copy of the GCC Runtime Library Exception along with this program;
   see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
   <http://www.gnu.org/licenses/>.  */
          
/*	This file includes barrier management functions for the Kalray MPPA 256 BSP */

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
