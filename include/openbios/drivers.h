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
void macio_nvram_init(char *path, uint32_t addr);
#endif
#ifdef CONFIG_DRIVER_SBUS
int ob_sbus_init(uint64_t base, int machine_id);
void tcx_init(uint64_t base);
void kbd_init(uint64_t base);
int keyboard_dataready(void);
unsigned char keyboard_readdata(void);
#ifdef CONFIG_DEBUG_CONSOLE_VIDEO
void init_video(unsigned long fb,  int width, int height, int depth, int rb);
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
extern uint16_t graphic_depth;
extern volatile unsigned char *power_reg;
extern volatile unsigned int *reset_reg;
extern const char *obp_stdin_path, *obp_stdout_path;
extern char obp_stdin, obp_stdout;
extern volatile struct sun4m_timer_regs *counter_regs;
extern uint32_t kernel_image;
extern uint32_t kernel_size;
extern uint32_t qemu_cmdline;
extern uint32_t cmdline_size;
extern char boot_device;
#endif
#ifdef CONFIG_DRIVER_FLOPPY
int ob_floppy_init(void);
#endif
