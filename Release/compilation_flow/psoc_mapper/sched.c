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

// #include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "sched.h"

// TODO move on DAG
char createdByMe = 0;

// FIXME inside dealer.c
void dumpnodes();
void dumpdeps();

struct dep {
	unsigned long src, dst;
};

char msg[1024];
#include <stdio.h>
#include <stdarg.h>
void _msg(char * format, ...) {
  va_list args;
  va_start (args, format);
  vsnprintf (msg, 10235, format, args);
  va_end (args);
}

void printNode(const struct node_t *node) {
	int i;
	printf("Node idMercurium %ld ", node->idMercurium);
// 	printf("address 0x%x ", _mycast_ node);
	printf("C %lld accWorkload %lld ", node->C, node->accWorkload);
// 	printf("preds 0x%x succs 0x%x", _mycast_ node->preds, _mycast_ node->succs);
// 	printf("\n");
	
	printf("| preds (%ld): ", node->num_preds);
	if(!node->num_preds)
		printf("-none- ");
	else {
		for(i=0; i<node->num_preds; i++)
			printf("%u ", _mycast_ node->preds[i]->idMercurium);
// 			printf("0x%x ", _mycast_ node->preds[i]);
	}
	
	printf("| succs (%ld): ", node->num_succs);
	if(!node->num_succs)
		printf("-none- ");
	else {
		for(i=0; i<node->num_succs; i++)
			printf("%u ", _mycast_ node->succs[i]->idMercurium);
	}
	printf("\n");
} // printNode

void printNodesSet(const char * txt, struct node_t **nodes, long nnodes) {
	printf("%s nnodes %ld: { ", txt, nnodes);
	int i;
	for(i=0; i<nnodes; i++) {
		printf("%ld ", nodes[i]->idMercurium);
	} // for
	
	printf("}\n");
} // printNodesSet

void printDag(const struct dag_t* dag) {
	int i;
	printf("dag %d (from digraph '%s') @ 0x%x ", dag->tdg_id, dag->name, _mycast_ dag);
	printf("len %ld vol %lld ", dag->len, dag->vol);
// 	printf("Z %ld ", dag->Z);
	printf("D %ld T %ld ", dag->D, dag->T);
	printf("\n#nodes %ld\n", dag->num_nodes);
	for(i=0; i<dag->num_nodes; i++)
		printNode(&dag->v[i]);
} // printDag

struct dag_t * newDag() {
	struct dag_t * dag = (struct dag_t *) malloc(sizeof(struct dag_t));
 	// _log("new dag @ 0x%x\n", _mycast_ dag);
	dag->v = NULL;
	dag->num_nodes = 0;
	dag->D = -1;
	dag->T = -1;
	dag->vol = -1;
	dag->len = -1;
	dag->R = -1;
	dag->Z = -1;
	
	strcpy(dag->name, "unknown");
	dag->tdg_id = -1;
	dag->sched = -1; // unknown
	
	return dag;
} // newDag

void dagDispose(struct dag_t * dag) {
	int i;
	
	if(dag) {
		for(i =0; i<dag->num_nodes; i++) {
			free(dag->v[i].preds);
			free(dag->v[i].succs);
		}
		free(dag->v);
		if(createdByMe)
			free(dag);
	}
} // disposeDag

long nodeIdx(struct node_t where[], long whereSize, struct node_t * who) {
	int i;
	for(i=0; i<whereSize; i++)
		if(where[i].idMercurium == who->idMercurium)
			return i;
		
	return -1L;
} // nodeIdx

void maxAccWorkload(struct node_t **nodes, long nnodes, wcet_t* max, int *maxIdx) {
	int j;
	*max = 0L;
	*maxIdx = -1;
	for(j=0; j<nnodes; j++)
		if(nodes[j]->accWorkload > *max) {
			*max = nodes[j]->accWorkload;
			*maxIdx = j;
		}
} // maxAccWorkload

void computeAccWorkload(struct node_t *v, long nnodes) {
	
	// NOTE Dealer already puts nodes in topo_logical order. Need to process inversely
	
	int i, maxIdx;
	wcet_t max;
	
	for(i=0; i<nnodes; i++) {
		v[i].accWorkload = v[i].C;
		if(v[i].num_preds) {
			maxAccWorkload(v[i].preds, v[i].num_preds, &max, &maxIdx);
			v[i].accWorkload += max;
		}
	} // for
} // computeAccWorkload

int dagCreateEmpty(long nnodes, struct dag_t ** dag) {
	// Allocate all nodes. Note that in "nodes" they are ordered :)
	int i;
	struct node_t *v = (struct node_t*) malloc(sizeof(struct node_t) * nnodes);
	if(!v)
		return 1;
	for(i =0; i<nnodes; i++) {
		v[i].idMercurium = -1;
		v[i].preds = NULL;
		v[i].succs = NULL;
		v[i].num_preds = 0;
		v[i].num_succs = 0;
		v[i].C = -1;
		v[i].accWorkload = -1;
		v[i].R = -1;
		v[i].thread = -1;
	}

	*dag = newDag();
	
	(*dag)->v = &v[0];
	(*dag)->num_nodes = nnodes;
	(*dag)->len = -1;
	(*dag)->vol = -1;
	(*dag)->R = -1;
	(*dag)->Z = -1;
	
	strcpy((*dag)->name, "");
	(*dag)->tdg_id = -1;
	(*dag)->sched = -1; // unknown
	
	return 0;
} // dagCreateEmpty

int tryMappingDagBak(struct dag_t *dst, struct dag_t *src) {
	int i;
	if(dst->num_nodes != src->num_nodes)
		return 1;
	
	for(i=0; i<dst->num_nodes; i++) {
		dst->v[i].R = src->v[i].R;
		dst->v[i].thread = src->v[i].thread;
	}
	
	dst->R = src->R;
	return 0;
} // tryMappingDagBak

