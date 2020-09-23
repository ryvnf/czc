#include "zc.h"

static struct expr eval_expr_(struct type *t, struct ast *ast, bool
        global_init);

const char *op_to_name(enum ast_tag tag)
{
    switch (tag) {
        case COMMA_EXPR:
            return "binary ,";
        case COND_EXPR:
            return "binary ?:";
        case ASGN_EXPR:
            return "binary =";
        case ADD_ASGN_EXPR:
            return "binary +=";
        case SUB_ASGN_EXPR:
            return "binary -=";
        case MUL_ASGN_EXPR:
            return "binary *=";
        case DIV_ASGN_EXPR:
            return "binary /=";
        case REM_ASGN_EXPR:
            return "binary %=";
        case OR_ASGN_EXPR:
            return "binary |=";
        case XOR_ASGN_EXPR:
            return "binary ~=";
        case AND_ASGN_EXPR:
            return "binary &=";
        case SHL_ASGN_EXPR:
            return "binary <<=";
        case SHR_ASGN_EXPR:
            return "binary >>=";
        case LOR_EXPR:
            return "binary ||";
        case LAND_EXPR:
            return "binary &&";
        case OR_EXPR:
            return "binary |";
        case XOR_EXPR:
            return "binary ~";
        case AND_EXPR:
            return "binary &";
        case EQ_EXPR:
            return "binary ==";
        case NE_EXPR:
            return "binary !=";
        case LT_EXPR:
            return "binary <";
        case GT_EXPR:
            return "binary >";
        case LE_EXPR:
            return "binary <=";
        case GE_EXPR:
            return "binary >=";
        case SHL_EXPR:
            return "binary <<";
        case SHR_EXPR:
            return "binary >>";
        case ADD_EXPR:
            return "binary +";
        case SUB_EXPR:
            return "binary -";
        case MUL_EXPR:
            return "binary *";
        case DIV_EXPR:
            return "binary /";
        case REM_EXPR:
            return "binary %";
        case NOT_EXPR:
            return "unary !";
        case COMPL_EXPR:
            return "unary ~";
        case UPLUS_EXPR:
            return "unary +";
        case NEG_EXPR:
            return "unary -";
        case REF_EXPR:
            return "reference ^";
        case DEREF_EXPR:
            return "dereference ^";
        case PRE_INC_EXPR:
            return "unary prefix ++";
        case PRE_DEC_EXPR:
            return "unary prefix --";
        case POST_INC_EXPR:
            return "unary postfix ++";
        case POST_DEC_EXPR:
            return "unary postfix --";
        case SIZEOF_EXPR:
            return "sizeof";
        case SUBSCR_EXPR:
            return "array subscript";
        case CALL_EXPR:
            return "function call";
        case MEMBER_EXPR:
            return "member access";
    }
    return ast_tag_name(tag);
}

int get_c_prec(enum ast_tag tag)
{
    switch (tag) {
        case COMMA_EXPR:
            return 0;
        case COND_EXPR:
            return 1;
        case ASGN_EXPR:
        case ADD_ASGN_EXPR:
        case SUB_ASGN_EXPR:
        case MUL_ASGN_EXPR:
        case DIV_ASGN_EXPR:
        case REM_ASGN_EXPR:
        case OR_ASGN_EXPR:
        case XOR_ASGN_EXPR:
        case AND_ASGN_EXPR:
        case SHL_ASGN_EXPR:
        case SHR_ASGN_EXPR:
            return 2;
        case LOR_EXPR:
            return 3;
        case LAND_EXPR:
            return 4;
        case OR_EXPR:
            return 5;
        case XOR_EXPR:
            return 6;
        case AND_EXPR:
            return 7;
        case EQ_EXPR:
        case NE_EXPR:
            return 8;
        case LT_EXPR:
        case GT_EXPR:
        case LE_EXPR:
        case GE_EXPR:
            return 9;
        case SHL_EXPR:
        case SHR_EXPR:
            return 10;
        case ADD_EXPR:
        case SUB_EXPR:
            return 11;
        case MUL_EXPR:
        case DIV_EXPR:
        case REM_EXPR:
            return 12;
        case CAST_EXPR:
            return 13;
        case NOT_EXPR:
        case COMPL_EXPR:
        case UPLUS_EXPR:
        case NEG_EXPR:
        case REF_EXPR:
        case PRE_INC_EXPR:
        case PRE_DEC_EXPR:
        case SIZEOF_EXPR:
        case DEREF_EXPR:
            return 14;
    }
    return 15;
}

bool is_rassoc(enum ast_tag tag)
{
    switch (tag) {
        case COND_EXPR:
        case ASGN_EXPR:
        case ADD_ASGN_EXPR:
        case SUB_ASGN_EXPR:
        case MUL_ASGN_EXPR:
        case DIV_ASGN_EXPR:
        case REM_ASGN_EXPR:
        case OR_ASGN_EXPR:
        case XOR_ASGN_EXPR:
        case AND_ASGN_EXPR:
        case SHL_ASGN_EXPR:
        case SHR_ASGN_EXPR:
            return true;
    }
    return false;
}

inline static struct rope *add_paren(struct rope *rope)
{
    rope = rope_new_tree(lparen_rope, rope);
    rope = rope_new_tree(rope, rparen_rope);
    return rope;
}

struct type *common_type(struct type *a, struct type *b)
{
    if ((is_int_type(a) && b->tag == INT_CONST_TYPE) ||
            (is_float_type(a) && b->tag == FLOAT_CONST_TYPE))
        return a;

    if (is_ptr_type(a) && is_ptr_type(b) && (is_void_type(ptr_type(b)->to)))
        return a;

    if ((is_int_type(b) && a->tag == INT_CONST_TYPE) ||
            (is_float_type(b) && a->tag == FLOAT_CONST_TYPE))
        return b;

    if (is_ptr_type(b) && is_ptr_type(a) && (is_void_type(ptr_type(a)->to)))
        return b;

