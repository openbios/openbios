
void init_video(unsigned long fb, int width, int height, int depth, int rb);
unsigned long get_color(int col_ind);
void set_color(int ind, unsigned long color);
void refresh_palette(void);
int video_get_res(int *w, int *h);
void draw_pixel(int x, int y, int colind);
void video_scroll(int height);
void fill_rect(int col_ind, int x, int y, int w, int h);
void video_mask_blit(void);

typedef struct osi_fb_info {
    unsigned long mphys;
    unsigned long mvirt;
    int rb, w, h, depth;
} osi_fb_info_t;

extern struct video_info {
    int has_video;
    osi_fb_info_t fb;
    unsigned long *pal;    /* 256 elements */
} video;
