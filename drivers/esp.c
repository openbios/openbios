/*
 *   OpenBIOS ESP driver
 *   
 *   Copyright (C) 2004 Jens Axboe <axboe@suse.de>
 *   Copyright (C) 2005 Stefan Reinauer <stepan@openbios.org>
 *
 *   Credit goes to Hale Landis for his excellent ata demo software
 *   OF node handling and some fixes by Stefan Reinauer
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   version 2
 *
 */

#include "openbios/config.h"
#include "openbios/bindings.h"
#include "openbios/kernel.h"
#include "libc/byteorder.h"
#include "libc/vsprintf.h"

#include "openbios/drivers.h"
#include "asm/dma.h"

#define PHYS_JJ_ESPDMA  0x78400000      /* ESP DMA controller */
#define PHYS_JJ_ESP     0x78800000      /* ESP SCSI */

#define REGISTER_NAMED_NODE( name, path )   do {                        \
        bind_new_node( name##_flags_, name##_size_,                     \
                       path, name##_m, sizeof(name##_m)/sizeof(method_t)); \
    } while(0)

#define REGISTER_NODE_METHODS( name, path )   do {                      \
        char *paths[1];                                                  \
                                                                        \
        paths[0] = path;                                                \
        bind_node( name##_flags_, name##_size_,                         \
                   paths, 1, name##_m, sizeof(name##_m)/sizeof(method_t)); \
    } while(0)

struct esp_dma {
    struct sparc_dma_registers *regs;
    enum dvma_rev revision;
};

typedef struct sd_private {
    unsigned int id;
    unsigned int hw_sector;
} sd_private_t;

struct esp_regs {
    unsigned int regs[16];
};

typedef struct esp_private {
    volatile struct esp_regs *ll;
    __u32 buffer_dvma;
    unsigned int irq;        /* device IRQ number    */

    struct esp_dma *espdma;         /* If set this points to espdma    */

    unsigned char *buffer;
} esp_private_t;

/* DECLARE data structures for the nodes.  */
DECLARE_UNNAMED_NODE(ob_sd, INSTALL_OPEN, sizeof(sd_private_t));
DECLARE_UNNAMED_NODE(ob_esp, INSTALL_OPEN, sizeof(esp_private_t));

// offset is multiple of 512, len in bytes
static int
ob_sd_read_sectors(sd_private_t *sd, int offset, void *dest, short len)
{
#if 0
    unsigned char *buffer = malloc(2048); // XXX setup dvma

    // Set SCSI target
    outb(PHYS_JJ_ESP + 4*4, sd->id & 7);
    // Set DMA address
    outl(PHYS_JJ_ESPDMA + 4, buffer);
    // Set DMA length
    outb(PHYS_JJ_ESP + 0*4, 10);
    outb(PHYS_JJ_ESP + 1*4, 0);
    // Set DMA direction
    outl(PHYS_JJ_ESPDMA + 0, 0x000);
    // Setup command = Read(10)
    buffer[0] = 0x80;
    buffer[1] = 0x28;
    buffer[2] = 0x00;
    buffer[3] = (offset >> 24) & 0xff;
    buffer[4] = (offset >> 16) & 0xff;
    buffer[5] = (offset >> 8) & 0xff;
    buffer[6] = offset & 0xff;
    buffer[7] = 0x00;
    buffer[8] = ((len / 512) >> 8) & 0xff;
    buffer[9] = (len / 512) & 0xff;
    // Set ATN, issue command
    outb(PHYS_JJ_ESP + 3*4, 0xc2);

    // Set DMA length
    outb(PHYS_JJ_ESP + 0*4, len & 0xff);
    outb(PHYS_JJ_ESP + 1*4, (len >> 8) & 0xff);
    // Set DMA direction
    outl(PHYS_JJ_ESPDMA + 0, 0x100);
    // Transfer
    outb(PHYS_JJ_ESP + 3*4, 0x90);
    memcpy(buffer, dest, len);
    free(buffer);
#endif
    return 0 * sd->id * offset * len * (int)dest;
}

static void
ob_sd_read_blocks(sd_private_t *sd)
{
    cell n = POP(), cnt=n;
    ucell blk = POP();
    char *dest = (char*)POP();

#ifdef CONFIG_DEBUG_ESP
    printk("ob_sd_read_blocks %lx block=%d n=%d\n", (unsigned long)dest, blk, n );
#endif
    while (n) {
        int len = n;

        if (ob_sd_read_sectors(sd, blk, dest, len)) {
            printk("ob_ide_read_blocks: error\n");
            RET(0);
        }
        dest += len * sd->hw_sector;
        n -= len;
        blk += len;
    }
    PUSH(cnt);
}

static void
ob_sd_block_size(sd_private_t *sd)
{
    PUSH(sd->hw_sector);
}

static unsigned int
get_block_size(sd_private_t *sd)
{
#if 1
    return 512 + sd->id * 0; // XXX
#else
    unsigned char *buffer = malloc(64); // XXX setup dvma
    unsigned int ret;

    // Set SCSI target
    outb(PHYS_JJ_ESP + 4*4, sd->id & 7);
    // Set DMA address
    outl(PHYS_JJ_ESPDMA + 4, buffer);
    // Set DMA length
    outb(PHYS_JJ_ESP + 0*4, 10);
    outb(PHYS_JJ_ESP + 1*4, 0);
    // Set DMA direction
    outl(PHYS_JJ_ESPDMA + 0, 0x000);
    // Setup command = Read Capacity
    buffer[0] = 0x80;
    buffer[1] = 0x25;
    buffer[2] = 0x00;
    buffer[3] = 0x00;
    buffer[4] = 0x00;
    buffer[5] = 0x00;
    buffer[6] = 0x00;
    buffer[7] = 0x00;
    buffer[8] = 0x00;
    buffer[9] = 0x00;
    buffer[10] = 0x00;
    // Set ATN, issue command
    outb(PHYS_JJ_ESP + 3*4, 0xc2);
    
    // Set DMA length
    outb(PHYS_JJ_ESP + 0*4, 0);
    outb(PHYS_JJ_ESP + 1*4, 8 & 0xff);
    // Set DMA direction
    outl(PHYS_JJ_ESPDMA + 0, 0x100);
    // Transfer
    outb(PHYS_JJ_ESP + 3*4, 0x90);
    ret = (buffer[4] << 24) | (buffer[5] << 16) | (buffer[6] << 8) | buffer[7];
    free(buffer);
    return ret;
#endif
}

