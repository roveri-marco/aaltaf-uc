#include <chrono>
#include <stdio.h>
#include <string.h>

#include "carchecker.h"
#include "formula/aalta_formula.h"
#include "ltlfchecker.h"
#include "solver.h"
#include "uc.h"

#define MAXN 100000
char in[MAXN];

using namespace aalta;

using TVar = std::chrono::high_resolution_clock::time_point;

using AaltaFormulaVec = std::vector<aalta_formula*>;

void print_help(const char* name) {
  cout << "usage: " << name << " [-e|-u|-v|-blsc|-t|-l|-h] [-f file | \"formula\"]" << endl;
  cout << "\t -f\t:\t Reads formulas from file" << endl;
  cout << "\t -e\t:\t Prints example when result is SAT" << endl;
  cout << "\t -u\t:\t Computes the unsat core w.r.t. the "
       << "conjunction of the input formulas."
       << "\n\t   \t \t\t If not specified, only firt formula is considered." << endl;
  cout << "\t -mus\t:\t Computes the minimal unsatisfiable subset w.r.t. the "
       << "conjunction of the input formulas." << endl;
  cout << "\t -emus\t:\t Enumerates all minimal unsatisfiable subsets w.r.t. the "
       << "conjunction of the input formulas." << endl;
  cout << "\t -v\t:\t Print verbose details" << endl;
  cout << "\t -blsc\t:\t Uses the BLSC checking method; Default is CDLSC" << endl;
  cout << "\t -t\t:\t Prints weak until formula and exit" << endl;
  cout << "\t -l\t:\t Prints weak until formula and continue" << endl;
  cout << "\t -h\t:\t Prints help information" << endl;
}

