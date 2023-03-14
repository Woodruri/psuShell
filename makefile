CC = gcc
DEBUG = -g

CFLAGS = $(DEBUG) -Wall -Wshadow -Wunreachable-code -Wredundant-decls -Wextra \
        -Wmissing-declarations -Wold-style-definition -Wmissing-prototypes \
        -Wdeclaration-after-statement -Wunsafe-loop-optimizations $(DEFINES) \
        -Wno-return-local-address -uninitialized -Wunused \
		# -Werror

PROG = cmd_parse

all: $(PROG) main.o

$(PROG): $(PROG).o main.o
	$(CC) $(CFLAGS) -o $@ $^
	chmod a+rx,g-w $@

main.o: main.c $(PROG).h
	$(CC) $(CFLAGS) -c $<

$(PROG).o: $(PROG).c $(PROG).h makefile
	$(CC) $(CFLAGS) -c $<

.PHONY: clean

clean:
	rm -f psush *.o *~ \#*

git:
	if [ ! -d .git] ; then git init; fi
	git add *.[ch] makefile
	git commit -m"no comments"

canvas:
	make clean
	cd ..; tar --exclude-backups --exclude-vcs -c -a -f ./${LOGNAME}-lab3.tar.gz $(DIR)

