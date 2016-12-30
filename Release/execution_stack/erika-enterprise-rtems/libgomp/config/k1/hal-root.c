/* Copyright 2014 DEI - Universita' di Bologna

author    DEI - Universita' di Bologna
          Alessandro Capotondi - alessandro.capotondi@unibo.it
          Giuseppe Tagliavini  - giuseppe.tagliavini@unibo.it
          Andrea Marongiu      - amarongiu@unibo.it
*/

/* SOURCES */
#include "appsupport.c"
#include "lock.c"
#include "memutils.c"
#if   defined BAR_SW
#include "bar_sw.c"
#elif defined BAR_SIGWAIT
#include "bar_sigwait.c"
#endif

