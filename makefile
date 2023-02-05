CC = gcc
DEBUG = -g

CFLAGS = $(DEBUG) -Wall -Wshadow -Wunreachable-code -Wredundant-decls -Wextra \
        -Wmissing-declarations -Wold-style-definition -Wmissing-prototypes \
        -Wdeclaration-after-statement -Wunsafe-loop-optimizations $(DEFINES) \
		# -Werror

PROG = cmd_parse

all: $(PROG)

$(PROG): $(PROG).o 
	$(CC) $(CFLAGS) -o $@ $^
	chmod a+rx,g-w $@

$(PROG).o: $(PROG).c $(PROG).h makefile
	$(CC) $(CFLAGS) -c $<

.PHONY: clean

clean:
	rm -f *.o *~ \#*

git:
	if [ ! -d .git] ; then git init; fi
	git add *.[ch] makefile
	git commit -m"no comments"
