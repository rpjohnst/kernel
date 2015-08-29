#include "page.h"
#include "memory.h"
#include <paging.h>
#include <stdalign.h>
#include <limits.h>

// TODO: larger pages

static uint64_t num_frames;
static struct page *page_frames;
static struct list free_pages;

void page_alloc_init(void) {
	// TODO: allocate page_frames on a page boundary and map it to a fixed location
	num_frames = memory_end() >> PAGE_SHIFT;
	page_frames = memory_alloc(
		PAGE_SIZE, memory_end(), num_frames * sizeof(*page_frames), alignof(*page_frames)
	);

	list_init(&free_pages);
	for (uint64_t i = 0; i < num_frames; i++) {
		page_frames[i].ref_count = 1;
		list_init(&page_frames[i].free);
	}

	uint64_t i = 0, found_start, found_end;
	while (memory_free_next(&i, &found_start, &found_end), i != (uint64_t)-1) {
		uint64_t page_start = (found_start + PAGE_SIZE - 1) >> PAGE_SHIFT;
		uint64_t page_end = found_end >> PAGE_SHIFT;

		if (page_start >= page_end)
			continue;

		for (uint64_t i = page_start; i < page_end; i++) {
			list_add_tail(&page_frames[i].free, &free_pages);
		}
	}
}

struct page *page_alloc() {
	// TODO: free up cache space on OOM
	if (list_empty(&free_pages))
		return NULL;

	struct page *page = containerof(free_pages.next, struct page, free);
	list_del(&page->free);

	page->ref_count++;
	return page;
}

void page_free(struct page *page) {
	page->ref_count--;
	if (page->ref_count == 0) {
		list_add_head(&page->free, &free_pages);
	}
}

void *page_address(struct page *page) {
	return VIRT_DIRECT((page - page_frames) * PAGE_SIZE);
}

struct page *page_from_address(void *address) {
	return &page_frames[PHYS_DIRECT(address) >> PAGE_SHIFT];
}
