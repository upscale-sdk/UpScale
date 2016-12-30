/* -----------------------------------------------------------------------------
 * Author list (in alphabetic order):
 * 
 *        Eduardo Quiñones <eduardo.quinones@bsc.es>
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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "boxer.h"


/*
  * struct box is stored in a tree, where it is stored a pointer
  * to the id of this struct. If we want to recover the struct
  * from the information returned by the tree, id must be
  * the first field of this struct.
  */
struct box {
	unsigned long id;
	char type;
	struct order order;
	unsigned nchild;
	char *locus;
	struct box *brother;
	struct box *child;
	struct box *next;
	struct expr *expr;
	union {
		struct task *task;
		int null;
	} u;
};

unsigned maxI;

static struct box *exptask(struct box *this);
static struct box *expcond(struct box *this);
static struct box *exploop(struct box *this);
static struct box *expexcl(struct box *this);
static struct box *expseq(struct box *this);

static struct box *(*exptbl[])(struct box *) = {
	[TBOX] = exptask,
	[CBOX] = expcond,
	[LBOX] = exploop,
	[EBOX] = expexcl,
	[SBOX] = expseq
};

static struct box *boxlist;  /* pointer to the head of the full boxlist */

unsigned long
newid(void)
{
	static unsigned long id;

	if (++id == 0) /* TODO: use a mask here */
		die("overflow in generated task id");
	return AUXID(id);
}

static struct box *
new(char type, struct box *child, struct box *brother)
{
	struct box *this;
	extern int opt_free;

	this = xcalloc(1, sizeof(struct box));
	this->type = type;
	this->child = child;
	this->brother = brother;
	this->id = (opt_free) ? 0: TREEID(TYPBOX, 0, newid());
	this->next = boxlist;
	return boxlist = this;
}

static void
killbox(struct box *this)
{
	struct box *p;

	for (p = this->child; p != NULL; p = p->brother) {
		if (p->type == TBOX)
			killtask(p->u.task);
		else if (p->child != NULL)
			killbox(p);
	}
	free(this->order.vec);
	if (this->id == 0)
		free(this);
}

int
delempty(struct box *box)
{
	struct box *p, *q, *next;
	int dirty, wasdirty = 0;

repeat:
	dirty = 0;
	for (p = q = box->child; p; q = p, p = next) {
		next = p->brother;
		if (p->child) {
			dirty |= delempty(p);
		} else if (p->type != TBOX || !p->u.task) {
			/* remove p */
			dirty = 1;
			killbox(p);
			if (p == q) {
				box->child = next;
				p = next;
			} else {
				q->brother = next;
				p = q;
			}
		}
	}
	if (dirty) {
		wasdirty = 1;
		goto repeat;
	}
	return wasdirty;
}

/*
 * Collapse an EBOX when it has only one child or  it is of the same type
 * that its father.
 */
int
collapsebox(struct box *box)
{
	struct box *p, *q, *next, *child;
	int dirty, wasdirty = 0;

repeat:
	dirty = 0;
	for (p = q = box->child; p; q = p, p = next) {
		next = p->brother;
		child = p->child;
		if (p->type == EBOX &&
		    (child && !child->brother ||
		     box->type == EBOX && p->u.null == box->u.null)) {
			dirty = 1;
			if (p == q)
				box->child = child;
			else
				q->brother = child;
			p->child = NULL;
			killbox(p);
			p = child;
			addchild(box, next);
		} else {
			dirty |= collapsebox(p);
		}
	}
	if (dirty) {
		wasdirty = 1;
		goto repeat;
	}
	return wasdirty;
}

struct box *
addchild(struct box *parent, struct box *box)
{
	struct box *p, *q;

	for (p = q = parent->child; p; q = p, p = p->brother) {
		if (p == box)
			return parent;
	}
	if (q == NULL)
		parent->child = box;
	else
		q->brother = box;
	return parent;		
}

struct box *
getnchild(struct box *box, unsigned n)
{
	struct box *p;

	for (p = box->child; p && --n > 0; p = p->brother)
		/* nothing */;
	return (n > 0) ? NULL : p;
}

struct box *
addtask(struct box *box, struct task *task)
{
	if (box->type != TBOX)
		die("assigning a task to a non TBOX");
	box->u.task = task;

	if (!box->expr)
		box->expr = newexpr("1", NULL);

	taskexpr(task, box->expr);
	return box;
}