/*
 * NOTE topo_logical order must be respected by contruction
 */
int dagCreate(char *graphName, int tdg_id, long* nodes, wcet_t *wcets, thread_t *nodeMaps, long nnodes, struct dep *deps, long ndeps, struct dag_t *dag[]) {
	int i,j;
	
// 	printf("createDag(tdg_id %d nnodes %ld)\n", tdg_id, nnodes);
// 	dumpnodes();
// 	dumpdeps();
	
	if(!dag) {
		printf("Internal structure (i.e., array of DAGs) not initialized\n");
		return 1;
	}
	
	if(strlen(graphName) >= GRAPHNAME_MAX_LEN) {
		printf("Graph name too long! (i.e., %d should be %d)\n", (int) strlen(graphName), GRAPHNAME_MAX_LEN);
		return 1;
	}
	
	// Allocate all nodes. Note that in "nodes" they are ordered :)
	struct node_t *v = (struct node_t*) malloc(sizeof(struct node_t) * nnodes);
	if(!v)
		return 1;
	for(i =0; i<nnodes; i++) {
		// FIXME do it in a separate fun?
		v[i].idMercurium = -1;
		v[i].dealerIdx = -1;
		void * datum = malloc(sizeof(struct node_t *) * MAX_PREDS);
		v[i].preds = datum;
		datum = malloc(sizeof(struct node_t *) * MAX_SUCCS);
		v[i].succs = datum;
		v[i].num_preds = 0;
		v[i].num_succs = 0;
		v[i].C = -1;
		v[i].accWorkload = -1;
		v[i].R = -1;
		v[i].thread = -1;
	}
	
	unsigned int *indexes = (unsigned int *) malloc((nodes[nnodes-1]+1) * sizeof(int));
	
	for(i=0; i<nnodes; i++) {
		v[i].idMercurium = nodes[i];
		v[i].dealerIdx = i;
		v[i].C = wcets[i];
		v[i].thread = nodeMaps[i];
		indexes[nodes[i]] = i;
	}
	
	struct node_t *src, *dst;
	for(i=0; i<ndeps; i++) {
		src = &v[indexes[deps[i].src]];
		dst = &v[indexes[deps[i].dst]];
		
		if(src->num_succs >= MAX_SUCCS) {
			printf("Too many successors for node %ld (%ld instead of %ld)!", src->idMercurium, src->num_succs, MAX_SUCCS);
			return 2;
		}
		if(dst->num_preds >= MAX_PREDS) {
			printf("Too many predecessors for node %ld (%ld instead of %ld)!", dst->idMercurium, dst->num_preds, MAX_PREDS);
			return 2;
		}
		
		src->succs[src->num_succs++] = dst;
		dst->preds[dst->num_preds++] = src;
	}
	free(indexes);
	
/*  task(i).v = computeAccWorkload(task(i).v); */
	computeAccWorkload(v, nnodes);
/*  [~, q] = max([task(i).v.accWorkload]); */
	wcet_t max = 0;
	long q = -1;
	wcet_t vol = 0L;
	
	for(j=0; j<nnodes; j++) {
		if(v[j].accWorkload > max)
			q = j;
		vol += v[j].C;
	}	
	
	if(!*dag) {
		createdByMe = 1;
		*dag = newDag();
		// printf("Created new dag @ 0x%x\n", _mycast_ *dag);
	}
	
	if(*dag == 0) {
		printf("Out of memory!\n");
		return 1;
	}
	
	(*dag)->v = &v[0];
	(*dag)->num_nodes = nnodes;
	(*dag)->len = v[q].accWorkload;
	(*dag)->vol = vol;
	(*dag)->R = -1;
	(*dag)->Z = -1;
	(*dag)->sched = -1; // unknown
	
	strcpy((*dag)->name, graphName);
	(*dag)->tdg_id = tdg_id;
	
	// we need the #cores (aka 'm') to compute Z: can't do this here
/* 	task(i).Z = task(i).len + (1 / m) * (task(i).wcw - task(i).len);    % simple upper-bound on intra-task interference */
	 
	return 0;
	
} // createDag

void dagDeleteExistingMaps(struct dag_t **dags, int dagsLength) {
	/* Delete existing map */
	int ind, i;
	for(ind=0; ind<dagsLength; ind++) {
		printf("Deleting existing MAP for dag %d (from digraph '%s')", dags[ind]->tdg_id, dags[ind]->name);
		for(i=0; i<dags[ind]->num_nodes; i++) {
			dags[ind]->v[i].thread = -1;
		}
	} // for
} // dagDeleteExistingMaps

int dagAssignParams(struct dag_t *dag, char ignoreDeadlinesFromFile) {
	FILE *fp;
	char buff[32];
	long D, T;
	char * fName = "dagParams.txt";
	fp = fopen(fName, "r");
	if(fp == NULL) {
		printf("Impossible to load params for DAG. File '%s' not found!\n", fName);
		return 1;
	}
	while(!feof(fp)) {
		fscanf(fp, "%s", buff);
		fscanf(fp, "%ld", &D);
		fscanf(fp, "%ld", &T);
		
		// "G0" results in tdg_id == 0...check that the string read is actually a number
		if(buff[0] > '9' || buff[0] < '0')
			return 1;
			
// 		if(!strcmp(dag->name, buff)) {
		int tdg_id = atoi(buff);
		if(dag->tdg_id == tdg_id) {
			// here I am!
			printf("Assigning D %ld and T %ld to %d. Buff is %s\n", D, T, dag->tdg_id, buff);
			dag->D = ignoreDeadlinesFromFile ? T : D;
			dag->T = T;
			break;
		}
		
	}
	fclose(fp);
// 	printDag(dag);
	return 0;
} // assignDAGParams

