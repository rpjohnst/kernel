#include <stdint.h>

void memory_add(uint64_t base, uint64_t size);
void memory_reserve(uint64_t base, uint64_t size);

void memory_pages_next(uint64_t *iterator, uint64_t *out_start, uint64_t *out_end);
void memory_free_next(uint64_t *iterator, uint64_t *out_start, uint64_t *out_end);

uint64_t memory_end();
uint64_t memory_find(uint64_t start, uint64_t end, uint64_t size, uint64_t align);
void *memory_alloc(uint64_t start, uint64_t end, uint64_t size, uint64_t align);
