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
          
/*	This file includes support functions for the Kalray MPPA 256 BSP */

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
