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

#ifndef __DAG_H__
#define __DAG_H__

/** Units */

/* Size of graph name in chars */
#define GRAPHNAME_MAX_LEN 256

/* Used in temp node representations */
#define MAX_PREDS 128L
#define MAX_SUCCS 128L

/** Some utils */
#ifndef _mycast
#define _mycast_ (unsigned int) (uintptr_t) 
#endif /* _mycast_ */

/** Datatypes */
typedef long long wcet_t;
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
	
	long countNodesWithoutWcet;
	long countNodesWithoutMapping;
};

/** Prototypes */
// CRUD
/* This is defined in dealer module */
struct dep;

int dagCreateEmpty(long nnodes, struct dag_t ** dag);
int dagCreate(char * graphName, int tdg_id, long *nodes, wcet_t *wcets, thread_t *nodeMaps, long nnodes, struct dep *deps, long ndeps, char makeSingleSrcSink, struct dag_t **dag);
void dagDispose(struct dag_t *dag);

/* Read from file: D, T, etc... */
int dagAssignParams(struct dag_t *dag, char ignoreDeadlinesFromFile);

void dagDeleteExistingMaps(struct dag_t **dags, int dagsLength);
long dagFindNodeIdx(struct node_t where[], long whereSize, struct node_t * who);
void dagDeleteExistingMaps(struct dag_t **dags, int dagsLength);

#endif /* __DAG_H__ */
