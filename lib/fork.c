// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;
    //pde_t *pde = NULL;
    //pte_t *pte = NULL;
    int pn = PGNUM(addr);

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at vpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
    /* lj */
    if(0 == (err & FEC_WR)) {
        panic("page not caused by writing(0x%x 0x%x 0x%x)", err, utf->utf_eip, addr);
    }
    if(0 == (vpt[pn] & PTE_COW)) {
        panic("page not writeable");
    }

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.
	//   No need to explicitly delete the old page's mapping.

	// LAB 4: Your code here.
	if ((r = sys_page_alloc(0, PFTEMP, PTE_P|PTE_U|PTE_W)) < 0)
		panic("sys_page_alloc: %e", r);

	memmove(PFTEMP, ROUNDDOWN(addr, PGSIZE), PGSIZE);

    /* remap original addr */
	if ((r = sys_page_map(0, PFTEMP, 0, ROUNDDOWN(addr, PGSIZE), PTE_P|PTE_U|PTE_W)) < 0)
		panic("sys_page_map: %e", r);
	if ((r = sys_page_unmap(0, PFTEMP)) < 0)
		panic("sys_page_unmap: %e", r);

    //panic("pgfault not implemented");
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	int r;
    int sperm = 0, dperm = 0;

	// LAB 4: Your code here.
    /* lj */
    //pde_t *pde = NULL;
    //pte_t *pte = NULL;

    //pde = &thisenv->env_pgdir[pn / NPDENTRIES];
    //pte = (pte_t *)PTE_ADDR(*pde);
    //pte = &pte[pn % NPTENTRIES];
    //cprintf("duppage 0x%x\n", pn << PGSHIFT);

    dperm = PTE_P | PTE_U;
    //if(*pte & (PTE_W | PTE_COW)) {
    if(vpt[pn] & (PTE_W | PTE_COW)) {
        dperm |= PTE_COW;

        //sperm = *pte & 0x3ff;
        sperm = vpt[pn] & 0x3ff;
        sperm |= PTE_COW;
        sperm &= (0xe0f &~PTE_W);//clear write permission

    }

	if ((r = sys_page_map(0, (void *)(pn << PGSHIFT), envid, (void *)(pn << PGSHIFT), dperm)) < 0)
		panic("sys_page_map: %e", r);

    //if(0xeebfd000 == (pn << PGSHIFT)) {
        //return r;
    //}
    if (sperm && (r = sys_page_map(0, (void *)(pn << PGSHIFT), 0, (void *)(pn << PGSHIFT), sperm)) < 0)
        panic("sys_page_map: %e", r);

    //panic("duppage not implemented");
	return r;
}

static void
duppage2(envid_t dstenv, void *addr)
{
	int r;

	// This is NOT what you should do in your fork.
	if ((r = sys_page_alloc(dstenv, addr, PTE_P|PTE_U|PTE_W)) < 0)
		panic("sys_page_alloc: %e", r);
	if ((r = sys_page_map(dstenv, addr, 0, UTEMP, PTE_P|PTE_U|PTE_W)) < 0)
		panic("sys_page_map: %e", r);
	memmove(UTEMP, addr, PGSIZE);
	if ((r = sys_page_unmap(0, UTEMP)) < 0)
		panic("sys_page_unmap: %e", r);
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use vpd, vpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
extern void _pgfault_upcall(void);
envid_t
fork(void)
{
	// LAB 4: Your code here.
    /* lj */
    envid_t envid = 0;
	extern unsigned char end[];
    volatile pde_t *pde = NULL;
    volatile pte_t *pte = NULL;
    int i = 0, j = 0;
    //int maxpn = UTOP / PGSIZE - 1; //don't copy UXSTACKTOP
    int maxpn = USTACKTOP / PGSIZE - 1; //don't copy [USTACKTOP-PGSIZE,..)
    int pg = 0;
	int r = 0;

    set_pgfault_handler(pgfault);
    if((envid = sys_exofork()) < 0) {
        panic("sys_exofork: %e", envid);
        return envid;
    }
	else if (envid == 0) {
		// child process
		thisenv = &envs[ENVX(sys_getenvid())];
        //NOTICE:must not set_pgfault_handler() in child,
        //and use it in father, or there will be kernel mode
        //page fault
        //set_pgfault_handler(thisenv->env_pgfault_upcall);

		return 0;
	}


    i = 0; 
    j = 0;
	for (i = PDX(UTEXT); i < NPDENTRIES; ++i) {
        //pde = &thisenv->env_pgdir[i];
        pde = &vpd[i];
        if(0 == (*pde & PTE_P)) {
            continue;
        }
        pte = (pte_t *)PTE_ADDR(*pde);
        for (j = 0; j < NPTENTRIES; ++j) {
            pg = i * NPTENTRIES + j;
            if(pg >= maxpn) {
                break;
            }
            if(0 == (vpt[pg] & PTE_P)) {
                //if(0 == (pte[j] & PTE_P)) {
                continue;
            }
            duppage(envid, pg);
        }
    }

    duppage2(envid, (void *)(USTACKTOP - PGSIZE));

    if((r = sys_page_alloc(envid, (void *)(UXSTACKTOP-PGSIZE), PTE_U | PTE_W | PTE_P)) < 0) {
    panic("error sys_page_alloc %e", r);
    }
    else if((r = sys_env_set_pgfault_upcall(envid, _pgfault_upcall)) < 0) {
    panic("error sys_env_set_pgfault_upcall %e ", r);
    }

	// Start the child environment running
	if ((r = sys_env_set_status(envid, ENV_RUNNABLE)) < 0)
		panic("sys_env_set_status: %e", r);

	return envid;
    //panic("fork not implemented");
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
