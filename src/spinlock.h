#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>

struct spinlock {
	struct spinlock_node *tail;
};

struct spinlock_node {
	struct spinlock_node *next;
	bool locked;
};

static inline void spin_lock(struct spinlock *lock, struct spinlock_node *node) {
	node->next = NULL;
	node->locked = false;

	struct spinlock_node *prev = atomic_exchange_explicit(&lock->tail, node, memory_order_acquire);
	if (prev == NULL) {
		return;
	}

	atomic_store_explicit(&prev->next, node, memory_order_relaxed);
	while (!atomic_load_explicit(&node->locked, memory_order_acquire)) {
		__asm__ volatile ("pause");
	}
}

static inline void spin_unlock(struct spinlock *lock, struct spinlock_node *node) {
	struct spinlock_node *next = atomic_load_explicit(&node->next, memory_order_relaxed);
	if (next == NULL) {
		struct spinlock_node *prev = node;
		if (atomic_compare_exchange_strong_explicit(
			&lock->tail, &prev, NULL, memory_order_release, memory_order_relaxed
		)) {
			return;
		}
		while ((next = atomic_load_explicit(&node->next, memory_order_relaxed)) == NULL) {
			__asm__ volatile ("pause");
		}
	}
	atomic_store_explicit(&next->locked, true, memory_order_release);
}
