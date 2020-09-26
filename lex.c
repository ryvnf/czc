#include "zc.h"

yylval_type yylval;
static int last_tok = 0;
char *lex_filepath;
int linenr = 0;
FILE *yyin = NULL;

struct {
    char *str;
    int tok;
} keywords[] = {
    { "as", AS_TOK },
    { "break", BREAK_TOK },
    { "case", CASE_TOK },
    { "continue", CONTINUE_TOK },
    { "default", DEFAULT_TOK },
    { "define", DEFINE_TOK },
    { "else", ELSE_TOK },
    { "fallthrough", FALLTHROUGH_TOK },
    { "false", FALSE_TOK },
    { "for", FOR_TOK },
    { "goto", GOTO_TOK },
    { "if", IF_TOK },
    { "include", INCLUDE_TOK },
    { "nil", NULL_TOK },
    { "return", RETURN_TOK },
    { "sizeof", SIZEOF_TOK },
    { "switch", SWITCH_TOK },
    { "true", TRUE_TOK },
    { "type", TYPE_TOK },
};

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

static void appendc(char **s, size_t *n, int c) {
    *s = realloc(*s, *n + 1);
    (*s)[*n] = c;
    (*n)++;
}

bool isident(int c)
{
    return isalpha(c) || c == '_';
}

bool isidnum(int c)
{
    return isalnum(c) || c == '_';
}

int hexdigit(int *val, int c)
{
    if (c >= '0' && c <= '9') {
        return *val = *val * 16 + c - '0';
    } else if (c >= 'a' && c <= 'f') {
        return *val = *val * 16 + c - 'a' + 10;
    } else if (c >= 'A' && c <= 'F') {
        return *val = *val * 16 + c - 'A' + 10;
    }
    return -1;
}

int octdigit(int *val, int c)
{
    if (c >= '0' && c <= '7') {
        return *val = *val * 8 + c - '0';
    }
    return -1;
}

int lex_getc() {
    int c = getc(yyin);
    if (c == '\n')
        linenr++;
    return c;
}

int lex_ungetc(int c) {
    if (c == '\n')
        linenr--;
    return ungetc(c, yyin);
}

int esc_char(int quote, FILE *fp)
{
    int c;
    while ((c = lex_getc()) != EOF) {
        if (c == '\n') {
            fatal(loc_new(lex_filepath, linenr), "missing terminating %c",
                    quote);
            return EOF;
        }
        if (c == quote) {
            return EOF;
        }

        if (c == '\\') {
            c = lex_getc();
            if (c == 'x') {
                int res = 0;
                if (hexdigit(&res, c = lex_getc()) < 0) {
                    lex_ungetc(c);
                    fatal(loc_new(lex_filepath, linenr),
                            "missing hex digit after \\x");
                    return 0;
                }
                if (hexdigit(&res, c = lex_getc()) < 0) {
                    lex_ungetc(c);
                }
                return res;
            }

            if (c >= '0' && c <= '7') {
                int res = 0;
                octdigit(&res, c);
                for (int i = 0; i < 2; i++) {
                    if (octdigit(&res, c = lex_getc()) < 0) {
                        lex_ungetc(c);
                        break;
                    }
                }
                return res;
            }

            switch (c) {
                case 'a': return '\n';
                case 'b': return '\b';
                case 'f': return '\f';
                case 'n': return '\n';
                case 't': return '\t';
                case 'v': return '\v';
                case '\\':
                case '"':
                case '\'':
                          return c;
            }

            fprintf(stderr, "unknown escape \\%c\n", c);
            return c;
        }

        return c;
    }
    return EOF;
}


#define RETURN(TOK) do { \
    yylval.linenr = loc_new(lex_filepath, linenr); \
    return last_tok = (TOK); \
} while (false)

void skip_line_comment(void)
{
    int c;
    while ((c = lex_getc()) != EOF) {
        if (c == '\n')
            return;
    }
}

void skip_block_comment(void)
{
    int prev_c = '\0';
    int c;
    while ((c = lex_getc()) != EOF) {
        if (c == '/' && prev_c == '*') {
            return;
        }
        prev_c = c;
    }
}

