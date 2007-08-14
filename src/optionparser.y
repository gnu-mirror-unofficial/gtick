%name-prefix="option_"
%{
/*
 * options file format parser
 *
 * This file is part of GTick
 *
 * Copyright 2003, 2004, 2005, 2006 Roland Stigge
 *
 * GTick is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * GTick is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GTick; if not, see <http://www.gnu.org/licenses/>.
 *
 */

/* regular GNU system includes */
#include <stdio.h>
#include <stdlib.h>

/* own header files */
#include "globals.h"
#include "option.h"

/* type for semantic values of symbols: malloc'ed strings */
#define YYSTYPE char*

/* let yyparse() accept one argument of type void* */
#define YYPARSE_PARAM option_list

int option_lex(void);
void option_error(const char *message);
%}

/* terminal symbols */
%token TOKEN_NAME
%token TOKEN_VALUE

%%

lines     :
	  | lines line
;

line      : TOKEN_NAME '=' TOKEN_VALUE
	      {
	        option_set(option_list, $1, $3);
		free($1);
		free($3);
	      }
	  | TOKEN_NAME '='
	      {
	        option_set(option_list, $1, "");
		free($1);
	      }
	  | error
	      {
	        fprintf(stderr, "Found error region from %d:%d up to %d:%d.\n",
	                @1.first_line, @1.first_column,
		        @1.last_line, @1.last_column);
	        $$ = $1;
	      }
;

%%

/*
 * callback for yyparse(), (also) called on errors (hopefully) handled
 * by error token actions in grammar, but not if errors occur to
 * often (bison needs 3 "correct" tokens to recover)
 */
void option_error(const char *message) {
  if (debug && option_nerrs == 1)
    fprintf(stderr,
	    "Warning: Parsing options file: first error at %d:%d: %s.\n",
	    option_lloc.first_line, option_lloc.first_column, message);
}