int cmpDAGsRateMonotonic(const void *p1, const void *p2)
{
	struct dag_t* d1 = * (struct dag_t **) p1, *d2 = * (struct dag_t **) p2;
	return (d1->D < d2->D ? -1 : 1);
} // cmpDAGsRateMonotonic

int cmpDAGsTdgId(const void *p1, const void *p2)
{
	struct dag_t* d1 = * (struct dag_t **) p1, *d2 = * (struct dag_t **) p2;
	return (d1->tdg_id < d2->tdg_id ? -1 : 1);
} // cmpDAGsTdgId

int cmpNodesByIdMercurium(const void *p1, const void *p2)
{
	struct node_t* n1 = * (struct node_t **) p1, *n2 = * (struct node_t **) p2;
	return (n1->idMercurium < n2->idMercurium? -1 : 1);
} // cmpDAGsRateMonotonic


/** Dynamic strategy */

#include <math.h>
float dynamicGetInterf_2(const struct dag_t * dag, float L, unsigned int m) {
	//_log("dag->name %s\n",dag->name);
	float q1 = dag->vol;
	float temp = L + dag->R - (dag->vol / m);
	float q2 = m * (((wcet_t) ceil(temp) ) % dag->T);
	float mymin = FloatSE(q1, q2) ? q1 : q2;
	//_log("q1 %f q2 %f temp %f dag->vol %ld dag->R %f dag->T %ld\n", q1, q2, temp, dag->vol, dag->R, dag->T);
	float res = dag->vol * floor(ceil(temp) / dag->T) + mymin;
	//_log("returning %f\n", res);
	return res;
} // getInterf_2

// function [R, sched] = runRTA_FP_2(ind, makespan)
int dynamicRunRTA_FP_2(struct dag_t *dag, float makespan, struct dag_t *interferingDags[], int interferingDagsLength,  unsigned int m, float *R) {
	*R = 0;
	char init = 1;
	int i;
	float Rold = makespan;

	if(FloatGT(Rold, dag->D)) {
		return 0; // not schedulable
	}
	
	while (FloatNE(*R, Rold) && FloatSE(*R, dag->D) ) {
		if(!init) {
			Rold = *R;
			*R = 0;
		}
		
		// CHECK does this mean this is the highest priority?
		if(interferingDagsLength != 0) {
			
			for(i=0; i<interferingDagsLength; i++)
				(*R) += 1./m * dynamicGetInterf_2(interferingDags[i], Rold, m);
			
			//*R = floor(*R);
			(*R) += makespan;
		}
		else
			*R = Rold;
		
		init = 0;
	}
	
	if(FloatEQ(Rold, *R)) {
		return 1; // schedulable
	}
	else if(FloatGT(*R, dag->D)) {
		return 0; // not schedulable
	}
	else {
	}

	return 1; // schedulable
} // runRTA_FP_2

void dynamicComputeWorstCaseResponseTime(struct dag_t *dag, unsigned int ncores, struct dag_t* interferingDags[], int interferingDagsLength, float *R, int *sched)
{
	// int i;
	// _log("dag %s ncores %u interferingDagsLength %d\n", dag->name, ncores, interferingDagsLength);
	// for(i=0; i<interferingDagsLength; i++)
		// _log("- interferingDag[%d] %s\n", i, interferingDags[i]->name);
	
	// First two terms of the equation
	dag->Z = dag->len + 1.0/ncores* (dag->vol - dag->len);
/* 	task(i).Z = task(i).len + (1 / m) * (task(i).wcw - task(i).len);    % simple upper-bound on intra-task interference */
	*R = 0.;
	*sched = 0;
	
	*sched = dynamicRunRTA_FP_2(dag, dag->Z, interferingDags, interferingDagsLength, ncores, R);
	
} // computeWorstCaseResponseTime

int dynamicSchedulabilityAnalysis(struct dag_t **dags, int dagsLength, unsigned int ncores) {
	
	int i, sched, ret;
	float R;
	
	// Delete existing map because I will produce a new one */
	dagDeleteExistingMaps(dags, dagsLength);

	qsort(dags, dagsLength, sizeof(struct dag_t *), cmpDAGsRateMonotonic);
	
	ret = 1; // = schedulable
	for(i=0; i<dagsLength; i++) {
		
		int interferingDagsLength;
		// Ugly. Do it in a more efficient way
		for(interferingDagsLength=0; interferingDagsLength<dagsLength; interferingDagsLength++)
// 			if(!strcmp(dags[interferingDagsLength]->name, dags[i]->name)) {
			if(dags[interferingDagsLength]->tdg_id == dags[i]->tdg_id) {
				break;
			}
			
		dynamicComputeWorstCaseResponseTime(dags[i], ncores, dags, interferingDagsLength, &R, &sched);
		
		dags[i]->R = (wcet_t) ceil(R);
		dags[i]->sched = sched;
		
		_log("DAG %d has R %lld and deadline %ld: it %s schedulable\n", dags[i]->tdg_id, dags[i]->R, dags[i]->D, dags[i]->sched ? "is" : "is not");
		
		if(!sched) {
			ret = 0;
			break;
		}
	}// for
	
	return ret;
} // dynamicSchedulabilityAnalysis

/** Static schedulability analysis and mapping */

/** matlab's */

