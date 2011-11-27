#include <pmm.h>
#include <stdio.h>
#include <mmu.h>
#include <memlayout.h>
#include <error.h>
#include <buddy_pmm.h>
#include <stdio.h>
#include <sync.h>
#include <types.h>
#include <slab.h>
#include <swap.h>
#include <string.h>
#include <kio.h>
#include <glue_mp.h>

PLS static size_t pls_used_pages;
PLS list_entry_t pls_page_struct_free_list;

// virtual address of physicall page array
struct Page *pages;
// amount of physical memory (in pages)
size_t npage = 0;

// virtual address of boot-time page directory
pde_t *boot_pgdir = NULL;

// physical memory management
const struct pmm_manager *pmm_manager;

/**
 * call pmm->init_memmap to build Page struct for free memory  
 */
void
init_memmap(struct Page *base, size_t n) {
	pmm_manager->init_memmap(base, n);
}

/**
 * boot_map_segment - setup&enable the paging mechanism
 * @param la    linear address of this memory need to map (after x86 segment map)
 * @param size  memory size
 * @param pa    physical address of this memory
 * @param perm  permission of this memory
 */
void
boot_map_segment(pde_t *pgdir, uintptr_t la, size_t size, uintptr_t pa, uint32_t perm) {
    assert(PGOFF(la) == PGOFF(pa));
    size_t n = ROUNDUP(size + PGOFF(la), PGSIZE) / PGSIZE;
    la = ROUNDDOWN(la, PGSIZE);
    pa = ROUNDDOWN(pa, PGSIZE);
    for (; n > 0; n --, la += PGSIZE, pa += PGSIZE) {
        pte_t *ptep = get_pte(pgdir, la, 1);
        assert(ptep != NULL);
        *ptep = pa | PTE_P | perm;
    }
}

size_t
nr_used_pages(void)
{
	return pls_read (used_pages);
}

/**************************************************
 * Page allocations.
 **************************************************/

/**
 * call pmm->alloc_pages to allocate a continuing n*PAGESIZE memory
 * @param n pages to be allocated
 */
struct Page *
alloc_pages(size_t n) {
	struct Page *page;
	bool intr_flag;
try_again:
	local_intr_save(intr_flag);
	{
		page = pmm_manager->alloc_pages(n);
	}
	local_intr_restore(intr_flag);
	if (page == NULL && try_free_pages(n)) {
        goto try_again;
    }
	return page;
}


/**
 * boot_alloc_page - allocate one page using pmm->alloc_pages(1) 
 * @return the kernel virtual address of this allocated page
 * note: this function is used to get the memory for PDT(Page Directory Table)&PT(Page Table)
 */
void *
boot_alloc_page(void) {
	struct Page *p = alloc_page();
	if (p == NULL) {
		panic("boot_alloc_page failed.\n");
	}
	return page2kva(p);
}

/**
 * free_pages - call pmm->free_pages to free a continuing n*PAGESIZE memory
 * @param base the first page to be freed
 * @param n number of pages to be freed
 */
void
free_pages(struct Page *base, size_t n) {
	bool intr_flag;
	local_intr_save(intr_flag);
	{
		pmm_manager->free_pages(base, n);
	}
	local_intr_restore(intr_flag);
}

/**
 * nr_free_pages - call pmm->nr_free_pages to get the size (nr*PAGESIZE) of current free memory
 * @return number of free pages
 */
size_t
nr_free_pages(void) {
	size_t ret;
	bool intr_flag;
	local_intr_save(intr_flag);
	{
		ret = pmm_manager->nr_free_pages();
	}
	local_intr_restore(intr_flag);
	return ret;
}

/**************************************************
 * Page table operations
 **************************************************/

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

/**
 * get related Page struct for linear address la using PDT pgdir
 * @param pgdir page directory
 * @param la linear address
 * @param ptep_store table entry stored if not NULL
 * @return @la's corresponding page descriptor
 */
struct Page *
get_page(pde_t *pgdir, uintptr_t la, pte_t **ptep_store) {
	pte_t *ptep = get_pte(pgdir, la, 0);
	if (ptep_store != NULL) {
		*ptep_store = ptep;
	}
	if (ptep != NULL && *ptep & PTE_P) {
		return pa2page(*ptep);
	}
	return NULL;
}

