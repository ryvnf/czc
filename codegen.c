#include "zc.h"

struct lbl {
    char *name;
    char *c_name;
    bool defined;
    struct ast *first_use;
    struct lbl *next;
};

static struct rope *type_to_c_(struct rope *decl, struct type *type, bool for_abi);
static struct rope *abi_type_to_c(struct rope *decl, struct type *type);

// List of labels used so they can be checked against defined labels
struct lbl *lbl_list = NULL;

struct lbl *new_lbl(const char *name, bool defined)
{
    struct lbl *lbl = malloc(sizeof *lbl);
    lbl->name = strdup(name);
    lbl->c_name = gen_c_ident();
    lbl->defined = defined;
    lbl->next = lbl_list;
    lbl_list = lbl;

    strmap_add(zc_func_labels, name, lbl);

    return lbl;
}

void check_lbl_list(struct lbl *lbl_node)
{
    if (lbl_node == NULL)
        return;

    if (!lbl_node->defined)
        fatal(lbl_node->first_use->loc, "undefined label %s", lbl_node->name);

    free(lbl_node->name);
    free(lbl_node->c_name);
    check_lbl_list(lbl_node->next);
    free(lbl_node);
}

struct rope *stmt_list_to_c(struct rope *rope, struct ast *ast,
        bool *fallthrough);

const char *goto_label(const char *name, struct ast *goto_stmt)
{
    struct lbl *lbl = strmap_get(zc_func_labels, name);

    if (lbl != NULL)
        return lbl->c_name;

    lbl = new_lbl(name, false);
    lbl->first_use = goto_stmt;

    return lbl->c_name;
}

const char *define_label(const char *name)
{
    struct lbl *lbl = strmap_get(zc_func_labels, name);

    if (lbl != NULL) {
        lbl->defined = true;
        return lbl->c_name;
    }

    lbl = new_lbl(name, true);
    return lbl->c_name;
}

void push_scope(void)
{
    current_scope = scope_new(current_scope, 256, 16);
}

void pop_scope(void)
{
    current_scope = current_scope->parent;
}

inline static struct rope *add_paren(struct rope *rope)
{
    rope = rope_new_tree(lparen_rope, rope);
    rope = rope_new_tree(rope, rparen_rope);
    return rope;
}

struct rope *add_indent_lev(struct rope *rope, size_t lev)
{
    if (rope == NULL)
        return NULL;
    for (int i = 0; i < lev; i++)
        rope = rope_new_tree(indent_rope, rope);
    return rope;
}

struct rope *add_indent(struct rope *rope)
{
    return add_indent_lev(rope, zc_indent_level);
}

struct rope *add_indent_nl(struct rope *rope)
{
    rope = rope_new_tree(add_indent(rope), nl_rope);
    return rope;
}

struct rope *ptr_type_to_c(struct rope *decl, struct ptr_type *type)
{
    decl = rope_new_tree(star_rope, decl);
    if (is_func_type(type->to) || is_array_type(type->to))
        decl = add_paren(decl);

    return type_to_c(decl, type->to);
}

struct rope *array_type_to_c(struct rope *decl, struct array_type *type)
{
    decl = rope_new_tree(decl, rope_new_fmt("[%zu]", type->len));
    return type_to_c(decl, type->of);
}

struct rope *func_type_to_c(struct rope *decl, struct func_type *type)
{
    decl = rope_new_tree(decl, lparen_rope);

    if (type->n_params == 0) {
        if (!type->has_vararg)
            decl = rope_new_tree(decl, void_rope);
    } else {
        for (size_t i = 0; i < type->n_params; ++i) {
            struct rope *param = abi_type_to_c(NULL, type->params[i]);
            decl = rope_new_tree(decl, param);

            if (i != type->n_params - 1)
                decl = rope_new_tree(decl, comma_sp_rope);
        }
        if (type->has_vararg) {
            decl = rope_new_tree(decl, comma_sp_rope);
            decl = rope_new_tree(decl, ellipsis_rope);
        }
    }

    decl = rope_new_tree(decl, rparen_rope);

    return abi_type_to_c(decl, type->ret);
}

struct rope *field_to_c(struct field *field)
{
    struct rope *rope = type_to_c(rope_new_s(field->name), field->type);
    rope = rope_new_tree(rope, semi_nl_rope);
    return rope_new_tree(indent_rope, rope);
}

