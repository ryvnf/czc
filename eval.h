#ifndef EVAL_H
#define EVAL_H

struct expr {
    struct type *type;
    struct rope *rope;
};

struct type *common_type(struct type *a, struct type *b);
struct type *target_type(struct type *wants, struct type *b);

struct type *eval_type(struct ast *ast);
struct expr eval_expr(struct ast *ast);
size_t eval_size(struct ast *ast);

#endif // !define EVAL_H
