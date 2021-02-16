// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * list.h - simple list primitives
 */

#ifndef LIST_H_
#define LIST_H_

#define container_of(ptr, type, member)                      \
	({                                                   \
		void *__mptr = (void *)(ptr);                \
		((type *)(__mptr - offsetof(type, member))); \
	})

struct list_head {
	struct list_head *next;
	struct list_head *prev;
};

typedef struct list_head list_t;

#define LIST_HEAD_INIT(name)                     \
	{                                        \
		.next = &(name), .prev = &(name) \
	}

#define LIST_HEAD(name) struct list_head name = LIST_HEAD_INIT(name)

#define INIT_LIST_HEAD(ptr)          \
	({                           \
		(ptr)->next = (ptr); \
		(ptr)->prev = (ptr); \
	})

static inline int
list_empty(const struct list_head *head)
{
	return head->next == head;
}

static inline void
__list_add(struct list_head *new, struct list_head *prev,
           struct list_head *next)
{
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

static inline void
list_add(struct list_head *new, struct list_head *head)
{
	__list_add(new, head, head->next);
}

static inline void
list_add_tail(struct list_head *new, struct list_head *head)
{
	__list_add(new, head->prev, head);
}

static inline void
__list_del(struct list_head *prev, struct list_head *next)
{
	next->prev = prev;
	prev->next = next;
}

static inline void
__list_del_entry(struct list_head *entry)
{
	__list_del(entry->prev, entry->next);
}

static inline void
list_del(struct list_head *entry)
{
	__list_del_entry(entry);
	entry->next = NULL;
	entry->prev = NULL;
}

#define list_entry(ptr, type, member) container_of(ptr, type, member)

#define list_first_entry(ptr, type, member) \
	list_entry((ptr)->next, type, member)

#define list_last_entry(ptr, type, member) list_entry((ptr)->prev, type, member)

#define list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

#define list_for_each_safe(pos, n, head)                       \
	for (pos = (head)->next, n = pos->next; pos != (head); \
	     pos = n, n = pos->next)

#define list_for_each_prev(pos, head) \
	for (pos = (head)->prev; pos != (head); pos = pos->prev)

#define list_for_each_prev_safe(pos, n, head)                  \
	for (pos = (head)->prev, n = pos->prev; pos != (head); \
	     pos = n, n = pos->prev)

#endif /* !LIST_H_ */
// vim:fenc=utf-8:tw=75:noet