// Lia = ismember(A,B) returns an array containing 1 (true) where the data in A is found in B. Elsewhere, it returns 0 (false).
char ismember(struct node_t *A, struct node_t **B, long BSize) {
	long i;
// 	_log(" A %ld, B %ld, BSize %ld\n", A->idMercurium, BSize == 0 ? -1 : B[0]->idMercurium, BSize);
	
	for(i=0; i<BSize; i++) {
		if(B[i]->idMercurium == A->idMercurium) {
			return 1;
		}
	}
	
	return 0;
} // ismember

// C = setdiff(A,B) returns the data in A that is not in B.
// 
//     If A and B are numeric arrays, logical arrays, character arrays, categorical arrays, datetime arrays, duration arrays, or cell arrays of strings, then setdiff returns the values in A that are not in B. The values of C are in sorted order.
long setdiff(struct node_t **A, long ASize, struct node_t **B, long BSize, struct node_t **res) {
	int ia, ib, ic = 0, found;
// 	printNodesSet("A is", &A[0], ASize);
// 	printNodesSet("B is", &B[0], BSize);
	for(ia=0; ia<ASize; ia++) {
		found = 0;
		for(ib=0; ib<BSize; ib++) {
			if(A[ia]->idMercurium == B[ib]->idMercurium) {
				found = 1;
				continue;
			}
		} // for ib
		if(!found)
			res[ic++] = A[ia];
	} // for ia
	
// 	printNodesSet("C is", &res[0], ic);
	return ic;
} // setdiff

// C = union(A,B) returns the combined data from A and B with no repetitions.
// 
//     If A and B are numeric arrays, logical arrays, character arrays, categorical arrays, datetime arrays, duration arrays, or cell arrays of strings, then union returns the combined values from A and B. The values of C are in sorted order.

long _union(struct node_t **A, long ASize, struct node_t **B, long BSize, struct node_t **res) {
	int ia, ib, ic = 0, found;
	
// 	printNodesSet("A was", &A[0], ASize);
// 	printNodesSet("B was", &B[0], BSize);
	// we will sort C afterwards..
// 	qsort(A, ASize, sizeof(struct node_t *), cmpNodesByIdMercurium);
// 	qsort(B, BSize, sizeof(struct node_t *), cmpNodesByIdMercurium);
// 	printNodesSet("now A is", &A[0], ASize);
// 	printNodesSet("now B is", &B[0], BSize);
	
	for(ia=0; ia<ASize; ia++) {
		found = 0;
		for(ib=0; ib<BSize; ib++) {
			if(B[ib]->idMercurium == A[ia]->idMercurium) {
				found = 1;
				break;
			}
		} // for ib
		if(!found)
			res[ic++] = A[ia];
	} // for ia
	
	for(ib=0; ib<BSize; ib++) {
		found = 0;
		for(ia=0; ia<ASize; ia++) {
			if(B[ib]->idMercurium == A[ia]->idMercurium) {
				found = 1;
				break;
			}
		} // for ib
		if(!found)
			res[ic++] = B[ib];
	} // for ia
	
	qsort(res, ic, sizeof(struct node_t *), cmpNodesByIdMercurium);
// 	printNodesSet("C is", &res[0], ic);
	
	return ic;
} // _union

void staticGetInReachables(struct dag_t** dags, long ind, struct node_t *v, struct node_t ** inR, long *numInR, thread_t thread, mapping_t type) {
	thread_t threadPrev = 0;
	int i;
// 	_log("dag %s node w/idMercurium %ld num preds %ld thread (param) %ld\n", dags[ind]->name, v->idMercurium, v->num_preds, thread);
	
	if(v->num_preds != 0) {
		for(i=0; i<v->num_preds; i++) {
			threadPrev = v->preds[i]->thread;
			
			if(threadPrev == thread &&
			   !ismember(v->preds[i], &inR[0], *numInR)) {
				inR[*numInR] = v->preds[i];
				(*numInR)++;
			} // if
		} // for
		
		// Recursive call on predecessors to populate my "inReacheables"
		// TODO Optimize it by storing the inReachable array?
		for(i=0; i<v->num_preds; i++) {
			staticGetInReachables(dags, ind, v->preds[i], inR, numInR, thread, type);
		} // for
		
	} // if
// 	_log("dag %s node w/idMercurium %ld num preds %	ld. Done\n", dags[ind]->name, v->idMercurium, v->num_preds);
} // staticGetInReachables

void staticGetOutReachables(struct dag_t** dags, long ind, struct node_t *v, struct node_t ** outR, long *numOutR, thread_t thread, mapping_t type) {
	thread_t threadSucc = 0;
	int i;
// 	_log("dag %s node w/idMercurium %ld num succs %ld thread (param) %ld\n", dags[ind]->name, v->idMercurium, v->num_succs, thread);
	
	if(v->num_succs != 0) {
		for(i=0; i<v->num_succs; i++) {
			threadSucc = v->succs[i]->thread;
			if(threadSucc == thread &&
			   !ismember(v->succs[i], &outR[0], *numOutR)) {
				outR[*numOutR] = v->succs[i];
				(*numOutR)++;
			} // if
		} // for
		
		// Recursive call on successors to populate my "outReacheables"
		for(i=0; i<v->num_succs; i++) {
			staticGetOutReachables(dags, ind, v->succs[i], outR, numOutR, thread, type);
		} // for
		
	} // if
// 	_log("dag %s node w/idMercurium %ld num succs %ld. Done\n", dags[ind]->name, v->idMercurium, v->num_succs);
} // staticGetOutReachables

// function vertSameThread = getVerticesOnSameThread(tsk, ind, k, thread, type)
void getVerticesOnSameThread(struct node_t *v, long nnodes, long k, struct node_t **vertSameThread, long *numVertSameThread, thread_t thread, mapping_t type) {
	*numVertSameThread = 0;
	long i = 0;
	
// 	_log("nodes is %ld thread is %d\n", v[k].idMercurium, thread);
	
	for(i=0; i<nnodes; i++) {
		if(i != k && v[i].thread == thread) {
			vertSameThread[*numVertSameThread] = &v[i];
			(*numVertSameThread)++;
		}
	} // for
	
} // getVerticesOnSameThread

