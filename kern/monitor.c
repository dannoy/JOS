// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line


struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
	{ "backtrace", "Backtrace the runtime environment", mon_backtrace},
};
#define NCOMMANDS (sizeof(commands)/sizeof(commands[0]))

unsigned read_eip();

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < NCOMMANDS; i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char entry[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
		(end-entry+1023)/1024);
	return 0;
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
	// Your code here.
    uint32_t current_bp, bp, ip, arg;
    uint32_t *i = NULL;
    uint32_t current_sp, current_eip;
    struct Eipdebuginfo dbg_info;
    int index = 0;

    current_bp = read_ebp();
    current_sp = read_esp();
    //cprintf("esp 0x%x \n", current_sp);
    debuginfo_eip((uintptr_t )(mon_backtrace), &dbg_info);
    cprintf("#%d ebp 0x%x reip 0x%x %.*s(0x%x,0x%x,0x%x)+0 at %s:%d\n",
                    index++,
                    current_bp, 
                    *((uint32_t *)current_bp + 1),
                    dbg_info.eip_fn_namelen, dbg_info.eip_fn_name,
                    *((uint32_t *)current_bp + 2),
                    *((uint32_t *)current_bp + 3),
                    *((uint32_t *)current_bp + 4),
                    dbg_info.eip_file,
                    dbg_info.eip_line);
    current_eip = *((uint32_t *)current_bp + 1);
    do {
        debuginfo_eip((uintptr_t )(*((uint32_t *)current_bp + 1)), &dbg_info);
        current_bp = *((uint32_t *)current_bp);

        if(!current_bp) {
            cprintf("#%d ebp 0x%08x reip (unknown)\n",
                        index++,
                        current_bp);
            break;
        }

        cprintf("#%d ebp 0x%x reip 0x%x %.*s(",
                        index++,
                        current_bp, 
                        *((uint32_t *)current_bp + 1),
                        dbg_info.eip_fn_namelen, dbg_info.eip_fn_name);

        //for(i = (uint32_t *)current_bp + 2; i < (uint32_t *)bp; ++i) {
        for(i = (uint32_t *)current_bp + 2; i < (uint32_t *)current_bp + 2 + dbg_info.eip_fn_narg; ++i) {
            cprintf("0x%x,", *((uint32_t *)i));
        }
        if(dbg_info.eip_fn_narg) {
            cprintf("\b");
        }
        cprintf(")+%d at %s:%d\n",
                    current_eip - dbg_info.eip_fn_addr,
                    dbg_info.eip_file,
                    dbg_info.eip_line);
        current_eip = *((uint32_t *)current_bp + 1);
    } while(current_bp);

	return 0;
}


/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < NCOMMANDS; i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

void
monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome to the JOS kernel monitor!\n");
	cprintf("Type 'help' for a list of commands.\n");


	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}

// return EIP of caller.
// does not work if inlined.
// putting at the end of the file seems to prevent inlining.
unsigned
read_eip()
{
	uint32_t callerpc;
	__asm __volatile("movl 4(%%ebp), %0" : "=r" (callerpc));
	return callerpc;
}
