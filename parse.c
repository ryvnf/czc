#include "zc.h"

#define MAX_N_TOKENS 8192

struct parse {
    struct token {
        enum tok type;
        yylval_type val;
    } *tokens;
    struct strmap *included_files;
    int n_tokens;
    int pos;
    int nest_include_level;
};

enum {
    COMMA_PREC,     // ,
    ASGN_PREC,      // = += -= *= /= <<= &= etc.
    COND_PREC,      // ?:
    TRUTH_OR_PREC,  // ||
    TRUTH_AND_PREC, // &&
    EQ_PREC,        // == !=
    REL_PREC,       // < > <= >=
    BIT_OR_PREC,    // |
    BIT_XOR_PREC,   // ~
    BIT_AND_PREC,   // &
    SHIFT_PREC,     // << >>
    ADD_PREC,       // + -
    MUL_PREC,       // * / %
    MEMBER_PREC     // .
};

struct ast *parse_with_include_map(const char *path, struct strmap
        *included_files, int nest_include_level);
static struct ast *parse_binop_expr(struct parse *parse, int prec);
static struct ast *parse_init(struct parse *parse);
static struct ast *parse_expr(struct parse *parse);
static struct ast *parse_type(struct parse *parse);
static struct ast *parse_block(struct parse *parse);
static struct ast *parse_stmt_list(struct parse *parse);
static struct ast *parse_stmt(struct parse *parse);

static struct parse *tokenize(const char *filepath)
{
    FILE *fp = fopen(filepath, "r"); 
    if (!fp) {
        perror(filepath);
        exit(1);
    }

    lex_filepath = strdup(filepath);
    linenr = 1;
    yyin = fp;

    struct parse *parse = malloc(sizeof *parse);
    parse->tokens = malloc(MAX_N_TOKENS * sizeof *parse->tokens);
    parse->pos = 0;
    parse->n_tokens = 0;

    for (;;) {
        if (parse->n_tokens >= MAX_N_TOKENS) {
            fprintf(stderr, "error: too many tokens\n");
            exit(1);
        }
        parse->tokens[parse->n_tokens].type = yylex();
        parse->tokens[parse->n_tokens].val = yylval;
        if (parse->tokens[parse->n_tokens++].type == EOF_TOK) {
            break;
        }
    }
    yylex_destroy();
    fclose(fp);

    return parse;
}

// Get current line number.
static struct loc *get_linenr(struct parse *parse)
{
    return parse->tokens[parse->pos].val.linenr;
}

// Get reference to the current token but do not consume it.
static struct token *peek_tok(struct parse *parse)
{
    return &parse->tokens[parse->pos];
}

// Get reference to the current token and consume it.
static struct token *get_tok(struct parse *parse)
{
    return &parse->tokens[parse->pos++];
}

// Delete parser context information.
static void parse_del(struct parse *parse)
{
    for (size_t i = 0; i < parse->n_tokens; ++i) {
        switch (parse->tokens[i].type) {
            case IDENT_TOK:
            case STR_TOK:
                free(parse->tokens[i].val.u.s);
        }
    }
    if (parse->nest_include_level == 0)
        strmap_del(parse->included_files, NULL);
    free(parse->tokens);
    free(parse);
}

// Print syntax error message.
static void syntax_error(struct parse *parse)
{
    struct loc *l = loc_ref(get_linenr(parse));
    fatal(l, "invalid syntax");
    loc_unref(l);
}

// Expect a token of type tok at the current parsing position.
static bool expect(struct parse *parse, enum tok tok)
{
    if (peek_tok(parse)->type == tok) {
        parse->pos++;
        return true;
    }
    return false;
}

// Parse something enclosed in tokens, like an parenthesised expression.
static struct ast *parse_enclosed(struct parse *parse, enum tok open_delim, 
        struct ast *(*parse_syntax)(struct parse *), char close_delim,
        bool report_err)
{
    if (!expect(parse, open_delim))
        goto err;

    struct ast *ast = parse_syntax(parse);
    if (ast == NULL)
        goto err;

    if (!expect(parse, close_delim)) {
        ast_unref(ast);
        goto err;
    }

    return ast;

err:
    if (report_err)
        syntax_error(parse);
    return NULL;
}

