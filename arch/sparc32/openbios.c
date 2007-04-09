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
void ob_ide_init(void);
void tcx_init(unsigned long base);
void kbd_init(unsigned long base);

int qemu_machine_type;

struct hwdef {
    unsigned long iommu_high, iommu_base;
    unsigned long slavio_high, slavio_base;
    unsigned long intctl_base, counter_base, nvram_base, ms_kb_base, serial_base;
    unsigned long fd_offset, counter_offset, intr_offset;
    unsigned long dma_base, esp_base, le_base;
    unsigned long tcx_base;
    int machine_id;
};

static const struct hwdef hwdefs[] = {
    /* SS-5 */
    {
        .iommu_high   = 0x0,
        .iommu_base   = 0x10000000,
        .tcx_base     = 0x50000000,
        .slavio_high  = 0x0,
        .slavio_base  = 0x71000000,
        .ms_kb_base   = 0x71000000,
        .serial_base  = 0x71100000,
        .nvram_base   = 0x71200000,
        .fd_offset    = 0x00400000,
        .counter_offset = 0x00d00000,
        .intr_offset  = 0x00e00000,
        .dma_base     = 0x78400000,
        .esp_base     = 0x78800000,
        .le_base      = 0x78c00000,
        .machine_id = 0x80,
    },
    /* SS-10 */
    {
        .iommu_high   = 0xf,
        .iommu_base   = 0xe0000000, // XXX Actually at 0xfe0000000ULL (36 bits)
        .tcx_base     = 0x20000000, // 0xe20000000ULL,
        .slavio_high  = 0xf,
        .slavio_base  = 0xf1000000, // 0xff1000000ULL,
        .ms_kb_base   = 0xf1000000, // 0xff1000000ULL,
        .serial_base  = 0xf1100000, // 0xff1100000ULL,
        .nvram_base   = 0xf1200000, // 0xff1200000ULL,
        .fd_offset    = 0x00700000, // 0xff1700000ULL,
        .counter_offset = 0x00300000, // 0xff1300000ULL,
        .intr_offset  = 0x00400000, // 0xff1400000ULL,
        .dma_base     = 0xf0400000, // 0xef0400000ULL,
        .esp_base     = 0xf0800000, // 0xef0800000ULL,
        .le_base      = 0xf0c00000, // 0xef0c00000ULL,
        .machine_id = 0x72,
    },
};

static const struct hwdef *hwdef;
static unsigned char intdict[256 * 1024];

static void init_memory(void)
{

	/* push start and end of available memory to the stack
	 * so that the forth word QUIT can initialize memory 
	 * management. For now we use hardcoded memory between
	 * 0x10000 and 0x9ffff (576k). If we need more memory
	 * than that we have serious bloat.
	 */

	PUSH((unsigned int)&_heap);
	PUSH((unsigned int)&_eheap);
}

static void
arch_init( void )
{
	void setup_timers(void);

	modules_init();
#ifdef CONFIG_DRIVER_SBUS
        ob_init_mmu(hwdef->iommu_high, hwdef->iommu_base);
	ob_sbus_init(hwdef->iommu_high, hwdef->iommu_base + 0x1000, hwdef->machine_id);

#ifdef CONFIG_DEBUG_CONSOLE_VIDEO
	init_video();
#endif
#endif
#ifdef CONFIG_DRIVER_OBIO
	ob_obio_init(hwdef->slavio_high, hwdef->slavio_base, hwdef->fd_offset,
                     hwdef->counter_offset, hwdef->intr_offset);
        nvram_init();
#endif
	device_end();

	bind_func("platform-boot", boot );
}

int openbios(void)
{
	extern struct sys_info sys_info;
        extern struct mem cmem;
        unsigned int i;

        for (i = 0; i < sizeof(hwdefs) / sizeof(struct hwdef); i++) {
            if (hwdefs[i].machine_id == qemu_machine_type) {
                hwdef = &hwdefs[i];
                break;
            }
        }
        if (!hwdef)
            for(;;); // Internal inconsistency, hang

        mem_init(&cmem, (char *) &_vmem, (char *)&_evmem);
#ifdef CONFIG_DRIVER_SBUS
        init_mmu_swift(hwdef->iommu_base);
#endif
#ifdef CONFIG_DEBUG_CONSOLE
#ifdef CONFIG_DEBUG_CONSOLE_SERIAL
	uart_init(hwdef->serial_base | (CONFIG_SERIAL_PORT? 0: 4),
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

	return 0;
}
