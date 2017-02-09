/* Copyright (C) 2005-2014 Free Software Foundation, Inc.
 C ontributed by Richard Henderson <r*th@redhat.com>.
 
 This file is part of the GNU OpenMP Library (libgomp).
 
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

/* Copyright 2014 DEI - Universita' di Bologna
   author       DEI - Universita' di Bologna
                Alessandro Capotondi - alessandro.capotondi@unibo.it
				Giuseppe Tagliavini - giuseppe.tagliavini@unibo.it
   info         #pragma omp barrier implemetation */

#include "libgomp.h"

/* Application-level barrier */
void
GOMP_barrier()
{
  //printf("GOMP_barrier start\n");
    
#ifdef TASKING_ENABLED
  //printf("--->\n");
  gomp_task_scheduler();
  //printf("<---\n");
#endif /* TASKING_ENABLED */
  
  gomp_hal_barrier(0x0);
  //printf("GOMP_barrier end\n");
}
