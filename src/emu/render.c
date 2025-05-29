/*
    render.c --
    Display rendering.
*/

#include "shared.h"

typedef struct obj
{
    uint16 yrange;
    uint16 xpos;
    uint16 attr;
} obj;

obj object_info[64];

uint8 sms_cram_expand_tableR[4] ALIGN;
uint8 sms_cram_expand_tableG[4] ALIGN;
uint8 sms_cram_expand_tableB[4] ALIGN;
uint8 gg_cram_expand_tableR[16] ALIGN;
uint8 gg_cram_expand_tableG[16] ALIGN;
uint8 gg_cram_expand_tableB[16] ALIGN;

/* Background drawing function */
void (*render_bg)(int line) = 0;
void (*render_obj)(int line) = 0;
void load_palette(int __asm("d0"), uint32 __asm("d1"));
void render_bg_sms_line(int __asm("d0"), int __asm("d1"), int __asm("d2"), int __asm("d3"),
                        uint16 * __asm("a0"), uint32 * __asm("a1"), uint8 * __asm("a2"), uint32 * __asm("a3"), int * __asm("a4"));
void render_obj_sms_line(int __asm("d0"), int __asm("d1"), int __asm("d2"), int __asm("d3"),
                         uint8 * __asm("a0"), uint8 * __asm("a1"), uint8 * __asm("a2"), int * __asm("a3"), int * __asm("a4"));
void render_obj_sms_line_double(int __asm("d0"), int __asm("d1"), int __asm("d2"), int __asm("d3"),
                                uint8 * __asm("a0"), uint8 * __asm("a1"), uint8 * __asm("a2"), int * __asm("a3"), int * __asm("a4"));

/* Pointer to output buffer */
uint8 *linebuf ALIGN;

/* Internal buffer for drawing non 8-bit displays */
uint8 internal_buffer[0x4000] ALIGN;

/* Precalculated pixel table */
#ifndef __VAMPIRE__

uint16 *pixel;
    
#endif

/* Dirty pattern info */
uint8 bg_name_dirty[0x400] ALIGN;     /* 1 = This pattern is dirty */
uint16 bg_name_list[0x400] ALIGN;     /* List of modified pattern indices */
uint16 bg_list_index ALIGN;           /* # of modified patterns in list */
uint8 bg_pattern_cache[0x40000] ALIGN;/* Cached and flipped patterns */

/* Pixel look-up table */
uint8 lut[0x20000] ALIGN;

/* Bitplane to packed pixel LUT */
uint32 bp_lut[0x20000] ALIGN;

static uint8 object_index_count;

/* CRAM palette in TMS compatibility mode */
static const uint8 tms_crom[] =
{
    0x00, 0x00, 0x08, 0x0C,
    0x10, 0x30, 0x01, 0x3C,
    0x02, 0x03, 0x05, 0x0F,
    0x04, 0x33, 0x15, 0x3F
};

/* Original TMS palette for SG-1000 & Colecovision */
uint8 tms_palette[16][3] =
{
    /* From Richard F. Drushel (http://users.stargate.net/~drushel/pub/coleco/twwmca/wk961118.html) */
    {   0 >> 3,   0 >> 2,   0 >> 3 },
    {   0 >> 3,   0 >> 2,   0 >> 3 },
    {  71 >> 3, 183 >> 2,  59 >> 3 },
    { 124 >> 3, 207 >> 2, 111 >> 3 },
    {  93 >> 3,  78 >> 2, 255 >> 3 },
    { 128 >> 3, 114 >> 2, 255 >> 3 },
    { 182 >> 3,  98 >> 2,  71 >> 3 },
    { 93  >> 3, 200 >> 2, 237 >> 3 },
    { 215 >> 3, 107 >> 2,  72 >> 3 },
    { 251 >> 3, 143 >> 2, 108 >> 3 },
    { 195 >> 3, 205 >> 2,  65 >> 3 },
    { 211 >> 3, 218 >> 2, 118 >> 3 },
    {  62 >> 3, 159 >> 2,  47 >> 3 },
    { 182 >> 3, 100 >> 2, 199 >> 3 },
    { 204 >> 3, 204 >> 2, 204 >> 3 },
    { 255 >> 3, 255 >> 2, 255 >> 3 }
};

