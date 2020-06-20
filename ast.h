#ifndef AST_H
#define AST_H

#define EXPAND_AST_TAGS(X) \
    X(NULL_CONST, 0x0000) \
    X(FALSE_CONST, 0x0001) \
    X(TRUE_CONST, 0x0002) \
    X(BREAK_STMT, 0x0003) \
    X(CONTINUE_STMT, 0x0004) \
    X(FALLTHROUGH_STMT, 0x0005) \
    X(VARARG_TYPE, 0x0006) \
    X(NAME, 0x1000) \
    X(STR_LIT, 0x1001) \
    X(CHAR_LIT, 0x1002) \
    X(INT_CONST, 0x2000) \
    X(FLOAT_CONST, 0x3000) \
    X(DECL, 0x4000) \
    X(FUNC_DEF, 0x4001) \
    X(TYPE_DEF, 0x4002) \
    X(ALIAS_DEF, 0x4003) \
    X(BLOCK, 0x4100) \
    X(IF_STMT, 0x4101) \
    X(WHILE_STMT, 0x4102) \
    X(DO_WHILE_STMT, 0x4103) \
    X(FOR_STMT, 0x4104) \
    X(RETURN_STMT, 0x4105) \
    X(GOTO_STMT, 0x4106) \
    X(CMPND_STMT, 0x4107) \
    X(SWITCH_STMT, 0x4108) \
    X(CASE_CLAUSE, 0x4109) \
    X(DEFAULT_CLAUSE, 0x410a) \
    X(LABEL_STMT, 0x410b) \
    X(COMMA_EXPR, 0x4200) \
    X(COND_EXPR, 0x4210) \
    X(ASGN_EXPR, 0x4220) \
    X(ADD_ASGN_EXPR, 0x4221) \
    X(SUB_ASGN_EXPR, 0x4222) \
    X(MUL_ASGN_EXPR, 0x4223) \
    X(DIV_ASGN_EXPR, 0x4224) \
    X(REM_ASGN_EXPR, 0x4225) \
    X(OR_ASGN_EXPR, 0x4226) \
    X(XOR_ASGN_EXPR, 0x4227) \
    X(AND_ASGN_EXPR, 0x4228) \
    X(SHL_ASGN_EXPR, 0x4229) \
    X(SHR_ASGN_EXPR, 0x422a) \
    X(LOR_EXPR, 0x4230) \
    X(LAND_EXPR, 0x4240) \
    X(EQ_EXPR, 0x4250) \
    X(NE_EXPR, 0x4251) \
    X(LT_EXPR, 0x4260) \
    X(GT_EXPR, 0x4261) \
    X(LE_EXPR, 0x4262) \
    X(GE_EXPR, 0x4263) \
    X(OR_EXPR, 0x4270) \
    X(XOR_EXPR, 0x4280) \
    X(AND_EXPR, 0x4290) \
    X(SHL_EXPR, 0x42A0) \
    X(SHR_EXPR, 0x42A1) \
    X(ADD_EXPR, 0x42B0) \
    X(SUB_EXPR, 0x42B1) \
    X(MUL_EXPR, 0x42C0) \
    X(DIV_EXPR, 0x42C1) \
    X(REM_EXPR, 0x42C2) \
    X(NOT_EXPR, 0x42D0) \
    X(COMPL_EXPR, 0x42D1) \
    X(UPLUS_EXPR, 0x42D2) \
    X(NEG_EXPR, 0x42D3) \
    X(REF_EXPR, 0x42D4) \
    X(PRE_INC_EXPR, 0x42D5) \
    X(PRE_DEC_EXPR, 0x42D6) \
    X(SIZEOF_EXPR, 0x42D7) \
    X(CAST_EXPR, 0x42E0) \
    X(DEREF_EXPR, 0x42F0) \
    X(SUBSCR_EXPR, 0x42F1) \
    X(CALL_EXPR, 0x42F2) \
    X(POST_INC_EXPR, 0x42F3) \
    X(POST_DEC_EXPR, 0x42F4) \
    X(MEMBER_EXPR, 0x42F5) \
    X(INIT_EXPR, 0x42F6) \
    X(PTR_EXPR, 0x4300) \
    X(ARRAY_EXPR, 0x4301) \
    X(FUNC_EXPR, 0x4302) \
    X(STRUCT_EXPR, 0x4303) \
    X(PARAM_LIST, 0x4400) \
    X(STMT_LIST, 0x4401) \
    X(EXPR_LIST, 0x4402) \
    X(CASE_LIST, 0x4403) \
    X(PROGRAM, 0x4502) \
    X(SYMREF, 0x5000) \
    X(TYPEREF, 0x6000) \
    X(INVALID_AST_TAG, 0xFFFF)

