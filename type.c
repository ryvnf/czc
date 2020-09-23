#include "zc.h"

// Basic types
struct type int_type[1] = { { INT_TYPE } };
struct type int8_type[1] = { { INT8_TYPE } };
struct type int16_type[1] = { { INT16_TYPE } };
struct type int32_type[1] = { { INT32_TYPE } };
struct type int64_type[1] = { { INT64_TYPE } };
struct type intptr_type[1] = { { INT64_TYPE } };
struct type ssize_type[1] = { { SSIZE_TYPE } };
struct type uint_type[1] = { { UINT_TYPE } };
struct type uint8_type[1] = { { UINT8_TYPE } };
struct type uint16_type[1] = { { UINT16_TYPE } };
struct type uint32_type[1] = { { UINT32_TYPE } };
struct type uint64_type[1] = { { UINT64_TYPE } };
struct type uintptr_type[1] = { { UINT64_TYPE } };
struct type size_type[1] = { { SIZE_TYPE } };
struct type float_type[1] = { { FLOAT_TYPE } };
struct type double_type[1] = { { DOUBLE_TYPE } };
struct type const_int_type[1] = { { INT_CONST_TYPE } };
struct type const_float_type[1] = { { FLOAT_CONST_TYPE } };
struct type bool_type[1] = { { BOOL_TYPE } };
struct type char_type[1] = { { CHAR_TYPE } };
struct type void_type[1] = { { VOID_TYPE } };
struct type *void_ptr_type = (struct type *)&(struct ptr_type){ { PTR_TYPE }, void_type };

// Compute the size of a basic type
size_t sizeof_basic_type(struct type *type)
{
    switch (type->tag & ~LVAL_TYPE_FLAG) {
        case INT8_TYPE:
        case UINT8_TYPE:
        case CHAR_TYPE:
        case BOOL_TYPE:
            return 1;

        case INT16_TYPE:
        case UINT16_TYPE:
            return 2;

        case UINT_TYPE:
        case INT_TYPE:
            return sizeof (int);

        case UINT32_TYPE:
        case INT32_TYPE:
            return 4;

        case SSIZE_TYPE:
        case SIZE_TYPE:
            return sizeof (size_t);

        case INT64_TYPE:
        case UINT64_TYPE:
            return 8;

        case INTPTR_TYPE:
        case UINTPTR_TYPE:
            return sizeof (intptr_t);

        case FLOAT_TYPE:
            return sizeof (float);

        case DOUBLE_TYPE:
            return sizeof (double);

        default:
            bug("Invalid argument to sizeof_basic_type: %x", type->tag);
    }
}

// Get the C integer type with specified signedness and size.
const char *int_to_c_type(bool is_signed, size_t size)
{
    assert(1 == sizeof (char));
    assert(2 == sizeof (short));
    assert(4 == sizeof (int));
    assert(8 == sizeof (long long));

    if (is_signed) {
        switch (size) {
            case 1:
                return "signed char";
            case 2:
                return "short";
            case 4:
                return "int";
            case 8:
                return "long long";
            default:
                unreachable();
        }
    } else {
        switch (size) {
            case 1:
                return "unsigned char";
            case 2:
                return "unsigned short";
            case 4:
                return "unsigned int";
            case 8:
                return "unsigned long long";
            default:
                unreachable();
        }
    }
}

// Get the C type for a basic type
const char *basic_type_c_name(struct type *type, bool for_abi)
{
    switch (type->tag & ~LVAL_TYPE_FLAG) {
        case EXTERN_TYPE:
        case VOID_TYPE:
            return "void";

        case CHAR_TYPE:
            if (for_abi)
                return "int";
            return "char";

        case BOOL_TYPE:
            if (for_abi)
                return "int";
            return "_Bool";

        case UINT_TYPE:
        case UINT8_TYPE:
        case UINT16_TYPE:
        case UINT32_TYPE:
        case UINT64_TYPE:
        case SIZE_TYPE:
        case UINTPTR_TYPE: {
            size_t n = sizeof_basic_type(type);

            if (for_abi) {
                size_t uint_size = sizeof_basic_type(uint_type);
                if (n < uint_size)
                    n = uint_size;
            }

            return int_to_c_type(false, n);
        }

        case INT_TYPE:
        case INT8_TYPE:
        case INT16_TYPE:
        case INT32_TYPE:
        case INT64_TYPE:
        case SSIZE_TYPE:
        case INTPTR_TYPE: {
            size_t n = sizeof_basic_type(type);

            if (for_abi) {
                size_t int_size = sizeof_basic_type(int_type);
                if (n < int_size)
                    n = int_size;
            }
            return int_to_c_type(true, sizeof_basic_type(type));
        }

        case FLOAT_TYPE:
            return "float";

        case DOUBLE_TYPE:
            return "double";

        default:
            bug("Invalid argument to basic_type_c_name: %x", type->tag);
    }
}

