/* tag: openbios forth environment, executable code
 *
 * Copyright (C) 2003 Patrick Mauritz, Stefan Reinauer
 *
 * See the file "COPYING" for further information about
 * the copyright and warranty status of this work.
 */

#include "openbios/config.h"
#include "openbios/bindings.h"
#include "openbios/drivers.h"
#include "asm/types.h"
#include "dict.h"
#include "openbios/kernel.h"
#include "openbios/stack.h"
#include "openbios/nvram.h"
#include "sys_info.h"
#include "openbios.h"
#include "libc/byteorder.h"
#define cpu_to_be16(x) __cpu_to_be16(x)
#include "openbios/firmware_abi.h"
#include "boot.h"
#include "../../drivers/timer.h" // XXX
#include "asi.h"
#include "libc/vsprintf.h"

#define REGISTER_NAMED_NODE( name, path )   do {                        \
        bind_new_node( name##_flags_, name##_size_,                     \
                       path, name##_m, sizeof(name##_m)/sizeof(method_t)); \
    } while(0)

#define REGISTER_NODE_METHODS( name, path )   do {                      \
        const char *paths[1];                                           \
                                                                        \
        paths[0] = path;                                                \
        bind_node( name##_flags_, name##_size_,                         \
                   paths, 1, name##_m, sizeof(name##_m)/sizeof(method_t)); \
    } while(0)

static unsigned char intdict[256 * 1024];

// XXX
#define NVRAM_SIZE       0x2000
#define NVRAM_IDPROM     0x1fd0
#define NVRAM_OB_START   (sizeof(ohwcfg_v3_t) + sizeof(struct sparc_arch_cfg))
#define NVRAM_OB_SIZE    ((NVRAM_IDPROM - NVRAM_OB_START) & ~15)

ohwcfg_v3_t nv_info;

#define OBIO_CMDLINE_MAX 256
static char obio_cmdline[OBIO_CMDLINE_MAX];

static uint8_t idprom[32];

struct cpudef {
    unsigned long iu_version;
    const char *name;
};

static void
mmu_open(void)
{
    RET(-1);
}

static void
mmu_close(void)
{
}

/*
  3.6.5 translate
  ( virt -- false | phys.lo ... phys.hi mode true )
*/
static void
mmu_translate(void)
{
    unsigned long virt, phys;

    virt = POP();

    asm("ldxa [%1] %2, %0\n"
        : "=r"(phys) : "r" (virt), "i" (ASI_DTLB_TAG_READ));

    if (phys & 0x8000000000000000) { // Valid entry?
        PUSH(phys >> 32);
        PUSH(phys & 0xffffffff);
        PUSH(0); // XXX
        PUSH(-1);
    } else {
        PUSH(0);
    }
}

/*
  ( index tte_data vaddr -- ? )
*/
static void
dtlb_load(void)
{
    unsigned long vaddr, tte_data, idx;

    vaddr = POP();
    tte_data = POP();
    idx = POP();

    asm("stxa %0, [%1] %2\n"
        "stxa %3, [%%g0] %4\n"
        : : "r" (vaddr), "r" (48), "i" (ASI_DMMU),
          "r" (tte_data), "i" (ASI_DTLB_DATA_IN));
}

/*
  ( index tte_data vaddr -- ? )
*/
static void
itlb_load(void)
{
    unsigned long vaddr, tte_data, idx;

    vaddr = POP();
    tte_data = POP();
    idx = POP();

    asm("stxa %0, [%1] %2\n"
        "stxa %3, [%%g0] %4\n"
        : : "r" (vaddr), "r" (48), "i" (ASI_IMMU),
          "r" (tte_data), "i" (ASI_ITLB_DATA_IN));
}

