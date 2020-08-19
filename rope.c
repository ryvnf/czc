#include "zc.h"

struct rope indent_rope[1] = { { .leaf = true, .val.s =  "    " } };
struct rope lparen_rope[1] = { { .leaf = true, .val.s =  "(" } };
struct rope rparen_rope[1] = { { .leaf = true, .val.s =  ")" } };
struct rope rparen_semi_nl_rope[1] = { { .leaf = true, .val.s =  ");\n" } };
struct rope rparen_sp_rope[1] = { { .leaf = true, .val.s =  ") " } };
struct rope lsquare_rope[1] = { { .leaf = true, .val.s =  "[" } };
struct rope rsquare_rope[1] = { { .leaf = true, .val.s =  "]" } };
struct rope lcurly_nl_rope[1] = { { .leaf = true, .val.s =  "{\n" } };
struct rope lcurly_rope[1] = { { .leaf = true, .val.s =  "{" } };
struct rope lcurly_sp_rope[1] = { { .leaf = true, .val.s =  "{ " } };
struct rope rcurly_rope[1] = { { .leaf = true, .val.s =  "}" } };
struct rope rcurly_nl_rope[1] = { { .leaf = true, .val.s =  "}\n" } };
struct rope rcurly_sp_rope[1] = { { .leaf = true, .val.s =  "}" } };
struct rope sp_rcurly_rope[1] = { { .leaf = true, .val.s =  " }" } };
struct rope rcurly_semi_nl_rope[1] = { { .leaf = true, .val.s = "};\n" } };
struct rope return_semi_rope[1] = { { .leaf = true, .val.s =  "return;" } };
struct rope break_semi_rope[1] = { { .leaf = true, .val.s =  "break;" } };
struct rope continue_semi_rope[1] = { { .leaf = true, .val.s =  "continue;" } };
struct rope return_sp_rope[1] = { { .leaf = true, .val.s =  "return " } };
struct rope semi_rope[1] = { { .leaf = true, .val.s =  ";" } };
struct rope comma_sp_rope[1] = { { .leaf = true, .val.s =  ", " } };
struct rope comma_nl_rope[1] = { { .leaf = true, .val.s =  ",\n" } };
struct rope ellipsis_rope[1] = { { .leaf = true, .val.s =  "..." } };
struct rope void_rope[1] = { { .leaf = true, .val.s =  "void" } };
struct rope if_sp_lparen_rope[1] = { { .leaf = true, .val.s =  "if (" } };
struct rope while_sp_lparen_rope[1] = { { .leaf = true, .val.s =  "while (" } };
struct rope for_sp_lparen_rope[1] = { { .leaf = true, .val.s =  "for (" } };
struct rope sp_while_sp_lparen_rope[1] = { { .leaf = true, .val.s =  " while (" } };
struct rope sp_else_sp_rope[1] = { { .leaf = true, .val.s =  " else " } };
struct rope do_sp_rope[1] = { { .leaf = true, .val.s =  "do " } };
struct rope for_sp_rope[1] = { { .leaf = true, .val.s =  "for " } };
struct rope star_rope[1] = { { .leaf = true, .val.s =  "*" } };
struct rope sp_rope[1] = { { .leaf = true, .val.s =  " " } };
struct rope nl_rope[1] = { { .leaf = true, .val.s =  "\n" } };
struct rope semi_nl_rope[1] = { { .leaf = true, .val.s =  ";\n" } };
struct rope extern_sp_rope[1] = { { .leaf = true, .val.s =  "extern " } };
struct rope struct_sp_rope[1] = { { .leaf = true, .val.s =  "struct " } };
struct rope dot_rope[1] = { { .leaf = true, .val.s =  "." } };
struct rope one_rope[1] = { { .leaf = true, .val.s =  "1" } };
struct rope zero_rope[1] = { { .leaf = true, .val.s =  "0" } };
struct rope sizeof_sp_rope[1] = { { .leaf = true, .val.s =  "sizeof " } };
struct rope question_mark_binop_rope[1] = { { .leaf = true, .val.s =  " ? " } };
struct rope colon_binop_rope[1] = { { .leaf = true, .val.s =  " : " } };
struct rope asgn_binop_rope[1] = { { .leaf = true, .val.s =  " = " } };
struct rope mulasgn_binop_rope[1] = { { .leaf = true, .val.s =  " *= " } };
struct rope divasgn_binop_rope[1] = { { .leaf = true, .val.s =  " /= " } };
struct rope remasgn_binop_rope[1] = { { .leaf = true, .val.s =  " %= " } };
struct rope addasgn_binop_rope[1] = { { .leaf = true, .val.s =  " += " } };
struct rope subasgn_binop_rope[1] = { { .leaf = true, .val.s =  " -= " } };
struct rope shlasgn_binop_rope[1] = { { .leaf = true, .val.s =  " <<= " } };
struct rope shrasgn_binop_rope[1] = { { .leaf = true, .val.s =  " >>= " } };
struct rope andasgn_binop_rope[1] = { { .leaf = true, .val.s =  " &= " } };
struct rope xorasgn_binop_rope[1] = { { .leaf = true, .val.s =  " ^= " } };
struct rope orasgn_binop_rope[1] = { { .leaf = true, .val.s =  " |= " } };
struct rope land_binop_rope[1] = { { .leaf = true, .val.s =  " && " } };
struct rope lor_binop_rope[1] = { { .leaf = true, .val.s =  " || " } };
struct rope eq_binop_rope[1] = { { .leaf = true, .val.s =  " == " } };
struct rope ne_binop_rope[1] = { { .leaf = true, .val.s =  " != " } };
struct rope lt_binop_rope[1] = { { .leaf = true, .val.s =  " < " } };
struct rope gt_binop_rope[1] = { { .leaf = true, .val.s =  " > " } };
struct rope le_binop_rope[1] = { { .leaf = true, .val.s =  " <= " } };
struct rope ge_binop_rope[1] = { { .leaf = true, .val.s =  " >= " } };
struct rope or_binop_rope[1] = { { .leaf = true, .val.s =  " | " } };
struct rope xor_binop_rope[1] = { { .leaf = true, .val.s =  " ^ " } };
struct rope and_binop_rope[1] = { { .leaf = true, .val.s =  " & " } };
struct rope shl_binop_rope[1] = { { .leaf = true, .val.s =  " << " } };
struct rope shr_binop_rope[1] = { { .leaf = true, .val.s =  " >> " } };
struct rope add_binop_rope[1] = { { .leaf = true, .val.s =  " + " } };
struct rope sub_binop_rope[1] = { { .leaf = true, .val.s =  " - " } };
struct rope mul_binop_rope[1] = { { .leaf = true, .val.s =  " * " } };
struct rope div_binop_rope[1] = { { .leaf = true, .val.s =  " / " } };
struct rope rem_binop_rope[1] = { { .leaf = true, .val.s =  " % " } };
struct rope not_rope[1] = { { .leaf = true, .val.s =  "!" } };
struct rope compl_rope[1] = { { .leaf = true, .val.s =  "~" } };
struct rope plus_rope[1] = { { .leaf = true, .val.s =  "+" } };
struct rope minus_rope[1] = { { .leaf = true, .val.s =  "-" } };
struct rope amp_rope[1] = { { .leaf = true, .val.s =  "&" } };
struct rope inc_rope[1] = { { .leaf = true, .val.s =  "++" } };
struct rope dec_rope[1] = { { .leaf = true, .val.s =  "--" } };
struct rope memcpy_lparen_rope[1] = { { .leaf = true, .val.s =  "memcpy(" } };
struct rope switch_sp_lparen_rope[1] = { { .leaf = true, .val.s = "switch (" } };
struct rope case_sp_rope[1] = { { .leaf = true, .val.s = "case " } };
struct rope default_colon_nl_rope[1] = { { .leaf = true, .val.s = "default:\n" } };
struct rope colon_nl_rope[1] = { { .leaf = true, .val.s = ":\n" } };
struct rope goto_sp_rope[1] = { { .leaf = true, .val.s = "goto " } };

