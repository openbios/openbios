#ifndef _H_PCI
#define _H_PCI

typedef uint32_t pci_addr;

typedef struct pci_arch_t pci_arch_t;

struct pci_arch_t {
    const char * name;
    uint16_t vendor_id;
    uint16_t device_id;
    unsigned long cfg_addr;
    unsigned long cfg_data;
    unsigned long cfg_base;
    unsigned long cfg_len;
    unsigned long mem_base;
    unsigned long mem_len;
    unsigned long io_base;
    unsigned long io_len;
    unsigned long rbase;
    unsigned long rlen;
};

extern pci_arch_t *arch;

#endif	/* _H_PCI */