struct box *
newbox(unsigned long id, int type, char *locus, struct expr *e)
{
	struct box *box;

	box = new(type, NULL, NULL);
	box->locus = xstrdup(locus);
	box->expr = e;
	if (id != 0) {
		box->id = TREEID(TYPBOX, 0, id);
		if (!install(&box->id))
			die("box definition '%lu' duplicated", box->id);
	}

	return box;
}

static struct box *expseq(struct box *this);

int
expandbox(struct box *root)
{
	struct box *p;

	root->child = expseq(root);

	/*
	 * A box diagram is complete when all the boxes of it
	 * are TBOX. At this moment it is full expanded.
	 */
	for (p = root->child; p && p->type == TBOX; p = p->brother)
		/* nothing */;
	return p == NULL;
}

static struct order
order(struct order *p, unsigned n, int mark)
{
	struct order order;
	size_t len;

	order = *p;
	len = ++order.nr;
	order.vec = xmalloc(sizeof(unsigned) * len);
	if (len > 1)
		memcpy(order.vec, p->vec,  (len-1) * sizeof(*p->vec));
	order.vec[len-1] = n << 1 | mark;
	return order;
}

int
cmporder(struct order *o1, struct order *o2)
{
	size_t  len1 = o1->nr, len2 = o2->nr;
	unsigned *p, *q;
	int r;

	for (p = o1->vec, q = o2->vec; len1-- && len2--; ++p, ++q) {
		if ((r = *p - *q) != 0)
			return r;
	}

	if (len1 == len2)
		return 0;
	return len1 ? 1 : -1;
}

static void setorder(struct box *this, struct order *order, unsigned n);
void enumtask(struct task *task, struct order *order);

static void
enumbox(struct box *this)
{
	struct box *p;
	unsigned n;

	if (this->type == TBOX) {
		enumtask(this->u.task, &this->order);
		return;
	}
	this->nchild = 0;
	for (p = this->child; p; p = p->brother) {
		if (!p->order.vec) {
			if (this->type == EBOX)
				n = 1;
			else if ((n = ++this->nchild) == 0)
				die("box numeration overflow");
			setorder(p, &this->order, n);
		}
	}
}

static void
setorder(struct box *this, struct order *o, unsigned n)
{
	this->order = order(o, n, 0);
	if (this->type == TBOX)
		enumtask(this->u.task, &this->order);
	else
		enumbox(this);
}

struct box *
enumerate(struct box *this)
{
	struct box *p;
	struct order o;
	unsigned n;

	memset(&o, 0, sizeof(o));
	n = 0;
	for (p = this; p; p = p->brother) {
		if (!p->order.vec)
			setorder(p, &o, ++n);
	}

	return this;
}

static struct box *
copy(struct box *this)
{
	struct box *p;
	struct task *task;

	if (this == NULL)
		return NULL;
	p = new(this->type, copy(this->child), copy(this->brother));
	p->locus = this->locus;
	p->expr = dupexpr(this->expr);
	if (p->type == TBOX) {
		task = duptask(this->u.task);
		taskexpr(task, p->expr);
		p->u.task = task;
	} else {
		p->u = this->u;
	}

	return p;
}

static struct box *
expcond(struct box *this)
{
	struct box *p;

	if (!evalable(this->expr))
		return this;
	if (evalcond(this->expr)) {
		p = this->child;
		this->child = NULL;
	} else {
		p = NULL;
	}

	killbox(this);
	return p;
}

/*
 * we are looking for a correct expansion, and it means
 * an expansion different of NULL and different of the
 * original box. If a box returns NULL, then this box
 * must be removed from the list of brothers, but it
 * do not stop the expansion process.
 */
static struct box *
expexcl(struct box *this)
{
	struct box *p, *q, *last, *found, *child;

	found = last = NULL;

	for (p = this->child; p && !found; p = p->brother) {
		q = (*exptbl[p->type])(p);
		if (q != p && q != NULL) {
			found = q;
			q = p->brother;
		}
		if (!last) {
			this->child = last = q;
		} else {
			last->brother = q;
			last = q;
		}
	}

	child = this->child;
	if (!found && child && !child->brother) {
		found = child;
		this->child = NULL;		
	} else if (!found && child) {
		return this;
	}

	killbox(this);
	return found;
}

static void
propagate(void *p1, void *p2)
{
	struct value *v = p1;
	struct box *this = p2;

	if (!this)
		return;
	for (; this; this = this->brother) {
		if (definevalue(this->expr, v))
			propagate(v, this->child);
	}
}

