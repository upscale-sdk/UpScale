/* -----------------------------------------------------------------------------
 * Author list (in alphabetic order):
 * 
 *        Eduardo Quiñones <eduardo.quinones@bsc.es>
 *        Paolo Burgio <paolo.burgio@unimore.it>
 *        Roberto Vargas <roberto.vargas@bsc.es>
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

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "sched.h"

#define ARROW    'a'
#define NUMBER   'n'
#define SYMBOL   's'
#define STRING   't'
#define COMMENT  'c'
#define MAXTOKEN 400
#define MAXDEP 200000

/* This will be printed at the beginning of execution */
#define VERSION_STRING "1.0"

// Libgomp TDG structure generation variables 
union {
	unsigned long i;
	char *s;
} tokenval;

struct gomp_task;

struct gomp_tdg {
	uint32_t id;
	struct gomp_task *task;
	short offin;
	short offout;
	char nin;
	char nout;
	signed char cnt;
};

/* Defined also in dag.c. If you edit this structure,
   remembed to change italso in that file! */
struct dep {
	unsigned long src, dst;
};

struct gomp_tdg **gtdg;
unsigned short **ins, **outs;
long *maxI, *maxT, *nnodes;
char **graphName;

struct dep *deps;
long *nodes = NULL;
long ndeps = 0;
unsigned lineno = 0;

char token;
int tdg_id = -1;
unsigned num_tdgs = 0;

char *filename;
char *output;
FILE *fgraph = NULL, *fout = NULL;
char toktext[MAXTOKEN];

int optverbose;

// Scheduling variables
#define DEFAULT_NCORES 4
int ncores = DEFAULT_NCORES;
wcet_t *MIETs, *MEETs, *MAETs;
// This must be global
scheduling_t scheduling = NONE;
thread_t **mappings;
int isSchedulable = 0;
thread_t *nodeMap;

void *xmalloc(size_t len);
void *xrealloc(void *buf, size_t len);

void
error(char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	fprintf(stderr, "psoc_mapper: %s:%d: ", filename, lineno);
	vfprintf(stderr, fmt, va);
	putc('\n', stderr);
	va_end(va);
	remove(output);
	exit(EXIT_FAILURE);
}

void
outmem(void)
{
	error("out of memory");
}

void *
xmalloc(size_t len)
{
	void *p = malloc(len);
	if (p == NULL)
		outmem();
	return p;
}

void *
xrealloc(void *p1, size_t len)
{
	void *p2 = realloc(p1, len);
	if (p2 == NULL)
		outmem();
	return p2;
}

void consume_comment() {
	int c = 0;
	while((c = getc(fgraph)) != '\n');
}

char
next(void)
{
	char *p;
	int c;
	unsigned long l;

	while (isspace(c = getc(fgraph))) {
		if (c == '\n')
			++lineno;
	}

	if (c == EOF || c == '{' || c == '}') {
		return token = c;
	} else if (c == '-') {
		if (getc(fgraph) == '>')
			return token = ARROW;
		error("syntax error");
	} else if (c == '"') {
		for (p = toktext; p < &toktext[MAXTOKEN-1]; *p++ = c) {
			if ((c = getc(fgraph)) == EOF)
				error("EOF found while parsing");
			if (c == '"') {
				*p = '\0';
				return token = STRING;
			}
		}
		error("string too long");
	} else if (isdigit(c)) {
		ungetc(c, fgraph);
		for (p = toktext; p < &toktext[MAXTOKEN-1]; *p++ = c) {
			if (!isdigit((c = getc(fgraph)))) {
				ungetc(c, fgraph);
				*p = '\0';
				l = strtoul(toktext, NULL, 10);
				if (l == LONG_MIN || l == LONG_MAX)
					error("incorrect number");
				tokenval.i = l;
				return token = NUMBER;
			}
		}
		error("number too long");
	} else if (isalpha(c) || c == '_') {
		ungetc(c, fgraph);
		for (p = toktext; p < &toktext[MAXTOKEN-1]; *p++ = c) {
			if (!isalnum(c = getc(fgraph)) && c != '_') {
				ungetc(c, fgraph);
				*p = '\0';
				tokenval.s = toktext;
				return token = SYMBOL;
			}
		}
		error("symbol too long");
        } else if (c == '/') {
		if (getc(fgraph) == '/')
			return token = COMMENT;
		error("syntax error");
        }
        else {
		return token = c;
	}
	error("Unknow symbol");
	return 0;
}

