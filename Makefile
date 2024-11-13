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
PROG2 = cmd_parse
PROGS = $(PROG1)

all: $(PROGS)

$(PROG1): $(PROG1).o $(PROG2).o
	$(CC) $(CFLAGS) -o $@ $^ -lmd

$(PROG1).o: $(PROG1).c
	$(CC) $(CFLAGS) -c $< -lmd

$(PROG2): $(PROG2).o
	$(CC) $(CFLAGS) -o $@ $^ -lmd

$(PROG2).o: $(PROG2).c $(PROG2).h
	$(CC) $(CFLAGS) -c $< -lmd

clean cls:
	rm -f $(PROGS) *.o *~ \#*

tar:
	tar cvfa bin_file_${LOGNAME}.tar.gz *.[ch] [mM]akefile

git:
	git add Makefile psush.c 
	git commit -m "end of work submit"
	git push 