struct rope *struct_def_to_c(struct struct_type *type)
{
    struct rope *rope;
    rope = rope_new_tree(struct_sp_rope, rope_new_s(type->cname));
    rope = rope_new_tree(rope, sp_rope);
    rope = rope_new_tree(rope, lcurly_nl_rope);

    for (size_t i = 0; i < type->n_fields; ++i)
        rope = rope_new_tree(rope, field_to_c(&type->fields[i]));

    return rope_new_tree(rope, rcurly_rope);
}

struct rope *struct_type_to_c(struct rope *decl, struct struct_type *type)
{
    struct rope *rope = rope_new_tree(struct_sp_rope, rope_new_s(type->cname));
    if (decl != NULL) {
        rope = rope_new_tree(rope, sp_rope);
        rope = rope_new_tree(rope, decl);
    }
    return rope;
}

struct rope *basic_type_to_c(struct rope *decl, struct type *type, bool for_abi)
{
    if (decl != NULL)
        decl = rope_new_tree(sp_rope, decl);
    return rope_new_tree(rope_new_s(basic_type_c_name(type, for_abi)), decl);
}

struct rope *type_to_c_(struct rope *decl, struct type *type, bool for_abi)
{
    switch (type_type(type->tag)) {
        case PTR_TYPE_FLAG:
            return ptr_type_to_c(decl, (struct ptr_type *)type);
        case ARRAY_TYPE_FLAG:
            return array_type_to_c(decl, (struct array_type *)type);
        case FUNC_TYPE_FLAG:
            return func_type_to_c(decl, (struct func_type *)type);
        case STRUCT_TYPE_FLAG:
            return struct_type_to_c(decl, (struct struct_type *)type);
        case VOID_TYPE_FLAG:
        case BOOL_TYPE_FLAG:
        case INT_TYPE_FLAG:
        case FLOAT_TYPE_FLAG:
        case EXTERN_TYPE_FLAG:
            return basic_type_to_c(decl, type, for_abi);
    }

    return NULL;
}

struct rope *type_to_c(struct rope *decl, struct type *type) {
    return type_to_c_(decl, type, false);
}

struct rope *abi_type_to_c(struct rope *decl, struct type *type) {
    return type_to_c_(decl, type, true);
}

struct rope *decl_to_c(struct decl_sym *decl)
{
    struct rope *c_decl = rope_new_s(decl->c_name); // TODO: generate unique identifier
    c_decl = type_to_c(c_decl, decl->type);
    return c_decl;
}

struct rope *expr_stmt_to_c(struct ast *ast)
{
    struct expr expr = eval_expr(NULL, ast);
    if (ast->tag == DECL)
        return NULL;
    return add_indent_nl(rope_new_tree(expr.rope, semi_rope));
}

struct rope *if_stmt_to_c(struct ast *ast)
{
    struct rope *rope = if_sp_lparen_rope;
    struct ast *expr_ast = ast_ast(ast, 0);
    struct ast *then_block_ast = ast_ast(ast, 1);
    struct ast *else_block_ast = ast_ast(ast, 2);

    push_scope();

    if (expr_ast != NULL) {
        struct expr expr = eval_expr(NULL, expr_ast);
        if (!is_bool_type(expr.type))
            fatal(NULL, "condition has to be of boolean type");
        rope = rope_new_tree(rope, expr.rope);
    } else {
        rope = rope_new_tree(rope, one_rope);
    }

    rope = rope_new_tree(rope, rparen_sp_rope);
    rope = rope_new_tree(rope, block_to_c(then_block_ast));

    if (else_block_ast) {
        rope = rope_new_tree(rope, sp_else_sp_rope);
        if (else_block_ast->tag == IF_STMT)
            rope = rope_new_tree(rope, if_stmt_to_c(else_block_ast));
        else 
            rope = rope_new_tree(rope, block_to_c(else_block_ast));
    }

    pop_scope();

    return add_indent_nl(rope);
}

struct rope *clause_to_c(struct ast *ast)
{
    struct rope *rope = NULL;
    struct ast *stmt_list;

