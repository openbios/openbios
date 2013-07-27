
void init_video(unsigned long fb, int width, int height, int depth, int rb);
unsigned long video_get_color(int col_ind);
void video_set_color(int ind, unsigned long color);
int video_get_res(int *w, int *h);
void video_mask_blit(void);
void video_invert_rect(void);
void video_fill_rect(void);

extern struct video_info {
    int has_video;

    phys_addr_t mphys;
    unsigned long mvirt;
    int rb, w, h, depth;

    unsigned long *pal;    /* 256 elements */
} video;
