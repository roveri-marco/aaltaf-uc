/*
 * added by Jianwen LI on December 20th, 2014
 * translate ltlf formulas to LTL formulas
*/

#ifndef LTLF_2_LTL_H
#define LTLF_2_LTL_H

#include "ltl_formula.h"
#include "trans.h"
#include "utility.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <set>

/*
 * translate the input ltlf formula to its equ-satisfiable ltl formula
*/
void ltlf2Andltlf(ltl_formula *);


#endif
