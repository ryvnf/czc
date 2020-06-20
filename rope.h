#ifndef ROPE_H
#define ROPE_H

// Data structure used for code generation to C.
struct rope {
    bool leaf;
    union {
        char *s;
        struct rope *childs[2];
    } val;
};

extern struct rope indent_rope[1];
extern struct rope lparen_rope[1];
extern struct rope rparen_rope[1];
extern struct rope rparen_semi_nl_rope[1];
extern struct rope rparen_sp_rope[1];
extern struct rope lsquare_rope[1];
extern struct rope rsquare_rope[1];
extern struct rope lcurly_nl_rope[1];
extern struct rope lcurly_sp_rope[1];
extern struct rope lcurly_rope[1];
extern struct rope rcurly_rope[1];
extern struct rope rcurly_nl_rope[1];
extern struct rope rcurly_sp_rope[1];
extern struct rope sp_rcurly_rope[1];
extern struct rope rcurly_semi_nl_rope[1];
extern struct rope return_semi_rope[1];
extern struct rope break_semi_rope[1];
extern struct rope continue_semi_rope[1];
extern struct rope return_sp_rope[1];
extern struct rope semi_rope[1];
extern struct rope comma_sp_rope[1];
extern struct rope comma_nl_rope[1];
extern struct rope ellipsis_rope[1];
extern struct rope void_rope[1];
extern struct rope if_sp_lparen_rope[1];
extern struct rope while_sp_lparen_rope[1];
extern struct rope for_sp_lparen_rope[1];
extern struct rope sp_while_sp_lparen_rope[1];
extern struct rope sp_else_sp_rope[1];
extern struct rope do_sp_rope[1];
extern struct rope for_sp_rope[1];
extern struct rope star_rope[1];
extern struct rope sp_rope[1];
extern struct rope nl_rope[1];
extern struct rope semi_nl_rope[1];
extern struct rope extern_sp_rope[1];
extern struct rope struct_sp_rope[1];
extern struct rope dot_rope[1];
extern struct rope one_rope[1];
extern struct rope zero_rope[1];
extern struct rope sizeof_sp_rope[1];
extern struct rope question_mark_binop_rope[1];
extern struct rope colon_binop_rope[1];
extern struct rope asgn_binop_rope[1];
extern struct rope mulasgn_binop_rope[1];
extern struct rope divasgn_binop_rope[1];
extern struct rope remasgn_binop_rope[1];
extern struct rope addasgn_binop_rope[1];
extern struct rope subasgn_binop_rope[1];
extern struct rope shlasgn_binop_rope[1];
extern struct rope shrasgn_binop_rope[1];
extern struct rope andasgn_binop_rope[1];
extern struct rope xorasgn_binop_rope[1];
extern struct rope orasgn_binop_rope[1];
extern struct rope land_binop_rope[1];
extern struct rope lor_binop_rope[1];
extern struct rope eq_binop_rope[1];
extern struct rope ne_binop_rope[1];
extern struct rope lt_binop_rope[1];
extern struct rope gt_binop_rope[1];
extern struct rope le_binop_rope[1];
extern struct rope ge_binop_rope[1];
extern struct rope or_binop_rope[1];
extern struct rope xor_binop_rope[1];
extern struct rope and_binop_rope[1];
extern struct rope shl_binop_rope[1];
extern struct rope shr_binop_rope[1];
extern struct rope add_binop_rope[1];
extern struct rope sub_binop_rope[1];
extern struct rope mul_binop_rope[1];
extern struct rope div_binop_rope[1];
extern struct rope rem_binop_rope[1];
extern struct rope not_rope[1];
extern struct rope compl_rope[1];
extern struct rope plus_rope[1];
extern struct rope minus_rope[1];
extern struct rope amp_rope[1];
extern struct rope inc_rope[1];
extern struct rope dec_rope[1];
extern struct rope star_rope[1];
extern struct rope memcpy_lparen_rope[1];
extern struct rope switch_sp_lparen_rope[1];
extern struct rope case_sp_rope[1];
extern struct rope default_colon_nl_rope[1];
extern struct rope colon_nl_rope[1];

struct rope *rope_new_s(const char *s);
struct rope *rope_new_fmt(const char *fmt, ...);

struct rope *rope_new_tree(struct rope *left, struct rope *right);
void rope_print(struct rope *rope);
void rope_print_to_file(struct rope *rope, FILE *fp);

#endif // !defined ROPE_H
