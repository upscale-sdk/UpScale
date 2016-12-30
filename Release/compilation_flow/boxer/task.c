/* -----------------------------------------------------------------------------
 * Author list (in alphabetic order):
 * 
 *        Eduardo Quiñones <eduardo.quinones@bsc.es>
 *        Sara Royuela <sara.royuela@bsc.es>
 *        Roberto Vargas <roberto.vargas@bsc.es>
 * 
 * -----------------------------------------------------------------------------
 * (C) Copyright 2016 Barcelona Supercomputing Center
 * 
 * This file is part of boxer (Compilation flow) of the P-SOCRATES project.
 *
 * Boxer is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published 
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Boxer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with boxer. If not, see <http://www.gnu.org/licenses/>.
 * -----------------------------------------------------------------------------
 */

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "boxer.h"

struct task;

struct deplist {
	struct task *dst;
	struct deplist *next;
};

/*
 * struct task is stored in a tree, where it is stored a pointer
 * to the id of this struct. If we want to recover the struct
 * from the information returned by the tree, id must be
 * the first field of this struct.
 */
struct task {
	unsigned long id;
	char killed;
	struct task *parent;
	char type;
	bool visited : 1;
	bool pushed : 1;
	char *locus;
	struct expr *expr;  /* pointer to tbox expression */
	struct order *order;    /* pointer to the time order */
	struct task *child;     /* pointer to the first direct instance */
	struct task *brother;   /* pointer to next direct instance */
	struct task *leaf;      /* next in leaf task list */
	struct task *instances; /* pointer to the leaf task list */
	struct task *next;      /* generic next pointer used for linked list */
	struct task *lnext;     /* next pointer used for leaf list */
	struct deplist *dlist;  /* list of dependencies of this task */
};

struct tleaf {
	struct task *head;
	struct task *tail;
};

struct dep {
	char transitive;
	struct task *src;
	struct task *dst;
	struct expr *expr;
	struct dep *next;
};

unsigned maxT;
static unsigned nleafs; /* number of leaf task generated */
struct dep *dhead;      /* head of the json dependencies list */
struct task *thead;     /* head of the json task list */
struct task *tlhead;    /* head of leaf task list */
struct dep *genhead;    /* head of generated dependencies */

extern void reset_tree_ids();


void 
reset_task_globals(void)
{
    nleafs = 0;
	
	struct task *t = thead, *next_t;
    while(t) {
		next_t = t->next;
		if(t->dlist) 
			free(t->dlist);
		free(t);
		t = next_t;
	}

	struct dep *d = genhead, *next_d;
	while(d) {
		next_d = d->next;
		free(d);
		d = next_d;
	}

	d=dhead;
	while(d) {
		next_d = d->next;
		free(d);
		d = next_d;
	}

	thead = NULL;
    tlhead = NULL;
    genhead = NULL;
    dhead = NULL;

	reset_tree_ids();
}

void
newdep(struct expr *e, unsigned long src, unsigned long dst)
{
	struct task *source, * destine;
	struct dep *dep;

	if (!(source = lookup(src)) || !(destine = lookup(dst)))
		die("one or more unknown tasks in dependency");
	normalizedep(e, source->expr, destine->expr);
	dep = xmalloc(sizeof(*dep));
	dep->transitive = 0;
	dep->src = source;
	dep->dst = destine;
	dep->expr = e;
	dep->next = dhead;
	dhead = dep;
}

void
killtask(struct task *task)
{
	task->killed = 1;
	killexpr(task->expr);
	/*
	 * order is own by the box, so it is not needed to free it
	 */
	/*
	 * we can not free the task because we need it to
	 * find the leafs of a pragma task
	 */
}

unsigned long
parent(struct task *task)
{
	return task->parent->id;
}

struct task *
duptask(struct task *task)
{
	struct task *p;

	p = xmalloc(sizeof(*task));
	p->child = p->leaf = NULL;
	p->brother = task->child;
	task->child = p;
	p->type = task->type;
	p->parent = task->parent;
	p->killed = p->visited = 0;
	p->locus = task->locus;
	p->id = TREEID(TYPTASK, 0, newid());
	p->order = NULL;
	p->dlist = NULL;
	p->lnext = p->next = NULL;
	return p;
}

void
enumtask(struct task *task, struct order *order)
{
	assert(!task->order || task->order == order);
	task->order = order;
}

void
taskexpr(struct task *task, struct expr *expr)
{
	task->expr = expr;
}

struct task *
newtask(unsigned id, int type, char *locus)
{
	struct task *taskp;

	if (id > maxT)
		maxT = id;
	id = TREEID(TYPTASK, 0, id);
	taskp = xcalloc(1, sizeof(*taskp));
	taskp->parent = taskp;
	taskp->id = id;
	taskp->type = type;
	taskp->locus = xstrdup(locus);
	taskp->leaf = taskp->brother = taskp->child = NULL;
	taskp->order = NULL;
	taskp->dlist = NULL;
	if (!install(&taskp->id))
		die("duplicated task '%lu'", id);

	taskp->lnext = NULL;
	taskp->next = thead;
	return thead = taskp;
}

