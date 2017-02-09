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

#include <ctype.h>
#include <errno.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "boxer.h"


struct str2int {
	int token;
	char *s;
};

enum {
	FUNCTION = 1, LOCUS, TDG_ID, VARIABLES, ID, NAME, TYPE, NUM_TDGS, TDG,
	NODE, NODES, CONTROL, WHEN, VALUES, EXPR, VARS,
	TRUE, FALSE, NIL, NUMBER, FRAC, EXP, STRING, DEPS,
	SOURCE,  TARGET, DEFCONTROL, CLOOP, CSWITCH,
	NBRANCHES, CIFELSE, CIMPLICIT, CONTROL_ID, BRANCH_ID,
	SIDE, SIZE
};

static char yytext[MAXTOKEN];
static int yytoken, safe;
static FILE *yyinput;
static unsigned linenum;
static char *filenam;
static jmp_buf recover;
int ntdgs;
static struct box *root;
static char function[MAXTOKEN];

static void
error(char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	fputs("boxer:", stderr);
	fprintf(stderr, "%s:%d: ", filenam, linenum);
	vfprintf(stderr, fmt, va);
	putc('\n', stderr);
	va_end(va);
	longjmp(recover, 1);
}

static int
symbol(void)
{
	char *bp;
	int c;

	for (bp = yytext; isalpha(c = getc(yyinput)); *bp++ = c) {
		if (bp == &yytext[MAXTOKEN-1])
			error("symbol too much long");
	}
	ungetc(c, yyinput);
	*bp = '\0';
	if (!strcmp(yytext, "true"))
		return TRUE;
	if (!strcmp(yytext, "false"))
		return FALSE;
	if (!strcmp(yytext, "null"))
		return NIL;
	error("unknown symbol '%s'", yytext);
}

static int
number(void)
{
	int c, status;
	char *bp;

	status = NUMBER;
	for (bp = yytext; bp < &yytext[MAXTOKEN-1]; *bp++ = c) {
		c = getc(yyinput);
		switch (status) {
		case NUMBER:
			if (c == '.')
				status = FRAC;
			else if (c == 'e' || c == 'E')
				status = EXP;
			else if (!isdigit(c))
				goto end_num;
			continue;
		case FRAC:
			if (!isdigit(c))
				goto end_num;
			continue;
		case EXP:
			if (c == '-' || c == '+' || isdigit(c))
				status = FRAC;
			continue;
		}
	}

end_num: ungetc(c, yyinput);
	*bp = '\0';
	return NUMBER;		
}

static int
string(void)
{
	int c;
	char *bp;	
	
	for (bp = yytext; (c = getc(yyinput)) != '"'; *bp++ = c) {
		if (c == '\\') {
			switch(c = getc(yyinput)) {
			case 'n':
				c = '\n';
				break;
			case 't':
				c = '\t';
				break;
			case 'b':
				c = '\b';
				break;
			case 'f':
				c = '\f';
				break;
			case 'r':
				c = '\r';
				break;
			case 'u':
				/* TODO: unicode characters */;
			}
		} else if (c == EOF) {
			break;
		}
		if (bp == &yytext[MAXTOKEN-1])
			error("token too long");
	}
	*bp = '\0';
	return STRING;
}
								
static int
next(void)
{
	int c;

	while (isspace(c = getc(yyinput))) {
		if (c == '\n')
			++linenum;
	}

	switch (c) {
	case EOF:
		if (!safe)
			error("found end of file while parsing");
	case '}': case '{': case '[': case ']': case ':': case ',':
		yytext[0] = yytoken = c;
		yytext[1] = '\0';
		break;
	case '"':
		yytoken = string();
		break;
	default:
		ungetc(c, yyinput);
		if (isalpha(c))
			yytoken = symbol();
		else if (isdigit(c))
			yytoken = number();
		else
			error("invalid character '%c'", c);
	}
	return yytoken;
}