// Parse a list of things delimited by a token, like a list of function
// arguments.
static struct ast_list *parse_list(struct parse *parse, enum tok delim,
        struct ast *(*parse_syntax)(struct parse *))
{
    size_t n_asts = 0;
    struct ast_list *head = NULL;
    struct ast_list **tailp = &head;

    for (;;) {
        int pos = parse->pos;
        struct ast *ast = parse_syntax(parse);
        if (ast == NULL) {
            parse->pos = pos;
            break;
        }

        *tailp = ast_list_new(ast);
        (*tailp)->ast = ast;

        n_asts++;
        tailp = &(*tailp)->next;

        if (peek_tok(parse)->type != delim)
            break;

        parse->pos++;
    }

    return head;
}

// Parse identifier at current position to a name AST node.
static struct ast *parse_name(struct parse *parse)
{
    if (peek_tok(parse)->type != IDENT_TOK)
	return NULL;

    struct loc *line = get_linenr(parse);
    return ast_new_s(line, NAME, strdup(get_tok(parse)->val.u.s));
}

// type_def : 'type' name type
static struct ast *parse_type_def(struct parse *parse)
{
    struct loc *line = get_linenr(parse);
    expect(parse, TYPE_TOK);

    struct ast *name = parse_name(parse);
    if (name == NULL)
	return NULL;

    struct ast *type = parse_type(parse);
    // If we cannot parse the type (and type becomes NULL), then it is a type
    // declaration to an incomplete type.

    return ast_new_ast(line, TYPE_DEF, 2, name, type);
}

// alias_def : 'define' ident expr
static struct ast *parse_alias_def(struct parse *parse)
{
    if (!expect(parse, DEFINE_TOK))
        return NULL;

    struct loc *line = get_linenr(parse);
    struct ast *name = parse_name(parse);
    if (name == NULL)
        return NULL;

    struct ast *expr = parse_expr(parse);
    if (expr == NULL)
        return NULL;

    return ast_new_ast(line, ALIAS_DEF, 2, name, expr);
}

// include : 'include' str_lit
static struct ast *parse_include(struct parse *parser)
{
    if (!expect(parser, INCLUDE_TOK))
        return NULL;

    if (peek_tok(parser)->type != STR_TOK)
        syntax_error(parser);

    const char *path = get_tok(parser)->val.u.s;

    // include file
    if (strmap_get(parser->included_files, path) == NULL) {
        FILE *fp = fopen(path, "r");
        if (fp == NULL) {
            struct loc *loc = loc_ref(get_linenr(parser));
            fatal(loc, "%s: %s", path, strerror(errno));
            loc_unref(loc);
        }

        struct ast *ast = parse_with_include_map(path, parser->included_files,
                parser->nest_include_level + 1);

        strmap_add(parser->included_files, path, (void *)0x1234);

        fclose(fp);
        return ast;
    }

    return ast_new(get_linenr(parser), ALREADY_INCLUDED);
}


// Parse expression with assignment precedence.
static struct ast *parse_asgn_expr(struct parse *parse)
{
    return parse_binop_expr(parse, ASGN_PREC);
}

/* expr_list : (expr_list ',')*
 *           | (expr_list ',')* expr_list
 */
static struct ast *parse_expr_list(struct parse *parse)
{
    struct loc *line = get_linenr(parse);
    return ast_new_list(line, EXPR_LIST, parse_list(parse, ',', parse_asgn_expr));
}

/* primary_expr : ident
 *              | ident type
 *              | constant
 *              | '{' init_list '}'
 */
