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
#include "asm/io.h"
#include "scsi.h"
#include "asm/dma.h"
#include "esp.h"

#define PHYS_JJ_ESPDMA  0x78400000      /* ESP DMA controller */
#define PHYS_JJ_ESP     0x78800000      /* ESP SCSI */
#define BUFSIZE         4096

#define REGISTER_NAMED_NODE( name, path )   do {                        \
        bind_new_node( name##_flags_, name##_size_,                     \
                       path, name##_m, sizeof(name##_m)/sizeof(method_t)); \
    } while(0)

#define REGISTER_NODE_METHODS( name, path )   do {                      \
        const char *paths[1];                                                  \
                                                                        \
        paths[0] = path;                                                \
        bind_node( name##_flags_, name##_size_,                         \
                   paths, 1, name##_m, sizeof(name##_m)/sizeof(method_t)); \
    } while(0)

#ifdef CONFIG_DEBUG_ESP
#define DPRINTF(fmt, args...)                   \
    do { printk(fmt , ##args); } while (0)
#else
#define DPRINTF(fmt, args...)
#endif

struct esp_dma {
    volatile struct sparc_dma_registers *regs;
    enum dvma_rev revision;
};

typedef struct sd_private {
    unsigned int bs;
    char *media_str;
    uint32_t sectors;
    uint8_t media;
    uint8_t id;
    uint8_t present;
    char model[40];
} sd_private_t;

struct esp_regs {
    unsigned char regs[ESP_REG_SIZE];
};

typedef struct esp_private {
    volatile struct esp_regs *ll;
    uint32_t buffer_dvma;
    unsigned int irq;        /* device IRQ number    */
    struct esp_dma espdma;
    unsigned char *buffer;
    sd_private_t sd[8];
} esp_private_t;

esp_private_t *global_esp;

/* DECLARE data structures for the nodes.  */
DECLARE_UNNAMED_NODE(ob_sd, INSTALL_OPEN, sizeof(sd_private_t *));
DECLARE_UNNAMED_NODE(ob_esp, INSTALL_OPEN, sizeof(esp_private_t *));

#ifdef CONFIG_DEBUG_ESP
static void dump_drive(sd_private_t *drive)
{
    printk("SCSI DRIVE @%lx:\n", (unsigned long)drive);
    printk("id: %d\n", drive->id);
    printk("media: %s\n", drive->media_str);
    printk("model: %s\n", drive->model);
    printk("sectors: %d\n", drive->sectors);
    printk("present: %d\n", drive->present);
    printk("bs: %d\n", drive->bs);
}
#endif

static int
do_command(esp_private_t *esp, sd_private_t *sd, int cmdlen, int replylen)
{
    int status;

    // Set SCSI target
    esp->ll->regs[ESP_BUSID] = sd->id & 7;
    // Set DMA address
    esp->espdma.regs->st_addr = esp->buffer_dvma;
    // Set DMA length
    esp->ll->regs[ESP_TCLOW] = cmdlen & 0xff;
    esp->ll->regs[ESP_TCMED] = (cmdlen >> 8) & 0xff;
    // Set DMA direction
    esp->espdma.regs->cond_reg = 0;
    // Set ATN, issue command
    esp->ll->regs[ESP_CMD] = ESP_CMD_SELA | ESP_CMD_DMA;
    // Check status
    status = esp->ll->regs[ESP_STATUS];

    if ((status & ESP_STAT_TCNT) != ESP_STAT_TCNT)
        return status;
    
    // Get reply
    // Set DMA address
    esp->espdma.regs->st_addr = esp->buffer_dvma;
    // Set DMA length
    esp->ll->regs[ESP_TCLOW] = replylen & 0xff;
    esp->ll->regs[ESP_TCMED] = (replylen >> 8) & 0xff;
    // Set DMA direction
    esp->espdma.regs->cond_reg = DMA_ST_WRITE;
    // Transfer
    esp->ll->regs[ESP_CMD] = ESP_CMD_TI | ESP_CMD_DMA;
    // Check status
    status = esp->ll->regs[ESP_STATUS];

    if ((status & ESP_STAT_TCNT) != ESP_STAT_TCNT)
        return status;
    else
        return 0; // OK
}

// offset is multiple of 512, len in bytes
static int
ob_sd_read_sectors(esp_private_t *esp, sd_private_t *sd, int offset, void *dest, short len)
{
    DPRINTF("ob_sd_read_sectors id %d %lx block=%d len=%d\n", sd->id, (unsigned long)dest, offset, len);

    // Setup command = Read(10)
    memset(esp->buffer, 0, 10);
    esp->buffer[0] = 0x80;
    esp->buffer[1] = READ_10;

    esp->buffer[3] = (offset >> 24) & 0xff;
    esp->buffer[4] = (offset >> 16) & 0xff;
    esp->buffer[5] = (offset >> 8) & 0xff;
    esp->buffer[6] = offset & 0xff;

    esp->buffer[8] = (len >> 8) & 0xff;
    esp->buffer[9] = len & 0xff;

    if (do_command(esp, sd, 10, len * 512))
        return 0;

    memcpy(dest, esp->buffer, len * 512);

    return 0;
}

static unsigned int
read_capacity(esp_private_t *esp, sd_private_t *sd)
{
    // Setup command = Read Capacity
    memset(esp->buffer, 0, 11);
    esp->buffer[0] = 0x80;
    esp->buffer[1] = READ_CAPACITY;

    if (do_command(esp, sd, 11, 8)) {
        sd->sectors = 0;
        sd->bs = 0;

        return 0;
    }
    sd->sectors =  (esp->buffer[0] << 24) | (esp->buffer[1] << 16) | (esp->buffer[2] << 8) | esp->buffer[3];
#if 0
    sd->bs = (esp->buffer[4] << 24) | (esp->buffer[5] << 16) | (esp->buffer[6] << 8) | esp->buffer[7];
#else
    sd->bs = 512;
#endif

    return 1;
}

static unsigned int
inquiry(esp_private_t *esp, sd_private_t *sd)
{
    char *media = "UNKNOWN";

    // Setup command = Inquiry
    memset(esp->buffer, 0, 7);
    esp->buffer[0] = 0x80;
    esp->buffer[1] = INQUIRY;

    esp->buffer[4] = BUFSIZE & 0xff;
    esp->buffer[5] = (BUFSIZE >> 8) & 0xff;

    if (do_command(esp, sd, 7, 36)) {
        sd->present = 0;
        sd->media = -1;
        return 0;
    }
    sd->present = 1;
    sd->media = esp->buffer[0];
    
    switch (sd->media) {
    case TYPE_DISK:
        media = "disk";
        break;
    case TYPE_ROM:
        media = "cdrom";
        break;
    }
    sd->media_str = media;
    memcpy(sd->model, &esp->buffer[16], 16);
    sd->model[17] = '\0';

    return 1;
}


static void
ob_sd_read_blocks(sd_private_t **sd)
{
    cell n = POP(), cnt = n;
    ucell blk = POP();
    char *dest = (char*)POP();

    DPRINTF("ob_sd_read_blocks id %d %lx block=%d n=%d\n", (*sd)->id, (unsigned long)dest, blk, n );

    n *= (*sd)->bs / 512;
    while (n) {
        if (ob_sd_read_sectors(global_esp, *sd, blk, dest, 1)) {
            DPRINTF("ob_ide_read_blocks: error\n");
            RET(0);
        }
        dest += (*sd)->bs;
        n--;
        blk++;
    }
    PUSH(cnt);
}

static void
ob_sd_block_size(sd_private_t **sd)
{
    PUSH((*sd)->bs);
}

static void
ob_sd_open(__attribute__((unused))sd_private_t **sd)
{
    int ret = 1, id;
    phandle_t ph;

    fword("my-unit");
    id = POP();
    *sd = &global_esp->sd[id];

    DPRINTF("opening drive %d\n", id);

    selfword("open-deblocker");

    /* interpose disk-label */
    ph = find_dev("/packages/disk-label");
    fword("my-args");
    PUSH_ph( ph );
    fword("interpose");
    
    RET ( -ret );
}

static void
ob_sd_close(__attribute__((unused)) sd_private_t **sd)
{
    selfword("close-deblocker");
}

NODE_METHODS(ob_sd) = {
    { "open",           ob_sd_open },
    { "close",          ob_sd_close },
    { "read-blocks",    ob_sd_read_blocks },
    { "block-size",     ob_sd_block_size },
};


static int
espdma_init(struct esp_dma *espdma)
{
    void *p;

    /* Hardcode everything for MrCoffee. */
    if ((p = (void *)map_io(PHYS_JJ_ESPDMA, 0x10)) == 0) {
        DPRINTF("espdma_init: cannot map registers\n");
        return -1;
    }
    espdma->regs = p;

    DPRINTF("dma1: ");

    switch ((espdma->regs->cond_reg) & DMA_DEVICE_ID) {
    case DMA_VERS0:
        espdma->revision = dvmarev0;
        DPRINTF("Revision 0 ");
        break;
    case DMA_ESCV1:
        espdma->revision = dvmaesc1;
        DPRINTF("ESC Revision 1 ");
        break;
    case DMA_VERS1:
        espdma->revision = dvmarev1;
        DPRINTF("Revision 1 ");
        break;
    case DMA_VERS2:
        espdma->revision = dvmarev2;
        DPRINTF("Revision 2 ");
        break;
    case DMA_VERHME:
        espdma->revision = dvmahme;
        DPRINTF("HME DVMA gate array ");
        break;
    case DMA_VERSPLUS:
        espdma->revision = dvmarevplus;
        DPRINTF("Revision 1 PLUS ");
        break;
    default:
        DPRINTF("unknown dma version %x",
               (espdma->regs->cond_reg) & DMA_DEVICE_ID);
        /* espdma->allocated = 1; */
        break;
    }
    DPRINTF("\n");

    return 0;
}

static void
ob_esp_initialize(__attribute__((unused)) esp_private_t **esp)
{
    phandle_t ph = get_cur_dev();

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
}

static void
ob_esp_decodeunit(__attribute__((unused)) esp_private_t **esp)
{
    fword("decode-unit-scsi");
}


static void
ob_esp_encodeunit(__attribute__((unused)) esp_private_t **esp)
{
    fword("encode-unit-scsi");
}

NODE_METHODS(ob_esp) = {
    { NULL,             ob_esp_initialize },
    { "decode-unit",    ob_esp_decodeunit },
    { "encode-unit",    ob_esp_encodeunit },
};

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
    esp_private_t *esp;

    DPRINTF("Initializing SCSI...");

    esp = malloc(sizeof(esp_private_t));
    global_esp = esp;

    if (espdma_init(&esp->espdma) != 0) {
        return -1;
    }
    /* Get the IO region */
    esp->ll = (void *)map_io(PHYS_JJ_ESP, sizeof(struct esp_regs));
    if (esp->ll == 0) {
        DPRINTF("Can't map ESP registers\n");
        return -1;
    }

    esp->buffer = (void *)dvma_alloc(BUFSIZE, &esp->buffer_dvma);
    if (!esp->buffer || !esp->buffer_dvma) {
        DPRINTF("Can't get a DVMA buffer\n");
        return -1;
    }

    // Chip reset
    esp->ll->regs[ESP_CMD] = ESP_CMD_RC;

    DPRINTF("done\n");
    DPRINTF("Initializing SCSI devices...");

    for (id = 0; id < 8; id++) {
        esp->sd[id].id = id;
        if (!inquiry(esp, &esp->sd[id]))
            continue;
        read_capacity(esp, &esp->sd[id]);

#ifdef CONFIG_DEBUG_ESP
        dump_drive(&esp->sd[id]);
#endif
    }

    sprintf(nodebuff, "/iommu/sbus/espdma/esp");
    REGISTER_NAMED_NODE(ob_esp, nodebuff);
    device_end();

    for (id = 0; id < 8; id++) {
        if (!esp->sd[id].present)
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
        if (esp->sd[id].media == TYPE_ROM) {
            counter_ptr = &cdcount;
        } else {
            counter_ptr = &diskcount;
        }
        if (*counter_ptr == 0) {
            add_alias(nodebuff, esp->sd[id].media_str);
        }
        sprintf(aliasbuff, "%s%d", esp->sd[id].media_str, *counter_ptr);
        add_alias(nodebuff, aliasbuff);
        sprintf(aliasbuff, "sd(0,%d,0)", id);
        add_alias(nodebuff, aliasbuff);
        (*counter_ptr)++;
    }
    DPRINTF("done\n");

    return 0;
}
