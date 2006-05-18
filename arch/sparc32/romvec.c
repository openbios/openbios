/*
 * PROM interface support
 * Copyright 1996 The Australian National University.
 * Copyright 1996 Fujitsu Laboratories Limited
 * Copyright 1999 Pete A. Zaitcev
 * This software may be distributed under the terms of the Gnu
 * Public License version 2 or later
 */

#include "openprom.h"
#include "stdint.h"
#include "asm/io.h"
#include "asm/types.h"
#include "libc/vsprintf.h"
#include "openbios/config.h"
#include "openbios/bindings.h"
#include "openbios/kernel.h"
#include "openbios/sysinclude.h"

#ifdef CONFIG_DEBUG_OBP
#define DPRINTF(fmt, args...)                   \
    do { printk(fmt , ##args); } while (0)
#else
#define DPRINTF(fmt, args...)
#endif

#define PAGE_SIZE 4096

static struct linux_mlist_v0 totphys[1];
static struct linux_mlist_v0 totmap[1];
static struct linux_mlist_v0 totavail[1];

static struct linux_mlist_v0 *ptphys;
static struct linux_mlist_v0 *ptmap;
static struct linux_mlist_v0 *ptavail;
static char obp_stdin, obp_stdout;
static int obp_fd_stdin, obp_fd_stdout;

static int obp_nextnode(int node);
static int obp_child(int node);
static int obp_proplen(int node, char *name);
static int obp_getprop(int node, char *name, char *val);
static int obp_setprop(int node, char *name, char *val, int len);
static const char *obp_nextprop(int node, char *name);
static int obp_devread(int dev_desc, char *buf, int nbytes);
static int obp_devseek(int dev_desc, int hi, int lo);

static const struct linux_nodeops nodeops0 = {
    obp_nextnode,	/* int (*no_nextnode)(int node); */
    obp_child,	/* int (*no_child)(int node); */
    obp_proplen,	/* int (*no_proplen)(int node, char *name); */
    obp_getprop,	/* int (*no_getprop)(int node,char *name,char *val); */
    obp_setprop,	/* int (*no_setprop)(int node, char *name,
                           char *val, int len); */
    obp_nextprop	/* char * (*no_nextprop)(int node, char *name); */
};

static struct linux_arguments_v0 obp_arg;
static const struct linux_arguments_v0 * const obp_argp = &obp_arg;

static void (*synch_hook)(void);

static struct linux_romvec romvec0;
static int prop_mem_reg[3];
static int prop_mem_avail[3];
static int prop_vmem_avail[6];

static void doublewalk(unsigned ptab1, unsigned va)
{
}

static int obp_nextnode(int node)
{
    int peer;

    PUSH(node);
    fword("peer");
    peer = POP();
    DPRINTF("obp_nextnode(0x%x) = 0x%x\n", node, peer);

    return peer;
}

static int obp_child(int node)
{
    int child;

    PUSH(node);
    fword("child");
    child = POP();
    DPRINTF("obp_child(0x%x) = 0x%x\n", node, child);

    return child;
}

static int obp_proplen(int node, char *name)
{
    int notfound;

    push_str(name);
    PUSH(node);
    fword("get-package-property");
    notfound = POP();

    if (notfound) {
        DPRINTF("obp_proplen(0x%x, %s) (not found)\n", node, name);

        return -1;
    } else {
        int len;

        len = POP();
        (void) POP();
        DPRINTF("obp_proplen(0x%x, %s) = %d\n", node, name, len);

        return len;
    }
}

static int obp_getprop(int node, char *name, char *value)
{
    int notfound;

    if (!name) {
        // NULL name means get first property
        name = "NULL";
        push_str("");
        PUSH(node);
        fword("next-property");
        notfound = POP();
        notfound = !notfound; // Different return value
    } else {
        push_str(name);
        PUSH(node);
        fword("get-package-property");
        notfound = POP();
    }
    if (notfound) {
        DPRINTF("obp_getprop(0x%x, %s) (not found)\n", node, name);

        return -1;
    } else {
        int len;
        char *str;
            
        len = POP();
        str = (char *) POP();
        memcpy(value, str, len);

        DPRINTF("obp_getprop(0x%x, %s) = %s\n", node, name, str);

        return len;
    }
}

static int obp_setprop(__attribute__((unused)) int node,
		       __attribute__((unused)) char *name,
		       __attribute__((unused)) char *value,
		       __attribute__((unused)) int len)
{
    DPRINTF("obp_setprop(0x%x, %s) = %s (%d)\n", node, name, value, len);

    return -1;
}

static const char *obp_nextprop(int node, char *name)
{
    int found;

    if (!name || *name == '\0') {
        // NULL name means get first property
        push_str("");
        name = "NULL";
    } else {
        push_str(name);
    }
    PUSH(node);
    fword("next-property");
    found = POP();
    if (!found) {
        DPRINTF("obp_nextprop(0x%x, %s) (not found)\n", node, name);
        (void) POP();
        (void) POP();

        return "";
    } else {
        int len;
        char *str;
            
        len = POP();
        str = (char *) POP();

        DPRINTF("obp_nextprop(0x%x, %s) = %s\n", node, name, str);

        return str;
    }
}

static int obp_nbgetchar(void)
{
    return getchar();
}

static int obp_nbputchar(int ch)
{
    putchar(ch);

    return 0;
}

static void obp_reboot(char *str)
{
    printk("rebooting (%s)\n", str);
    outb(0x71f00000, 1);
    for (;;) {}
}

static void obp_abort()
{
    printk("abort, power off\n");
    outb(0x71910000, 1);
    for (;;) {}
}

static void obp_halt()
{
    printk("halt, power off\n");
    outb(0x71910000, 1);
    for (;;) {}
}

static int obp_devopen(char *str)
{
    int ret;

    push_str(str);
    fword("open-dev");
    ret = POP();
    DPRINTF("obp_devopen(%s) = 0x%x\n", str, ret);

    return ret;
}

static int obp_devclose(int dev_desc)
{
    int ret;

    PUSH(dev_desc);
    fword("close-dev");
    ret = POP();

    DPRINTF("obp_devclose(0x%x) = %d\n", dev_desc, ret);

    return ret;
}

static int obp_rdblkdev(int dev_desc, int num_blks, int offset, char *buf)
{
    int ret, hi, lo, bs;

    bs = 512; // XXX: real blocksize?
    hi = ((uint64_t)offset * bs) >> 32;
    lo = ((uint64_t)offset * bs) & 0xffffffff;

    ret = obp_devseek(dev_desc, hi, lo);

    ret = obp_devread(dev_desc, buf, num_blks * bs) / bs;

    DPRINTF("obp_rdblkdev(fd 0x%x, num_blks %d, offset %d (hi %d lo %d), buf 0x%x) = %d\n", dev_desc, num_blks, offset, hi, lo, (int)buf, ret);

    return ret;
}

static char *obp_dumb_mmap(char *va, __attribute__((unused)) int which_io,
			   unsigned int pa, unsigned int size)
{
    unsigned int npages;
    unsigned int off;
    unsigned int mva;

    DPRINTF("obp_dumb_mmap: virta 0x%x, which_io %d, paddr 0x%x, sz %d\n", va, which_io, pa, size);
    off = pa & (PAGE_SIZE-1);
    npages = (off + size + (PAGE_SIZE-1)) / PAGE_SIZE;
    pa &= ~(PAGE_SIZE-1);

    mva = (unsigned int) va;
    while (npages-- != 0) {
        map_page(mva, pa, 1);
        mva += PAGE_SIZE;
        pa += PAGE_SIZE;
    }

    return va;
}

static void obp_dumb_munmap(__attribute__((unused)) char *va,
			    __attribute__((unused)) unsigned int size)
{
    DPRINTF("obp_dumb_munmap: virta 0x%x, sz %d\n", va, size);
}

static int obp_devread(int dev_desc, char *buf, int nbytes)
{
    int ret;

    PUSH((int)buf);
    PUSH(nbytes);
    push_str("read");
    PUSH(dev_desc);
    fword("$call-method");
    ret = POP();

    DPRINTF("obp_devread(fd 0x%x, buf 0x%x, nbytes %d) = %d\n", dev_desc, (int)buf, nbytes, ret);

    return ret;
}

static int obp_devwrite(int dev_desc, char *buf, int nbytes)
{
    int ret;

    PUSH((int)buf);
    PUSH(nbytes);
    push_str("write");
    PUSH(dev_desc);
    fword("$call-method");
    ret = POP();

    DPRINTF("obp_devwrite(fd 0x%x, buf %s, nbytes %d) = %d\n", dev_desc, buf, nbytes, ret);

    return nbytes;
}

static int obp_devseek(int dev_desc, int hi, int lo)
{
    int ret;

    PUSH(lo);
    PUSH(hi);
    push_str("seek");
    PUSH(dev_desc);
    fword("$call-method");
    ret = POP();

    DPRINTF("obp_devseek(fd 0x%x, hi %d, lo %d) = %d\n", dev_desc, hi, lo, ret);

    return ret;
}

static int obp_inst2pkg(int dev_desc)
{
    int ret;

    PUSH(dev_desc);
    fword("ihandle>phandle");
    ret = POP();

    DPRINTF("obp_inst2pkg(fd 0x%x) = 0x%x\n", dev_desc, ret);

    return ret;
}

static int obp_cpustart(unsigned int whichcpu, int ctxtbl_ptr,
                        int thiscontext, char *prog_counter)
{
    //int cpu, found;
    struct linux_prom_registers *smp_ctable = (void *)ctxtbl_ptr;

    DPRINTF("obp_cpustart: cpu %d, ctxptr 0x%x, ctx %d, pc 0x%x\n", whichcpu,
            smp_ctable->phys_addr, thiscontext, prog_counter);
#if 0
    found = obp_getprop(whichcpu, "mid", (char *)&cpu);
    if (found == -1)
        return -1;
    st_bypass(PHYS_JJ_EEPROM + 0x38, (unsigned int)prog_counter);
    st_bypass(PHYS_JJ_EEPROM + 0x3C, ((unsigned int)smp_ctable->phys_addr) >> 4);
    st_bypass(PHYS_JJ_EEPROM + 0x40, thiscontext);
    DPRINTF("obp_cpustart: sending interrupt to CPU %d\n", cpu);
    st_bypass(PHYS_JJ_INTR0 + 0x1000 * cpu + 8, 0x40000000);
#endif

    return 0;
}

static int obp_cpustop(__attribute__((unused)) unsigned int whichcpu)
{
    DPRINTF("obp_cpustop: cpu %d\n", whichcpu);

    return 0;
}

static int obp_cpuidle(__attribute__((unused)) unsigned int whichcpu)
{
    DPRINTF("obp_cpuidle: cpu %d\n", whichcpu);

    return 0;
}

static int obp_cpuresume(__attribute__((unused)) unsigned int whichcpu)
{
    DPRINTF("obp_cpuresume: cpu %d\n", whichcpu);

    return 0;
}

void *
init_openprom(unsigned long memsize, const char *cmdline, char boot_device)
{
    ptphys = totphys;
    ptmap = totmap;
    ptavail = totavail;

    /*
     * Form memory descriptors.
     */
    totphys[0].theres_more = 0;
    totphys[0].start_adr = (char *) 0;
    totphys[0].num_bytes = memsize;

    totavail[0].theres_more = 0;
    totavail[0].start_adr = (char*) 0;
    totavail[0].num_bytes = memsize;

    totmap[0].theres_more = 0;
    totmap[0].start_adr = &_start;
    totmap[0].num_bytes = (unsigned long) &_iomem - (unsigned long) &_start;

    prop_mem_reg[0] = 0;
    prop_mem_reg[1] = 0;
    prop_mem_reg[2] = memsize;
    prop_mem_avail[0] = 0;
    prop_mem_avail[1] = 0;
    prop_mem_avail[2] = va2pa((unsigned long)&_data) - 1;
    prop_vmem_avail[0] = 0;
    prop_vmem_avail[1] = 0;
    prop_vmem_avail[2] = (unsigned long)&_start - 1;
    prop_vmem_avail[3] = 0;
    prop_vmem_avail[4] = 0xffe00000;
    prop_vmem_avail[5] = 0x00200000;

    // Linux wants a R/W romvec table
    romvec0.pv_magic_cookie = LINUX_OPPROM_MAGIC;
    romvec0.pv_romvers = 0;
    romvec0.pv_plugin_revision = 77;
    romvec0.pv_printrev = 0x10203;
    romvec0.pv_v0mem.v0_totphys = &ptphys;
    romvec0.pv_v0mem.v0_prommap = &ptmap;
    romvec0.pv_v0mem.v0_available = &ptavail;
    romvec0.pv_nodeops = &nodeops0;
    romvec0.pv_bootstr = (void *)doublewalk;
    romvec0.pv_v0devops.v0_devopen = &obp_devopen;
    romvec0.pv_v0devops.v0_devclose = &obp_devclose;
    romvec0.pv_v0devops.v0_rdblkdev = &obp_rdblkdev;
    romvec0.pv_stdin = &obp_stdin;
    romvec0.pv_stdout = &obp_stdout;
    romvec0.pv_getchar = obp_nbgetchar;
    romvec0.pv_putchar = (void (*)(int))obp_nbputchar;
    romvec0.pv_nbgetchar = obp_nbgetchar;
    romvec0.pv_nbputchar = obp_nbputchar;
    romvec0.pv_reboot = obp_reboot;
    romvec0.pv_printf = (void (*)(const char *fmt, ...))printk;
    romvec0.pv_abort = obp_abort;
    romvec0.pv_halt = obp_halt;
    romvec0.pv_synchook = &synch_hook;
    romvec0.pv_v0bootargs = &obp_argp;
    romvec0.pv_v2devops.v2_inst2pkg = obp_inst2pkg;
    romvec0.pv_v2devops.v2_dumb_mmap = obp_dumb_mmap;
    romvec0.pv_v2devops.v2_dumb_munmap = obp_dumb_munmap;
    romvec0.pv_v2devops.v2_dev_open = obp_devopen;
    romvec0.pv_v2devops.v2_dev_close = (void (*)(int))obp_devclose;
    romvec0.pv_v2devops.v2_dev_read = obp_devread;
    romvec0.pv_v2devops.v2_dev_write = obp_devwrite;
    romvec0.pv_v2devops.v2_dev_seek = obp_devseek;
    obp_arg.boot_dev_ctrl = 0;
    obp_arg.boot_dev_unit = 0;
    obp_arg.argv[0] = "sd(0,0,0):d";

    switch(boot_device) {
    default:
    case 'a':
        obp_arg.argv[0] = "fd()";
        obp_arg.boot_dev[0] = 'f';
        obp_arg.boot_dev[1] = 'd';
        break;
    case 'd':
        obp_arg.boot_dev_unit = '2';
        obp_arg.argv[0] = "sd(0,2,0):d";
        // Fall through
    case 'c':
        obp_arg.boot_dev[0] = 's';
        obp_arg.boot_dev[1] = 'd';
        break;
    case 'n':
        obp_arg.argv[0] = "le()";
        obp_arg.boot_dev[0] = 'l';
        obp_arg.boot_dev[1] = 'e';
        break;
    }
    obp_arg.argv[1] = cmdline;
    romvec0.pv_v2bootargs.bootpath = &obp_arg.argv[0];
    romvec0.pv_v2bootargs.bootargs = &cmdline;
    romvec0.pv_v2bootargs.fd_stdin = &obp_fd_stdin;
    romvec0.pv_v2bootargs.fd_stdout = &obp_fd_stdout;

    push_str("/builtin/console");
    fword("open-dev");
    obp_fd_stdin = POP();
    push_str("/builtin/console");
    fword("open-dev");
    obp_fd_stdout = POP();
    
    obp_stdin = PROMDEV_TTYA;
    obp_stdout = PROMDEV_TTYA;

    romvec0.v3_cpustart = obp_cpustart;
    romvec0.v3_cpustop = obp_cpustop;
    romvec0.v3_cpuidle = obp_cpuidle;
    romvec0.v3_cpuresume = obp_cpuresume;

    return &romvec0;
}
