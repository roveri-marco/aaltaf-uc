#ifndef UC_CODE_H
#define	UC_CODE_H


#include <iostream>
#include <assert.h>
#include <vector>

#include "formula/aalta_formula.h"
#include "ltlparser/ltl_formula.h"
#include "ltlparser/trans.h"
#include "debug.h"

using namespace std;

namespace aalta {
  using AaltaFormulaVec=std::vector<aalta_formula*>;
  void get_formulas(FILE * file, AaltaFormulaVec & names,
		    AaltaFormulaVec & formulas,
		    aalta_formula * & conjunction,
		    bool get_imp = false);
}

#endif