static void
expect(int tok)
{
	if (yytoken != tok)
		error("unexpected '%s'", yytext);
	next();
}

static int
accept(int tok)
{
	if (yytoken == tok) {
		next();
		return 1;
	}
	return 0;
}

static char *
fieldstr(void)
{
	static char buf[MAXTOKEN];

	if (yytoken != STRING)
		error("unexpected '%s'", yytext);
	/*
	 * copy yytext to temporal buffer in order to avoid a
	 * memory leak if threre is some error calling next()
	 */
	strcpy(buf, yytext);
	next();
	return buf;
}

static double
fieldnum(void)
{
	double v;

	if (yytoken != NUMBER)
		error("unexpected '%s'", yytext);
	sscanf(yytext, "%lf", &v);
	next();
	return v;
}

static int
str2int(struct str2int *bp)
{
	if (yytoken != STRING)
		error( "expected a string instead of '%s'", yytext);
	for (; bp->token && strcmp(bp->s, yytext); ++bp)
		/* nothing */;
	if (!bp->token)
		error("unexpected string '%s'", yytext);

	next();
	return bp->token;
}

static int
fieldname(struct str2int *bp)
{
	int token = str2int(bp);
	expect(':');
	return token;
}

static struct box *
control(struct box *inner, struct task *task)
{
	unsigned long control, branch, id;
	struct box *outer;
	static struct str2int fields[] = {
		CONTROL_ID, "control_id",
		BRANCH_ID, "branch_id",
		0, NULL
	};

	control = branch = 0;
	expect('{');
	do {
		switch (fieldname(fields)) {
		case CONTROL_ID:
			if (control)
				goto duplicated;
			control = fieldnum();
			id = TREEID(TYPBOX, 0, control);
			if ((outer = lookup(id)) == NULL) {
				error("incorrect id '%ld' of control structure",
				      control);
			}
			break;
		case BRANCH_ID:
			if (branch++)
				goto duplicated;
			if (!control)
				error("branch id found before control id");
			if (!inner)
				error("branch id in inner box");
			expect('[');
			do {
				struct box *box;
				id = fieldnum();
				if ((box = getnchild(outer, id)) == NULL)
					error("incorrect branch id '%ld'", id);
				addchild(box, inner);
			} while (accept(','));
			expect(']');
			break;
		}
	} while (accept(','));
	expect('}');

	if (!inner) {
		addtask(outer, task);
		return outer;
	} else if (!branch) {
		addchild(getnchild(outer, 1), inner);
		return outer;
	} else {
		return outer;
	}

duplicated:
	error("duplicated field in expression variable definition", yytext);
}

static void
controllist(struct task *task)
{
	struct box *box = NULL;

	expect('[');
	if (accept(']'))
		return;
	do box = control(box, task); while (accept(','));

	addchild(root, box);
	expect(']');

	return;
}

static void
node(void)
{
	unsigned long id;
	int type, control;
	char locus[MAXTOKEN];
	static struct str2int fields[] = {
		ID, "id",
		LOCUS, "locus",
		TYPE, "type",
		CONTROL, "control",
		0, NULL
	};
	static struct str2int types[] = {
		NTASK, "Task",
		NWAIT, "Taskwait",
		NBARRIER, "Barrier",
		0, NULL
	};

	id = type = control = 0;
	*locus = '\0';

	expect('{');
	do {
		switch (fieldname(fields)) {
		case ID:
			if (id)
				goto duplicated;
			id = fieldnum();
			break;
		case LOCUS:
			if (*locus)
				goto duplicated;
			strcpy(locus, fieldstr());
			break;
		case TYPE:
			if (type)
				goto duplicated;
			type = str2int(types);
			break;
		case CONTROL:
			if (!id || !type || !locus[0])
				error("incorrect definiton of node");
			control = 1;
			controllist(newtask(id, type, locus));
			break;
		}
	} while (accept(','));

	expect('}');
	if (!control)
		error("node definition without control structure");
	return;

duplicated:
	error("duplicated field in node definition");
}