struct type *new_selfref_type(struct type_sym *sym)
{
    struct selfref_type *type = malloc(sizeof *type);
    type->type.tag = SELFREF_TYPE_FLAG;
    type->sym = sym;

    return (struct type *)type;
}

struct type *new_ptr_type(struct type *to)
{
    struct ptr_type *type = malloc(sizeof *type);
    type->type.tag = PTR_TYPE;
    type->to = to;

    return (struct type *)type;
}

struct type *new_array_type(struct type *of, size_t len)
{
    struct array_type *type = malloc(sizeof *type);
    type->type.tag = ARRAY_TYPE_FLAG;
    type->of = of;
    type->len = len;

    return (struct type *)type;
}

struct type *new_func_type(struct type *ret, size_t n_params,
        struct type *params[], bool has_vararg)
{
    struct func_type *type = malloc(sizeof *type +
            n_params * sizeof *type->params);
    type->type.tag = FUNC_TYPE_FLAG;
    type->ret = ret;
    type->n_params = n_params;
    type->has_vararg = has_vararg;
    
    for (size_t i = 0; i < n_params; i++) {
        type->params[i] = params[i];
    }

    return (struct type *)type;
}

struct type *new_struct_type(size_t n_fields, struct field fields[],
        char *cname)
{
    struct struct_type *type = malloc(sizeof *type +
            n_fields * sizeof *type->fields);

    type->type.tag = STRUCT_TYPE_FLAG;
    type->cname = cname;
    type->n_fields = n_fields;
    type->is_defined = false;
    type->id = zc_n_struct_types++;
    for (size_t i = 0; i < n_fields; i++)
        type->fields[i] = fields[i];

    return (struct type *)type;
}

struct type *new_extern_type(void)
{
    struct extern_type *type = malloc(sizeof *type);
    type->type.tag = EXTERN_TYPE_FLAG;
    type->id = zc_n_struct_types++;
    return (struct type *)type;
}

struct ptr_type *ptr_type(struct type *t)
{
    assert(is_ptr_type(t));
    return (struct ptr_type *)t;
}

struct array_type *array_type(struct type *t)
{
    assert(is_array_type(t));
    return (struct array_type *)t;
}

struct func_type *func_type(struct type *t)
{
    assert(is_func_type(t));
    return (struct func_type *)t;
}

struct struct_type *struct_type(struct type *t)
{
    return (struct struct_type *)t;
}

struct extern_type *extern_type(struct type *t)
{
    return (struct extern_type *)t;
}

struct selfref_type *selfref_type(struct type *t)
{
    assert(is_selfref_type(t));
    return (struct selfref_type *)t;
}

// Create a type from a PTR_EXPR node in the AST
static struct type *type_from_ptr_ast(struct ast *ast)
{
    assert(ast->tag == PTR_EXPR);
    return new_ptr_type(type_from_ast(ast_ast(ast, 0)));
}

// Create a type from an ARRAY_EXPR node in the AST
static struct type *type_from_array_ast(struct ast *ast)
{
    assert(ast->tag == ARRAY_EXPR);
    struct type *of = type_from_ast(ast_ast(ast, 0));
    size_t len = eval_size(ast_ast(ast, 1));

    return new_array_type(of, len);
}

// Create a type from a FUNC_EXPR node in the AST
static struct type *type_from_func_ast(struct ast *ast)
{
    assert(ast->tag == FUNC_EXPR);

    struct ast *param_list = ast_ast(ast, 0);

    size_t n_params = 0;
    struct ast **param_asts = ast_asts(param_list, &n_params);

    struct type *param_types[n_params];
    bool is_vararg = false;

