#ifndef _VDP_H_
#define _VDP_H_

#include <stdio.h>
#include "shared.h"

/*
    vdp1

    mode 4 when m4 set and m1 reset

    vdp2

    mode 4 when m4 set and m2,m1 != 1,0

*/

/* Display timing (NTSC) */

#define MASTER_CLOCK 3579545
#define LINES_PER_FRAME 262
#define FRAMES_PER_SECOND 60

/* VDP context */
typedef struct
{
    uint8 vram[0x4000];
    uint8 cram[0x40]; 
    uint8 reg[0x10];
    uint8 status;
    uint8 latch;
    uint8 pending;
    uint8 buffer;
    uint8 code;
    uint16 addr;
    int pn, ct, pg, sa, sg;
    int ntab;
    int satb;
    int line;
    int left;
    uint8 height;
    uint8 extended;
    uint8 mode;
    uint8 vint_pending;
    uint8 hint_pending;
    uint16 cram_latch;
    int spr_col;
    uint8 bd;
    int irq;
    int vscroll;
    int lpf;
    int spr_ovr;
} vdp_t;

/* Global data */
extern vdp_t vdp ALIGN;
extern uint8 hc_256[228];

/*** Vertical Counter Tables ***/
extern uint8 *vc_table[2][3];
/*** horizontal Counter Tables ***/
//extern uint8 *hc_table[2][1];

/* Function prototypes */
void vdp_init(void);
void vdp_shutdown(void);
void vdp_reset(void);
uint8 vdp_counter_r(uint8 offset);
uint8 vdp_read(uint8 offset);
void vdp_write(uint8 offset, uint8 data);
void gg_vdp_write(uint8 offset, uint8 data);
void tms_write(uint8 offset, uint8 data);
void viewport_check(void);

#endif /* _VDP_H_ */