void
expect(char tok)
{
	if (token != tok)
		error("unexpected token (e.g., %c instead of %c)", token, tok);
	next();
}

int
accept(char tok)
{
	if (token != tok)
		return 0;
	next();
	return 1;
}

unsigned long
number(void)
{
	unsigned long val;

	if (token != NUMBER)
		error("unexpected token (Not a number, e.g., %c)", token);
	val = tokenval.i;
	next();
	return val;
}


void
dumpnodes(int id)
{
    int i;
	for (i = 0; i < nnodes[id]; i++) {
		printf("%d) node %ld MIET %lld MEET %lld MAET %lld map (if available)%d\n",
		       i, nodes[i], MIETs[i], MEETs[i], MAETs[i], nodeMap[i]);
	}
}


void dumpdeps()
{
	int i;
	printf("ndeps %ld\n", ndeps);
	for(i=0; i<ndeps; i++) {
		printf("DEP (%d) src %ld -> dst %ld\n", i, deps[i].src, deps[i].dst); 
	}
}

void
addnode(long id)
{
	long *p;

	if (nodes && nnodes[tdg_id] > 0) {
		for (p = nodes; p < &nodes[nnodes[tdg_id]]; ++p) {
			if (*p == id)
				return;
		}
	}
	++nnodes[tdg_id];
	nodes = xrealloc(nodes,sizeof(*nodes) * nnodes[tdg_id]);
	MIETs = xrealloc(MIETs,sizeof(*MIETs) * nnodes[tdg_id]);
	MEETs = xrealloc(MEETs, sizeof(*MEETs) * nnodes[tdg_id]);
	MAETs = xrealloc(MAETs, sizeof(*MAETs) * nnodes[tdg_id]);
	nodeMap = xrealloc(nodeMap,sizeof(*nodeMap) * nnodes[tdg_id]);

	nodes[nnodes[tdg_id]-1] = id;
	MIETs[nnodes[tdg_id]-1] = -1;
	MEETs[nnodes[tdg_id]-1] = -1;
	MAETs[nnodes[tdg_id]-1] = -1;
	nodeMap[nnodes[tdg_id]-1] = -1;
}

void
parameter(unsigned long id)
{
	char *p;
	int val;

	if (token != SYMBOL)
		error("symbol expected");
	if (strcmp("label", toktext))
		error("label missed in parameter");
	next();
	expect('=');
	if (token != STRING || (p = strchr(toktext, '=')) == NULL)
		error("incorrect label in parameter");
	*p++ = '\0';
	val = atoi(p);
	if(!strcmp("tdg_id",toktext)) {
		if((tdg_id=val) >= num_tdgs)
			error("tdg_id equal or bigger than the total number of tdgs (%d>=%d)\n",tdg_id, num_tdgs);
	}
	if(tdg_id == -1)
		error("\"tdg_id\" not set; it must be the first parameter in the .dot file");

	if(!strcmp("maxI",toktext))
		maxI[tdg_id] = val;
	else if(!strcmp("maxT",toktext))
		maxT[tdg_id] = val;

	next();
	expect(',');
	expect(SYMBOL);  /* style */
	expect('=');
	expect(SYMBOL);  /* hidden */
	expect(']');
}

void
expect2(char tok1, char tok2) {
	if (token != tok1 && token != tok2)
		error("unexpected token (e.g., %c instead of 's' or 't'", token);
	next();
}

