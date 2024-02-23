/*
 * added by Jianwen LI on December 20th, 2014
 * translate ltlf formulas to LTL formulas
*/

namespace aalta {

};

#include "ltlf2Andltlf.h"
#include <stdlib.h>
#include <stdio.h>
#include <set>
#include <iostream>
#include <vector>
#include <iomanip>

using namespace std;
using namespace aalta;
#define MAXN 100000000


static std::vector<ltl_formula *> conjuncts;

void ltlf2Andltlf(ltl_formula *f) {
  if (f == NULL)
    return;
  ltl_formula *res;
  if (f->_var != NULL) {
    conjuncts.push_back(f);
  }
  else {
    switch (f->_type) {
    case eAND:
      ltlf2Andltlf(f->_right);
      ltlf2Andltlf(f->_left);
      break;
    default:
      // std::cout << to_string(f) << std::endl;
      conjuncts.push_back(f);
      break;
    }
  }
}


ltl_formula * ltlfRemoveW(ltl_formula *f) {
  if ((f == NULL) || (f->_var != NULL)) {
      return f;
  }
  else {
    ltl_formula * l = ltlfRemoveW(f->_left);
    ltl_formula * r = ltlfRemoveW(f->_right);

    if (f->_type == eWUNTIL) {
      return create_operation(eOR,
			      create_operation(eUNTIL, l, r),
			      create_operation(eGLOBALLY, NULL, l));
    } else {
      return create_operation(f->_type, l, r);
    }
  }
}


char in[MAXN];

int main (int argc, char ** argv) {
  bool tosmv = false;
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-s") == 0)
      tosmv = true;
    if (strcmp(argv[i], "-h") == 0) {
      std::cout << argv[0] << " [-h] [-s] [file] " << std::endl;
      std::cout << "\t-h\t Dumps this help and exit!" << std::endl;
      std::cout << "\t-s\t Dumps the formula in NuSMV format!" << std::endl;
      std::cout << "\tfile\t Reads the formula from file. If not specified it reads it from standard input!" << std::endl;
      exit(1);
    }
  }
  if (tosmv) argc--;

  if (argc == 1) {
    puts("please input the formula:");
    if (fgets(in, MAXN, stdin) == NULL) {
      printf("Error: read input!\n");
      exit(0);
    }
  }
  else {
    FILE * f = fopen(argv[tosmv ? 2 : 1], "r");
    if (f != NULL) {
      if (fgets(in, MAXN, f) == NULL) {
	printf("Error: read input!\n");
	exit(0);
      }
    } else {
      printf("Unable to open file \"%s\"\n", argv[1]);
      exit(0);
    }
  }

  // std::cout << in << std::endl;

  ltl_formula *root = getAST(in);
  ltlf2Andltlf(root);
  int c = 0;
  if (tosmv) {
    std::cout << "MODULE main" << std::endl;
    std::set<std::string> alphabet;
    for(auto i = conjuncts.begin(); i != conjuncts.end(); i++) {
      get_alphabet(alphabet, *i);
      *i = ltlfRemoveW(*i);
    }
    for(auto i = alphabet.begin(); i != alphabet.end(); i++) {
      std::cout << " VAR " << *i << " : boolean;" << std::endl;
    }
    std::cout << std::endl;
  }

  for(auto i = conjuncts.begin(); i != conjuncts.end(); i++) {
    if (tosmv) std::cout << "LTLSPEC NAME ";
    std::cout << "rr_r_" << std::setfill('0') << std::setw(5) << c++
	      << " := " << to_string(*i) << ";" << std::endl;
  }
  destroy_formula(root);
}