static struct ast *parse_primary_expr(struct parse *parse)
{
    struct loc *line = get_linenr(parse);
    switch (peek_tok(parse)->type) {
        case IDENT_TOK: {
            char *ident = get_tok(parse)->val.u.s;

            if (strcmp(ident, "true") == 0)
                return ast_new(line, TRUE_CONST);
            else if (strcmp(ident, "false") == 0)
                return ast_new(line, FALSE_CONST);
            else if (strcmp(ident, "null") == 0)
                return ast_new(line, NULL_CONST);

            struct ast *name = ast_new_s(line, NAME, ident);

            int pos = parse->pos;
            struct ast *type = parse_type(parse);
            if (type == NULL) {
                ast_unref(type);
                parse->pos = pos;
                return name;
            }

            return ast_new_ast(line, DECL, 2, name, type);
        }
        case NUM_TOK: {
            return ast_new_i(line, INT_CONST, get_tok(parse)->val.u.i);
        }
        case FLOAT_NUM_TOK: {
            return ast_new_f(line, FLOAT_CONST, get_tok(parse)->val.u.f);
        }
        case STR_TOK: {
            return ast_new_s(line, STR_LIT, get_tok(parse)->val.u.s);
        }
        case CHAR_TOK: {
            return ast_new_s(line, CHAR_LIT, get_tok(parse)->val.u.s);
        }
        case '(': {
            return parse_enclosed(parse, '(', parse_expr, ')', false);
        }
        case '{': {
            struct ast *ast = parse_enclosed(parse, '{', parse_expr_list, '}', false);
            if (ast == NULL)
                return NULL;

            ast->tag = INIT_EXPR;
            return ast;
        }
    }
    return NULL;
}

/* postfix_expr : primary_expr
 *              | postfix_expr '^'
 *              | postfix_expr '[' expr ']'
 *              | postfix_expr '(' expr_list ')'
 *              | postfix_expr 'as' type
 */
static struct ast *parse_postfix_expr(struct parse *parse)
{
    struct ast *expr = parse_primary_expr(parse);
    if (expr == NULL)
        return expr;

    for (;;) {
	struct loc *line = get_linenr(parse);
        switch (peek_tok(parse)->type) {
            case DEC_TOK: {
                parse->pos++;
                expr = ast_new_ast(line, POST_DEC_EXPR, 1, expr);
                continue;
            }
            case INC_TOK: {
                parse->pos++;
                expr = ast_new_ast(line, POST_INC_EXPR, 1, expr);
                continue;
            }
            case '^': {
                parse->pos++;
                expr = ast_new_ast(line, DEREF_EXPR, 1, expr);
                continue;
            }
            case '[': {
                struct ast *expr2 = parse_enclosed(parse, '[', parse_expr, ']', false);
                if (expr2 == NULL)
                    goto err;

                expr = ast_new_ast(line, SUBSCR_EXPR, 2, expr, expr2);
                continue;
            }
            case '(': {
                struct ast *expr_list = parse_enclosed(parse, '(',
                        parse_expr_list, ')', false);
                if (expr_list == NULL)
                    goto err;

                expr = ast_new_ast(line, CALL_EXPR, 2, expr, expr_list);
                continue;
            }
            case '.':
                parse->pos++;
                struct ast *member_ast = parse_name(parse);
                if (member_ast == NULL)
                    goto err;

                expr = ast_new_ast(line, MEMBER_EXPR, 2, expr, member_ast);
                continue;

        }
        break;
    }

    return expr;

err:
    ast_unref(expr);
    return NULL;
}

/* unary_expr : postfix_expr
 *            | '-' unary_expr
 *            | '+' unary_expr
 *            | '!' unary_expr
 *            | '~' unary_expr
 *            | '^' unary_expr
 *            | 'sizeof' unary_expr
 */
static struct ast *parse_unary_expr(struct parse *parse)
{
    struct loc *line = get_linenr(parse);
    enum ast_tag op = INVALID_AST_TAG;

    switch (peek_tok(parse)->type) {
        case '+':
            op = UPLUS_EXPR;
            break;

        case '-':
            op = NEG_EXPR;
            break;

        case '~':
            op = COMPL_EXPR;
            break;

        case '!':
            op = NOT_EXPR;
            break;

        case '^':
            op = REF_EXPR;
            break;

        case DEC_TOK:
            op = PRE_DEC_EXPR;
            break;

        case INC_TOK:
            op = PRE_INC_EXPR;
            break;

        case SIZEOF_TOK:
            op = SIZEOF_EXPR;
            break;

        default:
            return parse_postfix_expr(parse);
    }

    parse->pos++;
    struct ast *ast = parse_unary_expr(parse);
    if (ast == NULL)
        return NULL;

