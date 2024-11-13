CC = gcc
DEBUG = -g
DEFINES =
WERROR = 
#WERROR = -Werror 
CFLAGS = $(DEBUG) -Wall -Wextra -Wshadow -Wunreachable-code -Wredundant-decls \
	-Wmissing-declarations -Wold-style-definition \
	-Wmissing-prototypes -Wdeclaration-after-statement \
	-Wno-return-local-addr -Wunsafe-loop-optimizations \
	-Wuninitialized $(WERROR) $(DEFINES)

PROG1 = psush
PROGS = $(PROG1)

all: $(PROGS)

$(PROG1): $(PROG1).o
	$(CC) $(CFLAGS) -o $@ $^

$(PROG1).o:
	$(CC) $(CFLAGS) -c $<

clean cls:
	rm -f $(PROGS) *.o *~ \#*

tar:
	tar cvfa bin_file_${LOGNAME}.tar.gz *.[ch] [mM]akefile

git:
	git add Makefile psush.c
	git commit -m "end of work submit"
	git push 

