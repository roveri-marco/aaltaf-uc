#include "formula/aalta_formula.h"
#include "ltlfchecker.h"
#include "carchecker.h"
#include "solver.h"
#include <stdio.h>
#include <string.h>

#define MAXN 100000
char in[MAXN];

using namespace aalta;

void print_help() {
  cout << "usage: ./aalta_f [-e|-v|-blsc|-t|-h] [-f file] [\"formula\"]" << endl;
  cout << "\t -f\t:\t Read formulas from file" << endl;
  cout << "\t -e\t:\t print example when result is SAT" << endl;
  cout << "\t -u\t:\t computes the unsat core w.r.t. the conjunction of the input formulas" << endl;
  cout << "\t -v\t:\t print verbose details" << endl;
  cout << "\t -blsc\t:\t use the BLSC checking method; Default is CDLSC" << endl;
  cout << "\t -t\t:\t print weak until formula" << endl;
  cout << "\t -h\t:\t print help information" << endl;
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
    else if (strcmp (argv[i], "-h") == 0) {
      print_help ();
      exit (0);
    }
    else if (strcmp(argv[i], "-f") == 0) {
      if (i + 1 < argc) {
	ffile = (char *)malloc(strlen(argv[i+1])+1);
	sprintf(ffile, "%s", argv[i+1]);
	file = fopen(ffile, "r");
	i++;
	if (NULL == file) {
	  printf("Unable to open file %s\n", ffile);
	  free(ffile);
	}
      }
      else {
	print_help();
	exit(1);
      }
    }
    else {
      print_help();
      exit(1);
    }
  }

  if ((NULL == ffile) && (NULL == file)) {
    puts("please input the formula:");
    file = stdin;
  }

  aalta_formula* af;
  //set tail id to be 1
  af = aalta_formula::TAIL();

  if (uc) {

  }
  else {
    af = aalta_formula(file, true).unique();
    if (file != stdin) fclose(file);
    if (print_weak_until_free) {
      cout << af->to_string() << endl;
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
    bool res = checker.check ();
    printf ("%s\n", res ? "sat" : "unsat");
    if (evidence && res)
      checker.print_evidence ();
  }
  else {
    CARChecker checker (af, verbose, evidence);
    bool res = checker.check ();
    printf ("%s\n", res ? "sat" : "unsat");
    if (evidence && res)
      checker.print_evidence ();
  }
  aalta_formula::destroy();
}


int
main (int argc, char** argv)
{
  ltlf_sat (argc, argv);
  return 0;
}