    switch (ast->tag) {
        case CASE_CLAUSE: {
            size_t n_asts;
            struct ast **asts = ast_asts(ast_ast(ast, 0), &n_asts);

            for (size_t i = 0; i < n_asts; i++) {
                rope = rope_new_tree(rope, add_indent(case_sp_rope));
                
                struct expr expr = eval_expr(NULL, asts[i]);

                rope = rope_new_tree(rope, expr.rope);
                rope = rope_new_tree(rope, colon_nl_rope);
            }

            stmt_list = ast_ast(ast, 1);
            break;
        }
        case DEFAULT_CLAUSE: {
            rope = add_indent(default_colon_nl_rope);
            stmt_list = ast_ast(ast, 0);
            break;
        }
    }

    zc_indent_level++;
    bool fallthrough = false;

    size_t n_stmts;
    ast_asts(stmt_list, &n_stmts);

    if (n_stmts != 0) 
        rope = stmt_list_to_c(rope, stmt_list, &fallthrough);
    else
        rope = add_indent_nl(semi_rope);

    if (!fallthrough)
        rope = rope_new_tree(rope, add_indent_nl(break_semi_rope));
    zc_indent_level--;

    return rope;
}

struct rope *switch_block_to_c(struct ast *ast)
{
    struct rope *rope = lcurly_nl_rope;

    size_t n_clause_asts;
    struct ast **clause_asts = ast_asts(ast, &n_clause_asts);

    for (size_t i = 0; i < n_clause_asts; ++i) {
        struct rope *c_clause = clause_to_c(clause_asts[i]);
	if (rope != NULL)
	    rope = rope_new_tree(rope, c_clause);
    }

    rope = rope_new_tree(rope, add_indent(rcurly_rope));

    return rope;
}

struct rope *switch_stmt_to_c(struct ast *ast)
{
    struct rope *rope = switch_sp_lparen_rope;
    struct ast *expr_ast = ast_ast(ast, 0);
    struct ast *block_ast = ast_ast(ast, 1);

    push_scope();

    struct expr expr = eval_expr(NULL, expr_ast);
    if (!is_bool_type(expr.type) && !is_int_type(expr.type))
        fatal(ast->loc, "expression has to be of boolean or integer type");

    rope = rope_new_tree(rope, expr.rope);
    rope = rope_new_tree(rope, rparen_sp_rope);

    rope = rope_new_tree(rope, switch_block_to_c(block_ast));

    pop_scope();

    return add_indent_nl(rope);
}

struct rope *for_stmt_to_c(struct ast *ast)
{
    struct rope *rope = for_sp_lparen_rope;
    struct ast *init_expr_ast = ast_ast(ast, 0);
    struct ast *cond_expr_ast = ast_ast(ast, 1);
    struct ast *update_expr_ast = ast_ast(ast, 2);
    struct ast *block_ast = ast_ast(ast, 3);

    push_scope();

    if (init_expr_ast != NULL) {
        struct expr init_expr = eval_expr(NULL, init_expr_ast);
        rope = rope_new_tree(rope, init_expr.rope);
    }
    rope = rope_new_tree(rope, semi_rope);

    if (cond_expr_ast != NULL) {
        rope = rope_new_tree(rope, sp_rope);

        struct expr cond_expr = eval_expr(NULL, cond_expr_ast);
        if (!is_bool_type(cond_expr.type))
            fatal(ast->loc, "condition has to be of boolean type");

        rope = rope_new_tree(rope, cond_expr.rope);
    }
    rope = rope_new_tree(rope, semi_rope);

    if (update_expr_ast != NULL) {
        rope = rope_new_tree(rope, sp_rope);

        struct expr update_expr = eval_expr(NULL, update_expr_ast);
        rope = rope_new_tree(rope, update_expr.rope);
    }
    rope = rope_new_tree(rope, rparen_sp_rope);

    zc_loop_level++;
    rope = rope_new_tree(rope, block_to_c(block_ast));
    zc_loop_level--;

    pop_scope();

    return add_indent_nl(rope);
}

struct rope *return_stmt_to_c(struct ast *ast)
{
    struct ast *expr_ast = ast_ast(ast, 0);

    if (expr_ast == NULL) {
        if (zc_func_ret_type != void_type)
            fatal(ast->loc, "missing return value in non-void function");
        return return_semi_rope;
    }

