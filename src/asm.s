_LVOSuperState	        equ	    -150
_LVOUserState	        equ	    -156
_LVOOldOpenLibrary      equ     -408
_LVOCloseLibrary        equ     -414

_LVOReadJoyPort         equ     -30
_LVOSystemControlA      equ     -72
_LVOSetJoyPortAttrsA    equ     -132

TAG_USER                equ     $80000000
SJA_Dummy               equ     (TAG_USER+$00C00100)
SJA_Type                equ     (SJA_Dummy+1)

SCON_Dummy              equ     (TAG_USER+$00C00000)
SCON_TakeOverSys        equ     (SCON_Dummy+0)
SCON_KillReq            equ     (SCON_Dummy+1)
SCON_CDReboot           equ     (SCON_Dummy+2)
SCON_StopInput          equ     (SCON_Dummy+3)
SCON_RemCreateKeys      equ     (SCON_Dummy+5)

CDReboot_Off            equ     0
CDReboot_Default        equ     2

SJA_TYPE_GAMECTLR       equ     1
SJA_TYPE_AUTOSENSE      equ     0

                        xdef    _setup_context
                        xdef    _restore_context
                        xdef    _wait_sync

            IFD __VAMPIRE__

                        xdef    _copy_pixels_160
                        xdef    _copy_pixels_256
                        xdef    _load_palette

            ELSE

                        xdef    _copy_pixels

            ENDC

                        xdef    _copy_tms_bg_m2
                        xdef    _copy_tms_bg_m0
                        xdef    _render_obj_tms_8x8
                        xdef    _render_obj_tms_8x8_zoomed
                        xdef    _render_obj_tms_16x16
                        xdef    _render_obj_tms_16x16_zoomed
                        xdef    _render_bg_sms_line
                        xdef    _render_obj_sms_line
                        xdef    _render_obj_sms_line_double
                        xdef    _pad_read
                        xdef    _key_read

                        opt      o+
                        opt      all+

_setup_context:         movem.l d1/a0/a1/a6,-(a7)

                        move.l  4.w,a6
                        lea     lowlevelname(pc),a1
                        jsr     _LVOOldOpenLibrary(a6)
                        move.l  d0,lowlevelbase
                        beq.b   .no_lowlevel
                        move.l  d0,a6
                        lea     tag_list_off(pc),a1
                        jsr     _LVOSystemControlA(a6)
                        moveq   #0,d0
                        lea     tag_list_ctrl_pad(pc),a1
                        jsr     _LVOSetJoyPortAttrsA(a6)
                        moveq   #1,d0
                        lea     tag_list_ctrl_pad(pc),a1
                        jsr     _LVOSetJoyPortAttrsA(a6)
.no_lowlevel:
                        move.w  $dff002,old_dma
                        move.w  $dff01c,old_intena
                        move.w  #$7fff,d1
                        move.w  d1,$dff096
                        ; AHI needs software + audio interrupts
                        move.w  #%111100001111011,d1
                        move.w  d1,$dff09a
                        move.w  d1,$dff09c

                        move.l  4.w,a6
                        jsr     _LVOSuperState(a6)

            IFD __VAMPIRE__

                        move    sr,d1
                        or.w    #$800,d1
                        move    d1,sr

            ENDC

                        movec   vbr,d1
                        move.l  d1,our_vbr
                        jsr     _LVOUserState(a6)

                        lea     $bfed01,a0
                        clr.b   ($e01-$d01,a0)
                        move.b  #$8e,($401-$d01,a0)
                        clr.b   ($501-$d01,a0)
                        move.b  #$7f,(a0)
                        tst.b   (a0)
                        move.b  #%10001011,(a0)

                        move.l  our_vbr(pc),a0
                        move.l  $68(a0),old_lev2_irq
                        move.l  $6c(a0),old_lev3_irq
                        move.l  #lev2_irq,$68(a0)
                        move.l  #lev3_irq,$6c(a0)
                        move.w  #0,$dff1dc

            IFD __VAMPIRE__

                        move.w  #%10000,$dff1fc

            ENDC
                        move.w  #$c028,$dff09a
                        move.w  #$820f,$dff096

                        movem.l (a7)+,d1/a0/a1/a6
                        rts

