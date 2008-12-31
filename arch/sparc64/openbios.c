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
#include "openbios/nvram.h"
#include "sys_info.h"
#include "openbios.h"
#include "openbios/pci.h"
#include "asm/pci.h"
#include "libc/byteorder.h"
#define cpu_to_be16(x) __cpu_to_be16(x)
#include "openbios/firmware_abi.h"
#include "boot.h"
#include "../../drivers/timer.h" // XXX
#include "asi.h"
#include "spitfire.h"
#include "libc/vsprintf.h"
#define NO_QEMU_PROTOS
#include "openbios/fw_cfg.h"

#define BIOS_CFG_CMD  0x510
#define BIOS_CFG_DATA 0x511

#define NVRAM_ADDR_LO 0x74
#define NVRAM_ADDR_HI 0x75
#define NVRAM_DATA    0x77

#define APB_SPECIAL_BASE     0x1fe00000000ULL
#define PCI_CONFIG           (APB_SPECIAL_BASE + 0x1000000ULL)
#define APB_MEM_BASE         0x1ff00000000ULL

static unsigned char intdict[256 * 1024];

// XXX
#define NVRAM_SIZE       0x2000
#define NVRAM_IDPROM     0x1fd8
#define NVRAM_IDPROM_SIZE 32
#define NVRAM_OB_START   (sizeof(ohwcfg_v3_t) + sizeof(struct sparc_arch_cfg))
#define NVRAM_OB_SIZE    ((0x1fd0 - NVRAM_OB_START) & ~15)

static ohwcfg_v3_t nv_info;

#define OBIO_CMDLINE_MAX 256
static char obio_cmdline[OBIO_CMDLINE_MAX];

static uint8_t idprom[NVRAM_IDPROM_SIZE];

struct hwdef {
    pci_arch_t pci;
    uint16_t machine_id_low, machine_id_high;
};

static const struct hwdef hwdefs[] = {
    {
        .pci = {
            .cfg_addr = PCI_CONFIG,
            .cfg_data = 0,
            .cfg_base = 0x80000000ULL,
            .cfg_len = 0,
        },
        .machine_id_low = 0,
        .machine_id_high = 255,
    },
};

struct cpudef {
    unsigned long iu_version;
    const char *name;
};

#define PAGE_SIZE_4M   (4 * 1024 * 1024)
#define PAGE_SIZE_512K (512 * 1024)
#define PAGE_SIZE_64K  (64 * 1024)
#define PAGE_SIZE_8K   (8 * 1024)
#define PAGE_MASK_4M   (4 * 1024 * 1024 - 1)
#define PAGE_MASK_512K (512 * 1024 - 1)
#define PAGE_MASK_64K  (64 * 1024 - 1)
#define PAGE_MASK_8K   (8 * 1024 - 1)

#define UUID_FMT "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x"

static void
mmu_open(void)
{
    RET(-1);
}

static void
mmu_close(void)
{
}

static int
spitfire_translate(unsigned long virt, unsigned long *p_phys,
                   unsigned long *p_data, unsigned long *p_size)
{
    unsigned long phys, tag, data, mask, size;
    unsigned int i;

    for (i = 0; i < 64; i++) {
        data = spitfire_get_dtlb_data(i);
        if (data & 0x8000000000000000ULL) { // Valid entry?
            switch ((data >> 61) & 3) {
            default:
            case 0x0: // 8k
                mask = 0xffffffffffffe000ULL;
                size = PAGE_SIZE_8K;
                break;
            case 0x1: // 64k
                mask = 0xffffffffffff0000ULL;
                size = PAGE_SIZE_64K;
                break;
            case 0x2: // 512k
                mask = 0xfffffffffff80000ULL;
                size = PAGE_SIZE_512K;
                break;
            case 0x3: // 4M
                mask = 0xffffffffffc00000ULL;
                size = PAGE_SIZE_4M;
                break;
            }
            tag = spitfire_get_dtlb_tag(i);
            if ((virt & mask) == (tag & mask)) {
                phys = data & mask & 0x000001fffffff000ULL;
                phys |= virt & ~mask;
                *p_phys = phys;
                *p_data = data & 0xfff;
                *p_size = size;
                return -1;
            }
        }
    }

    return 0;
}