    struct expr expr = eval_expr(zc_func_ret_type, expr_ast);

    if (zc_func_ret_type == void_type)
        fatal(ast->loc, "return with value in function returning void");
    if (common_type(zc_func_ret_type, expr.type) != zc_func_ret_type)
        fatal(ast->loc, "invalid type for return");

    struct rope *rope = rope_new_tree(return_sp_rope, expr.rope);
    rope = rope_new_tree(rope, semi_rope);
    return add_indent_nl(rope);
}

struct rope *break_stmt_to_c(struct ast *ast)
{
    if (zc_loop_level == 0)
        fatal(ast->loc, "break not inside a loop");
    return add_indent_nl(break_semi_rope);
}

struct rope *continue_stmt_to_c(struct ast *ast)
{
    if (zc_loop_level == 0)
        fatal(ast->loc, "continue not inside a loop");
    return add_indent_nl(continue_semi_rope);
}

struct rope *label_stmt_to_c(struct ast *ast)
{
    struct ast *name_ast = ast_ast(ast, 0);
    const char *name = ast_s(name_ast);
    const char *c_name = define_label(name);

    struct ast *stmt_ast = ast_ast(ast, 1);

    struct rope *rope = rope_new_s(c_name);
    rope = rope_new_tree(rope, colon_nl_rope);

    if (stmt_ast)
        rope = rope_new_tree(rope, stmt_to_c(stmt_ast));
    else
        rope = rope_new_tree(rope, add_indent_nl(semi_rope));

    return rope;
}

struct rope *goto_stmt_to_c(struct ast *ast)
{
    struct ast *name_ast = ast_ast(ast, 0);
    const char *name = ast_s(name_ast);
    const char *c_name = goto_label(name, ast);

    struct rope *rope = goto_sp_rope;
    rope = rope_new_tree(rope, rope_new_s(c_name));
    rope = rope_new_tree(rope, semi_nl_rope);

    return add_indent(rope);
}

void handle_type_def(struct ast *ast)
{
    const char *name = ast_s(ast_ast(ast, 0));
    scope_add_typesym(current_scope, name, sym_new(ast));
}

void handle_alias_def(struct ast *ast)
{
    const char *name = ast_s(ast_ast(ast, 0));
    scope_add_sym(current_scope, name, sym_new(ast));
}

struct rope *stmt_to_c(struct ast *ast)
{
    switch (ast->tag) {
        default:
            return expr_stmt_to_c(ast);
        case TYPE_DEF:
	    handle_type_def(ast);
	    return NULL;
        case ALIAS_DEF:
	    handle_alias_def(ast);
	    return NULL;
        case IF_STMT:
            return if_stmt_to_c(ast);
        case SWITCH_STMT:
            return switch_stmt_to_c(ast);
        case FOR_STMT:
            return for_stmt_to_c(ast);
        case RETURN_STMT:
            return return_stmt_to_c(ast);
        case BREAK_STMT:
            return break_stmt_to_c(ast);
        case CONTINUE_STMT:
            return continue_stmt_to_c(ast);
        case GOTO_STMT:
            return goto_stmt_to_c(ast);
        case LABEL_STMT:
            return label_stmt_to_c(ast);
    }
}

struct rope *stmt_list_to_c(struct rope *rope, struct ast *ast,
        bool *fallthrough)
{
    size_t n_stmt_asts;
    struct ast **stmt_asts = ast_asts(ast, &n_stmt_asts);

    for (size_t i = 0; i < n_stmt_asts; ++i) {
        if (stmt_asts[i]->tag == FALLTHROUGH_STMT) {
            if (fallthrough == NULL || i != n_stmt_asts - 1)
                fatal(stmt_asts[i]->loc, "fallthrough not allowed here");

            *fallthrough = true;
            break;
        }

        struct rope *c_stmt = stmt_to_c(stmt_asts[i]);
        rope = rope_new_tree(rope, c_stmt);
    }

    return rope;
}

struct rope *block_to_c(struct ast *ast)
{
    struct rope *rope = lcurly_nl_rope;
    zc_indent_level++;

    rope = stmt_list_to_c(rope, ast, NULL);

    zc_indent_level--;
    rope = rope_new_tree(rope, add_indent(rcurly_rope));

    return rope;
}

