#include <string.h>
#include <stdint.h>

void cache_init(void);

struct cache *cache_create(uint32_t object_size);
void cache_shrink(struct cache *cache);
void cache_destroy(struct cache *cache);

void *cache_alloc(struct cache *cache);
void cache_free(struct cache *cache, void *object);

void *kmalloc(size_t size);
void kfree(void *ptr);
