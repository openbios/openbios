/*
 *   OpenBIOS driver prototypes
 *   
 *   (C) 2004 Stefan Reinauer <stepan@openbios.org>
 * 
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   version 2
 *
 */

#include "openbios/config.h"

#ifdef CONFIG_DRIVER_PCI
int ob_pci_init(void);
#endif
#ifdef CONFIG_DRIVER_SBUS
int ob_sbus_init(uint64_t base, int machine_id);
void tcx_init(uint64_t base);
void kbd_init(uint64_t base);
int keyboard_dataready(void);
unsigned char keyboard_readdata(void);
#ifdef CONFIG_DEBUG_CONSOLE_VIDEO
void init_video(void);
#endif
#endif
#ifdef CONFIG_DRIVER_IDE
int ob_ide_init(void);
#endif
#ifdef CONFIG_DRIVER_ESP
int ob_esp_init(unsigned int slot, uint64_t base, unsigned long espoffset,
                unsigned long dmaoffset);
#endif
#ifdef CONFIG_DRIVER_OBIO
int ob_obio_init(uint64_t slavio_base, unsigned long fd_offset,
                 unsigned long counter_offset, unsigned long intr_offset,
                 unsigned long aux1_offset, unsigned long aux2_offset);
int start_cpu(unsigned int pc, unsigned int context_ptr, unsigned int context,
              int cpu);
extern struct mem cmem;
#endif
#ifdef CONFIG_DRIVER_FLOPPY
int ob_floppy_init(void);
#endif