    return ast_new_ast(get_linenr(parse), op, 1, ast);
}

/* cast_expr : unary_expr
 *           | unary_expr 'as' type
 */
static struct ast *parse_cast_expr(struct parse *parse)
{
    struct ast *expr = parse_unary_expr(parse);
    if (expr == NULL)
        goto err0;

    struct loc *line = get_linenr(parse);
    if (peek_tok(parse)->type == AS_TOK) {
        parse->pos++;
        struct ast *type = parse_type(parse);
        if (type == NULL)
            goto err1;

        expr = ast_new_ast(line, CAST_EXPR, 2, expr, type); 
    }

    return expr;

err1:
    ast_unref(expr);
err0:
    return NULL;
}

/* expr : cast_expr
 *      | expr '*' expr | expr '/' expr | expr '%' expr
 *      | expr '+' expr | expr '-' expr
 *      | expr '<<' expr | expr '>>' expr
 *      | expr '&' expr
 *      | expr '~' expr
 *      | expr '|' expr
 *      | expr '<' expr | expr '>' expr | expr '<=' expr | expr '>=' expr
 *      | expr '==' expr | expr '!=' expr
 *      | expr '||' expr
 *      | expr '&&' expr
 *      | expr '=' expr | <compound assignment operators>
 *      | expr '?' expr ':' expr
 *      | expr ',' expr
 */
static struct ast *parse_binop_expr(struct parse *parse, int prec)
{
    struct ast *lhs_expr = parse_cast_expr(parse);
    if (lhs_expr == NULL)
        goto err0;