struct node_t ** inReachable = NULL, ** outReachable = NULL,
							** reachable = NULL, ** vertSameThread = NULL, **selfIntNodes = NULL,
							** inReachableAllThr = NULL, ** outReachableAllThr = NULL, **reachableAllThr = NULL,
							** checkedVert = NULL;
// function [selfInt, inclNodes] = computeSelfInterference(tsk, ind, k, inclNodes, thread, type)
/*
 * staticComputeSelfInterference
 *					dags - are *all* tasks/dags
 *					ind - is current dag index
 *					k - is node index
 *					inclNodes - is always empty (theorically unused, and ignored)
 * 					thread - is allocated thread
 * 					type - is map type (unused atm)
 */
wcet_t staticComputeSelfInterference(struct dag_t** dags, long ind, long k, struct node_t * inclNodes[], long numInclNodes, thread_t thread, mapping_t type) {
	// _log("***** dag %s node %ld (idMercurium %ld) thread %d\n", dags[ind]->name, k, dags[ind]->v[k].idMercurium, dags[ind]->v[k].thread);
	
	// *Reachable are globals to avoid reallocating them each time	
	wcet_t selfInt = 0LL;
	long numInReachables = 0L, numOutReachables = 0L, numReachables = 0L, numVertSameThread = 0L, numSelfIntNodes = 0L, i;
	
	staticGetInReachables(dags, ind, &dags[ind]->v[k], inReachable, &numInReachables, thread, type);
	staticGetOutReachables(dags, ind, &dags[ind]->v[k], outReachable, &numOutReachables, thread, type);
	
	numReachables = _union(&inReachable[0], numInReachables, &outReachable[0], numOutReachables, &reachable[0]);
	
	getVerticesOnSameThread(&dags[ind]->v[0], dags[ind]->num_nodes, k, vertSameThread, &numVertSameThread, thread, type);
	
	numSelfIntNodes = setdiff(vertSameThread, numVertSameThread, reachable, numReachables, selfIntNodes);
	
	// reuse vertSameThread: do we really need this?
	numVertSameThread = setdiff(selfIntNodes, numSelfIntNodes, inclNodes, numInclNodes, vertSameThread);
// 	printNodesSet("vertSameThread (as selfIntNodes) is", &vertSameThread[0], numVertSameThread);
	
	for(i=0; i<numVertSameThread; i++) {
		selfInt += vertSameThread[i]->C;
	}
	
	return selfInt;
} // staticComputeSelfInterference

int staticTryAllocationBestFit(struct dag_t **dags, int dagsLength, long ind, long k, thread_t currentThread, float Rmax, float *R) {

	_log("*** dag %s node %ld currentThread %d Rmax %f\n", dags[ind]->name, dags[ind]->v[k].idMercurium, currentThread, Rmax);
	
	int res = 1, i, p;
	float Rprev, Rstatic;
	
	// THIS IS 100% DUMMY. Should we remove it?
	struct node_t ** inclNodes = (struct node_t **) malloc(sizeof(struct node_t *) * MAX_PREDS);
	long numInclNodes = 0;
	
	wcet_t selfInt = staticComputeSelfInterference(dags, ind, k, inclNodes, numInclNodes, currentThread, MAPPING_BESTFIT);
// 	_log("selfInt is %lld\n", selfInt);
	
	if(inclNodes != NULL) free(inclNodes);
	
	Rprev = 0;
	Rstatic = dags[ind]->v[k].C + selfInt;
	if (FloatGT(Rstatic + Rmax, dags[ind]->D))
		_msg("Fail for node %ld DAG %s on thread %d: DAG C is %ld, selfInt %lld ==> RStatic %f Rmax %f  AG DDL %ld",
				dags[ind]->v[k].idMercurium, dags[ind]->name, currentThread, dags[ind]->v[k].C,selfInt, Rstatic, Rmax, dags[ind]->D);
	
	if(ind > 1) {
		
		while (Rprev != Rstatic) {
			Rprev = Rstatic;
			Rstatic = dags[ind]->v[k].C + selfInt;
			
			// for each higher priority task
			for(i=0; i<ind; i++) {
				for (p=0; p<dags[ind]->num_nodes; p++) {
					struct node_t * v = &dags[i]->v[p];
					if(v->thread == currentThread) {
						Rstatic = Rstatic + ceil((Rprev + v->R - v->C) / dags[i]->T) * v->C;
						
						if (FloatGT(Rstatic + Rmax, dags[ind]->D)) {
							_msg("Fail for node %ld DAG %s on thread %d: RStatic %f Rmax %f DAG DDL %ld\n", v->idMercurium, dags[ind]->name, currentThread, Rstatic, Rmax, dags[ind]->D);
							break;
						}
					} // if
					
					if (FloatGT(Rstatic + Rmax, dags[ind]->D))
						break;
				} // foreach node (p)
				
				if (FloatGT(Rstatic + Rmax, dags[ind]->D))
					break;
			} // foreach HP tasks (i)
			
		} // while
		
	} // if ind > 1
	
	
	// do not write nodes' sched: it's "try"AllocationBestFit!
	
	if (FloatGT(Rstatic + Rmax, dags[ind]->D)) {
		res = 0;
		*R = -1;
	}
	else {
		res = 1;
		*R = Rstatic + Rmax;
	}
	
 	_log("dag %s %s schedulable! Rstatic %f Rmax %f deadline %ld\n", dags[ind]->name, res ? "would be" : "would not be", Rstatic, Rmax, dags[ind]->D);
	
	return res;
} // staticTryAllocationBestFit

