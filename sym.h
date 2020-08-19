#ifndef SYM_H
#define SYM_H

// Structure to represent a symbol in the symbol table.
struct sym {
    enum sym_tag {
        UNRES_SYM,
        DECL_SYM,
        ALIAS_SYM,
        TYPE_SYM
    } tag;
    struct ast *loc;
};

// Alias symbol.
struct alias_sym {
    struct sym sym;
    struct ast *ast;
};

// Declared symbol.
struct decl_sym {
    struct sym sym;
    char *c_name;
    struct type *type;
    bool is_defined;
};

// Type symbol.
struct type_sym {
    struct sym sym;
    struct type *type;
};

// Unresolved symbol (can be an alias, declared, or type symbol).  All global
// symbols are put into the symbol table in a first pass as unresolved symbols,
// then they are resolved when they are needed.  This is to avoid the need to
// forward declarations, and cases when the definition of one symbol is
// dependent of another.
struct unres_sym {
    struct sym sym;
    union {
        struct decl_sym decl_sym;
        struct alias_sym alias_sym;
        struct type_sym type_sym;
    } val;
};

struct sym *sym_new(struct ast *ast);
struct sym *sym_new_type(struct type *type);
struct sym *typesym_from_ast(struct ast *ast);
struct sym *sym_from_ast(struct ast *ast);
struct sym *sym_res(struct sym *sym);

#endif // !defined SYM_H