/*
  3.6.5 translate
  ( virt -- false | phys.lo ... phys.hi mode true )
*/
static void
mmu_translate(void)
{
    unsigned long virt, phys, data, size;

    virt = POP();

    if (spitfire_translate(virt, &phys, &data, &size)) {
        PUSH(phys & 0xffffffff);
        PUSH(phys >> 32);
        PUSH(data);
        PUSH(-1);
        return;
    }
    PUSH(0);
}

static void
dtlb_load2(unsigned long vaddr, unsigned long tte_data)
{
    asm("stxa %0, [%1] %2\n"
        "stxa %3, [%%g0] %4\n"
        : : "r" (vaddr), "r" (48), "i" (ASI_DMMU),
          "r" (tte_data), "i" (ASI_DTLB_DATA_IN));
}

static void
dtlb_load3(unsigned long vaddr, unsigned long tte_data,
           unsigned long tte_index)
{
    asm("stxa %0, [%1] %2\n"
        "stxa %3, [%4] %5\n"
        : : "r" (vaddr), "r" (48), "i" (ASI_DMMU),
          "r" (tte_data), "r" (tte_index << 3), "i" (ASI_DTLB_DATA_ACCESS));
}

/*
  ( index tte_data vaddr -- ? )
*/
static void
dtlb_load(void)
{
    unsigned long vaddr, tte_data, idx;

    vaddr = POP();
    tte_data = POP();
    idx = POP();
    dtlb_load3(vaddr, tte_data, idx);
}

static void
itlb_load2(unsigned long vaddr, unsigned long tte_data)
{
    asm("stxa %0, [%1] %2\n"
        "stxa %3, [%%g0] %4\n"
        : : "r" (vaddr), "r" (48), "i" (ASI_IMMU),
          "r" (tte_data), "i" (ASI_ITLB_DATA_IN));
}

static void
itlb_load3(unsigned long vaddr, unsigned long tte_data,
           unsigned long tte_index)
{
    asm("stxa %0, [%1] %2\n"
        "stxa %3, [%4] %5\n"
        : : "r" (vaddr), "r" (48), "i" (ASI_IMMU),
          "r" (tte_data), "r" (tte_index << 3), "i" (ASI_ITLB_DATA_ACCESS));
}

/*
  ( index tte_data vaddr -- ? )
*/
static void
itlb_load(void)
{
    unsigned long vaddr, tte_data, idx;

    vaddr = POP();
    tte_data = POP();
    idx = POP();
    itlb_load3(vaddr, tte_data, idx);
}

static void
map_pages(unsigned long virt, unsigned long size, unsigned long phys)
{
    unsigned long tte_data, currsize;

    size = (size + PAGE_MASK_8K) & ~PAGE_MASK_8K;
    while (size >= PAGE_SIZE_8K) {
        currsize = size;
        if (currsize >= PAGE_SIZE_4M &&
            (virt & PAGE_MASK_4M) == 0 &&
            (phys & PAGE_MASK_4M) == 0) {
            currsize = PAGE_SIZE_4M;
            tte_data = 6ULL << 60;
        } else if (currsize >= PAGE_SIZE_512K &&
                   (virt & PAGE_MASK_512K) == 0 &&
                   (phys & PAGE_MASK_512K) == 0) {
            currsize = PAGE_SIZE_512K;
            tte_data = 4ULL << 60;
        } else if (currsize >= PAGE_SIZE_64K &&
                   (virt & PAGE_MASK_64K) == 0 &&
                   (phys & PAGE_MASK_64K) == 0) {
            currsize = PAGE_SIZE_64K;
            tte_data = 2ULL << 60;
        } else {
            currsize = PAGE_SIZE_8K;
            tte_data = 0;
        }
        tte_data |= phys | 0x8000000000000036ULL;
        dtlb_load2(virt, tte_data);
        itlb_load2(virt, tte_data);
        size -= currsize;
        phys += currsize;
        virt += currsize;
    }
}

