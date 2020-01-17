#include "zc.h"

struct sym *sym_new(struct ast *ast)
{
    struct sym *sym = malloc(sizeof (struct decl_sym));

    sym->loc = ast;
    sym->tag = UNRES_SYM;

    return sym;
}

struct sym *sym_new_type(struct type *type)
{
    struct type_sym *sym = malloc(sizeof *sym);

    sym->sym.loc = NULL;
    sym->sym.tag = TYPE_SYM;
    sym->type = type;

    return (struct sym *)sym;
}

struct sym *sym_res(struct sym *sym)
{
    if (sym == NULL)
        return NULL;

    if (sym->tag != UNRES_SYM)
        return sym;

    switch (sym->loc->tag) {
        case DECL: {
            sym->tag = DECL_SYM;
            struct decl_sym *decl_sym = (struct decl_sym *)sym;
            decl_sym->type = NULL;
            decl_sym->type = type_from_ast(ast_ast(sym->loc, 1));
            decl_sym->c_name = strdup(ast_s(ast_ast(sym->loc, 0)));

            add_global_decl(decl_sym);
            return sym;
        }
        case TYPE_DEF: {
            sym->tag = TYPE_SYM;
            struct type_sym *type_sym = (struct type_sym *)sym;
            type_sym->type = new_selfref_type(type_sym);

            struct ast *ast = ast_ast(sym->loc, 1);

            if (ast == NULL) {
                type_sym->type = new_extern_type();
            } else {
                type_sym->type = type_from_ast(ast);
                if (!resolve_selfref(&type_sym->type, false))
                    fatal(sym->loc->line, "type cannot reference itself in "
                            "this context");
                add_type_decl(type_sym->type);
            }

            return sym;
        }
        case ALIAS_DEF: {
            sym->tag = ALIAS_SYM;
            struct alias_sym *alias_sym = (struct alias_sym *)sym;
            alias_sym->ast = ast_ast(sym->loc, 1);
            return sym;
        }
        default:
            unreachable();
    }
}

struct sym *sym_from_ast(struct ast *ast)
{
    if (ast->tag == DECL) {
        struct decl_sym *decl_sym = (struct decl_sym *)sym_new(ast);
        decl_sym->sym.loc = ast;
        decl_sym->sym.tag = DECL_SYM;
        decl_sym->type = type_from_ast(ast_ast(decl_sym->sym.loc, 1));
	decl_sym->type->tag |= LVAL_TYPE_FLAG;
        decl_sym->c_name = gen_c_ident();

        return (struct sym *)decl_sym;
    }
    abort();
}
