

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define u8  unsigned char
#define u16 unsigned short
#define u32 unsigned int

#define PCI_DATA_HDR (u32) ( ('R' << 24) | ('I' << 16) | ('C' << 8) | 'P' )

char *rom=NULL;
size_t romlen=0;

typedef struct {
	u16	signature;
	u8	reserved[0x16];
	u16	dptr;
} rom_header_t;

typedef struct {
	u32	signature;
	u16	vendor;
	u16	device;
	u16	reserved_1;
	u16	dlen;
	u8	drevision;
	u8	class_hi;
	u16	class_lo;
	u16	ilen;
	u16	irevision;
	u8	type;
	u8	indicator;
	u16	reserved_2;
} pci_data_t;


/* make this endian safe without fancy system headers */
static u16 little_word(u16 val)
{
	u8 *ptr=(u8 *)&val;
	return (ptr[0]|(ptr[1]<<8));
}

static u16 little_dword(u16 val)
{
	u8 *ptr=(u8 *)&val;
	return (ptr[0]|(ptr[1]<<8)|(ptr[2]<<16)|(ptr[3]<<24));
}

/* dump the rom headers */
static int dump_rom_header(rom_header_t *data)
{
	u16 sig=little_word(data->signature);
	int i;
	
	printf ("PCI Expansion ROM Header:\n");
	
	printf ("  Signature: 0x%02x%02x (%s)\n", 
			sig&0xff,sig>>8,sig==0xaa55?"Ok":"Not Ok");
	
	printf ("  CPU unique data:");
	for (i=0;i<16;i++) {
		printf(" 0x%02x",data->reserved[i]);
		if (i==7) printf("\n                  ");
	}
	
	printf ("\n  Pointer to PCI Data Structure: 0x%04x\n\n",
						little_word(data->dptr));

	return (sig==0xaa55);
}

static int dump_pci_data(pci_data_t *data)
{
	u32 sig=little_dword(data->signature);
	u32 classcode=(data->class_hi<<16)|(little_word(data->class_lo));
	
	printf("PCI Data Structure:\n");
	printf("  Signature: '%c%c%c%c' (%s)\n", sig&0xff,(sig>>8)&0xff,
			(sig>>16)&0xff, sig>>24, sig==PCI_DATA_HDR?"Ok":"Not Ok");
	printf("  Vendor ID: 0x%04x\n", little_word(data->vendor));
	printf("  Device ID: 0x%04x\n", little_word(data->device));
	printf("  Reserved: 0x%04x\n", little_word(data->reserved_1));
	printf("  PCI Data Structure Length: 0x%04x (%d bytes)\n", 
			little_word(data->dlen), little_word(data->dlen));
	printf("  PCI Data Structure Revision: 0x%02x\n", data->drevision);
	printf("  Class Code: 0x%06x (",classcode);
	switch (classcode) {
	case 0x0100:
		printf("SCSI Storage");
		break;
	case 0x0101:
		printf("IDE Storage");
		break;
	case 0x0103:
		printf("IPI Storage");
		break;
	case 0x0104:
		printf("RAID Storage");
		break;
	case 0x0180:
		printf("Storage");
		break;
		
	case 0x0200:
		printf("Ethernet");
		break;
	case 0x0201:
		printf("Token Ring");
		break;
	case 0x0202:
		printf("FDDI");
		break;
	case 0x0203:
		printf("ATM");
		break;
	case 0x0280:
		printf("Network");
		
	case 0x0300:
		printf("VGA Display");
		break;
	case 0x0301:
		printf("XGA Display");
		break;
	case 0x0302:
		printf("3D Display");
		break;
	case 0x0380:
		printf("Display");
		break;
		
	default:
		printf("unkown");
	}
	printf(")\n  Image Length: 0x%04x blocks (%d bytes)\n", 
			little_word(data->ilen), little_word(data->ilen)*512);
	printf("  Revision Level of Code/Data: 0x%04x\n",
			little_word(data->irevision));
	printf("  Code Type: 0x%02x (", data->type);
	switch (data->type) {
	case 0:
		printf("Intel x86");
		break;
	case 1: 
		printf("Open Firmware");
		break;
	case 2:
		printf("HP PA Risc");
		break;
	case 3:
		printf("Intel EFI (unofficial)");
		break;
	default:
		printf("unknown as of PCI specs 2.2");
	}
	printf(")\n  Indicator: 0x%02x %s\n", data->indicator,
			data->indicator&0x80?"(last image in rom)":"");
	printf("  Reserved: 0x%04x\n\n", little_word(data->reserved_2));

	return (sig==PCI_DATA_HDR);
}

