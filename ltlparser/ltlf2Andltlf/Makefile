# Parser Makefile
# need flex bison

PARSERCFILES =	../ltllexer.c ../ltlparser.c
PARSERHFILES =	../ltllexer.h ../ltlparser.h
ALLCFILES   =	$(PARSERCFILES) ../ltl_formula.c ../trans.c ../utility.cpp ltlf2Andltlf.cpp
CC	    =   g++
CDEBUG	    =	-g
INCLUDE	    =   -I../ -I../..

ALL : run

ltlf2Andltlf:	$(PARSERCFILES)

../ltllexer.c:	../grammar/ltllexer.l
		flex ../grammar/ltllexer.l

../ltlparser.c:	../grammar/ltlparser.y
		bison ../grammar/ltlparser.y

run:	$(ALLCFILES)
	$(CC) -g $(INCLUDE) $(ALLCFILES) -o ltlf2Andltlf

run_debug:  $(ALLCFILES)
	    $(CC) $(INCLUDE) $(CDEBUG) $(ALLCFILES) -o ltlf2Andltl

run_release:	run

clean:
	rm -f *.o *~ $(PARSERHFILES) $(PARSERCFILES) ltlf2Andltlf
