/*
 * PROM interface support
 * Copyright 1996 The Australian National University.
 * Copyright 1996 Fujitsu Laboratories Limited
 * Copyright 1999 Pete A. Zaitcev
 * This software may be distributed under the terms of the Gnu
 * Public License version 2 or later
 */

#include "openprom.h"
#include "config.h"
#include "libopenbios/bindings.h"
#include "drivers/drivers.h"
#include "libopenbios/sys_info.h"
#include "boot.h"

#ifdef CONFIG_DEBUG_OBP
#define DPRINTF(fmt, args...)                   \
    do { printk(fmt , ##args); } while (0)
#else
#define DPRINTF(fmt, args...)
#endif

char obp_stdin, obp_stdout;
static int obp_fd_stdin, obp_fd_stdout;
const char *obp_stdin_path, *obp_stdout_path;

static int obp_nextnode(int node);
static int obp_child(int node);
static int obp_proplen(int node, const char *name);
static int obp_getprop(int node, const char *name, char *val);
static int obp_setprop(int node, const char *name, char *val, int len);
static const char *obp_nextprop(int node, const char *name);
static int obp_devread(int dev_desc, char *buf, int nbytes);
static int obp_devseek(int dev_desc, int hi, int lo);

struct linux_arguments_v0 obp_arg;
const char *bootpath;
static const struct linux_arguments_v0 * const obp_argp = &obp_arg;

static void (*sync_hook)(void);

static struct linux_romvec romvec0;

static void doublewalk(__attribute__((unused)) unsigned int ptab1,
                       __attribute__((unused)) unsigned int va)
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

static int obp_proplen(int node, const char *name)
{
    int notfound;

    if (!node) {
        DPRINTF("obp_proplen(0x0, %s) = -1\n", name);
        return -1;
    }

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

#ifdef CONFIG_DEBUG_OBP
static int looks_like_string(const char *str, int len)
{
    int i;
    int ret = (str[len-1] == '\0');
    for (i = 0; i < len-1 && ret; i++)
    {
        int ch = str[i] & 0xFF;
        if (ch < 0x20 || ch > 0x7F)
            ret = 0;
    }
    return ret;
}
#endif

static int obp_getprop(int node, const char *name, char *value)
{
    int notfound, found;
    int len;
    const char *str;

    if (!node) {
        DPRINTF("obp_getprop(0x0, %s) = -1\n", name);
        return -1;
    }

    if (!name) {
        // NULL name means get first property
        push_str("");
        PUSH(node);
        fword("next-property");
        found = POP();
        if (found) {
            len = POP();
            str = (char *) POP();
            DPRINTF("obp_getprop(0x%x, NULL) = %s\n", node, str);

            return (int)str;
        }
        DPRINTF("obp_getprop(0x%x, NULL) (not found)\n", node);

        return -1;
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
        len = POP();
        str = (char *) POP();
        if (len > 0)
            memcpy(value, str, len);
        else
            str = "NULL";

#ifdef CONFIG_DEBUG_OBP
        if (looks_like_string(str, len)) {
            DPRINTF("obp_getprop(0x%x, %s) = %s\n", node, name, str);
        } else {
            int i;
            DPRINTF("obp_getprop(0x%x, %s) = ", node, name);
            for (i = 0; i < len; i++) {
                DPRINTF("%02x%s", str[i] & 0xFF,
                        (len == 4 || i == len-1) ? "" : " ");
            }
            DPRINTF("\n");
        }
#endif

        return len;
    }
}

static const char *obp_nextprop(int node, const char *name)
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

        return "";
    } else {
        char *str;

        POP(); /* len */
        str = (char *) POP();

        DPRINTF("obp_nextprop(0x%x, %s) = %s\n", node, name, str);

        return str;
    }
}

static int obp_setprop(__attribute__((unused)) int node,
                       __attribute__((unused)) const char *name,
		       __attribute__((unused)) char *value,
		       __attribute__((unused)) int len)
{
    DPRINTF("obp_setprop(0x%x, %s) = %s (%d)\n", node, name, value, len);

    return -1;
}

static const struct linux_nodeops nodeops0 = {
    obp_nextnode,	/* int (*no_nextnode)(int node); */
    obp_child,	        /* int (*no_child)(int node); */
    obp_proplen,	/* int (*no_proplen)(int node, char *name); */
    obp_getprop,	/* int (*no_getprop)(int node,char *name,char *val); */
    obp_setprop,	/* int (*no_setprop)(int node, char *name,
                           char *val, int len); */
    obp_nextprop	/* char * (*no_nextprop)(int node, char *name); */
};

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
    *reset_reg = 1;
    printk("reboot failed\n");
    for (;;) {}
}

static void obp_abort(void)
{
    printk("abort, power off\n");
    *power_reg = 1;
    printk("power off failed\n");
    for (;;) {}
}