    for (;;) {
        enum ast_tag op;
        int rprec, next_prec;

	struct loc *line = get_linenr(parse);
        switch (peek_tok(parse)->type) {
            case ',':
                op = COMMA_EXPR;
                rprec = COMMA_PREC;
                next_prec = rprec + 1;
                break;

            case '=':
                op = ASGN_EXPR;
                next_prec = rprec = ASGN_PREC;
                break;

            case ADD_ASGN_TOK:
                op = ADD_ASGN_EXPR;
                next_prec = rprec = ASGN_PREC;
                break;

            case SUB_ASGN_TOK:
                op = SUB_ASGN_EXPR;
                next_prec = rprec = ASGN_PREC;
                break;

            case MUL_ASGN_TOK:
                op = MUL_ASGN_EXPR;
                next_prec = rprec = ASGN_PREC;
                break;

            case DIV_ASGN_TOK:
                op = DIV_ASGN_EXPR;
                next_prec = rprec = ASGN_PREC;
                break;

            case REM_ASGN_TOK:
                op = REM_ASGN_EXPR;
                next_prec = rprec = ASGN_PREC;
                break;

            case SHL_ASGN_TOK:
                op = SHL_ASGN_EXPR;
                next_prec = rprec = ASGN_PREC;
                break;

            case SHR_ASGN_TOK:
                op = SHR_ASGN_EXPR;
                next_prec = rprec = ASGN_PREC;
                break;

            case OR_ASGN_TOK:
                op = OR_ASGN_EXPR;
                next_prec = rprec = ASGN_PREC;
                break;

            case XOR_ASGN_TOK:
                op = XOR_ASGN_EXPR;
                next_prec = rprec = ASGN_PREC;
                break;

            case AND_ASGN_TOK:
                op = AND_ASGN_EXPR;
                next_prec = rprec = ASGN_PREC;
                break;

            case ANDAND_TOK:
                op = LAND_EXPR;
                rprec = TRUTH_AND_PREC;
                next_prec = rprec + 1;
                break;

            case OROR_TOK:
                op = LOR_EXPR;
                rprec = TRUTH_OR_PREC;
                next_prec = rprec + 1;
                break;

            case EQ_TOK:
                op = EQ_EXPR;
                rprec = EQ_PREC;
                next_prec = rprec + 1;
                break;

            case NE_TOK:
                op = NE_EXPR;
                rprec = EQ_PREC;
                next_prec = rprec + 1;
                break;

            case '<':
                op = LT_EXPR;
                rprec = REL_PREC;
                next_prec = rprec + 1;
                break;

            case '>':
                op = GT_EXPR;
                rprec = REL_PREC;
                next_prec = rprec + 1;
                break;

            case LE_TOK:
                op = LE_EXPR;
                rprec = REL_PREC;
                next_prec = rprec + 1;
                break;

            case GE_TOK:
                op = GE_EXPR;
                rprec = REL_PREC;
                next_prec = rprec + 1;
                break;

            case '|':
                op = OR_EXPR;
                rprec = BIT_OR_PREC;
                next_prec = rprec + 1;
                break;

            case '~':
                op = XOR_EXPR;
                rprec = BIT_XOR_PREC;
                next_prec = rprec + 1;
                break;

            case '&':
                op = AND_EXPR;
                rprec = BIT_AND_PREC;
                next_prec = rprec + 1;
                break;

            case SHL_TOK:
                op = SHL_EXPR;
                rprec = SHIFT_PREC;
                next_prec = rprec + 1;
                break;

            case SHR_TOK:
                op = SHR_EXPR;
                rprec = SHIFT_PREC;
                next_prec = rprec + 1;
                break;

            case '+':
                op = ADD_EXPR;
                rprec = ADD_PREC;
                next_prec = rprec + 1;
                break;

            case '-':
                op = SUB_EXPR;
                rprec = ADD_PREC;
                next_prec = rprec + 1;
                break;

            case '*':
                op = MUL_EXPR;
                rprec = MUL_PREC;
                next_prec = rprec + 1;
                break;

            case '/':
                op = DIV_EXPR;
                rprec = MUL_PREC;
                next_prec = rprec + 1;
                break;

            case '%':
                op = REM_EXPR;
                rprec = MUL_PREC;
                next_prec = rprec + 1;
                break;

            case '?':
                op = COND_EXPR;
                rprec = COND_PREC;
                next_prec = COND_PREC;

                if (prec > rprec)
                    return lhs_expr;

                parse->pos++;
                struct ast *rhs1_expr = parse_expr(parse);
                if (rhs1_expr == NULL)
                    goto err1;

                expect(parse, ':');
                struct ast *rhs2_expr = parse_binop_expr(parse, COND_PREC);
                if (rhs2_expr == NULL)
                    goto err1;

                lhs_expr = ast_new_ast(line, op, 3, lhs_expr, rhs1_expr, rhs2_expr);
                continue;

            case IDENT_TOK:
                goto err1;

            default:
                return lhs_expr;
        }

        if (prec > rprec)
            return lhs_expr;

        parse->pos++;

        struct ast *rhs_expr = parse_binop_expr(parse, next_prec);
        if (rhs_expr == NULL)
            goto err1;

        lhs_expr = ast_new_ast(line, op, 2, lhs_expr, rhs_expr);
    }

err1:
    ast_unref(lhs_expr);
err0:
    return NULL;
}

static struct ast *parse_expr(struct parse *parse)
{
    return parse_binop_expr(parse, COMMA_PREC);
}

static struct ast *parse_case(struct parse *parse)
{
    struct loc *linenr = get_linenr(parse);

    switch (peek_tok(parse)->type) {
        case CASE_TOK: {
            parse->pos++;
            struct ast *expr = parse_expr_list(parse);

            size_t n_childs;
            ast_asts(expr, &n_childs);

            if (n_childs == 0)
                syntax_error(parse);

            if (!expect(parse, ':'))
                return NULL;

            struct ast *stmt_list = parse_stmt_list(parse);
            return ast_new_ast(linenr, CASE_CLAUSE, 2, expr, stmt_list);
        }
        case DEFAULT_TOK: {
            parse->pos++;

            if (!expect(parse, ':'))
                return NULL;

            struct ast *stmt_list = parse_stmt_list(parse);
            return ast_new_ast(linenr, DEFAULT_CLAUSE, 1, stmt_list);
        }
        default:
            return NULL;
    }
}

static struct ast *parse_case_list(struct parse *parse)
{
    size_t n_asts = 0;
    struct ast_list *head = NULL;
    struct ast_list **tailp = &head;

    struct loc *linenr = get_linenr(parse);

