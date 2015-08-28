void page_alloc_init(void);

struct page *page_alloc();
void page_free(struct page *page);

void *page_address(struct page *page);
struct page *page_from_address(void *address);
