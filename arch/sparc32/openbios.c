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

#define IOMMU_BASE    0x10000000 /* First page of sun4m IOMMU */
#define SBUS_BASE     0x10001000
#define SLAVIO_BASE   0x71000000
#define TCX_BASE      0x50000000

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
        ob_init_mmu(IOMMU_BASE);
	ob_sbus_init(SBUS_BASE);

#ifdef CONFIG_DEBUG_CONSOLE_VIDEO
	init_video();
#endif
#endif
#ifdef CONFIG_DRIVER_OBIO
	ob_obio_init(SLAVIO_BASE);
        nvram_init();
#endif
	device_end();

	bind_func("platform-boot", boot );
}

int openbios(void)
{
	extern struct sys_info sys_info;
        extern struct mem cmem;

        mem_init(&cmem, (char *) &_vmem, (char *)&_evmem);
#ifdef CONFIG_DRIVER_SBUS
        init_mmu_swift(IOMMU_BASE);
#endif
#ifdef CONFIG_DEBUG_CONSOLE
#ifdef CONFIG_DEBUG_CONSOLE_SERIAL
	uart_init(CONFIG_SERIAL_PORT, CONFIG_SERIAL_SPEED);
#endif
#ifdef CONFIG_DEBUG_CONSOLE_VIDEO
	tcx_init(TCX_BASE);
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
