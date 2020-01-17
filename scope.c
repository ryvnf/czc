#include "zc.h"

struct scope *scope_new(struct scope *parent, size_t n_syms, size_t n_types)
{
    struct scope *scope = malloc(sizeof *scope);
    scope->symtbl = strmap_new(n_syms);
    scope->typetbl = strmap_new(n_types);
    scope->parent = parent;
    return scope;
}

void scope_add_sym(struct scope *scope, const char *name, struct sym *sym)
{
    strmap_add(scope->symtbl, name, sym);
}

void scope_add_typesym(struct scope *scope, const char *name, struct sym *sym)
{
    strmap_add(scope->typetbl, name, sym);
}

struct sym *scope_get_sym(struct scope *scope, const char *name)
{
    struct sym *sym = strmap_get(scope->symtbl, name);
    if (sym == NULL) {
        if (scope->parent == NULL)
            return NULL;
        return scope_get_sym(scope->parent, name);
    }
    return sym_res(sym);
}

struct type *sym_type(struct sym * sym)
{
    if (sym == NULL)
        return NULL;

    assert(sym->tag == TYPE_SYM);
    return ((struct type_sym *)sym)->type;
}

struct type *scope_get_type(struct scope *scope, const char *name)
{
    struct sym *sym = strmap_get(scope->typetbl, name);
    if (sym == NULL) {
        if (scope->parent == NULL)
            return NULL;
        return scope_get_type(scope->parent, name);
    }
    return sym_type(sym_res(sym));
}
