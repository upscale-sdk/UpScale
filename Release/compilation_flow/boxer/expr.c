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

#define _XOPEN_SOURCE

#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include "boxer.h"

/*
  * struct var is stored in a tree, where it is stored a pointer
  * to the id of this struct. If we want to recover the struct
  * from the information returned by the tree, id must be
  * the first field of this struct.
  */
struct var {
	unsigned long id;
	char *name;
	char *locus;
	char *type;
};

struct value {
	struct var *var;
	char defined, evaluated;
	int side;
	char *desc;
};

struct values {
	unsigned nr;
	struct value *vec;
};

struct expr {
	char *text;
	struct values *values;
};

static FILE *ccin, *ccout;
static pid_t child;

struct var *
newvar(unsigned long id, char *name, char *locus, char *type)
{
	struct var *var = xmalloc(sizeof(*var));

	var->id  = TREEID(TYPVAR, 0, id);
	var->name = xstrdup(name);
	var->locus = xstrdup(locus);
	var->type = xstrdup(type);
	if (!install(&var->id))
		die("var definition '%lu' duplicated", id);
	return var;
}

struct values *
newvalue(struct values *vals, struct var *var, char *desc, int side)
{
	size_t size;
	struct value *v;

	if (!vals)
		vals = xcalloc(1, sizeof(*vals));
	size = ++vals->nr * sizeof(*vals->vec);
	vals->vec = xrealloc(vals->vec, size);
	v = &vals->vec[vals->nr-1];
	v->var = var;
	v->desc = xstrdup(desc);
	v->side = side;

	return vals;
}

struct expr *
newexpr(char *text, struct values *values)
{
	struct expr *e = xmalloc(sizeof(*e));

	e->text = (text) ? xstrdup(text) : NULL;
	e->values = values;
	return e;
}

void
freevalues(struct values *v)
{
	struct value *p;
	int nr;

	if (!v)
		return;
	nr = v->nr;
	for (p = v->vec; nr--; ++p)
		free(p->desc);
	free(v);
}

void
killexpr(struct expr *exp)
{
	freevalues(exp->values);
	free(exp->text);
	free(exp);
}

static struct values *
dupvalues(struct values *src, int dupdesc)
{
	struct values *dst;
	size_t size;
	unsigned nr;
	struct value *p;

	if (!src)
		return NULL;
	nr = src->nr;
	size = sizeof(*src->vec) * nr;

	dst = xmalloc(sizeof(*dst));
	*dst = *src;
	dst->vec = memcpy(xmalloc(size), src->vec, size);

	if (dupdesc) {
		for (p = dst->vec; nr-- ; ++p)
			p->desc = xstrdup(p->desc);
	}

	return dst;
}

struct expr *
dupexpr(struct expr *e)
{
	struct expr *new;

	if (!e)
		return NULL;

	new = xmalloc(sizeof(*new));
	new->text = (e->text) ? xstrdup(e->text) : NULL;
	new->values = dupvalues(e->values, 1);

	return new;
}


struct expr *
negate(struct expr *e)
{
	struct expr *not;

	not = xmalloc(sizeof(*not));
	not->text = xmalloc(strlen(e->text) + 4);
	not->values = dupvalues(e->values, 1);
	sprintf(not->text, "!(%s)", e->text);

	return not;
}

void
initeval(void)
{
	int tochild[2], toparent[2];
	extern int opt_debug;
	static char *cmds[] = {
		"pcc1 | pcc2",
		"tee /dev/tty | pcc1 | pcc2",
		"tee /dev/tty | pcc1 | tee /dev/tty | pcc2",
		"tee /dev/tty | pcc1 | tee /dev/tty | pcc2 | tee /dev/tty"
	};
	char *cmd = cmds[opt_debug];

	if (pipe(tochild) < 0 || pipe(toparent) < 0)
		die("error opening pipes");
	switch (child = fork()) {
	case -1:
		die("error forking");
	case 0:
		close(tochild[1]);
		close(toparent[0]);
		dup2(tochild[0], STDIN_FILENO);
		dup2(toparent[1], STDOUT_FILENO);
		system(cmd);
		exit(0);
	default:
		close(tochild[0]);
		close(toparent[1]);
		ccin = fdopen(tochild[1], "w");
		ccout = fdopen(toparent[0], "r");
		if (!ccout || !ccin)
			die("error redirecting pipes");
		setvbuf(ccout, NULL, _IOLBF, 0);
		setvbuf(ccin, NULL, _IOLBF, 0);
		signal(SIGALRM, SIG_IGN);
		break;
	}
}

void
unset(char *s)
{
	int c;

	switch (s[0]) {
	case '[':
		goto bad_value;
	case '{':
		while ((c = *++s) != '}') {
			if (c == ',')
				goto bad_value;
			putc(c, ccin);
		}
		break;
	default:
		fputs(s, ccin);
	}
	return;
bad_value:
	die("variable value not evaluable");
}

