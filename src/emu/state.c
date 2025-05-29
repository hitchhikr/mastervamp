/*
    state.c --
    Save state management.
*/

#include "shared.h"

void system_save_state(FILE *fd)
{
    char *id = STATE_HEADER;
    uint16 version = STATE_VERSION;

    /* Write header */
    fwrite(id, sizeof(id), 1, fd);
    fwrite(&version, sizeof(version), 1, fd);

    /* Save VDP context */
    fwrite(&vdp, sizeof(vdp_t), 1, fd);

    /* Save SMS context */
    fwrite(&sms, sizeof(sms_t), 1, fd);

    fputc(cart.fcr[0], fd);
    fputc(cart.fcr[1], fd);
    fputc(cart.fcr[2], fd);
    fputc(cart.fcr[3], fd);

    /* Save SRAM */
    fwrite(cart.sram, 0x8000, 1, fd);

    /* Save Z80 context */
    fwrite(Z80_Context, sizeof(Z80_Regs), 1, fd);

    /* Save SN76489 context */
    fwrite(SN76489_GetContextPtr(0), SN76489_GetContextSize(), 1, fd);
}

void system_load_state(FILE *fd)
{
    int i;
    uint8 *buf;
    char id[4];
    uint16 version;

    /* Initialize everything */
    system_reset();

    /* Read header */
    fread(id, 4, 1, fd);
    fread(&version, 2, 1, fd);

    /* Load VDP context */
    fread(&vdp, sizeof(vdp_t), 1, fd);

    /* Load SMS context */
    fread(&sms, sizeof(sms_t), 1, fd);

    cart.fcr[0] = fgetc(fd);
    cart.fcr[1] = fgetc(fd);
    cart.fcr[2] = fgetc(fd);
    cart.fcr[3] = fgetc(fd);

    /* Load SRAM content */
    fread(cart.sram, 0x8000, 1, fd);

    /* Load Z80 context */
    fread(Z80_Context, sizeof(Z80_Regs), 1, fd);

    /* Load SN76489 context */
    fread(SN76489_GetContextPtr(0), SN76489_GetContextSize(), 1, fd);

    if ((sms.console != CONSOLE_COLECO) && (sms.console != CONSOLE_SG1000))
    {
        // Cartridge by default
        slot.rom = cart.rom;
        slot.pages = cart.pages;
        slot.mapper = cart.mapper;
        slot.fcr = &cart.fcr[0];

        // Restore mapping
        mapper_reset();
        cpu_readmap[0]  = &slot.rom[0];
        if (slot.mapper != MAPPER_KOREA_MSX)
        {
            mapper_16k_w(0, slot.fcr[0]);
            mapper_16k_w(1, slot.fcr[1]);
            mapper_16k_w(2, slot.fcr[2]);
            mapper_16k_w(3, slot.fcr[3]);
        }
        else
        {
            mapper_8k_w(0, slot.fcr[0]);
            mapper_8k_w(1, slot.fcr[1]);
            mapper_8k_w(2, slot.fcr[2]);
            mapper_8k_w(3, slot.fcr[3]);
        }
    }

    /* Force full pattern cache update */
    bg_list_index = 0x200;
    for(i = 0; i < 0x200; i++)
    {
        bg_name_list[i] = i;
        bg_name_dirty[i] = 1;
    }

    /* Restore palette */
    for(i = 0; i < REAL_PALETTE_SIZE; i++)
    {
        palette_sync(i);
    }
}