/*
  3.6.5 map
  ( phys.lo ... phys.hi virt size mode -- )
*/
static void
mmu_map(void)
{
    unsigned long virt, size, mode, phys;

    mode = POP();
    size = POP();
    virt = POP();
    phys = POP();
    phys <<= 32;
    phys |= POP();
    map_pages(virt, size, phys);
}

static void
itlb_demap(unsigned long vaddr)
{
    asm("stxa %0, [%0] %1\n"
        : : "r" (vaddr), "i" (ASI_IMMU_DEMAP));
}

static void
dtlb_demap(unsigned long vaddr)
{
    asm("stxa %0, [%0] %1\n"
        : : "r" (vaddr), "i" (ASI_DMMU_DEMAP));
}

static void
unmap_pages(unsigned long virt, unsigned long size)
{
    unsigned long phys, data;

    unsigned long currsize;

    // align size
    size = (size + PAGE_MASK_8K) & ~PAGE_MASK_8K;

    while (spitfire_translate(virt, &phys, &data, &currsize)) {

        itlb_demap(virt & ~0x1fffULL);
        dtlb_demap(virt & ~0x1fffULL);

        size -= currsize;
        virt += currsize;
    }
}

/*
  3.6.5 unmap
  ( virt size -- )
*/
static void
mmu_unmap(void)
{
    unsigned long virt, size;

    size = POP();
    virt = POP();
    unmap_pages(virt, size);
}

/*
  3.6.5 claim
  ( virt size align -- base )
*/
static void
mmu_claim(void)
{
    unsigned long virt, size, align;

    align = POP();
    size = POP();
    virt = POP();
    PUSH(virt); // XXX
}

/*
  3.6.5 release
  ( virt size -- )
*/
static void
mmu_release(void)
{
    unsigned long virt, size;

    size = POP();
    virt = POP();
    // XXX
}

DECLARE_UNNAMED_NODE(mmu, INSTALL_OPEN, 0);

NODE_METHODS(mmu) = {
    { "open",               mmu_open              },
    { "close",              mmu_close             },
    { "translate",          mmu_translate         },
    { "SUNW,dtlb-load",     dtlb_load             },
    { "SUNW,itlb-load",     itlb_load             },
    { "map",                mmu_map               },
    { "unmap",              mmu_unmap             },
    { "claim",              mmu_claim             },
    { "release",            mmu_release           },
};

/*
  ( addr -- ? )
*/
static void
set_trap_table(void)
{
    unsigned long addr;

    addr = POP();
    asm("wrpr %0, %%tba\n"
        : : "r" (addr));
}

