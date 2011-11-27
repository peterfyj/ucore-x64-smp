#include <pmm.h>
#include <slab.h>
#include <error.h>
#include <assert.h>
#include <string.h>
#include <kio.h>
#include <swap.h>
#include <glue_mp.h>
#include <glue_intr.h>
#include <mp.h>

PLS static size_t pls_used_pages;
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

	pls_write(used_pages, pls_read(used_pages) + npages);

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
	pls_write(used_pages, pls_read(used_pages) - npages);

	local_intr_restore_hw(flags);
}

size_t
nr_used_pages(void)
{
	return pls_read (used_pages);
}

void
pmm_init_ap(void)
{
	list_entry_t *page_struct_free_list = pls_get_ptr(page_struct_free_list);
	list_init(page_struct_free_list);
	pls_write (used_pages, 0);
}

pgd_t *
get_pgd(pgd_t *pgdir, uintptr_t la, bool create) {
    return &pgdir[PGX(la)];
}

pud_t *
get_pud(pgd_t *pgdir, uintptr_t la, bool create) {
#if PUXSHIFT == PGXSHIFT
	return get_pgd(pgdir, la, create);
#else /* PUXSHIFT == PGXSHIFT */
    pgd_t *pgdp;
    if ((pgdp = get_pgd(pgdir, la, create)) == NULL) {
        return NULL;
    }
    if (! ptep_present(pgdp)) {
        struct Page *page;
        if (!create || (page = alloc_page()) == NULL) {
            return NULL;
        }
        set_page_ref(page, 1);
        uintptr_t pa = page2pa(page);
        memset(KADDR(pa), 0, PGSIZE);
        ptep_map(pgdp, pa);
        ptep_set_u_write(pgdp);
    }
    return &((pud_t *)KADDR(PGD_ADDR(*pgdp)))[PUX(la)];
#endif /* PUXSHIFT == PGXSHIFT */
}

pmd_t *
get_pmd(pgd_t *pgdir, uintptr_t la, bool create) {
#if PMXSHIFT == PUXSHIFT
	return get_pud(pgdir, la, create);
#else /* PMXSHIFT == PUXSHIFT */
    pud_t *pudp;
    if ((pudp = get_pud(pgdir, la, create)) == NULL) {
        return NULL;
    }
    if (! ptep_present(pudp)) {
        struct Page *page;
        if (!create || (page = alloc_page()) == NULL) {
            return NULL;
        }
        set_page_ref(page, 1);
        uintptr_t pa = page2pa(page);
        memset(KADDR(pa), 0, PGSIZE);
		ptep_map(pudp, pa);
		ptep_set_u_write(pudp);
    }
    return &((pmd_t *)KADDR(PUD_ADDR(*pudp)))[PMX(la)];
#endif /* PMXSHIFT == PUXSHIFT */
}

pte_t *
get_pte(pgd_t *pgdir, uintptr_t la, bool create) {
#if PTXSHIFT == PMXSHIFT
	return get_pmd(pgdir, la, create);
#else /* PTXSHIFT == PMXSHIFT */
    pmd_t *pmdp;
    if ((pmdp = get_pmd(pgdir, la, create)) == NULL) {
        return NULL;
    }
    if (! ptep_present(pmdp)) {
        struct Page *page;
        if (!create || (page = alloc_page()) == NULL) {
            return NULL;
        }
        set_page_ref(page, 1);
        uintptr_t pa = page2pa(page);
        memset(KADDR(pa), 0, PGSIZE);
		ptep_map(pmdp, pa);
		ptep_set_u_write(pmdp);
    }
    return &((pte_t *)KADDR(PMD_ADDR(*pmdp)))[PTX(la)];
#endif /* PTXSHIFT == PMXSHIFT */
}

struct Page *
get_page(pgd_t *pgdir, uintptr_t la, pte_t **ptep_store) {
    pte_t *ptep = get_pte(pgdir, la, 0);
    if (ptep_store != NULL) {
        *ptep_store = ptep;
    }
    if (ptep != NULL && ptep_present(ptep)) {
        return pa2page(*ptep);
    }
    return NULL;
}