/*
  3.6.5 map
  ( phys.lo ... phys.hi virt size mode -- )
*/
static void
mmu_map(void)
{
    unsigned long virt, size, mode, phys, tte_data;

    mode = POP();
    size = POP();
    virt = POP();
    phys = POP();
    phys <<= 32;
    phys |= POP();

    tte_data = phys | 0x36;
    switch (size) {
    case 8192:
        break;
    case 65536:
        tte_data |= 2ULL << 60;
        break;
    case 512 * 1024:
        tte_data |= 4ULL << 60;
        break;
    case 4 * 1024 * 1024:
        tte_data |= 6ULL << 60;
        break;
    }
    asm("stxa %0, [%1] %2\n"
        "stxa %3, [%%g0] %4\n"
        : : "r" (virt), "r" (48), "i" (ASI_DMMU),
          "r" (tte_data), "i" (ASI_DTLB_DATA_IN));
    asm("stxa %0, [%1] %2\n"
        "stxa %3, [%%g0] %4\n"
        : : "r" (virt), "r" (48), "i" (ASI_DMMU),
          "r" (tte_data), "i" (ASI_DTLB_DATA_IN));
}

DECLARE_UNNAMED_NODE(mmu, INSTALL_OPEN, 0);

NODE_METHODS(mmu) = {
    { "open",               mmu_open              },
    { "close",              mmu_close             },
    { "translate",          mmu_translate         },
    { "SUNW,dtlb-load",     dtlb_load             },
    { "SUNW,itlb-load",     itlb_load             },
    { "map",                mmu_map               },
};

/*
  ( addr -- ? )
*/
static void
set_trap_table(void)
{
    unsigned long addr;

    addr = POP();
    asm("wrpr %0, %%tba\n"
        : : "r" (addr));
}

static void cpu_generic_init(const struct cpudef *cpu)
{
    unsigned long iu_version;
    char nodebuff[256];

    push_str("/");
    fword("find-device");

    fword("new-device");

    push_str(cpu->name);
    fword("device-name");

    push_str("cpu");
    fword("device-type");

    asm("rdpr %%ver, %0\n"
        : "=r"(iu_version) :);

    PUSH((iu_version >> 48) & 0xff);
    fword("encode-int");
    push_str("manufacturer#");
    fword("property");

    PUSH((iu_version >> 32) & 0xff);
    fword("encode-int");
    push_str("implementation#");
    fword("property");

    PUSH((iu_version >> 24) & 0xff);
    fword("encode-int");
    push_str("mask#");
    fword("property");

    PUSH(9);
    fword("encode-int");
    push_str("sparc-version");
    fword("property");

    fword("finish-device");

    // MMU node
    sprintf(nodebuff, "/%s", cpu->name);
    push_str(nodebuff);
    fword("find-device");

    fword("new-device");

    push_str("mmu");
    fword("device-name");

    fword("finish-device");

    sprintf(nodebuff, "/%s/mmu", cpu->name);

    REGISTER_NODE_METHODS(mmu, nodebuff);

    push_str("/chosen");
    fword("find-device");

    push_str(nodebuff);
    fword("open-dev");
    fword("encode-int");
    push_str("mmu");
    fword("property");

    // Trap table
    push_str("/packages/client-iface");
    fword("find-device");
    bind_func("SUNW,set-trap-table", set_trap_table);
}

static const struct cpudef sparc_defs[] = {
    {
        .iu_version = (0x04ULL << 48) | (0x02ULL << 32),
        .name = "FJSV,GP",
    },
    {
        .iu_version = (0x04ULL << 48) | (0x03ULL << 32),
        .name = "FJSV,GPUSK",
    },
    {
        .iu_version = (0x04ULL << 48) | (0x04ULL << 32),
        .name = "FJSV,GPUSC",
    },
    {
        .iu_version = (0x04ULL << 48) | (0x05ULL << 32),
        .name = "FJSV,GPUZC",
    },
    {
        .iu_version = (0x17ULL << 48) | (0x10ULL << 32),
        .name = "SUNW,UltraSPARC",
    },
    {
        .iu_version = (0x17ULL << 48) | (0x11ULL << 32),
        .name = "SUNW,UltraSPARC-II",
    },
    {
        .iu_version = (0x17ULL << 48) | (0x12ULL << 32),
        .name = "SUNW,UltraSPARC-IIi",
    },
    {
        .iu_version = (0x17ULL << 48) | (0x13ULL << 32),
        .name = "SUNW,UltraSPARC-IIe",
    },
    {
        .iu_version = (0x3eULL << 48) | (0x14ULL << 32),
        .name = "SUNW,UltraSPARC-III",
    },
    {
        .iu_version = (0x3eULL << 48) | (0x15ULL << 32),
        .name = "SUNW,UltraSPARC-III+",
    },
    {
        .iu_version = (0x3eULL << 48) | (0x16ULL << 32),
        .name = "SUNW,UltraSPARC-IIIi",
    },
    {
        .iu_version = (0x3eULL << 48) | (0x18ULL << 32),
        .name = "SUNW,UltraSPARC-IV",
    },
    {
        .iu_version = (0x3eULL << 48) | (0x19ULL << 32),
        .name = "SUNW,UltraSPARC-IV+",
    },
    {
        .iu_version = (0x3eULL << 48) | (0x22ULL << 32),
        .name = "SUNW,UltraSPARC-IIIi+",
    },
    {
        .iu_version = (0x22ULL << 48) | (0x10ULL << 32),
        .name = "SUNW,UltraSPARC",
    },
};

