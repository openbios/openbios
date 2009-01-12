
#define IO_ESCC_SIZE    0x00001000
#define IO_ESCC_OFFSET  0x00013000

#define ZS_REGS         8

void escc_init(const char *path, unsigned long addr);
void ob_zs_init(uint64_t base, uint64_t offset, int intr, int slave,
                int keyboard);