    for (;;) {
        int pos = parse->pos;
        struct ast *ast = parse_case(parse);
        if (ast == NULL) {
            parse->pos = pos;
            break;
        }

        *tailp = ast_list_new(ast);
        (*tailp)->ast = ast;

        n_asts++;
        tailp = &(*tailp)->next;
    }

    return ast_new_list(linenr, CASE_LIST, head);
}

/* switch_stmt : 'switch' expr '{' case_clause_list '}'
 */
static struct ast *parse_switch_stmt(struct parse *parse)
{
    if (!expect(parse, SWITCH_TOK))
        goto err0;

    struct loc *line = get_linenr(parse);
    struct ast *expr = parse_expr(parse);
    if (expr == NULL)
        goto err0;

    if (!expect(parse, '{'))
        goto err1;


    // TODO: parse cases
    struct ast *block = parse_case_list(parse);
    
    if (!expect(parse, '}'))
        goto err2;

    return ast_new_ast(line, SWITCH_STMT, 2, expr, block);

err2:
    free(block);
err1:
    free(expr);
err0:        
    return NULL;
}

/* if_stmt : 'if' expr? block
 *         | 'if' expr? block 'else' block
 *         | 'if' expr? block 'else' if_stmt
 */
static struct ast *parse_if_stmt(struct parse *parse)
{
    if (!expect(parse, IF_TOK))
        goto err0;

    struct loc *line = get_linenr(parse);

    struct ast *expr = NULL;
    if (peek_tok(parse)->type != '{') {
        expr = parse_expr(parse);
        if (expr == NULL)
            goto err0;
    }

    struct ast *then_block = parse_block(parse);
    if (then_block == NULL)
        goto err1;

    struct ast *else_block = NULL;
    if (peek_tok(parse)->type == ELSE_TOK) {
	parse->pos++;

        else_block = peek_tok(parse)->type == IF_TOK ?
                parse_if_stmt(parse) : parse_block(parse);

        if (else_block == NULL)
            goto err2;
    }

    return ast_new_ast(line, IF_STMT, 3, expr, then_block, else_block);

err2:
    ast_unref(then_block);
err1:
    ast_unref(expr);
err0:
    return NULL;
}

/* for_stmt : 'for' expr? block
 *          | 'for' expr? ';' expr? ';' expr? block
 */
static struct ast *parse_for_stmt(struct parse *parse)
{
    if (!expect(parse, FOR_TOK))
        return NULL;

    struct loc *line = get_linenr(parse);

    struct ast *expr0 = NULL;
    struct ast *expr1 = NULL;
    struct ast *expr2 = NULL;

    if (peek_tok(parse)->type != '{') {
        expr0 = parse_expr(parse);

        if (peek_tok(parse)->type == ';') {
            parse->pos++;
            expr1 = parse_expr(parse);

            if (!expect(parse, ';')) {
                ast_unref(expr0);
                goto err;
            }

            if (peek_tok(parse)->type != '{')
                expr2 = parse_expr(parse);
        } else {
            // swap expr0 and expr1 so the condition is always in expr1
            expr1 = expr0;
            expr0 = NULL;
        }
    }

    struct ast *block = parse_block(parse);
    if (block == NULL)
        goto err;

    return ast_new_ast(line, FOR_STMT, 4, expr0, expr1, expr2, block);

err:
    ast_unref(expr2);
    ast_unref(expr1);
    ast_unref(expr0);
    return NULL;
}

/* label_stmt : name ':' stmt?
 */
static struct ast *parse_label_stmt(struct parse *parse)
{
    struct loc *line = get_linenr(parse);
    struct ast *name = parse_name(parse);
    if (name == NULL)
        return NULL;

    if (!expect(parse, ':'))
        return NULL;

    struct ast *stmt = parse_stmt(parse);

    return ast_new_ast(line, LABEL_STMT, 2, name, stmt);
}

/* goto_stmt : 'goto' name
 */
static struct ast *parse_goto_stmt(struct parse *parse)
{
    if (!expect(parse, GOTO_TOK))
        return NULL;

    struct loc *line = get_linenr(parse);

    struct ast *name = parse_name(parse);
    if (name == NULL)
        return NULL;

    return ast_new_ast(line, GOTO_STMT, 1, name);
}