struct rope *func_body_to_c(struct ast *ast)
{
    zc_func_labels = strmap_new(32);
    lbl_list = NULL;

    struct rope *rope = lcurly_nl_rope;
    zc_func_decls_rope = NULL;

    zc_indent_level++;

    struct rope *stmts_rope = NULL;
    stmts_rope = stmt_list_to_c(stmts_rope, ast, NULL);

    rope = rope_new_tree(rope, zc_func_decls_rope);
    rope = rope_new_tree(rope, stmts_rope);

    zc_indent_level--;
    rope = rope_new_tree(rope, add_indent(rcurly_rope));

    strmap_del(zc_func_labels, NULL);
    check_lbl_list(lbl_list);

    return rope;
}

void add_global_decl(struct decl_sym *decl)
{
    if (is_void_type(decl->type))
        fatal(decl->sym.loc->loc, "cannot declare a variable of void type");

    if (is_extern_type(decl->type))
        fatal(decl->sym.loc->loc, "cannot declare a variable of incomplete type");

    add_type_decl(decl->type);
    struct rope *rope = decl_to_c(decl);
    rope = rope_new_tree(extern_sp_rope, rope);
    rope = rope_new_tree(rope, semi_nl_rope);
    zc_prog_decls_rope = rope_new_tree(zc_prog_decls_rope, rope);
}

void add_local_decl(struct decl_sym *decl)
{
    if (is_void_type(decl->type))
        fatal(decl->sym.loc->loc, "cannot declare a variable of void type");

    if (is_extern_type(decl->type))
        fatal(decl->sym.loc->loc, "cannot declare a variable of incomplete type");

    if (is_func_type(decl->type))
        fatal(decl->sym.loc->loc, "cannot declare a function here");

    add_type_decl(decl->type);
    struct rope *rope = NULL;

    rope = rope_new_tree(rope, decl_to_c(decl));
    rope = rope_new_tree(rope, semi_nl_rope);
    rope = add_indent_lev(rope, 1);

    zc_func_decls_rope = rope_new_tree(zc_func_decls_rope, rope);
}

void add_struct_def(struct struct_type *type)
{
    if (type->is_defined)
        return;
    type->is_defined = true;
    
    for (size_t i = 0; i < type->n_fields; i++)
        add_type_decl(type->fields[i].type);

    struct rope *rope = NULL;
    rope = struct_def_to_c(type);
    rope = rope_new_tree(rope, semi_nl_rope);
    zc_type_defs_rope = rope_new_tree(zc_type_defs_rope, rope);

    rope = struct_type_to_c(NULL, type);
    rope = rope_new_tree(rope, semi_nl_rope);
    zc_type_decls_rope = rope_new_tree(zc_type_decls_rope, rope);
}

void add_type_decl(struct type *type)
{
    switch (type_type(type->tag)) {
	case PTR_TYPE_FLAG:
            add_type_decl(ptr_type(type)->to);
            break;
	case ARRAY_TYPE_FLAG:
            add_type_decl(array_type(type)->of);
            break;
	case FUNC_TYPE_FLAG:
            add_type_decl(func_type(type)->ret);
            break;
	case STRUCT_TYPE_FLAG:
            add_struct_def(struct_type(type));
            break;
    }
}

struct rope *func_def_to_c(struct ast *ast)
{
    struct ast *decl_ast = ast_ast(ast, 0);

    struct ast *block_ast = ast_ast(ast, 1);
    const char *name = ast_s(ast_ast(decl_ast, 0));

    struct sym *sym = scope_get_sym(current_scope, name);
    assert(sym->tag == DECL_SYM);
    struct decl_sym *decl = (struct decl_sym *)sym;
    struct ast *func_type_ast = ast_ast(decl_ast, 1);
    struct ast *param_list_ast = ast_ast(func_type_ast, 0);

    size_t n_param_asts;
    struct ast **param_asts = ast_asts(param_list_ast, &n_param_asts);

    zc_func_ret_type = func_type(decl->type)->ret;

    push_scope();

