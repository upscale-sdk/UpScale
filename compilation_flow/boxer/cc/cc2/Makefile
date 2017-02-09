
OBJS = main.o code.o

all: cc2

$(OBJS) : cc2.h

cc2: $(OBJS)
	$(CC) $(LDFLAGS) $(CFLAGS) $(LIBS) $(OBJS) -o $@

install: cc2
	mkdir -p $(PREFIX)/bin
	cp cc2 $(PREFIX)/bin/pcc2

uninstall:
	rm -f $(PREFIX)/bin/pcc2

clean:
	rm -f $(OBJS)
	rm -f cc2 pcc2