void
nodeparameter(unsigned long id, unsigned long idx)
{
	// printf("nodeparameter(%lu, %lu)\n", id, idx);
	while(1)
	{
		if(token == EOF || token == '\n')
			error("Unexpected end of parameter");
			
		if(token == ']') // empty param
			break;
// 		printf("0) token %c tokentext %s\n", token, toktext);
		
		if (!strcmp("MIET", toktext) || !strcmp("miet", toktext)) {
			next();
			expect('=');
// 			printf("1) token %c tokentext %s\n", token, toktext);
			MIETs[idx] = atoi(toktext);// FIXME
// 			printf("-- Read MIET '%lld' for node %ld\n", MIETs[idx], id);
// 			printf("-- Read MIET '%s' for node %ld\n", toktext, id);
			next();
			
		}
		else if(!strcmp("MEET", toktext) || !strcmp("meet", toktext)) {
			next();
			expect('=');
// 			printf("2) token %c tokentext %s\n", token, toktext);
			MEETs[idx] = atoi(toktext);
// 			printf("-- Read MEET '%ld' for node %ld\n", MEETs[idx], id);
// 			printf("-- Read MEET '%s' for node %ld\n", toktext, id);
			next();
		}
		else if(!strcmp("MAET", toktext) || !strcmp("maet", toktext)) {
			next();
			expect('=');
// 			printf("3) token %c tokentext %s\n", token, toktext);
			MAETs[idx] = atoi(toktext);
// 			printf("-- Read MAET '%ld' for node %ld\n", MAETs[idx], id);
// 			printf("-- Read MAET '%s' for node %ld\n", toktext, id);
			next();
		}
		else if(!strcmp("MAP", toktext) || !strcmp("map", toktext))
		{
			next();
			expect('=');
			nodeMap[idx] = atoi(toktext);
// 			printf("-- Read mapping %d for node idx %lu\n", atoi(toktext), idx);
			next();
			
		}
		else {
			// I don't care of this parameter (but maybe someone else does)
			next();
			if(token == '=') {
				next();
				expect2(STRING, SYMBOL);
				
			}
		}
		
		if(token != ',') {
			expect(']');
			break;
		}
		// another param?
		else
			next();
	} // while(1)
}

void
deplist(void)
{
	struct dep *p;
	unsigned long src, dst = 0;

	while (token != '}') {
		if(token == COMMENT) {
			consume_comment();
			next();
			continue;
		}
		src = number();
		if (src == 0)
		if (accept('[')) {
			parameter(src);
			continue;
		}
		addnode(src);
		while (accept(ARROW)) {
			addnode(dst = number());
			if (++ndeps == MAXDEP)
				error("too much dependences");
			deps = xrealloc(deps,sizeof(*deps) * ndeps);
			p = &deps[ndeps-1];
			p->dst = dst;
			p->src = src;
			printf("Added dep #%ld: src %ld dst %ld\n", ndeps-1, src, dst); 
			src = dst;
		}
		if(!dst) // no arrow: properties
			while(accept('[')) {
				nodeparameter(src, nnodes[tdg_id]-1); // always the right id (atm)
			}
	}
}

int
cmpnodes(const void *p1, const void *p2)
{
	return *(long *) p1 - *(long *) p2;
}

int
cmpsrcdep(const void *p1, const void *p2)
{
	const struct dep *g1 = p1, *g2 = p2;

	return g1->src - g2->src;
}

int
cmpdstdep(const void *p1, const void *p2)
{
	const struct dep *g1 = p1, *g2 = p2;

	return g1->dst - g2->dst;
}

unsigned short
lookup(int tdg_i, long task_i)
{
	struct gomp_tdg *mid, *high, *low;

	low = &gtdg[tdg_i][0];
	high = &gtdg[tdg_i][nnodes[tdg_i]-1];
	 while (low <= high) {
		mid = &low[(high - low) / 2];
		if (task_i == mid->id)
			return mid - gtdg[tdg_i];
		else if (task_i < mid->id)
			high = mid - 1;
		else
			low = mid + 1;
	}
	error("looking for an incorrect task '%ld\n'", task_i);
	return 0;
}

