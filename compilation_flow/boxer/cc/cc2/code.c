
#include <stdio.h>

#include "cc2.h"

#define CONST 0
#define VAR 1
 
Datum stack[STACKSIZ];
Inst memory[MEMSIZ];

Inst *codep, *pc;
Datum *stackp;

void
initcode(void)
{
	codep = memory;
	stackp = stack;
}

Inst *
install(Inst i)
{
	Inst *p = codep;

	if (codep == &memory[MEMSIZ])
		error("out of memory");
	*codep++ = i;
	return p;
}

void
execute(Inst *p)
{
	pc = p;
	while (pc->f != STOP)
		(*(pc++)->f)();
}

static void
push(Datum d)
{
	if (stackp == &stack[STACKSIZ])
		error("stack overflow");
	*stackp++ = d;
}

static Datum
pop(void)
{
	if (stackp == stack)
		error("stack underflow");
	return *--stackp;
}

static Datum
eval(void)
{
	Datum d;

	d = pop();
	if (d.type == VAR)
		d = d.u.v->val;
	return d;
}

void
pushconst(void)
{
	Datum d;

	d = (pc++)->d;
	d.type = CONST;
	push(d);
}

void
pushvar(void)
{
	Datum d;

	d.type = VAR;
	d.u.v = (pc++)->v;
	if (!d.u.v->defined)
		error("variable not defined");
	push(d);
}

void
assign(void)
{
	Datum d1, d2;

	d1 = eval();
	d2 = pop();
	if (d2.type != VAR)
		error("assignment to constant");
	/* discard the type */
	pc++;
	d2.u.v->val = d1;
	push(d2);
}

void
or(void)
{
	Datum d1, d2;

	d1 = eval();
	d2 = eval();
	++pc;
	d1.u.i = d1.u.i || d2.u.i;
	d1.type = CONST;
	push(d1);
}

void
and(void)
{
	Datum d1, d2;

	d1 = eval();
	d2 = eval();
	++pc;
	d1.u.i = d1.u.i && d2.u.i;
	d1.type = CONST;
	push(d1);
}

void
add(void)
{
	Datum d1, d2;

	d1 = eval();
	d2 = eval();
	switch ((pc++)->d.u.c) {
	case 'I': d1.u.i += d2.u.i; break;
	case 'N': d1.u.u += d2.u.u; break;
	case 'L': d1.u.l += d2.u.l; break;
	case 'Z': d1.u.ul += d2.u.ul; break;
	case 'O': d1.u.ull += d2.u.ull; break;
	case 'Q': d1.u.ll += d2.u.ll; break;
	case 'J': d1.u.f += d2.u.f; break;
	case 'D': d1.u.d += d2.u.d; break;
	case 'H': d1.u.ld += d2.u.ld; break;
	default: error("incorrect type in add");
	}
	d1.type = CONST;
	push(d1);
}

void
assignop(void)
{
	Datum d1, d2;
	void (*fun)(void);

	fun = (pc++)->f;
	d1 = eval();
	d2 = pop();

	push(d1);
	push(d2);
	(*fun)();
	d1 = pop();
	push(d2);
	push(d1);
	assign();
}

void
postassign(void)
{
	Datum d1, d2, d3;
	void (*fun)(void);

	fun = (pc++)->f;
	d1 = eval();
	d2 = pop();
	d3 = d2.u.v->val;


	push(d1);
	push(d2);
	(*fun)();
	d1 = pop();
	push(d2);
	push(d1);
	assign();

	d1 = pop();
	push(d3);
}

