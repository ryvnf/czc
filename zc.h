#ifndef ZC_H
#define ZC_H

#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdnoreturn.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#include "ast.h"
#include "type.h"
#include "lex.h"
#include "parse.h"
#include "rope.h"
#include "strmap.h"
#include "error.h"
#include "eval.h"
#include "sym.h"
#include "codegen.h"
#include "scope.h"

// The amount of nested loops (used to know when continue or break is allowed)
extern int zc_loop_level;

// The amount of nested switch statements (used to know when fallthrough
// is allowed)
extern int zc_switch_level;

// The current indentation level when generating C code.
extern int zc_indent_level;

// This variable is incremented for each structure created.  It is used to give
// each structure an id when is used for comparing them for equality.
extern int zc_n_struct_types;

// The current local scope.
extern struct scope *current_scope;

// The scope with all global symbols.
extern struct scope *global_scope;

// Return type of current function being generated
extern struct type *zc_func_ret_type;

// The global variable/function declarations
extern struct rope *zc_prog_decls_rope;

// The global variable/function definitions
extern struct rope *zc_prog_defs_rope;

// The local variable declarations in the current function
extern struct rope *zc_func_decls_rope;

// Labels (used or defined) in the current function
extern struct strmap *zc_func_labels;

#endif // !defined ZC_H
