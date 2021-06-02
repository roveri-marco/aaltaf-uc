#include "formula/aalta_formula.h"
#include "ltlfchecker.h"
#include "carchecker.h"
#include "uc.h"
#include "solver.h"
#include <stdio.h>
#include <string.h>
#include <chrono>

#define MAXN 100000
char in[MAXN];

using namespace aalta;

using TVar=std::chrono::high_resolution_clock::time_point;

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
  TVar t0, t1, t2, t3, t4, t5, t6, t7;
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
    t0 = chrono::high_resolution_clock::now();
    get_formulas(file, names, formulas, af);
    t1 = chrono::high_resolution_clock::now();
    cout << "-- Parsing of the file time: "
      << to_string(chrono::duration_cast<chrono::nanoseconds>(t1-t0).count()/1e9)
      << endl;
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
  // Converts formula in NNF
  af = af->nnf();
  af = af->add_tail();
  // Rewrites weak next with N f <-> Tail | X f
  af = af->remove_wnext();
  // Rewrites weak yesterday with Z f <-> ! Y !f
  af = af->remove_wyesterday();
  // We remove the past and add the respective tableau variables and
  // tableau transition system (/\_i init_i G(/\_i trans_i)
  af = af->remove_past();
  // Simplify the formula
  af = af->simplify();
  // Pushes X over and/or operators
  af = af->split_next();
  // Pushes Y over and/or operators
  af = af->split_yesterday();
  t2 = chrono::high_resolution_clock::now();
  cout << "-- Preprocessing time: "
       << to_string(chrono::duration_cast<chrono::nanoseconds>(t2-t1).count()/1e9)
      << endl;

  // cout << af->to_string() << endl;

  if (blsc) {
    LTLfChecker checker (af, verbose, evidence);
    t3 = chrono::high_resolution_clock::now();
    cout << "-- Checker creation time: "
       << to_string(chrono::duration_cast<chrono::nanoseconds>(t3-t2).count()/1e9)
      << endl;
    if (uc) {
      checker.add_assumptions(names);
      t5 = chrono::high_resolution_clock::now();
      cout << "-- Checker add assumption time: "
	   << to_string(chrono::duration_cast<chrono::nanoseconds>(t4-t3).count()/1e9)
	   << endl;
    }
    bool res = checker.check ();
    t6 = chrono::high_resolution_clock::now();
    cout << "-- Checker check time: "
	   << to_string(chrono::duration_cast<chrono::nanoseconds>(t5-t4).count()/1e9)
	   << endl;
    if (!uc) {
      cout <<  (res ? "sat" : "unsat") << endl;
    } else {
      cout << "-- The set of formulas is " << (res ? "sat" : "unsat") << endl;
    }
    if (evidence && res)
      checker.print_evidence ();
    if (uc && !res) {
      cout << "-- unsat core:";
      checker.print_uc(); cout << endl;
      cout << "-- unsat core size: "
	   << checker.get_uc_size() << endl;
    }
  }
  else {
    CARChecker checker (af, verbose, evidence);
    t3 = chrono::high_resolution_clock::now();
    cout << "-- Checker creation time: "
	 << to_string(chrono::duration_cast<chrono::nanoseconds>(t3-t2).count()/1e9)
	 << endl;
    if (uc) {
      checker.add_assumptions(names);
      t4 = chrono::high_resolution_clock::now();
      cout << "-- Checker add assumption time: "
	   << to_string(chrono::duration_cast<chrono::nanoseconds>(t4-t3).count()/1e9)
	   << endl;
    }
    bool res = checker.check ();
    t5 = chrono::high_resolution_clock::now();
    cout << "-- Checker check time: "
	   << to_string(chrono::duration_cast<chrono::nanoseconds>(t5-t4).count()/1e9)
	   << endl;
    if (!uc) {
      cout <<  (res ? "sat" : "unsat") << endl;
    } else {
      cout << "-- The set of formulas is " << (res ? "sat" : "unsat") << endl;
    }
    if (evidence && res)
      checker.print_evidence ();
    if (uc && !res) {
      cout << "-- unsat core:";
      checker.print_uc(); cout << endl;
      cout << "-- unsat core size: "
	   << checker.get_uc_size() << endl;
    }
  }
  aalta_formula::destroy();
  t6 = chrono::high_resolution_clock::now();
  cout << "-- Checker unsat core extraction time: "
       << to_string(chrono::duration_cast<chrono::nanoseconds>(t6-t5).count()/1e9)
	   << endl;

  cout << "-- Checker total time: "
       << to_string(chrono::duration_cast<chrono::nanoseconds>(t6-t0).count()/1e9)
	   << endl;
}


int
main (int argc, char** argv)
{
  ltlf_sat (argc, argv);
  return 0;
}
