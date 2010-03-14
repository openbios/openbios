/*
 *   Creation Date: <2004/08/28 18:38:22 greg>
 *   Time-stamp: <2004/08/28 18:38:22 greg>
 *
 *	<init.c>
 *
 *	Initialization for qemu
 *
 *   Copyright (C) 2004 Greg Watson
 *   Copyright (C) 2005 Stefan Reinauer
 *
 *   based on mol/init.c:
 *
 *   Copyright (C) 1999, 2000, 2001, 2002, 2003, 2004 Samuel & David Rydh
 *      (samuel@ibrium.se, dary@lindesign.se)
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation
 *
 */

#include "openbios/config.h"
#include "openbios/bindings.h"
#include "openbios/pci.h"
#include "openbios/nvram.h"
#include "openbios/drivers.h"
#include "qemu/qemu.h"
#include "ofmem.h"
#include "openbios-version.h"
#include "libc/byteorder.h"
#include "libc/vsprintf.h"
#define NO_QEMU_PROTOS
#include "openbios/fw_cfg.h"
#include "arch/ppc/processor.h"

#define UUID_FMT "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x"

struct cpudef {
    unsigned long iu_version;
    const char *name;
    int icache_size, dcache_size;
    int icache_sets, dcache_sets;
    int icache_block_size, dcache_block_size;
    int clock_frequency;
    void (*initfn)(const struct cpudef *cpu);
};

static uint16_t machine_id = 0;

extern void unexpected_excep( int vector );
extern void setup_timers( void );

void
unexpected_excep( int vector )
{
	printk("openbios panic: Unexpected exception %x\n", vector );
	for( ;; )
		;
}

enum {
    ARCH_PREP = 0,
    ARCH_MAC99,
    ARCH_HEATHROW,
    ARCH_MAC99_U3,
};

int is_apple(void)
{
	return 1;
}

int is_oldworld(void)
{
	return machine_id == ARCH_HEATHROW;
}

int is_newworld(void)
{
	return (machine_id == ARCH_MAC99) ||
               (machine_id == ARCH_MAC99_U3);
}

static const pci_arch_t known_arch[] = {
        [ARCH_PREP] = { "PREP", PCI_VENDOR_ID_MOTOROLA,
                        PCI_DEVICE_ID_MOTOROLA_RAVEN,
                        0x80800000, 0x800c0000,
			0x80000000, 0x00100000, 0xf0000000, 0x10000000,
			0x80000000, 0x00010000, 0x00000000, 0x00400000,
			{ 9, 11, 9, 11 }
		      },
        [ARCH_MAC99] = { "MAC99", PCI_VENDOR_ID_APPLE,
                         PCI_DEVICE_ID_APPLE_UNI_N_PCI,
                         0xf2800000, 0xf2c00000,
                         0xf2000000, 0x02000000, 0x80000000, 0x10000000,
                         0xf2000000, 0x00800000, 0x00000000, 0x01000000,
                         { 0x1b, 0x1c, 0x1d, 0x1e }
		       },
        [ARCH_MAC99_U3] = { "MAC99_U3", PCI_VENDOR_ID_APPLE,
                            PCI_DEVICE_ID_APPLE_U3_AGP,
                            0xf0800000, 0xf0c00000,
                            0xf0000000, 0x02000000, 0x80000000, 0x10000000,
                            0xf2000000, 0x00800000, 0x00000000, 0x01000000,
                            { 0x1b, 0x1c, 0x1d, 0x1e }
                          },
        [ARCH_HEATHROW] = { "HEATHROW", PCI_VENDOR_ID_MOTOROLA,
                            PCI_DEVICE_ID_MOTOROLA_MPC106,
                            0xfec00000, 0xfee00000,
			    0x80000000, 0x7f000000, 0x80000000, 0x01000000,
			    0xfe000000, 0x00800000, 0xfd000000, 0x01000000,
			    { 21, 22, 23, 24 }
			  },
};
uint32_t isa_io_base;