/* Attribute expansion table */
static const uint32 atex[4] =
{
    0x00000000,
    0x10101010,
    0x20202020,
    0x30303030
};

/****************************************************************************/

void render_shutdown(void)
{

}

/* Initialize the rendering data */
void render_init(void)
{
    int i, j;
    int bx, sx, b, s, bp, bf, sf, c;

    make_tms_tables();

    /* Generate 64k of data for the look up table */
    for(bx = 0; bx < 0x100; bx++)
    {
        for(sx = 0; sx < 0x100; sx++)
        {
            /* Background pixel */
            b  = (bx & 0x0F);

            /* Background priority */
            bp = (bx & 0x20) ? 1 : 0;

            /* Full background pixel + priority + sprite marker */
            bf = (bx & 0x7F);

            /* Sprite pixel */
            s  = (sx & 0x0F);

            /* Full sprite pixel, w/ palette and marker bits added */
            sf = (sx & 0x0F) | 0x10 | 0x40;

            /* Overwriting a sprite pixel ? */
            if(bx & 0x40)
            {
                /* Return the input */
                c = bf;
            }
            else
            {
                /* Work out priority and transparency for both pixels */
                if(bp)
                {
                    /* Underlying pixel is high priority */
                    if(b)
                    {
                        c = bf | 0x40;
                    }
                    else
                    {
                        if(s)
                        {
                            c = sf;
                        }
                        else
                        {
                            c = bf;
                        }
                    }
                }
                else
                {
                    /* Underlying pixel is low priority */
                    if(s)
                    {
                        c = sf;
                    }
                    else
                    {
                        c = bf;
                    }
                }
            }

            /* Store result */
            lut[(bx << 8) | sx] = c;
        }
    }

    /* Make bitplane to pixel lookup table */
    for(i = 0; i < 256; i++)
    {
        for(j = 0; j < 256; j++)
        {
            int x;
            uint32 out = 0;
            for(x = 0; x < 8; x++)
            {
                out |= (j & (0x80 >> x)) ? (uint32) (8 << (x << 2)) : 0;
                out |= (i & (0x80 >> x)) ? (uint32) (4 << (x << 2)) : 0;
            }
            bp_lut[(i << 8) | j] = out;
        }
    }

    render_reset();
}

/* Set the palettes data */
void palette_init(void)
{
    int i;

    for(i = 0; i < 4; i++)
    {
        uint8 c = i << 6 | i << 4 | i << 2 | i;
        sms_cram_expand_tableR[i] = c >> 3;
        sms_cram_expand_tableG[i] = c >> 2;
        sms_cram_expand_tableB[i] = c >> 3;
    }
    for(i = 0; i < 16; i++)
    {
        uint8 c = i << 4 | i;
        gg_cram_expand_tableR[i] = c >> 3;
        gg_cram_expand_tableG[i] = c >> 2;
        gg_cram_expand_tableB[i] = c >> 3;
    }
    for(i = 0; i < REAL_PALETTE_SIZE; i++)
    {
        palette_sync(i);
    }
}

/* Reset the rendering data */
void render_reset(void)
{
    int i;

#ifdef __VAMPIRE__

    /* Clear display bitmap */
    memset(bitmap.data, 0, bitmap.width * bitmap.height * sizeof(unsigned char));

#else

    /* Clear display bitmap */
    memset(bitmap.data, 0, bitmap.width * bitmap.height * sizeof(unsigned short));

#endif

    palette_init();

    /* Invalidate pattern cache */
    memset(bg_name_dirty, 0, sizeof(bg_name_dirty));
    memset(bg_name_list, 0, sizeof(bg_name_list));
    bg_list_index = 0;
    memset(bg_pattern_cache, 0, sizeof(bg_pattern_cache));

    /* Pick default render routine */
    if(vdp.reg[0] & 4)
    {
        render_bg = render_bg_sms;
        render_obj = render_obj_sms;
    }
    else
    {
        render_bg = render_bg_tms;
        render_obj = render_obj_tms;
    }
}

