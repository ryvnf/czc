#ifndef LOC_H
#define LOC_H

struct loc {
    char *file;
    int line;
    int nref;
};

struct loc *loc_new(const char *file, int line);

struct loc *loc_ref(struct loc *loc);
void loc_unref(struct loc *loc);

#endif
