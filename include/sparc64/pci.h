#ifndef SPARC64_PCI_H
#define SPARC64_PCI_H

#include "asm/io.h"

#if !(defined(PCI_CONFIG_1) || defined(PCI_CONFIG_2))
#define PCI_CONFIG_1 1 /* default */
#endif

#ifdef PCI_CONFIG_1

/* PCI Configuration Mechanism #1 */

/* Have pci_addr in the same format as the values written to 0xcf8
 * so register accesses can be made easy. */
#define PCI_ADDR(bus, dev, fn) \
    ((pci_addr) (0x80000000u \
		| (uint32_t) (bus) << 16 \
		| (uint32_t) (dev) << 11 \
		| (uint32_t) (fn) << 8))

#define PCI_BUS(pcidev) ((uint8_t) ((pcidev) >> 16))
#define PCI_DEV(pcidev) ((uint8_t) ((pcidev) >> 11) & 0x1f)
#define PCI_FN(pcidev) ((uint8_t) ((pcidev) >> 8) & 7)

#define APB_SPECIAL_BASE     0x1fe00000000ULL
#define PCI_CONFIG           (APB_SPECIAL_BASE + 0x1000000ULL)
#define APB_MEM_BASE	     0x1ff00000000ULL

static inline uint8_t pci_config_read8(pci_addr dev, uint8_t reg)
{
    out_le32((void *)PCI_CONFIG, dev | (reg & ~3));
    return in_8((void *)(APB_MEM_BASE | (reg & 3)));
}

static inline uint16_t pci_config_read16(pci_addr dev, uint8_t reg)
{
    out_le32((void *)PCI_CONFIG, dev | (reg & ~3));
    return in_le16((void *)(APB_MEM_BASE | (reg & 2)));
}

static inline uint32_t pci_config_read32(pci_addr dev, uint8_t reg)
{
    out_le32((void *)PCI_CONFIG, dev | reg);
    return in_le32((void *)(APB_MEM_BASE | reg));
}

static inline void pci_config_write8(pci_addr dev, uint8_t reg, uint8_t val)
{
    out_le32((void *)PCI_CONFIG, dev | (reg & ~3));
    out_8((void *)(APB_MEM_BASE | (reg & 3)), val);
}

static inline void pci_config_write16(pci_addr dev, uint8_t reg, uint16_t val)
{
    out_le32((void *)PCI_CONFIG, dev | (reg & ~3));
    out_le16((void *)(APB_MEM_BASE | (reg & 2)), val);
}

static inline void pci_config_write32(pci_addr dev, uint8_t reg, uint32_t val)
{
    out_le32((void *)PCI_CONFIG, dev | reg);
    out_le32((void *)(APB_MEM_BASE | reg), val);
}

#else /* !PCI_CONFIG_1 */
#error PCI Configuration Mechanism is not specified or implemented
#endif

#endif /* SPARC64_PCI_H */