static inline void
page_remove_pte(pgd_t *pgdir, uintptr_t la, pte_t *ptep) {
    if (ptep_present(ptep)) {
        struct Page *page = pte2page(*ptep);
        if (!PageSwap(page)) {
            if (page_ref_dec(page) == 0) {
                free_page(page);
            }
        }
        else {
            if (ptep_dirty(ptep)) {
                SetPageDirty(page);
            }
            page_ref_dec(page);
        }
		ptep_unmap(ptep);
        mp_tlb_invalidate(pgdir, la);
    }
    else if (! ptep_invalid(ptep)) {
        swap_remove_entry(*ptep);
        ptep_unmap(ptep);
    }
}

void
page_remove(pgd_t *pgdir, uintptr_t la) {
    pte_t *ptep = get_pte(pgdir, la, 0);
    if (ptep != NULL) {
        page_remove_pte(pgdir, la, ptep);
    }
}

int
page_insert(pgd_t *pgdir, struct Page *page, uintptr_t la, pte_perm_t perm) {
    pte_t *ptep = get_pte(pgdir, la, 1);
    if (ptep == NULL) {
        return -E_NO_MEM;
    }
    page_ref_inc(page);
    if (*ptep != 0) {
        if (ptep_present(ptep) && pte2page(*ptep) == page) {
            page_ref_dec(page);
            goto out;
        }
        page_remove_pte(pgdir, la, ptep);
    }

out:
	ptep_map(ptep, page2pa(page));
	ptep_set_perm(ptep, perm);
    mp_tlb_invalidate(pgdir, la);
    return 0;
}

struct Page *
pgdir_alloc_page(pgd_t *pgdir, uintptr_t la, uint32_t perm) {
    struct Page *page = alloc_page();
    if (page != NULL) {
        if (page_insert(pgdir, page, la, perm) != 0) {
            free_page(page);
            return NULL;
        }
    }
    return page;
}

static void
unmap_range_pte(pgd_t *pgdir, pte_t *pte, uintptr_t base, uintptr_t start, uintptr_t end) {
    assert(start >= 0 && start < end && end <= PTSIZE);
    assert(start % PGSIZE == 0 && end % PGSIZE == 0);
    do {
        pte_t *ptep = &pte[PTX(start)];
        if (*ptep != 0) {
            page_remove_pte(pgdir, base + start, ptep);
        }
        start += PGSIZE;
    } while (start != 0 && start < end);
}

static void
unmap_range_pmd(pgd_t *pgdir, pmd_t *pmd, uintptr_t base, uintptr_t start, uintptr_t end) {
    assert(start >= 0 && start < end && end <= PMSIZE);
    size_t off, size;
    uintptr_t la = ROUNDDOWN(start, PTSIZE);
    do {
        off = start - la, size = PTSIZE - off;
        if (size > end - start) {
            size = end - start;
        }
        pmd_t *pmdp = &pmd[PMX(la)];
        if (ptep_present(pmdp)) {
            unmap_range_pte(pgdir, KADDR(PMD_ADDR(*pmdp)), base + la, off, off + size);
        }
        start += size, la += PTSIZE;
    } while (start != 0 && start < end);
}

static void
unmap_range_pud(pgd_t *pgdir, pud_t *pud, uintptr_t base, uintptr_t start, uintptr_t end) {
    assert(start >= 0 && start < end && end <= PUSIZE);
    size_t off, size;
    uintptr_t la = ROUNDDOWN(start, PMSIZE);
    do {
        off = start - la, size = PMSIZE - off;
        if (size > end - start) {
            size = end - start;
        }
        pud_t *pudp = &pud[PUX(la)];
        if (ptep_present(pudp)) {
            unmap_range_pmd(pgdir, KADDR(PUD_ADDR(*pudp)), base + la, off, off + size);
        }
        start += size, la += PMSIZE;
    } while (start != 0 && start < end);
}