_restore_context:       movem.l d0/a0,-(a7)
                        move.w  #$7fff,d0
                        move.w  d0,$dff096
                        move.w  d0,$dff09a
                        move.w  d0,$dff09c
                        move.l  our_vbr(pc),a0
                        move.l  old_lev2_irq(pc),$68(a0)
                        move.l  old_lev3_irq(pc),$6c(a0)
                        lea     $bfed01,a0
                        move.w  old_dma(pc),d0
                        or.w    #$8000,d0
                        move.w  d0,$dff096
                        move.w  old_intena(pc),d0
                        or.w    #$c000,d0
                        move.w  d0,$dff09a

                        move.l  lowlevelbase(pc),d0
                        beq.b   .no_lowlevel_lib
                        move.l  d0,a6
                        moveq   #0,d0
                        lea     tag_list_ctrl_exit(pc),a1
                        jsr     _LVOSetJoyPortAttrsA(a6)
                        moveq   #1,d0
                        lea     tag_list_ctrl_exit(pc),a1
                        jsr     _LVOSetJoyPortAttrsA(a6)
                        lea     tag_list_on(pc),a1
                        jsr     _LVOSystemControlA(a6)
                        lea     (a6),a1
                        move.l  4.w,a6
                        jsr     _LVOCloseLibrary(a6)
.no_lowlevel_lib:
                        movem.l (a7)+,d0/a0
                        rts

lev2_irq:               movem.l d0/a0,-(a7)
                        lea	    $bfee01,a0
                        move.b	-$100(a0),d0
;	                    bpl.b	.i2exit
.chkta                  lsr.w	#1,d0
                        bcc.b	.chktb
                        clr.b	(a0)
.chktb                  lsr.w	#1,d0
                        bcc.b	.chksp
                        ;TB handler
                        nop
.chksp                  lsr.w	#2,d0
                        bcc.b	.i2exit
                        move.b	-$200(a0),d0
                        move.b	#$59,(a0)
                        not.b	d0
                        ror.b	#1,d0
                        move.b	d0,buf_key
.i2exit                 moveq	#8,d0
                        lea	    $dff09c,a0
                        move.w	d0,(a0)
                        move.w	d0,(a0)
                        movem.l (a7)+,d0/a0
                        rte

lev3_irq:               st.b    vsynchro
                        move.w  #$20,$dff09c
                        rte

            IFD __VAMPIRE__

_copy_pixels_160:       
                        REPT    10
                        move16  (a0)+,(a1)+
                        ENDR
                        rts

_copy_pixels_256:
                        REPT    16
                        move16  (a0)+,(a1)+
                        ENDR
                        rts

            ELSE

_copy_pixels:           move.l  d1,-(a7)
                        asr.w   #5,d0
                        subq.w  #1,d0
                        moveq   #0,d1
.copy:
                        REPT    32
                        move.b  (a0)+,d1
                        move.w  (a1,d1.w*2),(a2)+
                        ENDR
                        dbf     d0,.copy
                        move.l  (a7)+,d1
                        rts

            ENDC

            IFD __VAMPIRE__

_load_palette:          movem.l d2/d3/a0,-(a7)
                        lea     $dff388,a0
                        ror.l   #8,d0
                        move.l  #$20000000,d3
                        move.l  d1,d2
                        add.l   d0,d2
                        move.l  d2,(a0)
                        add.l   d3,d0
                        move.l  d1,d2
                        add.l   d0,d2
                        move.l  d2,(a0)
                        add.l   d3,d0
                        move.l  d1,d2
                        add.l   d0,d2
                        move.l  d2,(a0)
                        add.l   d3,d0
                        move.l  d1,d2
                        add.l   d0,d2
                        move.l  d2,(a0)
                        add.l   d3,d0
                        move.l  d1,d2
                        add.l   d0,d2
                        move.l  d2,(a0)
                        add.l   d3,d0
                        move.l  d1,d2
                        add.l   d0,d2
                        move.l  d2,(a0)
                        add.l   d3,d0
                        move.l  d1,d2
                        add.l   d0,d2
                        move.l  d2,(a0)
                        add.l   d3,d0
                        move.l  d1,d2
                        add.l   d0,d2
                        move.l  d2,(a0)
                        movem.l (a7)+,d2/d3/a0
                        rts

            ENDC

