#include "zc.h"

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
            struct rope *param = type_to_c(NULL, type->params[i]);
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

    return type_to_c(decl, type->ret);
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

struct rope *basic_type_to_c(struct rope *decl, struct type *type)
{
    if (decl != NULL)
        decl = rope_new_tree(sp_rope, decl);
    return rope_new_tree(rope_new_s(basic_type_c_name(type)),  decl);
}

struct rope *type_to_c(struct rope *decl, struct type *type)
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
            return basic_type_to_c(decl, type);
    }

    return NULL;
}

struct rope *decl_to_c(struct decl_sym *decl)
{
    struct rope *c_decl = rope_new_s(decl->c_name); // TODO: generate unique identifier
    c_decl = type_to_c(c_decl, decl->type);
    return c_decl;
}

struct rope *expr_stmt_to_c(struct ast *ast)
{
    struct expr expr = eval_expr(ast);
    if (ast->tag == DECL)
        return NULL;
    return rope_new_tree(expr.rope, semi_rope);
}

struct rope *cmpnd_stmt_to_c(struct ast *ast)
{
    push_scope();

    struct rope *rope = block_to_c(ast_ast(ast, 0));

    pop_scope();
    return rope;
}

struct rope *if_stmt_to_c(struct ast *ast)
{
    struct rope *rope = if_sp_lparen_rope;
    struct ast *expr_ast = ast_ast(ast, 0);
    struct ast *then_block_ast = ast_ast(ast, 1);
    struct ast *else_block_ast = ast_ast(ast, 2);

    push_scope();

    struct expr expr = eval_expr(expr_ast);
    if (!is_bool_type(expr.type))
        fatal(-1, "condition has to be of boolean type");

    rope = rope_new_tree(rope, expr.rope);
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

    return rope;
}

struct rope *while_stmt_to_c(struct ast *ast)
{
    struct rope *rope = while_sp_lparen_rope;
    struct ast *expr_ast = ast_ast(ast, 0);
    struct ast *block_ast = ast_ast(ast, 1);

    push_scope();

    struct expr expr = eval_expr(expr_ast);
    if (!is_bool_type(expr.type))
        fatal(ast->line, "condition has to be of boolean type");

    rope = rope_new_tree(rope, expr.rope);
    rope = rope_new_tree(rope, rparen_sp_rope);

    zc_loop_level++;
    rope = rope_new_tree(rope, block_to_c(block_ast));
    zc_loop_level--;

    pop_scope();

    return rope;
}

struct rope *do_while_stmt_to_c(struct ast *ast)
{
    struct rope *rope = do_sp_rope;
    struct ast *block_ast = ast_ast(ast, 0);
    struct ast *expr_ast = ast_ast(ast, 1);

    push_scope();

    zc_loop_level++;
    rope = rope_new_tree(rope, block_to_c(block_ast));
    zc_loop_level--;

    struct expr expr = eval_expr(expr_ast);
    if (!is_bool_type(expr.type))
        fatal(ast->line, "condition has to be of boolean type");

    rope = rope_new_tree(rope, sp_while_sp_lparen_rope);
    rope = rope_new_tree(rope, expr.rope);
    rope = rope_new_tree(rope, rparen_semi_nl_rope);

    pop_scope();

    return rope;
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
        struct expr init_expr = eval_expr(init_expr_ast);
        rope = rope_new_tree(rope, init_expr.rope);
    }
    rope = rope_new_tree(rope, semi_rope);

    if (cond_expr_ast != NULL) {
        rope = rope_new_tree(rope, sp_rope);

        struct expr cond_expr = eval_expr(cond_expr_ast);
        if (!is_bool_type(cond_expr.type))
            fatal(ast->line, "condition has to be of boolean type");

        rope = rope_new_tree(rope, cond_expr.rope);
    }
    rope = rope_new_tree(rope, semi_rope);

    if (update_expr_ast != NULL) {
        rope = rope_new_tree(rope, sp_rope);

        struct expr update_expr = eval_expr(update_expr_ast);
        rope = rope_new_tree(rope, update_expr.rope);
    }
    rope = rope_new_tree(rope, rparen_sp_rope);

    zc_loop_level++;
    rope = rope_new_tree(rope, block_to_c(block_ast));
    zc_loop_level--;

    pop_scope();

    return rope;
}

struct rope *return_stmt_to_c(struct ast *ast)
{
    struct ast *expr_ast = ast_ast(ast, 0);