/* Draw the Master System or Game Gear background */
void render_bg_sms(int line)
{
    int locked = 0;
    int yscroll_mask = (vdp.extended) ? 256 : 224;
    int v_line = (line + vdp.vscroll) % yscroll_mask;
    int v_row  = (v_line & 7) << 3;
    UINT8 hscroll = ((vdp.reg[0] & 0x40) && (line < 0x10) && (sms.console != CONSOLE_GG)) ? 0 : (0x100 - vdp.reg[8]);
    int column = 0;
    uint16 attr;
    uint16 nt_addr = (vdp.ntab + ((v_line >> 3) << 6)) & (((sms.console == CONSOLE_SMS) && !(vdp.reg[2] & 1)) ? ~0x400 : 0xFFFF);
    uint16 *nt = (uint16 *) &vdp.vram[nt_addr];
    
    int nt_scroll = (hscroll >> 3);
    int shift = (hscroll & 7);
    uint32 atex_mask;
    uint32 *linebuf_ptr = (uint32 *) &linebuf[0 - shift];

    /* Draw first column (clipped) */
    if(shift)
    {
        int x;

        for(x = shift; x < 8; x++)
        {
            linebuf[(0 - shift) + x] = 0;
        }
        column++;
    }

    render_bg_sms_line(&column, vdp.reg[0], line, nt_scroll, nt, atex, bg_pattern_cache, linebuf_ptr, &v_row);

    /* Draw last column (clipped) */
    if(shift)
    {
        int x;
        uint8 c, a;

        uint8 *p = &linebuf[(0 - shift) + (column << 3)];

        attr = nt[(column + nt_scroll) & 0x1F];
        attr = ((attr & 0xff) << 8) | ((attr & 0xff00) >> 8);

        a = (attr >> 7) & 0x30;

        for(x = 0; x < shift; x++)
        {
            c = bg_pattern_cache[((attr & 0x7FF) << 6) | v_row | x];
            p[x] = c | a;
        }
    }
}

/* Draw sprites */
void render_obj_sms(int line)
{
    short i;
    int x;
    int start;
    int end;
    int xp;
    int yp;
    int n;
    uint8 sp;
    uint8 bg;
    uint8 *linebuf_ptr;
    uint8 *cache_ptr;

    int width = 8;

    /* Adjust dimensions for double size sprites */
    if(vdp.reg[1] & 0x01)
    {
        width <<= 1;
    }

    /* Draw sprites in front-to-back order */
    for(i = 0; i < object_index_count; i++)
    {
        /* Width of sprite */
        start = 0;
        end = width;

        /* Sprite X position */
        xp = object_info[i].xpos;

        /* Sprite Y range */
        yp = object_info[i].yrange;

        /* Pattern name */
        n = object_info[i].attr;

        /* X position shift */
        if(vdp.reg[0] & 0x08)
        {
            xp -= 8;
        }

        /* Add MSB of pattern name */
        if(vdp.reg[6] & 0x04)
        {
            n |= 0x0100;
        }

        /* Mask LSB for 8x16 sprites */
        if(vdp.reg[1] & 0x02)
        {
            n &= 0x01FE;
        }

        /* Clip sprites on left edge */
        if(xp < 0)
        {
            start = (0 - xp);
        }

        /* Clip sprites on right edge */
        if((xp + width) > 256)
        {
            end = (256 - xp);
        }

        /* Point to offset in line buffer */
        linebuf_ptr = (uint8 *) &linebuf[xp];

        if(end > start)
        {
            /* Draw double size sprite */
            if(vdp.reg[1] & 0x01)
            {
                /* Retrieve tile data from cached nametable */
                cache_ptr = (uint8 *) &bg_pattern_cache[(n << 6) | ((yp >> 1) << 3)];
                render_obj_sms_line_double(start, end, line, xp, cache_ptr, linebuf_ptr, lut, &vdp.status, &vdp.spr_col);
            }
            else /* Regular size sprite (8x8 / 8x16) */
            {
                /* Retrieve tile data from cached nametable */
                cache_ptr = (uint8 *) &bg_pattern_cache[(n << 6) | (yp << 3)];
                render_obj_sms_line(start, end, line, xp, cache_ptr, linebuf_ptr, lut, &vdp.status, &vdp.spr_col);
            }
        }
    }
}