_copy_tms_bg_m0:        movem.l d3-d5/d7/a5/a6,-(a7)
                        ; d0 = vdp.bd
                        ; d1 = pn
                        ; a0 = tms_lookup
                        ; a1 = ct
                        ; a2 = pg
                        ; a3 = bp_expand
                        ; a4 = lb
                        lsl.l   #8,d0
                        lea     (a0,d0.l*2),a0
                        move.l  d1,a5
                        move.l  a3,d5
                        moveq   #0,d1
                        moveq   #0,d3
                        moveq   #0,d4
                        moveq   #32-1,d7
.copy:                  move.b  (a5)+,d1
                        move.b  d1,d4
                        lsr.b   #3,d4
                        move.b  (a1,d4.w),d3
                        lea     (a0,d3.w*2),a6
                        move.b  (a2,d1.w*8),d4
                        move.l  d5,a3
                        lea     (a3,d4.w*8),a3
                        move.b  (a3)+,d3
                        move.b  (a6,d3.w),(a4)+
                        move.b  (a3)+,d3
                        move.b  (a6,d3.w),(a4)+
                        move.b  (a3)+,d3
                        move.b  (a6,d3.w),(a4)+
                        move.b  (a3)+,d3
                        move.b  (a6,d3.w),(a4)+
                        move.b  (a3)+,d3
                        move.b  (a6,d3.w),(a4)+
                        move.b  (a3)+,d3
                        move.b  (a6,d3.w),(a4)+
                        move.b  (a3)+,d3
                        move.b  (a6,d3.w),(a4)+
                        move.b  (a3),d3
                        move.b  (a6,d3.w),(a4)+
                        dbf     d7,.copy
                        movem.l (a7)+,d3-d5/d7/a5/a6
                        rts

_copy_tms_bg_m2:        movem.l d3-d5/d7/a5/a6,-(a7)
                        ; d0 = vdp.bd
                        ; d1 = pn
                        ; a0 = tms_lookup
                        ; a1 = ct
                        ; a2 = pg
                        ; a3 = bp_expand
                        ; a4 = lb
                        lsl.l   #8,d0
                        lea     (a0,d0.l*2),a0
                        move.l  d1,a5
                        move.l  a3,d5
                        moveq   #0,d1
                        moveq   #0,d3
                        moveq   #0,d4
                        moveq   #32-1,d7
.copy:                  move.b  (a5)+,d1
                        move.b  (a1,d1.w*8),d3
                        lea     (a0,d3.w*2),a6
                        move.b  (a2,d1.w*8),d4
                        move.l  d5,a3
                        lea     (a3,d4.w*8),a3
                        move.b  (a3)+,d3
                        move.b  (a6,d3.w),(a4)+
                        move.b  (a3)+,d3
                        move.b  (a6,d3.w),(a4)+
                        move.b  (a3)+,d3
                        move.b  (a6,d3.w),(a4)+
                        move.b  (a3)+,d3
                        move.b  (a6,d3.w),(a4)+
                        move.b  (a3)+,d3
                        move.b  (a6,d3.w),(a4)+
                        move.b  (a3)+,d3
                        move.b  (a6,d3.w),(a4)+
                        move.b  (a3)+,d3
                        move.b  (a6,d3.w),(a4)+
                        move.b  (a3),d3
                        move.b  (a6,d3.w),(a4)+
                        dbf     d7,.copy
                        movem.l (a7)+,d3-d5/d7/a5/a6
                        rts