static void dump_platform_extensions(u8 type, rom_header_t *data)
{
	u32 entry;
	
	switch (type) {
	case 0x00:
		printf("Platform specific data for x86 compliant option rom:\n");
		printf("  Initialization Size: 0x%02x (%d bytes)\n",
				data->reserved[0], data->reserved[0]*512);

		/* We do a hack here - implement a jump disasm to be able
		 * to output correct offset for init code. Once again x86
		 * is ugly.
		 */
		
		switch (data->reserved[1]) {
		case 0xeb: /* short jump */
			entry = data->reserved[2] + 2;
			/* a short jump instruction is 2 bytes,
			 * we have to add those to the offset
			 */
			break;
		case 0xe9: /* jump */
			entry = ((data->reserved[3]<<8)|data->reserved[2]) + 3;
			/* jump is 3 bytes, so add them */
			break;
		default:
			entry=0;
			break;
		}

		if (entry) {
			/* 0x55aa rom signature plus 1 byte len */
			entry += 3;
			printf( "  Entry point for INIT function:"
				" 0x%x\n\n",entry);
		} else
			printf( "  Unable to determin entry point for INIT"
				" function. Please report.\n\n");
		
		break;
	case 0x01:
		printf("Platform specific data for Open Firmware compliant rom:\n");
		printf("  Pointer to FCode program: 0x%04x\n\n",
				data->reserved[1]<<8|data->reserved[0]);
		break;
	default:
		printf("Parsing of platform specific data not available for this image\n\n");
	}
}

int main(int argc, char **argv)
{
	char		*name=argv[1];
	FILE		*romfile;
	struct stat 	finfo;

	rom_header_t	*rom_header;
	pci_data_t	*pci_data;

	int i=1;

	if (argc!=2) {
		printf ("\nUsage: %s <romimage.img>\n",argv[0]);
		printf ("\nromheaders dumps pci option rom headers "
				"according to PCI \n"
				"specs 2.2 in human readable form\n\n");
		return -1;
	}
	
	if (stat(name,&finfo)) {
		printf("Error while reading file information.\n");
		return -1;
	}

	romlen=finfo.st_size;

	rom=malloc(romlen);
	if (!rom) {
		printf("Out of memory.\n");
		return -1;
	}

        romfile=fopen(name,"r");
        if (!romfile) {
		printf("Error while opening file\n");
		return -1;
	}

	if (fread(rom, romlen, 1, romfile)!=1) {
		printf("Error while reading file\n");
		free(rom);
		return -1;
	}

	fclose(romfile);
	
	rom_header=(rom_header_t *)rom;

	do {
		printf("\nImage %d:\n",i);
		if (!dump_rom_header(rom_header)) {
			printf("Error occured. Bailing out.\n");
			break;
		}
		
		pci_data=(pci_data_t *)(rom+little_word(rom_header->dptr));
		
		if (!dump_pci_data(pci_data)) {
			printf("Error occured. Bailing out.\n");
			break;
		}
		
		dump_platform_extensions(pci_data->type, rom_header);
		
		rom_header+=little_word(pci_data->ilen)*512;
		i++;
	} while ((pci_data->indicator&0x80)!=0x80 &&
			romlen<(unsigned long)rom_header-(unsigned long)romlen);

	return 0;
}
	
	