void parse_satb(int line)
{
    /* Pointer to sprite attribute table */
    uint8 *st = (uint8 *) &vdp.vram[vdp.satb];

    /* Sprite counter (64 max.) */
    short i = 0;

    /* Line counter value */
    int vc = vc_table[sms.display][vdp.extended][line];

    /* Sprite height (8x8 by default) */
    int yp;
    int height = 8;
  
    /* Adjust height for 8x16 sprites */
    if(vdp.reg[1] & 0x02) 
    {
        height <<= 1;
    }

    /* Adjust height for zoomed sprites */
    if(vdp.reg[1] & 0x01)
    {
        height <<= 1;
    }

    /* Sprite count for current line (8 max.) */
    object_index_count = 0;

    for(i = 0; i < 64; i++)
    {
        /* Sprite Y position */
        yp = st[i];

        /* Found end of sprite list marker for non-extended modes ? */
        if(vdp.extended == 0 && yp == 0xD0)
        {
            return;
        }

        /* Actual Y position is +1 */
        yp++;

        /* Wrap Y coordinate for sprites > 240 */
        if(yp > 240)
        {
            yp -= 256;
        }

        /* Compare sprite position with current line counter */
        yp = vc - yp;

        /* Sprite is within vertical range ? */
        if((yp >= 0) && (yp < height))
        {
            /* Sprite limit reached? */
            if(object_index_count == 8)
            {
                /* Flag is set only during active area */
                if(line < vdp.height)
                {
                    vdp.spr_ovr = 1;
                }
                /* End of sprite parsing (eventually) */
                if(!sms.sprites_no_limit)
                {
                    return;
                }
            }

            /* Store sprite attributes for later processing */
            object_info[object_index_count].yrange = yp;
            object_info[object_index_count].xpos = st[0x80 + (i << 1)];
            object_info[object_index_count].attr = st[0x81 + (i << 1)];

            /* Increment Sprite count for current line */
            object_index_count++;
        }
    }
}

