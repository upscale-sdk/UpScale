
#include <stdbool.h>

typedef union inst Inst;
typedef struct var Var;
typedef struct datum Datum;

struct datum {
	char type;
	union {
		bool b;
		float f;
		double d;
		long double ld;
		unsigned char c;
		signed char sc;
		short s;
		unsigned short us;
		int i;
		unsigned u;
		long l;
		unsigned long ul;
		long long ll;
		unsigned long long ull;
		Var *v;
		Var **ptr;
		Inst *code;
	} u;
};

union inst {
	Datum d;
	Var *v;
	void (*f)(void);
};

struct var {
	Datum val;
	char *name;
	char defined;
};

extern void error(char *err);

#define STACKSIZ 32
#define MEMSIZ   2048
#define VARSIZ   2048

#define code(x) install((Inst) {.f = x})
#define data(x) install((Inst) {.d = x})
#define variable(x) install((Inst) {.v = x})

#define STOP ((void (*)(void)) 0)

extern void initcode(void);
extern Inst *install(Inst i);
extern void execute(Inst *p);