    if (type_equals(a, b))
        return a;

    return NULL;
}

struct type *target_type(struct type *want, struct type *b)
{
    if ((is_int_type(want) && b->tag == INT_CONST_TYPE) ||
            (is_float_type(want) && b->tag == FLOAT_CONST_TYPE))
        return want;

    if (is_ptr_type(want) && is_ptr_type(b) && (is_void_type(ptr_type(b)->to)))
        return want;

    // TODO: Is this neccessary?
    //if ((is_int_type(b) && want->tag == INT_CONST_TYPE) ||
    //        (is_float_type(b) && want->tag == FLOAT_CONST_TYPE))
    //    return b;

    if (is_ptr_type(b) && is_ptr_type(want) && (is_void_type(ptr_type(want)->to)))
        return want;

    // pointer decay
    if (is_ptr_type(want) && is_array_type(b) &&
            type_equals(ptr_type(want)->to, array_type(b)->of))
        return want;

    if (type_equals(want, b))
        return want;

    return NULL;
}

struct type *ptr_decay(struct type *type)
{
    if (type_type(type->tag) == ARRAY_TYPE_FLAG)
        return new_ptr_type(array_type(type)->of);
    return type;
}

void invalid_type(struct ast *ast)
{
    fatal(ast->loc, "invalid operand types for %s", op_to_name(ast->tag));
}

void incompatible_type(struct ast *ast)
{
    fatal(ast->loc, "incompatible types for %s", op_to_name(ast->tag));
}

void not_lval(struct ast *ast)
{
    fatal(ast->loc, "operand to %s is not an lvalue", op_to_name(ast->tag));
}

void require_type(struct ast *ast, struct type *type, enum type_tag accept)
{
    if (!(type->tag & (accept & TYPE_MASK)))
        invalid_type(ast);
}

void require_lval(struct ast *ast, struct type *type)
{
    if (!is_lval_type(type))
        not_lval(ast);
}

struct type *eval_cond_type(struct type *t, struct ast *ast)
{
    struct type *cond_type = eval_type(NULL, ast_ast(ast, 0));
    struct type *then_type = eval_type(t, ast_ast(ast, 1));
    struct type *else_type = eval_type(t, ast_ast(ast, 2));

    require_type(ast, cond_type, BOOL_TYPE);

    struct type *type = common_type(then_type, else_type);
    if (type == NULL)
        incompatible_type(ast);

    return type;
}

struct type *eval_binop_type(struct ast *ast, enum ast_tag accept)
{
    struct type *lhs_type = eval_type(NULL, ast_ast(ast, 0));
    struct type *rhs_type = eval_type(NULL, ast_ast(ast, 1));
    struct type *type = common_type(lhs_type, rhs_type);
    if (type == NULL)
        incompatible_type(ast);

    require_type(ast, type, accept);
    return type_dup(type, 0);
}

struct type *eval_relop_type(struct ast *ast, enum ast_tag accept)
{
    struct type *lhs_type = eval_type(NULL, ast_ast(ast, 0));
    struct type *rhs_type = eval_type(NULL, ast_ast(ast, 1));
    struct type *type = common_type(lhs_type, rhs_type);
    if (type == NULL)
        incompatible_type(ast);

    require_type(ast, type, accept);
    return bool_type;
}

struct type *eval_unop_type(struct ast *ast, enum ast_tag accept, bool lval_op)
{
    struct type *type = eval_type(NULL, ast_ast(ast, 0));

    require_type(ast, type, accept);
    if (lval_op)
        require_lval(ast, type);
    return type_dup(type, 0);
}

struct type *eval_add_type(struct ast *ast)
{
    struct type *lhs_type = ptr_decay(eval_type(NULL, ast_ast(ast, 0)));
    struct type *rhs_type = ptr_decay(eval_type(NULL, ast_ast(ast, 1)));

    if (is_ptr_type(lhs_type) && is_int_type(rhs_type))
        return lhs_type;

    if (is_int_type(lhs_type) && is_ptr_type(rhs_type))
        return rhs_type;

    struct type *type = common_type(lhs_type, rhs_type);
    if (type == NULL)
        incompatible_type(ast);

    require_type(ast, type, INT_TYPE | FLOAT_TYPE);
    return type_dup(type, 0);
}

struct type *eval_sub_type(struct ast *ast)
{
    struct type *lhs_type = ptr_decay(eval_type(NULL, ast_ast(ast, 0)));
    struct type *rhs_type = ptr_decay(eval_type(NULL, ast_ast(ast, 1)));

    if (is_ptr_type(lhs_type) && is_int_type(rhs_type))
        return type_dup(lhs_type, 0);

    if (is_int_type(lhs_type) && is_ptr_type(rhs_type))
        return type_dup(rhs_type, 0);

    if (is_ptr_type(lhs_type) && is_ptr_type(rhs_type) &&
            type_equals(lhs_type, rhs_type))
        return const_int_type;

    struct type *type = common_type(lhs_type, rhs_type);
    if (type == NULL)
        incompatible_type(ast);

    require_type(ast, type, PTR_TYPE | INT_TYPE | FLOAT_TYPE);
    return type_dup(type, 0);
}

struct type *eval_asgn_type(struct ast *ast)
{
    struct type *lhs_type = eval_type(NULL, ast_ast(ast, 0));
    require_lval(ast, lhs_type);

    struct type *type;
    switch (ast->tag) {
        case ASGN_EXPR:
            type = target_type(lhs_type, eval_type(lhs_type, ast_ast(ast, 1)));
            if (type == NULL)
                incompatible_type(ast);

            require_type(ast, type, ~(VOID_TYPE_FLAG | FUNC_TYPE_FLAG));
            break;
        case ADD_ASGN_EXPR:
            type = ptr_decay(eval_add_type(ast));
            break;
        case SUB_ASGN_EXPR:
            type = ptr_decay(eval_sub_type(ast));
            break;
        case MUL_ASGN_EXPR:
        case DIV_ASGN_EXPR:
            type = ptr_decay(eval_binop_type(ast, INT_TYPE_FLAG | FLOAT_TYPE_FLAG));
            break;
        case REM_ASGN_EXPR:
        case OR_ASGN_EXPR:
        case XOR_ASGN_EXPR:
        case AND_ASGN_EXPR:
        case SHL_ASGN_EXPR:
        case SHR_ASGN_EXPR:
            type = ptr_decay(eval_binop_type(ast, INT_TYPE_FLAG));
            break;
    }