static void cpu_generic_init(const struct cpudef *cpu, uint32_t clock_frequency)
{
    unsigned long iu_version;
    char nodebuff[256];

    push_str("/");
    fword("find-device");

    fword("new-device");

    push_str(cpu->name);
    fword("device-name");

    push_str("cpu");
    fword("device-type");

    asm("rdpr %%ver, %0\n"
        : "=r"(iu_version) :);

    PUSH((iu_version >> 48) & 0xff);
    fword("encode-int");
    push_str("manufacturer#");
    fword("property");

    PUSH((iu_version >> 32) & 0xff);
    fword("encode-int");
    push_str("implementation#");
    fword("property");

    PUSH((iu_version >> 24) & 0xff);
    fword("encode-int");
    push_str("mask#");
    fword("property");

    PUSH(9);
    fword("encode-int");
    push_str("sparc-version");
    fword("property");

    PUSH(0);
    fword("encode-int");
    push_str("cpuid");
    fword("property");

    PUSH(clock_frequency);
    fword("encode-int");
    push_str("clock-frequency");
    fword("property");

    fword("finish-device");

    // MMU node
    snprintf(nodebuff, sizeof(nodebuff), "/%s", cpu->name);
    push_str(nodebuff);
    fword("find-device");

    fword("new-device");

    push_str("mmu");
    fword("device-name");

    fword("finish-device");

    snprintf(nodebuff, sizeof(nodebuff), "/%s/mmu", cpu->name);

    REGISTER_NODE_METHODS(mmu, nodebuff);

    push_str("/chosen");
    fword("find-device");

    push_str(nodebuff);
    fword("open-dev");
    fword("encode-int");
    push_str("mmu");
    fword("property");

    // Trap table
    push_str("/openprom/client-services");
    fword("find-device");
    bind_func("SUNW,set-trap-table", set_trap_table);
}

static const struct cpudef sparc_defs[] = {
    {
        .iu_version = (0x04ULL << 48) | (0x02ULL << 32),
        .name = "FJSV,GP",
    },
    {
        .iu_version = (0x04ULL << 48) | (0x03ULL << 32),
        .name = "FJSV,GPUSK",
    },
    {
        .iu_version = (0x04ULL << 48) | (0x04ULL << 32),
        .name = "FJSV,GPUSC",
    },
    {
        .iu_version = (0x04ULL << 48) | (0x05ULL << 32),
        .name = "FJSV,GPUZC",
    },
    {
        .iu_version = (0x17ULL << 48) | (0x10ULL << 32),
        .name = "SUNW,UltraSPARC",
    },
    {
        .iu_version = (0x17ULL << 48) | (0x11ULL << 32),
        .name = "SUNW,UltraSPARC-II",
    },
    {
        .iu_version = (0x17ULL << 48) | (0x12ULL << 32),
        .name = "SUNW,UltraSPARC-IIi",
    },
    {
        .iu_version = (0x17ULL << 48) | (0x13ULL << 32),
        .name = "SUNW,UltraSPARC-IIe",
    },
    {
        .iu_version = (0x3eULL << 48) | (0x14ULL << 32),
        .name = "SUNW,UltraSPARC-III",
    },
    {
        .iu_version = (0x3eULL << 48) | (0x15ULL << 32),
        .name = "SUNW,UltraSPARC-III+",
    },
    {
        .iu_version = (0x3eULL << 48) | (0x16ULL << 32),
        .name = "SUNW,UltraSPARC-IIIi",
    },
    {
        .iu_version = (0x3eULL << 48) | (0x18ULL << 32),
        .name = "SUNW,UltraSPARC-IV",
    },
    {
        .iu_version = (0x3eULL << 48) | (0x19ULL << 32),
        .name = "SUNW,UltraSPARC-IV+",
    },
    {
        .iu_version = (0x3eULL << 48) | (0x22ULL << 32),
        .name = "SUNW,UltraSPARC-IIIi+",
    },
    {
        .iu_version = (0x3eULL << 48) | (0x23ULL << 32),
        .name = "SUNW,UltraSPARC-T1",
    },
    {
        .iu_version = (0x3eULL << 48) | (0x24ULL << 32),
        .name = "SUNW,UltraSPARC-T2",
    },
    {
        .iu_version = (0x22ULL << 48) | (0x10ULL << 32),
        .name = "SUNW,UltraSPARC",
    },
};

static const struct cpudef *
id_cpu(void)
{
    unsigned long iu_version;
    unsigned int i;

    asm("rdpr %%ver, %0\n"
        : "=r"(iu_version) :);
    iu_version &= 0xffffffff00000000ULL;

    for (i = 0; i < sizeof(sparc_defs)/sizeof(struct cpudef); i++) {
        if (iu_version == sparc_defs[i].iu_version)
            return &sparc_defs[i];
    }
    printk("Unknown cpu (psr %lx), freezing!\n", iu_version);
    for (;;);
}

