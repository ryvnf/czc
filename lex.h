#ifndef LEX_H
#define LEX_H

#define EXPAND_TOKENS(X) \
	X(EOF_TOK, 0) \
	X(SIZEOF_TOK, 256) \
	X(AS_TOK, 257) \
	X(IF_TOK, 258) \
	X(FOR_TOK, 261) \
	X(RETURN_TOK, 262) \
	X(GOTO_TOK, 263) \
	X(BREAK_TOK, 264) \
	X(CONTINUE_TOK, 265) \
	X(ANDAND_TOK, 266) \
	X(OROR_TOK, 267) \
	X(GE_TOK, 268) \
	X(LE_TOK, 269) \
	X(EQ_TOK, 270) \
	X(NE_TOK, 271) \
	X(SHL_TOK, 272) \
	X(SHR_TOK, 273) \
	X(ADD_ASGN_TOK, 274) \
	X(SUB_ASGN_TOK, 275) \
	X(MUL_ASGN_TOK, 276) \
	X(DIV_ASGN_TOK, 277) \
	X(REM_ASGN_TOK, 278) \
	X(SHL_ASGN_TOK, 279) \
	X(SHR_ASGN_TOK, 280) \
	X(OR_ASGN_TOK, 281) \
	X(XOR_ASGN_TOK, 282) \
	X(AND_ASGN_TOK, 283) \
	X(DEC_TOK, 284) \
	X(INC_TOK, 285) \
	X(STR_TOK, 286) \
	X(NUM_TOK, 287) \
	X(IDENT_TOK, 288) \
	X(ELSE_TOK, 289) \
	X(ELLIPSIS_TOK, 290) \
	X(TYPE_TOK, 291) \
        X(NULL_TOK, 292) \
        X(TRUE_TOK, 293) \
        X(FALSE_TOK, 294) \
        X(DEFINE_TOK, 295) \
        X(CHAR_TOK, 296) \
	X(FLOAT_NUM_TOK, 297) \
	X(CASE_TOK, 298) \
	X(DEFAULT_TOK, 299) \
	X(FALLTHROUGH_TOK, 300) \
	X(SWITCH_TOK, 301) \
	X(INCLUDE_TOK, 302)

enum tok {
#define member(name, val) name = val,
	EXPAND_TOKENS(member)
#undef member
};

typedef struct yylval_type yylval_type;
struct yylval_type {
    struct loc *linenr;
    union {
        long long i;
        char *s;
        double f;
    } u;
};

extern yylval_type yylval;
extern yylval_type lval;

extern FILE *yyin;

int yylex(void);
int yylex_destroy(void);

extern char *lex_filepath;
extern int linenr;

const char *tok_name(enum tok tok);

#endif /* !defined LEX_H */