    if (!type_equals(lhs_type, type))
        incompatible_type(ast);

    return type_dup(type, LVAL_TYPE_FLAG);
}

struct type *eval_ref_type(struct ast *ast)
{
    struct type *type = eval_type(NULL, ast_ast(ast, 0));
    require_lval(ast, type);

    return new_ptr_type(type);
}

struct type *eval_deref_type(struct ast *ast)
{
    struct type *type = ptr_decay(eval_type(NULL, ast_ast(ast, 0)));
    require_type(ast, type, PTR_TYPE);

    return type_dup(ptr_type(type)->to, LVAL_TYPE_FLAG);
}

struct type *eval_sizeof_type(struct ast *ast)
{
    struct type *type = eval_type(NULL, ast_ast(ast, 0));

    require_type(ast, type, ~(VOID_TYPE_FLAG | FUNC_TYPE_FLAG));

    return const_int_type;
}

struct type *eval_cast_type(struct ast *ast)
{
    struct type *rhs_type = type_from_ast(ast_ast(ast, 1));
    eval_type(rhs_type, ast_ast(ast, 0));

    return type_dup(rhs_type, 0);
}

struct type *eval_subscr_type(struct ast *ast)
{
    struct type *lhs_type = ptr_decay(eval_type(NULL, ast_ast(ast, 0)));
    struct type *rhs_type = ptr_decay(eval_type(NULL, ast_ast(ast, 1)));

    if (is_ptr_type(lhs_type) && is_int_type(rhs_type))
        return type_dup(ptr_type(lhs_type)->to, LVAL_TYPE_FLAG);

    if (is_int_type(lhs_type) && is_ptr_type(rhs_type))
        return type_dup(ptr_type(lhs_type)->to, LVAL_TYPE_FLAG);

    invalid_type(ast);
    return NULL;
}

struct type *eval_call_type(struct ast *ast)
{
    struct type *called_type = eval_type(NULL, ast_ast(ast, 0));
    require_type(ast, called_type, FUNC_TYPE_FLAG);

    struct func_type *called_func_type = func_type(called_type);

    bool has_vararg = called_func_type->has_vararg;
    size_t n_param_types = called_func_type->n_params;
    struct type **param_types = called_func_type->params;

    size_t n_arg_asts;
    struct ast **arg_asts = ast_asts(ast_ast(ast, 1), &n_arg_asts);

    if ((n_arg_asts != n_param_types && !has_vararg) || n_arg_asts <
            n_param_types)
        fatal(ast->loc, "invalid number of arguments");

    for (size_t i = 0; i < n_param_types; i++) {
        struct type *got_type = eval_type(param_types[i], arg_asts[i]);
        struct type *type = target_type(param_types[i], got_type);

        if (type == NULL || !type_equals(param_types[i], type))
            fatal(ast->loc, "invalid type for argument %zu in function call", i + 1);
    }

    return called_func_type->ret;
}

struct type *eval_member_type(struct ast *ast)
{
    struct ast *lhs_ast = ast_ast(ast, 0);
    struct ast *rhs_ast = ast_ast(ast, 1);
    struct type *lhs_type = eval_type(NULL, lhs_ast);
    const char *member_id = ast_s(rhs_ast);

    if (type_type(lhs_type->tag) != STRUCT_TYPE_FLAG)
        incompatible_type(ast);

    size_t n_fields = struct_type(lhs_type)->n_fields;
    struct field *fields = struct_type(lhs_type)->fields;

    for (size_t i = 0; i < n_fields; i++) {
        if (strcmp(member_id, fields[i].name) == 0)
            return type_dup(fields[i].type, LVAL_TYPE_FLAG);
    }

    fatal(ast->loc, "structure has no member %s", member_id);
    return NULL;
}

struct type *eval_str_lit_type(struct ast *ast)
{
    size_t n_chars = 0;
    const char *s = ast_s(ast);

    for (size_t i = 0; s[i] != '\0'; ++i) {
        if (s[i] == '\\')
            i++;
        ++n_chars;
    }

    return new_array_type(char_type, n_chars + 1);
}

struct type *eval_name_type(struct type *t, struct ast *ast)
{
    const char *name = ast_s(ast);

    struct sym *sym = scope_get_sym(current_scope, name);

    if (sym == NULL)
        fatal(ast->loc, "undeclared identifier '%s'", name);

    if (sym->tag == DECL_SYM) {
        struct type *type = ((struct decl_sym *)sym)->type;
        if (type == NULL)
            fatal(ast->loc, "undeclared identifier '%s'", name);
        return type_dup(type, LVAL_TYPE_FLAG);
    } else if (sym->tag == ALIAS_SYM) {
        struct expr expr = eval_expr(t, ((struct alias_sym *)sym)->ast);
        expr.rope = add_paren(expr.rope);
        return expr.type;
    }

    unreachable();
    return NULL;
}

struct type *eval_decl_type(struct ast *ast)
{
    struct ast *name_ast = ast_ast(ast, 0);
    const char *name = ast_s(name_ast);

    struct sym *sym = scope_get_sym(current_scope, name);

    if (!sym || sym->loc != ast) {
        sym = sym_from_ast(ast);
        add_local_decl((struct decl_sym *)sym);
        scope_add_sym(current_scope, name, sym);
    }

    return eval_name_type(NULL, name_ast);
}

struct type *eval_init_type(struct type *t, struct ast *ast)
{
    size_t n_childs;
    struct ast **childs = ast_asts(ast, &n_childs);

