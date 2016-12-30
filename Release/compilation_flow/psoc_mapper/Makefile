#CFLAGS=-m32
CFLAGS += -O0 -g3 -Wall
# CFLAGS += -DDEBUG
LDFLAGS += -lm
# CSRCS=dealer.c sched.c
CSRCS=sched.c psoc_mapper.c
EXENAME=psoc_mapper

all:
	$(CC) $(CSRCS) $(CFLAGS) -o $(EXENAME) $(LDFLAGS)
#   Cygwin might have no "make" installed: gcc psoc_mapper.c sched.c -O0 -g3  -Wall -o psoc_mapper -lm

install: all
	mkdir -p $(PREFIX)/bin
	cp psoc_mapper $(PREFIX)/bin
 
# uninstall:
# 	rm -f $(PREFIX)/bin/psoc_mapper

clean:
	rm -f psoc_mapper

run:
	./psoc_mapper regression-tests/14_test-melani1.dot regression-tests/15_test-melani2.dot
