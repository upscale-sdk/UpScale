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
   info         Libgomp main header file */

#ifndef __LIBGOMP_H__
#define __LIBGOMP_H__

#include <stddef.h>

/* Data structures for offload shared between host and target */
#include "shared_defs.h"

/* Platform appsupport */
#include "appsupport.h"

/* Platform HAL */
#include "hal.h"

/* omp API */
#include "omp.h"

/* library configuration */
#include "libgomp_config.h"

/* ... */
#include "context.h"

/* library global data definition */
#include "libgomp_globals.h"

/* Function prototypes. */

//NOTE not yet implemented
#if 0
/* affinity.c */

extern void gomp_init_affinity (void);
extern void gomp_init_thread_affinity (pthread_attr_t *);

/* alloc.c */

extern void *gomp_malloc (size_t) __attribute__((malloc));
extern void *gomp_malloc_cleared (size_t) __attribute__((malloc));
extern void *gomp_realloc (void *, size_t);

/* Avoid conflicting prototypes of alloca() in system headers by using
 *   GCC's builtin alloca().  */
#define gomp_alloca(x)  __builtin_alloca(x)

/* error.c */

extern void gomp_error (const char *, ...)
__attribute__((format (printf, 1, 2)));
extern void gomp_fatal (const char *, ...)
__attribute__((noreturn, format (printf, 1, 2)));

extern int gomp_iter_static_next (long *, long *);
extern bool gomp_iter_guided_next_locked (long *, long *);
extern bool gomp_iter_guided_next (long *, long *);


/* iter_ull.c */

extern int gomp_iter_ull_static_next (unsigned long long *,
unsigned long long *);
extern bool gomp_iter_ull_dynamic_next_locked (unsigned long long *,
unsigned long long *);
extern bool gomp_iter_ull_guided_next_locked (unsigned long long *,
unsigned long long *);

#if defined HAVE_SYNC_BUILTINS && defined __LP64__
extern bool gomp_iter_ull_dynamic_next (unsigned long long *,
unsigned long long *);
extern bool gomp_iter_ull_guided_next (unsigned long long *,
unsigned long long *);
#endif

/* ordered.c */

extern void gomp_ordered_first (void);
extern void gomp_ordered_last (void);
extern void gomp_ordered_next (void);
extern void gomp_ordered_static_init (void);
extern void gomp_ordered_static_next (void);
extern void gomp_ordered_sync (void);
#endif

/* iter.c */
//extern int gomp_iter_dynamic_next_locked (gomp_work_share_t *, long *, long *);
//extern int gomp_iter_dynamic_next (gomp_work_share_t *, long *, long *);


/* team.c */

//extern /*inline*/ int gomp_resolve_num_threads (int);
//extern /*inline*/ gomp_team_t *gomp_new_team();
//extern /*inline*/ void gomp_free_team(gomp_team_t *);
//extern /*inline*/ void gomp_master_region_start (void *, void *, int, gomp_team_t **); //NOTE custom
//extern /*inline*/ void gomp_master_region_end (gomp_team_t *);
//extern /*inline*/ void gomp_team_start (void *, void *, int, gomp_team_t **);
//extern /*inline*/ void gomp_team_end();
//extern void gomp_master_region_start (void *fn, void *data, int specified, gomp_team_t **team);
#ifdef TASKING_ENABLED
extern void gomp_task_scheduler ( void );
//extern void gomp_task_scheduler(gomp_team_t *team);
//extern void gomp_init_task (struct gomp_task *, struct gomp_task * /*, struct gomp_task_icv * */);
//extern void gomp_end_task (void);
//extern void gomp_barrier_handle_tasks (/*gomp_barrier_state_t*/);
#endif

/* work.c */

//extern /*inline*/ gomp_work_share_t *gomp_new_work_share(void);
//extern /*inline*/ void gomp_free_work_share (gomp_work_share_t *);
//extern /*inline*/ int gomp_work_share_start (gomp_work_share_t **);
//extern /*inline*/ void gomp_work_share_end_nowait ();


/* GOMP declarations */

/* barrier.c */

extern void GOMP_barrier (void);

/* critical.c */

extern void GOMP_critical_start (void);
extern void GOMP_critical_end (void);
extern void GOMP_atomic_start (void);
extern void GOMP_atomic_end (void);

/* loop.c */

extern int GOMP_loop_dynamic_start(long, long, long, long, long *, long *);
extern int GOMP_loop_dynamic_next (long *, long *);
extern void GOMP_parallel_loop_dynamic (void (*) (void *), void *, unsigned, long, long, long, long, unsigned);
extern void GOMP_parallel_loop_dynamic_start (void (*) (void *), void *, unsigned, long, long, long, long);
extern void GOMP_loop_end(void);
extern void GOMP_loop_end_nowait(void);

/* parallel.c */

extern void GOMP_parallel (void (*) (void*), void *, int, unsigned int);
extern void GOMP_parallel_start (void *, void *, int);
extern void GOMP_parallel_end (void);

/* sections.c */

extern void GOMP_parallel_sections(void (*) (void *), void *, unsigned, unsigned, unsigned);
extern int GOMP_sections_start (int);
extern void GOMP_sections_end(void);
extern void GOMP_sections_end_nowait(void);
extern int GOMP_sections_next(void);
extern void GOMP_parallel_sections_start(void (*) (void *), void *, unsigned, unsigned);


/* single.c */

extern int GOMP_single_start(void);
extern void *GOMP_single_copy_start(void);
extern void GOMP_single_copy_end(void *);

struct _omp_data_msg {
  char *memory_area;   // Area for libgomp global structures
  int  num_params;
  char *function_args; // Args for the offloaded function (from the host)
};

/* KERNEL POINTERS */
typedef void (*omp_fn)(struct _omp_param *);
extern omp_fn omp_functions[];

typedef struct EE_k1_omp_message_tag {
  struct _omp_param *params;
  omp_fn             func;
  int                slot_id;
} __attribute__ ((aligned(_K1_DCACHE_LINE_SIZE))) EE_k1_omp_message;


#endif  /* __LIBGOMP_H__ */

