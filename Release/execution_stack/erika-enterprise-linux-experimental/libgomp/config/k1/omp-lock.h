/* Copyright 2014 DEI - Universita' di Bologna

author    DEI - Universita' di Bologna
          Alessandro Capotondi - alessandro.capotondi@unibo.it
          Giuseppe Tagliavini  - giuseppe.tagliavini@unibo.it
          Andrea Marongiu      - amarongiu@unibo.it
          
info      lock management */

#ifndef __OMP_LOCK_H__
#define __OMP_LOCK_H__

#include "ee.h"

typedef SpinlockObjType omp_lock_t;

void omp_set_lock (omp_lock_t *lock);
void omp_unset_lock (omp_lock_t *lock);

#endif
