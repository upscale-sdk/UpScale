/* Copyright 2014 DEI - Universita' di Bologna

author    DEI - Universita' di Bologna
          Alessandro Capotondi - alessandro.capotondi@unibo.it
          Giuseppe Tagliavini  - giuseppe.tagliavini@unibo.it
          Andrea Marongiu      - amarongiu@unibo.it

info      support functions */

#ifndef __APPSUPPORT_H__
#define __APPSUPPORT_H__

#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include "ee.h"

#include "config.h"

#define ASSERT_LENGTH 40U
EE_k1_spinlock     assertions_lock;
EE_TYPEASSERTVALUE EE_assertions[ASSERT_LENGTH];

#ifndef INLINE
# if __GNUC__ && !__GNUC_STDC_INLINE__
#  define INLINE extern inline
# else
#  define INLINE static inline
# endif
#endif

//FIXME!
#define ALWAYS_INLINE INLINE

INLINE unsigned int get_proc_id() {
    return EE_get_curr_core_id ();
}

INLINE unsigned int get_proc_num(){
    return EE_K1_CORE_NUMBER-1;
}

INLINE unsigned int get_time() {
    return 0;
}

INLINE unsigned int get_cycle(){
    return 0;
}


#endif // __APPSUPPORT_H__
