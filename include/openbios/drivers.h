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
int ob_sbus_init(void);
#endif
#ifdef CONFIG_DRIVER_IDE
int ob_ide_init(void);
#endif
#ifdef CONFIG_DRIVER_ESP
int ob_esp_init(void);
#endif