    struct rope *rope = rope_new_s(name);
    rope = rope_new_tree(rope, lparen_rope);
    if (n_param_asts == 0) {
        rope = rope_new_tree(rope, rope_new_s("void"));
    } else {
        for (size_t i = 0; i < n_param_asts; ++i) {
            struct ast *param_ast = param_asts[i];
            struct rope *c_param;

            if (param_ast->tag == DECL) {
                struct sym *decl = sym_from_ast(param_ast);
                scope_add_sym(current_scope, ast_s(ast_ast(param_ast, 0)), decl);
                c_param = decl_to_c((struct decl_sym *)decl);
            } else {
                const char *tmpname = "_";
                struct type *type = type_from_ast(param_ast);
                c_param = abi_type_to_c(rope_new_s(tmpname), type);
            }

            rope = rope_new_tree(rope, c_param);

            if (i != n_param_asts - 1)
                rope = rope_new_tree(rope, rope_new_s(", "));
        }
    }
    rope = rope_new_tree(rope, rparen_rope);
    rope = abi_type_to_c(rope, func_type(decl->type)->ret);
    rope = rope_new_tree(rope, sp_rope);
    rope = rope_new_tree(rope, func_body_to_c(block_ast));
    rope = rope_new_tree(rope, nl_rope);
    rope = rope_new_tree(nl_rope, rope);

    pop_scope();

    return rope;
}

struct rope *init_ast_to_c(struct type *type, struct ast *init_ast, size_t nest_level)
{
    struct expr expr = eval_expr_global(type, init_ast);

    // In C ',' and '?:' operators have lower precedence than '='.  Should
    // probably be solved in a cleaner way in the future, until then this
    // works.
    if (init_ast->tag == COMMA_EXPR || init_ast->tag == COND_EXPR)
        expr.rope = add_paren(expr.rope);

    struct type *got_type = target_type(type, expr.type);

    if (got_type == NULL)
        fatal(init_ast->loc, "invalid type for initializer");

    if (!type_equals(got_type, type))
        fatal(init_ast->loc, "invalid type for initializer");

    return expr.rope;
}

struct rope *data_def_to_c(struct ast *ast)
{
    struct ast *decl_ast = ast_ast(ast, 0);

    struct ast *init_ast = ast_ast(ast, 1);
    const char *name = ast_s(ast_ast(decl_ast, 0));

    struct sym *sym = scope_get_sym(global_scope, name);
    assert(sym->tag == DECL_SYM);
    struct decl_sym *decl = (struct decl_sym *)sym;

    struct rope *rope = decl_to_c(decl);

    rope = rope_new_tree(rope, asgn_binop_rope);
    rope = rope_new_tree(rope, init_ast_to_c(decl->type, init_ast, 0));
    rope = rope_new_tree(rope, semi_nl_rope);
    rope = rope_new_tree(nl_rope, rope);

    return rope;
}

void append_def(struct rope *rope)
{
    zc_prog_defs_rope = rope_new_tree(zc_prog_defs_rope, rope);
}

void decl_pass(struct ast *ast)
{
    size_t n_extern_defs;
    struct ast **extern_defs = ast_asts(ast, &n_extern_defs);

    for (size_t i = 0; i < n_extern_defs; ++i) {
        struct ast *extern_def = extern_defs[i];
        struct ast *loc;
        bool is_defined = false;

        switch (extern_def->tag) {
            case TYPE_DEF: {
                loc = extern_def;
                const char *name = ast_s(ast_ast(loc, 0));
                struct type *type = scope_get_type(current_scope, name);
                struct ast *def_type_ast = ast_ast(loc, 1);
                scope_add_typesym(current_scope, name, sym_new(loc));

                if (def_type_ast != NULL) {
                    struct type *def_type = type_from_ast(def_type_ast);

                    if (type != NULL && !type_equals(type, def_type))
                        fatal(extern_def->loc, "%s is redefined as a different type", name);

                    type_del(def_type);
                }

                continue;
            }
            case ASGN_EXPR:
            case FUNC_DEF:
                is_defined = true;
                loc = ast_ast(extern_def, 0);
                break;
            case ALIAS_DEF:
            case DECL: {
                loc = extern_def;
                break;
            }
            case SOURCE_FILE:
                decl_pass(extern_def);
                continue;
            case ALREADY_INCLUDED:
                continue;
            default:
                bug("%s should not be here", ast_tag_name(extern_def->tag));
        }
        const char *name = ast_s(ast_ast(loc, 0));
        struct sym *sym = scope_get_sym(current_scope, name);

        bool redecl = false;

        if (sym != NULL) {
            if (sym->tag == DECL_SYM && loc->tag == DECL) {
                struct decl_sym *decl_sym = (struct decl_sym *)sym;

                if (is_defined && decl_sym->is_defined)
                    fatal(extern_def->loc, "%s is redefined", name);

                struct type *type = type_from_ast(ast_ast(loc, 1));

                if (!type_equals(type, ((struct decl_sym *)sym)->type))
                    fatal(extern_def->loc, "%s is declared as different type", name);

                type_del(type);
            } else {
                fatal(extern_def->loc, "%s is redefined", name);
            }
            redecl = true;
        }

        struct sym *new_sym = sym_new(loc);

        if (is_defined)
            ((struct decl_sym *)new_sym)->is_defined = true;

        if (!redecl)
            scope_add_sym(current_scope, name, new_sym);
    }
}

