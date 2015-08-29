#include "page.h"
#include "list.h"
#include <cache.h>
#include <paging.h>
#include <assert.h>
#include <stddef.h>
#include <stdalign.h>
#include <stdbool.h>

// TODO: larger slabs
// TODO: off-slab headers
// TODO: cache line alignment and coloring
// TODO: per-cpu object caches

struct cache {
	struct list partial;
	struct list empty;
	struct list full;

	uint32_t object_size;
	uint32_t slab_capacity;
};

struct slab {
	struct list list;

	void *objects;

	uint32_t ref_count;
	uint32_t next_free;
	uint32_t free[];
};

static struct cache caches = {
	.full = LIST_INIT(caches.full),
	.partial = LIST_INIT(caches.partial),
	.empty = LIST_INIT(caches.empty),

	.object_size = sizeof(struct cache),
};

// TODO: go bigger once we have bigger slab sizes
static struct {
	size_t size;
	struct cache *cache;
} sized_caches[] = {
	{ 8, NULL },
	{ 16, NULL },
	{ 32, NULL },
	{ 64, NULL },
	{ 96, NULL },
	{ 128, NULL },
	{ 192, NULL },
	{ 256, NULL },
	{ 512, NULL },
	{ 1024, NULL },
	{ 2048, NULL },
	{ 0, NULL },
};

static uint32_t calc_slab_capacity(uint32_t object_size) {
	uint32_t header = sizeof(struct slab);
	uint32_t entry = sizeof(uint32_t);

	uint32_t i = 0;
	while (round_up(header + i * entry, object_size) + i * object_size < PAGE_SIZE)
		i++;

	return i;
}

void cache_init(void) {
	caches.slab_capacity = calc_slab_capacity(caches.object_size);

	for (int i = 0; sized_caches[i].size != 0; i++)
		sized_caches[i].cache = cache_create(sized_caches[i].size);
}

struct cache *cache_create(uint32_t object_size) {
	struct cache *cache = cache_alloc(&caches);
	if (cache == NULL)
		return NULL;

	list_init(&cache->full);
	list_init(&cache->partial);
	list_init(&cache->empty);

	cache->object_size = object_size;
	cache->slab_capacity = calc_slab_capacity(cache->object_size);

	return cache;
}

void cache_shrink(struct cache *cache) {
	while (cache->empty.next != &cache->empty) {
		struct slab *slab = containerof(cache->empty.next, struct slab, list);
		list_del(cache->empty.next);

		page_free(page_from_address(slab));
	}
}

void cache_destroy(struct cache *cache) {
	cache_shrink(cache);
	assert(list_empty(&cache->partial) && list_empty(&cache->full));
	cache_free(&caches, cache);
}

static struct slab *slab_create(struct cache *cache) {
	struct page *page = page_alloc();
	if (page == NULL)
		return NULL;

	struct slab *slab = page_address(page);
	list_init(&slab->list);

	page->cache = cache;
	page->slab = slab;

	slab->objects = (char*)slab + sizeof(struct slab) + cache->slab_capacity * sizeof(uint32_t);

	slab->ref_count = 0;
	slab->next_free = 0;
	for (uint32_t i = 0; i < cache->slab_capacity; i++)
		slab->free[i] = i + 1;

	return slab;
}

void *cache_alloc(struct cache *cache) {
	struct list *entry = cache->partial.next;

	if (list_empty(&cache->partial)) {
		entry = cache->empty.next;

		if (list_empty(&cache->empty)) {
			struct slab *slab = slab_create(cache);
			if (slab == NULL)
				return NULL;

			entry = &slab->list;
			list_add_head(entry, &cache->partial);
		}
	}

	struct slab *slab = containerof(entry, struct slab, list);
	void *object = (char*)slab->objects + slab->next_free * cache->object_size;

	slab->ref_count++;
	slab->next_free = slab->free[slab->next_free];
	if (slab->next_free == cache->slab_capacity) {
		list_del(&slab->list);
		list_add_head(&slab->list, &cache->full);
	}

	return object;
}

void cache_free(struct cache *cache, void *object) {
	struct slab *slab = (struct slab*)((intptr_t)object & PAGE_MASK);

	uint32_t index = ((char*)object - (char*)slab->objects) / cache->object_size;
	slab->free[index] = slab->next_free;
	slab->next_free = index;

	uint32_t ref_count = slab->ref_count;
	slab->ref_count--;
	if (slab->ref_count == 0) {
		list_del(&slab->list);
		list_add_head(&slab->list, &cache->empty);
	}
	else if (ref_count == cache->slab_capacity) {
		list_del(&slab->list);
		list_add_head(&slab->list, &cache->partial);
	}
}

// TODO: fall back on large page allocation once we have large pages
void *kmalloc(size_t size) {
	for (int i = 0; sized_caches[i].size != 0; i++) {
		if (size > sized_caches[i].size)
			continue;

		return cache_alloc(sized_caches[i].cache);
	}

	return NULL;
}

void kfree(void *ptr) {
	if (ptr == NULL)
		return;

	struct page *page = page_from_address(ptr);
	cache_free(page->cache, ptr);
}