    if (expr_ast == NULL) {
        if (zc_func_ret_type != void_type)
            fatal(ast->line, "missing return value in non-void function");
        return return_semi_rope;
    }

    struct expr expr = eval_expr(expr_ast);

    if (zc_func_ret_type == void_type)
        fatal(ast->line, "return with value in function returning void");
    if (common_type(zc_func_ret_type, expr.type) != zc_func_ret_type)
        fatal(ast->line, "invalid type for return");

    struct rope *rope = rope_new_tree(return_sp_rope, expr.rope);
    rope = rope_new_tree(rope, semi_rope);
    return rope;
}

struct rope *break_stmt_to_c(struct ast *ast)
{
    if (zc_loop_level == 0)
        fatal(ast->line, "break not inside a loop");
    return break_semi_rope;
}

struct rope *continue_stmt_to_c(struct ast *ast)
{
    if (zc_loop_level == 0)
        fatal(ast->line, "continue not inside a loop");
    return continue_semi_rope;
}

struct rope *stmt_to_c(struct ast *ast)
{
    switch (ast->tag) {
        default:
            return expr_stmt_to_c(ast);
        case CMPND_STMT:
            return cmpnd_stmt_to_c(ast);
        case IF_STMT:
            return if_stmt_to_c(ast);
        case WHILE_STMT:
            return while_stmt_to_c(ast);
        case DO_WHILE_STMT:
            return do_while_stmt_to_c(ast);
        case FOR_STMT:
            return for_stmt_to_c(ast);
        case RETURN_STMT:
            return return_stmt_to_c(ast);
        case BREAK_STMT:
            return break_stmt_to_c(ast);
        case CONTINUE_STMT:
            return continue_stmt_to_c(ast);
    }
}

struct rope *block_to_c(struct ast *ast)
{
    struct rope *rope = lcurly_nl_rope;
    zc_indent_level++;

    size_t n_stmt_asts;
    struct ast **stmt_asts = ast_asts(ast, &n_stmt_asts);

    for (size_t i = 0; i < n_stmt_asts; ++i) {
        struct rope *c_stmt = stmt_to_c(stmt_asts[i]);
	if (rope != NULL)
	    rope = rope_new_tree(rope, add_indent_nl(c_stmt));
    }

    zc_indent_level--;
    rope = rope_new_tree(rope, add_indent(rcurly_rope));

    return rope;
}

struct rope *func_body_to_c(struct ast *ast)
{
    struct rope *rope = lcurly_nl_rope;
    zc_func_decls_rope = NULL;

    zc_indent_level++;

    size_t n_stmt_asts;
    struct ast **stmt_asts = ast_asts(ast, &n_stmt_asts);

    struct rope *stmts_rope = NULL;


    for (size_t i = 0; i < n_stmt_asts; ++i) {
        struct rope *c_stmt = stmt_to_c(stmt_asts[i]);
        if (c_stmt != NULL)
            stmts_rope = rope_new_tree(stmts_rope, add_indent_nl(c_stmt));
    }

    rope = rope_new_tree(rope, zc_func_decls_rope);
    rope = rope_new_tree(rope, stmts_rope);

    zc_indent_level--;
    rope = rope_new_tree(rope, add_indent(rcurly_rope));

    return rope;
}

void add_global_decl(struct decl_sym *decl)
{
    if (is_void_type(decl->type))
        fatal(decl->sym.loc->line, "cannot declare a variable of void type");

    if (is_extern_type(decl->type))
        fatal(decl->sym.loc->line, "cannot declare a variable of incomplete type");

    add_type_decl(decl->type);
    struct rope *rope = decl_to_c(decl);
    rope = rope_new_tree(extern_sp_rope, rope);
    rope = rope_new_tree(rope, semi_nl_rope);
    zc_prog_decls_rope = rope_new_tree(zc_prog_decls_rope, rope);
}

void add_local_decl(struct decl_sym *decl)
{
    if (is_void_type(decl->type))
        fatal(decl->sym.loc->line, "cannot declare a variable of void type");

    if (is_extern_type(decl->type))
        fatal(decl->sym.loc->line, "cannot declare a variable of incomplete type");

    if (is_func_type(decl->type))
        fatal(decl->sym.loc->line, "cannot declare a function here");

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
    zc_prog_defs_rope = rope_new_tree(zc_prog_defs_rope, rope);

    rope = struct_type_to_c(NULL, type);
    rope = rope_new_tree(rope, semi_nl_rope);
    zc_prog_decls_rope = rope_new_tree(zc_prog_decls_rope, rope);
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
                c_param = type_to_c(rope_new_s(tmpname), type);
            }

            rope = rope_new_tree(rope, c_param);

            if (i != n_param_asts - 1)
                rope = rope_new_tree(rope, rope_new_s(", "));
        }
    }
    rope = rope_new_tree(rope, rparen_rope);
    rope = type_to_c(rope, func_type(decl->type)->ret);
    rope = rope_new_tree(rope, sp_rope);
    rope = rope_new_tree(rope, func_body_to_c(block_ast));
    rope = rope_new_tree(rope, nl_rope);
    rope = rope_new_tree(nl_rope, rope);

    pop_scope();

    return rope;
}

