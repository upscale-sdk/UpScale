/* -----------------------------------------------------------------------------
 * Author list (in alphabetic order):
 * 
 *        Alessandra Melani <al.melani@sssup.it>
 *        Marko Bertogna <marko.bertogna@unimore.it>
 *        Paolo Burgio <paolo.burgio@unimore.it>
 * 
 * -----------------------------------------------------------------------------
 * (C) Copyright 2016 Barcelona Supercomputing Center, 
 *                    Università degli Studi di Modena e Reggio Emilia,
 *                    Scuola superiore Sant'Anna
 * 
 * This file is part of psoc_mapper (Compilation flow) of the P-SOCRATES project.
 *
 * Psoc_mapper is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published 
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Psoc_mapper is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with psoc_mapper. If not, see <http://www.gnu.org/licenses/>.
 * -----------------------------------------------------------------------------
 */

#ifndef __SCHED_H__
#define __SCHED_H__

#include "dag.h"

/** A few utils */

// #define DEBUG

#ifdef DEBUG
#   define _log(_s, ...)                           \
    {                                              \
      printf("[%s] " _s, __func__, ##__VA_ARGS__); \
    }
#else
#	define _log(...)
#endif

#define min(_l1, _l2) ( _l1 < _l2 ? _l1 : _l2 )


/** Support for float comparison */
#include "float.h"
static inline int
FloatGT(float f1, float f2) {
	int ret = (f1-f2) > FLT_EPSILON;
	return ret;
}

static inline int
FloatST(float f1, float f2) {
	int ret = (f2-f1) > FLT_EPSILON;
	return ret;
}

static inline int
FloatEQ(float f1, float f2) {
	int ret = (f1 - f2) < FLT_EPSILON && (f2 - f1) < FLT_EPSILON;
	return ret;
}

static inline int
FloatNE(float f1, float f2) {
	int ret = FloatEQ(f1, f2) ? 0 : 1;
	return ret;
}

static inline int
FloatGE(float f1, float f2) {	
	int ret = FloatST(f1, f2) ? 0 : 1;
	return ret;
}

static inline int
FloatSE(float f1, float f2) {	
	int ret = FloatGT(f1, f2) ? 0 : 1;
	return ret;
}


/** Data types and data structures */

typedef enum { NONE = 0, SCHED_MAP_STATIC, SCHED_DYNAMIC, SCHED_RR /* For debug purposes */, ANALYZE_STATIC } scheduling_t;
typedef enum { MAPPING_UNKNOWN = 0, MAPPING_RANDOM, MAPPING_LNSNL, MAPPING_FIRSTFIT, MAPPING_BESTFIT } mapping_t;
typedef enum { TIMING_NONE = 0, TIMING_MIET, TIMING_MEET, TIMING_MAET } timing_t;

/** Prototypes */

/* Dynamic sched analysis */
int dynamicSchedulabilityAnalysis(struct dag_t **dags, int dagsLength, unsigned int ncores);

/* Static sched analysis */
int staticSchedulabilityAnalysis(struct dag_t **dags, int dagsLength, unsigned int ncores);

/* Static sched analysis and mapping */
int staticSchedulabilityAnalysisAndMapping(struct dag_t **dags, int dagsLength, unsigned int ncores);
/* Fetch results */
void staticGetMapping(struct dag_t * dag, thread_t * mappings);

/* Fetch results */
void rrGetMapping(struct dag_t * dag, thread_t * mappings, unsigned int ncores);

/* Some useful utils */
int cmpDAGsTdgId(const void *p1, const void *p2);

#endif /* __SCHED_H__*/
