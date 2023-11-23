/*
 * added by Jianwen LI on December 20th, 2014
 * translate ltlf formulas to LTL formulas
*/

#include "ltlf2ltlf.h"

#include <stdlib.h>
#include <stdio.h>
#include <set>
#include <string>
#include <iostream>     // std::cout
#include <fstream>      // std::ifstream
#include <sstream>      // std::stringstream

using namespace std;

std::string to_stringA (ltl_formula *root)
{
  std::string res = "";
  if (root == NULL)
    return res;
  if (root->_var != NULL)
    res = std::string (root->_var);
  else if (root->_type == eTRUE)
    res = std::string ("TRUE");
  else if (root->_type == eFALSE)
    res = std::string ("FALSE");
  else
    {
      res += "(";
      res += to_stringA (root->_left);
      switch (root->_type)
        {
        case eNOT:
          res += "! ";
          break;
        case eNEXT:
          res += "<X> ";
          break;
        case eGLOBALLY:
          res += "<G> ";
          break;
        case eFUTURE:
          res += "<F> ";
          break;
        case eUNTIL:
          res += " <U> ";
          break;
        case eAND:
          res += " && ";
          break;
        case eOR:
          res += " || ";
          break;
        case eIMPLIES:
          res += " => ";
          break;
        case eEQUIV:
          res += " <=> ";
          break;
        default:
          fprintf (stderr, "Error formula!");
          exit (0);
        }
      res += to_stringA (root->_right);
      res += ")";
    }
  return res;
}

static void usage(const char * name) {
    std::cout << "Usage: " << name << "[-a] filename" << std::endl;
    std::cout << "\t -a: is optional and enables the output in ASP format, otherwise in the aaltaf_uc format" << std::endl;
    exit(1);
}


int main (int argc, char ** argv)
{
  bool to_asp = false;
  int file = 1;
  if (argc == 3) {
    if (strcmp("-a", argv[1]) == 0) {
      to_asp = true;
      file = 2;
    }
    else {
      usage(argv[0]);
    }
  }
  if (argc < 2 || argc > 3) {
    usage(argv[0]);
  }
  FILE * f = fopen(argv[file], "r");
  if (NULL == f) {
    std::cout << "Unable to open file \"" << argv[file] << "\"" << std::endl;
    exit(1);
  }
  ltl_formulas * r = getASTSF(f);
  fclose(f);

  for (int i = 0; i < r->size; i++) {
    if (to_asp) {
      std::cout << to_stringA(r->formulas[i]) << std::endl;
    } else {
      std::cout << to_string(r->names[i]) << " := " << to_string(r->formulas[i]) << " ;" << std::endl;
    }
  }
  free_ltl_formulas(r);
}