    if (is_array_type(t)) {
        struct array_type *a_t = array_type(t);

        if (a_t->len < n_childs)
            fatal(ast->loc, "excess elements in array initializer");

        for (size_t i = 0; i < n_childs; i++) {
            struct type *got_type = eval_type(a_t->of, childs[i]);
            struct type *type = target_type(a_t->of, got_type);

            if (type == NULL || !type_equals(a_t->of, type))
                fatal(ast->loc, "invalid type in array initializer");
        }
    } else if (is_struct_type(t)) {
        struct struct_type *s_t = struct_type(t);

        if (s_t->n_fields < n_childs)
            fatal(ast->loc, "excess elements in struct initializer");

        for (size_t i = 0; i < n_childs; i++) {
            struct type *field_type = s_t->fields[i].type;
            struct type *got_type = eval_type(field_type, childs[i]);
            struct type *type = target_type(field_type, got_type);

            if (type == NULL || !type_equals(field_type, type))
                fatal(ast->loc, "invalid type in struct initializer");
        }
    } else {
        fatal(ast->loc, "invalid type");
    }

    return type_dup(t, 0);
}

struct type *eval_type(struct type *t, struct ast *ast)
{
    struct type *type;
    switch (ast->tag) {
        case COMMA_EXPR:
            eval_type(NULL, ast_ast(ast, 0));
            type = eval_type(t, ast_ast(ast, 1));
            break;

        case COND_EXPR:
            type = eval_cond_type(t, ast);
            break;

        case ASGN_EXPR:
        case ADD_ASGN_EXPR:
        case SUB_ASGN_EXPR:
        case MUL_ASGN_EXPR:
        case DIV_ASGN_EXPR:
        case REM_ASGN_EXPR:
        case OR_ASGN_EXPR:
        case XOR_ASGN_EXPR:
        case AND_ASGN_EXPR:
        case SHL_ASGN_EXPR:
        case SHR_ASGN_EXPR:
            type = eval_asgn_type(ast);
            break;

        case LOR_EXPR:
        case LAND_EXPR:
            type = eval_binop_type(ast, BOOL_TYPE_FLAG);
            break;

        case EQ_EXPR:
        case NE_EXPR:
            type = eval_relop_type(ast, BOOL_TYPE_FLAG | INT_TYPE_FLAG | FLOAT_TYPE_FLAG |
                    PTR_TYPE_FLAG);
            break;

        case LT_EXPR:
        case GT_EXPR:
        case LE_EXPR:
        case GE_EXPR:
            type = eval_relop_type(ast, INT_TYPE_FLAG | FLOAT_TYPE_FLAG | PTR_TYPE_FLAG);
            break;

        case OR_EXPR:
        case XOR_EXPR:
        case AND_EXPR:
        case SHL_EXPR:
        case SHR_EXPR:
        case REM_EXPR:
            type = eval_binop_type(ast, INT_TYPE_FLAG);
            break;

        case ADD_EXPR:
            type = eval_add_type(ast);
            break;

        case SUB_EXPR:
            type = eval_sub_type(ast);
            break;

        case MUL_EXPR:
        case DIV_EXPR:
            type = eval_binop_type(ast, INT_TYPE_FLAG | FLOAT_TYPE_FLAG);
            break;

        case NOT_EXPR:
            type = eval_unop_type(ast, BOOL_TYPE_FLAG, false);
            break;

        case COMPL_EXPR:
            type = eval_unop_type(ast, INT_TYPE_FLAG, false);
            break;

        case UPLUS_EXPR:
        case NEG_EXPR:
            type = eval_unop_type(ast, INT_TYPE_FLAG | FLOAT_TYPE_FLAG, false);
            break;

        case REF_EXPR:
            type = eval_ref_type(ast);
            break;

        case PRE_INC_EXPR:
        case PRE_DEC_EXPR:
        case POST_INC_EXPR:
        case POST_DEC_EXPR:
            type = eval_unop_type(ast, INT_TYPE_FLAG | FLOAT_TYPE_FLAG | PTR_TYPE_FLAG, true);
            break;

        case SIZEOF_EXPR:
            type = eval_sizeof_type(ast);
            break;

        case CAST_EXPR:
            type = eval_cast_type(ast);
            break;

        case DEREF_EXPR:
            type = eval_deref_type(ast);
            break;

        case SUBSCR_EXPR:
            type = eval_subscr_type(ast);
            break;

        case CALL_EXPR:
            type = eval_call_type(ast);
            break;

        case MEMBER_EXPR:
            type = eval_member_type(ast);
            break;

        case INT_CONST:
        case CHAR_LIT:
            type = const_int_type;
            break;

        case FLOAT_CONST:
            type = const_float_type;
            break;

        case STR_LIT:
            type = eval_str_lit_type(ast);
            break;

        case TRUE_CONST:
            type = bool_type;
            break;

        case FALSE_CONST:
            type = bool_type;
            break;

        case NULL_CONST:
            type = void_ptr_type;
            break;

        case NAME:
            type = eval_name_type(t, ast);
            break;

        case DECL:
            type = eval_decl_type(ast);
            break;

        case INIT_EXPR:
            type = eval_init_type(t, ast);
            break;
    }

    if (type && type_type(type->tag) == SELFREF_TYPE_FLAG)
        fatal(ast->loc, "type cannot refer to itself in this context");

    return type;
}