static void
genvar(void)
{
	int id;
	char name[MAXTOKEN], type[MAXTOKEN], locus[MAXTOKEN];
	static struct str2int fields[] = {
		ID, "id",
		NAME, "name",
		LOCUS, "locus",
		TYPE, "type",
		0, NULL
	};
	expect('{');

	id = 0;
	*name = *type = *locus = '\0';

	do {
		switch (fieldname(fields)) {
		case ID:
			if (id++)
				goto duplicated;
			id = fieldnum();
			break;
		case NAME:
			if (*name)
				goto duplicated;
			strcpy(name, fieldstr());
			break;
		case LOCUS:
			if (*locus)
				goto duplicated;
			strcpy(locus, fieldstr());
			break;
		case TYPE:
			if (*type)
				goto duplicated;
			strcpy(type, fieldstr());
			break;
		}
	} while (accept(','));

	if (!id || !*name || !*type || !*locus)
		error("missed some field in variables definition");
	newvar(id, name, locus, type);
	expect('}');
	return;

duplicated:
	error("duplicated field in a variable definition");
}

static void
varlist(void)
{
	expect('[');
	if (accept(']'))
		return;

	do
		genvar();
	while (accept(','));
		
	expect(']');
}

static void
nodelist(void)
{
	expect('[');
	if (accept(']'))
		return;

	do node(); while (accept(','));

	expect(']');
}

static struct values *
value(struct values *p, int isdep)
{
	unsigned long id;
	int side;
	struct var *var;
	char desc[MAXTOKEN], *s;
	static struct str2int fields[] = {
		ID,"id",
		VALUES,"values",
		SIDE, "side",
		0, NULL
	};
	side = 0;
	var = NULL;
	desc[0] = '\0';
	expect('{');

	do {
		switch (fieldname(fields)) {
		case ID:
			if (var)
				goto duplicated;
			id = fieldnum();
			if ((var = lookup(TREEID(TYPVAR, 0, id))) == NULL)
				error("incorrect variable in expression");
			break;
		case SIDE:
			if (!isdep)
				error("side attribute in no dependence expression");
			if (side)
				goto duplicated;
			s = fieldstr();
			if (!strcmp(s, "source"))
				side = -1;
			else if (!strcmp(s, "target"))
				side = 1;
			else
				error("incorrect side value '%s'", s);
			break;
		case VALUES:
			if (desc[0])
				goto duplicated;
			strcpy(desc, fieldstr());
			break;
		}
	} while (accept(','));

	expect('}');

	if (isdep && !side)
		error("variable in dependence without side attribute");
	if (!var || !desc[0])
		error("incorrect expression variable");
	return newvalue(p, var, desc, side);

duplicated:
	error("duplicated field in expression variable definition");
}

static struct values *
valuelist(int isdep)
{
	struct values *p = NULL;

	expect('[');
	if (accept(']'))
		return NULL;;

	do p = value(p, isdep); while (accept(','));

	expect(']');
	return p;
}

static char *
expr(void)
{
	switch (yytoken) {
	case STRING:
		return fieldstr();
	case TRUE:
		next();
		return "1";
	default:
		error("incorrect expression '%s' in condition", yytext);
	}
}

static struct expr *
when(int isdep)
{
	struct values *v;
	char e[MAXTOKEN];
	static struct str2int fields[] = {
		EXPR, "expression",
		VARS, "vars",
		0, NULL
	};
	e[0] = '\0';
	v = NULL;

	expect('{');
	do {
		switch (fieldname(fields)) {
		case EXPR:
			if (*e)
				goto duplicated;
			strcpy(e, expr());
			break;
		case VARS:
			if (v)
				goto duplicated;
			v = valuelist(isdep);
			break;
		}
	} while (accept(','));
	expect('}');

	if (!e[0])
		error("incorrect expression definition");
	return newexpr(e, v);

duplicated:
	error("duplicated field in when definition");
}