static void
ob_sd_open(sd_private_t *sd)
{
    int ret=1;
    phandle_t ph;

    fword("my-unit");
    sd->id = POP();
    sd->hw_sector = get_block_size(sd);

#ifdef CONFIG_DEBUG_ESP
    printk("opening drive %d\n", sd->id);
#endif

#if 0
    dump_drive(drive);

    if (drive->type != esp_type_ata)
        ret= !ob_esp_atapi_drive_ready(drive);
#endif

    selfword("open-deblocker");

    /* interpose disk-label */
    ph = find_dev("/packages/disk-label");
    fword("my-args");
    PUSH_ph( ph );
    fword("interpose");
    
    RET ( -ret );
}

static void
ob_sd_close(__attribute__((unused)) sd_private_t *sd)
{
    selfword("close-deblocker");
}

NODE_METHODS(ob_sd) = {
    { "open",           ob_sd_open },
    { "close",          ob_sd_close },
    { "read-blocks",    ob_sd_read_blocks },
    { "block-size",     ob_sd_block_size },
};

static void
ob_esp_initialize(esp_private_t *esp)
{
    phandle_t ph=get_cur_dev();

    set_int_property(ph, "#address-cells", 2);
    set_int_property(ph, "#size-cells", 0);

    /* set device type */
    push_str("scsi");
    fword("device-type");

    /* set reg */
    PUSH(4);
    fword("encode-int");
    PUSH(0x08800000);
    fword("encode-int");
    fword("encode+");
    PUSH(0x00000010);
    fword("encode-int");
    fword("encode+");
    push_str("reg");
    fword("property");
#if 1
    esp->ll = (void *)PHYS_JJ_ESP;
    esp->buffer = (void *)0x100000; // XXX
#else
    /* Get the IO region */
    esp->ll = map_io(PHYS_JJ_ESP, sizeof (struct esp_regs));
    if (esp->ll == 0)
        return -1;

    esp->buffer = dvma_alloc(BUFSIZE, &esp->buffer_dvma);
    esp->espdma = espdma;
#endif
    // Chip reset
    outb((int)esp->ll + 3*2, 2);
}

static void
ob_esp_decodeunit(__attribute__((unused)) esp_private_t * esp)
{
    fword("decode-unit-scsi");
}


static void
ob_esp_encodeunit(__attribute__((unused)) esp_private_t * esp)
{
    fword("encode-unit-scsi");
}

NODE_METHODS(ob_esp) = {
    { NULL,             ob_esp_initialize },
    { "decode-unit",    ob_esp_decodeunit },
    { "encode-unit",    ob_esp_encodeunit },
};

static int
drive_present(int drive)
{
    // XXX
    if (drive == 0 || drive == 2)
        return 1;
    return 0;
}

static int
drive_cdrom(int drive)
{
    // XXX
    if (drive == 2)
        return 1;
    return 0;
}

static void
add_alias(const unsigned char *device, const unsigned char *alias)
{
    push_str("/aliases");
    fword("find-device");
    push_str(device);
    fword("encode-string");
    push_str(alias);
    fword("encode-string");
    fword("property");
}

int ob_esp_init(void)
{
    int id, diskcount = 0, cdcount = 0, *counter_ptr;
    char nodebuff[256], aliasbuff[256];
    const char *type;

#ifdef CONFIG_DEBUG_ESP
    printk("Initializing SCSI...");
#endif
    sprintf(nodebuff, "/iommu/sbus/espdma/esp");
    REGISTER_NAMED_NODE(ob_esp, nodebuff);
    device_end();
#ifdef CONFIG_DEBUG_ESP
    printk("done\n");
    printk("Initializing SCSI devices...");
#endif
    for (id = 0; id < 8; id++) {
        if (!drive_present(id))
            continue;
        push_str("/iommu/sbus/espdma/esp");
        fword("find-device");
        fword("new-device");
        push_str("sd");
        fword("device-name");
        push_str("block");
        fword("device-type");
        fword("is-deblocker");
        PUSH(id);
        fword("encode-int");
        PUSH(0);
        fword("encode-int");
        fword("encode+");
        push_str("reg");
        fword("property");
        fword("finish-device");
        sprintf(nodebuff, "/iommu/sbus/espdma/esp/sd@%d,0", id);
        REGISTER_NODE_METHODS(ob_sd, nodebuff);
        if (drive_cdrom(id)) {
            type = "cdrom";
            counter_ptr = &cdcount;
        } else {
            type = "disk";
            counter_ptr = &diskcount;
        }
        if (*counter_ptr == 0) {
            add_alias(nodebuff, type);
        }
        sprintf(aliasbuff, "%s%d", type, *counter_ptr);
        (*counter_ptr)++;
        add_alias(nodebuff, aliasbuff);
    }
#ifdef CONFIG_DEBUG_ESP
    printk("done\n");
#endif

    return 0;
}