    for (size_t i = 0; i < n_params; i++) {
        if (param_asts[i]->tag == DECL) {
            param_types[i] = type_from_ast(ast_ast(param_asts[i], 1));
        } else if (param_asts[i]->tag == VARARG_TYPE) {
            is_vararg = true;
            n_params--;
            break;
        } else {
            param_types[i] = type_from_ast(param_asts[i]);
        }

        if (is_void_type(param_types[i]))
            fatal(param_asts[i]->loc, "function parameter cannot be of void type");

        if (is_extern_type(param_types[i]))
            fatal(param_asts[i]->loc, "function parameter cannot be of incomplete type");

        if (is_array_type(param_types[i]))
            fatal(param_asts[i]->loc, "function parameter cannot be of array type");

        if (is_func_type(param_types[i]))
            fatal(param_asts[i]->loc, "function parameter cannot be of function type");
    }

    struct ast *ret_ast = ast_ast(ast, 1);
    struct type *ret_type = type_from_ast(ret_ast);

    if (is_extern_type(ret_type))
        fatal(ret_ast->loc, "function return value cannot be of incomplete type");

    if (is_array_type(ret_type))
        fatal(ret_ast->loc, "function return value cannot be of array type");

    if (is_func_type(ret_type))
        fatal(ret_ast->loc, "function return value cannot be of function type");

    return new_func_type(ret_type, n_params, param_types, is_vararg);
}

// Create a type from a NAME node in the AST
static struct type *type_from_name_ast(struct ast *ast)
{
    const char *name = ast_s(ast);
    struct type *type = scope_get_type(current_scope, name);

    if (type == NULL)
        fatal(ast->loc, "undefined type name '%s'", name);

    return type;
}

// Create structure field from a DECL node in the AST
static struct field field_from_decl(struct ast *ast)
{
    struct field field;
    field.name = strdup(ast_s(ast_ast(ast, 0)));
    field.type = type_from_ast(ast_ast(ast, 1));
    field.type->tag |= LVAL_TYPE_FLAG;
    return field;
}

// Create structure from a STRUCT_EXPR node in the AST
static struct type *type_from_struct_ast(struct ast *ast)
{
    size_t n_fields;
    struct ast **asts = ast_asts(ast, &n_fields);
    struct struct_type *type = malloc(sizeof *type +
            n_fields * sizeof (struct field));

    type->type.tag = STRUCT_TYPE_FLAG;
    type->cname = gen_c_ident();
    type->n_fields = n_fields;
    type->is_defined = false;
    type->id = zc_n_struct_types++;

    for (size_t i = 0; i < n_fields; ++i)
        type->fields[i] = field_from_decl(asts[i]);

    for (size_t i = 0; i < n_fields; ++i) {
        for (size_t j = i + 1; j < n_fields; ++j) {
            const char *a = type->fields[i].name, *b = type->fields[j].name;
            if (strcmp(a, b) == 0)
                fatal(ast->loc, "duplicate member %s in structure", a);
        }
    }

    return (struct type *)type;
}

// Create type from a node in the AST
struct type *type_from_ast(struct ast *ast)
{
    switch (ast->tag) {
        case PTR_EXPR:
            return type_from_ptr_ast(ast);
        case ARRAY_EXPR:
            return type_from_array_ast(ast);
        case FUNC_EXPR:
            return type_from_func_ast(ast);
        case NAME:
            return type_from_name_ast(ast);
        case STRUCT_EXPR:
            return type_from_struct_ast(ast);
    }
    abort();
}

