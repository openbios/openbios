
void init_video(unsigned long fb, int width, int height, int depth, int rb);
unsigned long video_get_color(int col_ind);
void video_set_color(int ind, unsigned long color);
int video_get_res(int *w, int *h);
void video_mask_blit(void);
void video_invert_rect(void);
void video_fill_rect(void);

extern struct video_info {
    int has_video;

    volatile phys_addr_t mphys;
    volatile ucell *mvirt;
    volatile ucell *rb, *w, *h, *depth;

    volatile ucell *pal;    /* 256 elements */
} video;

#define VIDEO_DICT_VALUE(x)  (*(ucell *)(x))
