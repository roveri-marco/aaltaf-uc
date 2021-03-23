/*
 * 定义AST转化函数
 * File:   trans.c
 * Author: yaoyinbo
 *
 * Created on October 20, 2013, 3:32 PM
 */
#include "trans.h"

#include <stdlib.h>

ltl_formula *
getAST (const char *input)
{
  char * str = (char *)malloc(strlen(input) + strlen("_ack_x :=  ;") + 1);
  ltl_formulas *formulas = allocate_ltl_formulas(1);
  ltl_formula * res;
  yyscan_t scanner;
  YY_BUFFER_STATE state;

  if (yylex_init (&scanner)) {
      // couldn't initialize
      return NULL;
  }

  sprintf(str, "_ack_x := %s ;", input);

  state = yy_scan_string (str, scanner);
  if (yyparse (&formulas, scanner)) {
    // error parsing
    return NULL;
  }

  yy_delete_buffer (state, scanner);
  yylex_destroy (scanner);

  res = formulas->formulas[0];
  free(formulas);
  return res;
}

ltl_formula *
getASTF(FILE * file)
{
  ltl_formulas *formulas = allocate_ltl_formulas(1);
  ltl_formula * res;
  yyscan_t scanner;
  YY_BUFFER_STATE state;

  if (yylex_init (&scanner)) {
    // couldn't initialize
    return NULL;
  }

  state = yy_create_buffer(file, YY_BUF_SIZE, scanner);

  yy_switch_to_buffer(state, scanner);

  if (yyparse(&formulas, scanner)) {
    // error parsing
    return NULL;
  }

  yy_delete_buffer(state, scanner);
  yylex_destroy (scanner);

  res = formulas->formulas[0];

  free(formulas);

  return res;
}

ltl_formulas *
getASTSF(FILE * file)
{
  ltl_formulas *formulas = allocate_ltl_formulas(10);
  yyscan_t scanner;
  YY_BUFFER_STATE state;

  if (yylex_init (&scanner)) {
    // couldn't initialize
    return NULL;
  }

  state = yy_create_buffer(file, YY_BUF_SIZE, scanner);

  yy_switch_to_buffer(state, scanner);

  if (yyparse(&formulas, scanner)) {
    // error parsing
    return NULL;
  }

  yy_delete_buffer(state, scanner);
  yylex_destroy (scanner);

  return formulas;
}