// Nodes are arranged in topological order
void
create_tdg_struct(void)
{
	struct dep *dp, *dlim;
	struct gomp_tdg *mp, *mlim;
	unsigned short *inp, *outp;
	int n;
	size_t nsiz, dsiz;

	nsiz = sizeof(struct gomp_tdg) * nnodes[tdg_id];
	dsiz = sizeof(unsigned short) * ndeps;

	if(gtdg[tdg_id])
		error("TDG with id %d already defined (name %s)\n",tdg_id,graphName[tdg_id]);

	gtdg[tdg_id] = xmalloc(nsiz);
	memset(gtdg[tdg_id], 0, nsiz);
	ins[tdg_id] = xmalloc(dsiz);
	outs[tdg_id] = xmalloc(dsiz);
	dlim = &deps[ndeps];
	mlim = &gtdg[tdg_id][nnodes[tdg_id]];

	qsort(nodes, nnodes[tdg_id], sizeof(long), cmpnodes);
	for (n = 0; n < nnodes[tdg_id]; ++n) {
		gtdg[tdg_id][n].id = nodes[n];
		gtdg[tdg_id][n].cnt = -1;
	}

	qsort(deps, ndeps, sizeof(*deps), cmpdstdep);
	dp = deps;
	inp = ins[tdg_id];
	for (mp = gtdg[tdg_id]; mp < mlim; ++mp)  {
		mp->offin = inp - ins[tdg_id];
		for (n = 0; dp < dlim && mp->id == dp->dst; ++n)
			*inp++ = lookup(tdg_id,(dp++)->src);
		mp->nin = n;
	}

	qsort(deps, ndeps, sizeof(*deps), cmpsrcdep);
	dp = deps;
	outp = outs[tdg_id];
	for (mp = gtdg[tdg_id]; mp < mlim; ++mp)  {
		mp->offout = outp - outs[tdg_id];
		for (n = 0; dp < dlim && mp->id == dp->src; ++n)
			*outp++ = lookup(tdg_id,(dp++)->dst);
		mp->nout = n;
	}

	if (optverbose) {
		printf("size map=%ld\nsize in=%ld\nsize out=%ld\ntotal=%ld\n",
						(long) nsiz, (long) dsiz, (long) dsiz,
						(long) nsiz + 2*dsiz);
	}
}

void
read_graph(void)
{
	char name[GRAPHNAME_MAX_LEN];
	next();

	if (token != SYMBOL || strcmp(tokenval.s, "digraph"))
		error("digraph expected");
	next();

	accept(SYMBOL);
    strcpy(name, toktext);
	expect('{');
	deplist();
	expect('}');

	graphName[tdg_id] =  xmalloc(sizeof(char)*GRAPHNAME_MAX_LEN);
	strcpy(graphName[tdg_id],name);
}

void
read_extra_infos(void)
{
	char name[GRAPHNAME_MAX_LEN];
	next();

	if (token != SYMBOL || strcmp(tokenval.s, "digraph"))
		error("digraph expected");
	next();

	accept(SYMBOL);
	strcpy(name, toktext);
	printf("name '%s' graphName '%s' cmp %d\n", name, graphName[tdg_id],strcmp(name, graphName[tdg_id]));
	
	if(!strcmp(name, graphName[tdg_id])) {
		expect('{');
		deplist();
		expect('}');
	}
	else {
		error("digraph name in file does not match what expected (i.e., '%s' against '%s')\n", name, graphName[tdg_id]);
	}
}