void
ternary(void)
{
	Datum cond, yes, no, res;

	no = eval();
	yes = eval();
	cond = eval();

	switch ((pc++)->d.u.c) {
	case 'I': res.u.i = cond.u.i ? yes.u.i : no.u.i; break;
	case 'N': res.u.u = cond.u.u ? yes.u.u : no.u.u; break;
	case 'L': res.u.l = cond.u.l ? yes.u.l : no.u.l; break;
	case 'Z': res.u.ul = cond.u.ul ? yes.u.ul : no.u.ul; break;
	case 'O': res.u.ull = cond.u.ull ? yes.u.ull : no.u.ull; break;
	case 'Q': res.u.ll = cond.u.ll ? yes.u.ll : no.u.ll; break;
	case 'J': res.u.f = cond.u.f ? yes.u.f : no.u.f; break;
	case 'D': res.u.d = cond.u.d ? yes.u.d : no.u.d; break;
	case 'H': res.u.ld = cond.u.ld ? yes.u.ld : no.u.ld; break;
	default: error("incorrect type in ternary");
	}
	res.type = CONST;
	push(res);
}

void
sub(void)
{
	Datum d1, d2;

	d1 = eval();
	d2 = eval();
	switch ((pc++)->d.u.c) {
	case 'I': d2.u.i -= d1.u.i; break;
	case 'N': d2.u.u -= d1.u.u; break;
	case 'L': d2.u.l -= d1.u.l; break;
	case 'Z': d2.u.ul -= d1.u.ul; break;
	case 'O': d2.u.ull -= d1.u.ull; break;
	case 'Q': d2.u.ll -= d1.u.ll; break;
	case 'J': d2.u.f -= d1.u.f; break;
	case 'D': d2.u.d -= d1.u.d; break;
	case 'H': d2.u.ld -= d1.u.ld; break;
	default: error("incorrect type in sub");
	}
	d2.type = CONST;
	push(d2);
}

void
mul(void)
{
	Datum d1, d2;

	d1 = eval();
	d2 = eval();
	switch ((pc++)->d.u.c) {
	case 'I': d1.u.i *= d2.u.i; break;
	case 'N': d1.u.u *= d2.u.u; break;
	case 'L': d1.u.l *= d2.u.l; break;
	case 'Z': d1.u.ul *= d2.u.ul; break;
	case 'O': d1.u.ull *= d2.u.ull; break;
	case 'Q': d1.u.ll *= d2.u.ll; break;
	case 'J': d1.u.f *= d2.u.f; break;
	case 'D': d1.u.d *= d2.u.d; break;
	case 'H': d1.u.ld *= d2.u.ld; break;
	default: error("incorrect type in mul");
	}
	d1.type = CONST;
	push(d1);
}

void
xdiv(void)
{
	Datum d1, d2;

	/* TODO: check division by zero */
	d1 = eval();
	d2 = eval();
	switch ((pc++)->d.u.c) {
	case 'I': d2.u.i /= d1.u.i; break;
	case 'N': d2.u.u /= d1.u.u; break;
	case 'L': d2.u.l /= d1.u.l; break;
	case 'Z': d2.u.ul /= d1.u.ul; break;
	case 'O': d2.u.ull /= d1.u.ull; break;
	case 'Q': d2.u.ll /= d1.u.ll; break;
	case 'J': d2.u.f /= d1.u.f; break;
	case 'D': d2.u.d /= d1.u.d; break;
	case 'H': d2.u.ld /= d1.u.ld; break;
	default: error("incorrect type in xdiv");
	}
	d2.type = CONST;
	push(d2);
}

void
lshift(void)
{
	Datum d1, d2;

	d2 = eval();
	d1 = eval();
	switch ((pc++)->d.u.c) {
	case 'I': d2.u.i <<= d1.u.i; break;
	case 'N': d2.u.u <<= d1.u.u; break;
	case 'L': d2.u.l <<= d1.u.l; break;
	case 'Z': d2.u.ul <<= d1.u.ul; break;
	case 'O': d2.u.ull <<= d1.u.ull; break;
	case 'Q': d2.u.ll <<= d1.u.ll; break;
	default: error("incorrect type in lshift");
	}
	d2.type = CONST;
	push(d2);
}

