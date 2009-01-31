/*
 * tag: dict management
 *
 * Copyright (C) 2003-2005 Stefan Reinauer, Patrick Mauritz
 *
 * See the file "COPYING" for further information about
 * the copyright and warranty status of this work.
 */

#include "openbios/config.h"
#include "openbios/kernel.h"
#include "dict.h"
#ifdef BOOTSTRAP
#include <string.h>
#else
#include "libc/string.h"
#endif
#include "cross.h"


unsigned char *dict = NULL;
ucell *last;
cell dicthead = 0;

/* lfa2nfa
 * converts a link field address to a name field address,
 * i.e find pointer to a given words name
 */

ucell lfa2nfa(ucell ilfa)
{
	/* get offset from dictionary start */
	ilfa = ilfa - (ucell)pointer2cell(dict);
	ilfa--;				/* skip status        */
	while (dict[--ilfa] == 0);	/* skip all pad bytes */
	ilfa -= (dict[ilfa] - 128);
	return ilfa + (ucell)pointer2cell(dict);
}

/* lfa2cfa
 * converts a link field address to a code field address.
 * in this forth implementation this is just a fixed offset
 */

static xt_t lfa2cfa(ucell ilfa)
{
	return (xt_t)(ilfa + sizeof(cell));
}


/* fstrlen - returns length of a forth string. */

static ucell fstrlen(ucell fstr)
{
	fstr -= pointer2cell(dict)+1;
	//fstr -= pointer2cell(dict); FIXME
	while (dict[++fstr] < 128)
		;
	return dict[fstr] - 128;
}

/* to_lower - convert a character to lowecase */

static int to_lower(int c)
{
	return ((c >= 'A') && (c <= 'Z')) ? (c - 'A' + 'a') : c;
}

/* fstrcmp - compare null terminated string with forth string. */

static int fstrcmp(const char *s1, ucell fstr)
{
	char *s2 = (char*)cell2pointer(fstr);
	while (*s1) {
		if ( to_lower(*(s1++)) != to_lower(*(s2++)) )
			return -1;
	}
	return 0;
}

/* findword
 * looks up a given word in the dictionary. This function
 * is used by the c based interpreter and to find the "initialize"
 * word.
 */

xt_t findword(const char *s1)
{
	ucell tmplfa, len;

	if (!last)
		return 0;

	tmplfa = read_ucell(last);

	len = strlen(s1);

	while (tmplfa) {
		ucell nfa = lfa2nfa(tmplfa);

		if (len == fstrlen(nfa) && !fstrcmp(s1, nfa)) {
			return lfa2cfa(tmplfa);
		}

		tmplfa = read_ucell(cell2pointer(tmplfa));
	}

	return 0;
}


void dump_header(dictionary_header_t *header)
{
	printk("OpenBIOS dictionary:\n");
	printk("  version:     %d\n", header->version);
	printk("  cellsize:    %d\n", header->cellsize);
	printk("  endianess:   %s\n", header->endianess?"big":"little");
	printk("  compression: %s\n", header->compression?"yes":"no");
	printk("  relocation:  %s\n", header->relocation?"yes":"no");
	printk("  checksum:    %08x\n", target_long(header->checksum));
	printk("  length:      %08x\n", target_long(header->length));
	printk("  last:        %0" FMT_CELL_x "\n", target_cell(header->last));
}

ucell load_dictionary(const char *data, ucell len)
{
	u32 checksum=0;
	const char *checksum_walk;
	ucell *walk, *reloc_table;
	dictionary_header_t *header=(dictionary_header_t *)data;

	/* assertions */
	if (len <= (sizeof(dictionary_header_t)) || strncmp(DICTID, data, 8))
		return 0;
#ifdef CONFIG_DEBUG_DICTIONARY
	dump_header(header);
#endif

	checksum_walk=data;
	while (checksum_walk<data+len) {
		checksum+=read_long(checksum_walk);
		checksum_walk+=sizeof(u32);
	}

	if(checksum) {
		printk("Checksum invalid (%08x)!\n", checksum);
		return 0;
	}

	data += sizeof(dictionary_header_t);
	len -= sizeof(dictionary_header_t);

	dicthead = target_long(header->length);

	memcpy(dict, data, dicthead);
	reloc_table=(ucell *)(data+dicthead);

#ifdef CONFIG_DEBUG_DICTIONARY
	printk("\nmoving dictionary (%x bytes) to %x\n",
			(ucell)dicthead, (ucell)dict);
	printk("\ndynamic relocation...");
#endif

	for (walk = (ucell *) dict; walk < (ucell *) (dict + dicthead);
	     walk++) {
		int pos, bit, l;
		l=(walk-(ucell *)dict);
		pos=l/BITS;
		bit=l&~(-BITS);
                if (reloc_table[pos] & target_ucell((ucell)1ULL << bit)) {
			// printk("%lx, pos %x, bit %d\n",*walk, pos, bit);
			write_ucell(walk, read_ucell(walk)+pointer2cell(dict));
		}
	}

#ifdef CONFIG_DEBUG_DICTIONARY
	printk(" done.\n");
#endif

	last = (ucell *)(dict + target_ucell(header->last));

	return -1;
}