static char *
eval(char *s, struct values *vals)
{
    if (strcmp(s, "-INF") == 0 || strcmp(s, "+INF") == 0)
        die("Infinity value found. "
            "Task Dependency Graph cannot be expanded. "
            "Check Mercurium report for more information.");
	static int nfun;
	static char buf[MAXTOKEN];
	struct value *p;
	int c;
	unsigned n;
	char *cp;

	if (vals) {
		n = vals->nr;
		for (p = vals->vec; n--; ++p)
			p->defined = 0;
	}

	fprintf(ccin, "void f(void)\n{\n", ++nfun);
	for (cp = s; (c = *cp) != '\0'; ++cp) {
		if (c == '$') {
			if (!vals)
				die("Parametrized expression without values");
			n = atoi(cp + 1);
			if (n == 0 || n > vals->nr)
				die("incorrect variable number '%d'", n);
			p = &vals->vec[n-1];
			if (p->defined)
				continue;
			p->defined = 1;
			fprintf(ccin, "\t%s i%d;\n\ti%d = ",
			        p->var->type, n, n);
			unset(p->desc);
			putc(';', ccin);
			putc('\n', ccin);
		}
	}

	putc('\t', ccin);
	putc('@', ccin);
	while ((c = *s++) != '\0') {
		if (c == '$')
			c = 'i';
		putc(c, ccin);
	}
	fputs(";\n}\n", ccin);

	alarm(10);
	if (!fgets(buf, sizeof(buf), ccout))
		die("error reading of compiler");
	alarm(0);
	if ((n = strlen(buf)) > 1 && buf[n-1] == '\n')
		buf[n-1] = '\0';
	return buf;
}

int
condition(char *s, struct values *v)
{
	return !strcmp(eval(s, v), "1");
}

/*
 * Nextval generates the next value of a set or range. Vals is a pointer
 * to the values of the contex, n is the position of the value in vals,
 * curval is a string with the previous value (or NULL in the first iteration),
 * and iter is the number of iteration
 */
static char *
nextval(struct values *vals, unsigned n, char *curval, unsigned iter)
{
	static char buf[MAXTOKEN];
	char fmt[40], tmp[MAXTOKEN], *s, *t;
	char lower[MAXTOKEN], upper[MAXTOKEN], inc[MAXTOKEN];
	struct value *v = &vals->vec[n];
	extern int opt_debug;

	if (opt_debug == 3) {
		fprintf(stderr, "Nextval of %s (%s-%s)\n",
		        v->var->name,
		        (curval) ? curval : "-", v->desc);
	}
	switch (v->desc[0]) {
	case '[':
		sprintf(fmt, "[%%%d[^:]:%%%d[^:]:%%%d[^]]]",
		        MAXTOKEN, MAXTOKEN, MAXTOKEN);
		if (sscanf(v->desc, fmt, lower, upper, inc) != 3)
			goto error;
		strcpy(lower, eval(lower, vals));
		strcpy(upper, eval(upper, vals));
		strcpy(inc, eval(inc, vals));
		if (iter != 0) {
			snprintf(buf, sizeof(buf), "%s+%s", curval, inc);
			return eval(buf, vals);
		}
		if (snprintf(tmp, MAXTOKEN, "(%s) > 0", inc) >= MAXTOKEN)
			goto error;
		return strcpy(buf, (condition(tmp, vals)) ?  lower : upper);
	case '{':
		/*
		 * TODO: this is wrong because we don't know the relation
		 *       because the set of values and the iterations,
		 *       so we have to say unknown.
		 */
		for (s = &v->desc[1]; n != 0 && (t = strchr(s, ',')); s = t+1)
			--n;
		/*
		 * if n is bigger than the number of values, then we return
		 * the last
		 */
		sprintf(fmt, "%%%d[^,}]}", MAXTOKEN);
		if (sscanf(s, fmt, buf) != 1)
			goto error;
		return eval(buf, vals);
	}
error:
	die("incorrect expression in nextval '%s','%s'", v->desc, curval);
}

struct values *
genctx(struct expr *e, struct values *curvec, unsigned n)
{
	unsigned nr, i, skip;
	int c;
	char *desc, *p;
	struct value *new, *cur;
	struct values *v;

	v = dupvalues(e->values, 0);
	nr = e->values->nr;

	for (new = v->vec, i = 0; i < nr; ++new, ++i)
		new->evaluated = 0;

repeat:
	skip = 0;
	cur = (curvec) ? curvec->vec : NULL;
	for (new = v->vec, i = 0; i < nr; ++new, ++i) {
		if (cur) {
			desc = cur->desc;
			++cur;
		} else {
			desc = NULL;
		}
		for (p = new->desc; c = *p; ++p) {
			if (c == '$' && !v->vec[atoi(p+1) - 1].evaluated) {
					skip = 1;
					break;
			}
		}
		if (c == '\0' && !new->evaluated) {
			new->desc = xstrdup(nextval(v, i, desc, n));
			new->evaluated = 1;
		}
	}
	if (skip)
		goto repeat;

	return v;
}