int yylex(void)
{
    int c;
    while ((c = lex_getc()) != EOF) {
        if (isspace(c)) {
            continue;
        }

        if (isident(c)) {
            size_t n = 0;
            char *s = malloc(n);
            appendc(&s, &n, c);

            while (isidnum(c = lex_getc())) {
                appendc(&s, &n, c);
            }
            appendc(&s, &n, '\0');
            lex_ungetc(c);

            for (int i = 0; i < ARRAY_LEN(keywords); i++) {
                if (strcmp(s, keywords[i].str) == 0) {
                    free(s);
                    RETURN(keywords[i].tok);
                }
            }

            yylval.u.s = s;
            RETURN(IDENT_TOK);
        }

        if (isdigit(c)) {
            size_t n = 0;
            char *s = malloc(n);
            appendc(&s, &n, c);

            while (isidnum(c = lex_getc())) {
                appendc(&s, &n, c);
            }
            appendc(&s, &n, '\0');
            lex_ungetc(c);

            char *p;
            long long res = strtoll(s, &p, 0);
            if (*p != '\0') {
                error(loc_new(lex_filepath, linenr),
                        "not a valid number %s\n", s);
            }

            free(s);
            yylval.u.i = res;
            RETURN(NUM_TOK);
        }

        if (c == '\'' || c == '"') {
            int quote = c;
            size_t n = 0;
            char *s = malloc(n);

            while ((c = esc_char(quote, yyin)) != EOF) {
                appendc(&s, &n, c);
            }

            if (quote == '"') {
                appendc(&s, &n, '\0');
                yylval.u.chars.s = s;
                yylval.u.chars.n = n;
                RETURN(STR_TOK);
            }

            if (n == 0) {
                fprintf(stderr, "empty character constant\n");
                yylval.u.i = 0;
            } else {
                if (n > 1) {
                    fatal(loc_new(lex_filepath, linenr),
                            "more than one character in character constant\n");
                }
                yylval.u.i = s[0];
            }

            free(s);
            RETURN(NUM_TOK);
        }

        switch (c) {
            case '*':
            case '%':
            case '~':
            case '=':
            case '!': {
                int c2 = lex_getc();
                if (c2 == '=') {
                    switch (c) {
                        case '*': RETURN(MUL_ASGN_TOK);
                        case '%': RETURN(REM_ASGN_TOK);
                        case '~': RETURN(XOR_ASGN_TOK);
                        case '=': RETURN(EQ_TOK);
                        case '!': RETURN(NE_TOK);
                    }
                    unreachable();
                }
                lex_ungetc(c2);
                break;
            }

            case '<':
            case '>': {
                int c2 = lex_getc();
                if (c2 == c) {
                    int c3 = lex_getc();
                    if (c3  == '=') {
                        switch (c) {
                            case '<': RETURN(SHL_ASGN_TOK);
                            case '>': RETURN(SHR_ASGN_TOK);
                        }
                    }
                    lex_ungetc(c3);

                    switch (c) {
                        case '<': RETURN(SHL_TOK);
                        case '>': RETURN(SHR_TOK);
                    }
                } else if (c2 == '=') {
                    switch (c) {
                        case '<': RETURN(LE_TOK);
                        case '>': RETURN(GE_TOK);
                    }
                }
                lex_ungetc(c2);
                break;
            }

            case '+':
            case '-':
            case '|':
            case '&': {
                int c2 = lex_getc();
                if (c2 == '=') {
                    switch (c) {
                        case '+': RETURN(ADD_ASGN_TOK);
                        case '-': RETURN(SUB_ASGN_TOK);
                        case '|': RETURN(OR_ASGN_TOK);
                        case '&': RETURN(AND_ASGN_TOK);
                    }
                } else if (c2 == c) {
                    switch (c) {
                        case '+': RETURN(INC_TOK);
                        case '-': RETURN(DEC_TOK);
                        case '|': RETURN(OROR_TOK);
                        case '&': RETURN(ANDAND_TOK);
                    }
                }
                lex_ungetc(c2);
                break;
            }

            case '.': {
                int c2 = lex_getc();
                if (c2 == '.') {
                    int c3 = lex_getc();
                    if (c3 == '.') {
                        RETURN(ELLIPSIS_TOK);
                    }
                    lex_ungetc(c3);

                    RETURN(STRCAT_TOK);
                }

                lex_ungetc(c2);
                break;
            }

            case '/': {
                int c2 = lex_getc();
                if (c2 == '=') {
                    RETURN(DIV_ASGN_TOK);
                } else if (c2 == '/') {
                    skip_line_comment();
                    continue;
                } else if (c2 == '*') {
                    skip_block_comment();
                    continue;
                }
                lex_ungetc(c2);
                break;
            }
        }

        RETURN(c);
    }

    RETURN(EOF_TOK);
}
