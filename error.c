#include "zc.h"

void error(int line, const char *fmt, ...) {
	assert(fmt);

	va_list va;
	va_start(va, fmt);

        if (line >= 0)
            fprintf(stderr, "%d: error: ", line);
        else
            fprintf(stderr, "error: ");

	vfprintf(stderr, fmt, va);
        putc('\n', stderr);

	va_end(va);
}

void fatal(int line, const char *fmt, ...) {
	assert(fmt);

	va_list va;
	va_start(va, fmt);

        if (line >= 0)
            fprintf(stderr, "%d: error: ", line);
        else
            fprintf(stderr, "error: ");

	vfprintf(stderr, fmt, va);
        putc('\n', stderr);

	va_end(va);

        exit(1);
}

void bug(const char *fmt, ...) {
	assert(fmt); /* catching bugs within bugs */

	va_list va;
	va_start(va, fmt);

        fprintf(stderr, "internal error: ");
	vfprintf(stderr, fmt, va);
        putc('\n', stderr);

	va_end(va);
	abort();
}
