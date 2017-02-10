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

/* This file lists all the files to be included for compilation.  */

#include "appsupport.h"
#include "config.h"
#include "hal.h"
#include "memutils.h"
#include "mutex.h"
#include "bar.h"
#include "omp-lock.h"

#include "libgomp.h"
#include "libgomp_globals.h"
#include "libgomp_config.h"

/* SOURCES */

#include "hal-root.c"
#include "barrier.c"
#include "critical.c"
#include "env.c"
#include "iter.c"
#ifdef TASKING_ENABLED
#ifdef TASK_CORE_STATIC_SCHED
#include "task_private.c"
#else
#include "task.c"
#endif
#ifdef STATIC_TDG_ENABLED
#include "tdg.c" 
#endif
#endif // TASKING_ENABLED
#include "libgomp.c"
#include "work.c"
#include "team.c"
//#include "loop.c"
#include "parallel.c"
//#include "sections.c"
#include "single.c"