static void
dep(void)
{
	int s, t;
	struct expr *e;
	static struct str2int fields [] = {
		SOURCE, "source",
		TARGET, "target",
		WHEN, "when",
		SIZE, "size",
		0, NULL
	};

	s = t = 0;
	e = NULL;
	expect('{');
	do {
		switch (fieldname(fields)) {
		case SOURCE:
			if (s)
				goto duplicated;
			s = fieldnum();
			s = TREEID(TYPTASK, 0, s);
			break;
		case TARGET:
			if (t)
				goto duplicated;
			t = fieldnum();
			t = TREEID(TYPTASK, 0, t);
			break;
		case SIZE:
			fieldstr();
			break;
		case WHEN:
			if (e)
				goto duplicated;
			e = when(1);
			break;
		}
	} while (accept(','));
	expect('}');

	newdep(e, s, t);

	return;

duplicated:
	error("duplicated field in dependencie definition");
}

static void
deplist(void)
{
	expect('[');

	if (accept(']'))
		return;

	do dep(); while (accept(','));

	expect(']');
}

static void
gencstruct(unsigned  long id,
           char *locus, int type, int nbranches,
           struct expr *e)
{
	struct box *box;

	if (!id || !type || type != CIMPLICIT && !*locus)
		error("missed some fields of control structure definition");

	/*
	 * TODO: check correctness of e in the boxes that require it
	*/
	switch (type) {
	case CLOOP:
		if (nbranches)
			goto badnbranches;
		box = newbox(id, LBOX, locus, e);
		addchild(box, newbox(0, SBOX, "", NULL));
		break;
	case CSWITCH:
		break;
	case CIFELSE:
		switch (nbranches) {
		case 1: case 2:
			box = newbox(id, EBOX, locus, NULL);
			/* TODO: Update correctly the expression */
			while (nbranches--) {
				addchild(box, newbox(newid(), CBOX, locus, e));
				e = negate(e);
			}
			break;
		default:
			goto badnbranches;
		}
		break;
	case CIMPLICIT:
		if (nbranches)
			goto badnbranches;
		if (*locus)
			error("locus field in an Implicit control structure");
		box = newbox(id, TBOX, "", e);
		break;
	}
	return;

badnbranches:
	error("incorrect nbranches value in control definition");
}

static void
cstruct(void)
{
	int id, type, w, nbranches;
	struct expr *e;
	char locus[MAXTOKEN];
	static struct str2int fields[] = {
		ID, "id",
		TYPE, "type",
		LOCUS, "locus",
		WHEN, "when",
		NBRANCHES, "nbranches",
		0, NULL
	};
	static struct str2int types[] = {
		CLOOP, "Loop",
		CSWITCH, "Switch",
		CIFELSE, "IfElse",
		CIMPLICIT, "Implicit",
		0, NULL
	};

	nbranches = id = type = 0;
	e = NULL;
	*locus = '\0';

	expect('{');

	do {
		switch (fieldname(fields)) {
		case ID:
			if (id)
				goto duplicated;
			id = fieldnum();
			break;
		case TYPE:
			if (type)
				goto duplicated;
			type = str2int(types);
			break;
		case LOCUS:
			if (*locus)
				goto duplicated;
			strcpy(locus, fieldstr());
			break;
		case WHEN:
			if (e)
				goto duplicated;
			e = when(0);
			break;
		case NBRANCHES:
			if (nbranches)
				goto duplicated;
			nbranches = fieldnum();
			break;
		}
	} while (accept(','));

	expect('}');
	gencstruct(id, locus, type, nbranches, e);
	return;

duplicated:
	error("duplicated field in contol structure definition");
}

static void
cstructlist(void)
{
	expect('[');
	if (accept(']'))
		return;

	do cstruct(); while (accept(','));

	expect(']');
}

