/*
    Copyright (C) 1998-2004 Charles MacDonald
    Copyright (C) 2024 Franck Charlet

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "shared.h"

input_t input[2] ALIGN;
bitmap_t bitmap ALIGN;
cart_t cart ALIGN;

/* Run the virtual console emulation for one frame */
void system_frame()
{
    int iline, frame_z80 = 0;
    int l_int;

    /* Debounce pause key */
    if((input[0].system & INPUT_PAUSE) || (input[1].system & INPUT_PAUSE))
    {
        if(!sms.paused)
        {
            sms.paused = 1;
            z80_set_irq_line(INPUT_LINE_NMI, ASSERT_LINE);
            z80_set_irq_line(INPUT_LINE_NMI, CLEAR_LINE);
        }
    }
    else
    {
        sms.paused = 0;
    }
    
    /* Reset TMS Text offset counter */
    text_counter = 0;
    
    /* VDP register 9 is latched during VBLANK */
    vdp.vscroll = vdp.reg[9];

    /* Reload Horizontal Interrupt counter */
    vdp.left = vdp.reg[0xA];

    /* Reset collision flag infos */
    vdp.spr_col = 0xff00;
    
    /* Render a frame */
    for(vdp.line = 0; vdp.line < vdp.lpf; vdp.line++)
    {
        iline = vdp.height;
        render_line_noscale(vdp.line);
        if(sms.console >= CONSOLE_SMS)
        {
            if(vdp.line <= iline)
            {
                if(--vdp.left < 0)
                {
                    vdp.left = vdp.reg[0xA];
                    vdp.hint_pending = 1;
                    /* Line interrupts enabled ? */
                    if(vdp.reg[0] & 0x10)
                    {
                        z80_set_irq_line(0, ASSERT_LINE);
                        if(!(z80_get_elapsed_cycles() % CYCLES_PER_LINE))
                        {
                            frame_z80 += z80_execute(1);
                        }
                    }
                }
            }
        }
        /* VBlank interrupt */
        if(vdp.line == iline)
        {
            vdp.status |= 0x80;
            vdp.vint_pending = 1;
            // Frame interrupts enabled ?
            if(vdp.reg[1] & 0x20)
            {
                z80_set_irq_line(vdp.irq, ASSERT_LINE);
            }
        }
        frame_z80 += z80_execute(CYCLES_PER_LINE);
//        sound_update(vdp.line);
    }

    /* Adjust Z80 cycle count for next frame */
    z80_cycle_count -= frame_z80;
}

void system_init(void)
{
    sms_init();
    pio_init();
    vdp_init();
    render_init();

    sms.save = 0;
}

void system_shutdown(void)
{
    z80_exit();
    sms_shutdown();
    pio_shutdown();
    vdp_shutdown();
    render_shutdown();
}

void system_reset(void)
{
    sms_reset();
    pio_reset();
    vdp_reset();
    render_reset();
    sound_reset();
    system_manage_sram(cart.sram, SLOT_CART, SRAM_LOAD);
}

void system_poweron(void)
{
    system_init();
    system_reset();
}

void system_poweroff(void)
{
    system_manage_sram(cart.sram, SLOT_CART, SRAM_SAVE);
    if(cart.rom)
    {
        free(cart.rom);
        cart.rom = NULL;
    }
}
