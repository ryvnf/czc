#include "zc.h"

void error(struct loc *loc, const char *fmt, ...) {
	assert(fmt);

	va_list va;
	va_start(va, fmt);

        if (loc != NULL)
            fprintf(stderr, "%s:%d: error: ", loc->file, loc->line);
        else
            fprintf(stderr, "error: ");

	vfprintf(stderr, fmt, va);
        putc('\n', stderr);

	va_end(va);
}

void fatal(struct loc *loc, const char *fmt, ...) {
	assert(fmt);

	va_list va;
	va_start(va, fmt);

        if (loc != NULL)
            fprintf(stderr, "%s:%d: error: ", loc->file, loc->line);
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
