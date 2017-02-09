/* Copyright 2014 DEI - Universita' di Bologna

author    DEI - Universita' di Bologna
          Alessandro Capotondi - alessandro.capotondi@unibo.it
          Giuseppe Tagliavini  - giuseppe.tagliavini@unibo.it
          Andrea Marongiu      - amarongiu@unibo.it
          
info      mutex management */

#ifndef __MUTEX_H__
#define __MUTEX_H__

#include "omp-lock.h"

#define gomp_mutex_lock(x)		omp_set_lock(x)
#define gomp_mutex_unlock(x)	omp_unset_lock(x)

#endif // _MUTEX_H__
