/*
    sound.c --
    Sound emulation.
*/
#include "shared.h"
#include "sound.h"

snd_t snd;
int16 *psg;
static int16 **psg_buffer;
int ask_sound;
int16 snd_buffer1[2048] ALIGN;
int16 snd_buffer2[2048] ALIGN;
struct MsgPort *mp;
struct AHIRequest *io;
int AHI_device_opened;
struct Library *AHIBase = NULL;
struct AHIAudioCtrl *actrl;
char name[64];
unsigned int BufferLen = 0;
int audio_playing = FALSE;
int buf_len;
struct AHISampleInfo Sample0 =
{
    AHIST_M16S,
    NULL,
    NULL,
};

struct AHISampleInfo Sample1 =
{
    AHIST_M16S,
    NULL,
    NULL,
};

#ifdef __VAMPIRE__

extern unsigned char *Screen;

#else

extern unsigned short *Screen;

#endif

int flip_flop;

/* Generic PSG stereo mixer callback */
unsigned int SoundFunc(void)
{
    int i;
    short *dest_1;

    if(snd.enabled)
    {
        sound_update();
        if(flip_flop == 0)
        {
            dest_1 = (short *) Sample0.ahisi_Address;
            for(i = 0; i < snd.sample_count; i++)
            {
                *dest_1++ = psg_buffer[0][i] << 3;
                *dest_1++ = psg_buffer[1][i] << 3;
            }
            AHI_SetSound(0, 1, 0, 0, actrl, NULL);
        }
        else
        {
            dest_1 = (short *) Sample1.ahisi_Address;
            for(i = 0; i < snd.sample_count; i++)
            {
                *dest_1++ = psg_buffer[0][i] << 3;
                *dest_1++ = psg_buffer[1][i] << 3;
            }
            AHI_SetSound(0, 0, 0, 0, actrl, NULL);
        }
        flip_flop = !flip_flop;
    }
    return NULL;
}

struct Hook SoundHook =
{
    0,0,
    SoundFunc,
    NULL,
    NULL,
};

int sound_init(void)
{
    int i;
    
    buf_len = snd.fps;

    /* Disable sound until initialization is complete */
    snd.enabled = FALSE;
    AHI_device_opened = FALSE;

    /* Calculate number of samples generated per frame */
    snd.sample_count = (snd.sample_rate / buf_len);

    /* Calculate size of sample buffer */
    snd.buffer_size = snd.sample_count << 1;

    /* Allocate emulated sound streams */
    for(i = 0; i < STREAM_MAX; i++)
    {
        snd.stream[i] = AllocVec(snd.buffer_size, MEMF_CHIP | MEMF_CLEAR);
        if(!snd.stream[i])
        {
            return 0;
        }
    }

    psg_buffer = (int16 **) &snd.stream[STREAM_PSG_L];

    /* Set up SN76489 emulation */
    SN76489_Init(snd.sample_rate);

    mp = CreateMsgPort();
    if(!mp)
    {
        return 0;
    }

    io = (struct AHIRequest *) CreateIORequest(mp, sizeof(struct AHIRequest));
    if(!io)
    {
        return 0;
    }

    io->ahir_Version = 4;

    if(OpenDevice(AHINAME, AHI_NO_UNIT, (struct IORequest *) io, NULL) != 0)
    {
        return 0;
    }

    AHI_device_opened = TRUE;
    AHIBase = (struct Library *) io->ahir_Std.io_Device;

    actrl = AHI_AllocAudio(AHIA_AudioID, AHI_DEFAULT_ID,
                           AHIA_MixFreq, snd.sample_rate,
                           AHIA_Channels, 2,
                           AHIA_Sounds, 2,
                            AHIA_SoundFunc, &SoundHook,
                           TAG_DONE
                          );
    if(actrl)
    {
        unsigned int obtainedMixingfrequency;
        unsigned int playsamples;
        AHI_GetAudioAttrs(AHI_INVALID_ID, actrl, AHIDB_MaxPlaySamples, &playsamples, TAG_DONE);
        AHI_ControlAudio(actrl, AHIC_MixFreq_Query, &obtainedMixingfrequency, TAG_DONE);

        BufferLen = playsamples;

        Sample0.ahisi_Type = AHIST_S16S;
        Sample0.ahisi_Length = BufferLen;
        Sample0.ahisi_Address = AllocVec(BufferLen << 2, MEMF_PUBLIC | MEMF_CLEAR);
        if(Sample0.ahisi_Address)
        {
            Sample1.ahisi_Type = AHIST_S16S;
            Sample1.ahisi_Length = BufferLen;
            Sample1.ahisi_Address = AllocVec(BufferLen << 2, MEMF_PUBLIC | MEMF_CLEAR);
            if(Sample1.ahisi_Address)
            {
                if(!AHI_LoadSound(0, AHIST_DYNAMICSAMPLE, &Sample0, actrl))
                {
                    if(!AHI_LoadSound(1, AHIST_DYNAMICSAMPLE, &Sample1, actrl))
                    {
                        AHI_SetFreq(0, obtainedMixingfrequency, actrl, AHISF_IMM);
                        AHI_SetVol(0, 0x10000, 0x8000, actrl, AHISF_IMM);
                        AHI_SetSound(0, 0, 0, 0, actrl, AHISF_IMM);
                        return 1;
                    }
                }
            }
        }
        return 0;
    }
    return 0;
}

