
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
		    aalta_formula * & conjunction) {

    ltl_formulas * parsed = getASTSF(file);
    conjunction = aalta_formula::TRUE();
    for (int i = 0; i<parsed->size; i++) {
      aalta_formula * f = aalta_formula(parsed->formulas[i],
					false, true).unique();
      aalta_formula * n = aalta_formula(parsed->names[i],
					false, true).unique();
      // n -> f ---  !n | f
      aalta_formula * imp = aalta_formula(parsed->names[i],
					  true, true).unique();
      imp = aalta_formula(aalta_formula::Or, imp, f).unique();
      conjunction = aalta_formula(aalta_formula::And, conjunction, imp).unique();
      names.push_back(n);
      formulas.push_back(imp);
    }
  }
}
