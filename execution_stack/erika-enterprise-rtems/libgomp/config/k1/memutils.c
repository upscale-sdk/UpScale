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
          
/*	This file includes memory management functions for the Kalray MPPA 256 BSP */

#include "appsupport.h"
#include "libgomp.h"

//#define shmem_next SHMEM_NEXT
//#define MEMCHECK_MALLOCS
//#define STACK_IN_SHARED

inline void print_shmem_utilization() {
	//_printdecp("Heap occupation (in bytes) is", ((unsigned int) shmem_next) - SHARED_BASE);
}


void shmalloc_init(unsigned int address) {
	//shmem_next = SHARED_BASE + address;
	//SHMEM_LOCK = (unsigned int) LOCKS(SHMALLOC_LOCK_ID);
}

inline void *shmalloc(unsigned int size) {

	return memalign(8, size);
}

void shfree(void *address) {
  free(address);
}