static void
fw_cfg_read(uint16_t cmd, char *buf, unsigned int nbytes)
{
    unsigned int i;

    outw(__cpu_to_le16(cmd), BIOS_CFG_CMD);
    for (i = 0; i < nbytes; i++)
        buf[i] = inb(BIOS_CFG_DATA);
}

static uint64_t
fw_cfg_read_i64(uint16_t cmd)
{
    char buf[sizeof(uint64_t)];

    fw_cfg_read(cmd, buf, sizeof(uint64_t));

    return __le64_to_cpu(*(uint64_t *)buf);
}

static uint32_t
fw_cfg_read_i32(uint16_t cmd)
{
    char buf[sizeof(uint32_t)];

    fw_cfg_read(cmd, buf, sizeof(uint32_t));

    return __le32_to_cpu(*(uint32_t *)buf);
}

static uint16_t
fw_cfg_read_i16(uint16_t cmd)
{
    char buf[sizeof(uint16_t)];

    fw_cfg_read(cmd, buf, sizeof(uint16_t));

    return __le16_to_cpu(*(uint16_t *)buf);
}

static uint8_t nvram_read_byte(uint16_t offset)
{
    outb(offset & 0xff, NVRAM_ADDR_LO);
    outb(offset >> 8, NVRAM_ADDR_HI);
    return inb(NVRAM_DATA);
}

static void nvram_read(uint16_t offset, char *buf, unsigned int nbytes)
{
    unsigned int i;

    for (i = 0; i < nbytes; i++)
        buf[i] = nvram_read_byte(offset + i);
}

static void nvram_write_byte(uint16_t offset, uint8_t val)
{
    outb(offset & 0xff, NVRAM_ADDR_LO);
    outb(offset >> 8, NVRAM_ADDR_HI);
    outb(val, NVRAM_DATA);
}

static void nvram_write(uint16_t offset, const char *buf, unsigned int nbytes)
{
    unsigned int i;

    for (i = 0; i < nbytes; i++)
        nvram_write_byte(offset + i, buf[i]);
}

static uint8_t qemu_uuid[16];