// Compare types for equality.
bool type_equals(struct type *a, struct type *b)
{
    if (type_type(a->tag) != type_type(b->tag))
        return false;

    switch (type_type(a->tag)) {
        case PTR_TYPE_FLAG: {
            struct ptr_type *a2 = ptr_type(a);
            struct ptr_type *b2 = ptr_type(b);

            return type_equals(a2->to, b2->to);
        }
        case ARRAY_TYPE_FLAG: {
            struct array_type *a2 = array_type(a);
            struct array_type *b2 = array_type(b);

            if (a2->len != b2->len)
                return false;

            return type_equals(a2->of, b2->of);
        }
        case FUNC_TYPE_FLAG: {
            struct func_type *a2 = func_type(a);
            struct func_type *b2 = func_type(b);

            if (a2->n_params != b2->n_params)
                return false;

            for (size_t i = 0; i < a2->n_params; ++i) {
                if (!type_equals(a2->params[i], b2->params[i]))
                    return false;
            }

            return type_equals(a2->ret, b2->ret);
        }

        case VOID_TYPE_FLAG:
        case BOOL_TYPE_FLAG:
        case INT_TYPE_FLAG:
        case FLOAT_TYPE_FLAG:
            return (a->tag & ~LVAL_TYPE_FLAG) == (b->tag & ~LVAL_TYPE_FLAG);

        case STRUCT_TYPE_FLAG:
            return struct_type(a)->id == struct_type(b)->id;

        case EXTERN_TYPE_FLAG:
            return extern_type(a)->id == extern_type(b)->id;
    }

    unreachable();
    return false;
}

void type_del(struct type *type)
{
}

// Duplicate a type.
struct type *type_dup(struct type *t, enum type_tag flags)
{
    struct type *t2;
    switch (type_type(t->tag)) {
        case VOID_TYPE_FLAG:
        case BOOL_TYPE_FLAG:
        case INT_TYPE_FLAG:
        case FLOAT_TYPE_FLAG:
            t2 = malloc(sizeof *t2);
            memcpy(t2, t, sizeof *t2);
            break;
        case PTR_TYPE_FLAG:
            t2 = malloc(sizeof (struct ptr_type));
            memcpy(t2, t, sizeof (struct ptr_type));
            break;
        case ARRAY_TYPE_FLAG:
            t2 = malloc(sizeof (struct array_type));
            memcpy(t2, t, sizeof (struct array_type));
            break;
        case FUNC_TYPE_FLAG: {
            struct func_type *t_ = (struct func_type *)t;
            size_t n = sizeof *t_ + t_->n_params * sizeof *t_->params;
            struct func_type *t2_ = malloc(n);
            t2 = (struct type *)memcpy(t2_, t, n);
            break;
        }
        case STRUCT_TYPE_FLAG: {
            struct struct_type *t_ = (struct struct_type *)t;
            size_t n = sizeof *t_ + t_->n_fields * sizeof *t_->fields;
            struct struct_type *t2_ = malloc(n);
            t2 = (struct type *)memcpy(t2_, t, n);
            break;
        }
        default:
            unreachable();
    }
    t2->tag = (t2->tag & ~LVAL_TYPE_FLAG) | flags;
    return t2;
}

bool resolve_selfref(struct type **type, bool selfref_ok);
bool resolve_selfref_struct(struct struct_type *type, bool selfref_ok)
{
    size_t n_fields = type->n_fields;
    struct field *fields = type->fields;

    for (size_t i = 0; i < n_fields; i++) {
        struct field *field = &fields[i];

        if (is_ptr_type(field->type)) {
            if (!resolve_selfref(&ptr_type(field->type)->to, true))
                return false;
        } else {
            if (!resolve_selfref(&field->type, false))
                return false;
        }
    }
    return true;
}

// Resolve self referencing types in a type.  Can only resolve self referencing
// types if they are referenced by a pointer contained in a structure (as that
// is what C permits).
//
// A better solution would probably have been to have a seperate type for named
// types.  That would also avoid having to copy large structure types around,
// and simplify comparing types for equality.
bool resolve_selfref(struct type **type, bool selfref_ok)
{
    switch (type_type((*type)->tag)) {
        case SELFREF_TYPE_FLAG:
            if (selfref_ok) {
                *type = selfref_type(*type)->sym->type;
            }
            return selfref_ok;
        case PTR_TYPE_FLAG:
            return resolve_selfref(&ptr_type(*type)->to, selfref_ok);
        case ARRAY_TYPE_FLAG:
            return resolve_selfref(&array_type(*type)->of, selfref_ok);
        case FUNC_TYPE_FLAG:
            for (size_t i = 0; i < func_type(*type)->n_params; i++)
                if (!resolve_selfref(&func_type(*type)->params[i], selfref_ok))
                    return false;
            return resolve_selfref(&func_type(*type)->ret, selfref_ok);
        case STRUCT_TYPE_FLAG:
            return resolve_selfref_struct(struct_type(*type), selfref_ok);
    }
    return true;
}