size_t alignof_type(struct loc *loc, struct type *type)
{
    switch (type_type(type->tag)) {
        case VOID_TYPE_FLAG:
            fatal(loc, "cannot compute the size of void");
            return 0;
        case FUNC_TYPE_FLAG:
            fatal(loc, "cannot compute the size of a function");
            return 0;
        case BOOL_TYPE_FLAG:
        case INT_TYPE_FLAG:
        case FLOAT_TYPE_FLAG:
            return sizeof_basic_type(type);
        case PTR_TYPE_FLAG:
            // This would not be portable on computers where size of function
            // pointers is different from regular pointers.
            return sizeof (void *);
        case ARRAY_TYPE_FLAG: {
            struct type *elem_type = array_type(type)->of;
            return alignof_type(loc, elem_type);
        }
        case STRUCT_TYPE_FLAG: {
            size_t n_fields = struct_type(type)->n_fields;
            struct field *fields = struct_type(type)->fields;
            size_t max_align = 1;

            for (size_t i = 0; i < n_fields; i++) {
                size_t align = alignof_type(loc, fields[i].type);
                if (align > max_align)
                    max_align = align;
            }

            return max_align;
        }
    }

    unreachable();
    return 0;
}

size_t align_size(size_t size, size_t align)
{
    if (size % align == 0)
        return size;

    return size + (align - size % align);
}

/**
 * TODO: The size calculating will probably be wrong on some architectures.
 * Hopefully it should be accurate on desktop comptuers.
 */
size_t sizeof_type(struct loc *loc, struct type *type)
{
    switch (type_type(type->tag)) {
        case VOID_TYPE_FLAG:
            fatal(loc, "cannot compute the size of void");
            return 0;
        case FUNC_TYPE_FLAG:
            fatal(loc, "cannot compute the size of a function");
            return 0;
        case BOOL_TYPE_FLAG:
        case INT_TYPE_FLAG:
        case FLOAT_TYPE_FLAG:
            return sizeof_basic_type(type);
        case PTR_TYPE_FLAG:
            // This would not be portable on computers where size of function
            // pointers is different from regular pointers.
            return sizeof (void *);
        case ARRAY_TYPE_FLAG: {
            struct type *elem_type = array_type(type)->of;
            size_t elem_size = sizeof_type(loc, elem_type);
            size_t elem_align = alignof_type(loc, elem_type);
            size_t n_elems = array_type(type)->len;
            return align_size(elem_size, elem_align) * n_elems;
        }
        case STRUCT_TYPE_FLAG: {
            size_t n_fields = struct_type(type)->n_fields;
            struct field *fields = struct_type(type)->fields;
            size_t size = 0;

            for (size_t i = 0; i < n_fields; i++) {
                size_t field_size = sizeof_type(loc, fields[i].type);
                size_t field_align = alignof_type(loc, fields[i].type);
                size += align_size(field_size, field_align);
            }

            return size;
        }
    }

    unreachable();
    return 0;
}

size_t eval_name_size(struct ast *ast)
{
    const char *name = ast_s(ast);

    struct sym *sym = scope_get_sym(current_scope, name);
    if (sym == NULL)
        fatal(ast->loc, "undeclared identifier '%s'", name);

    if (sym->tag != ALIAS_SYM) {
        fatal(ast->loc, "accessing variables or functions is allowed in this "
                "context");
        unreachable();
    }

    return eval_size(((struct alias_sym *)sym)->ast);
}

size_t eval_size(struct ast *ast)
{
    switch (ast->tag) {
        case COMMA_EXPR:
            eval_size(ast_ast(ast, 0));
            return eval_size(ast_ast(ast, 1));

        case OR_EXPR:
            return eval_size(ast_ast(ast, 0)) | eval_size(ast_ast(ast, 1));

        case XOR_EXPR:
            return eval_size(ast_ast(ast, 0)) ^ eval_size(ast_ast(ast, 1));

        case AND_EXPR:
            return eval_size(ast_ast(ast, 0)) & eval_size(ast_ast(ast, 1));

        case SHL_EXPR:
            return eval_size(ast_ast(ast, 0)) << eval_size(ast_ast(ast, 1));

        case SHR_EXPR:
            return eval_size(ast_ast(ast, 0)) >> eval_size(ast_ast(ast, 1));

        case ADD_EXPR:
            return eval_size(ast_ast(ast, 0)) + eval_size(ast_ast(ast, 1));

        case SUB_EXPR:
            return eval_size(ast_ast(ast, 0)) - eval_size(ast_ast(ast, 1));

        case MUL_EXPR:
            return eval_size(ast_ast(ast, 0)) * eval_size(ast_ast(ast, 1));

        case DIV_EXPR:
            return eval_size(ast_ast(ast, 0)) / eval_size(ast_ast(ast, 1));

        case REM_EXPR:
            return eval_size(ast_ast(ast, 0)) % eval_size(ast_ast(ast, 1));

        case COMPL_EXPR:
            return ~eval_size(ast_ast(ast, 0));

        case UPLUS_EXPR:
            return +eval_size(ast_ast(ast, 0));

        case NEG_EXPR:
            return -eval_size(ast_ast(ast, 0));

        case SIZEOF_EXPR:
            return sizeof_type(ast->loc, eval_type(NULL, ast_ast(ast, 0)));

        case INT_CONST:
            return ast_i(ast);

        case NAME:
            return eval_name_size(ast);

        case TRUE_TOK:
        case FALSE_TOK:
            fatal(ast->loc, "boolean constants are not implemented in this "
                    "context (yet)");
            unreachable();
            return 0;

        case NULL_TOK:
            fatal(ast->loc, "pointers are not allowed in this context (yet)");
            unreachable();
            return 0;
    }

    const char *op_name = op_to_name(ast->tag);
    if (op_name) {
        fatal(ast->loc, "%s operator is allowed in this context (yet)",
                op_name);
    }

    return 0;
}

struct expr eval_asgn_expr(struct ast *ast)
{
    struct ast *lhs_ast = ast_ast(ast, 0);
    struct ast *rhs_ast = ast_ast(ast, 1);
    struct expr lhs_expr = eval_expr(NULL, lhs_ast);
    struct expr rhs_expr = eval_expr(lhs_expr.type, rhs_ast);

