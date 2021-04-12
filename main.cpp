#include "formula/aalta_formula.h"
#include "ltlfchecker.h"
#include "carchecker.h"
#include "uc.h"
#include "solver.h"
#include <stdio.h>
#include <string.h>

#define MAXN 100000
char in[MAXN];

using namespace aalta;

using AaltaFormulaVec=std::vector<aalta_formula*>;

void print_help(const char *name) {
  cout << "usage: " << name << " [-e|-u|-v|-blsc|-t|-l|-h] [-f file | \"formula\"]" << endl;
  cout << "\t -f\t:\t Reads formulas from file" << endl;
  cout << "\t -e\t:\t Prints example when result is SAT" << endl;
  cout << "\t -u\t:\t Computes the unsat core w.r.t. the "
       << "conjunction of the input formulas."
       << "\n\t   \t \t\t If not specified, only firt formula is considered." << endl;
  cout << "\t -v\t:\t Print verbose details" << endl;
  cout << "\t -blsc\t:\t Uses the BLSC checking method; Default is CDLSC" << endl;
  cout << "\t -t\t:\t Prints weak until formula and exit" << endl;
  cout << "\t -l\t:\t Prints weak until formula and continue" << endl;
  cout << "\t -h\t:\t Prints help information" << endl;
}

void
ltlf_sat (int argc, char** argv)
{
  bool uc = false;
  bool verbose = false;
  bool evidence = false;
  int input_count = 0;
  bool blsc = false;
  bool print_weak_until_free = false;
  bool print_formula_and_continue = false;
  char * ffile = NULL;
  FILE * file = NULL;
  for (int i = 1; i < argc; i++) {
    if (strcmp (argv[i], "-v") == 0)
      verbose = true;
    else if (strcmp (argv[i], "-e") == 0)
      evidence = true;
    else if (strcmp (argv[i], "-u") == 0)
      uc = true;
    else if (strcmp (argv[i], "-blsc") == 0)
      blsc = true;
    else if (strcmp (argv[i], "-t") == 0)
      print_weak_until_free = true;
    else if (strcmp (argv[i], "-l") == 0)
      print_formula_and_continue = true;
    else if (strcmp (argv[i], "-h") == 0) {
      print_help (argv[0]);
      exit (0);
    }
    else if (strcmp(argv[i], "-f") == 0) {
      if (i + 1 < argc) {
	ffile = (char *)malloc(strlen(argv[i+1])+1);
	sprintf(ffile, "%s", argv[i+1]);
	file = fopen(ffile, "r");
	i++;
	if (NULL == file) {
	  printf("Unable to open file \"%s\"\n"
		 "since either the file does not exist,\n"
		 "or you do not have the rights to open it\n", ffile);
	  free(ffile);
	  exit(1);
	}
      }
      else {
	print_help(argv[0]);
	exit(1);
      }
    }
    else {
      print_help(argv[0]);
      exit(1);
    }
  }

  if ((NULL == ffile) && (NULL == file)) {
    puts("please input the formula:");
    file = stdin;
  }

  aalta_formula* af;
  AaltaFormulaVec names;
  AaltaFormulaVec formulas;
  //set tail id to be 1
  af = aalta_formula::TAIL();

  if (uc) {
    get_formulas(file, names, formulas, af);
    if (file != stdin) fclose(file);
    if (print_weak_until_free || print_formula_and_continue) {
      auto n = names.begin();
      auto f = formulas.begin();
      // cout << af->to_string() << endl;

      for (; n != names.end(); ) {
	auto el = *n; auto el1 = *f;
	cout << "" << el->to_string() << " := "
	     << el1->to_string() << ";" << endl;
	n++; f++;
      }
      if (!print_formula_and_continue)
	return;
    }

  }
  else {
    af = aalta_formula(file, true).unique();
    if (file != stdin) fclose(file);
    if (print_weak_until_free || print_formula_and_continue) {
      cout << af->to_string() << endl;
      if (!print_formula_and_continue)
	return;
    }
  }

  af = af->nnf();
  af = af->add_tail();
  af = af->remove_wnext();
  af = af->simplify();
  af = af->split_next();

  // cout << af->to_string() << endl;

  if (blsc) {
    LTLfChecker checker (af, verbose, evidence);
    if (uc) {
      checker.add_assumptions(names);
    }
    bool res = checker.check ();
    if (!uc) {
      cout <<  (res ? "sat" : "unsat") << endl;
    } else {
      cout << "The set of formulas is " << (res ? "sat" : "unsat") << endl;
    }
    if (evidence && res)
      checker.print_evidence ();
    if (uc && !res) {
      cout << "unsat core:";
      checker.print_uc(); cout << endl;
    }
  }
  else {
    CARChecker checker (af, verbose, evidence);
    if (uc) {
      checker.add_assumptions(names);
    }
    bool res = checker.check ();
    if (!uc) {
      cout <<  (res ? "sat" : "unsat") << endl;
    } else {
      cout << "The set of formulas is " << (res ? "sat" : "unsat") << endl;
    }
    if (evidence && res)
      checker.print_evidence ();
    if (uc && !res) {
      cout << "unsat core:";
      checker.print_uc(); cout << endl;
    }
  }
  aalta_formula::destroy();
}


int
main (int argc, char** argv)
{
  ltlf_sat (argc, argv);
  return 0;
}