void update_bg_pattern_cache(void)
{
    int i;
    uint8 x, y;
    uint16 name;
    uint8 y3;
    uint8 y7;
    uint8 *dst;
    uint8 dirty;
    uint8 *dst0000;
    uint8 *dst8000;
    uint8 *dst10000;
    uint8 *dst18000;
    uint8 c;

    if(!bg_list_index)
    {
        return;
    }

    for(i = 0; i < bg_list_index; i++)
    {
        name = bg_name_list[i];
        bg_name_list[i] = 0;
        
        dirty = bg_name_dirty[name];
        if(dirty)
        {
            dst = &bg_pattern_cache[name << 6];
            for(y = 0; y < 8; y++)
            {
                /* Test bits */
                if(1 << y)
                {
                    y3 = y << 3;
                    y7 = (y ^ 7) << 3;
                    uint16 bp01 = *((uint16 *) &vdp.vram[(name << 5) | (y << 2)]);
                    uint16 bp23 = *((uint16 *) &vdp.vram[(name << 5) | (y << 2) | 2]);
                    uint32 temp = (bp_lut[bp01] >> 2) | (bp_lut[bp23]);

                    dst0000 = dst + y3;
                    dst10000 = dst + 0x10000 + y7;
                    dst8000 = (dst + 0x8000 + y3) + 7;
                    dst18000 = (dst + 0x18000 + y7) + 7;

                    c = (uint8) ((temp >> (0 << 2)) & 0xF);
                    *dst0000++ = c;
                    *dst10000++ = c;
                    *dst8000-- = c;
                    *dst18000-- = c;
                    c = (uint8) ((temp >> (1 << 2)) & 0xF);
                    *dst0000++ = c;
                    *dst10000++ = c;
                    *dst8000-- = c;
                    *dst18000-- = c;
                    c = (uint8) ((temp >> (2 << 2)) & 0xF);
                    *dst0000++ = c;
                    *dst10000++ = c;
                    *dst8000-- = c;
                    *dst18000-- = c;
                    c = (uint8) ((temp >> (3 << 2)) & 0xF);
                    *dst0000++ = c;
                    *dst10000++ = c;
                    *dst8000-- = c;
                    *dst18000-- = c;
                    c = (uint8) ((temp >> (4 << 2)) & 0xF);
                    *dst0000++ = c;
                    *dst10000++ = c;
                    *dst8000-- = c;
                    *dst18000-- = c;
                    c = (uint8) ((temp >> (5 << 2)) & 0xF);
                    *dst0000++ = c;
                    *dst10000++ = c;
                    *dst8000-- = c;
                    *dst18000-- = c;
                    c = (uint8) ((temp >> (6 << 2)) & 0xF);
                    *dst0000++ = c;
                    *dst10000++ = c;
                    *dst8000-- = c;
                    *dst18000-- = c;
                    c = (uint8) ((temp >> (7 << 2)) & 0xF);
                    *dst0000 = c;
                    *dst10000 = c;
                    *dst8000 = c;
                    *dst18000 = c;
                }
            }
            bg_name_dirty[name] = 0;
        }
    }
    bg_list_index = 0;
}

/* Update a palette entry */
void palette_sync(int index)
{
    int r;
    int g;
    int b;
    int y;
    int u;
    int v;

    // Unless we are forcing an update, if not in mode 4, exit

    if((vdp.reg[0] & 4) || IS_GG)
    {
        if(IS_GG)
        {
            /* ----BBBBGGGGRRRR */
            r = (vdp.cram[(index << 1)]) & 0x0F;
            g = (vdp.cram[(index << 1)] >> 4) & 0x0F;
            b = (vdp.cram[(index << 1) | 1]) & 0x0F;

            r = gg_cram_expand_tableR[r];
            g = gg_cram_expand_tableG[g];
            b = gg_cram_expand_tableB[b];
        }
        else
        {
            /* --BBGGRR */
            r = (vdp.cram[index] >> 0) & 3;
            g = (vdp.cram[index] >> 2) & 3;
            b = (vdp.cram[index] >> 4) & 3;

            r = sms_cram_expand_tableR[r];
            g = sms_cram_expand_tableG[g];
            b = sms_cram_expand_tableB[b];
        }
    }
    else
    {
        /* TMS Mode (16 colors only) */
        int color = index & 0x0F;

        if (sms.console < CONSOLE_SMS)
        {
            r = tms_palette[color][0];
            g = tms_palette[color][1];
            b = tms_palette[color][2];
        }
        else
        {
            /* Fixed CRAM palette in TMS mode */ 
            r = (tms_crom[color] >> 0) & 3;
            g = (tms_crom[color] >> 2) & 3;
            b = (tms_crom[color] >> 4) & 3;

            r = sms_cram_expand_tableR[r];
            g = sms_cram_expand_tableG[g];
            b = sms_cram_expand_tableB[b];
        }
    }

#ifdef __VAMPIRE__

    // Update the hardware palette
    load_palette(index, MAKE_PIXEL32(r, g, b));

#else

    pixel[index] = MAKE_PIXEL16(r, g, b);
    pixel[index + 32] = pixel[index];
    pixel[index + 64] = pixel[index];
    pixel[index + 96] = pixel[index];
    pixel[index + 128] = pixel[index];
    pixel[index + 160] = pixel[index];
    pixel[index + 192] = pixel[index];
    pixel[index + 224] = pixel[index];

#endif

}