void staticGetInReachablesAllThr(struct dag_t** dags, long ind, struct node_t *v, struct node_t ** inR, long *numInR) {
	int i;
	
// 	_log("dag %s node w/idMercurium %ld num preds %ld thread (param) %ld\n", dags[ind]->name, v->idMercurium, v->num_preds, thread);
	
	if(v->num_preds != 0) {
		for(i=0; i<v->num_preds; i++) {
// 			printNodesSet("\t(1) inReachables was", &inR[0], *numInR);
			
			if(!ismember(v->preds[i], &inR[0], *numInR)) {
				inR[*numInR] = v->preds[i];
				(*numInR)++;
			} // if
// 			printNodesSet("\t(1) now inReachables is", &inR[0], *numInR);
		} // for
		
		// Recursive call on predecessors to populate my "inReacheables"
		for(i=0; i<v->num_preds; i++) {
// 			printNodesSet("\t(2) inReachables was", &inR[0], *numInR);
			staticGetInReachablesAllThr(dags, ind, v->preds[i], inR, numInR);
// 			printNodesSet("\t(2) now inReachables is", &inR[0], *numInR);
		} // for
		
	} // if
// 	_log("dag %s node w/idMercurium %ld num preds %	ld. Done\n", dags[ind]->name, v->idMercurium, v->num_preds);
} // staticGetInReachablesAllThr

void staticGetOutReachablesAllThr(struct dag_t** dags, long ind, struct node_t *v, struct node_t ** outR, long *numOutR) {
	int i;
	
// 	_log("dag %s node w/idMercurium %ld num succs %ld thread (param) %ld\n", dags[ind]->name, v->idMercurium, v->num_succs, thread);
	
	if(v->num_succs != 0) {
		for(i=0; i<v->num_succs; i++) {
// 			printNodesSet("\t(1) outReachables was", &outR[0], *numOutR);
			
			if(!ismember(v->succs[i], &outR[0], *numOutR)) {
				outR[*numOutR] = v->succs[i];
				(*numOutR)++;
			} // if
// 			printNodesSet("\t(1) now outReachables is", &outR[0], *numOutR);
		} // for
		
		// Recursive call on successors to populate my "outReacheables"
		for(i=0; i<v->num_succs; i++) {
// 			printNodesSet("\t(2) outReachables was", &outR[0], *numOutR);
			staticGetOutReachablesAllThr(dags, ind, v->succs[i], outR, numOutR);
// 			printNodesSet("\t(2) now outReachables is", &outR[0], *numOutR);			
		} // for
		
	} // if
// 	_log("dag %s node w/idMercurium %ld num succs %ld. Done\n", dags[ind]->name, v->idMercurium, v->num_succs);
} // staticGetOutReachablesAllThr

// function [sched, tsk] = schedulabilityCheckBF(tsk, ind, k, subOrder)
int staticSchedulabilityCheckBF(struct dag_t ** dags, long dagsLength, long ind, long k, struct node_t * subOrder, long subOrderSize) {
	_log("\n");
	float Rmax = 0., R;
	int schedV = 1;
		
	long numInReachablesAllThr = 0L, numOutReachablesAllThr = 0L,
		numReachablesAllThr = 0L, numCheckedVert = 0L, i, j;
	
	staticGetInReachablesAllThr(dags, ind, &dags[ind]->v[k], inReachableAllThr, &numInReachablesAllThr);
	staticGetOutReachablesAllThr(dags, ind, &dags[ind]->v[k], outReachableAllThr, &numOutReachablesAllThr);
	
	numReachablesAllThr = _union(&inReachableAllThr[0], numInReachablesAllThr, &outReachableAllThr[0], numOutReachablesAllThr, &reachableAllThr[0]);

	// use numInReachablesAllThr to create node_t ** from subOrder which comes from node_t []
	for(i=0; i<subOrderSize; i++)
		inReachableAllThr[i] = &subOrder[i];
	numInReachablesAllThr = subOrderSize;
	
	numCheckedVert = setdiff(inReachableAllThr, subOrderSize, reachableAllThr, numReachablesAllThr, checkedVert);
	
	// TODO reordering
	
	for(i=0; i<numCheckedVert; i++) {
		for(j=0; j<checkedVert[i]->num_preds; j++ ) {
			if(FloatGT(checkedVert[i]->preds[j]->R, Rmax))
				Rmax = checkedVert[i]->preds[j]->R;
		}
	
		// FIXME do it more efficiently...
		int k = nodeIdx(&dags[ind]->v[0], dags[ind]->num_nodes, checkedVert[i]);
		schedV = staticTryAllocationBestFit(dags, dagsLength, ind, k, checkedVert[i]->tryThread, Rmax, &R);
		
		if(schedV == 0)	 {
			R = -1;
			checkedVert[i]->tryR = -1;
			 _log("CHECK FAILED for node %ld when attempting to schedule on thread %d: the reason follows:\n\t%s\n", checkedVert[j]->idMercurium, checkedVert[j]->tryThread, msg);
			
			break;
		}
		else
			checkedVert[i]->tryR = R;
		
	} // for i
	
	_log("returning %d\n", schedV);
	
	return schedV;
} // staticSchedulabilityCheckBF

