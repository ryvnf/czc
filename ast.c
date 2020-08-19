#include "zc.h"

// Create a node with string value.
struct ast *ast_new(struct loc *loc, enum ast_tag tag)
{
    struct ast *ast = malloc(sizeof *ast);
    ast->loc = loc;
    ast->tag = tag;
    return ast;
}

// Create a node with string value.
struct ast *ast_new_s(struct loc *loc, enum ast_tag tag, char *s)
{
    struct ast_s *ast = malloc(sizeof *ast);
    ast->ast.tag = tag;
    ast->ast.loc = loc;
    ast->s = strdup(s);
    return (struct ast *)ast;
}

// Create a node with integer value.
struct ast *ast_new_i(struct loc *loc, enum ast_tag tag, long long int i)
{
    struct ast_i *ast = malloc(sizeof *ast);
    ast->ast.tag = tag;
    ast->ast.loc = loc;
    ast->i = i;
    return (struct ast *)ast;
}

// Create a node with float value.
struct ast *ast_new_f(struct loc *loc, enum ast_tag tag, double f)
{
    struct ast_f *ast = malloc(sizeof *ast);
    ast->ast.tag = tag;
    ast->ast.loc = loc;
    ast->f = f;
    return (struct ast *)ast;
}

// Create a node with children.
struct ast *ast_new_ast(struct loc *loc, enum ast_tag tag, size_t n_childs, ...)
{
    struct ast_ast *ast = malloc(sizeof *ast + n_childs * sizeof *ast->childs);
    ast->ast.tag = tag;
    ast->ast.loc = loc;
    ast->n_childs = n_childs;
    
    va_list va;
    va_start(va, n_childs);

    for (size_t i = 0; i < n_childs; i++)
        ast->childs[i] = va_arg(va, struct ast *);

    va_end(va);
    return (struct ast *)ast;
}

// Create a node with the children from the nodes in the ast_list.
struct ast *ast_new_list(struct loc *loc, enum ast_tag tag, struct ast_list *ast_list)
{
    size_t n_childs = 0;
    for (struct ast_list *i = ast_list; i != NULL; i = i->next)
        n_childs++;

    struct ast_ast *ast = malloc(sizeof *ast + n_childs * sizeof *ast->childs);
    ast->ast.tag = tag;
    ast->ast.loc = loc;
    ast->n_childs = n_childs;

    size_t i = 0;
    for (struct ast_list *j = ast_list; j != NULL; j = j->next)
        ast->childs[i++] = ast_ref(j->ast);

    ast_list_del(ast_list);
    return (struct ast *)ast;
}

// Create new node in linked list of nodes.
struct ast_list *ast_list_new(struct ast *ast)
{
    struct ast_list *ast_list = malloc(sizeof *ast_list);
    ast_list->ast = ast;
    ast_list->next = NULL;

    return ast_list;
}

// Recursively delete list of nodes.
void ast_list_del(struct ast_list *ast_list)
{
    if (ast_list == NULL)
        return;

    ast_unref(ast_list->ast);
    ast_list_del(ast_list->next);
    free(ast_list);
}

// Get string value of a node.
const char *ast_s(struct ast *ast)
{
    assert(ast_has_s(ast->tag));
    return ((struct ast_s *)ast)->s;
}

// Get integer value of a node.
long long ast_i(struct ast *ast)
{
    assert(ast_has_i(ast->tag));
    return ((struct ast_i *)ast)->i;
}

// Get float value of a node.
double ast_f(struct ast *ast)
{
    assert(ast_has_f(ast->tag));
    return ((struct ast_f *)ast)->f;
}

// Get the child node with index from a node
struct ast *ast_ast(struct ast *ast, size_t index)
{
    assert(ast_has_ast(ast->tag));
    return ((struct ast_ast *)ast)->childs[index];
}

// Get all child nodes of a node.  Pointer to the child nodes is returned; the
// number of child nodes gets written to the pointer argument.
struct ast **ast_asts(struct ast *ast, size_t *n_childs)
{
    assert(ast_has_ast(ast->tag));
    assert(n_childs);

    *n_childs = ((struct ast_ast *)ast)->n_childs;
    return ((struct ast_ast *)ast)->childs;
}

// Increase the reference counter of a node.
struct ast *ast_ref(struct ast *ast)
{
    return ast; // TODO: Implement
}

// Decrease the reference counter of a node.
void ast_unref(struct ast *ast)
{
    // TODO: Implement
}

// Get the tag name of a node.
const char *ast_tag_name(enum ast_tag tag)
{
    switch (tag) {
#define tag_case(name, val) \
            case name: \
                       return #name;
        EXPAND_AST_TAGS(tag_case)
#undef tag_case
    }
    return NULL;
}

static void ast_print_childs(struct ast *ast, int indent)
{
    size_t n_childs;
    struct ast **childs = ast_asts(ast, &n_childs);

    for (size_t i = 0; i < n_childs; i++) {
        ast_print(childs[i], indent + 1);
    }
}

// Print the AST.
void ast_print(struct ast *ast, int indent)
{
    for (int j = 0; j < indent; j++)
        fputs("  ", stdout);

    if (ast == NULL) {
        puts("NULL");
        return;
    }

    switch (ast_val_type(ast->tag)) {
        case AST_NO_VAL:
            printf("%s\n", ast_tag_name(ast->tag));
            break;
        case AST_S:
            printf("%s \"%s\"\n", ast_tag_name(ast->tag), ast_s(ast));
            break;
        case AST_I:
            printf("%s %lld\n", ast_tag_name(ast->tag), ast_i(ast));
            break;
        case AST_F:
            printf("%s %f\n", ast_tag_name(ast->tag), ast_f(ast));
            break;
        case AST_AST:
            printf("%s\n", ast_tag_name(ast->tag));
            ast_print_childs(ast, indent);
            break;
    }
}

bool ast_equals(struct ast *a, struct ast *b)
{
    if (a->tag != b->tag)
        return false;

    switch (a->tag) {
        case AST_NO_VAL:
            return true;
        case AST_S:
            return strcmp(ast_s(a), ast_s(b));
        case AST_I:
            return ast_i(a) == ast_i(b);
        case AST_F:
            return ast_f(a) == ast_f(b);
        case AST_AST: {
            size_t n_a_asts, n_b_asts;
            struct ast **a_asts = ast_asts(a, &n_a_asts);
            struct ast **b_asts = ast_asts(b, &n_b_asts);

            if (n_a_asts != n_b_asts)
                return false;

            for (size_t i = 0; i < n_a_asts; i++) {
                if (!ast_equals(a_asts[i], b_asts[i]))
                    return false;
            }

            return true;
        }
    }

    bug("unknown AST-type");
}