/**
 * page_remove_pte - free an Page sturct which is related linear address la
 *                 - and clean(invalidate) pte which is related linear address la
 * @param pgdir page directory (not used)
 * @param la logical address of the page to be removed
 * @param page table entry of the page to be removed
 * note: PT is changed, so the TLB need to be invalidate 
 */
inline void
page_remove_pte(pde_t *pgdir, uintptr_t la, pte_t *ptep) {
    if (*ptep & PTE_P) {
        struct Page *page = pte2page(*ptep);
		if (!PageSwap (page)) {
			if (page_ref_dec(page) == 0) {
				free_page(page);
			}
		} else {
			if (*ptep & PTE_D) {
                SetPageDirty(page);
            }
            page_ref_dec(page);
		}
        *ptep = 0;
		tlb_invalidate(pgdir, la);
    } else if (*ptep != 0) {
        swap_remove_entry(*ptep);
        *ptep = 0;
    }
}

/**
 * page_insert - build the map of phy addr of an Page with the linear addr @la
 * @param pgdir page directory
 * @param page the page descriptor of the page to be inserted
 * @param la logical address of the page
 * @param perm permission of the page
 * @return 0 on success and error code when failed
 */
int
page_insert(pde_t *pgdir, struct Page *page, uintptr_t la, uint32_t perm) {
	pte_t *ptep = get_pte(pgdir, la, 1);
	if (ptep == NULL) {
		return -E_NO_MEM;
	}

	/* If there's an non-zero entry, free it unless the page is exactly the same. */
	page_ref_inc(page);
	if (*ptep != 0) {
		if ((*ptep & PTE_P) && pte2page(*ptep) == page) {
            page_ref_dec(page);
            goto out;
        }
        page_remove_pte(pgdir, la, ptep);
	}

out:
	*ptep = page2pa(page) | PTE_P | perm;
	tlb_update (pgdir, la);
	
	return 0;
}

/**
 * page_remove - free an Page which is related linear address la and has an validated pte
 * @param pgdir page directory
 * @param la logical address of the page to be removed
 */
void
page_remove(pde_t *pgdir, uintptr_t la) {
	pte_t *ptep = get_pte(pgdir, la, 0);
	if (ptep != NULL) {
		page_remove_pte(pgdir, la, ptep);
	}
}

/**
 * pgdir_alloc_page - call alloc_page & page_insert functions to 
 *                  - allocate a page size memory & setup an addr map
 *                  - pa<->la with linear address la and the PDT pgdir
 * @param pgdir    page directory
 * @param la       logical address for the page to be allocated
 * @param perm     permission of the page
 * @return         the page descriptor of the allocated
 */
struct Page *
pgdir_alloc_page(pde_t *pgdir, uintptr_t la, uint32_t perm) {
    struct Page *page = alloc_page();
    if (page != NULL) {
        if (page_insert(pgdir, page, la, perm) != 0) {
            free_page(page);
            return NULL;
        }
    }
    return page;
}

/**************************************************
 * memory management for users
 **************************************************/
void
unmap_range(pde_t *pgdir, uintptr_t start, uintptr_t end) {
    assert(start % PGSIZE == 0 && end % PGSIZE == 0);
    assert(USER_ACCESS(start, end));

    do {
        pte_t *ptep = get_pte(pgdir, start, 0);
        if (ptep == NULL) {
            start = ROUNDDOWN(start + PTSIZE, PTSIZE);
            continue ;
        }
        if (*ptep != 0) {
            page_remove_pte(pgdir, start, ptep);
        }
        start += PGSIZE;
    } while (start != 0 && start < end);
}

void
exit_range(pde_t *pgdir, uintptr_t start, uintptr_t end) {
    assert(start % PGSIZE == 0 && end % PGSIZE == 0);
    assert(USER_ACCESS(start, end));

    start = ROUNDDOWN(start, PTSIZE);
    do {
        int pde_idx = PDX(start);
        if (pgdir[pde_idx] & PTE_P) {
            free_page(pde2page(pgdir[pde_idx]));
            pgdir[pde_idx] = 0;
        }
        start += PTSIZE;
    } while (start != 0 && start < end);
}