static const struct cpudef *
id_cpu(void)
{
    unsigned long iu_version;
    unsigned int i;

    asm("rdpr %%ver, %0\n"
        : "=r"(iu_version) :);
    iu_version &= 0xffffffff00000000ULL;

    for (i = 0; i < sizeof(sparc_defs)/sizeof(struct cpudef); i++) {
        if (iu_version == sparc_defs[i].iu_version)
            return &sparc_defs[i];
    }
    printk("Unknown cpu (psr %lx), freezing!\n", iu_version);
    for (;;);
}

void arch_nvram_get(char *data)
{
    unsigned short i;
    unsigned char *nvptr = (unsigned char *)&nv_info;
    uint32_t size;
    const struct cpudef *cpu;

    for (i = 0; i < sizeof(ohwcfg_v3_t); i++) {
        outb(i & 0xff, 0x74);
        outb(i  >> 8, 0x75);
        *nvptr++ = inb(0x77);
    }

    printk("Nvram id %s, version %d\n", nv_info.struct_ident,
           nv_info.struct_version);
    if (strcmp(nv_info.struct_ident, "QEMU_BIOS") ||
        nv_info.struct_version != 3 ||
        OHW_compute_crc(&nv_info, 0x00, 0xF8) != nv_info.crc) {
        printk("Unknown nvram, freezing!\n");
        for (;;);
    }

    kernel_image = nv_info.kernel_image;
    kernel_size = nv_info.kernel_size;
    size = nv_info.cmdline_size;
    if (size > OBIO_CMDLINE_MAX - 1)
        size = OBIO_CMDLINE_MAX - 1;
    memcpy(&obio_cmdline, (void *)(long)nv_info.cmdline, size);
    obio_cmdline[size] = '\0';
    qemu_cmdline = (uint64_t)obio_cmdline;
    cmdline_size = size;
    boot_device = nv_info.boot_devices[0];

    printk("kernel addr %lx size %lx\n", kernel_image, kernel_size);
    if (size)
        printk("kernel cmdline %s\n", obio_cmdline);

    for (i = 0; i < NVRAM_OB_SIZE; i++) {
        outb((i + NVRAM_OB_START) & 0xff, 0x74);
        outb((i + NVRAM_OB_START) >> 8, 0x75);
        data[i] = inb(0x77);
    }

    printk("CPUs: %x", nv_info.nb_cpus);
    cpu = id_cpu();
    //cpu->initfn();
    cpu_generic_init(cpu);
    printk(" x %s\n", cpu->name);

    // Add /idprom
    push_str("/");
    fword("find-device");

    for (i = 0; i < 32; i++) {
        outb((i + 0x1fd8) & 0xff, 0x74);
        outb((i + 0x1fd8) >> 8, 0x75);
        idprom[i] = inb(0x77);
    }

    PUSH((long)&idprom);
    PUSH(32);
    fword("encode-bytes");
    push_str("idprom");
    fword("property");

    push_str("/memory");
    fword("find-device");

    PUSH(0);
    fword("encode-int");
    PUSH((int)(nv_info.RAM0_size >> 32));
    fword("encode-int");
    fword("encode+");
    PUSH((int)(nv_info.RAM0_size & 0xffffffff));
    fword("encode-int");
    fword("encode+");
    push_str("reg");
    fword("property");

    PUSH(0);
    fword("encode-int");
    PUSH(0);
    fword("encode-int");
    fword("encode+");
    PUSH((unsigned long)&_start - 4096);
    fword("encode-int");
    fword("encode+");
    PUSH(0);
    fword("encode-int");
    fword("encode+");
    PUSH(va2pa((unsigned long)&_iomem));
    fword("encode-int");
    fword("encode+");
    PUSH(-va2pa((unsigned long)&_iomem));
    fword("encode-int");
    fword("encode+");
    push_str("available");
    fword("property");

    // XXX
    push_str("/virtual-memory");
    fword("find-device");

    PUSH(0);
    fword("encode-int");
    PUSH(0);
    fword("encode-int");
    fword("encode+");
    PUSH(0);
    fword("encode-int");
    fword("encode+");
    PUSH(0);
    fword("encode-int");
    fword("encode+");
    PUSH(0);
    fword("encode-int");
    fword("encode+");
    PUSH(0);
    fword("encode-int");
    fword("encode+");
    push_str("translations");
    fword("property");
}