    // if lhs is array, generate memcpy instead
    if (is_array_type(lhs_expr.type)) {
        struct rope *rope = memcpy_lparen_rope;
        rope = rope_new_tree(rope, lhs_expr.rope);
        rope = rope_new_tree(rope, comma_sp_rope);
        rope = rope_new_tree(rope, rhs_expr.rope);
        rope = rope_new_tree(rope, comma_sp_rope);
        rope = rope_new_tree(rope, sizeof_sp_rope);
        rope = rope_new_tree(rope, lparen_rope);
        rope = rope_new_tree(rope, type_to_c(NULL, lhs_expr.type));
        rope = rope_new_tree(rope, rparen_rope);
        rope = rope_new_tree(rope, rparen_rope);

        return (struct expr) {
            .rope = rope,
            .type = eval_type(NULL, ast)
        };
    }

    int prec = get_c_prec(ast->tag);
    int lhs_prec = get_c_prec(lhs_ast->tag);
    int rhs_prec = get_c_prec(rhs_ast->tag);

    if (lhs_prec < prec || (lhs_prec == prec && is_rassoc(ast->tag)))
        lhs_expr.rope = add_paren(lhs_expr.rope);

    if (rhs_prec < prec || (rhs_prec == prec && !is_rassoc(ast->tag)))
        rhs_expr.rope = add_paren(rhs_expr.rope);

    struct rope *rope = lhs_expr.rope;
    rope = rope_new_tree(rope, asgn_binop_rope);
    rope = rope_new_tree(rope, rhs_expr.rope);

    return (struct expr) { 
        .rope = rope,
        .type = eval_type(NULL, ast)
    };
}

struct expr eval_comma_expr(struct type *t, struct ast *ast)
{
    struct ast *lhs_ast = ast_ast(ast, 0);
    struct ast *rhs_ast = ast_ast(ast, 1);
    struct expr lhs_expr = eval_expr(NULL, lhs_ast);
    struct expr rhs_expr = eval_expr(t, rhs_ast);

    int prec = get_c_prec(ast->tag);
    int lhs_prec = get_c_prec(lhs_ast->tag);
    int rhs_prec = get_c_prec(rhs_ast->tag);

    if (lhs_prec < prec || (lhs_prec == prec && is_rassoc(ast->tag)))
        lhs_expr.rope = add_paren(lhs_expr.rope);

    if (rhs_prec < prec || (rhs_prec == prec && !is_rassoc(ast->tag)))
        rhs_expr.rope = add_paren(rhs_expr.rope);

    struct rope *rope = lhs_expr.rope;
    rope = rope_new_tree(rope, comma_sp_rope);
    rope = rope_new_tree(rope, rhs_expr.rope);

    return (struct expr) { 
        .rope = rope,
        .type = eval_type(NULL, ast)
    };
}


struct expr eval_binop_expr(struct ast *ast, struct rope *c_op)
{
    struct ast *lhs_ast = ast_ast(ast, 0);
    struct ast *rhs_ast = ast_ast(ast, 1);
    struct expr lhs_expr = eval_expr(NULL, lhs_ast);
    struct expr rhs_expr = eval_expr(NULL, rhs_ast);

    int prec = get_c_prec(ast->tag);
    int lhs_prec = get_c_prec(lhs_ast->tag);
    int rhs_prec = get_c_prec(rhs_ast->tag);

    if (lhs_prec < prec || (lhs_prec == prec && is_rassoc(ast->tag)))
        lhs_expr.rope = add_paren(lhs_expr.rope);

    if (rhs_prec < prec || (rhs_prec == prec && !is_rassoc(ast->tag)))
        rhs_expr.rope = add_paren(rhs_expr.rope);

    struct rope *rope = lhs_expr.rope;
    rope = rope_new_tree(rope, c_op);
    rope = rope_new_tree(rope, rhs_expr.rope);

    return (struct expr) { 
        .rope = rope,
        .type = eval_type(NULL, ast)
    };
}

struct expr eval_preop_expr(struct ast *ast, struct rope *c_op)
{
    struct ast *expr_ast = ast_ast(ast, 0);
    struct expr expr = eval_expr(NULL, expr_ast);
    if (get_c_prec(expr_ast->tag) < get_c_prec(ast->tag))
        expr.rope = add_paren(expr.rope);

    return (struct expr) { 
        .rope = rope_new_tree(c_op, expr.rope),
        .type = eval_type(NULL, ast)
    };
}

struct expr eval_postop_expr(struct ast *ast, struct rope *c_op)
{
    struct ast *expr_ast = ast_ast(ast, 0);
    struct expr expr = eval_expr(NULL, expr_ast);
    if (get_c_prec(expr_ast->tag) < get_c_prec(ast->tag))
        expr.rope = add_paren(expr.rope);

    return (struct expr) { 
        .rope = rope_new_tree(expr.rope, c_op),
        .type = eval_type(NULL, ast)
    };
}

struct expr eval_cond_expr(struct type *t, struct ast *ast)
{
    struct type *type = eval_type(NULL, ast);
    struct ast *cond_ast = ast_ast(ast, 0);
    struct ast *then_ast = ast_ast(ast, 1);
    struct ast *else_ast = ast_ast(ast, 2);
    struct expr cond_expr = eval_expr(NULL, cond_ast);
    struct expr then_expr = eval_expr(t, then_ast);
    struct expr else_expr = eval_expr(t, else_ast);

    int prec = get_c_prec(ast->tag);
    int cond_prec = get_c_prec(cond_ast->tag);
    int else_prec = get_c_prec(else_ast->tag);

    if (cond_prec <= prec)
        cond_expr.rope = add_paren(cond_expr.rope);

    if (else_prec < prec)
        else_expr.rope = add_paren(else_expr.rope);

    struct rope *rope = cond_expr.rope;
    rope = rope_new_tree(rope, rope_new_s(" ? "));
    rope = rope_new_tree(rope, then_expr.rope);
    rope = rope_new_tree(rope, rope_new_s(" : "));
    rope = rope_new_tree(rope, else_expr.rope);

    return (struct expr){
        .type = type,
        .rope = rope
    };
}