void ltlf_sat(int argc, char** argv) {
  TVar  t0, t1, t2, t3, t4, t5, t6, t7;
  bool  uc                         = false;
  bool  mus                        = false;
  bool  emus                       = false;
  bool  emus2                      = false;
  bool  verbose                    = false;
  bool  evidence                   = false;
  int   input_count                = 0;
  bool  blsc                       = false;
  bool  print_weak_until_free      = false;
  bool  print_formula_and_continue = false;
  char* ffile                      = NULL;
  FILE* file                       = NULL;
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-v") == 0)
      verbose = true;
    else if (strcmp(argv[i], "-e") == 0)
      evidence = true;
    else if (strcmp(argv[i], "-u") == 0)
      uc = true;
    else if (strcmp(argv[i], "-mus") == 0) {
      mus = true;
      uc  = true;
    } else if (strcmp(argv[i], "-emus") == 0) {
      emus = true;
      // mus = true;
      uc = true;
    } else if (strcmp(argv[i], "-emus2") == 0) {
      emus2 = true;
    } else if (strcmp(argv[i], "-blsc") == 0)
      blsc = true;
    else if (strcmp(argv[i], "-t") == 0)
      print_weak_until_free = true;
    else if (strcmp(argv[i], "-l") == 0)
      print_formula_and_continue = true;
    else if (strcmp(argv[i], "-h") == 0) {
      print_help(argv[0]);
      exit(0);
    } else if (strcmp(argv[i], "-f") == 0) {
      if (i + 1 < argc) {
        ffile = (char*)malloc(strlen(argv[i + 1]) + 1);
        sprintf(ffile, "%s", argv[i + 1]);
        file = fopen(ffile, "r");
        i++;
        if (NULL == file) {
          printf("Unable to open file \"%s\"\n"
                 "since either the file does not exist,\n"
                 "or you do not have the rights to open it\n",
                 ffile);
          free(ffile);
          exit(1);
        }
      } else {
        print_help(argv[0]);
        exit(1);
      }
    } else {
      print_help(argv[0]);
      exit(1);
    }
  }

  if ((NULL == ffile) && (NULL == file)) {
    puts("please input the formula:");
    file = stdin;
  }

  aalta_formula*  af;
  AaltaFormulaVec names;
  AaltaFormulaVec formulas;
  // set tail id to be 1
  af = aalta_formula::TAIL();

  if (emus2) {
    t0 = chrono::high_resolution_clock::now();
    get_formulas(file, names, formulas, af);
    t1 = chrono::high_resolution_clock::now();
    cout << "-- Parsing of the file time: "
         << to_string(chrono::duration_cast<chrono::nanoseconds>(t1 - t0).count() / 1e9) << endl;

    if (file != stdin)
      fclose(file);

    af = af->nnf();
    af = af->add_tail();
    af = af->remove_wnext();
    af = af->simplify();
    af = af->split_next();

    t2 = chrono::high_resolution_clock::now();
    cout << "-- Preprocessing time: "
         << to_string(chrono::duration_cast<chrono::nanoseconds>(t2 - t1).count() / 1e9) << endl;

    CARChecker checker(af, verbose, evidence);
    t3 = chrono::high_resolution_clock::now();
    checker.add_assumptions(names);

    double first_checker_creation =
        chrono::duration_cast<chrono::nanoseconds>(t3 - t2).count() / 1e9;

    if (!checker.check()) {
      t4 = chrono::high_resolution_clock::now();
      double first_checker_check =
          chrono::duration_cast<chrono::nanoseconds>(t4 - t3).count() / 1e9;

      std::vector<MUSInfo> all_mus =
          checker.enumerate_all_mus_v2(names, first_checker_creation, first_checker_check);

      t5 = chrono::high_resolution_clock::now();
      cout << "-- Enumeration of mus time: "
           << to_string(chrono::duration_cast<chrono::nanoseconds>(t5 - t4).count() / 1e9) << endl;
    }
    return;
  }

  if (uc) {
    t0 = chrono::high_resolution_clock::now();
    get_formulas(file, names, formulas, af);
    t1 = chrono::high_resolution_clock::now();
    cout << "-- Parsing of the file time: "
         << to_string(chrono::duration_cast<chrono::nanoseconds>(t1 - t0).count() / 1e9) << endl;
    if (file != stdin)
      fclose(file);
    if (print_weak_until_free || print_formula_and_continue) {
      auto n = names.begin();
      auto f = formulas.begin();
      // cout << af->to_string() << endl;

      for (; n != names.end();) {
        auto el  = *n;
        auto el1 = *f;
        cout << "" << el->to_string() << " := " << el1->to_string() << ";" << endl;
        n++;
        f++;
      }
      if (!print_formula_and_continue)
        return;
    }
  } else {
    af = aalta_formula(file, true).unique();
    if (file != stdin)
      fclose(file);
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
  // Simplify the formula
  af = af->simplify();
  // Pushes X over and/or operators
  af = af->split_next();
  t2 = chrono::high_resolution_clock::now();
  cout << "-- Preprocessing time: "
       << to_string(chrono::duration_cast<chrono::nanoseconds>(t2 - t1).count() / 1e9) << endl;

  // cout << af->to_string() << endl;

  if (blsc) {
    LTLfChecker checker(af, verbose, evidence);
    t3 = chrono::high_resolution_clock::now();
    cout << "-- Checker creation time: "
         << to_string(chrono::duration_cast<chrono::nanoseconds>(t3 - t2).count() / 1e9) << endl;
    if (uc) {
      checker.add_assumptions(names);
      t5 = chrono::high_resolution_clock::now();
      cout << "-- Checker add assumption time: "
           << to_string(chrono::duration_cast<chrono::nanoseconds>(t4 - t3).count() / 1e9) << endl;
    }
    bool res = checker.check();
    t6       = chrono::high_resolution_clock::now();
    cout << "-- Checker check time: "
         << to_string(chrono::duration_cast<chrono::nanoseconds>(t5 - t4).count() / 1e9) << endl;
    if (!uc) {
      cout << (res ? "sat" : "unsat") << endl;
    } else {
      cout << "-- The set of formulas is " << (res ? "sat" : "unsat") << endl;
    }
    if (evidence && res)
      checker.print_evidence();
    if (uc && !res) {
      cout << "-- unsat core:";
      checker.print_uc();
      cout << endl;
      cout << "-- unsat core size: " << checker.get_uc_size() << endl;
    }
  } else {
    CARChecker checker(af, verbose, evidence);
    t3 = chrono::high_resolution_clock::now();
    cout << "-- Checker creation time: "
         << to_string(chrono::duration_cast<chrono::nanoseconds>(t3 - t2).count() / 1e9) << endl;
    if (uc) {
      checker.add_assumptions(names);
      t4 = chrono::high_resolution_clock::now();
      cout << "-- Checker add assumption time: "
           << to_string(chrono::duration_cast<chrono::nanoseconds>(t4 - t3).count() / 1e9) << endl;
    }
    bool res = checker.check();
    t5       = chrono::high_resolution_clock::now();
    cout << "-- Checker check time: "
         << to_string(chrono::duration_cast<chrono::nanoseconds>(t5 - t4).count() / 1e9) << endl;
    if (!uc) {
      cout << (res ? "sat" : "unsat") << endl;
    } else {
      cout << "-- The set of formulas is " << (res ? "sat" : "unsat") << endl;
    }
    if (evidence && res)
      checker.print_evidence();
    if (uc && !res) {
      cout << "-- unsat core:";
      checker.print_uc();
      cout << endl;
      cout << "-- unsat core size: " << checker.get_uc_size() << endl;
      if (mus) {
        cout << "-- minimal unsatisfiable core:";
        checker.print_mus();
        cout << endl;
        cout << "-- minimal unsatisfiable core size: " << checker.get_mus_size() << endl;
      }
      if (emus) {
        cout << "-- enumeration of mus: " << endl;
        // DRAFT:
        // checker.add_assumptions(names, false);
        // checker.print_all_mus();
        // ffile = (char *)malloc(strlen(argv[2])+1);
        // sprintf(ffile, "%s", argv[2]);
        // file = fopen(ffile, "r");
        // aalta_formula* af;
        // af = aalta_formula::TAIL();
        // AaltaFormulaVec names2;
        // AaltaFormulaVec formulas2;
        // get_formulas(file, names2, formulas2, af, false, true);
        // af = af->nnf();
        // af = af->add_tail();
        // // Rewrites weak next with N f <-> Tail | X f
        // af = af->remove_wnext();
        // // Simplify the formula
        // af = af->simplify();
        // // Pushes X over and/or operators
        // af = af->split_next();
        // checker.add_assumptions(names, false);
        checker.print_all_mus();
      }
    }
  }
  aalta_formula::destroy();
  t6 = chrono::high_resolution_clock::now();
  cout << "-- Checker unsat core extraction time: "
       << to_string(chrono::duration_cast<chrono::nanoseconds>(t6 - t5).count() / 1e9) << endl;

  cout << "-- Checker total time: "
       << to_string(chrono::duration_cast<chrono::nanoseconds>(t6 - t0).count() / 1e9) << endl;
}

int main(int argc, char** argv) {
  ltlf_sat(argc, argv);
  return 0;
}
