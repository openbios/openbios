#ifndef PPC_PCI_H
#define PPC_PCI_H

#include "asm/io.h"

/* Sandpoint example */
#define ISA_IO_BASE 0x80000000
#define ISA_MEM_BASE 0xc0000000
#define PCIC0_CFGADDR 0xfec00cf8
#define PCIC0_CFGDATA 0xfee00cfc
#ifndef _IO_BASE
#define _IO_BASE ISA_IO_BASE
#endif

#if !(PCI_CONFIG_1 || PCI_CONFIG_2)
#define PCI_CONFIG_1 1 /* default */
#endif

#ifdef PCI_CONFIG_1

/* PCI Configuration Mechanism #1 */

/* Have pci_addr in the same format as the values written to PCIC0_CFGADDR
 * so register accesses can be made easy. */
#define PCI_ADDR(bus, dev, fn) \
    ((pci_addr) (0x80000000u \
		| (uint32_t) (bus) << 16 \
		| (uint32_t) (dev) << 11 \
		| (uint32_t) (fn) << 8))

#define PCI_BUS(pcidev) ((uint8_t) ((pcidev) >> 16))
#define PCI_DEV(pcidev) ((uint8_t) ((pcidev) >> 11) & 0x1f)
#define PCI_FN(pcidev) ((uint8_t) ((pcidev) >> 8) & 7)

static inline uint8_t pci_config_read8(pci_addr dev, uint8_t reg)
{
        uint8_t res;

        out_le32((unsigned *)PCIC0_CFGADDR, (dev | (reg & ~3)));
        res = in_8((unsigned char *)PCIC0_CFGDATA + (reg & 3));
        return res;
}

static inline uint16_t pci_config_read16(pci_addr dev, uint8_t reg)
{
	uint16_t res;

        out_le32((unsigned *)PCIC0_CFGADDR, (dev | (reg & ~3)));
        res = in_le16((unsigned char *)PCIC0_CFGDATA + (reg & 2));
        return res;
}

static inline uint32_t pci_config_read32(pci_addr dev, uint8_t reg)
{
	uint32_t res;

        out_le32((unsigned *)PCIC0_CFGADDR, (dev | (reg & ~3)));
        res = in_le32((unsigned char *)PCIC0_CFGDATA);
        return res;
}

static inline void pci_config_write8(pci_addr dev, uint8_t reg, uint8_t val)
{
    outl(dev | (reg & ~3), 0xcf8);
    outb(val, 0xcfc | (reg & 3));
}

static inline void pci_config_write16(pci_addr dev, uint8_t reg, uint16_t val)
{
    outl(dev | (reg & ~3), 0xcf8);
    outw(val, 0xcfc | (reg & 2));
}

static inline void pci_config_write32(pci_addr dev, uint8_t reg, uint32_t val)
{
    outl(dev | reg, 0xcf8);
    outl(val, 0xcfc);
}

#else /* !PCI_CONFIG_1 */
#error PCI Configuration Mechanism is not specified or implemented
#endif

#endif /* PPC_PCI_H */