void
rshift(void)
{
	Datum d1, d2;

	d2 = eval();
	d1 = eval();
	switch ((pc++)->d.u.c) {
	case 'I': d2.u.i >>= d1.u.i; break;
	case 'N': d2.u.u >>= d1.u.u; break;
	case 'L': d2.u.l >>= d1.u.l; break;
	case 'Z': d2.u.ul >>= d1.u.ul; break;
	case 'O': d2.u.ull >>= d1.u.ull; break;
	case 'Q': d2.u.ll >>= d1.u.ll; break;
	default: error("incorrect type in rshift");
	}
	d2.type = CONST;
	push(d2);
}

void
neg(void)
{
	Datum d;

	d = eval();
	switch ((pc++)->d.u.c) {
	case 'I': d.u.i = -d.u.i; break;
	case 'N': d.u.u = -d.u.u; break;
	case 'L': d.u.l = -d.u.l; break;
	case 'Z': d.u.ul = -d.u.ul; break;
	case 'O': d.u.ull = -d.u.ull; break;
	case 'Q': d.u.ll = -d.u.ll; break;
	case 'J': d.u.f = -d.u.f; break;
	case 'D': d.u.d = -d.u.d; break;
	case 'H': d.u.ld = -d.u.ld; break;
	default: error("incorrect type in neg");
	}
	d.type = CONST;
	push(d);
}

void
cpl(void)
{
	Datum d;

	d = eval();
	switch ((pc++)->d.u.c) {
	case 'I': d.u.i = ~d.u.i; break;
	case 'N': d.u.u = ~d.u.u; break;
	case 'L': d.u.l = ~d.u.l; break;
	case 'Z': d.u.ul = ~d.u.ul; break;
	case 'O': d.u.ull = ~d.u.ull; break;
	case 'Q': d.u.ll = ~d.u.ll; break;
	default: error("incorrect type in neg");
	}
	d.type = CONST;
	push(d);
}

void
mod(void)
{
	Datum d1, d2;

	/* TODO: check division by zero */
	d2 = eval();
	d1 = eval();
	switch ((pc++)->d.u.c) {
	case 'I': d2.u.i %= d1.u.i; break;
	case 'N': d2.u.u %= d1.u.u; break;
	case 'L': d2.u.l %= d1.u.l; break;
	case 'Z': d2.u.ul %= d1.u.ul; break;
	case 'O': d2.u.ull %= d1.u.ull; break;
	case 'Q': d2.u.ll %= d1.u.ll; break;
	default: error("incorrect type in mod");
	}
	d2.type = CONST;
	push(d2);
}

void
gt(void)
{
	Datum d1, d2;

	d1 = eval();
	d2 = eval();
	switch ((pc++)->d.u.c) {
	case 'I': d2.u.i = d2.u.i > d1.u.i; break;
	case 'N': d2.u.u = d2.u.u > d1.u.u; break;
	case 'L': d2.u.l = d2.u.l > d1.u.l; break;
	case 'Z': d2.u.ul = d2.u.ul > d1.u.ul; break;
	case 'O': d2.u.ull = d2.u.ull > d1.u.ull; break;
	case 'Q': d2.u.ll = d2.u.ll > d1.u.ll; break;
	case 'J': d2.u.f = d2.u.f > d1.u.f; break;
	case 'D': d2.u.d = d2.u.d > d1.u.d; break;
	case 'H': d2.u.ld = d2.u.ld > d1.u.ld; break;
	default: error("incorrect type in gt");
	}
	d2.type = CONST;
	push(d2);
}

