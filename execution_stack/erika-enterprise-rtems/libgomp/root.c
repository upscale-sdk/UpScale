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