int staticSchedulabilityAnalysisAndMapping(struct dag_t **dags, int dagsLength, unsigned int ncores) {
	int ind, k, sched, ret, i, schedV, l, indCurr;
	float Rmax, R, Rcurr;
	_log("dagsLength %d ncores %d\n", dagsLength, ncores);

	
	inReachable = (struct node_t **) malloc(sizeof(struct node_t *) * MAX_PREDS);
	outReachable = (struct node_t **) malloc(sizeof(struct node_t *) * MAX_PREDS);
	reachable = (struct node_t **) malloc(sizeof(struct node_t *) * MAX_PREDS);
	vertSameThread = (struct node_t **) malloc(sizeof(struct node_t *) * MAX_PREDS);
	selfIntNodes = (struct node_t **) malloc(sizeof(struct node_t *) * MAX_PREDS);
	inReachableAllThr = (struct node_t **) malloc(sizeof(struct node_t *) * MAX_PREDS);
	outReachableAllThr = (struct node_t **) malloc(sizeof(struct node_t *) * MAX_PREDS);
	reachableAllThr = (struct node_t **) malloc(sizeof(struct node_t *) * MAX_PREDS);
	checkedVert = (struct node_t **) malloc(sizeof(struct node_t *) * MAX_PREDS);
	
	// Delete existing map because I will produce a new one */
	dagDeleteExistingMaps(dags, dagsLength);
	
	qsort(dags, dagsLength, sizeof(struct dag_t *), cmpDAGsRateMonotonic);
	
	ret = 1; // = schedulable
	struct dag_t *task_save, **task_new_proc;
	task_new_proc = (struct dag_t **) malloc(sizeof(struct dag_t **) * ncores);
	for(ind=0; ind<dagsLength; ind++) {
		/// function [R, sched, tsk] = runRTA_FP_static_???(tsk, ind)
		_log("Scheduling DAG %s (tdg_id %d)\n", dags[ind]->name, dags[ind]->tdg_id);
		
		dagCreateEmpty(dags[ind]->num_nodes, &task_save);
		for(i=0; i<ncores; i++) {
			dagCreateEmpty(dags[ind]->num_nodes, &task_new_proc[i]);
		}
		
		sched = 1;
		for(k=0; k<dags[ind]->num_nodes; k++) {
			_log("SCHEDULING NODE %ld OF DAG %s\n", dags[ind]->v[k].idMercurium, dags[ind]->name);
			// compute offset due to predecessors (max response time among immediate predecessors)
			Rmax = 0;
			for(l=0; l<dags[ind]->v[k].num_preds; l++) {
				if(FloatGT(dags[ind]->v[k].preds[l]->R, Rmax))
					Rmax = dags[ind]->v[k].preds[l]->R;
			} // foreach pred (l)
			
			Rcurr = FLT_MAX;
			indCurr = -1;

			tryMappingDagBak(task_save, dags[ind]);
	
			for(i=0; i<ncores; i++) {
				tryMappingDagBak(task_new_proc[i], dags[ind]);
			}
			
			// No need to do this, as staticSchedulabilityCheckBF accepts the lenght
			// subOrder = order(1 : z - 1);
			int subOrderSize = k-1;
			schedV = 1;
			for(i=0; i<ncores; i++) {
				R = 0;
				tryMappingDagBak(dags[ind], task_save);
				//_log("Trying to schedule node %ld of dag %s on core %d out of %d\n", dags[ind]->v[k].idMercurium, dags[ind]->name, i, ncores);
				schedV = staticTryAllocationBestFit(dags, dagsLength, ind, k, i, Rmax, &R);
				
				// _log("dag %s, node %ld %s schedulable on core %d out of %d with R %f\n", dags[ind]->name, dags[ind]->v[k].idMercurium, schedV ? "is (possibly)" : "IS NOT", i, ncores, R);
				// if(!schedV)
					// _log("\tThe reason follows: \n\t%s\n", msg);
				
				if(schedV && FloatST(R, Rcurr)) {
					
					// Fill thread and R values we are trying with
					dags[ind]->v[k].tryThread = i;
					dags[ind]->v[k].tryR = R;
					// _log("Performing schedulability check R %f Rcurr %f\n", R, Rcurr);
				
					int schedOth = staticSchedulabilityCheckBF(dags, dagsLength, ind, k, dags[ind]->v, subOrderSize);
					// _log("Other tasks %s schedulable if I put %ld on thread %d\n", schedOth ? "are" : "are NOT ", dags[ind]->v[k].idMercurium, i);
						
					if(schedOth == 1) {
						indCurr = i;
						Rcurr = R;
						// Thread and R are ok, confirm and pass to another core
						dags[ind]->v[k].thread = dags[ind]->v[k].tryThread;
						dags[ind]->v[k].R = dags[ind]->v[k].tryR;
						// printf("[staticSchedulabilityAnalysis] Mapping node idMercurium %ld of DAG tdg_id %d on thread %d\n",
									 // dags[ind]->v[k].idMercurium, dags[ind]->tdg_id, dags[ind]->v[k].thread);						
						
						tryMappingDagBak(task_new_proc[i], dags[ind]);
					}
					
				} // if
			} // foreach core
						
			if(indCurr != -1) {
				tryMappingDagBak(dags[ind], task_new_proc[indCurr]);
				_log("dag %s, node %ld will be scheduled on core %d with R %lld\n", dags[ind]->name, dags[ind]->v[k].idMercurium, dags[ind]->v[k].thread, dags[ind]->v[k].R);
			}
			else {
				sched = 0;
				break;
			}
			
		}// for each node in dag
		
		if(task_new_proc)
			for(i=0; i<ncores; i++)
				dagDispose(task_new_proc[i]);
			
		if(task_save) {
			dagDispose(task_save);
		}
				
		dags[ind]->sched = sched;
		if(!sched) {
			R = -1;
			// put on task
			ret = 0;
			break;
		}
		
		//break; // FIXME REMOVE
	}// for each dag
	
	if(task_new_proc) {
		free(task_new_proc);
	}
	
	if(checkedVert != NULL) free(checkedVert);
	if(reachableAllThr != NULL) free(reachableAllThr);
	if(outReachableAllThr != NULL) free(outReachableAllThr);
	if(inReachableAllThr != NULL) free(inReachableAllThr);
	if(selfIntNodes != NULL) free(selfIntNodes);
	if(vertSameThread != NULL) free(vertSameThread);
	if(reachable != NULL) free(reachable);
	if(outReachable != NULL) free(outReachable);
	if(inReachable != NULL) free(inReachable);
	
	return ret;
} // staticSchedulabilityAnalysisAndMapping