void arch_nvram_get(char *data)
{
    uint32_t size;
    const struct cpudef *cpu;
    const char *bootpath;
    char buf[256];
    uint32_t temp;
    uint64_t ram_size;
    uint32_t clock_frequency;
    uint16_t machine_id;

    nvram_read(0, (char *)&nv_info, sizeof(ohwcfg_v3_t));

    fw_cfg_read(FW_CFG_SIGNATURE, buf, 4);
    buf[4] = '\0';

    printk("Configuration device id %s", buf);

    temp = fw_cfg_read_i32(FW_CFG_ID);
    machine_id = fw_cfg_read_i16(FW_CFG_MACHINE_ID);

    printk(" version %d machine id %d\n", temp, machine_id);

    if (temp != 1) {
        printk("Incompatible configuration device version, freezing\n");
        for(;;);
    }

    kernel_image = nv_info.kernel_image;
    kernel_size = nv_info.kernel_size;
    size = nv_info.cmdline_size;
    if (size > OBIO_CMDLINE_MAX - 1)
        size = OBIO_CMDLINE_MAX - 1;
    memcpy(&obio_cmdline, (void *)(long)nv_info.cmdline, size);
    obio_cmdline[size] = '\0';
    qemu_cmdline = (uint64_t)obio_cmdline;
    cmdline_size = size;
    boot_device = nv_info.boot_devices[0];

    if (kernel_size)
        printk("kernel addr %llx size %llx\n", kernel_image, kernel_size);
    if (size)
        printk("kernel cmdline %s\n", obio_cmdline);

    nvram_read(NVRAM_OB_START, data, NVRAM_OB_SIZE);

    temp = fw_cfg_read_i32(FW_CFG_NB_CPUS);

    printk("CPUs: %x", temp);

    clock_frequency = 100000000;

    cpu = id_cpu();
    //cpu->initfn();
    cpu_generic_init(cpu, clock_frequency);
    printk(" x %s\n", cpu->name);

    // Add /uuid
    fw_cfg_read(FW_CFG_UUID, (char *)qemu_uuid, 16);

    printk("UUID: " UUID_FMT "\n", qemu_uuid[0], qemu_uuid[1], qemu_uuid[2],
           qemu_uuid[3], qemu_uuid[4], qemu_uuid[5], qemu_uuid[6],
           qemu_uuid[7], qemu_uuid[8], qemu_uuid[9], qemu_uuid[10],
           qemu_uuid[11], qemu_uuid[12], qemu_uuid[13], qemu_uuid[14],
           qemu_uuid[15]);

    push_str("/");
    fword("find-device");

    PUSH((long)&qemu_uuid);
    PUSH(16);
    fword("encode-bytes");
    push_str("uuid");
    fword("property");

    // Add /idprom
    nvram_read(NVRAM_IDPROM, (char *)idprom, NVRAM_IDPROM_SIZE);

    PUSH((long)&idprom);
    PUSH(32);
    fword("encode-bytes");
    push_str("idprom");
    fword("property");

    PUSH(500 * 1000 * 1000);
    fword("encode-int");
    push_str("clock-frequency");
    fword("property");

    push_str("/memory");
    fword("find-device");

    ram_size = fw_cfg_read_i64(FW_CFG_RAM_SIZE);

    // All memory: 0 to RAM_size
    PUSH(0);
    fword("encode-int");
    PUSH(0);
    fword("encode-int");
    fword("encode+");
    PUSH((int)(ram_size >> 32));
    fword("encode-int");
    fword("encode+");
    PUSH((int)(ram_size & 0xffffffff));
    fword("encode-int");
    fword("encode+");
    push_str("reg");
    fword("property");

    // Available memory: 0 to va2pa(_start)
    PUSH(0);
    fword("encode-int");
    PUSH(0);
    fword("encode-int");
    fword("encode+");
    PUSH((va2pa((unsigned long)&_data) - 8192) >> 32);
    fword("encode-int");
    fword("encode+");
    PUSH((va2pa((unsigned long)&_data) - 8192) & 0xffffffff);
    fword("encode-int");
    fword("encode+");
    push_str("available");
    fword("property");

    // XXX
    // Translations
    push_str("/virtual-memory");
    fword("find-device");

    // 0 to 16M: 1:1
    PUSH(0);
    fword("encode-int");
    PUSH(0);
    fword("encode-int");
    fword("encode+");
    PUSH(0);
    fword("encode-int");
    fword("encode+");
    PUSH(16 * 1024 * 1024);
    fword("encode-int");
    fword("encode+");
    PUSH(0x80000000);
    fword("encode-int");
    fword("encode+");
    PUSH(0x00000036);
    fword("encode-int");
    fword("encode+");

    // _start to _data: ROM used
    PUSH(0);
    fword("encode-int");
    fword("encode+");
    PUSH((unsigned long)&_start);
    fword("encode-int");
    fword("encode+");
    PUSH(0);
    fword("encode-int");
    fword("encode+");
    PUSH((unsigned long)&_data - (unsigned long)&_start);
    fword("encode-int");
    fword("encode+");
    PUSH(0x800001ff);
    fword("encode-int");
    fword("encode+");
    PUSH(0xf0000074);
    fword("encode-int");
    fword("encode+");

    // _data to _end: end of RAM
    PUSH(0);
    fword("encode-int");
    fword("encode+");
    PUSH((unsigned long)&_data);
    fword("encode-int");
    fword("encode+");
    PUSH(0);
    fword("encode-int");
    fword("encode+");
    PUSH((unsigned long)&_data - (unsigned long)&_start);
    fword("encode-int");
    fword("encode+");
    PUSH(((va2pa((unsigned long)&_data) - 8192) >> 32) | 0x80000000);
    fword("encode-int");
    fword("encode+");
    PUSH(((va2pa((unsigned long)&_data) - 8192) & 0xffffffff) | 0x36);
    fword("encode-int");
    fword("encode+");

    // VGA buffer (128k): 1:1
    PUSH(0x1ff);
    fword("encode-int");
    fword("encode+");
    PUSH(0x004a0000);
    fword("encode-int");
    fword("encode+");
    PUSH(0);
    fword("encode-int");
    fword("encode+");
    PUSH(128 * 1024);
    fword("encode-int");
    fword("encode+");
    PUSH(0x800001ff);
    fword("encode-int");
    fword("encode+");
    PUSH(0x004a0076);
    fword("encode-int");
    fword("encode+");

    push_str("translations");
    fword("property");

    push_str("/chosen");
    fword("find-device");

    if (nv_info.boot_devices[0] == 'c')
        bootpath = "/pci/isa/ide0/disk@0,0:a";
    else
        bootpath = "/pci/isa/ide1/cdrom@0,0:a";
    push_str(bootpath);
    fword("encode-string");
    push_str("bootpath");
    fword("property");

    push_str(obio_cmdline);
    fword("encode-string");
    push_str("bootargs");
    fword("property");
}

