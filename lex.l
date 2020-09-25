%{
#include "zc.h"

#define YYSTYPE_IS_TRIVIAL 1
#define YYSTYPE_IS_DECLARED 1

yylval_type yylval;

enum tok last_tok;
int linenr = 0;
char *lex_filepath = NULL;

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
    yylval.linenr = loc_new(lex_filepath, linenr);

    // for some reason strdup doesn't work
    yylval.u.s = malloc(strlen(yytext) + 1);
    strcpy(yylval.u.s, yytext);

    return last_tok = FLOAT_NUM_TOK;
}

static bool in_comment = false;
%}

%option noyywrap
%option yylineno

%%

"sizeof" {
    if (!in_comment) {
	yylval.linenr = loc_new(lex_filepath, linenr);
	return last_tok = SIZEOF_TOK;
    }
}

"type" {
    if (!in_comment) {
	yylval.linenr = loc_new(lex_filepath, linenr);
	return last_tok = TYPE_TOK;
    }
}

"define" {
    if (!in_comment) {
	yylval.linenr = loc_new(lex_filepath, linenr);
	return last_tok = DEFINE_TOK;
    }
}

"include" {
    if (!in_comment) {
	yylval.linenr = loc_new(lex_filepath, linenr);
	return last_tok = INCLUDE_TOK;
    }
}

"as" {
    if (!in_comment) {
	yylval.linenr = loc_new(lex_filepath, linenr);
	return last_tok = AS_TOK;
    }
}


"if" {
    if (!in_comment) {
	yylval.linenr = loc_new(lex_filepath, linenr);
	return last_tok = IF_TOK;
    }
}

"else" {
    if (!in_comment) {
	yylval.linenr = loc_new(lex_filepath, linenr);
	return last_tok = ELSE_TOK;
    }
}

"switch" {
    if (!in_comment) {
	yylval.linenr = loc_new(lex_filepath, linenr);
	return last_tok = SWITCH_TOK;
    }
}

"case" {
    if (!in_comment) {
	yylval.linenr = loc_new(lex_filepath, linenr);
	return last_tok = CASE_TOK;
    }
}

"default" {
    if (!in_comment) {
	yylval.linenr = loc_new(lex_filepath, linenr);
	return last_tok = DEFAULT_TOK;
    }
}

"for" {
    if (!in_comment) {
	yylval.linenr = loc_new(lex_filepath, linenr);
	return last_tok = FOR_TOK;
    }
}


"return" {
    if (!in_comment) {
	yylval.linenr = loc_new(lex_filepath, linenr);
	return last_tok = RETURN_TOK;
    }
}

"goto" {
    if (!in_comment) {
	yylval.linenr = loc_new(lex_filepath, linenr);
	return last_tok = GOTO_TOK;
    }
}

"break" {
    if (!in_comment) {
	yylval.linenr = loc_new(lex_filepath, linenr);
	return last_tok = BREAK_TOK;
    }
}

"continue" {
    if (!in_comment) {
	yylval.linenr = loc_new(lex_filepath, linenr);
	return last_tok = CONTINUE_TOK;
    }
}

"fallthrough" {
    if (!in_comment) {
	yylval.linenr = loc_new(lex_filepath, linenr);
	return last_tok = FALLTHROUGH_TOK;
    }
}

"&&" {
    if (!in_comment) {
	yylval.linenr = loc_new(lex_filepath, linenr);
	return last_tok = ANDAND_TOK;
    }
}

"||" {
    if (!in_comment) {
	yylval.linenr = loc_new(lex_filepath, linenr);
	return last_tok = OROR_TOK;
    }
}

">=" {
    if (!in_comment) {
	yylval.linenr = loc_new(lex_filepath, linenr);
	return last_tok = GE_TOK;
    }
}

"<=" {
    if (!in_comment) {
	yylval.linenr = loc_new(lex_filepath, linenr);
	return last_tok = LE_TOK;
    }
}

"==" {
    if (!in_comment) {
	yylval.linenr = loc_new(lex_filepath, linenr);
	return last_tok = EQ_TOK;
    }
}

"!=" {
    if (!in_comment) {
	yylval.linenr = loc_new(lex_filepath, linenr);
	return last_tok = NE_TOK;
    }
}

"<<" {
    if (!in_comment) {
	yylval.linenr = loc_new(lex_filepath, linenr);
	return last_tok = SHL_TOK;
    }
}

">>" {
    if (!in_comment) {
	yylval.linenr = loc_new(lex_filepath, linenr);
	return last_tok = SHR_TOK;
    }
}

"+=" {
    if (!in_comment) {
	yylval.linenr = loc_new(lex_filepath, linenr);
	return last_tok = ADD_ASGN_TOK;
    }
}

"-=" {
    if (!in_comment) {
	yylval.linenr = loc_new(lex_filepath, linenr);
	return last_tok = SUB_ASGN_TOK;
    }
}

