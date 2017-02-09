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
//#define TASKING_ENABLED
#ifdef TASKING_ENABLED
# include "task.c"
#endif // TASKING_ENABLED
#include "libgomp.c"
#include "work.c"
#include "team.c"
#include "loop.c"
#include "parallel.c"
#include "sections.c"
#include "single.c"

