#ifndef CODEGEN_H
#define CODEGEN_H

struct rope *type_to_c(struct rope *decl, struct type *type);
struct rope *decl_to_c(struct decl_sym *decl);
struct rope *program_to_c(struct symtbl *symtbl, struct ast *ast);
struct rope *expr_stmt_to_c(struct ast *ast);
struct rope *if_stmt_to_c(struct ast *ast);
struct rope *while_stmt_to_c(struct ast *ast);
struct rope *do_while_stmt_to_c(struct ast *ast);
struct rope *for_stmt_to_c(struct ast *ast);
struct rope *return_stmt_to_c(struct ast *ast);
struct rope *cmpnd_stmt_to_c(struct ast *ast);
struct rope *stmt_to_c(struct ast *ast);
struct rope *block_to_c(struct ast *ast);
struct rope *func_def_to_c(struct ast *ast);
struct rope *decl_to_c(struct decl_sym *decl);
void add_global_decl(struct decl_sym *decl);
void add_local_decl(struct decl_sym *decl);
void add_type_decl(struct type *type);
char *gen_c_ident(void);

void codegen_to_file(struct ast *ast, FILE *fp); 

#endif // !defined CODEGEN_H
