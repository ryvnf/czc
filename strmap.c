#include "zc.h"

struct strmap_e {
	char *key;
	void *val;
	struct strmap_e *next;
};

struct strmap {
	size_t n;
	struct strmap_e **map;
};

struct strmap *strmap_new(size_t n)
{
    struct strmap *map = malloc(sizeof *map);
    map->n = n;
    map->map = calloc(n, sizeof *map->map);

    return map;
}

/* hash string to 64-bit hash using the fnv-1a 64-bit hash function */
static uint64_t strhash64(const char *s)
{
	uint64_t h = 14695981039346656037ULL;
	for (int i = 0; s[i] != '\0'; ++i) {
		h ^= s[i];
		h *= 1099511628211;
	}
	return h;
}

void *strmap_get(struct strmap *map, const char *key)
{
    size_t h = strhash64(key) % map->n;
    for (struct strmap_e *e = map->map[h]; e != NULL; e = e->next) {
        if (strcmp(key, e->key) == 0)
            return e->val;
    }
    return NULL;
}

void strmap_add(struct strmap *map, const char *key, void *val)
{
    int n = strhash64(key) % map->n;
    struct strmap_e *e = malloc(sizeof *e);

    e->key = strdup(key);
    e->val = val;
    e->next = map->map[n];

    map->map[n] = e;
}

void del_strmap_e(struct strmap_e *e, void (*val_free)(void *))
{
    if (e == NULL)
        return;

    del_strmap_e(e->next, val_free);
    free(e->key);

    if (val_free != NULL)
        val_free(e->val);

    free(e);
}

void strmap_del(struct strmap *map, void (*val_free)(void *))
{
    for (size_t i = 0; i < map->n; ++i)
        del_strmap_e(map->map[i], val_free);

    free(map->map);
}