int
copy_range(pde_t *to, pde_t *from, uintptr_t start, uintptr_t end, bool share) {
    assert(start % PGSIZE == 0 && end % PGSIZE == 0);
    assert(USER_ACCESS(start, end));

    do {
        pte_t *ptep = get_pte(from, start, 0), *nptep;
        if (ptep == NULL) {
            start = ROUNDDOWN(start + PTSIZE, PTSIZE);
            continue ;
        }
        if (*ptep != 0) {
            if ((nptep = get_pte(to, start, 1)) == NULL) {
                return -E_NO_MEM;
            }
            int ret;
            assert(*ptep != 0 && *nptep == 0);
            if (*ptep & PTE_P) {
                uint32_t perm = (*ptep & PTE_USER);
				struct Page *page = pte2page(*ptep);
				if (!share && (*ptep & PTE_W)) {
                    perm &= ~PTE_W;
                    page_insert(from, page, start, perm | (*ptep & PTE_SWAP));
                }
                ret = page_insert(to, page, start, perm);
                assert(ret == 0);
            } else {
                swap_entry_t entry = *ptep;
                swap_duplicate(entry);
                *nptep = entry;
            }
        }
        start += PGSIZE;
    } while (start != 0 && start < end);
    return 0;
}

/**************************************************
 * Memory tests
 **************************************************/
/**
 * Check the correctness of the system.
 */
void
check_alloc_page(void) {
	pmm_manager->check();
	
	kprintf("check_alloc_page() succeeded.\n");
}

/**
 * Check page table
 */
void
check_pgdir(void) {
	assert(npage <= KMEMSIZE / PGSIZE);
	assert(boot_pgdir != NULL && (uint32_t)PGOFF(boot_pgdir) == 0);
	assert(get_page(boot_pgdir, TEST_PAGE, NULL) == NULL);

	struct Page *p1, *p2;
	p1 = alloc_page();
	assert(page_insert(boot_pgdir, p1, TEST_PAGE, 0) == 0);

	pte_t *ptep;
	assert((ptep = get_pte(boot_pgdir, TEST_PAGE, 0)) != NULL);
	assert(pa2page(*ptep) == p1);
	assert(page_ref(p1) == 1);

	ptep = &((pte_t *)KADDR(PTE_ADDR(boot_pgdir[PDX(TEST_PAGE)])))[1];
	assert(get_pte(boot_pgdir, TEST_PAGE + PGSIZE, 0) == ptep);

	p2 = alloc_page();
	assert(page_insert(boot_pgdir, p2, TEST_PAGE + PGSIZE, PTE_U | PTE_W) == 0);
	assert((ptep = get_pte(boot_pgdir, TEST_PAGE + PGSIZE, 0)) != NULL);
	assert(*ptep & PTE_U);
	assert(*ptep & PTE_W);
	assert(boot_pgdir[PDX(TEST_PAGE)] & PTE_U);
	assert(page_ref(p2) == 1);

	assert(page_insert(boot_pgdir, p1, TEST_PAGE + PGSIZE, 0) == 0);
	assert(page_ref(p1) == 2);
	assert(page_ref(p2) == 0);
	assert((ptep = get_pte(boot_pgdir, TEST_PAGE + PGSIZE, 0)) != NULL);
	assert(pa2page(*ptep) == p1);
	assert((*ptep & PTE_U) == 0);

	page_remove(boot_pgdir, TEST_PAGE);
	assert(page_ref(p1) == 1);
	assert(page_ref(p2) == 0);

	page_remove(boot_pgdir, TEST_PAGE + PGSIZE);
	assert(page_ref(p1) == 0);
	assert(page_ref(p2) == 0);

	assert(page_ref(pa2page(boot_pgdir[PDX(TEST_PAGE)])) == 1);
	free_page(pa2page(boot_pgdir[PDX(TEST_PAGE)]));
	boot_pgdir[PDX(TEST_PAGE)] = 0;

	kprintf("check_pgdir() succeeded.\n");
}
