#ifndef STRMAP_H
#define STRMAP_H

// Data structure to map strings to void pointers.
struct strmap;

struct strmap *strmap_new(size_t n);
void **strmap_get_ptr(struct strmap *map, const char *key);
void *strmap_get(struct strmap *map, const char *key);
void strmap_add(struct strmap *map, const char *key, void *val);
void strmap_del(struct strmap *map, void (*val_free)(void *));

#endif // !defined STRMAP_H