// function [R, sched, tsk] = runRTA_FP_static_threadRandom(tsk, ind)
int staticSchedulabilityAnalysis(struct dag_t **dags, int dagsLength, unsigned int ncores) {
    int ind, k, sched, ret, schedV, l;
	float Rmax, R;
	
	_log("dagsLength %d ncores %d\n", dagsLength, ncores);
	
	inReachable = (struct node_t **) malloc(sizeof(struct node_t *) * MAX_PREDS);
	outReachable = (struct node_t **) malloc(sizeof(struct node_t *) * MAX_PREDS);
	reachable = (struct node_t **) malloc(sizeof(struct node_t *) * MAX_PREDS);
	vertSameThread = (struct node_t **) malloc(sizeof(struct node_t *) * MAX_PREDS);
	selfIntNodes = (struct node_t **) malloc(sizeof(struct node_t *) * MAX_PREDS);
	inReachableAllThr = (struct node_t **) malloc(sizeof(struct node_t *) * MAX_PREDS);
	outReachableAllThr = (struct node_t **) malloc(sizeof(struct node_t *) * MAX_PREDS);
	reachableAllThr = (struct node_t **) malloc(sizeof(struct node_t *) * MAX_PREDS);
	checkedVert = (struct node_t **) malloc(sizeof(struct node_t *) * MAX_PREDS);
	
	qsort(dags, dagsLength, sizeof(struct dag_t *), cmpDAGsRateMonotonic);
	
	ret = 1; // = schedulable
	
	for(ind=0; ind<dagsLength; ind++) {
		
		sched = 1;
		for(k=0; k<dags[ind]->num_nodes; k++) {
			thread_t currentThread = dags[ind]->v[k].thread;
			
			_log("SCHEDULABILITY ANALYSIS FOR NODE %ld OF DAG %s MAPPED ON THREAD %d\n", dags[ind]->v[k].idMercurium, dags[ind]->name, dags[ind]->v[k].thread);
			R = 0;
			// compute offset due to predecessors (max response time among immediate predecessors)
			Rmax = 0;
			for(l=0; l<dags[ind]->v[k].num_preds; l++) {
				if(FloatGT(dags[ind]->v[k].preds[l]->R, Rmax))
					Rmax = dags[ind]->v[k].preds[l]->R;
			} // foreach pred (l)
						
			schedV = staticTryAllocationBestFit(dags, dagsLength,
			                                 ind, k, currentThread, Rmax, &R);
											 
			if(schedV == 0) {
				R = -1;
				sched = 0;
				_log("CHECK FAILED for node %ld scheduled on thread %d: the reason follows:\n\t%s\n", dags[ind]->v[k].idMercurium, dags[ind]->v[k].thread, "NULL" /* msg */);
			
				break;
			}
			else {
				dags[ind]->v[k].R = R;
				_log("dag %s, node %ld can be scheduled on core %d with R %lld\n", dags[ind]->name, dags[ind]->v[k].idMercurium, dags[ind]->v[k].thread, dags[ind]->v[k].R);
			}
			
		} // for each node
		
		dags[ind]->sched = sched;
		if(!sched) {
			R = -1;
			// put on task
			ret = 0;
			break;
		}
	} // for each dag

	if(checkedVert != NULL) free(checkedVert);
	if(reachableAllThr != NULL) free(reachableAllThr);
	if(outReachableAllThr != NULL) free(outReachableAllThr);
	if(inReachableAllThr != NULL) free(inReachableAllThr);
	if(selfIntNodes != NULL) free(selfIntNodes);
	if(vertSameThread != NULL) free(vertSameThread);
	if(reachable != NULL) free(reachable);
	if(outReachable != NULL) free(outReachable);
	if(inReachable != NULL) free(inReachable);
	
	return ret;
} //staticSchedulabilityAnalysis2

void staticGetMapping(struct dag_t * dag, thread_t * mappings) {
	// _log("dag %s, mapping array 0x%x\n", dag->name, _mycast_ mappings);
	int k;
	
	for(k=0; k<dag->num_nodes; k++) {
// 		printf("Writing mapping of node %ld of dag %s to thread %d at index %ld\n", dag->v[k].idMercurium, dag->name, dag->v[k].thread, dag->v[k].dealerIdx);
		mappings[dag->v[k].dealerIdx] = dag->v[k].thread;
	}
} // staticGetMapping

void rrGetMapping(struct dag_t * dag, thread_t * mappings, unsigned int ncores) {
	// _log("dag %s, mapping array 0x%x\n", dag->name, _mycast_ mappings);
	int k;
	static unsigned int rrPrev = 0;	
	
	for(k=0; k<dag->num_nodes; k++) {
// 		_log("Writing mapping of node %ld of dag %s to thread %d at index %ld\n",
// 				 dag->v[k].idMercurium, dag->name, dag->v[k].thread, dag->v[k].dealerIdx);
		mappings[dag->v[k].dealerIdx] = rrPrev;
		rrPrev = (rrPrev + 1 ) % ncores;
	}
} // rrGetMapping