void
ge(void)
{
	Datum d1, d2;

	d1 = eval();
	d2 = eval();
	switch ((pc++)->d.u.c) {
	case 'I': d2.u.i = d2.u.i >= d1.u.i; break;
	case 'N': d2.u.u = d2.u.u >= d1.u.u; break;
	case 'L': d2.u.l = d2.u.l >= d1.u.l; break;
	case 'Z': d2.u.ul = d2.u.ul >= d1.u.ul; break;
	case 'O': d2.u.ull = d2.u.ull >= d1.u.ull; break;
	case 'Q': d2.u.ll = d2.u.ll >= d1.u.ll; break;
	case 'J': d2.u.f = d2.u.f >= d1.u.f; break;
	case 'D': d2.u.d = d2.u.d >= d1.u.d; break;
	case 'H': d2.u.ld = d2.u.ld >= d1.u.ld; break;
	default: error("incorrect type in ge");
	}
	d2.type = CONST;
	push(d2);
}

void
lt(void)
{
	Datum d1, d2;

	d1 = eval();
	d2 = eval();
	switch ((pc++)->d.u.c) {
	case 'I': d2.u.i = d2.u.i < d1.u.i; break;
	case 'N': d2.u.u = d2.u.u < d1.u.u; break;
	case 'L': d2.u.l = d2.u.l < d1.u.l; break;
	case 'Z': d2.u.ul = d2.u.ul < d1.u.ul; break;
	case 'O': d2.u.ull = d2.u.ull < d1.u.ull; break;
	case 'Q': d2.u.ll = d2.u.ll < d1.u.ll; break;
	case 'J': d2.u.f = d2.u.f < d1.u.f; break;
	case 'D': d2.u.d = d2.u.d < d1.u.d; break;
	case 'H': d2.u.ld = d2.u.ld < d1.u.ld; break;
	default: error("incorrect type in lt");
	}
	d2.type = CONST;
	push(d2);
}

void
le(void)
{
	Datum d1, d2;

	d1 = eval();
	d2 = eval();
	switch ((pc++)->d.u.c) {
	case 'I': d2.u.i = d2.u.i <= d1.u.i; break;
	case 'N': d2.u.u = d2.u.u <= d1.u.u; break;
	case 'L': d2.u.l = d2.u.l <= d1.u.l; break;
	case 'Z': d2.u.ul = d2.u.ul <= d1.u.ul; break;
	case 'O': d2.u.ull = d2.u.ull <= d1.u.ull; break;
	case 'Q': d2.u.ll = d2.u.ll <= d1.u.ll; break;
	case 'J': d2.u.f = d2.u.f <= d1.u.f; break;
	case 'D': d2.u.d = d2.u.d <= d1.u.d; break;
	case 'H': d2.u.ld = d2.u.ld <= d1.u.ld; break;
	default: error("incorrect type in ge");
	}
	d2.type = CONST;
	push(d2);
}

void
eq(void)
{
	Datum d1, d2;

	d1 = eval();
	d2 = eval();
	switch ((pc++)->d.u.c) {
	case 'I': d2.u.i = d2.u.i == d1.u.i; break;
	case 'N': d2.u.u = d2.u.u == d1.u.u; break;
	case 'L': d2.u.l = d2.u.l == d1.u.l; break;
	case 'Z': d2.u.ul = d2.u.ul == d1.u.ul; break;
	case 'O': d2.u.ull = d2.u.ull == d1.u.ull; break;
	case 'Q': d2.u.ll = d2.u.ll == d1.u.ll; break;
	case 'J': d2.u.f = d2.u.f == d1.u.f; break;
	case 'D': d2.u.d = d2.u.d == d1.u.d; break;
	case 'H': d2.u.ld = d2.u.ld == d1.u.ld; break;
	default: error("incorrect type in ge");
	}
	d2.type = CONST;
	push(d2);
}

void
ne(void)
{	
	Datum d1, d2;

	d1 = eval();
	d2 = eval();
	switch ((pc++)->d.u.c) {
	case 'I': d2.u.i = d2.u.i != d1.u.i; break;
	case 'N': d2.u.u = d2.u.u != d1.u.u; break;
	case 'L': d2.u.l = d2.u.l != d1.u.l; break;
	case 'Z': d2.u.ul = d2.u.ul != d1.u.ul; break;
	case 'O': d2.u.ull = d2.u.ull != d1.u.ull; break;
	case 'Q': d2.u.ll = d2.u.ll != d1.u.ll; break;
	case 'J': d2.u.f = d2.u.f != d1.u.f; break;
	case 'D': d2.u.d = d2.u.d != d1.u.d; break;
	case 'H': d2.u.ld = d2.u.ld != d1.u.ld; break;
	default: error("incorrect type in ge");
	}
	d2.type = CONST;
	push(d2);
}