struct rope *init_ast_to_c(struct type *type, struct ast *init_ast, size_t nest_level)
{
    struct rope *rope;

    if (init_ast->tag == EXPR_LIST) {
        size_t n_asts;
        struct ast **asts = ast_asts(init_ast, &n_asts);

        rope = lcurly_sp_rope;
        nest_level++;

        if (type_type(type->tag) == ARRAY_TYPE_FLAG) {
            size_t len = array_type(type)->len;
            struct type *elem_type = array_type(type)->of;

            if (n_asts > len)
                fatal(init_ast->line, "excess elements in initializer");

            for (size_t i = 0; i < n_asts; i++) {
                rope = rope_new_tree(rope, init_ast_to_c(elem_type, asts[i], nest_level));
                if (i != n_asts - 1)
                    rope = rope_new_tree(rope, comma_sp_rope);
                else
                    rope = rope_new_tree(rope, sp_rope);
            }
        } else if (type_type(type->tag) == STRUCT_TYPE_FLAG) {
            size_t n_fields = struct_type(type)->n_fields;
            struct field *fields = struct_type(type)->fields;

            if (n_asts > n_fields)
                fatal(init_ast->line, "excess elements in initializer");

            for (size_t i = 0; i < n_asts; i++) {
                rope = rope_new_tree(rope, init_ast_to_c(fields[i].type, asts[i], nest_level));
                if (i != n_asts - 1)
                    rope = rope_new_tree(rope, comma_sp_rope);
                else
                    rope = rope_new_tree(rope, sp_rope);
            }
        }

        nest_level--;
        rope = rope_new_tree(rope, rcurly_rope);

    } else {
        struct expr expr = eval_expr(init_ast);

        // In C ',' and '?:' operators have lower precedence than '='.  Should
        // probably be solved in a cleaner way in the future, until then this
        // works.
        if (init_ast->tag == COMMA_EXPR || init_ast->tag == COND_EXPR)
            expr.rope = add_paren(expr.rope);

        struct type *got_type = target_type(type, expr.type);

        if (got_type == NULL)
            fatal(init_ast->line, "invalid type for initializer");

        if (!type_equals(got_type, type))
            fatal(init_ast->line, "invalid type for initializer");

        rope = expr.rope;
    }
    return rope;
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

    zc_prog_decls_rope = NULL;
    zc_prog_defs_rope = NULL;

    size_t n_extern_defs;
    struct ast **extern_defs = ast_asts(ast, &n_extern_defs);

    for (size_t i = 0; i < n_extern_defs; ++i) {
        struct ast *extern_def = extern_defs[i];
        struct ast *loc;
        switch (extern_def->tag) {
            case TYPE_DEF: {
                loc = extern_def;
                const char *name = ast_s(ast_ast(loc, 0));
                if (scope_get_type(current_scope, name) != NULL)
                    fatal(extern_def->line, "global type %s is already defined", name);
                scope_add_typesym(current_scope, name, sym_new(loc));
                continue;
            }
            case ASGN_EXPR:
            case FUNC_DEF:
                loc = ast_ast(extern_def, 0);
                break;
            case ALIAS_DEF:
            case DECL: {
                loc = extern_def;
                break;
            }
        }
        const char *name = ast_s(ast_ast(loc, 0));

        if (scope_get_sym(current_scope, name) != NULL)
            fatal(extern_def->line, "global identifier %s is already declared", name);

        scope_add_sym(current_scope, name, sym_new(loc));
    }

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
        }
    }

    ast_unref(ast);

    struct rope *zc_prog_rope = zc_prog_decls_rope;
    zc_prog_rope = rope_new_tree(zc_prog_rope, zc_prog_defs_rope);
    rope_print_to_file(zc_prog_rope, fp);
}

char *gen_c_ident()
{
    static int n = 0;
    char *ident = malloc(13);
    snprintf(ident, 13, "id%d", n++);
    return ident;
}