void
generate_gomp_file()
{
	int i, id, n;
	struct gomp_tdg *p;

	fputs("// File automatically generated\n",fout);	

	fputs("struct gomp_task;\n"
	      "struct gomp_tdg_t {\n"
	      "\tunsigned long id;\n"
	      "\tstruct gomp_task *task;\n"
	      "\tshort offin;\n"
	      "\tshort offout;\n"
	      "\tchar nin;\n"
	      "\tchar nout;\n"
	      "\tsigned char cnt;\n"
	      "\tint map;\n"
	      "\tlong task_counter;\n"
	      "\tlong task_counter_end;\n"
	      "\tlong runtime_counter;\n"
	      "};\n",fout);
	
// 	for(id=0; id<num_tdgs; id++) {
// 		p = gtdg[id];
// 		fprintf(fout, "'%s' (tdg_id %d)\n", graphName[id], id);
// 		for (p = gtdg[id], n = nnodes[id]; n--; ++p)
// 			printf("id %d p->id %d\n", id, p->id);
// 	}
	
	for(id=0; id<num_tdgs; id++) {
		fprintf(fout, "\n// TDG extracted from '%s' (tdg_id %d)\n", graphName[id], id);

		fprintf(fout,"struct gomp_tdg_t gomp_tdg_%d[%ld] = {\n", id, nnodes[id]);
		for (p = gtdg[id], n = nnodes[id]; n--; ++p) {
// 			printf("---> reading %d from mappings[%d][%ld] nnodes[%d] is %ld n is %d\n", mappings[id][nnodes[id]-n-1], id, nnodes[id]-n-1, id, nnodes[id], n);

		int currmap = -1;
		if((scheduling == SCHED_MAP_STATIC || scheduling == ANALYZE_STATIC)
			 && isSchedulable)
			currmap = mappings[id][nnodes[id]-n-1]; /* They were produced in topological order */
			
			fprintf(fout,
		    	"\t{.id = %u, .task = 0, "
		       	".offin = %d, .offout = %d, "
		       	".nin = %d, .nout = %d, .cnt = -1, .map = %d, .task_counter=0, .task_counter_end=0, .runtime_counter=0},\n",
		       	p->id, p->offin, p->offout, p->nin, p->nout, currmap);
		}
		fprintf(fout,"};\nunsigned short gomp_tdg_ins_%d[] = {\n\t", id);
		for (p = gtdg[id], n = nnodes[id];n--; ++p) {
			for (i = 0; i < p->nin; ++i)
				fprintf(fout, "%u,", ins[id][p->offin + i]);
		}
		fprintf(fout,"\n};\nunsigned short gomp_tdg_outs_%d[] = {\n\t", id);
		for (p = gtdg[id], n = nnodes[id];n--; ++p) {
			for (i = 0; i < p->nout; ++i)
				fprintf(fout, "%u,", outs[id][p->offout + i]);
		}
		fputs("\n};\n",fout);
	}

	fputs("// All TDGs are store in a single data structure\n",fout);
	fprintf(fout,"unsigned gomp_num_tdgs = %d;\n", num_tdgs);
	fprintf(fout,"\nstruct gomp_tdg_t *gomp_tdg[%d] = {\n\t",num_tdgs);
	for(id=0; id<num_tdgs; id++) 
		fprintf(fout,"gomp_tdg_%d,",id);
	fputs("\n};\n",fout);

	fprintf(fout,"\nunsigned short *gomp_tdg_ins[%d] = {\n\t",num_tdgs);
	for(id=0; id<num_tdgs; id++) 
		fprintf(fout,"gomp_tdg_ins_%d,",id);
	fputs("\n};\n",fout);

	fprintf(fout,"\nunsigned short *gomp_tdg_outs[%d] = {\n\t",num_tdgs);
	for(id=0; id<num_tdgs; id++) 
		fprintf(fout,"gomp_tdg_outs_%d,",id);
	fputs("\n};\n",fout);

	fprintf(fout,"\nunsigned gomp_tdg_ntasks[%d] = {\n\t", num_tdgs);
	for(id=0; id<num_tdgs; id++) 
		fprintf(fout,"%ld,",nnodes[id]);
	fputs("\n};\n",fout);

	fprintf(fout,"unsigned gomp_maxI[%d] = {\n\t", num_tdgs);
  	for(id=0; id<num_tdgs; id++) 
        fprintf(fout,"%ld,",maxI[id]);
    fputs("\n};\n",fout);

	fprintf(fout,"unsigned gomp_maxT[%d] = {\n\t", num_tdgs);
  	for(id=0; id<num_tdgs; id++) 
		fprintf(fout,"%ld,",maxT[id]);
    fputs("\n};\n",fout);
}

void
init_global_tdg_structs()
{
    int i;

	gtdg = xmalloc(sizeof(struct gomg_tdg *) * num_tdgs);
	outs = xmalloc(sizeof(unsigned short *) * num_tdgs);
	ins = xmalloc(sizeof(unsigned short *) * num_tdgs);
	nnodes = xmalloc(sizeof(long) * num_tdgs);
	maxT = xmalloc(sizeof(long) * num_tdgs);
	maxI = xmalloc(sizeof(long) * num_tdgs);
	graphName = xmalloc(sizeof(char *) * num_tdgs);

	for(i=0; i<num_tdgs; i++) {
		gtdg[i] = NULL;
		outs[i] = NULL;
		ins[i] = NULL;
		nnodes[i] = 0;
		maxT[i] = 0;
		maxI[i] = 0;
		graphName[i] = NULL;
	}
}

void resetGlobals() {
	// No need to free deps as it's allocated using xrealloc
	if(deps) free(deps);
	deps = NULL;
	lineno = ndeps = 0;
	free(MIETs);
	MIETs = 0;
	free(MEETs);
	MEETs = 0;
	free(MAETs);
	MAETs = 0;
}

void
free_global_tdg_structs()
{
    int i;

	for(i=0; i<num_tdgs; i++) {
		if(gtdg[i]) free(gtdg[i]);
		if(outs[i]) free(outs[i]);
		if(ins[i]) free(ins[i]);
	}
	if(gtdg) free(gtdg);
	if(outs) free(outs);
	if(ins) free(ins);
	if(nnodes) free(nnodes);
	if(maxT) free(maxT);
	if(maxI) free(maxI);
	if(graphName) free(graphName);
	if(deps) free(deps);
}