"*=" {
    if (!in_comment) {
	yylval.linenr = loc_new(lex_filepath, linenr);
	return last_tok = MUL_ASGN_TOK;
    }
}

"/=" {
    if (!in_comment) {
	yylval.linenr = loc_new(lex_filepath, linenr);
	return last_tok = DIV_ASGN_TOK;
    }
}

"%=" {
    if (!in_comment) {
	yylval.linenr = loc_new(lex_filepath, linenr);
	return last_tok = REM_ASGN_TOK;
    }
}

"<<=" {
    if (!in_comment) {
	yylval.linenr = loc_new(lex_filepath, linenr);
	return last_tok = SHL_ASGN_TOK;
    }
}

">>=" {
    if (!in_comment) {
	yylval.linenr = loc_new(lex_filepath, linenr);
	return last_tok = SHR_ASGN_TOK;
    }
}

"|=" {
    if (!in_comment) {
	yylval.linenr = loc_new(lex_filepath, linenr);
	return last_tok = OR_ASGN_TOK;
    }
}

"~=" {
    if (!in_comment) {
	yylval.linenr = loc_new(lex_filepath, linenr);
	return last_tok = XOR_ASGN_TOK;
    }
}

"&=" {
    if (!in_comment) {
	yylval.linenr = loc_new(lex_filepath, linenr);
	return last_tok = AND_ASGN_TOK;
    }
}

"--" {
    if (!in_comment) {
	yylval.linenr = loc_new(lex_filepath, linenr);
	return last_tok = DEC_TOK;
    }
}

"++" {
    if (!in_comment) {
	yylval.linenr = loc_new(lex_filepath, linenr);
	return last_tok = INC_TOK;
    }
}

"..." {
    if (!in_comment) {
	yylval.linenr = loc_new(lex_filepath, linenr);
	return last_tok = ELLIPSIS_TOK;
    }
}

\"(\\.|[^\\"])*\" {
    if (!in_comment) {
	yylval.linenr = loc_new(lex_filepath, linenr);

	size_t n_chars = strlen(yytext) - 2;

	// for some reason strdup doesn't work
	yylval.u.s = malloc(n_chars + 1);
	memcpy(yylval.u.s, yytext + 1, n_chars);
	yylval.u.s[n_chars] = '\0';

	return last_tok = STR_TOK;
    }
}

\'(\\.|[^\\'])\' {
    if (!in_comment) {
	yylval.linenr = loc_new(lex_filepath, linenr);

	size_t n_chars = strlen(yytext) - 2;

	// for some reason strdup doesn't work
	yylval.u.s = malloc(n_chars + 1);
	memcpy(yylval.u.s, yytext + 1, n_chars);
	yylval.u.s[n_chars] = '\0';

	return last_tok = CHAR_TOK;
    }
}

0[0-9]* {
    if (!in_comment) {
	yylval.linenr = loc_new(lex_filepath, linenr);

	/* octal base */
	char *p;
	yylval.u.i = strtoll(yytext + 1, &p, 8);
	if (*p != '\0')
	    fatal(yylval.linenr, "%s: not a valid octal number", yytext);
	return last_tok = NUM_TOK;
    }
}

0[xX][0-9a-fA-F]+ {
    if (!in_comment) {
	yylval.linenr = loc_new(lex_filepath, linenr);
	yylval.u.i = strtoll(yytext + 2, NULL, 16);
	return last_tok = NUM_TOK;
    }
}

[0-9]+ {
    if (!in_comment) {
	yylval.linenr = loc_new(lex_filepath, linenr);
	yylval.u.i = strtoll(yytext, NULL, 10);
	return last_tok = NUM_TOK;
    }
}

[a-zA-Z_][a-zA-Z_0-9]* {
    if (!in_comment) {
	yylval.linenr = loc_new(lex_filepath, linenr);

	// for some reason strdup doesn't work
	yylval.u.s = malloc(strlen(yytext) + 1);
	strcpy(yylval.u.s, yytext);

	return last_tok = IDENT_TOK;
    }
}

[0-9]+[eE][+-]?[0-9]+ {
    if (!in_comment) {
	return float_constant();
    }
}

[0-9]*"."[0-9]+([eE][+-]?[0-9]+)? {
    if (!in_comment) {
	return float_constant();
    }
}

[0-9]+"."[0-9]*([eE][+-]?[0-9]+)? {
    if (!in_comment) {
	return float_constant();
    }
}

\n {
    if (!in_comment) {
	yylval.linenr = loc_new(lex_filepath, linenr++);

	switch (last_tok) {
	    case IDENT_TOK:
	    case STR_TOK:
	    case CHAR_TOK:
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
}

"//".* {
    /* ignore */
}

"/*" {
    in_comment = true;
}

"*/" {
    in_comment = false;
}

[ \t\v\f] {
    /* ignore */
}

. {
    if (!in_comment) {
	yylval.linenr = loc_new(lex_filepath, linenr);
	return last_tok = *yytext;
    }
}

%%
