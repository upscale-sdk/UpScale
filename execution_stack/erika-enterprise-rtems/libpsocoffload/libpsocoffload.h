/* Copyright (C) 2015, 2017 DEI - Universita' di Bologna
   Contributed by:
   Alessandro Capotondi <alessandro.capotondi@unibo.it>
   Daniele Cesarini <daniele.cesarini@unibo.it>
   Andrea Marongiu  <a.marongiu@unibo.it>
   Giuseppe Tagliavini <giuseppe.tagliavini@unibo.it>
*/

/* Libpsocoffload is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   Libpsocoffload is distributed in the hope that it will be useful, but WITHOUT ANY
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


#ifndef INCLUDE_LIBPSOCOFFLOAD_H
#define INCLUDE_LIBPSOCOFFLOAD_H

#include "shared_defs.h"

// ARCH_ANDEY  = 0
// ARCH_BOSTAN = 1
static const int erika_enterprise_version = 0;

extern void GOMP_init(int device);
extern void GOMP_target(int device, void (*fn) (void *), const void *target_fn, void *data, unsigned int slot_id, unsigned int mask);
extern void GOMP_target_wait(int device, void *data, unsigned int slot_id);
void GOMP_deinit(int device);

#endif /* INCLUDE_LIBPSOCOFFLOAD_H */