;                for(x = start; x < end; x++)
;                {
;                    if(ex[0][x])
;                    {
;                        /* Check sprite collision */
;                        if ((lb[x] & 0x40) && !(vdp.status & 0x20))
;                        {
;                            /* pixel-accurate SPR_COL flag */
;                            vdp.status |= 0x20;
;                            vdp.spr_col = (line << 8) | ((p->xpos + x + 13) >> 1);
;                        }
;                        lb[x] = lut[lb[x]];
;                    }
;                }

_render_obj_tms_8x8:    movem.l d4-d7/a5,-(a7)
                        ; d0 = start
                        ; d1 = end
                        ; d2 = line
                        ; d3 = p->xpos
                        ; a0 = &vdp.status
                        ; a1 = &vdp.spr_col
                        ; a2 = ex.b
                        ; a3 = lb.b
                        ; a4 = lut.b
                        lsl.w   #8,d2
                        moveq   #0,d5
                        moveq   #$20,d6
                        moveq   #$40,d7
                        add.l   d0,a3
                        move.l  (a2),a5
                        add.l   d0,a5
.copy:                  move.b  (a3),d5
                        tst.b   (a5)+
                        beq.b   .no_pixel
                        move.b  d5,d4
                        and.b   d7,d4
                        beq.b   .no_collision
                        move.l  (a0),d4
                        and.b   d6,d4
                        bne.b   .no_collision
                        or.l    d6,(a0)
                        move.l  d3,d4
                        add.w   d0,d4
                        add.w   #13,d4
                        lsr.w   #1,d4
                        or.w    d2,d4
                        move.l  d4,(a1)
.no_collision:          move.b  (a4,d5.w),(a3)
.no_pixel:              addq.w  #1,d0
                        addq.w  #1,a3
                        cmp.w   d1,d0
                        bne.b   .copy
                        movem.l (a7)+,d4-d7/a5
                        rts

;                for(x = start; x < end; x++)
;                {
;                    if(ex[0][x >> 1])
;                    {
;                        /* Check sprite collision */
;                        if ((lb[x] & 0x40) && !(vdp.status & 0x20))
;                        {
;                            /* pixel-accurate SPR_COL flag */
;                            vdp.status |= 0x20;
;                            vdp.spr_col = (line << 8) | ((p->xpos + x + 13) >> 1);
;                        }
;                        lb[x] = lut[lb[x]];
;                    }
;                }

_render_obj_tms_8x8_zoomed:
                        movem.l d4-d7/a5,-(a7)
                        ; d0 = start
                        ; d1 = end
                        ; d2 = line
                        ; d3 = p->xpos
                        ; a0 = &vdp.status
                        ; a1 = &vdp.spr_col
                        ; a2 = ex.b
                        ; a3 = lb.b
                        ; a4 = lut.b
                        lsl.w   #8,d2
                        moveq   #0,d5
                        moveq   #$20,d6
                        moveq   #$40,d7
                        add.l   d0,a3
                        move.l  (a2),a5
.copy:                  move.w  d0,d4
                        lsr.w   #1,d4
                        move.b  (a3),d5
                        tst.b   (a5,d4.w)
                        beq.b   .no_pixel
                        move.b  d5,d4
                        and.b   d7,d4
                        beq.b   .no_collision
                        move.l  (a0),d4
                        and.b   d6,d4
                        bne.b   .no_collision
                        or.l    d6,(a0)
                        move.l  d3,d4
                        add.w   d0,d4
                        add.w   #13,d4
                        lsr.w   #1,d4
                        or.w    d2,d4
                        move.l  d4,(a1)
.no_collision:          move.b  (a4,d5.w),(a3)
.no_pixel:              addq.w  #1,d0
                        addq.w  #1,a3
                        cmp.w   d1,d0
                        bne.b   .copy
                        movem.l (a7)+,d4-d7/a5
                        rts

;                for(x = start; x < end; x++)
;                {
;                    if(ex[(x >> 3) & 1][x & 7])
;                    {
;                        /* Check sprite collision */
;                        if ((lb[x] & 0x40) && !(vdp.status & 0x20))
;                        {
;                            /* pixel-accurate SPR_COL flag */
;                            vdp.status |= 0x20;
;                            vdp.spr_col = (line << 8) | ((p->xpos + x + 13) >> 1);
;                        }
;                        lb[x] = lut[lb[x]];
;                    }
;                }

