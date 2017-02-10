/* Copyright (C) 2010, 2017 DEI - Universita' di Bologna
   Contributed by:
   Alessandro Capotondi <alessandro.capotondi@unibo.it>
   Daniele Cesarini <daniele.cesarini@unibo.it>
   Andrea Marongiu  <a.marongiu@unibo.it>
   Giuseppe Tagliavini <giuseppe.tagliavini@unibo.it>
*/

/* Copyright (C) 2005, 2009 Free Software Foundation, Inc.
   Contributed by Richard Henderson <rth@redhat.com>.

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

/* This file defines the OpenMP internal control variables, and arranges
   for them to be initialized from environment variables at startup.  */

#include "libgomp.h"

/************************* APIs *************************/

inline void 
omp_set_num_threads (int n)
{
    GOMP_WARN_NOT_SUPPORTED("omp_set_num_threads");
}

inline void
omp_set_dynamic (int val)
{
    //gomp_dyn_var = val;
}

inline int
omp_get_dynamic (void)
{
    //return gomp_dyn_var;
    return 0;
}

inline void
omp_set_nested (int val)
{
    //gomp_nest_var = val;
}

inline int
omp_get_nested (void)
{
    //return gomp_nest_var;
    return 0;
}
