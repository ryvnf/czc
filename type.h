#ifndef TYPE_H
#define TYPE_H

struct symtbl;

// If *_TYPE_FLAG is set then the type is of type *.  For example, tag values
// where 'tag & INT_TYPE_FLAG' is true are of integer type.
enum type_tag {
    VOID_TYPE_FLAG = 0x0001,
    VOID_TYPE = 0x0001,

    INT_TYPE_FLAG = 0x0002,
    INT_TYPE = 0x0002,
    INT8_TYPE = 0x1002,
    INT16_TYPE = 0x2002,
    INT32_TYPE = 0x3002,
    INT64_TYPE = 0x4002,
    INTPTR_TYPE = 0x5002,
    SSIZE_TYPE = 0x6002,
    UINT_TYPE = 0x7002,
    UINT8_TYPE = 0x8002,
    UINT16_TYPE = 0x9002,
    UINT32_TYPE = 0xA002,
    UINT64_TYPE = 0xB002,
    UINTPTR_TYPE = 0xC002,
    SIZE_TYPE = 0xD002,
    CHAR_TYPE = 0xE002,
    INT_CONST_TYPE = 0xF002,

    FLOAT_TYPE_FLAG = 0x0004,
    FLOAT_TYPE = 0x1004,
    DOUBLE_TYPE = 0x2004,
    FLOAT_CONST_TYPE = 0x3004,

    PTR_TYPE_FLAG = 0x0008,
    PTR_TYPE = 0x1008,

    ARRAY_TYPE_FLAG = 0x0010,
    FUNC_TYPE_FLAG = 0x0020,
    STRUCT_TYPE_FLAG = 0x0040,
    BOOL_TYPE_FLAG = 0x0080,
    BOOL_TYPE = 0x0080,
    SELFREF_TYPE_FLAG = 0x0100,

    EXTERN_TYPE_FLAG = 0x0200,
    EXTERN_TYPE = 0x0200,

    // Flag which defines the type kind
    TYPE_MASK = 0x03FF,

    // Flag set when the type is an lvalue
    LVAL_TYPE_FLAG = 0x0400
};

struct type {
    enum type_tag tag;
};

// Type generated when a type definition references itself, like a structure
// containing a pointer to itself.  This kind of type is used as a place-holder
// when resolving the type, and is replaced by the type from the 'sym' member
// after it has been resolved.
struct selfref_type {
    struct type type;
    struct type_sym *sym;
};

struct ptr_type {
    struct type type;
    struct type *to;
};

struct array_type {
    struct type type;
    struct type *of;
    size_t len;
};

struct func_type {
    struct type type;
    struct type *ret;
    size_t n_params;
    bool has_vararg;
    struct type *params[];
};

struct field {
    const char *name;
    struct type *type;
};

struct extern_type {
    struct type type;
    int id;
};

struct struct_type {
    struct type type;
    const char *cname;
    size_t n_fields;
    bool is_defined;
    int id;
    struct field fields[];
};

size_t sizeof_basic_type(struct type *type);
const char *basic_type_c_name(struct type *type);

extern struct type int_type[1];
extern struct type int8_type[1];
extern struct type int16_type[1];
extern struct type int32_type[1];
extern struct type int64_type[1];
extern struct type ssize_type[1];
extern struct type uint_type[1];
extern struct type uint8_type[1];
extern struct type uint16_type[1];
extern struct type uint32_type[1];
extern struct type uint64_type[1];
extern struct type size_type[1];
extern struct type float_type[1];
extern struct type double_type[1];
extern struct type const_int_type[1];
extern struct type const_float_type[1];
extern struct type *void_ptr_type;
extern struct type bool_type[1];
extern struct type char_type[1];
extern struct type void_type[1];

struct type *new_selfref_type(struct type_sym *sym);

struct type *new_ptr_type(struct type *to);

struct type *new_array_type(struct type *of, size_t len);

struct type *new_func_type(struct type *ret, size_t n_params,
        struct type *params[], bool has_vararg);

struct type *new_struct_type(size_t n_fields, struct field fields[],
        char *ctype);

struct type *new_extern_type(void);

static inline enum type_tag type_type(enum type_tag tag)
{
    return tag & TYPE_MASK;
}

static inline void type_set_flags(struct type *t, enum type_tag flags) {
    t->tag |= (flags & LVAL_TYPE_FLAG);
}

static inline void type_clr_flags(struct type *t, enum type_tag flags) {
    t->tag &= ~(flags & LVAL_TYPE_FLAG);
}

bool type_equals(struct type *a, struct type *b);

static inline bool is_void_type(struct type *t)
{
    return t->tag & VOID_TYPE_FLAG;
}

static inline bool is_bool_type(struct type *t)
{
    return t->tag & BOOL_TYPE_FLAG;
}

static inline bool is_int_type(struct type *t)
{
    return t->tag & INT_TYPE_FLAG;
}

static inline bool is_float_type(struct type *t)
{
    return t->tag & FLOAT_TYPE_FLAG;
}

static inline bool is_ptr_type(struct type *t)
{
    return t->tag & PTR_TYPE_FLAG;
}

static inline bool is_selfref_type(struct type *t)
{
    return t->tag & SELFREF_TYPE_FLAG;
}

static inline bool is_array_type(struct type *t)
{
    return t->tag & ARRAY_TYPE_FLAG;
}

static inline bool is_func_type(struct type *t)
{
    return t->tag & FUNC_TYPE_FLAG;
}

static inline bool is_struct_type(struct type *t)
{
    return t->tag & STRUCT_TYPE_FLAG;
}

static inline bool is_extern_type(struct type *t)
{
    return t->tag & EXTERN_TYPE_FLAG;
}

static inline bool is_lval_type(struct type *t)
{
    return t->tag & LVAL_TYPE_FLAG;
}

struct selfref_type *selfref_type(struct type *t);

struct ptr_type *ptr_type(struct type *t);
struct array_type *array_type(struct type *t);
struct func_type *func_type(struct type *t);
struct struct_type *struct_type(struct type *t);
struct extern_type *extern_type(struct type *t);

void type_del(struct type *t);
struct type *type_dup(struct type *t, enum type_tag flags);

struct type *type_from_ast(struct ast *ast);
bool resolve_selfref(struct type **type, bool selfref_ok);

#endif // !defined TYPE_H
