/*
 *  linux/arch/arm/kernel/swp_emulate.c
 *
 *  Copyright (C) 2009 ARM Limited
 *  __user_* functions adapted from include/asm/uaccess.h
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  Implements emulation of the SWP/SWPB instructions using load-exclusive and
 *  store-exclusive for processors that have them disabled (or future ones that
 *  might not implement them).
 *
 *  Syntax of SWP{B} instruction: SWP{B}<c> <Rt>, <Rt2>, [<Rn>]
 *  Where: Rt  = destination
 *	   Rt2 = source
 *	   Rn  = address
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/syscalls.h>
#include <linux/perf_event.h>

#include <asm/traps.h>
#include <asm/uaccess.h>



static unsigned long sdivcounterT;
static unsigned long udivcounterT;
static unsigned long sdivcounterA;
static unsigned long udivcounterA;
static pid_t         previous_pid;

#ifdef CONFIG_PROC_FS
static int proc_read_status(char *page, char **start, off_t off, int count,
			    int *eof, void *data)
{
	char *p = page;
	int len;

	p += sprintf(p, "Emulated SDIV.A:\t\t%lu\n", sdivcounterA);
	p += sprintf(p, "Emulated UDIV.A:\t\t%lu\n", udivcounterA);
	p += sprintf(p, "Emulated SDIV.T:\t\t%lu\n", sdivcounterT);
	p += sprintf(p, "Emulated UDIV.T:\t\t%lu\n", udivcounterT);
	if (previous_pid != 0)
		p += sprintf(p, "Last process:\t\t%d\n", previous_pid);

	len = (p - page) - off;
	if (len < 0)
		len = 0;

	*eof = (len <= count) ? 1 : 0;
	*start = page + off;

	return len;
}
#endif

static inline bool isDivideByZeroTrappingOn(void)
{
	uint32_t controlReg;

	asm("mrc p15, 0, %0, c1, c0, 0":"=r"(controlReg));

	return !!((controlReg >> 19) & 1);
}

#define HANDLER_START(typ, dispnm)									\
		typ n, m, d;										\
		if (current->pid != previous_pid) {							\
			pr_debug("\"%s\" (%ld) uses unsupported " dispnm				\
				" instruction - this will be slow\n",					\
				 current->comm, (unsigned long)current->pid);				\
			previous_pid = current->pid;							\
		}
#define HANDLER_WORK											\
		if (unlikely(!m)) {									\
			if (unlikely(isDivideByZeroTrappingOn()))					\
				return -1;								\
			else										\
				d = 0;									\
		} else											\
			d = n / m;

#define HANDLER_END(nm, t)										\
		regs->ARM_pc += 4;									\
													\
		nm ## counter ## t++;									\
		return 0;										\
	}

#define GETREGS_T											\
		n = regs->uregs[(instr >> 16) & 15];							\
		m = regs->uregs[(instr >>  0) & 15];

#define SAVEREGS_T											\
		regs->uregs[(instr >>  8) & 15] = d;

#define GETREGS_A											\
		n = regs->uregs[(instr >>  0) & 15];							\
		m = regs->uregs[(instr >>  8) & 15];

#define SAVEREGS_A											\
		regs->uregs[(instr >> 16) & 15] = d;


#define GENERATE_HANDLER_T(nm, dispnm, typ)								\
	static int nm##_handler_t(struct pt_regs *regs, unsigned int instr) {				\
		HANDLER_START(typ, dispnm)								\
		GETREGS_T										\
		HANDLER_WORK										\
		SAVEREGS_T										\
		HANDLER_END(nm, T)

#define GENERATE_HANDLER_A(nm, dispnm, typ)								\
	static int nm##_handler_a(struct pt_regs *regs, unsigned int instr) {				\
		HANDLER_START(typ, dispnm)								\
		GETREGS_A										\
		HANDLER_WORK										\
		SAVEREGS_A										\
		HANDLER_END(nm, A)


GENERATE_HANDLER_T(sdiv, "SDIV.T", int32_t)
GENERATE_HANDLER_T(udiv, "UDIV.T", uint32_t)
GENERATE_HANDLER_A(sdiv, "SDIV.A", int32_t)
GENERATE_HANDLER_A(udiv, "UDIV.A", uint32_t)


static struct undef_hook sdiv_hook_a = {
	.instr_mask = 0x0ff0f0f0,
	.instr_val  = 0x0710f010,
	.cpsr_mask  = PSR_T_BIT | PSR_J_BIT,
	.cpsr_val   = 0,
	.fn	    = sdiv_handler_a,
};

static struct undef_hook udiv_hook_a = {
	.instr_mask = 0x0ff0f0f0,
	.instr_val  = 0x0730f010,
	.cpsr_mask  = PSR_T_BIT | PSR_J_BIT,
	.cpsr_val   = 0,
	.fn	    = udiv_handler_a,
};

static struct undef_hook sdiv_hook_t = {
	.instr_mask = 0xfff0f0f0,
	.instr_val  = 0xfb90f0f0,
	.cpsr_mask  = PSR_T_BIT | PSR_J_BIT,
	.cpsr_val   = PSR_T_BIT,
	.fn	    = sdiv_handler_t,
};

static struct undef_hook udiv_hook_t = {
	.instr_mask = 0xfff0f0f0,
	.instr_val  = 0xfbb0f0f0,
	.cpsr_mask  = PSR_T_BIT | PSR_J_BIT,
	.cpsr_val   = PSR_T_BIT,
	.fn	    = udiv_handler_t,
};

/*
 * Register handler and create status file in /proc/cpu
 * Invoked as late_initcall, since not needed before init spawned.
 */
static int __init div_emulation_init(void)
{
#ifdef CONFIG_PROC_FS
	struct proc_dir_entry *res;

	res = create_proc_entry("cpu/div_emulation", S_IRUGO, NULL);

	if (!res)
		return -ENOMEM;

	res->read_proc = proc_read_status;
#endif /* CONFIG_PROC_FS */

	printk(KERN_NOTICE "Registering SDIV/UDIV emulation handler\n");
	register_undef_hook(&sdiv_hook_t);
	register_undef_hook(&udiv_hook_t);
	register_undef_hook(&sdiv_hook_a);
	register_undef_hook(&udiv_hook_a);

	return 0;
}

late_initcall(div_emulation_init);
