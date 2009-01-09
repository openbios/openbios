#ifndef _ASM_IO_H
#define _ASM_IO_H

#include "asm/types.h"

extern unsigned long va_shift; // Set in entry.S
// Defined in ldscript
extern char _start, _data, _stack, _estack, _end, _vmem, _evmem, _iomem;

static inline unsigned long
va2pa(unsigned long va)
{
    if ((va >= (unsigned long)&_data) &&
        (va < (unsigned long)&_end))
        return va - va_shift;
    else
        return va;
}

static inline unsigned long
pa2va(unsigned long pa)
{
    if ((pa + va_shift >= (unsigned long)&_data) &&
        (pa + va_shift< (unsigned long)&_end))
        return pa + va_shift;
    else
        return pa;
}

// XXX check use and merge
#define phys_to_virt(phys) ((void *) ((unsigned long) (phys)))
#define virt_to_phys(virt) ((unsigned long) (virt))

#ifndef BOOTSTRAP

extern unsigned long isa_io_base;

/*
 * The insw/outsw/insl/outsl macros don't do byte-swapping.
 * They are only used in practice for transferring buffers which
 * are arrays of bytes, and byte-swapping is not appropriate in
 * that case.  - paulus
 */
#define insw(port, buf, ns)	_insw_ns((uint16_t *)((port)+isa_io_base), (buf), (ns))
#define outsw(port, buf, ns)	_outsw_ns((uint16_t *)((port)+isa_io_base), (buf), (ns))

#define inb(port)		in_8((uint8_t *)((port)+isa_io_base))
#define outb(val, port)		out_8((uint8_t *)((port)+isa_io_base), (val))
#define inw(port)		in_be16((uint16_t *)((port)+isa_io_base))
#define outw(val, port)		out_be16((uint16_t *)((port)+isa_io_base), (val))
#define inl(port)		in_be32((uint32_t *)((port)+isa_io_base))
#define outl(val, port)		out_be32((uint32_t *)((port)+isa_io_base), (val))

/*
 * 8, 16 and 32 bit, big and little endian I/O operations, with barrier.
 */
static inline int in_8(volatile unsigned char *addr)
{
    int ret;

    __asm__ __volatile__("lduba [%1] 0x15, %0\n\t"
                         :"=r"(ret):"r"(addr):"memory");

    return ret;
}

static inline void out_8(volatile unsigned char *addr, int val)
{
    __asm__ __volatile__("stba %0, [%1] 0x15\n\t"
                         : : "r"(val), "r"(addr):"memory");
}

static inline int in_le16(volatile unsigned short *addr)
{
    int ret, tmp;

    // XXX
    __asm__ __volatile__("lduha [%1] 0x15, %0\n\t"
                         :"=r"(ret):"r"(addr):"memory");

    tmp = (ret << 8) & 0xff00;
    tmp |= (ret >> 8) & 0xff;
    return tmp;
}

static inline int in_be16(volatile unsigned short *addr)
{
    int ret;

    __asm__ __volatile__("lduha [%1] 0x15, %0\n\t"
                         :"=r"(ret):"r"(addr):"memory");

    return ret;
}

static inline void out_le16(volatile unsigned short *addr, int val)
{
    unsigned tmp;

    // XXX
    tmp = (val << 8) & 0xff00;
    tmp |= (val >> 8) & 0xff;
    __asm__ __volatile__("stha %0, [%1] 0x15\n\t"
                         : : "r"(tmp), "r"(addr):"memory");
}

static inline void out_be16(volatile unsigned short *addr, int val)
{
    __asm__ __volatile__("stha %0, [%1] 0x15\n\t"
                         : : "r"(val), "r"(addr):"memory");
}

static inline unsigned in_le32(volatile unsigned *addr)
{
    unsigned ret, tmp;

    // XXX
    __asm__ __volatile__("lduwa [%1] 0x15, %0\n\t"
                         :"=r"(ret):"r"(addr):"memory");

    tmp = ret << 24;
    tmp |= (ret << 8) & 0xff0000;
    tmp |= (ret >> 8) & 0xff00;
    tmp |= (ret >> 24) & 0xff;
    return tmp;
}

static inline unsigned in_be32(volatile unsigned *addr)
{
    unsigned ret;

    __asm__ __volatile__("lduwa [%1] 0x15, %0\n\t"
                         :"=r"(ret):"r"(addr):"memory");

    return ret;
}

static inline void out_le32(volatile unsigned *addr, int val)
{
    unsigned tmp;
    // XXX
    tmp = val << 24;
    tmp |= (val << 8) & 0xff0000;
    tmp |= (val >> 8) & 0xff00;
    tmp |= (val >> 24) & 0xff;
    __asm__ __volatile__("stwa %0, [%1] 0x15\n\t"
                         : : "r"(tmp), "r"(addr):"memory");
}

static inline void out_be32(volatile unsigned *addr, int val)
{
    __asm__ __volatile__("stwa %0, [%1] 0x15\n\t"
                         : : "r"(val), "r"(addr):"memory");
}

static inline void _insw_ns(volatile uint16_t * port, void *buf, int ns)
{
	uint16_t *b = (uint16_t *) buf;

	while (ns > 0) {
		*b++ = in_le16(port);
		ns--;
	}
}

static inline void _outsw_ns(volatile uint16_t * port, const void *buf,
			     int ns)
{
	uint16_t *b = (uint16_t *) buf;

	while (ns > 0) {
		out_le16(port, *b++);
		ns--;
	}
}

static inline void _insw(volatile uint16_t * port, void *buf, int ns)
{
	uint16_t *b = (uint16_t *) buf;

	while (ns > 0) {
		*b++ = in_be16(port);
		ns--;
	}
}

static inline void _outsw(volatile uint16_t * port, const void *buf,
			  int ns)
{
	uint16_t *b = (uint16_t *) buf;

	while (ns > 0) {
		out_be16(port, *b++);
		ns--;
	}
}
#else /* BOOTSTRAP */
#ifdef FCOMPILER
#define inb(reg) ((u8)0xff)
#define inw(reg) ((u16)0xffff)
#define inl(reg) ((u32)0xffffffff)
#define outb(reg, val) do{} while(0)
#define outw(reg, val) do{} while(0)
#define outl(reg, val) do{} while(0)
#else
extern u8 inb(u32 reg);
extern u16 inw(u32 reg);
extern u32 inl(u32 reg);
extern void insw(u32 reg, void *addr, unsigned long count);
extern void outb(u32 reg, u8 val);
extern void outw(u32 reg, u16 val);
extern void outl(u32 reg, u32 val);
extern void outsw(u32 reg, const void *addr, unsigned long count);
#endif
#endif
#endif /* _ASM_IO_H */
