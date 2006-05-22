#ifndef _ASM_IO_H
#define _ASM_IO_H

extern unsigned long virt_offset;

#define phys_to_virt(phys) ((void *) ((unsigned long) (phys) - virt_offset))
#define virt_to_phys(virt) ((unsigned long) (virt) + virt_offset)

#define __SLOW_DOWN_IO "outb %%al,$0x80;"
static inline void slow_down_io(void) 
{
	__asm__ __volatile__( 
		__SLOW_DOWN_IO
#ifdef REALLY_SLOW_IO
		__SLOW_DOWN_IO __SLOW_DOWN_IO __SLOW_DOWN_IO
#endif
		: : );
}

#define BUILDIO(bwl,bw,type) \
static inline void out##bwl(unsigned type value, int port) { \
	__asm__ __volatile__("out" #bwl " %" #bw "0, %w1" : : "a"(value), "Nd"(port)); \
} \
static inline unsigned type in##bwl(int port) { \
	unsigned type value; \
	__asm__ __volatile__("in" #bwl " %w1, %" #bw "0" : "=a"(value) : "Nd"(port)); \
	return value; \
} \
static inline void out##bwl##_p(unsigned type value, int port) { \
	out##bwl(value, port); \
	slow_down_io(); \
} \
static inline unsigned type in##bwl##_p(int port) { \
	unsigned type value = in##bwl(port); \
	slow_down_io(); \
	return value; \
} \
static inline void outs##bwl(int port, const void *addr, unsigned long count) { \
	__asm__ __volatile__("rep; outs" #bwl : "+S"(addr), "+c"(count) : "d"(port)); \
} \
static inline void ins##bwl(int port, void *addr, unsigned long count) { \
	__asm__ __volatile__("rep; ins" #bwl : "+D"(addr), "+c"(count) : "d"(port)); \
}

#ifndef BOOTSTRAP
BUILDIO(b,b,char)
BUILDIO(w,w,short)
BUILDIO(l,,int)
#endif

#endif