_render_obj_tms_16x16:  movem.l d4-d7/a5,-(a7)
                        ; d0 = start
                        ; d1 = end
                        ; d2 = line
                        ; d3 = p->xpos
                        ; a0 = &vdp.status
                        ; a1 = &vdp.spr_col
                        ; a2 = ex.b
                        ; a3 = lb.b
                        ; a4 = lut.b
                        lsl.w   #8,d2
                        moveq   #$20,d6
                        moveq   #$40,d7
                        add.l   d0,a3
.copy:                  move.w  d0,d4
                        and.w   #7,d4
                        bfextu  d0{28:1},d5
                        move.l  (a2,d5.w*4),a5
                        move.b  (a3),d5
                        tst.b   (a5,d4.w)
                        beq.b   .no_pixel
                        move.b  d5,d4
                        and.b   d7,d4
                        beq.b   .no_collision
                        move.l  (a0),d4
                        and.b   d6,d4
                        bne.b   .no_collision
                        or.l    d6,(a0)
                        move.l  d3,d4
                        add.w   d0,d4
                        add.w   #13,d4
                        asr.w   #1,d4
                        or.w    d2,d4
                        move.l  d4,(a1)
.no_collision:          move.b  (a4,d5.w),(a3)
.no_pixel:              addq.w  #1,d0
                        addq.w  #1,a3
                        cmp.w   d1,d0
                        bne.b   .copy                       
                        movem.l (a7)+,d4-d7/a5
                        rts

;                for(x = start; x < end; x++)
;                {
;                    if(ex[(x >> 4) & 1][(x >> 1) & 7])
;                    {
;                        /* Check sprite collision */
;                        if ((lb[x] & 0x40) && !(vdp.status & 0x20))
;                        {
;                            /* pixel-accurate SPR_COL flag */
;                            vdp.status |= 0x20;
;                            vdp.spr_col = (line << 8) | ((p->xpos + x + 13) >> 1);
;                        }
;                        lb[x] = lut[lb[x]];
;                    }
;                }

_render_obj_tms_16x16_zoomed: 
                        movem.l d4-d7/a5,-(a7)
                        ; d0 = start
                        ; d1 = end
                        ; d2 = line
                        ; d3 = p->xpos
                        ; a0 = &vdp.status
                        ; a1 = &vdp.spr_col
                        ; a2 = ex.b
                        ; a3 = lb.b
                        ; a4 = lut.b
                        lsl.w   #8,d2
                        moveq   #$20,d6
                        moveq   #$40,d7
                        add.l   d0,a3
.copy:                  bfextu  d0{28:3},d4
                        bfextu  d0{27:1},d5
                        move.l  (a2,d5.w*4),a5
                        move.b  (a3),d5
                        tst.b   (a5,d4.w)
                        beq.b   .no_pixel
                        move.b  d5,d4
                        and.b   d7,d4
                        beq.b   .no_collision
                        move.l  (a0),d4
                        and.b   d6,d4
                        bne.b   .no_collision
                        or.l    d6,(a0)
                        move.l  d3,d4
                        add.w   d0,d4
                        add.w   #13,d4
                        asr.w   #1,d4
                        or.w    d2,d4
                        move.l  d4,(a1)
.no_collision:          move.b  (a4,d5.w),(a3)
.no_pixel:              addq.w  #1,d0
                        addq.w  #1,a3
                        cmp.w   d1,d0
                        bne.b   .copy
                        movem.l (a7)+,d4-d7/a5
                        rts


