#include "zc.h"

// The amount of nested loops (used to know when continue or break is allowed)
int zc_loop_level;

// The current indentation level when generating C code.
int zc_indent_level;

// This variable is incremented for each structure created.  It is used to give
// each structure an id when is used for comparing them for equality.
int zc_n_struct_types;

// The current local scope.
struct scope *current_scope;

// The scope with all global symbols.
struct scope *global_scope;

// Return type of current function being generated
struct type *zc_func_ret_type;

// The global variable/function declarations
struct rope *zc_prog_decls_rope;

// The global variable/function definitions
struct rope *zc_prog_defs_rope;

// The local variable declarations in the current function
struct rope *zc_func_decls_rope;

void unrecognized_opt(const char *opt)
{
    fatal(-1, "unrecognized command line option '%s'\n", opt);
}

void missing_arg_for_opt(const char *opt)
{
    fatal(-1, "missing argument for option '%s'\n", opt);
}

char *get_file_ext(const char *filename)
{
    char *ext = strrchr(filename, '.');
    if (ext == NULL)
        return NULL;

    return ext + 1;
}

// List of command-line arguments.
struct arg_list {
    char *arg;
    struct arg_list *next;
};

void arg_list_del(struct arg_list *arg_list)
{
    if (arg_list == NULL)
        return;

    free(arg_list->arg);
    arg_list_del(arg_list->next);
    free(arg_list);
}

struct arg_list *arg_list_new(const char *arg)
{
    struct arg_list *arg_list = malloc(sizeof *arg_list);
    arg_list->arg = strdup(arg);
    arg_list->next = NULL;

    return arg_list;
}

// Generate C code for file name and output to file handle.
void gen_input_file(const char *file_name, FILE *output_fp)
{
    FILE *input_fp = fopen(file_name, "r"); 
    
    struct ast *ast = parse(input_fp);

    codegen_to_file(ast, output_fp);

    fclose(input_fp);
}

// Generate C code for source files and output to file name.
void gen_c(struct arg_list *src_files, char *out)
{
    if (out) {
        FILE *fp;
        if ((fp = fopen(out, "w")) == NULL) {
            perror("fopen");
            exit(1);
        }

        gen_input_file(src_files->arg, fp);
        fclose(fp);
    }

    for (struct arg_list *i = src_files; i != NULL; i = i->next) {
        char *c_file_name = strdup(i->arg);
        char *c_file_ext = get_file_ext(c_file_name);
        c_file_ext[0] = 'c';
        c_file_ext[1] = '\0';

        FILE *fp;
        if ((fp = fopen(c_file_name, "w")) == NULL) {
            perror("fopen");
            exit(1);
        }
        
        printf("%s\n", i->arg);
        gen_input_file(i->arg, fp);
        fclose(fp);
        free(c_file_name);
    }
}

// Print the AST of the input files.
void print_ast_files(struct arg_list *src_files)
{
    for (struct arg_list *i = src_files; i != NULL; i = i->next) {
        printf("--- FILE %s ---\n", i->arg);
        FILE *fp = fopen(i->arg, "r");
        if (!fp) {
            perror(i->arg);
            exit(1);
        }
        struct ast *ast = parse(fp);
        ast_print(ast, 0);
        fclose(fp);
    }
}

// Invoke GCC on the source file, with GCC arguments.  gcc_args_tailp is a
// pointer to the last element of the gcc_args linked list, used to append more
// arguments.
void invoke_gcc(struct arg_list *src_files, struct arg_list *gcc_args,
        struct arg_list **gcc_args_tailp)
{
    // The beginning of the temporary file arguments passed to GCC (used to
    // remove them after compilation).
    struct arg_list **tmp_files = gcc_args_tailp;

    for (struct arg_list *i = src_files; i != NULL; i = i->next) {
        char c_file_name[] = "/tmp/czc.XXXXXX.c";
        int fd;
        
        if ((fd = mkstemps(c_file_name, 2)) < 0)
            perror("mkstemp");

        FILE *fp;
        if ((fp = fdopen(fd, "w")) == NULL)
            perror("fdopen");

        printf("%s\n", i->arg);
        gen_input_file(i->arg, fp);

        *gcc_args_tailp = arg_list_new(c_file_name);
        gcc_args_tailp = &(*gcc_args_tailp)->next;

        fclose(fp);
    }

    // Count number of arguments.
    size_t gcc_argc = 0;
    for (struct arg_list *i = gcc_args; i != NULL; i = i->next)
        gcc_argc++;

    char **gcc_argv = malloc((gcc_argc + 1) * sizeof *gcc_argv);

    // Initialize the arguments.
    struct arg_list *i = gcc_args;
    for (size_t j = 0; j < gcc_argc; ++j) {
        gcc_argv[j] = i->arg;
        i = i->next;
    }
    gcc_argv[gcc_argc] = NULL;

    // Execute GCC.
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(1);
    } else if (pid == 0) {
        if (execvp("gcc", gcc_argv) < 0) {
            perror("execvp");
            exit(1);
        }
    }

    waitpid(pid, NULL, 0);

    // Remove the created temporary files.
    for (struct arg_list *i = *tmp_files; i != NULL; i = i->next)
        unlink(i->arg);
}

void arg_list_print(const struct arg_list *args)
{
    if (args == NULL)
        return;

    printf("%s ", args->arg);
    arg_list_print(args->next);
}