enum ast_tag {
    /* tags when define which type the value of the ast node has */
    AST_NO_VAL = 0x0000,
    AST_S = 0x1000,
    AST_I = 0x2000,
    AST_F = 0x3000,
    AST_AST = 0x4000,

#define enum_def(NAME, VAL) NAME = VAL,
    EXPAND_AST_TAGS(enum_def)
#undef enum_def
};

// Node without an associated value.
struct ast {
    enum ast_tag tag;
    int line;       // Line number in source code.
    size_t n_refs;  // Reference counter.
};

// Node with a string value.
struct ast_s {
    struct ast ast;
    char *s;
};

// Node with an integer value.
struct ast_i {
    struct ast ast;
    long long int i;
};

// Node with a float value.
struct ast_f {
    struct ast ast;
    double f;
};

// Node with has a number of child nodes.
struct ast_ast {
    struct ast ast;
    size_t n_childs;
    struct ast *childs[];
};

// Structure to represent a list of nodes.  This is not used in the actual
// AST but is used when building the AST for nodes which can have an arbitrary
// number of children.
struct ast_list {
    struct ast *ast;
    struct ast_list *next;
};

// Get the value type of an AST.
static inline enum ast_tag ast_val_type(enum ast_tag tag)
{
    return tag & 0xF000;
}

static inline bool ast_has_s(enum ast_tag tag)
{
    return ast_val_type(tag) == AST_S;
}

static inline bool ast_has_i(enum ast_tag tag)
{
    return ast_val_type(tag) == AST_I;
}

static inline bool ast_has_f(enum ast_tag tag)
{
    return ast_val_type(tag) == AST_F;
}

static inline bool ast_has_ast(enum ast_tag tag)
{
    return ast_val_type(tag) == AST_AST;
}

// Create new nodes, with associated value.
struct ast *ast_new(int line, enum ast_tag tag);
struct ast *ast_new_s(int line, enum ast_tag tag, char *s);
struct ast *ast_new_i(int line, enum ast_tag tag, long long int i);
struct ast *ast_new_f(int line, enum ast_tag tag, double f);
struct ast *ast_new_ast(int line, enum ast_tag tag, size_t n_childs, ...);

// Create a node with the children from the nodes in the ast_list.
struct ast *ast_new_list(int line, enum ast_tag tag, struct ast_list *ast_list);

// Create new node in linked list of nodes.
struct ast_list *ast_list_new(struct ast *ast);

// Recursively delete linked list of nodes.
void ast_list_del(struct ast_list *ast_list);

// Get the value of a node.
const char *ast_s(struct ast *ast);
long long ast_i(struct ast *ast);
double ast_f(struct ast *ast);

// Get the child node with index of a node.
struct ast *ast_ast(struct ast *ast, size_t index);

// Get all child nodes of a node.  Pointer to the child nodes is returned; the
// number of child nodes gets written to the pointer argument.
struct ast **ast_asts(struct ast *ast, size_t *n_childs);

// Increase or decrease the reference counter of a node.
struct ast *ast_ref(struct ast *ast);
void ast_unref(struct ast *ast);

// Get the tag name of a node.
const char *ast_tag_name(enum ast_tag tag);

// Print the AST.
void ast_print(struct ast *ast, int indent);

#endif // !defined AST_H
