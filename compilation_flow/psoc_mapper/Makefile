#CFLAGS=-m32
CFLAGS += -O0 -g3 -Wall
#CFLAGS += -DDEBUG
LDFLAGS += -lm
# CSRCS=dealer.c sched.c
LIB_CSRCS=sched.c dag.c
CSRCS=psoc_mapper.c
EXENAME=psoc_mapper

lib:
	$(CC) $(CSRCS) ${LIB_CSRCS} $(CFLAGS) -o $(EXENAME) $(LDFLAGS)
	

all: lib
#   Cygwin might have no "make" installed: gcc psoc_mapper.c sched.c dag.c -O0 -g3  -Wall -o psoc_mapper -lm

install: all
	mkdir -p $(PREFIX)/bin
	cp psoc_mapper $(PREFIX)/bin
 
# uninstall:
# 	rm -f $(PREFIX)/bin/psoc_mapper

clean:
	rm -f psoc_mapper

run:
	./psoc_mapper -f -a regression-tests/14_test-melani1.dot regression-tests/15_test-melani2.dot
