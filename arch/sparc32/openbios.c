/* tag: openbios forth environment, executable code
 *
 * Copyright (C) 2003 Patrick Mauritz, Stefan Reinauer
 *
 * See the file "COPYING" for further information about
 * the copyright and warranty status of this work.
 */

#include "openbios/config.h"
#include "libopenbios/bindings.h"
#include "drivers/drivers.h"
#include "asm/types.h"
#include "dict.h"
#include "kernel/kernel.h"
#include "kernel/stack.h"
#include "openbios/nvram.h"
#include "../../drivers/timer.h" // XXX
#include "sys_info.h"
#include "openbios.h"
#include "boot.h"
#include "video_subr.h"

#define MEMORY_SIZE     (128*1024)       /* 16K ram for hosted system */
#define DICTIONARY_SIZE (256*1024)      /* 256K for the dictionary   */

static ucell *memory;

int qemu_machine_type;

struct hwdef {
    uint64_t iommu_base, slavio_base;
    uint64_t intctl_base, counter_base, nvram_base, ms_kb_base, serial_base;
    unsigned long fd_offset, counter_offset, intr_offset;
    unsigned long aux1_offset, aux2_offset;
    uint64_t dma_base, esp_base, le_base;
    uint64_t tcx_base;
    int machine_id_low, machine_id_high;
};

static const struct hwdef hwdefs[] = {
    /* SS-5 */
    {
        .iommu_base   = 0x10000000,
        .tcx_base     = 0x50000000,
        .slavio_base  = 0x71000000,
        .ms_kb_base   = 0x71000000,
        .serial_base  = 0x71100000,
        .nvram_base   = 0x71200000,
        .fd_offset    = 0x00400000,
        .counter_offset = 0x00d00000,
        .intr_offset  = 0x00e00000,
        .aux1_offset  = 0x00900000,
        .aux2_offset  = 0x00910000,
        .dma_base     = 0x78400000,
        .esp_base     = 0x78800000,
        .le_base      = 0x78c00000,
        .machine_id_low = 32,
        .machine_id_high = 63,
    },
    /* SS-10 */
    {
        .iommu_base   = 0xfe0000000ULL,
        .tcx_base     = 0xe20000000ULL,
        .slavio_base  = 0xff1000000ULL,
        .ms_kb_base   = 0xff1000000ULL,
        .serial_base  = 0xff1100000ULL,
        .nvram_base   = 0xff1200000ULL,
        .fd_offset    = 0x00700000, // 0xff1700000ULL,
        .counter_offset = 0x00300000, // 0xff1300000ULL,
        .intr_offset  = 0x00400000, // 0xff1400000ULL,
        .aux1_offset  = 0x00800000, // 0xff1800000ULL,
        .aux2_offset  = 0x00a01000, // 0xff1a01000ULL,
        .dma_base     = 0xef0400000ULL,
        .esp_base     = 0xef0800000ULL,
        .le_base      = 0xef0c00000ULL,
        .machine_id_low = 64,
        .machine_id_high = 65,
    },
    /* SS-600MP */
    {
        .iommu_base   = 0xfe0000000ULL,
        .tcx_base     = 0xe20000000ULL,
        .slavio_base  = 0xff1000000ULL,
        .ms_kb_base   = 0xff1000000ULL,
        .serial_base  = 0xff1100000ULL,
        .nvram_base   = 0xff1200000ULL,
        .fd_offset    = -1,
        .counter_offset = 0x00300000, // 0xff1300000ULL,
        .intr_offset  = 0x00400000, // 0xff1400000ULL,
        .aux1_offset  = 0x00800000, // 0xff1800000ULL,
        .aux2_offset  = 0x00a01000, // 0xff1a01000ULL, XXX should not exist
        .dma_base     = 0xef0081000ULL,
        .esp_base     = 0xef0080000ULL,
        .le_base      = 0xef0060000ULL,
        .machine_id_low = 66,
        .machine_id_high = 66,
    },
};

static const struct hwdef *hwdef;

void setup_timers(void)
{
}

void udelay(unsigned int usecs)
{
}

void mdelay(unsigned int msecs)
{
}

static void init_memory(void)
{
    memory = malloc(MEMORY_SIZE);
    if (!memory)
        printk("panic: not enough memory on host system.\n");

    /* we push start and end of memory to the stack
     * so that it can be used by the forth word QUIT
     * to initialize the memory allocator
     */

    PUSH((ucell)memory);
    PUSH((ucell)memory + MEMORY_SIZE);
}

static void
arch_init( void )
{
	modules_init();
        ob_init_mmu();
        ob_init_iommu(hwdef->iommu_base);
#ifdef CONFIG_DRIVER_OBIO
	ob_obio_init(hwdef->slavio_base, hwdef->fd_offset,
                     hwdef->counter_offset, hwdef->intr_offset,
                     hwdef->aux1_offset, hwdef->aux2_offset);
        nvconf_init();
#endif
#ifdef CONFIG_DRIVER_SBUS
#ifdef CONFIG_DEBUG_CONSOLE_VIDEO
	init_video((unsigned long)vmem, 1024, 768, 8, 1024);
#endif
	ob_sbus_init(hwdef->iommu_base + 0x1000ULL, qemu_machine_type);
#endif
	device_end();

	bind_func("platform-boot", boot );
}

int openbios(void)
{
        unsigned int i;

        for (i = 0; i < sizeof(hwdefs) / sizeof(struct hwdef); i++) {
            if (hwdefs[i].machine_id_low <= qemu_machine_type &&
                hwdefs[i].machine_id_high >= qemu_machine_type) {
                hwdef = &hwdefs[i];
                break;
            }
        }
        if (!hwdef)
            for(;;); // Internal inconsistency, hang

#ifdef CONFIG_DRIVER_SBUS
        init_mmu_swift();
#endif
#ifdef CONFIG_DEBUG_CONSOLE
#ifdef CONFIG_DEBUG_CONSOLE_SERIAL
	uart_init(hwdef->serial_base | (CONFIG_SERIAL_PORT? 0ULL: 4ULL),
                  CONFIG_SERIAL_SPEED);
#endif
#ifdef CONFIG_DEBUG_CONSOLE_VIDEO
	tcx_init(hwdef->tcx_base);
	kbd_init(hwdef->ms_kb_base);
#endif
	/* Clear the screen.  */
	cls();
#endif

        collect_sys_info(&sys_info);

        dict = malloc(DICTIONARY_SIZE);
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

        free(dict);
	return 0;
}