;    /* Draw a line of the background */
;    for(; column < 32; column++)
;    {
;        /* Stop vertical scrolling for leftmost eight columns */
;        if((vdp.reg[0] & 0x80) && (!locked) && (column >= 24))
;        {
;            locked = 1;
;            v_row = (line & 7) << 3;
;        }
;
;        /* Get name table attribute word */
;        attr = nt[(column + nt_scroll) & 0x1F];
;        attr = ((attr & 0xff) << 8) | ((attr & 0xff00) >> 8);
;
;        /* Expand priority and palette bits */
;        atex_mask = atex[(attr >> 11) & 3];
;
;        /* Point to a line of pattern data in cache */
;        cache_ptr = (uint32 *) &bg_pattern_cache[((attr & 0x7FF) << 6) | (v_row)];
;    
;        /* Copy the left half, adding the attribute bits in */
;        linebuf_ptr[(column << 1)] = cache_ptr[0] | (atex_mask);
;        
;        /* Copy the right half, adding the attribute bits in */
;        linebuf_ptr[(column << 1) | 1] = cache_ptr[1] | (atex_mask);
;    }

_render_bg_sms_line:    movem.l d4-d7/a5/a6,-(a7)
                        ; d0 = column
                        ; d1 = vdp.reg[0]
                        ; d2 = line
                        ; d3 = nt_scroll
                        ; a0 = nt
                        ; a1 = atex
                        ; a2 = bg_pattern_cache
                        ; a3 = linebuf_ptr
                        ; a4 = v_row
                        move.l  d0,a6
                        move.l  (a6),d0
                        moveq   #0,d4
                        and.l   #7,d2
                        lsl.w   #3,d2

            IFD __VAMPIRE__

                        lea     $7ff.w,a5

            ENDC

.loop                   move.l  d1,d5
                        and.w   #$80,d5
                        beq.b   .stop_vert
                        tst.b   d4
                        bne.b   .stop_vert
                        cmp.w   #24,d0
                        blt.b   .stop_vert
                        st.b    d4
                        move.l  d2,(a4)
.stop_vert:             move.w  d3,d5
                        add.w   d0,d5
                        and.w   #$1f,d5

            IFD __VAMPIRE__

                        movex.w (a0,d5.w*2),d5

            ELSE

                        move.w  (a0,d5.w*2),d5
                        ror.w   #8,d5

            ENDC
    
                        move.w  d5,d6
                        moveq   #11,d7
                        lsr.w   d7,d6
                        and.l   #3,d6

            IFD __VAMPIRE__

                        and.l   a5,d5

            ELSE

                        and.l   #$7ff,d5
                
            ENDC

                        lsl.l   #6,d5
                        or.l    (a4),d5
                        move.l  (a1,d6.l*4),d6
                        movem.l (a2,d5.l),d5/d7
                        or.l    d6,d5
                        or.l    d6,d7
                        movem.l d5/d7,(a3,d0.l*8)
                        addq.l  #1,d0
                        cmp.l   #32,d0
                        bne.b   .loop
                        move.l  d0,(a6)
                        movem.l (a7)+,d4-d7/a5/a6
                        rts

;            /* Draw sprite line */
;            for(x = start; x < end; x++)
;            {
;                /* Source pixel from cache */
;                sp = cache_ptr[x];
;
;                /* Only draw opaque sprite pixels */
;                if(sp)
;                {
;                    /* Background pixel from line buffer */
;                    bg = linebuf_ptr[x];
;
;                    /* Look up result */
;                    linebuf_ptr[x] = lut[(bg << 8) | sp];
;
;                    /* Check sprite collision */
;                    if((bg & 0x40) && !(vdp.status & 0x20))
;                    {
;                        /* pixel-accurate SPR_COL flag */
;                        vdp.status |= 0x20;
;                        vdp.spr_col = (line << 8) | ((xp + x + 13) >> 1);
;                    }
;                }
;            }

_render_obj_sms_line:   movem.l d4/d5/d6/d7,-(a7)
                        ; d0 = start
                        ; d1 = end
                        ; d2 = line
                        ; d3 = xp
                        ; a0 = cache_ptr
                        ; a1 = linebuf_ptr
                        ; a2 = lut
                        ; a3 = vdp.status
                        ; a4 = vdp.spr_col
                        lsl.l   #8,d2
                        moveq   #$20,d7
