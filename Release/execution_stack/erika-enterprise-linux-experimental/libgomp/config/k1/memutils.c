/* Copyright 2014 DEI - Universita' di Bologna

author    DEI - Universita' di Bologna
          Alessandro Capotondi - alessandro.capotondi@unibo.it
          Giuseppe Tagliavini  - giuseppe.tagliavini@unibo.it
          Andrea Marongiu      - amarongiu@unibo.it
          
info      memory management */

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
  return free(address);
}

