/* Copyright 2014 DEI - Universita' di Bologna

author    DEI - Universita' di Bologna
          Alessandro Capotondi - alessandro.capotondi@unibo.it
          Giuseppe Tagliavini  - giuseppe.tagliavini@unibo.it
          Andrea Marongiu      - amarongiu@unibo.it
          
info      memory management */

#ifndef __MEMUTILS_H__
#define __MEMUTILS_H__

extern inline void print_shmem_utilization(void);
extern inline void *shmalloc(unsigned int);
extern inline void shfree(void *);

#endif /* __MEMUTILS_H__ */