#include "list.h"
#include <stdint.h>

struct page {
	union {
		struct list free;

		struct {
			struct cache *cache;
			struct slab *slab;
		};
	};

	uint32_t ref_count;
};

void page_alloc_init(void);

struct page *page_alloc();
void page_free(struct page *page);

void *page_address(struct page *page);
struct page *page_from_address(void *address);