static void
tdgbody(void)
{
	int tdg_id, vars, nodes, deps, control;
	static char locus[MAXTOKEN];
	static struct str2int fields[] = {
		FUNCTION, "function",
		LOCUS, "locus",
        TDG_ID, "tdg_id",
		DEPS, "dependencies",
		VARIABLES, "variables",
		DEFCONTROL, "control_structures",
		NODES, "nodes",
		0, NULL
	};

	tdg_id = vars = deps = nodes = control = 0;
	*function = *locus = '\0';

	expect('{');
	do {
		switch (fieldname(fields)) {
		case FUNCTION:
			if (*function)
				goto duplicated;
			strcpy(function, fieldstr());
			break;
		case LOCUS:
			if (*locus)
				goto duplicated;
			strcpy(locus, fieldstr());
			break;
                case TDG_ID:
                        if (tdg_id)
                                goto duplicated;
                        tdg_id = fieldnum();
			break;
		case VARIABLES:
			if (vars++)
				goto duplicated;
			varlist();
			break;
		case DEFCONTROL:
			if (control++)
				goto duplicated;
			cstructlist();
			break;
		case NODES:
			if (nodes++)
				goto duplicated;
			if (!*locus || !control)
				error("nodes before locus or control");
			root = newbox(0, SBOX, locus, NULL);
			nodelist();
			break;
		case DEPS:
			if (deps++)
				goto duplicated;
			deplist();
			break;
		}
	} while (accept(','));

	expect('}');
	return;

duplicated:
	error("duplicated field in tdg values");
}

static void
init_tdgs(void)
{
        static struct str2int fields[] = {
                TDG, "tdgs",
                0, NULL
        };

        if (fieldname(fields) != TDG)
                error("unknown field in tdg description");

        expect('[');
        if (accept(']'))
        {
            if (ntdgs != 0)
            {
                error("Missing tdg definition");
            }
            return;
        }
        
        return;
}

static void
finish_tdgs()
{
        /*
            * yytoken already have the value of the next token, that should be
            * '}', and if it is the last token of the file, the call to expect
            * will read EOF, and in this case we are in a safe position, out
            * of any tdg definition.
            */
        safe = 1;
        expect('}');
}

static int
num_tdgs(void)
{
        static struct str2int fields[] = {
                NUM_TDGS, "num_tdgs",
                0, NULL
        };

        safe = 0;
        /*
            * yytoken already have the value of the next token, that should be
            * '{', and the call to expect will read the next token, and if
            * it is EOF we are in an unsafe position, in a tdg definition
            */
        expect('{');
        if (fieldname(fields) != NUM_TDGS)
                error("unknown field in tdg description");

        int n_tdgs = fieldnum();
        expect(',');

        return n_tdgs;
}

struct box*
readtdgjson(char *funcname)
{
        tdgbody();
        strcpy (funcname, function);
        accept(',');    // if more tdgs

        if (root) {
            delempty(root);
            collapsebox(root);
            enumerate(root);
        }

        if (accept(']')) {    // if last tdg
            finish_tdgs();
            if (yyinput)
                fclose(yyinput);
            yyinput = NULL;
        }

        return root;
}

int
initjsonfile(char *name)
{
	if (setjmp(recover)) {
		root = NULL;
		goto close_file;
	}
	if (!name) {
		filenam = "<stdin>";
		yyinput = stdin;
	} else {
		filenam = name;
		if ((yyinput = fopen(name, "r")) == NULL) {
			die("boxer: Error opening input file",
			    strerror(errno));
		}
	}
	linenum = 1;
	safe = 1;
	next();

        // Get the number of TDGs
        ntdgs = num_tdgs();

        // Prepare for reading the first tdg
        init_tdgs();

        return ntdgs;

close_file:
	if (yyinput)
		fclose(yyinput);
	yyinput = NULL;
	return -1;
}