struct expr eval_sizeof_expr(struct ast *ast)
{
    struct expr expr = eval_expr(NULL, ast_ast(ast, 0));
    struct rope *rope = rope_new_tree(sizeof_sp_rope, expr.rope);

    return (struct expr){ .type = const_int_type, .rope = rope };
}

struct expr eval_cast_expr(struct ast *ast)
{
    bug("TODO");
    return (struct expr){ 0 };
}

struct expr eval_call_expr(struct ast *ast)
{
    struct type *type = eval_type(NULL, ast);
    struct expr called_expr = eval_expr(NULL, ast_ast(ast, 0));
    struct rope *rope = called_expr.rope;


    size_t n_arg_asts;
    struct ast **arg_asts = ast_asts(ast_ast(ast, 1), &n_arg_asts);

    rope = rope_new_tree(rope, rope_new_s("("));

    struct func_type *called_type = func_type(called_expr.type);

    for (size_t i = 0; i < n_arg_asts; i++) {
        struct expr arg_expr = eval_expr(called_type->params[i], arg_asts[i]);

        rope = rope_new_tree(rope, arg_expr.rope);

        if (i != n_arg_asts - 1)
            rope = rope_new_tree(rope, rope_new_s(", "));
    }

    rope = rope_new_tree(rope, rope_new_s(")"));

    return (struct expr){ .rope = rope, .type = type };
}

struct expr eval_subscr_expr(struct ast *ast)
{
    struct ast *lhs_ast = ast_ast(ast, 0);
    struct ast *rhs_ast = ast_ast(ast, 1);
    struct expr lhs_expr = eval_expr(NULL, lhs_ast);
    struct expr rhs_expr = eval_expr(NULL, rhs_ast);

    if (get_c_prec(lhs_ast->tag) < get_c_prec(ast->tag))
        lhs_expr.rope = add_paren(lhs_expr.rope);

    struct rope *rope = lhs_expr.rope;
    rope = rope_new_tree(rope, lsquare_rope);
    rope = rope_new_tree(rope, rhs_expr.rope);
    rope = rope_new_tree(rope, rsquare_rope);

    return (struct expr){ .rope = rope, .type = eval_type(NULL, ast) };
}

struct expr eval_member_expr(struct ast *ast)
{
    struct ast *lhs_ast = ast_ast(ast, 0);
    struct ast *rhs_ast = ast_ast(ast, 1);
    struct expr lhs_expr = eval_expr(NULL, lhs_ast);
    const char *member_id = ast_s(rhs_ast);

    int prec = get_c_prec(MEMBER_EXPR);
    int lhs_prec = get_c_prec(lhs_ast->tag);

    if (lhs_prec < prec || (lhs_prec == prec && is_rassoc(ast->tag)))
        lhs_expr.rope = add_paren(lhs_expr.rope);

    struct rope *rope = lhs_expr.rope;
    rope = rope_new_tree(rope, dot_rope);
    rope = rope_new_tree(rope, rope_new_s(member_id));

    return (struct expr) { 
        .rope = rope,
        .type = eval_type(NULL, ast)
    };
}

struct expr eval_name_expr(struct type *t, struct ast *ast)
{
    const char *name = ast_s(ast);
    struct sym *sym = scope_get_sym(current_scope, name);
    if (sym == NULL)
        fatal(ast->loc, "undeclared identifier '%s'", name);

    switch (sym->tag) {
        case DECL_SYM:
            return (struct expr) {
                .rope = rope_new_s(((struct decl_sym *)sym)->c_name),
                .type = ((struct decl_sym *)sym)->type
            };
        case ALIAS_SYM:
            return eval_expr(t, ((struct alias_sym *)sym)->ast);
    }
    abort();
    return (struct expr){ 0 };
}

struct expr eval_int_const_expr(struct ast *ast)
{
    long long int i = ast_i(ast);
    return (struct expr) {
        .rope = rope_new_fmt("%lld", i),
        .type = const_int_type
    };
}

struct expr eval_float_const_expr(struct ast *ast)
{
    return (struct expr) {
        .type = eval_type(NULL, ast),
        .rope = rope_new_fmt("%f", ast_f(ast))
    };
}

struct expr eval_str_lit_expr(struct ast *ast)
{
    return (struct expr) {
        .type = eval_type(NULL, ast),
        .rope = rope_new_fmt("\"%s\"", ast_s(ast))
    };
}

struct expr eval_char_lit_expr(struct ast *ast)
{
    return (struct expr) {
        .type = eval_type(NULL, ast),
        .rope = rope_new_fmt("\'%s\'", ast_s(ast))
    };
}

struct expr eval_decl_expr(struct ast *ast)
{
    // First evaluate type, which will add the declaration to symbol table.
    struct ast *name_ast = ast_ast(ast, 0);
    struct type *type = eval_decl_type(ast);

    // Then evalute the name of the declared symbol.
    return eval_name_expr(NULL, name_ast);
}

struct expr eval_init_expr(struct type *t, struct ast *ast, bool global_init)
{
    struct type *type = eval_type(t, ast);

    size_t n_childs;
    struct ast **childs = ast_asts(ast, &n_childs);

    struct rope *rope;
   
    // If this is not part of a global variable initializer, make this
    // initializer a compound expression.
    if (!global_init) {
        rope = lparen_rope;
        rope = rope_new_tree(rope, type_to_c(NULL, t));
        rope = rope_new_tree(rope, rparen_sp_rope);
        rope = rope_new_tree(rope, lcurly_sp_rope);
    } else {
        rope = lcurly_sp_rope;
    }

