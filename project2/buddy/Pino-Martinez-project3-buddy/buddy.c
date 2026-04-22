/**
 * Buddy Allocator
 *
 * For the list library usage, see http://www.mcs.anl.gov/~kazutomo/list/
 */

/**************************************************************************
 * Conditional Compilation Options
 **************************************************************************/
#define USE_DEBUG 0

/**************************************************************************
 * Included Files
 **************************************************************************/
#include <stdio.h>
#include <stdlib.h>

#include "buddy.h"
#include "list.h"

/**************************************************************************
 * Public Definitions
 **************************************************************************/
#define MIN_ORDER 12
#define MAX_ORDER 20

#define PAGE_SIZE (1<<MIN_ORDER)
/* page index to address */
#define PAGE_TO_ADDR(page_idx) (void *)((page_idx*PAGE_SIZE) + g_memory)

/* address to page index */
#define ADDR_TO_PAGE(addr) ((unsigned long)((void *)addr - (void *)g_memory) / PAGE_SIZE)

/* find buddy address */
#define BUDDY_ADDR(addr, o) (void *)((((unsigned long)addr - (unsigned long)g_memory) ^ (1<<o)) \
									 + (unsigned long)g_memory)

#if USE_DEBUG == 1
#  define PDEBUG(fmt, ...) \
	fprintf(stderr, "%s(), %s:%d: " fmt,			\
		__func__, __FILE__, __LINE__, ##__VA_ARGS__)
#  define IFDEBUG(x) x
#else
#  define PDEBUG(fmt, ...)
#  define IFDEBUG(x)
#endif

/**************************************************************************
 * Public Types
 **************************************************************************/
/* BEGIN MODIFICATION: Added 'order' and 'free' fields to page_t.
 * - order: tracks the block size order this page heads (MIN_ORDER..MAX_ORDER)
 * - free:  1 if this page is the head of a free block, 0 if allocated
 */
typedef struct {
	struct list_head list;
	int order; /* Order of the block this page represents */
	int free;  /* 1 = head of a free block, 0 = allocated */
} page_t;
/* END MODIFICATION */

/**************************************************************************
 * Global Variables
 **************************************************************************/
/* free lists*/
struct list_head free_area[MAX_ORDER+1];

/* memory area */
char g_memory[1<<MAX_ORDER];

/* page structures */
page_t g_pages[(1<<MAX_ORDER)/PAGE_SIZE];

/**************************************************************************
 * Public Function Prototypes
 **************************************************************************/

/**************************************************************************
 * Local Functions
 **************************************************************************/

/**
 * Initialize the buddy system
 */
void buddy_init()
{
	int i;
	int n_pages = (1<<MAX_ORDER) / PAGE_SIZE;

	/* BEGIN MODIFICATION: Initialize every page_t to order=0, free=0 */
	for (i = 0; i < n_pages; i++) {
		g_pages[i].order = 0;
		g_pages[i].free  = 0;
		INIT_LIST_HEAD(&g_pages[i].list);
	}
	/* END MODIFICATION */

	/* initialize freelist */
	for (i = MIN_ORDER; i <= MAX_ORDER; i++) {
		INIT_LIST_HEAD(&free_area[i]);
	}

	/* BEGIN MODIFICATION: Mark page 0 as the single free block of MAX_ORDER */
	g_pages[0].order = MAX_ORDER;
	g_pages[0].free  = 1;
	/* END MODIFICATION */

	/* add the entire memory as a freeblock */
	list_add(&g_pages[0].list, &free_area[MAX_ORDER]);
}

/**
 * Allocate a memory block.
 *
 * On a memory request, the allocator returns the head of a free-list of the
 * matching size (i.e., smallest block that satisfies the request). If the
 * free-list of the matching block size is empty, then a larger block size will
 * be selected. The selected (large) block is then splitted into two smaller
 * blocks. Among the two blocks, left block will be used for allocation or be
 * further splitted while the right block will be added to the appropriate
 * free-list.
 *
 * @param size size in bytes
 * @return memory block address
 */
void *buddy_alloc(int size)
{
	/* BEGIN MODIFICATION: buddy_alloc implementation */

	/* Guard against invalid sizes */
	if (size <= 0) return NULL;

	/* ----------------------------------------------------------------
	 * Step 1: Find the smallest order whose block size >= requested size.
	 * Block size at order O is (1 << O) bytes.
	 * ---------------------------------------------------------------- */
	int order = MIN_ORDER;
	while ((1 << order) < size) {
		order++;
	}

	if (order > MAX_ORDER) return NULL; /* Request too large */

	/* ----------------------------------------------------------------
	 * Step 2: Walk up free_area[] to find the smallest available order
	 * that can satisfy the request.
	 * ---------------------------------------------------------------- */
	int found_order = order;
	while (found_order <= MAX_ORDER && list_empty(&free_area[found_order])) {
		found_order++;
	}

	if (found_order > MAX_ORDER) return NULL; /* Out of memory */

	/* ----------------------------------------------------------------
	 * Step 3: Remove the chosen block from its free list.
	 * ---------------------------------------------------------------- */
	page_t *page = list_entry(free_area[found_order].next, page_t, list);
	list_del_init(&page->list);
	page->free = 0;

	/* ----------------------------------------------------------------
	 * Step 4: Split the block down to the target order.
	 * Each split: the right (buddy) half is added to the lower free list,
	 * and the left half is either split again or used for the allocation.
	 * ---------------------------------------------------------------- */
	while (found_order > order) {
		found_order--;

		/* Compute the right-buddy address at the new (smaller) order */
		int    page_idx   = (int)(page - g_pages);
		void  *addr       = PAGE_TO_ADDR(page_idx);
		void  *buddy_addr = BUDDY_ADDR(addr, found_order);
		int    buddy_idx  = (int)ADDR_TO_PAGE(buddy_addr);
		page_t *buddy     = &g_pages[buddy_idx];

		/* Place the right buddy onto the free list at found_order */
		buddy->order = found_order;
		buddy->free  = 1;
		list_add(&buddy->list, &free_area[found_order]);

		/* The left block (page) continues at the smaller order */
		page->order = found_order;
	}

	/* Mark the allocated block */
	page->order = order;
	page->free  = 0;

	return PAGE_TO_ADDR((int)(page - g_pages));
	/* END MODIFICATION */
}

/**
 * Free an allocated memory block.
 *
 * Whenever a block is freed, the allocator checks its buddy. If the buddy is
 * free as well, then the two buddies are combined to form a bigger block. This
 * process continues until one of the buddies is not free.
 *
 * @param addr memory block address to be freed
 */
void buddy_free(void *addr)
{
	/* BEGIN MODIFICATION: buddy_free implementation */

	/* Locate the page descriptor for this address */
	int     page_idx = (int)ADDR_TO_PAGE(addr);
	page_t *page     = &g_pages[page_idx];
	int     order    = page->order;

	/* Mark this block as free */
	page->free = 1;

	/* ----------------------------------------------------------------
	 * Coalescing loop:
	 * While we are below MAX_ORDER, check if our buddy is also free
	 * and at the same order. If so, merge and try again at order+1.
	 * ---------------------------------------------------------------- */
	while (order < MAX_ORDER) {
		void   *buddy_addr = BUDDY_ADDR(addr, order);
		int     buddy_idx  = (int)ADDR_TO_PAGE(buddy_addr);
		page_t *buddy      = &g_pages[buddy_idx];

		/* Buddy must be free AND at exactly the same order to coalesce */
		if (!buddy->free || buddy->order != order) {
			break;
		}

		/* Remove the buddy from its free list */
		list_del_init(&buddy->list);
		buddy->free = 0;

		/* The merged block starts at whichever address is lower */
		if (buddy_addr < addr) {
			addr = buddy_addr;
			page = buddy;
		}

		/* Step up to the next order */
		order++;
		page->order = order;
		page->free  = 1;
	}

	/* Insert the (possibly coalesced) block into the appropriate free list */
	list_add(&page->list, &free_area[order]);
	/* END MODIFICATION */
}

/**
 * Print the buddy system status---order oriented
 *
 * print free pages in each order.
 */
void buddy_dump()
{
	int o;
	for (o = MIN_ORDER; o <= MAX_ORDER; o++) {
		struct list_head *pos;
		int cnt = 0;
		list_for_each(pos, &free_area[o]) {
			cnt++;
		}
		printf("%d:%dK ", cnt, (1<<o)/1024);
	}
	printf("\n");
}