void
entry( void )
{
        uint32_t temp = 0;
        char buf[5];

        arch = &known_arch[ARCH_HEATHROW];

        fw_cfg_init();

        fw_cfg_read(FW_CFG_SIGNATURE, buf, 4);
        buf[4] = '\0';
        if (strncmp(buf, "QEMU", 4) == 0) {
            temp = fw_cfg_read_i32(FW_CFG_ID);
            if (temp == 1) {
                machine_id = fw_cfg_read_i16(FW_CFG_MACHINE_ID);
                arch = &known_arch[machine_id];
            }
        }

	isa_io_base = arch->io_base;

        if (temp != 1) {
            printk("Incompatible configuration device version, freezing\n");
            for(;;);
        }

	ofmem_init();
	initialize_forth();
	/* won't return */

	printk("of_startup returned!\n");
	for( ;; )
		;
}

static void
cpu_generic_init(const struct cpudef *cpu)
{
    unsigned long iu_version;

    push_str("/cpus");
    fword("find-device");

    fword("new-device");

    push_str(cpu->name);
    fword("device-name");

    push_str("cpu");
    fword("device-type");

    asm("mfpvr %0\n"
        : "=r"(iu_version) :);
    PUSH(iu_version);
    fword("encode-int");
    push_str("cpu-version");
    fword("property");

    PUSH(cpu->dcache_size);
    fword("encode-int");
    push_str("dcache-size");
    fword("property");

    PUSH(cpu->icache_size);
    fword("encode-int");
    push_str("icache-size");
    fword("property");

    PUSH(cpu->dcache_sets);
    fword("encode-int");
    push_str("dcache-sets");
    fword("property");

    PUSH(cpu->icache_sets);
    fword("encode-int");
    push_str("icache-sets");
    fword("property");

    PUSH(cpu->dcache_block_size);
    fword("encode-int");
    push_str("dcache-block-size");
    fword("property");

    PUSH(cpu->icache_block_size);
    fword("encode-int");
    push_str("icache-block-size");
    fword("property");

    PUSH(fw_cfg_read_i32(FW_CFG_PPC_TBFREQ));
    fword("encode-int");
    push_str("timebase-frequency");
    fword("property");

    PUSH(fw_cfg_read_i32(FW_CFG_PPC_CPUFREQ));
    fword("encode-int");
    push_str("clock-frequency");
    fword("property");

    push_str("running");
    fword("encode-string");
    push_str("state");
    fword("property");
}

static void
cpu_add_pir_property(void)
{
    unsigned long pir;

    asm("mfspr %0, 1023\n"
        : "=r"(pir) :);
    PUSH(pir);
    fword("encode-int");
    push_str("reg");
    fword("property");
}

static void
cpu_604_init(const struct cpudef *cpu)
{
    cpu_generic_init(cpu);
    cpu_add_pir_property();

    fword("finish-device");
}

static void
cpu_750_init(const struct cpudef *cpu)
{
    cpu_generic_init(cpu);

    PUSH(0);
    fword("encode-int");
    push_str("reg");
    fword("property");

    fword("finish-device");
}

static void
cpu_g4_init(const struct cpudef *cpu)
{
    cpu_generic_init(cpu);
    cpu_add_pir_property();

    fword("finish-device");
}

/* In order to get 64 bit aware handlers that rescue all our
   GPRs from getting truncated to 32 bits, we need to patch the
   existing handlers so they jump to our 64 bit aware ones. */
static void
ppc64_patch_handlers(void)
{
    uint32_t *dsi = (uint32_t *)0x300UL;
    uint32_t *isi = (uint32_t *)0x400UL;

    // Patch the first DSI handler instruction to: ba 0x2000
    *dsi = 0x48002002;

    // Patch the first ISI handler instruction to: ba 0x2200
    *isi = 0x48002202;

    // Invalidate the cache lines
    asm ( "icbi 0, %0" : : "r"(dsi) );
    asm ( "icbi 0, %0" : : "r"(isi) );
}

