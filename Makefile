
FORMULAFILES =	formula/aalta_formula.cpp formula/olg_formula.cpp formula/olg_item.cpp

PARSERFILES  =	ltlparser/ltl_formula.c ltlparser/ltllexer.c ltlparser/ltlparser.c ltlparser/trans.c

UTILFILES    =	util/utility.cpp utility.cpp

SOLVER		= minisat/core/Solver.cc aaltasolver.cpp solver.cpp carsolver.cpp

CHECKING	= ltlfchecker.cpp carchecker.cpp

OTHER		= evidence.cpp uc.cpp


ALLFILES     =	$(CHECKING) $(SOLVER) $(FORMULAFILES) $(PARSERFILES) $(UTILFILES) $(OTHER) main.cpp

ALLF_OBJS := $(ALLFILES:%=%.o)

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

release : OFLAG=$(RELEASEFLAG)
release : ltlparser/ltllexer.c ltlparser/ltlparser.c $(ALLF_OBJS)
	$(CC) $(FLAG) $(RELEASEFLAG) $(ALLF_OBJS) -lz -o aaltaf

debug : OFLAG=$(DEBUGFLAG)
debug : ltlparser/ltllexer.c ltlparser/ltlparser.c $(ALLF_OBJS)
	$(CC) $(FLAG) $(DEBUGFLAG) $(ALLF_OBJS) -lz -o aaltaf

clean :
	rm -f *.o *~ aaltaf ltlparser/ltllexer.* ltlparser/ltlparser.*

test :
	echo $(ALLFILES)
	echo $(ALLF_OBJS)

# c source
%.c.o: %.c
	$(CC) $(FLAG) $(OFLAG) -c $< -o $@

# c++ source
%.cpp.o: %.cpp
	$(CXX) $(FLAG) $(OFLAG) -c $< -o $@

# c++ source
%.cc.o: %.cc
	$(CXX) $(FLAG) $(OFLAG) -c $< -o $@