int main(int argc, char **argv)
{
    char *out = NULL;

    struct arg_list *gcc_args = arg_list_new("gcc");
    struct arg_list **gcc_args_tailp = &gcc_args->next;
    struct arg_list *src_files = NULL;
    struct arg_list **src_files_tailp = &src_files;

    // Colors are disabled to make messages from 'gcc' consistent with other
    // messages which doesn't have color.
    *gcc_args_tailp = arg_list_new("-fno-diagnostics-color");
    gcc_args_tailp = &(*gcc_args_tailp)->next;

    // We don't want warnings from GCC to leak through.  Will still output
    // errors if the generated code does not compile.
    *gcc_args_tailp = arg_list_new("-w");
    gcc_args_tailp = &(*gcc_args_tailp)->next;

    // Tell GCC that the source has been preprocessed.  This is to avoid having
    // to escape trigraphs in the generated C code.
    *gcc_args_tailp = arg_list_new("-fpreprocessed");
    gcc_args_tailp = &(*gcc_args_tailp)->next;

    int input_file_count = 0;

    enum {
        TO_EXE,
        TO_C,
        TO_ASM,
        TO_OBJ,
        AST_PRINT
    } mode = TO_EXE;

    for (int i = 1; i < argc; ++i) {
        if (argv[i][0] == '-') {
            if (strcmp(argv[i], "--print-ast") == 0) {
                mode = AST_PRINT;
                continue;
            }
            if (strcmp(argv[i], "--to-c") == 0) {
                mode = TO_C;
                *gcc_args_tailp = arg_list_new(argv[i]);
                gcc_args_tailp = &(*gcc_args_tailp)->next;
                continue;
            }

            switch (argv[i][1]) {
                // These options require thir arguments to be in the option
                // flag.  Their arguments can be anything.  They are copied
                // directly to the GCC command-line.
                case 'O':
                case 'f':
                    *gcc_args_tailp = arg_list_new(argv[i]);
                    gcc_args_tailp = &(*gcc_args_tailp)->next;
                    break;

                // The '-o' option doesn't require its arguments to be in the
                // option flag.  So if the flag doesn't have an argument, parse
                // the next argv element as argument for the option.  The
                // option gets added to the GCC command-line, but it also sets
                // the variable 'out'.
                case 'o':
                    *gcc_args_tailp = arg_list_new(argv[i]);
                    gcc_args_tailp = &(*gcc_args_tailp)->next;

                    if (argv[i][2] != '\0') {
                        out = argv[i] + 1;
                    } else {
                        if ((out = argv[++i]) == NULL)
                            missing_arg_for_opt(argv[i - 1]);
                        *gcc_args_tailp = arg_list_new(argv[i]);
                        gcc_args_tailp = &(*gcc_args_tailp)->next;
                    }

                    break;

                // These options doesn't require their arguments to be in the
                // option flag.  So if the flag doesn't have an argument, parse
                // the next argv element as argument for the option.  The
                // options are copied directly to the GCC command-line.
                case 'l':
                case 'L':
                    *gcc_args_tailp = arg_list_new(argv[i]);
                    gcc_args_tailp = &(*gcc_args_tailp)->next;

                    if (argv[i][2] == '\0') {
                        if (argv[++i] == NULL)
                            missing_arg_for_opt(argv[i - 1]);
                        *gcc_args_tailp = arg_list_new(argv[i]);
                        gcc_args_tailp = &(*gcc_args_tailp)->next;
                    }
                    break;

                case 'c':
                    mode = TO_OBJ;
                    if (argv[i][2] != '\0')
                        unrecognized_opt(argv[i]);

                    *gcc_args_tailp = arg_list_new(argv[i]);
                    gcc_args_tailp = &(*gcc_args_tailp)->next;
                    break;

                case 'S':
                    mode = TO_ASM;
                    if (argv[i][2] != '\0')
                        unrecognized_opt(argv[i]);

                    *gcc_args_tailp = arg_list_new(argv[i]);
                    gcc_args_tailp = &(*gcc_args_tailp)->next;
                    break;

                default:
                    unrecognized_opt(argv[i]);
            }
            continue;
        }

        // Arguments left are input files.  These are interpreted differently
        // depending on their extension, like .o for object files passed to the
        // linker or .s for assembly files passed to the assembler.  Only .z
        // and .zc files are recognized as source files for the language, all
        // other file types are passed directly to the GCC command-line.
        const char *ext = get_file_ext(argv[i]);
        input_file_count++;

        if (ext == NULL || (strcmp(ext, "z") != 0 &&
                    strcmp(ext, "zc") != 0 && strcmp(ext, "Z") != 0 &&
                    strcmp(ext, "ZC") != 0)) {
            *gcc_args_tailp = arg_list_new(argv[i]);
            gcc_args_tailp = &(*gcc_args_tailp)->next;
            continue;
        }

        *src_files_tailp = arg_list_new(argv[i]);
        src_files_tailp = &(*src_files_tailp)->next;
    }

    if (input_file_count == 0) {
        fatal(-1, "no input files");
        exit(1);
    }


    if (out != NULL && input_file_count != 1 && mode != TO_EXE) {
        fatal(-1, "cannot specify '-o' with '-c', '-S', or '-C' with "
                "multiple files\n");
        exit(1);
    }

    if (src_files != NULL) {
        if (mode == TO_C) {
            gen_c(src_files, out);
        } else if (mode == AST_PRINT) {
            print_ast_files(src_files);
        } else {
            invoke_gcc(src_files, gcc_args, gcc_args_tailp);
        }
    }

    arg_list_del(src_files);
    arg_list_del(gcc_args);
}