static void
cpu_970_init(const struct cpudef *cpu)
{
    cpu_generic_init(cpu);

    PUSH(0);
    fword("encode-int");
    push_str("reg");
    fword("property");
    
    PUSH(0);
    PUSH(0);
    fword("encode-bytes");
    push_str("64-bit");
    fword("property");

    fword("finish-device");

    /* The 970 is a PPC64 CPU, so we need to activate
     * 64bit aware interrupt handlers */

    ppc64_patch_handlers();

    /* The 970 also implements the HIOR which we need to set to 0 */

    mtspr(S_HIOR, 0);
}

static const struct cpudef ppc_defs[] = {
    {
        .iu_version = 0x00040000,
        .name = "PowerPC,604",
        .icache_size = 0x4000,
        .dcache_size = 0x4000,
        .icache_sets = 0x80,
        .dcache_sets = 0x80,
        .icache_block_size = 0x20,
        .dcache_block_size = 0x20,
        .clock_frequency = 0x07de2900,
        .initfn = cpu_604_init,
    },
    { // XXX find out real values
        .iu_version = 0x00090000,
        .name = "PowerPC,604e",
        .icache_size = 0x4000,
        .dcache_size = 0x4000,
        .icache_sets = 0x80,
        .dcache_sets = 0x80,
        .icache_block_size = 0x20,
        .dcache_block_size = 0x20,
        .clock_frequency = 0x07de2900,
        .initfn = cpu_604_init,
    },
    { // XXX find out real values
        .iu_version = 0x000a0000,
        .name = "PowerPC,604r",
        .icache_size = 0x4000,
        .dcache_size = 0x4000,
        .icache_sets = 0x80,
        .dcache_sets = 0x80,
        .icache_block_size = 0x20,
        .dcache_block_size = 0x20,
        .clock_frequency = 0x07de2900,
        .initfn = cpu_604_init,
    },
    { // XXX find out real values
        .iu_version = 0x80040000,
        .name = "PowerPC,MPC86xx",
        .icache_size = 0x8000,
        .dcache_size = 0x8000,
        .icache_sets = 0x80,
        .dcache_sets = 0x80,
        .icache_block_size = 0x20,
        .dcache_block_size = 0x20,
        .clock_frequency = 0x14dc9380,
        .initfn = cpu_750_init,
    },
    {
        .iu_version = 0x000080000,
        .name = "PowerPC,750",
        .icache_size = 0x8000,
        .dcache_size = 0x8000,
        .icache_sets = 0x80,
        .dcache_sets = 0x80,
        .icache_block_size = 0x20,
        .dcache_block_size = 0x20,
        .clock_frequency = 0x14dc9380,
        .initfn = cpu_750_init,
    },
    { // XXX find out real values
        .iu_version = 0x10080000,
        .name = "PowerPC,750",
        .icache_size = 0x8000,
        .dcache_size = 0x8000,
        .icache_sets = 0x80,
        .dcache_sets = 0x80,
        .icache_block_size = 0x20,
        .dcache_block_size = 0x20,
        .clock_frequency = 0x14dc9380,
        .initfn = cpu_750_init,
    },
    { // XXX find out real values
        .iu_version = 0x70000000,
        .name = "PowerPC,750",
        .icache_size = 0x8000,
        .dcache_size = 0x8000,
        .icache_sets = 0x80,
        .dcache_sets = 0x80,
        .icache_block_size = 0x20,
        .dcache_block_size = 0x20,
        .clock_frequency = 0x14dc9380,
        .initfn = cpu_750_init,
    },
    { // XXX find out real values
        .iu_version = 0x70020000,
        .name = "PowerPC,750",
        .icache_size = 0x8000,
        .dcache_size = 0x8000,
        .icache_sets = 0x80,
        .dcache_sets = 0x80,
        .icache_block_size = 0x20,
        .dcache_block_size = 0x20,
        .clock_frequency = 0x14dc9380,
        .initfn = cpu_750_init,
    },
    { // XXX find out real values
        .iu_version = 0x800c0000,
        .name = "PowerPC,74xx",
        .icache_size = 0x8000,
        .dcache_size = 0x8000,
        .icache_sets = 0x80,
        .dcache_sets = 0x80,
        .icache_block_size = 0x20,
        .dcache_block_size = 0x20,
        .clock_frequency = 0x14dc9380,
        .initfn = cpu_750_init,
    },
    {
        .iu_version = 0x0000c0000,
        .name = "PowerPC,G4",
        .icache_size = 0x8000,
        .dcache_size = 0x8000,
        .icache_sets = 0x80,
        .dcache_sets = 0x80,
        .icache_block_size = 0x20,
        .dcache_block_size = 0x20,
        .clock_frequency = 0x1dcd6500,
        .initfn = cpu_g4_init,
    },
    { // XXX find out real values
        .iu_version = 0x003C0000,
        .name = "PowerPC,970FX",
        .icache_size = 0x10000,
        .dcache_size = 0x8000,
        .icache_sets = 0x80,
        .dcache_sets = 0x80,
        .icache_block_size = 0x80,
        .dcache_block_size = 0x80,
        .clock_frequency = 0x1dcd6500,
        .initfn = cpu_970_init,
    },
};

