#ifndef ERROR_H
#define ERROR_H

/* print error message */
void error(int line, const char *fmt, ...);

/* print error message and exit program */
noreturn void fatal(int line, const char *fmt, ...);

/* print error message and abort program */
noreturn void bug(const char *fmt, ...);

/* bug if this statement gets reached */
#define unreachable() bug("%s:%d `unreachable()' reached in `%s'", __FILE__, \
                          __LINE__, __func__)

/* bug if condition isn't true */
#define assert(cond) if (!(cond)) bug("%s:%d `assert(%s)' failed in `%s'", \
                                      __FILE__, __LINE__, #cond, __func__)

#endif /* !defined ERROR_H */
