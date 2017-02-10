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

#include "dag.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/** In psoc_mapper.c: useful for debugging */
void dumpnodes();
void dumpdeps();

/* Same as defined in dealer. Potentially dangerous! */
struct dep {
	unsigned long src, dst;
};

char createdByMe = 0;

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

long dagFindNodeIdx(struct node_t where[], long whereSize, struct node_t * who) {
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

int initNode(struct node_t * n) {
	void * datum;
	n->idMercurium = -1;
	n->dealerIdx = -1;
	datum = malloc(sizeof(struct node_t *) * MAX_PREDS);
	if(!datum)
		return 1;
	n->preds = datum;
	datum = malloc(sizeof(struct node_t *) * MAX_SUCCS);
	if(!datum)
		return 1;
	n->succs = datum;
	n->num_preds = 0;
	n->num_succs = 0;
	n->C = -1;
	n->accWorkload = -1;
	n->R = -1;
	n->thread = -1;
	return 0;
} // initNode

int buildDeps(struct dep *deps, long ndeps, struct node_t *v, unsigned int *indexes) {
	int i;
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
	return 0;
} // buildDeps

/*
 * NOTE topo_logical order must be respected by contruction
 */
int dagCreate(char *graphName, int tdg_id, long* nodes, wcet_t *wcets, thread_t *nodeMaps, long nnodes, struct dep *deps, long ndeps, char makeSingleSrcSink, struct dag_t *dag[]) {
	int i, j;
	
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
		int ret =	initNode(&v[i]);
		if(ret != 0)
			return ret;
	}
	
	unsigned int *indexes = (unsigned int *) malloc((nodes[nnodes-1] + 1) * sizeof(int));
	unsigned int *indexes2 = (unsigned int *) malloc((nodes[nnodes-1] + 1 + 2 /* In case we have to add source/sink */) * sizeof(int));
	
	long countNodesWithoutWcet = 0, countNodesWithoutMapping = 0, maxIdMercurium = 0;
	
	for(i=0; i<nnodes; i++) {
		v[i].idMercurium = nodes[i];
		
		if(nodes[i] > maxIdMercurium)
			maxIdMercurium = nodes[i];
		
		v[i].dealerIdx = i;
		
		if(wcets[i] <= 0)
			countNodesWithoutWcet++;
		v[i].C = wcets[i];
		if(nodeMaps[i] <= 0)
			countNodesWithoutMapping++;
		v[i].thread = nodeMaps[i];
		indexes[nodes[i]] = i;
	}
	
	int ret = buildDeps(&deps[0], ndeps, &v[0], &indexes[0]);
	if(ret != 0) // Something went wrong (e.g. out of mem)
		return ret;
	
	if(makeSingleSrcSink) {
		
		long sources[MAX_SUCCS], sinks[MAX_SUCCS], countSources = 0, countSinks = 0;
		
		/* First, check for multiple source/sink nodes  */
		for(i=0; i<nnodes; i++) {
			if(!v[i].num_preds)
				sources[countSources++] = v[i].idMercurium;
			
			if(!v[i].num_succs)
				sinks[countSinks++] = v[i].idMercurium;
		}
		
		printf("There are %ld sources: ", countSources);
		for(i=0; i<countSources; i++)
			printf("%ld ", sources[i]);
		printf("\n");
		
		printf("There are %ld sinks: ", countSinks);
		for(i=0; i<countSinks; i++)
			printf("%ld ", sinks[i]);
		printf("\n");
		
		// Adjust single source/single sink
		// NOTES: (i)  nodes must be topologically (partially) ordered
		//        (ii) Remember indexes!
		if(countSources > 1 || countSinks > 1) {
			unsigned int nodesToAdd = 0;
			
			if(countSources > 1) nodesToAdd++;
			if(countSinks > 1) nodesToAdd++;
		
			struct node_t *v_new = (struct node_t*) malloc(sizeof(struct node_t) * (nnodes + nodesToAdd)), *n;
			
			// Make new array with nodes
			
			printf("Before:\n");
			for(j=0; j<nnodes; j++)
				printNode(&v[j]);
			
			int istart = countSources > 1 ? 1 : 0;
			for(j=0; j<nnodes; j++) {
				v_new[istart + j] = v[j];
				v_new[istart + j].num_succs = 0;
				v_new[istart + j].num_preds = 0;
				indexes2[v_new[istart + j].idMercurium] = istart + j;
			}
			nnodes += nodesToAdd;
							
			// Add new source
			if(countSources > 1) {
				n = &v_new[0];
				int ret =	initNode(n);
				if(ret != 0)
					return ret;
				n->idMercurium = ++maxIdMercurium;
				indexes2[n->idMercurium] = 0;
			}
			
			// Add sink: easier as we don't need to shift..
			if(countSinks > 1) {
				n = &v_new[nnodes - 1];
				int ret =	initNode(n);
				if(ret != 0)
					return ret;
				n->idMercurium = ++maxIdMercurium;
				indexes2[n->idMercurium] = nnodes - 1;
			}		
			
			// Re-build dependencies for the new graph
			// Then, add pointers to succs at new source node
			// Then, add ptrs to preds at the new sink node
			// (Can't do these things earlier because index2 is consistent only now)
			
			int ret = buildDeps(&deps[0], ndeps, &v_new[0], &indexes2[0]);
			if(ret != 0)
				return ret;
			
			if(countSources > 1) {
				struct node_t *src = &v_new[0];
				
				for(j=0; j<countSources; j++) {
					n = &v_new[indexes2[sources[j]]];
					
					src->succs[src->num_succs++] = n;
					n->preds[n->num_preds++] = src;
				}
			} // if countSources > 1
			
			if(countSinks> 1) {
				struct node_t *sink = &v_new[nnodes-1];
				
				for(j=0; j<countSinks; j++) {
					n = &v_new[indexes2[sinks[j]]];
					
					v_new[nnodes-1].preds[sink->num_preds++] = n;
					n->succs[n->num_succs++] = sink;
				}
			} // if countSinks > 1
			
			free(v);
			v = v_new;
			
			printf("After:\n");
			for(j=0; j<nnodes; j++)
				printNode(&v[j]);
		}
	} // if make single source-single sink

	
	free(indexes2);
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
	
	(*dag)->countNodesWithoutWcet = countNodesWithoutWcet;
	(*dag)->countNodesWithoutMapping = countNodesWithoutMapping;
	
	if((*dag)->countNodesWithoutWcet)
		printf("Warning! Dag #%d ('%s') has %ld nodes without WCET out of %ld!\n", (*dag)->tdg_id, (*dag)->name, (*dag)->countNodesWithoutWcet, (*dag)->num_nodes);
	
	// we need the #cores (aka 'm') to compute Z: can't do this here
/* 	task(i).Z = task(i).len + (1 / m) * (task(i).wcw - task(i).len);    % simple upper-bound on intra-task interference */
	 
	return 0;
	
} // createDag

void dagDeleteExistingMaps(struct dag_t **dags, int dagsLength) {
	/* Delete existing map */
	int ind, i;
	for(ind=0; ind<dagsLength; ind++) {
//		printf("Deleting existing MAP for dag %d (from digraph '%s')\n", dags[ind]->tdg_id, dags[ind]->name);
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
			//printf("Assigning D %ld and T %ld to %d. Buff is %s\n", D, T, dag->tdg_id, buff);
			dag->D = ignoreDeadlinesFromFile ? T : D;
			dag->T = T;
			break;
		}
		
	}
	fclose(fp);
// 	printDag(dag);
	return 0;
} // assignDAGParams