
DIRS = lib cc1 cc2

all clean install uninstall:
	for i in $(DIRS) ;\
	do \
		(cd $$i && $(MAKE) CC=c99 $@) ;\
	done
