#include "openbios/config.h"
#include "openbios/bindings.h"
#include "libc/byteorder.h"
#include "libc/vsprintf.h"

#include "adb.h"


#include "cuda.h"
//#define DEBUG_CUDA
#ifdef DEBUG_CUDA
#define CUDA_DPRINTF(fmt, args...) \
	do { printk("CUDA - %s: " fmt, __func__ , ##args); } while (0)
#else
#define CUDA_DPRINTF(fmt, args...) do { } while (0)
#endif
#define ADB_DPRINTF CUDA_DPRINTF

#define IO_CUDA_OFFSET	0x00016000
#define IO_CUDA_SIZE	0x00002000

/* VIA registers - spaced 0x200 bytes apart */
#define RS              0x200           /* skip between registers */
#define B               0               /* B-side data */
#define A               RS              /* A-side data */
#define DIRB            (2*RS)          /* B-side direction (1=output) */
#define DIRA            (3*RS)          /* A-side direction (1=output) */
#define T1CL            (4*RS)          /* Timer 1 ctr/latch (low 8 bits) */
#define T1CH            (5*RS)          /* Timer 1 counter (high 8 bits) */
#define T1LL            (6*RS)          /* Timer 1 latch (low 8 bits) */
#define T1LH            (7*RS)          /* Timer 1 latch (high 8 bits) */
#define T2CL            (8*RS)          /* Timer 2 ctr/latch (low 8 bits) */
#define T2CH            (9*RS)          /* Timer 2 counter (high 8 bits) */
#define SR              (10*RS)         /* Shift register */
#define ACR             (11*RS)         /* Auxiliary control register */
#define PCR             (12*RS)         /* Peripheral control register */
#define IFR             (13*RS)         /* Interrupt flag register */
#define IER             (14*RS)         /* Interrupt enable register */
#define ANH             (15*RS)         /* A-side data, no handshake */

/* Bits in B data register: all active low */
#define TREQ            0x08            /* Transfer request (input) */
#define TACK            0x10            /* Transfer acknowledge (output) */
#define TIP             0x20            /* Transfer in progress (output) */

/* Bits in ACR */
#define SR_CTRL         0x1c            /* Shift register control bits */
#define SR_EXT          0x0c            /* Shift on external clock */
#define SR_OUT          0x10            /* Shift out if 1 */

/* Bits in IFR and IER */
#define IER_SET         0x80            /* set bits in IER */
#define IER_CLR         0               /* clear bits in IER */
#define SR_INT          0x04            /* Shift register full/empty */

#define CUDA_BUF_SIZE 16

#define ADB_PACKET      0
#define CUDA_PACKET     1

static uint8_t cuda_readb (cuda_t *dev, int reg)
{
	    return *(volatile uint8_t *)(dev->base + reg);
}

static void cuda_writeb (cuda_t *dev, int reg, uint8_t val)
{
	    *(volatile uint8_t *)(dev->base + reg) = val;
}

static void cuda_wait_irq (cuda_t *dev)
{
    int val;

//    CUDA_DPRINTF("\n");
    for(;;) {
        val = cuda_readb(dev, IFR);
        cuda_writeb(dev, IFR, val & 0x7f);
        if (val & SR_INT)
            break;
    }
}



static int cuda_request (cuda_t *dev, uint8_t pkt_type, const uint8_t *buf,
                         int buf_len, uint8_t *obuf)
{
    int i, obuf_len, val;

    cuda_writeb(dev, ACR, cuda_readb(dev, ACR) | SR_OUT);
    cuda_writeb(dev, SR, pkt_type);
    cuda_writeb(dev, B, cuda_readb(dev, B) & ~TIP);
    if (buf) {
        //CUDA_DPRINTF("Send buf len: %d\n", buf_len);
        /* send 'buf' */
        for(i = 0; i < buf_len; i++) {
            cuda_wait_irq(dev);
            cuda_writeb(dev, SR, buf[i]);
            cuda_writeb(dev, B, cuda_readb(dev, B) ^ TACK);
        }
    }
    cuda_wait_irq(dev);
    cuda_writeb(dev, ACR, cuda_readb(dev, ACR) & ~SR_OUT);
    cuda_readb(dev, SR);
    cuda_writeb(dev, B, cuda_readb(dev, B) | TIP | TACK);

    obuf_len = 0;
    if (obuf) {
        cuda_wait_irq(dev);
        cuda_readb(dev, SR);
        cuda_writeb(dev, B, cuda_readb(dev, B) & ~TIP);
        for(;;) {
            cuda_wait_irq(dev);
            val = cuda_readb(dev, SR);
            if (obuf_len < CUDA_BUF_SIZE)
                obuf[obuf_len++] = val;
            if (cuda_readb(dev, B) & TREQ)
                break;
            cuda_writeb(dev, B, cuda_readb(dev, B) ^ TACK);
        }
        cuda_writeb(dev, B, cuda_readb(dev, B) | TIP | TACK);

        cuda_wait_irq(dev);
        cuda_readb(dev, SR);
    }
//    CUDA_DPRINTF("Got len: %d\n", obuf_len);

    return obuf_len;
}



static int cuda_adb_req (void *host, const uint8_t *snd_buf, int len,
                         uint8_t *rcv_buf)
{
    uint8_t buffer[CUDA_BUF_SIZE], *pos;

 //   CUDA_DPRINTF("len: %d %02x\n", len, snd_buf[0]);
    len = cuda_request(host, ADB_PACKET, snd_buf, len, buffer);
    if (len > 1 && buffer[0] == ADB_PACKET) {
        pos = buffer + 2;
        len -= 2;
    } else {
        pos = buffer + 1;
        len = -1;
    }
    memcpy(rcv_buf, pos, len);

    return len;
}


DECLARE_UNNAMED_NODE(ob_cuda, INSTALL_OPEN, sizeof(int));

static void
ob_cuda_initialize (int *idx)
{
	extern phandle_t pic_handle;
	phandle_t ph=get_cur_dev();
	int props[2];

	push_str("via-cuda");
	fword("device-type");

	set_int_property(ph, "#address-cells", 1);
        set_int_property(ph, "#size-cells", 0);

	set_property(ph, "compatible", "cuda", 5);

	props[0] = __cpu_to_be32(IO_CUDA_OFFSET);
	props[1] = __cpu_to_be32(IO_CUDA_SIZE);

	set_property(ph, "reg", (char *)&props, sizeof(props));
	set_int_property(ph, "interrupt-parent", pic_handle);
	// HEATHROW
	set_int_property(ph, "interrupts", 0x12);
}

static void
ob_cuda_open(int *idx)
{
	RET(-1);
}

static void
ob_cuda_close(int *idx)
{
}

static void
ob_cuda_decode_unit(void *private)
{
	PUSH(0);
	fword("decode-unit-pci-bus");
}

static void
ob_cuda_encode_unit(void *private)
{
	fword("encode-unit-pci");
}

NODE_METHODS(ob_cuda) = {
	{ NULL,			ob_cuda_initialize	},
	{ "open",		ob_cuda_open		},
	{ "close",		ob_cuda_close		},
	{ "decode-unit",	ob_cuda_decode_unit	},
	{ "encode-unit",	ob_cuda_encode_unit	},
};

DECLARE_UNNAMED_NODE(rtc, INSTALL_OPEN, sizeof(int));

static void
rtc_open(int *idx)
{
	RET(-1);
}

NODE_METHODS(rtc) = {
	{ "open",		rtc_open		},
};

static void
rtc_init(char *path)
{
	phandle_t ph, aliases;
	char buf[64];

        snprintf(buf, sizeof(buf), "%s/rtc", path);
	REGISTER_NAMED_NODE(rtc, buf);

	ph = find_dev(buf);
	set_property(ph, "device_type", "rtc", 4);
	set_property(ph, "compatible", "rtc", 4);

	aliases = find_dev("/aliases");
	set_property(aliases, "rtc", buf, strlen(buf) + 1);

}

cuda_t *cuda_init (char *path, uint32_t base)
{
    cuda_t *cuda;
    char buf[64];

    base += IO_CUDA_OFFSET;
    CUDA_DPRINTF(" base=%08x\n", base);
    cuda = malloc(sizeof(cuda_t));
    if (cuda == NULL)
        return NULL;

    snprintf(buf, sizeof(buf), "%s/via-cuda", path);
    REGISTER_NAMED_NODE(ob_cuda, buf);

    cuda->base = base;
    cuda_writeb(cuda, B, cuda_readb(cuda, B) | TREQ | TIP);
#ifdef CONFIG_DRIVER_ADB
    cuda->adb_bus = adb_bus_new(cuda, &cuda_adb_req);
    if (cuda->adb_bus == NULL) {
        free(cuda);
        return NULL;
    }
    adb_bus_init(buf, cuda->adb_bus);
#endif

    rtc_init(buf);

    return cuda;
}

#ifdef CONFIG_DRIVER_ADB

DECLARE_UNNAMED_NODE( adb, INSTALL_OPEN, sizeof(int));

static void
adb_initialize (int *idx)
{
	phandle_t ph=get_cur_dev();

	push_str("adb");
	fword("device-type");

	set_property(ph, "compatible", "adb", 4);
	set_int_property(ph, "#address-cells", 1);
	set_int_property(ph, "#size-cells", 0);
}

static void
adb_open(int *idx)
{
	RET(-1);
}

static void
adb_close(int *idx)
{
}

NODE_METHODS( adb ) = {
	{ NULL,			adb_initialize		},
	{ "open",		adb_open		},
	{ "close",		adb_close		},
};

adb_bus_t *adb_bus_new (void *host,
                        int (*req)(void *host, const uint8_t *snd_buf,
                                   int len, uint8_t *rcv_buf))
{
    adb_bus_t *new;

    new = malloc(sizeof(adb_bus_t));
    if (new == NULL)
        return NULL;
    new->host = host;
    new->req = req;

    return new;
}

/* Check and relocate all ADB devices as suggested in
 *  * ADB_manager Apple documentation
 *   */
int adb_bus_init (char *path, adb_bus_t *bus)
{
    char buf[64];
    uint8_t buffer[ADB_BUF_SIZE];
    uint8_t adb_addresses[16] =
        { 8, 9, 10, 11, 12, 13, 14, -1, -1, -1, -1, -1, -1, -1, 0, };
    adb_dev_t tmp_device, **cur;
    int address;
    int reloc = 0, next_free = 7;
    int keep;

    snprintf(buf, sizeof(buf), "%s/adb", path);
    REGISTER_NAMED_NODE( adb, buf);
    /* Reset the bus */
    // ADB_DPRINTF("\n");
    adb_reset(bus);
    cur = &bus->devices;
    memset(&tmp_device, 0, sizeof(adb_dev_t));
    tmp_device.bus = bus;
    for (address = 1; address < 8 && adb_addresses[reloc] > 0;) {
        if (address == ADB_RES) {
            /* Reserved */
            address++;
            continue;
        }
        //ADB_DPRINTF("Check device on ADB address %d\n", address);
        tmp_device.addr = address;
        switch (adb_reg_get(&tmp_device, 3, buffer)) {
        case 0:
            //ADB_DPRINTF("No device on ADB address %d\n", address);
            /* Register this address as free */
            if (adb_addresses[next_free] != 0)
                adb_addresses[next_free++] = address;
            /* Check next ADB address */
            address++;
            break;
        case 2:
           /* One device answered :
            * make it available and relocate it to a free address
            */
            if (buffer[0] == ADB_CHADDR) {
                /* device self test failed */
                ADB_DPRINTF("device on ADB address %d self-test failed "
                            "%02x %02x %02x\n", address,
                            buffer[0], buffer[1], buffer[2]);
                keep = 0;
            } else {
                //ADB_DPRINTF("device on ADB address %d self-test OK\n",
                //            address);
                keep = 1;
            }
            ADB_DPRINTF("Relocate device on ADB address %d to %d (%d)\n",
                        address, adb_addresses[reloc], reloc);
            buffer[0] = ((buffer[0] & 0x40) & ~0x90) | adb_addresses[reloc];
            if (keep == 1)
                buffer[0] |= 0x20;
            buffer[1] = ADB_CHADDR_NOCOLL;
            if (adb_reg_set(&tmp_device, 3, buffer, 2) < 0) {
                ADB_DPRINTF("ADB device relocation failed\n");
                return -1;
            }
            if (keep == 1) {
                *cur = malloc(sizeof(adb_dev_t));
                if (*cur == NULL) {
                    return -1;
                }
                (*cur)->type = address;
                (*cur)->bus = bus;
                (*cur)->addr = adb_addresses[reloc++];
                /* Flush buffers */
                adb_flush(*cur);
                switch ((*cur)->type) {
                case ADB_PROTECT:
                    ADB_DPRINTF("Found one protected device\n");
                    break;
                case ADB_KEYBD:
                    ADB_DPRINTF("Found one keyboard on address %d\n", address);
                    adb_kbd_new(buf, *cur);
                    break;
                case ADB_MOUSE:
                    ADB_DPRINTF("Found one mouse on address %d\n", address);
                    adb_mouse_new(buf, *cur);
                    break;
                case ADB_ABS:
                    ADB_DPRINTF("Found one absolute positioning device\n");
                    break;
                case ADB_MODEM:
                    ADB_DPRINTF("Found one modem\n");
                    break;
                case ADB_RES:
                    ADB_DPRINTF("Found one ADB res device\n");
                    break;
                case ADB_MISC:
                    ADB_DPRINTF("Found one ADB misc device\n");
                    break;
                }
                cur = &((*cur)->next);
            }
            break;
        case 1:
        case 3 ... 7:
            /* SHOULD NOT HAPPEN : register 3 is always two bytes long */
            ADB_DPRINTF("Invalid returned len for ADB register 3\n");
            return -1;
        case -1:
            /* ADB ERROR */
            ADB_DPRINTF("error gettting ADB register 3\n");
            return -1;
        }
    }

    return 0;
}
#endif