void
help()
{
	printf("\nUsage: psoc_mapper [OPTION]... FILE...\n");
	printf("Create the table with OpenMP task parts to be send to accelerator thread queues\n");
	printf("\nMandatory arguments to long options are mandatory for short options too.\n");
	printf("  -a\t\tProduce mapping and scheduling using MAET. Only one between options '-a|-e|-i' can be specified. Default '-i'.\n");
	printf("  -c\t\tSpecify number of cores for mapping and schedulability analysis (default %d)\n", DEFAULT_NCORES);
	printf("  -d\t\tMap OpenMP task parts in thread local queue, and perform schedulability analysis for the DYNAMIC|GLOBAL approach.\n");
	printf("    \t\tOnly one between '-d|-s' can be specified. By default don't either map or perform schedulability analysis\n");
	printf("  -e\t\tProduce mapping and scheduling using MEET Only one between options '-a|-e|-i' can be specified. Default '-i'.\n");
	printf("  -f\t\tPerform schedulability analysis for the STATIC|PARTITIONED approach with the mapping given in the .dot input files\n");
	printf("  -i\t\tProduce mapping and scheduling using MIET Only one between options '-a|-e|-i' can be specified. Default '-i'.\n");
	// printf("  -r\t\tMap OpenMP task parts in thread local queue in RR fashion, and don't perform any schedulability analysis (always return \"Yes\")\n");
	printf("  -o FILE\tOuput task table to file\n");
	printf("  -s\t\tMap OpenMP task parts in global queue, and perform schedulability analysis for the STATIC|PARTITIONED approach\n");
	printf("    \t\tOnly one between '-d|-s' can be specified. By default don't either map or perform schedulability analysis\n");
	printf("  -v\t\tIncrease verbosity level\n");
	printf("\n");
}

void
usage(void)
{
	help();
	//fputs("usage: psoc_mapper [-v | -s | -d] [-o output] [graph-files\n",
	//      stderr);
	exit(EXIT_FAILURE);
}

