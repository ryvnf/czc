%{
#include "zc.h"

# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1

yylval_type yylval;

enum tok last_tok;
size_t linenr = 1;

const char *tok_name(enum tok tok) {
	switch (tok) {
#define tok_case(name, val)  \
		case name: \
			   return #name;
		EXPAND_TOKENS(tok_case);
#undef tok_case
	}
	return NULL;
}

enum tok float_constant(void)
{
  yylval.linenr = linenr;

  // for some reason strdup doesn't work
  yylval.u.s = malloc(strlen(yytext) + 1);
  strcpy(yylval.u.s, yytext);

  return last_tok = FLOAT_NUM_TOK;
}

%}

%option noyywrap
%option yylineno

%%

"sizeof" {
  yylval.linenr = linenr;
  return last_tok = SIZEOF_TOK;
}

"type" {
  yylval.linenr = linenr;
  return last_tok = TYPE_TOK;
}

"define" {
  yylval.linenr = linenr;
  return last_tok = DEFINE_TOK;
}

"as" {
  yylval.linenr = linenr;
  return last_tok = AS_TOK;
}


"if" {
  yylval.linenr = linenr;
  return last_tok = IF_TOK;
}

"else" {
  yylval.linenr = linenr;
  return last_tok = ELSE_TOK;
}

"switch" {
  yylval.linenr = linenr;
  return last_tok = SWITCH_TOK;
}

"case" {
  yylval.linenr = linenr;
  return last_tok = CASE_TOK;
}

"default" {
  yylval.linenr = linenr;
  return last_tok = DEFAULT_TOK;
}

"while" {
  yylval.linenr = linenr;
  return last_tok = WHILE_TOK;
}


"for" {
  yylval.linenr = linenr;
  return last_tok = FOR_TOK;
}


"return" {
  yylval.linenr = linenr;
  return last_tok = RETURN_TOK;
}

"goto" {
  yylval.linenr = linenr;
  return last_tok = GOTO_TOK;
}

"break" {
  yylval.linenr = linenr;
  return last_tok = BREAK_TOK;
}

"continue" {
  yylval.linenr = linenr;
  return last_tok = CONTINUE_TOK;
}

"fallthrough" {
  yylval.linenr = linenr;
  return last_tok = FALLTHROUGH_TOK;
}

"&&" {
  yylval.linenr = linenr;
  return last_tok = ANDAND_TOK;
}

"||" {
  yylval.linenr = linenr;
  return last_tok = OROR_TOK;
}

">=" {
  yylval.linenr = linenr;
  return last_tok = GE_TOK;
}

"<=" {
  yylval.linenr = linenr;
  return last_tok = LE_TOK;
}

"==" {
  yylval.linenr = linenr;
  return last_tok = EQ_TOK;
}

"!=" {
  yylval.linenr = linenr;
  return last_tok = NE_TOK;
}

"<<" {
  yylval.linenr = linenr;
  return last_tok = SHL_TOK;
}

">>" {
  yylval.linenr = linenr;
  return last_tok = SHR_TOK;
}

"+=" {
  yylval.linenr = linenr;
  return last_tok = ADD_ASGN_TOK;
}

"-=" {
  yylval.linenr = linenr;
  return last_tok = SUB_ASGN_TOK;
}

"*=" {
  yylval.linenr = linenr;
  return last_tok = MUL_ASGN_TOK;
}

"/=" {
  yylval.linenr = linenr;
  return last_tok = DIV_ASGN_TOK;
}

"%=" {
  yylval.linenr = linenr;
  return last_tok = REM_ASGN_TOK;
}

"<<=" {
  yylval.linenr = linenr;
  return last_tok = SHL_ASGN_TOK;
}

">>=" {
  yylval.linenr = linenr;
  return last_tok = SHR_ASGN_TOK;
}

"|=" {
  yylval.linenr = linenr;
  return last_tok = OR_ASGN_TOK;
}

"~=" {
  yylval.linenr = linenr;
  return last_tok = XOR_ASGN_TOK;
}

"&=" {
  yylval.linenr = linenr;
  return last_tok = AND_ASGN_TOK;
}

"--" {
  yylval.linenr = linenr;
  return last_tok = DEC_TOK;
}

"++" {
  yylval.linenr = linenr;
  return last_tok = INC_TOK;
}

"..." {
  yylval.linenr = linenr;
  return last_tok = ELLIPSIS_TOK;
}

\"(\\.|[^\\"])*\" {
  yylval.linenr = linenr;

  size_t n_chars = strlen(yytext) - 2;

  // for some reason strdup doesn't work
  yylval.u.s = malloc(n_chars + 1);
  memcpy(yylval.u.s, yytext + 1, n_chars);
  yylval.u.s[n_chars] = '\0';

  return last_tok = STR_TOK;
}

\'(\\.|[^\\'])\' {
  yylval.linenr = linenr;

  size_t n_chars = strlen(yytext) - 2;

  // for some reason strdup doesn't work
  yylval.u.s = malloc(n_chars + 1);
  memcpy(yylval.u.s, yytext + 1, n_chars);
  yylval.u.s[n_chars] = '\0';

  return last_tok = CHAR_TOK;
}

[0-9]+ {
  yylval.linenr = linenr;
  yylval.u.i = atoi(yytext);
  return last_tok = NUM_TOK;
}

[a-zA-Z_][a-zA-Z_0-9]* {
  yylval.linenr = linenr;

  // for some reason strdup doesn't work
  yylval.u.s = malloc(strlen(yytext) + 1);
  strcpy(yylval.u.s, yytext);

  return last_tok = IDENT_TOK;
}

[0-9]+[eE][+-]?[0-9]+ {
  return float_constant();
}

[0-9]*"."[0-9]+([eE][+-]?[0-9]+)? {
  return float_constant();
}

[0-9]+"."[0-9]*([eE][+-]?[0-9]+)? {
  return float_constant();
}

\n {
  yylval.linenr = linenr++;

  switch (last_tok) {
    case IDENT_TOK:
    case STR_TOK:
    case NUM_TOK:
    case FLOAT_NUM_TOK:
    case DEC_TOK:
    case INC_TOK:
    case '^':
    case ')':
    case ']':
    case '}':
    case RETURN_TOK:
    case BREAK_TOK:
    case CONTINUE_TOK:
      return last_tok = ';';
  }
}

[ \t\v\f] {
  /* ignore */
}

. {
  yylval.linenr = linenr;
  return last_tok = *yytext;
}

%%