static struct {
    char data[1 << 20];
    size_t n;
} rope_buf;

struct rope *rope_indent(void)
{
    struct rope *rope = malloc(sizeof *rope);
    rope->leaf = true;
    rope->val.s = rope_buf.data + rope_buf.n;
    
    size_t i = 0;
    while (i < zc_indent_level) {
        for (size_t j = 0; j < 4; ++j) {
            rope->val.s[i++] = ' ';
            rope_buf.n += 4;
        }
    }
    rope->val.s[i] = '\0';
    rope_buf.n++;

    return rope;
}

struct rope *rope_line(struct rope *rope)
{
    rope = rope_new_tree(rope_indent(), rope);
    return rope_new_tree(rope, rope_new_s("\n"));
}

struct rope *rope_new_fmt(const char *fmt, ...)
{
    struct rope *rope = malloc(sizeof *rope);
    rope->leaf = true;
    rope->val.s = rope_buf.data + rope_buf.n;

    va_list va;
    va_start(va, fmt);

    char *data = rope_buf.data + rope_buf.n;
    size_t n_left = sizeof rope_buf.data - rope_buf.n;

    size_t n_written = vsnprintf(data, n_left, fmt, va) + 1;
    if (n_written > n_left)
        fatal(NULL, "not enough space in rope buffer");

    rope_buf.n += n_written;

    va_end(va);

    return rope;
}

struct rope *rope_new_s(const char *s)
{
    return rope_new_fmt("%s", s);
}

struct rope *rope_new_tree(struct rope *left, struct rope *right)
{
    struct rope *rope = malloc(sizeof *rope);
    rope->leaf = false;
    rope->val.childs[0] = left;
    rope->val.childs[1] = right;

    return rope;
}

void rope_print_to_file(struct rope *rope, FILE *fp)
{
    if (!rope)
        return;

    if (rope->leaf) {
        fputs(rope->val.s, fp);
        return;
    }

    rope_print_to_file(rope->val.childs[0], fp);
    rope_print_to_file(rope->val.childs[1], fp);
}

void rope_print(struct rope *rope)
{
    rope_print_to_file(rope, stdout);
}
