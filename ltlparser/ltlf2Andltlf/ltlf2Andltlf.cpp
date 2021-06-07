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
#define MAXN 1000000


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
      std::cout << to_string(f) << std::endl;
      conjuncts.push_back(f);
      break;
    }
  }
}


char in[MAXN];

int main (int argc, char ** argv) {
  if (argc == 1) {
    puts("please input the formula:");
    if (fgets(in, MAXN, stdin) == NULL) {
      printf("Error: read input!\n");
      exit(0);
    }
  }
  else {
    FILE * f = fopen(argv[1], "r");
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

  ltl_formula *root = getAST(in);
  ltlf2Andltlf(root);
  int c = 0;
  for(auto i = conjuncts.begin(); i != conjuncts.end(); i++) {
    std::cout << "_r_" << std::setfill('0') << std::setw(5) << c++ << " : "
	      << to_string(*i) << ";" << std::endl;
  }
  destroy_formula(root);
}
