OBJS = json.o wrappers.o main.o tree.o box.o task.o expr.o

CFLAGS = -ggdb3 -O0

all: cc boxer display

$(OBJS): boxer.h

cc:
	cd cc; make  CC=c99 install

boxer: $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS)

install: boxer
	mkdir -p $(PREFIX)/bin #$(PREFIX)/man/man1
	cp boxer $(PREFIX)/bin
	cd cc; make install

uninstall:
	rm -f $(PREFIX)/bin/boxer

clean:
	rm -f boxer  display *.o 
	rm -f cc/cc1/cc1 cc/cc1/pcc1 cc/cc2/cc2 cc/cc2/pcc2
	rm -f cc/cc1/*.o cc/cc2/*.o
