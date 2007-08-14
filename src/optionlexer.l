%option prefix="option_"
%option nounput
%{
/*
 * options file format lexer
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

/* type for semantic values of symbols: malloc'ed strings */
#define YYSTYPE char*

/* GNU headers */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* GTK+ headers */
#include <glib.h>

/* own headers */
#include "globals.h"
#include "optionparser.h"
#include "optionlexer.h"

char* option_filename;
YYLTYPE option_lloc;

void option_locate();
%}

LETTER    [a-zA-Z_]
DIGIT     [0-9]

/* exclusive start condition for C-style comments */
%x _VALUE_

%%

"#"[^\n]*\n                  { /* eat up comment lines */
                               option_locate();
                             }
{LETTER}({LETTER}|{DIGIT})*  {
                               option_locate();
			       option_lval = strdup(option_text);
			       return TOKEN_NAME;
			     }
"="                          {
                               option_locate();
			       BEGIN(_VALUE_);
			       return *option_text;
			     }
<_VALUE_>[^ \t\n"]([^\n"]*[^ \t\n"])* {
                               option_locate();
			       option_lval = strdup(option_text);
			       return TOKEN_VALUE;
			     }
<_VALUE_>\"[^"\n]*\"           {
			       int len = strlen(option_text) - 2;
			       
                               option_locate();
			       option_lval = (char *)g_malloc(len + 1);
			       strncpy(option_lval, option_text + 1, len);
			       option_lval[len] = '\0';
			       return TOKEN_VALUE;
			     }
<_VALUE_>\n                  {
                               option_locate();
			       BEGIN(INITIAL);
			     }
<_VALUE_>.                   { /* eat up rest (whitespace & unknown) */
                               option_locate();
			     }
.|\n                         { /* eat up rest (whitespace & unknown) */
                               option_locate();
			     }

%%

/* For portability's sake */
int option_wrap() { return 1; }

/*
 * adjusts option_lloc according to option_text
 */
void option_locate() {
  char* temp;

  option_lloc.first_line = option_lloc.last_line;
  option_lloc.first_column = option_lloc.last_column;
  
  for (temp = yytext; *temp != '\0'; temp++) {
    if (*temp == '\n') {
      ++ option_lloc.last_line;
      option_lloc.last_column = 1;
    } else {
      ++ option_lloc.last_column;
    }
  }
}

/*
 * to be called directly before the parser run
 * sets up option_filename (to be free'd by option_lexer_deinit) on success
 *
 * on success, option_in contains valid FILE* handle, else NULL
 */
void option_lexer_init(char* filename) {
  option_lloc.first_line = 1;
  option_lloc.first_column = 1;
  option_lloc.last_line = 1;
  option_lloc.last_column = 1;
  if ((option_in = fopen(filename, "r"))) {
    option_filename = strdup(filename);
  }
}

/*
 * to be called directly after the parser run
 */
void option_lexer_deinit() {
  if (fclose(option_in) == EOF) {
    fprintf(stderr, "Warning: Couldn't close options file.\n");
  }
  free(option_filename);
}