/* stmt : if_stmt
 *      | for_stmt
 *      | 'return' expr?
 *      | 'break'
 *      | 'continue'
 *      | 'fallthrough'
 *      | label
 */
static struct ast *parse_stmt(struct parse *parse)
{
    struct loc *line = get_linenr(parse);
    switch (peek_tok(parse)->type)
    {
        case IF_TOK:
            return parse_if_stmt(parse);

        case FOR_TOK:
            return parse_for_stmt(parse);

        case SWITCH_TOK:
            return parse_switch_stmt(parse);

        case TYPE_TOK:
            return parse_type_def(parse);

        case DEFINE_TOK:
            return parse_alias_def(parse);

        case RETURN_TOK:
            parse->pos++;
            return ast_new_ast(line, RETURN_STMT, 1, parse_expr(parse));

        case GOTO_TOK:
            return parse_goto_stmt(parse);

        case BREAK_TOK:
            parse->pos++;
            return ast_new(line, BREAK_STMT);

        case CONTINUE_TOK:
            parse->pos++;
            return ast_new(line, CONTINUE_STMT);

        case FALLTHROUGH_TOK:
            parse->pos++;
            return ast_new(line, FALLTHROUGH_STMT);

        case IDENT_TOK:  {
            size_t pos = parse->pos;
            struct ast *lbl = parse_label_stmt(parse);
            if (lbl == NULL) {
                parse->pos = pos;
                break;
            }

            return lbl;
        }
    }

    return parse_expr(parse);
}

/* param : ident type
 *       | type
 */
static struct ast *parse_param(struct parse *parse)
{
    struct loc *line = get_linenr(parse);
    if (peek_tok(parse)->type == IDENT_TOK) {
        struct ast *name = ast_new_s(line, NAME, strdup(get_tok(parse)->val.u.s));

        int pos = parse->pos;
        struct ast *type = parse_type(parse);
        if (type == NULL) {
            parse->pos = pos;
            return name;
        }

        return ast_new_ast(line, DECL, 2, name, type);
    }

    return parse_type(parse);
}

/* decl : ident type
 *       | type
 */
static struct ast *parse_decl(struct parse *parse)
{
    struct ast *name = parse_name(parse);
    if (name == NULL)
        return NULL;

    struct ast *type = parse_type(parse);
    if (type == NULL)
        return NULL;

    return ast_new_ast(name->loc, DECL, 2, name, type);
}

/* param_list : (param_list ',')*
 *            | (param_list ',')* param
 */
static struct ast *parse_param_list(struct parse *parse)
{
    struct ast_list *head = NULL, **tailp = &head;

    for (;;) {
        int pos = parse->pos;
        struct ast *ast = parse_param(parse);
        if (ast == NULL) {
            parse->pos = pos;
            break;
        }

        *tailp = ast_list_new(ast);
        (*tailp)->ast = ast;

        tailp = &(*tailp)->next;

        if (peek_tok(parse)->type != ',')
            break;

        parse->pos++;
    }

    struct loc *line = get_linenr(parse);

    if (peek_tok(parse)->type == ELLIPSIS_TOK) {
        *tailp = ast_list_new(ast_new(get_linenr(parse), VARARG_TYPE));
        (*tailp)->ast = ast_new(line, VARARG_TYPE);
        parse->pos++;
    }

    return ast_new_list(line, PARAM_LIST, head);
}

/* decl_list : (decl ',')* decl?
 */
static struct ast *parse_decl_list(struct parse *parse)
{
    struct ast_list **tailp, *head = parse_list(parse, ',', parse_decl);
    struct loc *line = get_linenr(parse);

    return ast_new_list(line, STRUCT_EXPR, head);
}

/* type : ident
 *      | '^' type
 *      | '[' expr ']' type
 *      | '(' param_list ')' type
 */
