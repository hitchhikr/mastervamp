#ifndef _SOUND_H_
#define _SOUND_H_

#include "shared.h"
#include <devices/ahi.h>
#include <proto/exec.h>
#include <proto/ahi.h>
#include <utility/hooks.h>
#include <clib/alib_protos.h>

enum
{
    STREAM_PSG_L,               /* PSG left channel */
    STREAM_PSG_R,               /* PSG right channel */
    STREAM_MAX                  /* Total # of sound streams */
};  

/* Sound emulation structure */
typedef struct
{
    void (*mixer_callback)(int length);
    int16 *stream[STREAM_MAX];
    int enabled;
    int fps;
    int buffer_size;
    int sample_count;
    int sample_rate;
    int next_frame;
    uint32 psg_clock;
} snd_t;

/* Global data */
extern snd_t snd ALIGN;

/* Function prototypes */
void psg_write(int data);
void psg_stereo_w(int data);
int sound_init(void);
void sound_play(void);
void sound_shutdown(void);
void sound_reset(void);
void sound_update(void);

#endif /* _SOUND_H_ */
