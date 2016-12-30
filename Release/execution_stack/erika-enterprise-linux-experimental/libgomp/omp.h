#ifndef __OMP_H__
#define __OMP_H__

/* Standard public APIs */

/* env.c */
void omp_set_num_threads (int);
void omp_set_dynamic (int);
int omp_get_dynamic (void);
void omp_set_nested (int);
int omp_get_nested (void);

/* libgomp.c */
int omp_get_num_procs(void);

/* parallel.c */
int omp_get_num_threads(void);
int omp_get_max_threads(void);
int omp_get_thread_num(void);
int omp_in_parallel(void);

double omp_get_wtime(void);
int omp_get_wtick(void);

#include "omp-lock.h"

#endif /* __OMP_H__ */