static void obp_halt(void)
{
    printk("halt, power off\n");
    *power_reg = 1;
    printk("power off failed\n");
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
    int ret = 1;

    PUSH(dev_desc);
    fword("close-dev");

    DPRINTF("obp_devclose(0x%x) = %d\n", dev_desc, ret);

    return ret;
}

static int obp_rdblkdev(int dev_desc, int num_blks, int offset, char *buf)
{
    int ret, hi, lo, bs;

    bs = 512;
    hi = ((uint64_t)offset * bs) >> 32;
    lo = ((uint64_t)offset * bs) & 0xffffffff;

    ret = obp_devseek(dev_desc, hi, lo);

    ret = obp_devread(dev_desc, buf, num_blks * bs) / bs;

    DPRINTF("obp_rdblkdev(fd 0x%x, num_blks %d, offset %d (hi %d lo %d), buf 0x%x) = %d\n", dev_desc, num_blks, offset, hi, lo, (int)buf, ret);

    return ret;
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

    //DPRINTF("obp_devwrite(fd 0x%x, buf %s, nbytes %d) = %d\n", dev_desc, buf, nbytes, ret);

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
    fword("ihandle>non-interposed-phandle");
    ret = POP();

    DPRINTF("obp_inst2pkg(fd 0x%x) = 0x%x\n", dev_desc, ret);

    return ret;
}

static int obp_cpustart(__attribute__((unused))unsigned int whichcpu,
                        __attribute__((unused))int ctxtbl_ptr,
                        __attribute__((unused))int thiscontext,
                        __attribute__((unused))char *prog_counter)
{
    int cpu, found;
    struct linux_prom_registers *smp_ctable = (void *)ctxtbl_ptr;

    DPRINTF("obp_cpustart: cpu %d, ctxptr 0x%x, ctx %d, pc 0x%x\n", whichcpu,
            smp_ctable->phys_addr, thiscontext, (unsigned int)prog_counter);

    found = obp_getprop(whichcpu, "mid", (char *)&cpu);
    if (found == -1)
        return -1;
    DPRINTF("cpu found, id %d -> cpu %d\n", whichcpu, cpu);

    return start_cpu((unsigned int)prog_counter, ((unsigned int)smp_ctable->phys_addr) >> 4,
              thiscontext, cpu);
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

static void obp_fortheval_v2(char *str)
{
  // for now, move something to the stack so we
  // don't get a stack underrun.
  //
  // FIXME: find out why solaris doesnt put its stuff on the stack
  //
  fword("0");
  fword("0");

  DPRINTF("obp_fortheval_v2(%s)\n", str);
  push_str(str);
  fword("eval");
}

void *
init_openprom(void)
{
    // Linux wants a R/W romvec table
    romvec0.pv_magic_cookie = LINUX_OPPROM_MAGIC;
    romvec0.pv_romvers = 3;
    romvec0.pv_plugin_revision = 2;
    romvec0.pv_printrev = 0x20019;
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
    romvec0.pv_synchook = &sync_hook;
    romvec0.pv_v0bootargs = &obp_argp;
    romvec0.pv_fortheval.v2_eval = obp_fortheval_v2;
    romvec0.pv_v2devops.v2_inst2pkg = obp_inst2pkg;
    romvec0.pv_v2devops.v2_dumb_mem_alloc = obp_dumb_memalloc;
    romvec0.pv_v2devops.v2_dumb_mem_free = obp_dumb_memfree;
    romvec0.pv_v2devops.v2_dumb_mmap = obp_dumb_mmap;
    romvec0.pv_v2devops.v2_dumb_munmap = obp_dumb_munmap;
    romvec0.pv_v2devops.v2_dev_open = obp_devopen;
    romvec0.pv_v2devops.v2_dev_close = (void (*)(int))obp_devclose;
    romvec0.pv_v2devops.v2_dev_read = obp_devread;
    romvec0.pv_v2devops.v2_dev_write = obp_devwrite;
    romvec0.pv_v2devops.v2_dev_seek = obp_devseek;

    romvec0.pv_v2bootargs.bootpath = &bootpath;

    romvec0.pv_v2bootargs.bootargs = &obp_arg.argv[1];
    romvec0.pv_v2bootargs.fd_stdin = &obp_fd_stdin;
    romvec0.pv_v2bootargs.fd_stdout = &obp_fd_stdout;

    push_str(obp_stdin_path);
    fword("open-dev");
    obp_fd_stdin = POP();
    push_str(obp_stdout_path);
    fword("open-dev");
    obp_fd_stdout = POP();

    romvec0.v3_cpustart = obp_cpustart;
    romvec0.v3_cpustop = obp_cpustop;
    romvec0.v3_cpuidle = obp_cpuidle;
    romvec0.v3_cpuresume = obp_cpuresume;

    return &romvec0;
}