static void
linkleafs(struct task *task, struct tleaf *leaf)
{
	for ( ;task ; task = task->brother) {
		if (task->child) {
			linkleafs(task->child, leaf);
		} else if (!task->killed) {
			task->lnext = tlhead;
			tlhead = task;
			++nleafs;
			if (!leaf->head) {
				leaf->head = leaf->tail = task;
			} else {
				leaf->tail->leaf = task;
				leaf->tail = task;
			}
		}
	}
}

void
genleafs(void)
{
	struct task *task;
	struct tleaf leaf;

	for (task = thead; task; task = task->next) {
		memset(&leaf, 0, sizeof(leaf));
		linkleafs(task, &leaf);
		task->instances = leaf.head;
	}
}

void
printjdeps(FILE *fp)
{
	struct dep *p;
	char *s;

	for (p = dhead; p; p = p->next) {
		s = getexpr(p->expr);
		fprintf(fp, "t%lu -> t%lu [label=\"%.20s%s\"]\n",
		         p->src->id, p->dst->id,
		         s, (strlen(s) > 20) ? "..." : "");
	}
}

void
printdeps(FILE *fp)
{
	struct dep *p;
	struct task *tp, *q;
	static char *taskfmt[] = {
		"\t%lu\n", "\t%lu[label=\"%c\\n%lu\\n%s\\n%lu\"]\n"
	};
	extern int opt_parent, opt_nodes;
	unsigned id;

	for (tp = thead; tp; tp = tp->next) {
		if (tp->type != NTASK && !opt_nodes)
			continue;
		tp = tp->parent;
		for (q = tp->instances; q; q = q->leaf) {
			id = order2hash(tp->parent->id, q->order);
			fprintf(fp, taskfmt[opt_parent],
			        id, tp->type, tp->parent->id,
				order2str(q->order,1), id);
		}
	}

	for (p = genhead; p; p = p->next) {
		extern int opt_transitive;
		struct task *src = p->src, *dst = p->dst;

		if ((dst->type != NTASK || src->type != NTASK) && !opt_nodes ||
		    p->transitive && opt_transitive) {
			continue;
		}
		fprintf(fp, "\t%lu -> %lu\n",
		        order2hash(src->parent->id, src->order),
		        order2hash(dst->parent->id, dst->order));
	}
}

static bool
isconnected(struct task **queue, struct task *src, struct task *dst)
{
	struct task **sp, *tp, *cur;
	struct deplist *listp;
	for (tp = tlhead; tp; tp = tp->lnext)
		tp->pushed = 0;

	sp = queue;
	for (cur = src;; cur = *--sp) {
		for (listp = cur->dlist; listp; listp = listp->next) {
			tp = listp->dst;
			if (cur != src && tp == dst)
				return 1;
			if (cur == src && tp == dst
			||  cmporder(tp->order, dst->order) > 0
			||  tp->pushed)
				continue;
			tp->pushed = 1;
			*sp++ = tp;
		}
		if (sp == queue)
			return 0;
	}
}

static void
createldeps(void)
{
	struct dep *dep1, *dep2;
	struct deplist *dlist;
	struct task *tp;

	for (dep1 = genhead; dep1; dep1 = dep1->next) {
		tp = dep1->src;
		if (tp->visited)
			continue;
		tp->visited = 1;
		for (dep2 = genhead; dep2; dep2 = dep2->next) {
			if (dep2->src != tp)
				continue;
			dlist = xmalloc(sizeof(*dlist));
			dlist->next = tp->dlist;
			dlist->dst = dep2->dst;
			tp->dlist = dlist;
		}
	}
}

static void
chktransitive(void)
{
	struct dep *dep;
	struct task  **queue;
	queue = xmalloc(nleafs * sizeof(*queue));
	createldeps();
	for (dep = genhead; dep; dep = dep->next)
		dep->transitive = isconnected(queue, dep->src, dep->dst);

	free(queue);
}

void
expanddep(void)
{
	struct dep *p, *dep;
	struct task *src, *dst;
	extern int opt_debug;

	for (p = dhead; p; p = p->next) {
		for (src = p->src->instances; src; src = src->leaf) {
			for (dst = p->dst->instances; dst; dst = dst->leaf) {
				if (cmporder(src->order, dst->order) >= 0)
					continue;
                                if (opt_debug == 3) {
					fprintf(stderr,
					        "check dependence between %ld-%ld\n",
					         src->parent->id, dst->parent->id);
				}
				if (!evalcond2(p->expr, src->expr, dst->expr))
					continue;
				dep = xmalloc(sizeof(*dep));
				dep->src = src;
				dep->dst = dst;
				dep->next = genhead;
				genhead = dep;
			}
		}
	}
	chktransitive();
}