void arch_nvram_put(char *data)
{
    unsigned short i;

    for (i = 0; i < NVRAM_OB_SIZE; i++) {
        outb((i + NVRAM_OB_START) & 0xff, 0x74);
        outb((i + NVRAM_OB_START) >> 8, 0x75);
        outb(data[i], 0x77);
    }
}

int arch_nvram_size(void)
{
    return NVRAM_OB_SIZE;
}

void setup_timers(void)
{
}

void udelay(unsigned int usecs)
{
}

static void init_memory(void)
{

	/* push start and end of available memory to the stack
	 * so that the forth word QUIT can initialize memory
	 * management. For now we use hardcoded memory between
	 * 0x10000 and 0x9ffff (576k). If we need more memory
	 * than that we have serious bloat.
	 */

	PUSH((ucell)&_heap);
	PUSH((ucell)&_eheap);
}

static void
arch_init( void )
{
	modules_init();
#ifdef CONFIG_DRIVER_PCI
	ob_pci_init();
#endif
#ifdef CONFIG_DRIVER_IDE
	setup_timers();
	ob_ide_init();
#endif
#ifdef CONFIG_DRIVER_FLOPPY
	ob_floppy_init();
#endif
#ifdef CONFIG_DEBUG_CONSOLE_VIDEO
	init_video();
#endif

        nvram_init();
        ob_su_init(0x1fe02000000ULL, 0x3f8ULL, 0);

        device_end();

	bind_func("platform-boot", boot );
}

int openbios(void)
{
#ifdef CONFIG_DEBUG_CONSOLE
#ifdef CONFIG_DEBUG_CONSOLE_SERIAL
	uart_init(CONFIG_SERIAL_PORT, CONFIG_SERIAL_SPEED);
#endif
#ifdef CONFIG_DEBUG_CONSOLE_VGA
	video_init();
#endif
	/* Clear the screen.  */
	cls();
        printk("OpenBIOS for Sparc64\n");
#endif

        collect_sys_info(&sys_info);
	
	dict=intdict;
	load_dictionary((char *)sys_info.dict_start,
			(unsigned long)sys_info.dict_end
                        - (unsigned long)sys_info.dict_start);
	
#ifdef CONFIG_DEBUG_BOOT
	printk("forth started.\n");
	printk("initializing memory...");
#endif

	init_memory();

#ifdef CONFIG_DEBUG_BOOT
	printk("done\n");
#endif

	PUSH_xt( bind_noname_func(arch_init) );
	fword("PREPOST-initializer");
	
	PC = (ucell)findword("initialize-of");

	if (!PC) {
		printk("panic: no dictionary entry point.\n");
		return -1;
	}
#ifdef CONFIG_DEBUG_DICTIONARY
	printk("done (%d bytes).\n", dicthead);
	printk("Jumping to dictionary...\n");
#endif

	enterforth((xt_t)PC);
        arch_init(); // XXX
        printk("force boot\n");
        push_str("/pci/isa/ide0/disk@0,0:a");
        boot(); // XXX
        printk("falling off...\n");
	return 0;
}
