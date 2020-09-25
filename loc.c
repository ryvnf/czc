#include "zc.h"

struct loc *loc_new(const char *file, int line)
{
    struct loc *l = malloc(sizeof *l);
    l->line = line;
    l->file = strdup(file);
    l->nref = 1;
    return l;
}

void loc_del(struct loc *loc)
{
    free(loc->file);
    free(loc);
}

struct loc *loc_ref(struct loc *loc)
{
    loc->nref++;
    return loc;
}

void loc_unref(struct loc *loc)
{
    // TODO: Fix memory leaks later
    //if (--loc->nref == 0)
    //    loc_del(loc);
}
