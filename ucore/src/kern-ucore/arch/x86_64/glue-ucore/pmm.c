#include <pmm.h>
#include <slab.h>
#include <error.h>
#include <assert.h>
#include <atom.h>
#include <string.h>
#include <kio.h>
#include <swap.h>
#include <glue_mp.h>
#include <glue_intr.h>
#include <mp.h>

static volatile size_t used_pages;
PLS list_entry_t pls_page_struct_free_list;

static struct Page *
page_struct_alloc(uintptr_t pa)
{
	list_entry_t *page_struct_free_list = pls_get_ptr(page_struct_free_list);

	if (list_empty(page_struct_free_list))
	{
		struct Page *p = KADDR_DIRECT(kalloc_pages(1));
		
		int i;
		for (i = 0; i < PGSIZE / sizeof(struct Page); ++ i)
			list_add(page_struct_free_list, &p[i].page_link);
	}

	list_entry_t *entry = list_next(page_struct_free_list);
	list_del(entry);

	struct Page *page = le2page(entry, page_link);

	page->pa = pa;
	atomic_set(&page->ref, 0);
	page->flags = 0;
	list_init(entry);
	
	return page;
}

static void
page_struct_free(struct Page *page)
{
	list_entry_t *page_struct_free_list = pls_get_ptr(page_struct_free_list);
	list_add(page_struct_free_list, &page->page_link);
}

struct Page *
alloc_pages(size_t npages)
{
	struct Page *result;
	uintptr_t base = kalloc_pages(npages);
	size_t i;
	int flags;

	local_intr_save_hw(flags);
	
	result = page_struct_alloc(base);
	kpage_private_set(base, result);

	for (i = 1; i < npages; ++ i)
	{
		struct Page *page = page_struct_alloc(base + i * PGSIZE);
		kpage_private_set(base + i * PGSIZE, page);
	}

	while (1)
	{
		size_t old = used_pages;
		if (cmpxchg64(&used_pages, old, old + npages) == old) break;
	}

	local_intr_restore_hw(flags);
	return result;
}

void
free_pages(struct Page *base, size_t npages)
{
	uintptr_t basepa = page2pa(base);
	size_t i;
	int flags;

	local_intr_save_hw(flags);
	
	for (i = 0; i < npages; ++ i)
	{
		page_struct_free(kpage_private_get(basepa + i * PGSIZE));
	}
	
	kfree_pages(basepa, npages);
	while (1)
	{
		size_t old = used_pages;
		if (cmpxchg64(&used_pages, old, old - npages) == old) break;
	}

	local_intr_restore_hw(flags);
}

size_t
nr_used_pages(void)
{
	return used_pages;
}

void
pmm_init(void)
{
	used_pages = 0;
}

void
pmm_init_ap(void)
{
	list_entry_t *page_struct_free_list = pls_get_ptr(page_struct_free_list);
	list_init(page_struct_free_list);
}
