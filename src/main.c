#include "main.h"
#include "emu/shared.h"
#include "emu/prefs.h"
#include "emu/pad.h"
#include "emu/sound.h"

#define WIDTH 320

#ifdef __VAMPIRE__

#define HEIGHT 240

#else

#define HEIGHT 256

#endif

#define DEPTH 16

int pad_read(int __asm("d0"));
int key_read();

#ifdef __VAMPIRE__

unsigned int CHUNKYPTR;
unsigned short GFXMODE;
void setup_context();
void restore_context();
void wait_sync();

#else

struct Library *P96Base;
struct RenderInfo ri_back;

#endif

int switch_gfx = 0;

unsigned char *gfx_buffer1;
unsigned char *gfx_buffer2;

int key;

// -----------------------------------------------------
int running = 0;
int display_menu = 0;
int sent_reset = 0;

#ifdef __VAMPIRE__

unsigned char *Screen;

#else

unsigned short *Screen;

#endif

const char *PREFS_FILENAME = "SMS.CFG";

PREFERENCES prefs =
{
    { 0, 2, 4, 3 },
    { 0, 2, 4, 3 },
    0,
    5,
    { 0, 5, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
};

// -------------------------------------------------------
// Save the SRAM
// sram is already saved in save states
// i don't know if it should be saved independently.
void system_manage_sram(uint8_t *sram, int slot, int mode)
{
    
}

void free_gfx_mem()
{

#ifndef __VAMPIRE__

    if(pixel)
    {
        FreeMem(pixel, PALETTE_SIZE * sizeof(unsigned int));
    }
    pixel = NULL;

#endif

    if(gfx_buffer1)
    {
        FreeMem(gfx_buffer1, WIDTH * HEIGHT * 2 * sizeof(unsigned short));
    }
    gfx_buffer1 = NULL;
    if(gfx_buffer2)
    {
        FreeMem(gfx_buffer2, WIDTH * HEIGHT * 2 * sizeof(unsigned short));
    }
    gfx_buffer2 = NULL;
}

// -----------------------------------------------------

int main(int argc, char **argv)
{
    int pad_response;
    int first_frame;
    int current_display;
    int sprites_no_limit;
    int cur_arg;
    char switch_char;

#ifndef __VAMPIRE__

    struct Screen *screen;
    struct RenderInfo ri;
    ULONG lock;

#endif

    printf("MasterVamp v0.7 BETA\n");
    printf("A Master System/Game Gear/Coleco Vision/Sg1000 emulator.\n");
    printf("Written by Franck 'hitchhikr' Charlet.\n\n");

    if(argc < 2 || argc > 4)
    {
        printf("Usage: masv [N] [S] <romname.sms|.gg|.sg|.col>\n");
        exit(1);
    }

    gfx_buffer1 = (unsigned short *) AllocMem(WIDTH * HEIGHT * 2 * sizeof(unsigned short), MEMF_CLEAR);
    gfx_buffer2 = (unsigned short *) AllocMem(WIDTH * HEIGHT * 2 * sizeof(unsigned short), MEMF_CLEAR);

#ifdef __VAMPIRE__

    if(!gfx_buffer1 || !gfx_buffer2)

#else

    pixel = (unsigned int *) AllocMem(PALETTE_SIZE * sizeof(unsigned int), MEMF_CLEAR);
    if(!gfx_buffer1 || !gfx_buffer2 || !pixel)

#endif

    {
        free_gfx_mem();
        printf("Can't allocate memory.\n");
        exit(1);
    }

#ifndef __VAMPIRE__

    P96Base = OpenLibrary("Picasso96API.library", 0);
    if(P96Base == NULL)
    {
        free_gfx_mem();
        printf("Can't open Picasso96API.library.\n");
        exit(1);
    }

#endif

    current_display = DISPLAY_PAL;
    sprites_no_limit = FALSE;
    cur_arg = 1;

    if(argc > 2)
    {
        while(cur_arg < (argc - 1))
        {
            switch_char = toupper(argv[cur_arg][0]);
            switch(switch_char)
            {
                 case 'N':
                    current_display = DISPLAY_NTSC;
                    break;

                 case 'S':
                    sprites_no_limit = TRUE;
                    break;

                 default:
                    printf("Unknown switch '%c'.\n", switch_char);
                    exit(1);
                    break;
            }
            cur_arg++;
        }
    }

    if(load_rom(argv[cur_arg], current_display, sprites_no_limit) == 0)
    {
        free_gfx_mem();
        printf("Can't load rom '%s'.\n", argv[cur_arg]);
        exit(1);
    }

    snd.fps = FPS_PAL;
    snd.psg_clock = (sms.display == DISPLAY_NTSC) ? CLOCK_NTSC : CLOCK_PAL;
    snd.sample_rate = 22050;
    snd.mixer_callback = NULL;

    prefs_load(PREFS_FILENAME);

    sound_init();
    sound_play();

#ifndef __VAMPIRE__

	if(screen = p96OpenScreenTags(P96SA_Width, WIDTH,
								  P96SA_Height, HEIGHT,
								  P96SA_Depth, DEPTH,
								  P96SA_AutoScroll, FALSE,
                                  P96SA_NoSprite, TRUE,
                                  TAG_DONE))
    {
        if(lock = p96LockBitMap(&screen->BitMap, (UBYTE *) &ri, sizeof(ri)))
        {
            // Clear the borders
            p96RectFill(&screen->RastPort, 0, 0, WIDTH, HEIGHT, 0);

            ri_back.BytesPerRow = WIDTH * 2;
            ri_back.RGBFormat = RGBFB_R5G6B5;

#endif

            bitmap.width  = WIDTH;
            bitmap.height = HEIGHT;
            bitmap.data = gfx_buffer1;
            bitmap.viewport.x = IS_GG ? 48 : 0;
            bitmap.viewport.y = IS_GG ? 24 : 0;
            bitmap.viewport.w = IS_GG ? 160 : 256;
            bitmap.viewport.h = IS_GG ? 144 : 192;

            system_poweron();

            first_frame = FALSE;

            setup_context();

#ifdef __VAMPIRE__

            CHUNKYPTR = (*(volatile unsigned int *) 0xdfe1ec);
            GFXMODE = (*(volatile unsigned short *) 0xdfe1f4);
            wait_sync();
            (*(volatile unsigned short *) 0xdff1e6) = 0;
            (*(volatile unsigned int *) 0xdff1ec) = (unsigned int) gfx_buffer1;
            // Set 320x240x8
            (*(volatile unsigned short *) 0xdff1f4) = 0x0201;

#endif

            key = 0;
            running = 1;

            while(running)
            {
                wait_sync();
                switch_gfx ^= 1;

#ifdef __VAMPIRE__
                
                if(switch_gfx)
                {
                    Screen = gfx_buffer1;
                }
                else
                {
                    Screen = gfx_buffer2;
                }

#else

                if(switch_gfx)
                {
                    ri_back.Memory = gfx_buffer1;
                }
                else
                {
                    ri_back.Memory = gfx_buffer2;
                }
                Screen = (unsigned short *) ri_back.Memory;

#endif
                if(first_frame)
                {
                    system_frame();
                }

                // Pad control
                if(IS_GG)
                {
                    input[0].system = 0;
                    input[0].pad = 0;
                    pad_response = pad_read(1);
                    if(pad_response & PAD_BUTTON_RED) input[0].pad |= INPUT_BUTTON2;
                    if(pad_response & PAD_BUTTON_BLUE) input[0].pad |= INPUT_BUTTON1;
                    if(pad_response & PAD_BUTTON_GREEN) input[0].system |= INPUT_START;
                    if(pad_response & PAD_BUTTON_LEFT) input[0].pad |= INPUT_LEFT;
                    if(pad_response & PAD_BUTTON_RIGHT) input[0].pad |= INPUT_RIGHT;
                    if(pad_response & PAD_BUTTON_UP) input[0].pad |= INPUT_UP;
                    if(pad_response & PAD_BUTTON_DOWN) input[0].pad |= INPUT_DOWN;
                }
                else
                {
                    input[0].system = 0;
                    input[0].pad = 0;
                    pad_response = pad_read(1);
                    if(pad_response & PAD_BUTTON_RED) input[0].pad |= INPUT_BUTTON2;
                    if(pad_response & PAD_BUTTON_BLUE) input[0].pad |= INPUT_BUTTON1;
                    if(pad_response & PAD_BUTTON_BLUE) input[0].system |= INPUT_START;
                    if(pad_response & PAD_BUTTON_GREEN) input[0].system |= INPUT_PAUSE;
                    if(IS_SMS)
                    {
                        if(pad_response & PAD_BUTTON_YELLOW) sent_reset = 1;
                    }
                    if(pad_response & PAD_BUTTON_LEFT) input[0].pad |= INPUT_LEFT;
                    if(pad_response & PAD_BUTTON_RIGHT) input[0].pad |= INPUT_RIGHT;
                    if(pad_response & PAD_BUTTON_UP) input[0].pad |= INPUT_UP;
                    if(pad_response & PAD_BUTTON_DOWN) input[0].pad |= INPUT_DOWN;

                    input[1].system = 0;
                    input[1].pad = 0;
                    pad_response = pad_read(0);
                    if(pad_response & PAD_BUTTON_RED) input[1].pad |= INPUT_BUTTON2;
                    if(pad_response & PAD_BUTTON_BLUE) input[1].pad |= INPUT_BUTTON1;
                    if(pad_response & PAD_BUTTON_BLUE) input[1].system |= INPUT_START;
                    if(pad_response & PAD_BUTTON_GREEN) input[1].system |= INPUT_PAUSE;
                    if(IS_SMS)
                    {
                        if(pad_response & PAD_BUTTON_YELLOW) sent_reset = 1;
                    }
                    if(pad_response & PAD_BUTTON_LEFT) input[1].pad |= INPUT_LEFT;
                    if(pad_response & PAD_BUTTON_RIGHT) input[1].pad |= INPUT_RIGHT;
                    if(pad_response & PAD_BUTTON_UP) input[1].pad |= INPUT_UP;
                    if(pad_response & PAD_BUTTON_DOWN) input[1].pad |= INPUT_DOWN;
                }

                key = key_read();
                // 'ESCAPE' key
                if(key == 0x45)
                {
                    break;
                }
                // 'SPACE' key
                if(key == 0x40)
                {
                    input[0].system |= INPUT_START;
                    input[1].system |= INPUT_START;
                }
                if(IS_SMS)
                {
                    // 'P' key
                    if(key == 0x19)
                    {
                        input[0].system |= INPUT_PAUSE;
                        input[1].system |= INPUT_PAUSE;
                    }
                }
                coleco.keypad[0] = 0xff;
                if(IS_TMS)
                {
                    // '1 to =' keys
                    switch(key)
                    {
                        case 1:
                        case 2:
                        case 3:
                        case 4:
                        case 5:
                        case 6:
                        case 7:
                        case 8:
                        case 9:
                            coleco.keypad[0] = key;
                            break;
                        case 0xa:
                            coleco.keypad[0] = 0;
                            break;
                        case 0xb:
                            coleco.keypad[0] = 10;
                            break;
                        case 0xc:
                            coleco.keypad[0] = 11;
                            break;
                    }
                }

                if(sent_reset)
                {
                    sent_reset = 0;
                    system_reset();
                }

#ifdef __VAMPIRE__

                (*(volatile unsigned int *) 0xdff1ec) = (unsigned int) Screen;

#else

                p96WritePixelArray(&ri_back, 0, 0, &screen->RastPort, 0, 0, WIDTH, HEIGHT);

#endif

                if(IS_GG)
                {
                    bitmap.data = Screen + ((WIDTH - bitmap.viewport.w) >> 1) + (((HEIGHT - 192) >> 1) * WIDTH);
                }
                else
                {
                    bitmap.data = Screen + ((WIDTH - bitmap.viewport.w) >> 1) + (((HEIGHT - bitmap.viewport.h) >> 1) * WIDTH);
                }

                first_frame = TRUE;
            }

#ifndef __VAMPIRE__

            p96UnlockBitMap(&screen->BitMap, lock);
        }
        else
        {
            printf("Can't lock bitmap.\n");
        }
        restore_context();
        p96CloseScreen(screen);
    }
    else
    {
        printf("Can't open screen.\n");
    }

    CloseLibrary(P96Base);

#else

    (*(volatile unsigned int *) 0xdff1ec) = CHUNKYPTR;
    (*(volatile unsigned short *) 0xdff1f4) = GFXMODE;
    restore_context();

#endif

    system_shutdown();
    system_poweroff();
    sound_shutdown();
    free_gfx_mem();
    return(0);
}
