#ifndef LIST_H
#define LIST_H

#include <stddef.h>
#include <stdbool.h>

struct list {
	struct list *prev, *next;
};

#define LIST_INIT(list) { &(list), &(list) }

static inline void list_init(struct list *list) {
	list->prev = list;
	list->next = list;
}

static inline void list_insert(struct list *new, struct list *prev, struct list *next) {
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

static inline void list_add_head(struct list *new, struct list *head) {
	list_insert(new, head, head->next);
}

static inline void list_add_tail(struct list *new, struct list *head) {
	list_insert(new, head->prev, head);
}

static inline void list_remove(struct list *prev, struct list *next) {
	next->prev = prev;
	prev->next = next;
}

static inline void list_del(struct list *entry) {
	list_remove(entry->prev, entry->next);
}

static inline bool list_empty(struct list *list) {
	return list->next == list;
}

#define containerof(ptr, type, member) ((type*)((char*)(ptr) - offsetof(type, member)))

#endif
