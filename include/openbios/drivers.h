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
#ifndef OPENBIOS_DRIVERS_H
#define OPENBIOS_DRIVERS_H

#include "openbios/config.h"

/* modules/video.c */
int video_get_res(int *w, int *h);
void draw_pixel(int x, int y, int colind);
void set_color(int ind, ulong color);
void video_scroll(int height);
void init_video(unsigned long fb, int width, int height, int depth, int rb);

/* modules/console.c */
int console_draw_str(const char *str);
void console_close(void);
void cls(void);

#ifdef CONFIG_DRIVER_PCI
/* drivers/pci.c */
int ob_pci_init(void);

/* arch/ppc/qemu/qemu.c */
void macio_nvram_init(const char *path, uint32_t addr);
#endif
#ifdef CONFIG_DRIVER_SBUS
/* drivers/sbus.c */
int ob_sbus_init(uint64_t base, int machine_id);

/* arch/sparc32/console.c */
void tcx_init(uint64_t base);
void kbd_init(uint64_t base);
int keyboard_dataready(void);
unsigned char keyboard_readdata(void);
#endif
#ifdef CONFIG_DRIVER_IDE
/* drivers/ide.c */
int ob_ide_init(const char *path, uint32_t io_port0, uint32_t ctl_port0,
                uint32_t io_port1, uint32_t ctl_port1);
#endif
#ifdef CONFIG_DRIVER_ESP
/* drivers/esp.c */
int ob_esp_init(unsigned int slot, uint64_t base, unsigned long espoffset,
                unsigned long dmaoffset);
#endif
#ifdef CONFIG_DRIVER_OBIO
/* drivers/obio.c */
int ob_obio_init(uint64_t slavio_base, unsigned long fd_offset,
                 unsigned long counter_offset, unsigned long intr_offset,
                 unsigned long aux1_offset, unsigned long aux2_offset);
int start_cpu(unsigned int pc, unsigned int context_ptr, unsigned int context,
              int cpu);

/* drivers/iommu.c */
extern struct mem cmem;

/* drivers/sbus.c */
extern uint16_t graphic_depth;

/* drivers/obio.c */
extern volatile unsigned char *power_reg;
extern volatile unsigned int *reset_reg;
extern volatile struct sun4m_timer_regs *counter_regs;

/* arch/sparc32/romvec.c */
extern const char *obp_stdin_path, *obp_stdout_path;
extern char obp_stdin, obp_stdout;

/* arch/sparc32/boot.c */
extern uint32_t kernel_image;
extern uint32_t kernel_size;
extern uint32_t qemu_cmdline;
extern uint32_t cmdline_size;
extern char boot_device;
#endif
#ifdef CONFIG_DRIVER_FLOPPY
int ob_floppy_init(void);
#endif

#endif /* OPENBIOS_DRIVERS_H */
