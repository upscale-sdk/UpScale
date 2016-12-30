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

#define _mycast_ (unsigned int) (uintptr_t) 
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

/* Defined in dealer.c */
struct dep;
typedef long long wcet_t;
typedef enum { NONE = 0, SCHED_MAP_STATIC, SCHED_DYNAMIC, SCHED_RR /* For debug purposes */, ANALYZE_STATIC } scheduling_t;
typedef enum { MAPPING_UNKNOWN = 0, MAPPING_RANDOM, MAPPING_LNSNL, MAPPING_FIRSTFIT, MAPPING_BESTFIT } mapping_t;
typedef enum { TIMING_NONE = 0, TIMING_MIET, TIMING_MEET, TIMING_MAET } timing_t;
typedef int thread_t;

/* v = struct('pred', {}, 'succ', {}, 'cond', {}, 'depth', {}, 'width', {}, 'C', {}, 'accWorkload', {}, 'condPred', {}, 'branchList', {}); */
struct node_t {
	long idMercurium;
	long dealerIdx; // to quickly build the dep table when we are done
	
	struct node_t **preds;
	struct node_t **succs;
	long num_preds;
	long num_succs;
        
	// long cond;
	// long depth;
	// long width;
	wcet_t C;
	wcet_t accWorkload;
	// long condPred;
	// long branchList;
	
	// Statich thread mapping
	wcet_t R;
	thread_t thread;
	wcet_t tryR;
	thread_t tryThread;
};

/* Size of graph name in chars */
#define GRAPHNAME_MAX_LEN 256

/* Used in temp node representations */
#define MAX_PREDS 128L
#define MAX_SUCCS 128L

/* task  = struct('v', {}, 'D', {}, 'T', {}, 'wcw', {}, 'vol', {}, 'len', {}, 'R', {}, 'mksp', {}, 'Z', {}); */
struct dag_t {
	struct node_t* v; // nodes
	long num_nodes;
	long D;
	long T;
// 	wcet_t wcw;
	wcet_t vol;
	long len;
	wcet_t R;
// 	long mksp;
	float Z; // was long
        
	char name[GRAPHNAME_MAX_LEN];
	int tdg_id;
	char sched; // 1: (yes) | 0 (no) | -1 (unknown) TODO an enum
};

/** Prototypes */

int dagCreate(char * graphName, int tdg_id, long *nodes, wcet_t *wcets, thread_t *nodeMaps, long nnodes, struct dep *deps, long ndeps, struct dag_t **dag);
void dagDispose(struct dag_t *dag);

/* Read from file: D, T, etc... */
int dagAssignParams(struct dag_t *dag, char ignoreDeadlinesFromFile);

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

int cmpDAGsTdgId(const void *p1, const void *p2);

// TODO remove
long setdiff(struct node_t **A, long ASize, struct node_t **B, long BSize, struct node_t **res);
void printNodesSet(const char * txt, struct node_t **nodes, long nnodes);

#endif /* __SCHED_H__*/