void
land(void)
{
	Datum d1, d2;

	d1 = eval();
	d2 = eval();
	switch ((pc++)->d.u.c) {
	case 'I': d2.u.i &= d1.u.i; break;
	case 'N': d2.u.u &= d1.u.u; break;
	case 'L': d2.u.l &= d1.u.l; break;
	case 'Z': d2.u.ul &= d1.u.ul; break;
	case 'O': d2.u.ull &= d1.u.ull; break;
	case 'Q': d2.u.ll &= d1.u.ll; break;
	default: error("incorrect type in mod");
	}
	d2.type = CONST;
	push(d2);
}

void
lor(void)
{
	Datum d1, d2;

	d1 = eval();
	d2 = eval();
	switch ((pc++)->d.u.c) {
	case 'I': d2.u.i |= d1.u.i; break;
	case 'N': d2.u.u |= d1.u.u; break;
	case 'L': d2.u.l |= d1.u.l; break;
	case 'Z': d2.u.ul |= d1.u.ul; break;
	case 'O': d2.u.ull |= d1.u.ull; break;
	case 'Q': d2.u.ll |= d1.u.ll; break;
	default: error("incorrect type in mod");
	}
	d2.type = CONST;
	push(d2);
}

void
lxor(void)
{
	Datum d1, d2;

	d1 = eval();
	d2 = eval();
	switch ((pc++)->d.u.c) {
	case 'I': d2.u.i ^= d1.u.i; break;
	case 'N': d2.u.u ^= d1.u.u; break;
	case 'L': d2.u.l ^= d1.u.l; break;
	case 'Z': d2.u.ul ^= d1.u.ul; break;
	case 'O': d2.u.ull ^= d1.u.ull; break;
	case 'Q': d2.u.ll ^= d1.u.ll; break;
	default: error("incorrect type in mod");
	}
	d2.type = CONST;
	push(d2);
}

void
print(void)
{
	Datum d;

	d = eval();
	switch ((pc++)->d.u.c) {
	case 'B': printf("%s\n", d.u.b ? "true": "false"); break;
	case 'M': printf("%hhu\n", d.u.sc); break;
	case 'C': printf("%hhd\n", d.u.c); break;
	case 'E': printf("%hu\n", d.u.us); break;
	case 'K': printf("%hd\n", d.u.s); break;
	case 'N': printf("%u\n", d.u); break;
	case 'I': printf("%d\n", d.u.i); break;
	case 'L': printf("%ld\n", d.u.l); break;
	case 'Z': printf("%lu\n", d.u.ul); break;
	case 'O': printf("%llu\n", d.u.ull); break;
	case 'Q': printf("%lld\n", d.u.ll); break;
	case 'J': printf("%f\n", d.u.f); break;
	case 'D': printf("%f\n", d.u.d); break;
	case 'H': printf("%Lf\n", d.u.ld); break;
	default: error("incorrect type in print");
	}
}

