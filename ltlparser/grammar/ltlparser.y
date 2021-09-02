%{
/*
 * ltlparser.y file
 * To generate the parser run: "bison ltlparser.y"
 */

#include "ltl_formula.h"
#define YYERROR_VERBOSE 1
#include "ltlparser.h"
#include "ltllexer.h"

int yyerror(ltl_formulas **formulas, yyscan_t scanner, const char *msg) {
  extern char * yytext;
  fprintf (stderr, "\033[31mERROR\033[0m: %s\n", msg);
  // exit(1);
  return 0;
}

%}

%code requires {
#ifndef YY_TYPEDEF_YY_SCANNER_T
#define YY_TYPEDEF_YY_SCANNER_T
typedef void* yyscan_t;
#endif
}

%output  "../ltlparser.c"
%defines "../ltlparser.h"

%define api.pure
%lex-param   { yyscan_t scanner }
%parse-param { ltl_formulas **formulas }
%parse-param { yyscan_t scanner }

%union {
  char var_name[200]; // To avoid allocating a string and/or referring
		      // to yytext
  ltl_formula *formula;
}

%left TOKEN_EQUIV
%left TOKEN_IMPLIES
%left TOKEN_OR
%left TOKEN_AND
%left TOKEN_RELEASE
%left TOKEN_UNTIL
%left TOKEN_WEAK_UNTIL
%left TOKEN_SINCE
%left TOKEN_TRIGGER
%right TOKEN_FUTURE
%right TOKEN_GLOBALLY
%right TOKEN_NEXT
%right TOKEN_WEAK_NEXT
%right TOKEN_NOT
%right TOKEN_YESTERDAY
%right TOKEN_ZYESTERDAY
%right TOKEN_ONCE
%right TOKEN_HISTORICALLY

%token TOKEN_TRUE
%token TOKEN_FALSE
%token TOKEN_LPAREN
%token TOKEN_RPAREN
%token <var_name> TOKEN_VARIABLE
%token TOKEN_EQBYDEF
%token TOKEN_SEMI

%type <formula> expr
%type <formula> conjunction

%%

input : conjunction { };

conjunction : TOKEN_VARIABLE TOKEN_EQBYDEF expr TOKEN_SEMI
              { push_ltlformula(*formulas, create_var($1), $3); $$ = $3; }
            | conjunction TOKEN_VARIABLE TOKEN_EQBYDEF expr TOKEN_SEMI
	      { push_ltlformula(*formulas, create_var($2), $4); $$ = $4; }
            ;

expr
	: TOKEN_VARIABLE	{ $$ = create_var($1);	}
	| TOKEN_TRUE		{ $$ = create_operation( eTRUE, NULL, NULL );	}
	| TOKEN_FALSE		{ $$ = create_operation( eFALSE, NULL, NULL );	}
	| TOKEN_LPAREN expr TOKEN_RPAREN { $$ = $2;	}
	| expr TOKEN_EQUIV expr		{ $$ = create_operation( eEQUIV, $1, $3 );		}
	| expr TOKEN_IMPLIES expr	{ $$ = create_operation( eIMPLIES, $1, $3 );	}
	| expr TOKEN_OR expr		{ $$ = create_operation( eOR, $1, $3 );			}
	| expr TOKEN_AND expr		{ $$ = create_operation( eAND, $1, $3 );		}
	| expr TOKEN_RELEASE expr	{ $$ = create_operation( eRELEASE, $1, $3 );	}
	| expr TOKEN_UNTIL expr		{ $$ = create_operation( eUNTIL, $1, $3 );		}
	| expr TOKEN_WEAK_UNTIL expr	{ $$ = create_operation( eWUNTIL, $1, $3 );		}
	| expr TOKEN_SINCE expr		{ $$ = create_operation( eSINCE, $1, $3 );		}
	| expr TOKEN_TRIGGER expr	{ $$ = create_operation( eTRIGGER, $1, $3 );		}
	| TOKEN_FUTURE expr		{ $$ = create_operation( eFUTURE, NULL, $2 );	}
	| TOKEN_GLOBALLY expr		{ $$ = create_operation( eGLOBALLY, NULL, $2 );	}
	| TOKEN_NEXT expr		{ $$ = create_operation( eNEXT, NULL, $2 );		}
	| TOKEN_WEAK_NEXT expr		{ $$ = create_operation( eWNEXT, NULL, $2 );		}
	| TOKEN_NOT expr		{ $$ = create_operation( eNOT, NULL, $2 );		}
	| TOKEN_YESTERDAY expr		{ $$ = create_operation( eYESTERDAY, NULL, $2 );		}
	| TOKEN_ZYESTERDAY expr		{ $$ = create_operation( eZYESTERDAY, NULL, $2 );		}
	| TOKEN_HISTORICALLY expr	{ $$ = create_operation( eHISTORICALLY, NULL, $2 );		}
	| TOKEN_ONCE expr		{ $$ = create_operation( eONCE, NULL, $2 );		}
	;

%%
