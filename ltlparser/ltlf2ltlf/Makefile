# Parser Makefile
# need flex bison

PARSERCFILES =	../ltllexer.c ../ltlparser.c
PARSERHFILES =	../ltllexer.h ../ltlparser.h
ALLCFILES   =	$(PARSERCFILES) ../ltl_formula.c ../trans.c ../utility.cpp ltlf2ltlf.cpp
CC	    =   g++
CDEBUG	    =	-g
INCLUDE	    =   -I ../

.DEFAULT: run

ltlf2ltlf:	$(PARSERCFILES)
	$(CC) $(INCLUDE) $(ALLCFILES) -o ltlf2ltlf

ltllexer.c:	../grammar/ltllexer.l
		flex ../grammar/ltllexer.l

ltlparser.c:	../grammar/ltlparser.y
		bison ../grammar/ltlparser.y

run:	$(ALLCFILES)
	$(CC) $(INCLUDE) $(ALLCFILES) -o ltlf2ltlf

run_debug:  $(ALLCFILES)
	    $(CC) $(INCLUDE) $(CDEBUG) $(ALLCFILES) -o ltl2ltlf

run_release:	run

clean:
	rm -f *.o *~ $(PARSERHFILES) $(PARSERCFILES) ltl2ltlf