void codegen_pass(struct ast *ast)
{
    size_t n_extern_defs;
    struct ast **extern_defs = ast_asts(ast, &n_extern_defs);

    for (size_t i = 0; i < n_extern_defs; ++i) {
        struct ast *extern_def = extern_defs[i];

        switch (extern_def->tag) {
            case ASGN_EXPR:
                append_def(data_def_to_c(extern_def));
                break;
            case FUNC_DEF:
                append_def(func_def_to_c(extern_def));
                break;
            case TYPE_DEF: {
                scope_get_type(current_scope, ast_s(ast_ast(extern_def, 0)));
                continue;
            }
            case SOURCE_FILE:
                codegen_pass(extern_def);
        }
    }
}

void codegen_to_file(struct ast *ast, FILE *fp)
{
    struct scope *predecl_symtbl = scope_new(NULL, 1, 64);

    scope_add_typesym(predecl_symtbl, "int", sym_new_type(int_type));
    scope_add_typesym(predecl_symtbl, "int8", sym_new_type(int8_type));
    scope_add_typesym(predecl_symtbl, "int16", sym_new_type(int16_type));
    scope_add_typesym(predecl_symtbl, "int32", sym_new_type(int32_type));
    scope_add_typesym(predecl_symtbl, "int64", sym_new_type(int64_type));
    scope_add_typesym(predecl_symtbl, "ssize", sym_new_type(ssize_type));
    scope_add_typesym(predecl_symtbl, "uint", sym_new_type(uint_type));
    scope_add_typesym(predecl_symtbl, "uint8", sym_new_type(uint8_type));
    scope_add_typesym(predecl_symtbl, "uint16", sym_new_type(uint16_type));
    scope_add_typesym(predecl_symtbl, "uint32", sym_new_type(uint32_type));
    scope_add_typesym(predecl_symtbl, "uint64", sym_new_type(uint64_type));
    scope_add_typesym(predecl_symtbl, "size", sym_new_type(size_type));
    scope_add_typesym(predecl_symtbl, "float", sym_new_type(float_type));
    scope_add_typesym(predecl_symtbl, "double", sym_new_type(double_type));
    scope_add_typesym(predecl_symtbl, "bool", sym_new_type(bool_type));
    scope_add_typesym(predecl_symtbl, "char", sym_new_type(char_type));
    scope_add_typesym(predecl_symtbl, "void", sym_new_type(void_type));

    current_scope = global_scope = scope_new(predecl_symtbl, 65536, 8192);

    zc_type_decls_rope = NULL;
    zc_type_defs_rope = NULL;
    zc_prog_decls_rope = NULL;
    zc_prog_defs_rope = NULL;

    decl_pass(ast);
    codegen_pass(ast);

    ast_unref(ast);

    struct rope *zc_file_rope = zc_type_decls_rope;
    zc_file_rope = rope_new_tree(zc_file_rope, zc_type_defs_rope);
    zc_file_rope = rope_new_tree(zc_file_rope, zc_prog_decls_rope);
    zc_file_rope = rope_new_tree(zc_file_rope, zc_prog_defs_rope);
    rope_print_to_file(zc_file_rope, fp);
}

char *gen_c_ident()
{
    static int n = 0;
    char *ident = malloc(13);
    snprintf(ident, 13, "id%d", n++);
    return ident;
}
