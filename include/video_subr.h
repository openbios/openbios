#ifndef VIDEO_SUBR_H
#define VIDEO_SUBR_H

/* drivers/vga_load_regs.c */
void vga_load_regs(void);

/* drivers/vga_set_mode.c */
void vga_set_gmode (void);
void vga_set_amode (void);
void vga_font_load(unsigned char *vidmem, const unsigned char *font, int height, int num_chars);

/* drivers/vga_vbe.c */
void vga_set_color(int i, unsigned int r, unsigned int g, unsigned int b);
void vga_vbe_set_mode(int width, int height, int depth);
void vga_vbe_init(const char *path, unsigned long fb, uint32_t fb_size,
                  unsigned long rom, uint32_t rom_size);

/* packages/video.c */
int video_get_res(int *w, int *h);
void draw_pixel(int x, int y, int colind);
void set_color(int ind, unsigned long color);
void video_scroll(int height);
void init_video(unsigned long fb, int width, int height, int depth, int rb);

/* libopenbios/console_common.c */
int console_draw_str(const char *str);
int console_init(void);
void console_close(void);
void cls(void);

extern volatile uint32_t *dac;

#endif /* VIDEO_SUBR_H */
