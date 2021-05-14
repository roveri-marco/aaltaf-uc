
FORMULAFILES =	formula/aalta_formula.cpp formula/olg_formula.cpp formula/olg_item.cpp

PARSERFILES  =	ltlparser/ltl_formula.c ltlparser/ltllexer.c ltlparser/ltlparser.c ltlparser/trans.c

UTILFILES    =	util/utility.cpp utility.cpp

SOLVER		= minisat/core/Solver.cc aaltasolver.cpp solver.cpp carsolver.cpp

CHECKING	= ltlfchecker.cpp carchecker.cpp

OTHER		= evidence.cpp uc.cpp


ALLFILES     =	$(CHECKING) $(SOLVER) $(FORMULAFILES) $(PARSERFILES) $(UTILFILES) $(OTHER) main.cpp


CC	    =   g++
FLAG    = -I./  -I./minisat/  -D __STDC_LIMIT_MACROS -D __STDC_FORMAT_MACROS
DEBUGFLAG   =	-DDEBUG -g -pg
RELEASEFLAG =	-O2 -g

aaltaf :	release

ltlparser/ltllexer.c : ltlparser/grammar/ltllexer.l
	cd ltlparser/grammar && flex ltllexer.l

ltlparser/ltlparser.c : ltlparser/grammar/ltlparser.y
	cd ltlparser/grammar && bison ltlparser.y

.PHONY : release debug clean

release : $(ALLFILES)
	$(CC) $(FLAG) $(RELEASEFLAG) $(ALLFILES) -lz -o aaltaf

debug : $(ALLFILES)
	$(CC) $(FLAG) $(DEBUGFLAG) $(ALLFILES) -lz -o aaltaf

clean :
	rm -f *.o *~ aaltaf ltlparser/ltllexer.* ltlparser/ltlparser.*
