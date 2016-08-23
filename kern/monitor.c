// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/dwarf.h>
#include <kern/kdebug.h>
#include <kern/dwarf_api.h>
#include <kern/trap.h>

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
	{ "backtrace", "Display information about the kernel", mon_backtrace }
};
#define NCOMMANDS (sizeof(commands)/sizeof(commands[0]))

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
	extern char _start[], entry[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  _start                  %08x (phys)\n", _start);
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
		ROUNDUP(end - entry, 1024) / 1024);
	return 0;
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
	cprintf("Stack backtrace:\n");
	uint64_t *addr = (uint64_t*)read_rbp(), *addr_rsp = (uint64_t*)read_rsp();
//	cprintf("%016x %016x\n", addr, addr_rsp);
	uint32_t i;
	uint64_t temp;
	read_rip(temp);

	for(i = 0; addr; i++) {

//		cprintf("%016x %016x\n", addr, addr_rsp) ;
		struct Ripdebuginfo *t_debug_info = NULL;
		debuginfo_rip(temp, t_debug_info);

		char t[100];
		int j = 0;
		for(; j < t_debug_info->rip_fn_namelen; j++)
			t[j] = t_debug_info->rip_fn_name[j];
		t[t_debug_info->rip_fn_namelen] = 0;


	 	cprintf("  rbp %016x  rip  %016x", addr, temp);
		cprintf("  %s:%d: %s+%016x  args:%d ", t_debug_info->rip_file, t_debug_info->rip_line, t, -t_debug_info->rip_fn_addr + temp, t_debug_info->rip_fn_narg);
		temp = *(addr+1);


		int sum[5], bef[5];
		memset(sum, 0, sizeof(sum));
		memset(bef, 0, sizeof(bef));
		for(j = 0; j < t_debug_info->rip_fn_narg; j++) {
			int temp = t_debug_info->size_fn_arg[j];
			int rcate = 0;
			while(temp) rcate++, temp /= 2;
			rcate--;

			sum[rcate]++;

		}

		for(j = 1; j <= 4; j++) {
			bef[j] = bef[j - 1] + sum[j - 1] * (1ll << (j - 1));
			if(bef[j] != 0) {
				bef[j] = (bef[j] - 1) / (1ll << j) + 1;
				bef[j] <<= j;
			}
		}

	//	cprintf("\n");
	//	for(j = 0; j < 5; j++) {
	//		cprintf("%d|%d ", sum[j], bef[j]);
//		}
//		cprintf("\n");

		int rsum = bef[4];
		memset(sum, 0, sizeof(sum));
		for(j = 0; j < t_debug_info->rip_fn_narg; j++) {
			if(t_debug_info->size_fn_arg[j] == 1) {
				sum[0]++;
	 			cprintf(" %016x", *((uint8_t*)addr_rsp+ rsum-bef[0]-sum[0]));
			} else 	if(t_debug_info->size_fn_arg[j] == 2) {
				sum[1]++;
	 			cprintf(" %016x", *(uint16_t*)((uint8_t*)addr_rsp+rsum-bef[1]-sum[1]*2));
			} else if(t_debug_info->size_fn_arg[j] == 4) {
				sum[2]++;
	 			cprintf(" %016x", *(uint32_t*)((uint8_t*)addr_rsp+rsum-bef[2]-sum[2]*4));
			} else if(t_debug_info->size_fn_arg[j] == 8) {
				sum[3]++;
	 			cprintf(" %016x", *(uint64_t*)((uint8_t*)addr_rsp+rsum-bef[3]-sum[3]*8));
			} 
	    } 
		addr_rsp = addr + 2;
    	addr = (uint64_t*)(*addr);

		cprintf("\n");


	}
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

	if (tf != NULL)
		print_trapframe(tf);

	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}