    if (is_array_type(t)) {
        struct array_type *a_t = array_type(t);

        for (size_t i = 0; i < n_childs; i++) {
            struct expr expr = eval_expr_(a_t->of, childs[i], global_init);
            rope = rope_new_tree(rope, expr.rope);

            if (i != n_childs - 1)
                rope = rope_new_tree(rope, comma_sp_rope);
            else
                rope = rope_new_tree(rope, sp_rope);
        }
    } else if (is_struct_type(t)) {
        struct struct_type *s_t = struct_type(t);

        for (size_t i = 0; i < n_childs; i++) {
            struct expr expr = eval_expr_(s_t->fields[i].type, childs[i], global_init);
            rope = rope_new_tree(rope, expr.rope);

            if (i != n_childs - 1)
                rope = rope_new_tree(rope, comma_sp_rope);
            else
                rope = rope_new_tree(rope, sp_rope);
        }
    }

    rope = rope_new_tree(rope, rcurly_rope);

    return (struct expr) { .type = type, .rope = rope };
}

static struct expr eval_expr_(struct type *t, struct ast *ast, bool global_init)
{
    switch (ast->tag) {
        case COMMA_EXPR:
            return eval_comma_expr(t, ast);

        case COND_EXPR:
            return eval_cond_expr(t, ast);

        case ASGN_EXPR:
            return eval_asgn_expr(ast);
        case ADD_ASGN_EXPR:
            return eval_binop_expr(ast, addasgn_binop_rope);
        case SUB_ASGN_EXPR:
            return eval_binop_expr(ast, subasgn_binop_rope);
        case MUL_ASGN_EXPR:
            return eval_binop_expr(ast, mulasgn_binop_rope);
        case DIV_ASGN_EXPR:
            return eval_binop_expr(ast, divasgn_binop_rope);
        case REM_ASGN_EXPR:
            return eval_binop_expr(ast, remasgn_binop_rope);
        case OR_ASGN_EXPR:
            return eval_binop_expr(ast, orasgn_binop_rope);
        case XOR_ASGN_EXPR:
            return eval_binop_expr(ast, xorasgn_binop_rope);
        case AND_ASGN_EXPR:
            return eval_binop_expr(ast, andasgn_binop_rope);
        case SHL_ASGN_EXPR:
            return eval_binop_expr(ast, shlasgn_binop_rope);
        case SHR_ASGN_EXPR:
            return eval_binop_expr(ast, shrasgn_binop_rope);

        case LOR_EXPR:
            return eval_binop_expr(ast, lor_binop_rope);
        case LAND_EXPR:
            return eval_binop_expr(ast, land_binop_rope);

        case EQ_EXPR:
            return eval_binop_expr(ast, eq_binop_rope);
        case NE_EXPR:
            return eval_binop_expr(ast, ne_binop_rope);

        case LT_EXPR:
            return eval_binop_expr(ast, lt_binop_rope);
        case GT_EXPR:
            return eval_binop_expr(ast, gt_binop_rope);
        case LE_EXPR:
            return eval_binop_expr(ast, le_binop_rope);
        case GE_EXPR:
            return eval_binop_expr(ast, ge_binop_rope);

        case OR_EXPR:
            return eval_binop_expr(ast, or_binop_rope);
        case XOR_EXPR:
            return eval_binop_expr(ast, xor_binop_rope);
        case AND_EXPR:
            return eval_binop_expr(ast, and_binop_rope);
        case SHL_EXPR:
            return eval_binop_expr(ast, shl_binop_rope);
        case SHR_EXPR:
            return eval_binop_expr(ast, shr_binop_rope);

        case ADD_EXPR:
            return eval_binop_expr(ast, add_binop_rope);

        case SUB_EXPR:
            return eval_binop_expr(ast, sub_binop_rope);

        case MUL_EXPR:
            return eval_binop_expr(ast, mul_binop_rope);
        case DIV_EXPR:
            return eval_binop_expr(ast, div_binop_rope);
        case REM_EXPR:
            return eval_binop_expr(ast, rem_binop_rope);

        case NOT_EXPR:
            return eval_preop_expr(ast, not_rope);

        case COMPL_EXPR:
            return eval_preop_expr(ast, compl_rope);

        case UPLUS_EXPR:
            return eval_preop_expr(ast, plus_rope);
        case NEG_EXPR:
            return eval_preop_expr(ast, minus_rope);

        case REF_EXPR:
            return eval_preop_expr(ast, amp_rope);

        case PRE_INC_EXPR:
            return eval_preop_expr(ast, inc_rope);
        case PRE_DEC_EXPR:
            return eval_preop_expr(ast, dec_rope);

        case SIZEOF_EXPR:
            return eval_sizeof_expr(ast);

        case CAST_EXPR:
            return eval_cast_expr(ast);

        case DEREF_EXPR:
            return eval_preop_expr(ast, star_rope);

        case SUBSCR_EXPR:
            return eval_subscr_expr(ast);

        case CALL_EXPR:
            return eval_call_expr(ast);

        case POST_INC_EXPR:
            return eval_postop_expr(ast, inc_rope);
        case POST_DEC_EXPR:
            return eval_postop_expr(ast, dec_rope);

        case MEMBER_EXPR:
            return eval_member_expr(ast);

        case INT_CONST:
            return eval_int_const_expr(ast);

        case FLOAT_CONST:
            return eval_float_const_expr(ast);

        case STR_LIT:
            return eval_str_lit_expr(ast);

        case CHAR_LIT:
            return eval_char_lit_expr(ast);

        case TRUE_CONST:
            return (struct expr) { .rope = one_rope, .type = eval_type(NULL, ast) };

        case FALSE_CONST:
            return (struct expr) { .rope = zero_rope, .type = eval_type(NULL, ast) };

        case NULL_CONST:
            return (struct expr) { .rope = zero_rope, .type = eval_type(NULL, ast) };

        case NAME:
            return eval_name_expr(t, ast);

        case DECL:
            return eval_decl_expr(ast);

        case INIT_EXPR:
            return eval_init_expr(t, ast, global_init);
    }
    return (struct expr){ 0 };
}

struct expr eval_expr(struct type *t, struct ast *ast)
{
    return eval_expr_(t, ast, false);
}

struct expr eval_expr_global(struct type *t, struct ast *ast)
{
    return eval_expr_(t, ast, true);
}
