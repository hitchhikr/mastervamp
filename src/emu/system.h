#ifndef _SYSTEM_H_
#define _SYSTEM_H_

/* These can be used for 'input[].pad[]' */
#define INPUT_UP 0x00000001
#define INPUT_DOWN 0x00000002
#define INPUT_LEFT 0x00000004
#define INPUT_RIGHT 0x00000008
#define INPUT_BUTTON2 0x00000010
#define INPUT_BUTTON1 0x00000020

/* These can be used for 'input[].system' */
#define INPUT_START 0x00000001  /* Game Gear only */    
#define INPUT_PAUSE 0x00000002  /* Master System only */
#define INPUT_RESET 0x00000004  /* Master System only */

enum
{
    SRAM_SAVE = 0,
    SRAM_LOAD = 1
};

/* User input structure */
typedef struct
{
    uint32 pad;
    uint32 system;
} input_t;

/* Game image structure */
typedef struct
{
    uint8 *rom;
    uint32 loaded;
    uint32 size;
    uint32 pages;
    uint32 crc;
    uint32 sram_crc;
    int mapper;
    uint8 sram[0x8000];
    uint8 fcr[4];
} cart_t;

/* Bitmap structure */
typedef struct
{

#ifdef __VAMPIRE__

    uint8 *data;

#else

    uint16 *data;

#endif

    int width;
    int height;
    int pitch;
    int depth;
    struct
    {
        int x, y, w, h;
    } viewport;        
} bitmap_t;

/* Global variables */
extern bitmap_t bitmap;     /* Display bitmap */
extern cart_t cart;         /* Game cartridge data */
extern input_t input[];     /* Controllers input */

/* Function prototypes */
void system_frame(void);
void system_init(void);
void system_shutdown(void);
void system_reset(void);
void system_manage_sram(uint8 *sram, int slot, int mode);
void system_poweron(void);
void system_poweroff(void);

void parse_satb(int line);
void update_bg_pattern_cache(void);

#ifdef __VAMPIRE__

void copy_pixels_160(uint8 * __asm("a0"), uint8 * __asm("a1"));
void copy_pixels_256(uint8 * __asm("a0"), uint8 * __asm("a1"));

#else

void copy_pixels(int __asm("d0"), uint8 * __asm("a0"), uint16 * __asm("a1"), uint16 * __asm("a2"));

#endif

static int prev_line = -1;

/* Top Border area height */
static uint8 active_border[2][3] =
{
    { 24, 8,  0 },  /* NTSC VDP */
    { 48, 32, 24 }  /* PAL VDP */
};

/* Active Scan Area height */
static uint16 active_range[2] =
{
    243, /* NTSC VDP */
    294  /* PAL VDP */
};

inline void render_line_noscale(int line)
{
    short i;
    uint16 pix;
    uint8 *buf;
    uint16 *p;
    int top_border;
    int vline;

    if(prev_line == line)
    {
        return;
    }

    prev_line = line;
    top_border = active_border[sms.display][vdp.extended];
    vline = (line + top_border) % vdp.lpf;
    if(vline >= active_range[sms.display])
    {
        return;
    }

    linebuf = &internal_buffer[0];
    if(vdp.spr_ovr)
    {
        vdp.spr_ovr = 0;
        vdp.status |= 0x40;
    }
    if(vdp.mode > 7)
    {
        parse_satb(line);
    }
    else
    {
        parse_line(line);
    }
    if((line < bitmap.viewport.y) || (line >= (bitmap.viewport.h + bitmap.viewport.y)))
    {
        if((vdp.mode > 7) && (vdp.reg[1] & 0x40))
        {
            render_obj(line);
        }
        return;
    }
    else
    {
        /* Display enabled ? */
        if(vdp.reg[1] & 0x40)
        {
            update_bg_pattern_cache();
            render_bg(line);
            render_obj(line);
            if((vdp.reg[0] & 0x20) && IS_SMS)
            {
                linebuf[0] = BACKDROP_COLOR;
                linebuf[1] = BACKDROP_COLOR;
                linebuf[2] = BACKDROP_COLOR;
                linebuf[3] = BACKDROP_COLOR;
                linebuf[4] = BACKDROP_COLOR;
                linebuf[5] = BACKDROP_COLOR;
                linebuf[6] = BACKDROP_COLOR;
                linebuf[7] = BACKDROP_COLOR;
            }
        }
        else
        {
            memset(linebuf, BACKDROP_COLOR, bitmap.viewport.w + (2 * bitmap.viewport.x));
        }
    }

    // Render the line
    buf = linebuf + bitmap.viewport.x;
    p = &bitmap.data[(line * 320)];

#ifdef __VAMPIRE__

    if(IS_GG)
    {
        copy_pixels_160(buf, p);
    }
    else
    {
        copy_pixels_256(buf, p);
    }

#else

    copy_pixels(bitmap.viewport.w, buf, pixel, p);

#endif

}

#endif /* _SYSTEM_H_ */