void
cast(void)
{
	Datum d;
	char t1, t2;

	d = eval();
	t1 = (pc++)->d.u.c;
	t2 = (pc++)->d.u.c;
	switch (t1) {
	case 'R':
		switch (t2) {
		case 'B': d.u.b = (bool) d.u.ptr; break;
		case 'M': d.u.sc = (unsigned char) d.u.ptr; break;
		case 'C': d.u.c = (char) d.u.ptr; break;
		case 'E': d.u.us = (unsigned short) d.u.ptr; break;
		case 'K': d.u.s = (short) d.u.ptr; break;
		case 'N': d.u.u = (unsigned) d.u.ptr; break;
		case 'I': d.u.i = (int) d.u.ptr; break;
		case 'L': d.u.l = (long) d.u.ptr; break;
		case 'Z': d.u.ul = (unsigned long) d.u.ptr; break;
		case 'O': d.u.ull = (unsigned long long) d.u.ptr; break;
		case 'Q': d.u.ll = (long long) d.u.ptr; break;
		default: goto bad_type;
		}
		break;
	case 'B':
		switch (t2) {
		case 'B': d.u.b = d.u.b; break;
		case 'M': d.u.sc = d.u.b; break;
		case 'C': d.u.c = d.u.b; break;
		case 'E': d.u.us = d.u.b; break;
		case 'K': d.u.s = d.u.b; break;
		case 'N': d.u.u = d.u.b; break;
		case 'I': d.u.i = d.u.b; break;
		case 'L': d.u.l = d.u.b; break;
		case 'Z': d.u.ul = d.u.b; break;
		case 'O': d.u.ull = d.u.b; break;
		case 'Q': d.u.ll = d.u.b; break;
		case 'J': d.u.f = d.u.b; break;
		case 'D': d.u.d = d.u.b; break;
		case 'H': d.u.ld = d.u.b; break;
		default: goto bad_type;
		}
		break;
	case 'C':
		switch (t2) {
		case 'B': d.u.b = d.u.c; break;
		case 'M': d.u.sc = d.u.c; break;
		case 'C': d.u.c = d.u.c; break;
		case 'E': d.u.us = d.u.c; break;
		case 'K': d.u.s = d.u.c; break;
		case 'N': d.u.u = d.u.c; break;
		case 'I': d.u.i = d.u.c; break;
		case 'L': d.u.l = d.u.c; break;
		case 'Z': d.u.ul = d.u.c; break;
		case 'O': d.u.ull = d.u.c; break;
		case 'Q': d.u.ll = d.u.c; break;
		case 'J': d.u.f = d.u.c; break;
		case 'D': d.u.d = d.u.c; break;
		case 'H': d.u.ld = d.u.c; break;
		default: goto bad_type;
		}
		break;
	case 'E':
		switch (t2) {
		case 'B': d.u.b = d.u.sc; break;
		case 'M': d.u.sc = d.u.sc; break;
		case 'C': d.u.c = d.u.sc; break;
		case 'E': d.u.us = d.u.sc; break;
		case 'K': d.u.s = d.u.sc; break;
		case 'N': d.u.u = d.u.sc; break;
		case 'I': d.u.i = d.u.sc; break;
		case 'L': d.u.l = d.u.sc; break;
		case 'Z': d.u.ul = d.u.sc; break;
		case 'O': d.u.ull = d.u.sc; break;
		case 'Q': d.u.ll = d.u.sc; break;
		case 'J': d.u.f = d.u.sc; break;
		case 'D': d.u.d = d.u.sc; break;
		case 'H': d.u.ld = d.u.sc; break;
		default: goto bad_type;
		}
		break;
	case 'K':
		switch (t2) {
		case 'B': d.u.b = d.u.s; break;
		case 'M': d.u.sc = d.u.s; break;
		case 'C': d.u.c = d.u.s; break;
		case 'E': d.u.us = d.u.s; break;
		case 'K': d.u.s = d.u.s; break;
		case 'N': d.u.u = d.u.s; break;
		case 'I': d.u.i = d.u.s; break;
		case 'L': d.u.l = d.u.s; break;
		case 'Z': d.u.ul = d.u.s; break;
		case 'O': d.u.ull = d.u.s; break;
		case 'Q': d.u.ll = d.u.s; break;
		case 'J': d.u.f = d.u.s; break;
		case 'D': d.u.d = d.u.s; break;
		case 'H': d.u.ld = d.u.s; break;
		default: goto bad_type;
		}
		break;
	case 'N':
		switch (t2) {
		case 'B': d.u.b = d.u.u; break;
		case 'M': d.u.sc = d.u.u; break;
		case 'C': d.u.c = d.u.u; break;
		case 'E': d.u.us = d.u.u; break;
		case 'K': d.u.s = d.u.u; break;
		case 'N': d.u.u = d.u.u; break;
		case 'I': d.u.i = d.u.u; break;
		case 'L': d.u.l = d.u.u; break;
		case 'Z': d.u.ul = d.u.u; break;
		case 'O': d.u.ull = d.u.u; break;
		case 'Q': d.u.ll = d.u.u; break;
		case 'J': d.u.f = d.u.u; break;
		case 'D': d.u.d = d.u.u; break;
		case 'H': d.u.ld = d.u.u; break;
		default: goto bad_type;
		}
		break;
	case 'I':
		switch (t2) {
		case 'B': d.u.b = d.u.i; break;
		case 'M': d.u.sc = d.u.i; break;
		case 'C': d.u.c = d.u.i; break;
		case 'E': d.u.us = d.u.i; break;
		case 'K': d.u.s = d.u.i; break;
		case 'N': d.u.u = d.u.i; break;
		case 'I': d.u.i = d.u.i; break;
		case 'L': d.u.l = d.u.i; break;
		case 'Z': d.u.ul = d.u.i; break;
		case 'O': d.u.ull = d.u.i; break;
		case 'Q': d.u.ll = d.u.i; break;
		case 'J': d.u.f = d.u.i; break;
		case 'D': d.u.d = d.u.i; break;
		case 'H': d.u.ld = d.u.i; break;
		default: goto bad_type;
		}
		break;
	case 'L':
		switch (t2) {
		case 'B': d.u.b = d.u.l; break;
		case 'M': d.u.sc = d.u.l; break;
		case 'C': d.u.c = d.u.l; break;
		case 'E': d.u.us = d.u.l; break;
		case 'K': d.u.s = d.u.l; break;
		case 'N': d.u.u = d.u.l; break;
		case 'I': d.u.i = d.u.l; break;
		case 'L': d.u.l = d.u.l; break;
		case 'Z': d.u.ul = d.u.l; break;
		case 'O': d.u.ull = d.u.l; break;
		case 'Q': d.u.ll = d.u.l; break;
		case 'J': d.u.f = d.u.l; break;
		case 'D': d.u.d = d.u.l; break;
		case 'H': d.u.ld = d.u.l; break;
		default: goto bad_type;
		}
		break;
	case 'Z':
		switch (t2) {
		case 'B': d.u.b = d.u.ul; break;
		case 'M': d.u.sc = d.u.ul; break;
		case 'C': d.u.c = d.u.ul; break;
		case 'E': d.u.us = d.u.ul; break;
		case 'K': d.u.s = d.u.ul; break;
		case 'N': d.u.u = d.u.ul; break;
		case 'I': d.u.i = d.u.ul; break;
		case 'L': d.u.l = d.u.ul; break;
		case 'Z': d.u.ul = d.u.ul; break;
		case 'O': d.u.ull = d.u.ul; break;
		case 'Q': d.u.ll = d.u.ul; break;
		case 'J': d.u.f = d.u.ul; break;
		case 'D': d.u.d = d.u.ul; break;
		case 'H': d.u.ld = d.u.ul; break;
		default: goto bad_type;
		}
		break;
	case 'O':
		switch (t2) {
		case 'B': d.u.b = d.u.ull; break;
		case 'M': d.u.sc = d.u.ull; break;
		case 'C': d.u.c = d.u.ull; break;
		case 'E': d.u.us = d.u.ull; break;
		case 'K': d.u.s = d.u.ull; break;
		case 'N': d.u.u = d.u.ull; break;
		case 'I': d.u.i = d.u.ull; break;
		case 'L': d.u.l = d.u.ull; break;
		case 'Z': d.u.ul = d.u.ull; break;
		case 'O': d.u.ull = d.u.ull; break;
		case 'Q': d.u.ll = d.u.ull; break;
		case 'J': d.u.f = d.u.ull; break;
		case 'D': d.u.d = d.u.ull; break;
		case 'H': d.u.ld = d.u.ull; break;
		default: goto bad_type;
		}
		break;
	case 'Q':
		switch (t2) {
		case 'B': d.u.b = d.u.ll; break;
		case 'M': d.u.sc = d.u.ll; break;
		case 'C': d.u.c = d.u.ll; break;
		case 'E': d.u.us = d.u.ll; break;
		case 'K': d.u.s = d.u.ll; break;
		case 'N': d.u.u = d.u.ll; break;
		case 'I': d.u.i = d.u.ll; break;
		case 'L': d.u.l = d.u.ll; break;
		case 'Z': d.u.ul = d.u.ll; break;
		case 'O': d.u.ull = d.u.ll; break;
		case 'Q': d.u.ll = d.u.ll; break;
		case 'J': d.u.f = d.u.ll; break;
		case 'D': d.u.d = d.u.ll; break;
		case 'H': d.u.ld = d.u.ll; break;
		default: goto bad_type;
		}
		break;
	case 'J':
		switch (t2) {
		case 'B': d.u.b = d.u.f; break;
		case 'M': d.u.sc = d.u.f; break;
		case 'C': d.u.c = d.u.f; break;
		case 'E': d.u.us = d.u.f; break;
		case 'K': d.u.s = d.u.f; break;
		case 'N': d.u.u = d.u.f; break;
		case 'I': d.u.i = d.u.f; break;
		case 'L': d.u.l = d.u.f; break;
		case 'Z': d.u.ul = d.u.f; break;
		case 'O': d.u.ull = d.u.f; break;
		case 'Q': d.u.ll = d.u.f; break;
		case 'J': d.u.f = d.u.f; break;
		case 'D': d.u.d = d.u.f; break;
		case 'H': d.u.ld = d.u.f; break;
		default: goto bad_type;
		}
		break;
	case 'D':
		switch (t2) {
		case 'B': d.u.b = d.u.d; break;
		case 'M': d.u.sc = d.u.d; break;
		case 'C': d.u.c = d.u.d; break;
		case 'E': d.u.us = d.u.d; break;
		case 'K': d.u.s = d.u.d; break;
		case 'N': d.u.u = d.u.d; break;
		case 'I': d.u.i = d.u.d; break;
		case 'L': d.u.l = d.u.d; break;
		case 'Z': d.u.ul = d.u.d; break;
		case 'O': d.u.ull = d.u.d; break;
		case 'Q': d.u.ll = d.u.d; break;
		case 'J': d.u.f = d.u.d; break;
		case 'D': d.u.d = d.u.d; break;
		case 'H': d.u.ld = d.u.d; break;
		default: goto bad_type;
		}
		break;
	case 'H':
		switch (t2) {
		case 'B': d.u.b = d.u.ld; break;
		case 'M': d.u.sc = d.u.ld; break;
		case 'C': d.u.c = d.u.ld; break;
		case 'E': d.u.us = d.u.ld; break;
		case 'K': d.u.s = d.u.ld; break;
		case 'N': d.u.u = d.u.ld; break;
		case 'I': d.u.i = d.u.ld; break;
		case 'L': d.u.l = d.u.ld; break;
		case 'Z': d.u.ul = d.u.ld; break;
		case 'O': d.u.ull = d.u.ld; break;
		case 'Q': d.u.ll = d.u.ld; break;
		case 'J': d.u.f = d.u.ld; break;
		case 'D': d.u.d = d.u.ld; break;
		case 'H': d.u.ld = d.u.ld; break;
		default: goto bad_type;
		}
		break;
	default:
		goto bad_type;
	}

	push(d);
	return;
bad_type:
	error("bad type in casting");
}
