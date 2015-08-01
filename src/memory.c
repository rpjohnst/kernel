#include "memory.h"
#include <paging.h>
#include <string.h>
#include <assert.h>
#include <stddef.h>

// TODO: move these somewhere useful
#define min(x, y) ((x) < (y) ? (x) : (y))
#define max(x, y) ((x) > (y) ? (x) : (y))
#define clamp(x, start, end) min(max((x), (start)), (end))
#define align(x, align) (((x) + (align) - 1) & ~((align) - 1))

struct memory_map {
	size_t count;
	size_t capacity;
	struct memory_region *regions;
};

struct memory_region {
	uint64_t base;
	uint64_t size;
};

static struct memory_region memory_regions[128];
static struct memory_region reserved_regions[128];

static struct memory_map memory = {
	.count = 0,
	.capacity = 128,
	.regions = memory_regions,
};

static struct memory_map reserved = {
	.count = 0,
	.capacity = 128,
	.regions = reserved_regions,
};

// insert a non-overlapping, ordered region into map
static void memory_map_insert(struct memory_map *map, size_t i, uint64_t base, uint64_t size) {
	assert(map->count <= map->capacity);

	struct memory_region *region = &map->regions[i];
	memmove(region + 1, region, (map->count - i) * sizeof(*region));
	region->base = base;
	region->size = size;

	map->count++;
}

// coalesce adjacent regions in map
static void memory_map_merge(struct memory_map *map) {
	size_t i = 1;
	while (i < map->count) {
		struct memory_region *this = &map->regions[i - 1];
		struct memory_region *next = &map->regions[i];

		if (this->base + this->size != next->base) {
			assert(this->base + this->size <= next->base);
			i++;
			continue;
		}

		this->size += next->size;
		memmove(next, next + 1, (map->count - i) * sizeof(*next));
		map->count--;
	}
}

// add a new region to map, taking care of overlaps
static void memory_map_add(struct memory_map *map, uint64_t base, uint64_t size) {
	uint64_t end = base + size;

	size_t i;
	for (i = 0; i < map->count; i++) {
		struct memory_region *region = &map->regions[i];
		uint64_t region_end = region->base + region->size;

		// skip regions fully below the new one
		if (region_end <= base)
			continue;

		// stop for regions fully above the new one
		if (region->base >= end)
			break;

		// insert the piece of new region below
		if (region->base > base)
			memory_map_insert(map, i++, base, region->base - base);

		base = min(region_end, end);
	}

	// insert any leftover piece of new region
	if (base < end)
		memory_map_insert(map, i, base, end - base);

	memory_map_merge(map);
}

// add a region of available memory
void memory_add(uint64_t base, uint64_t size) {
	memory_map_add(&memory, base, size);
}

// reserve a region of memory
void memory_reserve(uint64_t base, uint64_t size) {
	memory_map_add(&reserved, base, size);
}

// iterator for pages of existing memory
void memory_pages_next(uint64_t *iterator, uint64_t *out_start, uint64_t *out_end) {
	for (; *iterator < memory.count; ++*iterator) {
		struct memory_region *region = &memory.regions[*iterator];

		uint64_t page_start = (region->base + PAGE_SIZE - 1) >> PAGE_SHIFT;
		uint64_t page_end = (region->base + region->size) >> PAGE_SHIFT;

		// skip regions smaller than one page
		if (page_start >= page_end)
			continue;

		*out_start = page_start;
		*out_end = page_end;

		++*iterator;
		return;
	}

	*iterator = (uint64_t)-1;
}

// iterator for free memory regions
void memory_free_next(uint64_t *iterator, uint64_t *out_start, uint64_t *out_end) {
	uint32_t ia = *iterator & 0xffffffff;
	uint32_t ie = *iterator >> 32;

	// loop through available regions
	for (; ia < memory.count; ia++) {
		struct memory_region *available = &memory.regions[ia];
		uint64_t available_start = available->base;
		uint64_t available_end = available->base + available->size;

		// loop through negative space of exclude
		for (; ie < reserved.count + 1; ie++) {
			struct memory_region *exclude = &reserved.regions[ie];
			uint64_t include_start = ie == 0 ? 0 : exclude[-1].base + exclude[-1].size;
			uint64_t include_end = ie < reserved.count ? exclude->base : (uint64_t)-1;

			// we passed the end of the current available region, go to the next one
			if (include_start >= available_end)
				break;

			// keep going until the include region overlaps the available region
			if (include_end <= available_start)
				continue;

			// output intersection of the two regions
			*out_start = max(available_start, include_start);
			*out_end = min(available_end, include_end);

			// advance the lowest region once more before returning
			if (available_end <= include_end)
				ia++;
			else
				ie++;

			*iterator = (uint64_t)ia | (uint64_t)ie << 32;
			return;
		}
	}

	*iterator = (uint64_t)-1;
}

uint64_t memory_end() {
	assert(memory.count > 0);
	return memory.regions[memory.count - 1].base + memory.regions[memory.count - 1].size;
}

// find a free region in [start, end) with a power-of-two alignment
uint64_t memory_find(uint64_t start, uint64_t end, uint64_t size, uint64_t align) {
	uint64_t i = 0, found_start, found_end;
	while (memory_free_next(&i, &found_start, &found_end), i != (uint64_t)-1) {
		found_start = clamp(found_start, start, end);
		found_end = clamp(found_end, start, end);

		uint64_t align_start = align(found_start, align);
		assert(align_start != 0); // TODO: remove this after unconditionally reserving first page?
		if (align_start < found_end && found_end - align_start >= size)
			return align_start;
	}

	return 0;
}

void *memory_alloc(uint64_t start, uint64_t end, uint64_t size, uint64_t align) {
	uint64_t phys = memory_find(start, end, size, align);
	memory_reserve(phys, size);

	void *virt = VIRT_DIRECT(phys);
	memset(virt, 0, size);
	return virt;
}