int
main(int argc, char *argv[])
{
	char *cp;
	int c, i, countFiles;
	struct dag_t **dag;
	timing_t timing = TIMING_NONE;
	printf("psoc_mapper Tool version %s (c) The P-SOCRATES Consortium\n", VERSION_STRING);
	
	if(argc < 2) {
		printf("psoc_mapper: No input files specified\n");
		usage();
		exit(EXIT_FAILURE);
	}

	--argc;
	while (*++argv && argv[0][0] == '-') {
		--argc;
		for (cp = &argv[0][1]; (c = *cp); ++cp) {
			switch (c) {
				case 'v':
					optverbose = 1;
					break;
				// output file
				case 'o':
					if (*++argv == NULL)
						usage();
					--argc;
					output = *argv;
					break;
					
				/* ncores */
				case 'c':
					if (*++argv == NULL)
						usage();
					--argc;
					ncores = atoi(*argv);
					break;
				
				/* Print help */
				case 'h':
					help();
					return 0;
					
				/* Sched/mapping types */
				case 's':
					scheduling = SCHED_MAP_STATIC;
					break;					
				case 'd':
					scheduling = SCHED_DYNAMIC;
					break;					
				case 'r':
					scheduling = SCHED_RR;
					break;
				case 'f':
					scheduling = ANALYZE_STATIC;
					break;
					
				/* Timing analysis attributes */
				case 'a':
					if(timing != TIMING_NONE)
					{
						fputs("psoc_mapper: Error. You can only specify one timing attribute (-a | -e | -i)\n\n", stderr);
						exit(EXIT_FAILURE);
					}
					timing = TIMING_MAET;
					break;
				case 'e':
					if(timing != TIMING_NONE)
					{
						fputs("psoc_mapper: Error. You can only specify one timing attribute (-a | -e | -i)\n\n", stderr);
						exit(EXIT_FAILURE);
					}
					timing =  TIMING_MEET;
					break;
				case 'i':
					if(timing != TIMING_NONE)
					{
						fputs("psoc_mapper: Error. You can only specify one timing attribute (-a | -e | -i)\n\n", stderr);
						exit(EXIT_FAILURE);
					}
					timing = TIMING_MIET;
					break;
					
				default:
					usage();
			}
		}
	} // end of flags parsing
	// The rest of arguments are TDGs included in .dot files
	num_tdgs = argc;
	
	if(timing == TIMING_NONE) {
		if(scheduling == ANALYZE_STATIC) {
			timing = TIMING_MAET;
			printf("Performing static sched. analysis: using MAET by default as timing attribute\n");
		}
		else {
			timing = TIMING_MIET;
			printf("Using MIET by default as timing attribute\n");
		}
	}
	
	if(fout)
		fclose(fout);
	if (output) {
		if ((fout = fopen(output, "w")) == NULL) {
			perror("psoc_mapper: Opening output file");
			exit(EXIT_FAILURE);
		}
	} else {
		output = "<stdout>";
		fout = stdout;
	}
	
	init_global_tdg_structs();
	// Init local tdg structs
	dag = xmalloc(sizeof(struct dag_t *) * num_tdgs);
	for(i=0; i<num_tdgs; i++) 
		dag[i] = NULL;
	
	for(countFiles=0; countFiles<num_tdgs; countFiles++) {
// 		printf("*argv is %s\n", *argv);

		if(fgraph != 0)
			fclose(fgraph);
		if ((filename = *argv)) {
			if ((fgraph = fopen(filename, "r")) == NULL) {
				perror("psoc_mapper: Opening input file");
				exit(EXIT_FAILURE);
			}
		} else {
			filename = "<stdin>";
			fgraph = stdin;
		}

		read_graph();
		
		char readAdditionalNodes = 1;
		if(readAdditionalNodes) {
			char additionalNodesFilename[GRAPHNAME_MAX_LEN + 10];
// 			snprintf(additionalNodesFilename, strlen(graphName[tdg_id]) + 4, "%s-extra.dot", graphName[tdg_id]);
			sprintf(additionalNodesFilename, "%s-extra.dot", graphName[tdg_id]);
			printf("Will read additional nodes for graph '%s' from file is '%s'\n", graphName[tdg_id], additionalNodesFilename);
			 
			if(fgraph) {
				fclose(fgraph);
				fgraph = NULL;
			}
			if ((fgraph = fopen(additionalNodesFilename, "r")) == NULL) {
				printf("Cannot find file %s. Won't read any additional node for graph '%s'\n", additionalNodesFilename, graphName[tdg_id]);
			}
			else {
				read_extra_infos();
			} // else file found			
		} // if readAdditionalNodes
		
		create_tdg_struct();
		
		int err;
		if(scheduling != NONE) {
			wcet_t *wcet;
			switch(timing) {
				case TIMING_MIET:
					wcet = &MIETs[0];
					break;
				case TIMING_MEET:
					wcet = &MEETs[0];
					break;
				case TIMING_MAET:
					wcet = &MAETs[0];
					break;
				default:
					wcet = &MIETs[0];
					break;
			} // switch
			
			err = dagCreate(graphName[tdg_id], tdg_id, &nodes[0], &wcet[0], &nodeMap[0], nnodes[tdg_id], &deps[0], ndeps, 1 /* Fix single source-single sink */, &dag[tdg_id]);
			
			if(err) {
				printf("Error %d creating DAG! Exiting...\n", err);
				return 1;
			}
			
			err = dagAssignParams(dag[tdg_id], 0);
			if(err) {
				printf("Error %d assigning DAG params to (%s - tdg_id %d)! Exiting...\n", err, graphName[tdg_id], tdg_id);
				return 1;
			}
		}
		resetGlobals();
			
		argv++;
	} // for countFiles<num_tdgs
	
	if(scheduling == NONE)
	{
		printf("No schedulability analysis neither mapping were performed. Use -s|-d switch to enable them\n");
		fprintf(fout, "// No schedulability analysis neither mapping were performed. Use -s|-d switch to enable them\n");
	}
	else
	{
		isSchedulable = 0;
		
		char action_performed_friendly[200];
		switch(scheduling)
		{
			case NONE:
				sprintf(action_performed_friendly, "No action performed by mapping+analysis tool");
				break;
				
			case  SCHED_MAP_STATIC:
				sprintf(action_performed_friendly, "Static/partitioned mapping + schedulability analysis");
				break;
				
			case SCHED_DYNAMIC:
				sprintf(action_performed_friendly, "Dynamic/global mapping + schedulability analysis");
				break;
				
			case ANALYZE_STATIC:
				sprintf(action_performed_friendly, "Schedulability analysis for a given static/partitioned mapping");
				break;
				
			case SCHED_RR:
				sprintf(action_performed_friendly, "Round robin mapping, no schedulability analysis");
				break;
				
			default:
				sprintf(action_performed_friendly, "Unknown action");
				break;
		}
		
		printf("Action performed by mapping tool: %s\n", action_performed_friendly);
		fprintf(fout, "// Action performed by mapping tool: %s\n",action_performed_friendly);
		
		switch(scheduling) {
			
			case SCHED_MAP_STATIC:
				isSchedulable = staticSchedulabilityAnalysisAndMapping(&dag[0], num_tdgs, ncores);	
				break;
				
			case SCHED_DYNAMIC:
				isSchedulable = dynamicSchedulabilityAnalysis(&dag[0], num_tdgs, ncores);	
				break;
				
				/* For debug */
			case SCHED_RR:
				// I do this later, when invoking rrGetMapping
				isSchedulable = 1;
				break;
				
			case ANALYZE_STATIC:
				isSchedulable = staticSchedulabilityAnalysis(&dag[0], num_tdgs, ncores);	
				break;
				
			default:
				break;
		} // switch

		printf("Application schedulable: \"%s\"\n", isSchedulable ? "Yes" : "No");
		for(i=0; i<num_tdgs; i++) {
			printf("Offload %d: Analysis response time = %lld \n", i, dag[i]->R);
		}
		
		printf("Schedulability analysis returned: \"%s\"\n", isSchedulable ? "Yes" : "No");
		fprintf(fout, "// Schedulability analysis returned: \"%s\"\n", isSchedulable ? "Yes" : "No");
		fprintf(fout, "char isSchedulable = %d;\n\n", isSchedulable ? 1 : 0);
	
		// scheduler puts DAGS in its own order: reorder them!
		qsort(&dag[0], num_tdgs, sizeof(struct dag_t *), cmpDAGsTdgId);
			
		mappings = (thread_t **) malloc(sizeof(thread_t *) * num_tdgs);
	} // if scheduling != NONE
// 	printf("Allocated mappings root struct @ 0x%d, size %ld (%d tdgs)\n", _mycast_ mappings, sizeof(thread_t *) * num_tdgs, num_tdgs);
	
	for(i=0; i<num_tdgs; i++) {
		if(isSchedulable) {
			// This array cannot be empty
			mappings[i] = (thread_t *) malloc(sizeof(thread_t) * dag[i]->num_nodes);
// 			printf("Allocated mappings struct @ 0x%d, size %ld (%ld nodes)\n", _mycast_ mappings[i], sizeof(thread_t) * dag[i]->num_nodes, dag[i]->num_nodes);
			
			switch(scheduling) {
				case ANALYZE_STATIC:
				case SCHED_MAP_STATIC:
					staticGetMapping(dag[i], mappings[i]);
					break;
					
				case SCHED_RR:
					rrGetMapping(dag[i], mappings[i], ncores);
					break;
				
				/* With _DYNAMIC and _NONE of course there is no mapping */
				default:
					break;
			} // switch
			
#if 0
			int k;
			printf("Mapping for DAG %s (tdg_id %d) - %ld nodes:\n", dag[i]->name, dag[i]->tdg_id, dag[i]->num_nodes);
			for(k=0; k<dag[i]->num_nodes; k++)
				printf("\t- [k %d] Node %ld (idMercurium %ld) -> thread %d mappings[%d][%d] %d \n", k,
							 nodes[k], dag[i]->v[k].idMercurium, dag[i]->v[k].thread, i, k, mappings[i][k]);
			printf("\n\n");
#endif
			
			
		} // if is schedulable
		else {
// 			_log("Not schedulable! (i=%d) \n", i);
			if(mappings)
				mappings[i] = NULL;
		}
	} // for

	generate_gomp_file();
		
	/** Done. Dispose everything */
	free_global_tdg_structs();
	// Free local structures
	for(i=0; i<num_tdgs; i++) {
		if(mappings && mappings[i])
			free(mappings[i]);		
		dagDispose(dag[i]);
	}
	if(mappings)
		free(mappings);
	if(dag)
		free(dag);
	if(fgraph)
		fclose(fgraph);
	
	return 0;
}
