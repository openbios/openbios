
void init_video(unsigned long fb, int width, int height, int depth, int rb);
unsigned long video_get_color(int col_ind);
void video_set_color(int ind, unsigned long color);
int video_get_res(int *w, int *h);
void video_mask_blit(void);
void video_invert_rect(void);
void video_fill_rect(void);

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
