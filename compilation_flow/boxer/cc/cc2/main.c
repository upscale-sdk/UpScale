
#include <stdio.h>
#include <stdlib.h>

#include "cc2.h"

Var vars[VARSIZ];

extern void pushvar(void), add(void), sub(void), mul(void),
	xdiv(void), print(void), pushconst(void), assign(void),
	cast(void), rshift(void), lshift(void), neg(void), cpl(void),
	mod(void), gt(void), ge(void), lt(void), le(void), eq(void),
	ne(void), land(void), lor(void), lxor(void), assignop(void),
	ternary(void), postassign(void), and(void), or(void);

void
error(char *err)
{
	fprintf(stderr, "cc2: %s\n", err);
	exit(EXIT_FAILURE);
}

char
type(void)
{
	char c;

	switch (c = getchar()) {
	case 'B': case 'M': case 'C': case 'E': case 'K': case 'N': case 'D':
	case 'I': case 'L': case 'Z': case 'O': case 'J': case 'F': case 'H':
		return c;
	default:
		error("incorrect type");
	}
}

void decl(void), stmt(void);

void
function(unsigned id)
{
	int c;

	initcode();

	if (getchar() != '{' || getchar() != '\n')
		error("incorrect intermediate input");
	while ((c = getchar()) != '}') {
		ungetc(c, stdin);
		if (c == '\t')
			stmt();
		else
			decl();
	}
	if (getchar() != '\n')
		error("incorrect intermediate input");
	fflush(stdout);
}

void
decl(void)
{
	unsigned id, c, isfun;
	char name[31];

	getchar(); /* skip class storage */
	scanf("%u", &id);

	id &= VARSIZ-1;
	vars[id].defined = 1;

	getchar(); /* skip tab */
	isfun = type() == 'F';

	if ((c = getchar()) == '\t') {
		scanf("%30[^\t\n]", name);
		c = getchar();
	}
	if (c == '\n')
		return;

	if (isfun) {
		function(id);
		return;
	}

	error("incorrect intermediate input");
}

void
inmediate(void)
{
	int c;
	Datum d;

	switch (c = getchar()) {
	case 'I':
		ungetc((c = getchar()), stdin);
		if (c == '\t')
			d.u.i = sizeof(int);
		else
			scanf("%x", &d.u.i);
		break;
	case 'F':
		ungetc((c = getchar()), stdin);
		if (c == '\t')
			d.u.i = sizeof(float);
		else
			scanf("%f", &d.u.f);
		break;
	default:
		ungetc(c, stdin);
		scanf("%x", &d.u.i);
		break;
	}
	code(pushconst);
	data(d);
}

void
colon(void)
{
	Datum d;
	void (*fun)(void);
	int c = getchar();

	switch (c) {
	case '+': fun = add; break;
	case '-': fun = sub; break;
	case '*': fun = mul; break;
	case '/': fun = xdiv; break;
	case 'l': fun = lshift; break;
	case 'r': fun = rshift; break;
	case '_': fun = neg; break;
	case '~': fun = cpl; break;
	case '%': fun = mod; break;
	case '&': fun = land; break;
	case '|': fun = lor; break;
	case '^': fun = lxor; break;
	case 'B': case 'M': case 'C': case 'E': case 'K': case 'N': case 'D':
	case 'I': case 'L': case 'Z': case 'O': case 'J': case 'Q': case 'H':
		ungetc(c, stdin);
		d.u.c = type();
		code(assign);
		data(d);
		return;
	default:
		error("incorrect intermediate code");
	}
	d.u.c = type();
	code(assignop);
	code(fun);
	data(d);
	data(d);
	return;
}

void
semicolon(void)
{
	Datum d;
	void (*fun)(void);
	int c = getchar();

	switch (c) {
	case '+': fun = add; break;
	case '-': fun = sub; break;
	default:
		error("incorrect intermediate code");
	}
	d.u.c = type();
	code(postassign);
	code(fun);
	data(d);
	data(d);
	return;
}
void
stmt(void)
{
	int c;
	unsigned id;
	Datum d;
	Inst *p;
	void (*fun)(void);
	extern Inst *codep;

	p = codep;
	while ((c = getchar()) == '\t') {
		switch (c = getchar()) {
		case 'A': case 'T': case 'R': case 'G':
			scanf("%u", &id);
			id &= VARSIZ-1;
			code(pushvar);
			variable(&vars[id]);
			continue;
		case 'B': case 'M': case 'C': case 'E': case 'K':
		case 'N': case 'D': case 'I': case 'L': case 'Z':
		case 'O': case 'J': case 'Q': case 'H':
			code(cast);
			ungetc(c, stdin);
			d.u.c = type();
			data(d);
			d.u.c = type();
			data(d);
			continue;
		case '#': inmediate(); continue;
		case ';': semicolon(); continue;
		case ':': colon(); continue;
		case '?': fun = ternary; break;
		case '+': fun = add; break;
		case '-': fun = sub; break;
		case '*': fun = mul; break;
		case '/': fun = xdiv; break;
		case 'l': fun = lshift; break;
		case 'r': fun = rshift; break;
		case '_': fun = neg; break;
		case '~': fun = cpl; break;
		case '%': fun = mod; break;
		case '>': fun = gt; break;
		case ']': fun = ge; break;
		case '<': fun = lt; break;
		case '[': fun = le; break;
		case '=': fun = eq; break;
		case '!': fun = ne; break;
		case '&': fun = land; break;
		case '|': fun = lor; break;
		case '^': fun = lxor; break;
		case 'k': fun = print; break;
		case 'y': fun = and; break;
		case 'o': fun = or; break;
		default:
			error("incorrect statement");
		}
		d.u.c = type();
		code(fun);
		data(d);
	}
	code(STOP);
	if (c != '\n')
		error("incorrect intermediate input");
	execute(p);
}

int
main(void)
{
	int c;

	setbuf(stdin, NULL);
	setbuf(stdout, NULL);
	while (ungetc(c = getchar(), stdin) != EOF) {
		switch (c) {
		case 'G': case 'Y':
			decl();
			break;
		case '\t':
			initcode();
			stmt();
			break;
		}
	}
	return 0;
}