static struct ast *parse_type(struct parse *parse)
{
    struct loc *line = get_linenr(parse);
    switch (peek_tok(parse)->type) {
        case '^': {
            parse->pos++;

	    struct ast *type = parse_type(parse);
            if (type == NULL)
                return type;

            return ast_new_ast(line, PTR_EXPR, 1, type);
        }
        case '[': {
            struct ast *expr = parse_enclosed(parse, '[', parse_expr, ']', false);
            if (expr == NULL)
                return NULL;

            struct ast *type = parse_type(parse);
            if (type == NULL) {
                ast_unref(expr);
                return type;
            }

            return ast_new_ast(line, ARRAY_EXPR, 2, type, expr);
        }
        case '(': {
            struct ast *param_list = parse_enclosed(parse, '(',
                    parse_param_list, ')', false);

            struct ast *type = parse_type(parse);
            if (type == NULL) {
                ast_unref(param_list);
                return NULL;
            }

            return ast_new_ast(line, FUNC_EXPR, 2, param_list, type);
        }
        case IDENT_TOK: {
            char *s = parse->tokens[parse->pos++].val.u.s;
            return ast_new_s(line, NAME, s);
        }
        case '{': {
            struct ast *field_list = parse_enclosed(parse, '{',
                    parse_decl_list, '}', false);
            return field_list;
        }
    }

    return NULL;
}

static struct ast *parse_stmt_list(struct parse *parse)
{
    struct loc *line = get_linenr(parse);
    return ast_new_list(line, STMT_LIST, parse_list(parse, ';', parse_stmt));
}

/* block : '{' stmt_list '}'
 */
static struct ast *parse_block(struct parse *parse)
{
    return parse_enclosed(parse, '{', parse_stmt_list, '}', true);
}

static struct ast *parse_init_list(struct parse *parse)
{
    struct loc *line = get_linenr(parse);
    return ast_new_list(line, INIT_EXPR, parse_list(parse, ',', parse_init));
}

static struct ast *parse_init(struct parse *parse)
{
    struct ast *expr;
    if (peek_tok(parse)->type == '{') 
        return parse_enclosed(parse, '{', parse_init_list, '}', true);
    else
        return parse_asgn_expr(parse);
}

/* extern_def : ident type
 *            | ident type '=' expr
 *            | ident '(' param_list ')' type block
 *            | 'include' string
 */
static struct ast *parse_extern_def(struct parse *parse)
{
    struct loc *line = get_linenr(parse);
    switch (peek_tok(parse)->type) {
        case IDENT_TOK: {
            struct ast *name = ast_new_s(line, NAME,
		    strdup(get_tok(parse)->val.u.s));

            bool is_func = peek_tok(parse)->type == '(';
            struct ast *type = parse_type(parse);
            if (type == NULL) {
                ast_unref(name);
                return NULL;
            }

            struct ast *decl = ast_new_ast(line, DECL, 2, name, type);

            if (is_func && peek_tok(parse)->type == '{')
                return ast_new_ast(line, FUNC_DEF, 2, decl, parse_block(parse));

            if (peek_tok(parse)->type == ';') {
                return decl;
            }

            // haven't decided if data definitions should have '=' or not
            if (peek_tok(parse)->type == '=')
                parse->pos++;

            struct ast *init = parse_init(parse);
            if (init == NULL)
                return NULL;

            return ast_new_ast(line, ASGN_EXPR, 2, decl, init);
        }
	case TYPE_TOK:
	    return parse_type_def(parse);
	case DEFINE_TOK:
	    return parse_alias_def(parse);
        case INCLUDE_TOK:
            return parse_include(parse);
    }

    return NULL;
}

/* program : (extern_def ';')*
 *         | (extern_def ';')* extern_def
 */
struct ast *parse_program(struct parse *parse)
{
    struct loc *line = get_linenr(parse);
    struct ast_list *extern_def_list = parse_list(parse, ';', parse_extern_def);

    if (peek_tok(parse)->type != EOF_TOK)
        syntax_error(parse);

    return ast_new_list(line, SOURCE_FILE, extern_def_list);
}

struct ast *parse_with_include_map(const char *path, struct strmap
        *included_files, int nest_include_lev)
{
    struct parse *parse = tokenize(path);
    parse->included_files = included_files;
    parse->nest_include_level = nest_include_lev;
    struct ast *ast = parse_program(parse);

    parse_del(parse);

    return ast;
}

struct ast *parse(const char *path)
{
    return parse_with_include_map(path, strmap_new(1024), 0);
}