static struct box *
exploop(struct box *this)
{
	struct box *tree = NULL, *last = NULL, *p, *childp;
	struct values *v = NULL;
	struct order *orderp;
	unsigned n;
	extern int opt_debug;

	childp = this->child;
	orderp = &this->order;

	if (!evalable(this->expr))
		return this;
	this->nchild = 0;
	if (opt_debug == 3)
		fprintf(stderr, "expanding loop %s\n", this->locus);
	for (n = 0; v = nextiter(this->expr, v, n); ++n) {
		if (n+1 > maxI)
			maxI = n+1;
		p = copy(childp);
		if (!tree)
			tree = p;
		if (last)
			last->brother = p;
		forallvalues(v, p, propagate);
		/*
		 * enumerate all the childs in each loop expansion
		 */
		while (p) {
			last = p;
			last->order = order(orderp, ++this->nchild, 1);
			enumbox(last);
			p = last->brother;
		}
	}

	killbox(this);
	return tree;
}

static struct box *
exptask(struct box *this)
{
	return this;
}

static struct box *
expseq(struct box *this)
{
	struct box *tree, *last, *q, *p, *next;

	tree = last = NULL;
	for (p = this->child; p; p = next) {
		next = p->brother;
		q = (*exptbl[p->type])(p);
		if (!tree)
			tree = q;
		if (last)
			last->brother = q;
		/*
		 * We want to concatenate the list returned by
		 * expand() to the list pinted by last. A previous
		 * expansion could put some boxes before of
		 * the next brother of this, that it is pointed by next,
		 * so we have to search the last element before next
		 */
		while (q && q != next) {
			last = q;
			q = last->brother;
		}
	}

	return tree;
}

unsigned long
order2hash(unsigned long id, struct order *order)
{
	unsigned long h = 0, v;
	unsigned *p, n;
	extern unsigned maxI, maxT;

	if (n = order->nr) {
		for (p = &order->vec[n-1]; n--; --p) {
				v = *p;
				if ((v & 1) == 0)
					continue;
				v >>= 1;
				h += v;
				h *= maxI;
		}
	}

	h *= maxT;
	h += id;
//    printf("Task id %d results in task instance id %d\n",id,h);
	return h;
}

char *
order2str(struct order *order, int loops)
{
	unsigned *p, off = 0, n, v;
	static char buf[1024];

	if (!order->vec)
		return "?";

	for (p = order->vec, n = order->nr; n ; ++p, --n) {
		v = *p;
		if (loops) {
			 if ((v&1) == 0)
				continue;
			v >>= 1;
		}
		off += sprintf(buf+off, "%u", v);
		buf[off++] = '.';
	}
	if (off > 1)
		--off;
	buf[off] = '\0';

	return buf;
}

void
printbox(struct box *box, FILE *fp)
{
	extern unsigned long parent(struct task *task);
	extern char *getexpr(struct expr *e);
	unsigned long id = box->id;
	static char *boxnames[] = {
		[TBOX] = "tbox",
		[EBOX] = "ebox",
		[LBOX] = "lbox",
		[CBOX] = "cbox",
		[SBOX] = "sbox"
	};

	fprintf(fp, "%lu [shape=box,label=\"%s%c %lu\\n%s\\n%s\"]\n",
	        id, boxnames[box->type],
	        (box->type == EBOX && box->u.null) ? '*' : ' ',
	        id, order2str(&box->order, 0), getexpr(box->expr));
	if (box->type == TBOX) {
		unsigned long tid = OBJ2ID(box->u.task);
		fprintf(fp, "t%lu [group=task,label=\"%lu\\n%lu\"]\n"
		        "%lu -> t%lu [style=dotted]\n",
		        tid, tid, parent(box->u.task), id, tid);
	}
	if (box->child) {
		fprintf(fp, "%lu -> %lu [label=child]\n",
		        id, box->child->id);
		printbox(box->child, fp);
	}
	if (box->brother)  {
		fprintf(fp, "%lu -> %lu [label=brother]\n",
		        id, box->brother->id);
		printbox(box->brother, fp);
	}
}

static void
printrank(struct box *box, FILE *fp)
{
	struct box *p;

	fputs("{ rank = same; ", fp);
	for (p = box; p; p = p->brother)
		fprintf(fp, "%lu%c", p->id, (p->brother) ? ';' : '}');
	putc('\n', fp);
	for (p = box; p; p = p->brother) {
		if (p->child)
			printrank(p->child, fp);
	}
}

void
printboxes(struct box *root, FILE *fp)
{
	printbox(root, fp);
	printrank(root, fp);
}
