#ifndef _RENDER_H_
#define _RENDER_H_

/* Pack RGB data into a 32-bit RGB format */
#define MAKE_PIXEL32(r, g, b) (r << 19) | (g << 10) | (b << 3)
/* Pack RGB data into a 16-bit 565 RGB format */
#define MAKE_PIXEL16(r, g, b) (r << 11) | (g << 5) | b

/* Used for blanking a line in whole or in part */
#define BACKDROP_COLOR (0x10 | (vdp.reg[7] & 0x0F))

#define REAL_PALETTE_SIZE 32
#define PALETTE_SIZE 258

typedef struct
{
    uint8 r;
    uint8 g;
    uint8 b;
    uint8 a;
} RGB, *LPRGB;

extern uint8 sms_cram_expand_tableR[4];
extern uint8 sms_cram_expand_tableG[4];
extern uint8 sms_cram_expand_tableB[4];
extern uint8 gg_cram_expand_tableR[16];
extern uint8 gg_cram_expand_tableG[16];
extern uint8 gg_cram_expand_tableB[16];
extern void (*render_bg)(int line);
extern void (*render_obj)(int line);
extern uint8 *linebuf;
extern uint8 internal_buffer[];

#ifndef __VAMPIRE__

extern uint16 *pixel;

#endif

extern uint8 bg_name_dirty[0x400];
extern uint16 bg_name_list[0x400];
extern uint16 bg_list_index;
extern uint8 bg_pattern_cache[0x40000];
extern uint8 tms_lookup[16][256][2];
extern uint8 tms_lookup2[16][256][2];
extern uint8 mc_lookup[16][256][8];
extern uint8 txt_lookup[256][2];
extern uint8 txt_lookup2[256][2];
extern uint8 bp_expand[256][8];
extern uint8 lut[0x20000];

void render_shutdown(void);
void render_init(void);
void palette_init(void);
void render_reset(void);
void render_bg_sms(int line);
void render_obj_sms(int line);
void update_bg_pattern_cache(void);
void palette_sync(int index);
void remap_8_to_16(int line);

#endif /* _RENDER_H_ */