void arch_nvram_put(char *data)
{
    nvram_write(0, data, NVRAM_OB_SIZE);
}

int arch_nvram_size(void)
{
    return NVRAM_OB_SIZE;
}

void setup_timers(void)
{
}

void udelay(unsigned int usecs)
{
}

static void init_memory(void)
{

	/* push start and end of available memory to the stack
	 * so that the forth word QUIT can initialize memory
	 * management. For now we use hardcoded memory between
	 * 0x10000 and 0x9ffff (576k). If we need more memory
	 * than that we have serious bloat.
	 */

	PUSH((ucell)&_heap);
	PUSH((ucell)&_eheap);
}

static void
arch_init( void )
{
        unsigned int i;
        uint16_t machine_id;
        const struct hwdef *hwdef = NULL;

        machine_id = fw_cfg_read_i16(FW_CFG_MACHINE_ID);

        for (i = 0; i < sizeof(hwdefs) / sizeof(struct hwdef); i++) {
            if (hwdefs[i].machine_id_low <= machine_id &&
                hwdefs[i].machine_id_high >= machine_id) {
                hwdef = &hwdefs[i];
                break;
            }
        }
        if (!hwdef)
            for(;;); // Internal inconsistency, hang

	modules_init();
#ifdef CONFIG_DRIVER_PCI
        //ob_pci_init();
#endif
#ifdef CONFIG_DRIVER_IDE
	setup_timers();
	ob_ide_init("/pci/isa", 0x1f0, 0x3f4, 0x170, 0x374);
#endif
#ifdef CONFIG_DRIVER_FLOPPY
	ob_floppy_init();
#endif
#ifdef CONFIG_DEBUG_CONSOLE_VIDEO
	init_video();
#endif

        nvconf_init();
        ob_su_init(0x1fe02000000ULL, 0x3f8ULL, 0);

        device_end();

	bind_func("platform-boot", boot );
        printk("\n"); // XXX needed for boot, why?
}

int openbios(void)
{
#ifdef CONFIG_DEBUG_CONSOLE
#ifdef CONFIG_DEBUG_CONSOLE_SERIAL
	uart_init(CONFIG_SERIAL_PORT, CONFIG_SERIAL_SPEED);
#endif
#ifdef CONFIG_DEBUG_CONSOLE_VGA
	video_init();
#endif
	/* Clear the screen.  */
	cls();
        printk("OpenBIOS for Sparc64\n");
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
        printk("falling off...\n");
	return 0;
}