void sound_shutdown(void)
{
    int i;

    /* Shut down SN76489 emulation */
    SN76489_Shutdown();

    if(actrl)
    {
        if(audio_playing)
        {
            AHI_ControlAudio(actrl, AHIC_Play, FALSE, TAG_DONE);
        }
        audio_playing = FALSE;
        AHI_FreeAudio(actrl);
        actrl = NULL;
    }

    if(AHI_device_opened && io)
    {
        CloseDevice((struct IORequest *) io);
        AHI_device_opened = FALSE;
    }

    if(io)
    {
        DeleteIORequest((struct IORequest *) io);
        io = NULL;
    }

    if(mp)
    {
        DeleteMsgPort(mp);
        mp = NULL;
    }

    if(Sample1.ahisi_Address)
    {
        FreeVec(Sample1.ahisi_Address);
        Sample1.ahisi_Address = NULL;
    }
    if(Sample0.ahisi_Address)
    {
        FreeVec(Sample0.ahisi_Address);
        Sample0.ahisi_Address = NULL;
    }

    /* Free emulated sound streams */
    for(i = 0; i < STREAM_MAX; i++)
    {
        if(snd.stream[i])
        {
            FreeVec(snd.stream[i]);
            snd.stream[i] = NULL;
        }
    }

}

void sound_play(void)
{
    if(actrl)
    {
        if(AHI_ControlAudio(actrl, AHIC_Play, TRUE, TAG_DONE) == AHIE_OK)
        {
            audio_playing = TRUE;
            // Inform other functions that we can use sound
            snd.enabled = TRUE;
        }
    }
}

void sound_reset(void)
{
    if(!snd.enabled)
    {
        return;
    }

    /* Reset SN76489 emulator */
    SN76489_Reset();
}

void sound_update()
{
    int16 *psg[2];

    // Do a tiny bit
    psg[0] = psg_buffer[0];
    psg[1] = psg_buffer[1];
    
    // Generate SN76489 sample data
    SN76489_Update(psg, snd.sample_count);
}

/*--------------------------------------------------------------------------*/
/* Sound chip access handlers                                               */
/*--------------------------------------------------------------------------*/

void psg_stereo_w(int data)
{
    if(!snd.enabled)
    {
        return;
    }
    SN76489_GGStereoWrite(data);
}

void psg_write(int data)
{
    if(!snd.enabled)
    {
        return;
    }
    SN76489_Write(data);
}