.loop:                  move.b  (a0,d0.l),d4
                        beq.b   .opaque
                        move.b  (a1,d0.l),d5
                        move.b  d5,d6
                        lsl.w   #8,d6
                        or.b    d4,d6
                        move.b  (a2,d6.w),(a1,d0.l)
                        and.w   #$40,d5
                        beq.b   .opaque
                        move.l  (a3),d5
                        and.w   d7,d5
                        bne.b   .opaque
                        or.l    d7,(a3)
                        move.l  d3,d5
                        add.l   d0,d5
                        add.l   #13,d5
                        asr.l   #1,d5
                        or.l    d2,d5
                        move.l  d5,(a4)
.opaque:                addq.l  #1,d0
                        cmp.l   d1,d0
                        bne.b   .loop
                        movem.l (a7)+,d4/d5/d6/d7
                        rts

;            /* Draw sprite line (at 1/2 dot rate) */
;            for(x = start; x < end; x++)
;            {
;                /* Source pixel from cache */
;                sp = cache_ptr[(x >> 1)];
;
;                /* Only draw opaque sprite pixels */
;                if(sp)
;                {
;                    /* Background pixel from line buffer */
;                    bg = linebuf_ptr[x];
;
;                    /* Look up result */
;                    linebuf_ptr[x] = lut[(bg << 8) | (sp)];
;
;                    /* Check sprite collision */
;                    if((bg & 0x40) && !(vdp.status & 0x20))
;                    {
;                        /* pixel-accurate SPR_COL flag */
;                        vdp.status |= 0x20;
;                        vdp.spr_col = (line << 8) | ((xp + x + 13) >> 1);
;                    }
;                }

_render_obj_sms_line_double:
                        movem.l d4/d5/d6/d7,-(a7)
                        ; d0 = start
                        ; d1 = end
                        ; d2 = line
                        ; d3 = xp
                        ; a0 = cache_ptr
                        ; a1 = linebuf_ptr
                        ; a2 = lut
                        ; a3 = vdp.status
                        ; a4 = vdp.spr_col
                        lsl.l   #8,d2
                        moveq   #$20,d7
.loop:                  move.w  d0,d5
                        lsr.w   #1,d5
                        move.b  (a0,d5.l),d4
                        beq.b   .opaque
                        move.b  (a1,d0.l),d5
                        move.b  d5,d6
                        lsl.w   #8,d6
                        or.b    d4,d6
                        move.b  (a2,d6.w),(a1,d0.l)
                        and.w   #$40,d5
                        beq.b   .opaque
                        move.l  (a3),d5
                        and.w   d7,d5
                        bne.b   .opaque
                        or.l    d7,(a3)
                        move.l  d3,d5
                        add.l   d0,d5
                        add.l   #13,d5
                        asr.l   #1,d5
                        or.l    d2,d5
                        move.l  d5,(a4)
.opaque:                addq.l  #1,d0
                        cmp.l   d1,d0
                        bne.b   .loop
                        movem.l (a7)+,d4/d5/d6/d7
                        rts

PAD_BUTTON_BLUE         equ     $800000
PAD_BUTTON_RED          equ     $400000
PAD_BUTTON_YELLOW       equ     $200000
PAD_BUTTON_GREEN        equ     $100000

PAD_BUTTON_FORWARD      equ     $80000
PAD_BUTTON_REVERSE      equ     $40000
PAD_BUTTON_PLAY         equ     $20000

PAD_BUTTON_UP           equ     8
PAD_BUTTON_DOWN         equ     4
PAD_BUTTON_LEFT         equ     2
PAD_BUTTON_RIGHT        equ     1

_pad_read:              movem.l d1-a6,-(a7)
                        move.l  lowlevelbase(pc),d1
                        beq.b   .no_lowlevel
                        move.l  d1,a6
                        jsr     _LVOReadJoyPort(a6)
                        movem.l (a7)+,d1-a6
                        rts
.no_lowlevel:
                        move.b  d0,d3
                        move.w  $dff00a,d0                  ; Read Up Down Left Right
                        tst.b   d3
                        beq.b   .joy_2_reg
                        move.w  $dff00c,d0
.joy_2_reg:
                        move.w  d0,d1
                        add.w   d1,d1
                        eor.w   d0,d1
                        move.w  #$0202,d2
                        and.w   d2,d0                       ; Left/right
                        and.w   d2,d1                       ; Up/Down
                        lea     joy_dat(pc),a0
                        move.w  d0,(a0)+                    ; Left/right
                        move.w  d1,(a0)                     ; Up/Down

                        moveq   #0,d0

                        tst.b   d3
                        bne.b   .joy_2_2nd_fire
                        
                        btst.b  #6,$bfe001                  ; 1st fire button
                        bne.s   .joy_1_no_fire
                        or.l    #PAD_BUTTON_RED,d0
.joy_1_no_fire:         btst    #10,$dff016                 ; 2nd fire button
                        bne.b   .joy_1_no_2nd_fire
                        or.l    #PAD_BUTTON_BLUE,d0
.joy_1_no_2nd_fire:     bra.b   .no_2nd_fire
.joy_2_2nd_fire:
                        btst.b  #7,$bfe001                  ; 1st fire button
                        bne.s   .no_fire
                        or.l    #PAD_BUTTON_RED,d0
.no_fire:               btst    #14,$dff016                 ; 2nd fire button
                        bne.b   .no_2nd_fire
                        or.l    #PAD_BUTTON_BLUE,d0
.no_2nd_fire:

            IFD __VAMPIRE__

                        btst    #10,$dff220
                        beq.b   .joy_1_3rd_fire
                        or.l    #PAD_BUTTON_YELLOW,d0
.joy_1_3rd_fire:
                        btst    #10,$dff222
                        beq.b   .joy_2_3rd_fire
                        or.l    #PAD_BUTTON_YELLOW,d0
.joy_2_3rd_fire:

            ENDC

                        lea     joy_dat(pc),a0
                        move.b  (a0)+,d1
                        beq.b   .no_joy_left
                        or.l    #PAD_BUTTON_LEFT,d0
.no_joy_left:
                        move.b  (a0)+,d1
                        beq.b   .no_joy_right
                        or.l    #PAD_BUTTON_RIGHT,d0
.no_joy_right:
                        move.b  (a0)+,d1
                        beq.b   .no_joy_up
                        or.l    #PAD_BUTTON_UP,d0
.no_joy_up:
                        move.b  (a0),d1
                        beq.b   .no_joy_down
                        or.l    #PAD_BUTTON_DOWN,d0
.no_joy_down:
                        move.w  #-1,$dff034
                        movem.l (a7)+,d1-a6
                        rts

_wait_sync:             tst.b   vsynchro(pc)
                        beq.b   _wait_sync
                        sf.b    vsynchro
                        rts

_key_read:              moveq   #0,d0
                        move.b  buf_key(pc),d0
                        rts

lowlevelbase:           dc.l    0
lowlevelname:
                        dc.b    "lowlevel.library",0
                        even
tag_list_ctrl_pad:      dc.l    SJA_Type,SJA_TYPE_GAMECTLR
                        dc.l    0
tag_list_ctrl_exit:     dc.l    SJA_Type,SJA_TYPE_AUTOSENSE
                        dc.l    0
tag_list_off:           dc.l    SCON_KillReq,1
                        dc.l    SCON_StopInput,1
                        dc.l    SCON_CDReboot,CDReboot_Off
                        dc.l    SCON_RemCreateKeys,1
                        dc.l    0
tag_list_on:            dc.l    SCON_KillReq,0
                        dc.l    SCON_StopInput,0
                        dc.l    SCON_CDReboot,CDReboot_Default
                        dc.l    0
buf_key:                dc.b    0
joy_dat:
joy_left:               dc.b    0
joy_right:              dc.b    0
joy_up:                 dc.b    0
joy_down:               dc.b    0
vsynchro:               dc.b    0
                        even
old_dma:                dc.w    0
old_intena:             dc.w    0
our_vbr:                dc.l    0
old_lev2_irq:           dc.l    0
old_lev3_irq:           dc.l    0
                        end
