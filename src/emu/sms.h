/******************************************************************************
 *  Sega Master System / GameGear Emulator
 *  Copyright (C) 1998-2007  Charles MacDonald
 *
 *  additionnal code by Eke-Eke (SMS Plus GX)
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *   Sega Master System console emulation
 *
 ******************************************************************************/

#ifndef _SMS_H_
#define _SMS_H_

#define CYCLES_PER_LINE 228

enum
{
    SLOT_BIOS = 0,
    SLOT_CARD = 1,
    SLOT_CART = 2,
    SLOT_EXP = 3
};

enum
{
    MAPPER_NONE = 0,
    MAPPER_SEGA = 1,
    MAPPER_CODIES = 2,
    MAPPER_KOREA = 3,
    MAPPER_KOREA_MSX = 4
};

enum
{
    DISPLAY_NTSC = 0,
    DISPLAY_PAL = 1
};

enum
{
    FPS_NTSC = 60,
    FPS_PAL = 50
};

enum
{
    CLOCK_NTSC = 3579545,
    CLOCK_PAL = 3546895,
    CLOCK_NTSC_SMS1 = 3579527
};

enum
{
    CONSOLE_COLECO = 0x10,
    CONSOLE_SG1000 = 0x11,
    CONSOLE_SC3000 = 0x12,
    CONSOLE_SF7000 = 0x13,

    CONSOLE_SMS = 0x20,
    CONSOLE_SMS2 = 0x21,

    CONSOLE_GG = 0x40,
    CONSOLE_GGMS = 0x41
};

#define HWTYPE_TMS (CONSOLE_COLECO & 0x10)
#define HWTYPE_SMS (CONSOLE_SMS & 0x20)
#define HWTYPE_GG (CONSOLE_GG & 0x40)

#define IS_TMS (sms.console & HWTYPE_TMS)
#define IS_SMS (sms.console & HWTYPE_SMS)
#define IS_GG (sms.console & HWTYPE_GG)

enum
{
    TERRITORY_DOMESTIC = 0,
    TERRITORY_EXPORT = 1
};

/* SMS context */
typedef struct
{
    uint8 wram[0x2000];
    uint32 paused;
    uint32 save;
    uint32 territory;
    uint32 console;
    uint32 display;
    uint32 sprites_no_limit;
    //uint32 fm_detect;
    uint32 glasses_3d;
    uint32 hlatch;
    //uint32 use_fm;
    uint32 memctrl;
    uint32 ioctrl;
    struct
    {
        uint32 pdr;    /* Parallel data register */
        uint32 ddr;    /* Data direction register */
        uint32 txdata; /* Transmit data buffer */
        uint32 rxdata; /* Receive data buffer */
        uint32 sctrl;  /* Serial mode control and status */
    } sio;
    uint32 device[2];
    uint32 gun_offset;
    uint32 use_scale;
} sms_t;

/* BIOS ROM */
typedef struct
{
    uint8 rom[8192];  
    uint32 enabled;
    uint32 pages;
    uint8 fcr[4];
} bios_t;

typedef struct
{
    uint8 *rom;
    uint32 pages;
    uint8 *fcr;
    uint32 mapper;
} slot_t;

typedef struct
{
    uint8 rom[8192];    /* BIOS ROM */
    uint32 pio_mode;    /* PIO mode */
    uint8 keypad[2];    /* Keypad inputs */
} t_coleco;

/* Global data */
extern sms_t sms;
extern bios_t bios;
extern slot_t slot;
extern t_coleco coleco;
extern uint8 dummy_write[0x4000];
extern uint8 dummy_read[0x4000];

/* Function prototypes */
extern void sms_init(void);
extern void sms_reset(void);
extern void sms_shutdown(void);
extern void mapper_reset(void);
extern void mapper_8k_w(uint16 address, uint8 data);
extern void mapper_16k_w(uint16 address, uint8 data);

#endif /* _SMS_H_ */