struct values *
nextiter(struct expr *e, struct values *curvec, unsigned n)
{
	struct values *v;

	v = genctx(e, curvec, n);
	if (!condition(e->text, v)) {
		freevalues(v);
		return NULL;
	}
	return v;
}

int
evalable(struct expr *e)
{
	int c;
	char *s;
	struct value *p;
	unsigned nr;

	if (!e->values)
		return 1;
	nr = e->values->nr;
	for (p = e->values->vec; nr-- ; ++p) {
		if (*(s = p->desc) == '[')
			continue;
		if (*s != '{')
			goto noeval;
		while ((c = *++s) != '}') {
			if (c == ',')
				goto noeval;
		}
	}
	return 1;
noeval:
	fprintf(stderr, "exp: %s is not evaluable\n", e->text);
	abort();
}

void
forallvalues(struct values *v, void *par, void (*fun)(void *, void *))
{
	struct value *p;
	unsigned nr;

	nr = v->nr;
	for (p = v->vec; nr--; ++p)
		(*fun)(p, par);
}

int
definevalue(struct expr *e, struct value *v)
{
	struct value *p;
	char tmp[MAXTOKEN], *s, *t;
	unsigned nr;

	if (!e || !e->values)
		return 1;

	nr = e->values->nr;
	s = v->desc;
	for (p = e->values->vec; nr-- ; ++p) {
		if (p->var != v->var)
			continue;
		t = p->desc;
		if (*t == '{' || *t == '[')
			return 0;
		if (snprintf(tmp, MAXTOKEN, "(%s)+(%s)", s, t) >= MAXTOKEN)
			die("incorrect expression");
		free(t);
		t  = eval(tmp, e->values);
		if (snprintf(tmp, MAXTOKEN, "{%s}", t) >= MAXTOKEN)
			die("incorrect value definition");
		p->desc = xstrdup(tmp);
		return 1;
	}
	return 1;
}

int
evalcond(struct expr *e)
{
	int r;
	struct values *v;

	v = genctx(e, NULL, 0);
	r = condition(e->text, v);
	freevalues(v);
	return r;
}

static struct value *
findvalue(struct values *source, struct values *target,
          unsigned long id, int side)
{
	struct values *vals = (side > 0) ? target : source;
	unsigned long tid = TREEID(TYPVAR, 0, id);
	unsigned n;
	struct value *p;

	for (n = vals->nr, p = vals->vec; n--; ++p) {
		if (p->var->id == tid)
			return p;
	}

	die("incorrect variable in dependence exxpression");
}
	
/*
 * This function is called when we want to evaluate the expressions
 * related to dependencies. It means we are going to have
 * the condition expression of the dependence and the expressions
 * of source and target tboxes (the expression itself is not important,
 * but the values of the associated variables are).
 */
int
evalcond2(struct expr *e, struct expr *etask1, struct expr *etask2)
{
	struct values *source = etask1->values, *target = etask2->values;
	unsigned nr,  r, isnvalue = 0;
	struct value *p, *q;
	struct values *v;
	char *s, *t;

	if (!e->values) {
		v = NULL;
	} else {
		v = dupvalues(e->values, 0);
		nr = (e->values) ? e->values->nr : 0;
		for (p = v->vec; nr--; ++p) {
			q = findvalue(source, target, p->var->id, p->side);
			s = p->desc = xstrdup(q->desc);
			switch (s[0]) {
			case '[':
				isnvalue = 1;
				break;
			case '{':
				if (!strchr(s, ',') && (t = strchr(s, '}'))) {
					s[0] = ' ';
					t[0] = ' ';
				} else {
					isnvalue = 1;
				}
				break;
			default:
				isnvalue = 1;
			}
		}
	}
	r = 1;
	if (!isnvalue)
		r = condition(e->text, v);
	freevalues(v);
	return r;
}

char *
getexpr(struct expr *e)
{
	return (!e || !e->text) ? "" : e->text;
}

void
normalizedep(struct expr *e, struct expr *src, struct expr *dst)
{
	struct value *v;
	struct values **vec;
	unsigned nr;

	if (!e->values)
		return;

	nr = e->values->nr;
	for (v = e->values->vec; nr--; ++v) {
		vec = (v->side < 0) ? &src->values : &dst->values;
		*vec = newvalue(*vec, v->var, v->desc, 0);
	}
}
