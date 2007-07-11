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
#include "sys_info.h"
#include "openbios.h"

void boot(void);

static unsigned char intdict[256 * 1024];

// XXX
#define NVRAM_SIZE       0x2000
#define NVRAM_IDPROM     0x1fd0
#define NVRAM_OB_OFFSET  256
#define NVRAM_OB_SIZE    ((NVRAM_IDPROM - NVRAM_OB_OFFSET) & ~15)

static struct qemu_nvram_v1 {
    char id_string[16];
    uint32_t version;
    uint32_t nvram_size; // not used in Sun4m
    char unused1[8];
    char arch[12];
    char curr_cpu;
    char smp_cpus;
    char unused2;
    char nographic;
    uint32_t ram_size;
    char boot_device;
    char unused3[3];
    uint32_t kernel_image;
    uint32_t kernel_size;
    uint32_t cmdline;
    uint32_t cmdline_size;
    uint32_t initrd_image;
    uint32_t initrd_size;
    uint32_t nvram_image;
    uint16_t width;
    uint16_t height;
    uint16_t depth;
    char unused4[158];
    uint16_t crc;
} nv_info;

#define OBIO_CMDLINE_MAX 256
static char obio_cmdline[OBIO_CMDLINE_MAX];

void arch_nvram_get(char *data)
{
    unsigned short i;
    unsigned char *nvptr = &nv_info;
    uint32_t size;
    extern uint32_t kernel_image;
    extern uint32_t kernel_size;
    extern uint32_t cmdline;
    extern uint32_t cmdline_size;
    extern char boot_device;

    for (i = 0; i < sizeof(struct qemu_nvram_v1); i++) {
        outb(i & 0xff, 0x74);
        outb(i  >> 8, 0x75);
        *nvptr++ = inb(0x77);
    }

    kernel_image = nv_info.kernel_image;
    kernel_size = nv_info.kernel_size;
    size = nv_info.cmdline_size;
    if (size > OBIO_CMDLINE_MAX - 1)
        size = OBIO_CMDLINE_MAX - 1;
    memcpy(obio_cmdline, nv_info.cmdline, size);
    obio_cmdline[size] = '\0';
    cmdline = obio_cmdline;
    cmdline_size = size;
    printk("kernel addr %x size %x\n", kernel_image, kernel_size);
    if (size)
        printk("kernel cmdline %s\n", obio_cmdline);

    for (i = 0; i < NVRAM_OB_SIZE; i++) {
        outb((i + NVRAM_OB_OFFSET) & 0xff, 0x74);
        outb((i + NVRAM_OB_OFFSET) >> 8, 0x75);
        data[i] = inb(0x77);
    }
}

void arch_nvram_put(char *data)
{
    unsigned short i;

    for (i = 0; i < NVRAM_OB_SIZE; i++) {
        outb((i + NVRAM_OB_OFFSET) & 0xff, 0x74);
        outb((i + NVRAM_OB_OFFSET) >> 8, 0x75);
        outb(data[i], 0x77);
    }
}

int arch_nvram_size()
{
    return NVRAM_OB_SIZE;
}

void setup_timers()
{
}

void udelay()
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
	void setup_timers(void);

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
	device_end();

	bind_func("platform-boot", boot );
}

int openbios(void)
{
	extern struct sys_info sys_info;

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