static const struct cpudef *
id_cpu(void)
{
    unsigned long iu_version;
    unsigned int i;

    asm("mfpvr %0\n"
        : "=r"(iu_version) :);
    iu_version &= 0xffff0000;

    for (i = 0; i < sizeof(ppc_defs)/sizeof(struct cpudef); i++) {
        if (iu_version == ppc_defs[i].iu_version)
            return &ppc_defs[i];
    }
    printk("Unknown cpu (pvr %lx), freezing!\n", iu_version);
    for (;;);
}

static void go( void );

static void
go( void )
{
    ucell addr;

    addr = POP();

    call_elf( 0, 0, addr);
}

void
arch_of_init( void )
{
#ifdef USE_RTAS
	phandle_t ph;
#endif
	uint64_t ram_size;
        const struct cpudef *cpu;
        char buf[64], qemu_uuid[16];
        const char *stdin_path, *stdout_path;
        uint32_t temp = 0;

    ofmem_t *ofmem = ofmem_arch_get_private();

        modules_init();
        setup_timers();
#ifdef CONFIG_DRIVER_PCI
        ob_pci_init();
#endif

        printk("\n");
        printk("=============================================================\n");
        printk(PROGRAM_NAME " " OPENBIOS_VERSION_STR " [%s]\n",
               OPENBIOS_BUILD_DATE);

        fw_cfg_read(FW_CFG_SIGNATURE, buf, 4);
        buf[4] = '\0';
        printk("Configuration device id %s", buf);

        temp = fw_cfg_read_i32(FW_CFG_ID);
        printk(" version %d machine id %d\n", temp, machine_id);

        temp = fw_cfg_read_i32(FW_CFG_NB_CPUS);

        printk("CPUs: %x\n", temp);

        ram_size = ofmem->ramsize;

        printk("Memory: %lldM\n", ram_size / 1024 / 1024);

        fw_cfg_read(FW_CFG_UUID, qemu_uuid, 16);

        printk("UUID: " UUID_FMT "\n", qemu_uuid[0], qemu_uuid[1], qemu_uuid[2],
               qemu_uuid[3], qemu_uuid[4], qemu_uuid[5], qemu_uuid[6],
               qemu_uuid[7], qemu_uuid[8], qemu_uuid[9], qemu_uuid[10],
               qemu_uuid[11], qemu_uuid[12], qemu_uuid[13], qemu_uuid[14],
               qemu_uuid[15]);

	/* set device tree root info */

	push_str("/");
	fword("find-device");

	switch(machine_id) {
	case ARCH_HEATHROW:	/* OldWord */

		/* model */

		push_str("Power Macintosh");
		fword("model");

		/* compatible */

		push_str("AAPL,PowerMac G3");
		fword("encode-string");
		push_str("MacRISC");
		fword("encode-string");
		fword("encode+");
		push_str("compatible");
		fword("property");

		/* misc */

		push_str("device-tree");
		fword("encode-string");
		push_str("AAPL,original-name");
		fword("property");

		PUSH(0);
		fword("encode-int");
		push_str("AAPL,cpu-id");
		fword("property");

		PUSH(66 * 1000 * 1000);
		fword("encode-int");
		push_str("clock-frequency");
		fword("property");
		break;

	case ARCH_MAC99:
	case ARCH_MAC99_U3:
	case ARCH_PREP:
	default:

		/* model */

		push_str("PowerMac2,1");
		fword("model");

		/* compatible */

		push_str("PowerMac2,1");
		fword("encode-string");
		push_str("MacRISC");
		fword("encode-string");
		fword("encode+");
		push_str("Power Macintosh");
		fword("encode-string");
		fword("encode+");
		push_str("compatible");
		fword("property");

		/* misc */

		push_str("bootrom");
		fword("device-type");

		PUSH(100 * 1000 * 1000);
		fword("encode-int");
		push_str("clock-frequency");
		fword("property");
		break;
	}

	/* Perhaps we can store UUID here ? */

	push_str("0000000000000");
	fword("encode-string");
	push_str("system-id");
	fword("property");

	/* memory info */

	push_str("/memory");
	fword("find-device");

	/* all memory */

	PUSH(ram_size >> 32);
	fword("encode-int");
	PUSH(ram_size & 0xffffffff);
	fword("encode-int");
	fword("encode+");
	PUSH(0);
	fword("encode-int");
	fword("encode+");
	PUSH(0);
	fword("encode-int");
	fword("encode+");
	push_str("reg");
	fword("property");

        cpu = id_cpu();
        cpu->initfn(cpu);
        printk("CPU type %s\n", cpu->name);

	snprintf(buf, sizeof(buf), "/cpus/%s", cpu->name);
	ofmem_register(find_dev("/memory"), find_dev(buf));
	node_methods_init(buf);

#ifdef USE_RTAS
	if( !(ph=find_dev("/rtas")) )
		printk("Warning: No /rtas node\n");
	else {
		ulong size = 0x1000;
		while( size < (ulong)of_rtas_end - (ulong)of_rtas_start )
			size *= 2;
		set_property( ph, "rtas-size", (char*)&size, sizeof(size) );
	}
#endif

        if (fw_cfg_read_i16(FW_CFG_NOGRAPHIC)) {
                if (CONFIG_SERIAL_PORT) {
                       stdin_path = "scca";
                       stdout_path = "scca";
                } else {
                       stdin_path = "sccb";
                       stdout_path = "sccb";
                }
        } else {
                stdin_path = "adb-keyboard";
                stdout_path = "screen";
        }

        push_str("/chosen");
        fword("find-device");

        push_str(stdin_path);
        fword("open-dev");
        fword("encode-int");
        push_str("stdin");
        fword("property");

        push_str(stdout_path);
        fword("open-dev");
        fword("encode-int");
        push_str("stdout");
        fword("property");

        push_str(stdin_path);
        fword("pathres-resolve-aliases");
        push_str("input-device");
        fword("$setenv");

        push_str(stdout_path);
        fword("pathres-resolve-aliases");
        push_str("output-device");
        fword("$setenv");

        push_str(stdin_path);
        fword("input");

        push_str(stdout_path);
        fword("output");

#if 0
	if( getbool("tty-interface?") == 1 )
#endif
		fword("activate-tty-interface");

	device_end();

	bind_func("platform-boot", boot );
	bind_func("(go)", go);
}