static void
unmap_range_pgd(pgd_t *pgd, uintptr_t start, uintptr_t end) {
    size_t off, size;
    uintptr_t la = ROUNDDOWN(start, PUSIZE);
    do {
        off = start - la, size = PUSIZE - off;
        if (size > end - start) {
            size = end - start;
        }
        pgd_t *pgdp = &pgd[PGX(la)];
        if (ptep_present(pgdp)) {
            unmap_range_pud(pgd, KADDR(PGD_ADDR(*pgdp)), la, off, off + size);
        }
        start += size, la += PUSIZE;
    } while (start != 0 && start < end);
}

void
unmap_range(pgd_t *pgdir, uintptr_t start, uintptr_t end) {
    assert(start % PGSIZE == 0 && end % PGSIZE == 0);
    assert(USER_ACCESS(start, end));
    unmap_range_pgd(pgdir, start, end);
}

static void
exit_range_pmd(pmd_t *pmd) {
    uintptr_t la = 0;
    do {
        pmd_t *pmdp = &pmd[PMX(la)];
        if (ptep_present(pmdp)) {
            free_page(pmd2page(*pmdp)), *pmdp = 0;
        }
        la += PTSIZE;
    } while (la != PMSIZE);
}

static void
exit_range_pud(pud_t *pud) {
    uintptr_t la = 0;
    do {
        pud_t *pudp = &pud[PUX(la)];
        if (ptep_present(pudp)) {
            exit_range_pmd(KADDR(PUD_ADDR(*pudp)));
            free_page(pud2page(*pudp)), *pudp = 0;
        }
        la += PMSIZE;
    } while (la != PUSIZE);
}

static void
exit_range_pgd(pgd_t *pgd, uintptr_t start, uintptr_t end) {
    start = ROUNDDOWN(start, PUSIZE);
    do {
        pgd_t *pgdp = &pgd[PGX(start)];
        if (ptep_present(pgdp)) {
            exit_range_pud(KADDR(PGD_ADDR(*pgdp)));
            free_page(pgd2page(*pgdp)), *pgdp = 0;
        }
        start += PUSIZE;
    } while (start != 0 && start < end);
}

void
exit_range(pgd_t *pgdir, uintptr_t start, uintptr_t end) {
    assert(start % PGSIZE == 0 && end % PGSIZE == 0);
    assert(USER_ACCESS(start, end));
    exit_range_pgd(pgdir, start, end);
}

int
copy_range(pgd_t *to, pgd_t *from, uintptr_t start, uintptr_t end, bool share) {
    assert(start % PGSIZE == 0 && end % PGSIZE == 0);
    assert(USER_ACCESS(start, end));

    do { 
        pte_t *ptep = get_pte(from, start, 0), *nptep;
        if (ptep == NULL) {
            if (get_pud(from, start, 0) == NULL) {
                start = ROUNDDOWN(start + PUSIZE, PUSIZE);
            }
            else if (get_pmd(from, start, 0) == NULL) {
                start = ROUNDDOWN(start + PMSIZE, PMSIZE);
            }
            else {
                start = ROUNDDOWN(start + PTSIZE, PTSIZE);
            }
            continue ;
        }
        if (*ptep != 0) {
            if ((nptep = get_pte(to, start, 1)) == NULL) {
                return -E_NO_MEM;
            }
            int ret;
            assert(*ptep != 0 && *nptep == 0);
            if (ptep_present(ptep)) {
                pte_perm_t perm = ptep_get_perm(ptep, PTE_USER);
                struct Page *page = pte2page(*ptep);
                if (!share && ptep_s_write(ptep)) {
					ptep_unset_s_write(&perm);
					pte_perm_t perm_with_swap_stat = ptep_get_perm(ptep, PTE_SWAP);
					ptep_set_perm(&perm_with_swap_stat, perm);
                    page_insert(from, page, start, perm_with_swap_stat);
                }
                ret = page_insert(to, page, start, perm);
                assert(ret == 0);
            }
            else {
                swap_entry_t entry;
				ptep_copy(&entry, ptep);
                swap_duplicate(entry);
				ptep_copy(nptep, &entry);
            }
        }
        start += PGSIZE;
    } while (start != 0 && start < end);
    return 0;
}
