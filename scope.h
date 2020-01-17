#ifndef SCOPE_H
#define SCOPE_H

// Data structures to keep track of which symbols are visible in a scope.
// Works like a stack, where new scopes are pushed and popped.
struct scope {
    struct strmap *symtbl;
    struct strmap *typetbl;
    struct scope *parent;
};

struct scope *scope_new(struct scope *parent, size_t n_syms, size_t n_types);

void scope_add_sym(struct scope *scope, const char *name, struct sym *sym);
void scope_add_typesym(struct scope *scope, const char *name, struct sym *sym);

struct sym *scope_get_sym(struct scope *scope, const char *name);
struct type *scope_get_type(struct scope *scope, const char *name);

#endif // !defined SCOPE_H
