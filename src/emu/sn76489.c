/* 
    SN76489 emulation
    by Maxim in 2001 and 2002
    converted from my original Delphi implementation

    I'm a C newbie so I'm sure there are loads of stupid things
    in here which I'll come back to some day and redo

    Includes:
    - Super-high quality tone channel "oversampling" by calculating fractional positions on transitions
    - Noise output pattern reverse engineered from actual SMS output
    - Volume levels taken from actual SMS output

    07/08/04  Charles MacDonald
    Modified for use with SMS Plus:
    - Added support for multiple PSG chips.
    - Added reset/config/update routines.
    - Added context management routines.
    - Removed SN76489_GetValues().
    - Removed some unused variables.
*/

#include "shared.h"
#include "prefs.h"
#include "sound.h"

//#define LONG_MIN 0
#define NoiseInitialState 0x8000    /* Initial state of shift register */
#define PSG_CUTOFF 0x6              /* Value below which PSG does not output */

/* These values are taken from a real SMS2's output */
#define VOL100(x) (x * 100) / 100
#define VOL90(x) (x * 90) / 100
#define VOL80(x) (x * 80) / 100
#define VOL70(x) (x * 70) / 100
#define VOL60(x) (x * 60) / 100
#define VOL50(x) (x * 50) / 100
#define VOL40(x) (x * 40) / 100
#define VOL30(x) (x * 30) / 100
#define VOL20(x) (x * 20) / 100
#define VOL10(x) (x * 10) / 100
#define VOL00(x) (x * 0) / 100

#ifdef __VAMPIRE__

extern unsigned char *Screen;

#else

extern unsigned short *Screen;

#endif

static const ALIGN int PSGVolumeValues[][16] =
{
	{
      VOL00(892), VOL00(774), VOL00(669), VOL00(575), VOL00(492), VOL00(417), VOL00(351), VOL00(292),
      VOL00(239), VOL00(192), VOL00(150), VOL00(113), VOL00(80),  VOL00(50),  VOL00(24),  VOL00(0)
    },

	{
      VOL10(892), VOL10(774), VOL10(669), VOL10(575), VOL10(492), VOL10(417), VOL10(351), VOL10(292),
      VOL10(239), VOL10(192), VOL10(150), VOL10(113), VOL10(80),  VOL10(50),  VOL10(24),  VOL10(0)
    },

	{
      VOL20(892), VOL20(774), VOL20(669), VOL20(575), VOL20(492), VOL20(417), VOL20(351), VOL20(292),
      VOL20(239), VOL20(192), VOL20(150), VOL20(113), VOL20(80),  VOL20(50),  VOL20(24),  VOL20(0)
    },

	{
      VOL30(892), VOL30(774), VOL30(669), VOL30(575), VOL30(492), VOL30(417), VOL30(351), VOL30(292),
      VOL30(239), VOL30(192), VOL30(150), VOL30(113), VOL30(80),  VOL30(50),  VOL30(24),  VOL30(0)
    },

	{
      VOL40(892), VOL40(774), VOL40(669), VOL40(575), VOL40(492), VOL40(417), VOL40(351), VOL40(292),
      VOL40(239), VOL40(192), VOL40(150), VOL40(113), VOL40(80),  VOL40(50),  VOL40(24),  VOL40(0)
    },

	{
      VOL50(892), VOL50(774), VOL50(669), VOL50(575), VOL50(492), VOL50(417), VOL50(351), VOL50(292),
      VOL50(239), VOL50(192), VOL50(150), VOL50(113), VOL50(80),  VOL50(50),  VOL50(24),  VOL50(0)
    },

	{
      VOL60(892), VOL60(774), VOL60(669), VOL60(575), VOL60(492), VOL60(417), VOL60(351), VOL60(292),
      VOL60(239), VOL60(192), VOL60(150), VOL60(113), VOL60(80),  VOL60(50),  VOL60(24),  VOL60(0)
    },

	{
      VOL70(892), VOL70(774), VOL70(669), VOL70(575), VOL70(492), VOL70(417), VOL70(351), VOL70(292),
      VOL70(239), VOL70(192), VOL70(150), VOL70(113), VOL70(80),  VOL70(50),  VOL70(24),  VOL70(0)
    },

	{
      VOL80(892), VOL80(774), VOL80(669), VOL80(575), VOL80(492), VOL80(417), VOL80(351), VOL80(292),
      VOL80(239), VOL80(192), VOL80(150), VOL80(113), VOL80(80),  VOL80(50),  VOL80(24),  VOL80(0)
    },

	{
      VOL90(892), VOL90(774), VOL90(669), VOL90(575), VOL90(492), VOL90(417), VOL90(351), VOL90(292),
      VOL90(239), VOL90(192), VOL90(150), VOL90(113), VOL90(80),  VOL90(50),  VOL90(24),  VOL90(0)
    },

	{
      VOL100(892), VOL100(774), VOL100(669), VOL100(575), VOL100(492), VOL100(417), VOL100(351), VOL100(292),
      VOL100(239), VOL100(192), VOL100(150), VOL100(113), VOL100(80),  VOL100(50),  VOL100(24),  VOL100(0)
    },

};

int stereo0[2];
int stereo1[2];
int stereo2[2];
int stereo3[2];
extern PREFERENCES prefs;

static SN76489_Context SN76489 ALIGN;

void SN76489_ReInit(int SamplingRate)
{
    SN76489_Context *p = &SN76489;
    p->dClock = (float) snd.psg_clock / 16 / SamplingRate;
    p->Clock = 0;
    p->NumClocksForSample = 0;
}

void SN76489_Init(int SamplingRate)
{
    SN76489_Context *p = &SN76489;
    p->dClock = (float) snd.psg_clock / 16 / SamplingRate;
    SN76489_Config(MUTE_ALLON, BOOST_ON, 10, (sms.console < CONSOLE_SMS) ? FB_SC3000 : FB_SEGAVDP);
    SN76489_Reset();
    p->Ready = 1;
}

void SN76489_Reset()
{
    SN76489_Context *p = &SN76489;
    int i;

    SN76489_GGStereoWrite(0xff);

    for(i = 0; i <= 3; i++)
    {
        /* Initialise PSG state */
        p->Registers[2 * i] = 1;            /* tone freq = 1 */
        p->Registers[2 * i + 1] = 0xf;      /* vol = off */
        p->NoiseFreq = 0x10;

        /* Set counters to 0 */
        p->ToneFreqVals[i] = 0;

        /* Set flip-flops to 1 */
        p->ToneFreqPos[i] = 1;

        /* Set intermediate positions to do-not-use value */
        p->IntermediatePos[i] = LONG_MIN;
    }

    p->LatchedRegister = 0;

    /* Initialise noise generator */
    p->NoiseShiftRegister = NoiseInitialState;

    /* Zero clock */
    p->Clock = 0;
    p->NumClocksForSample = 0;
}

int SN76489_IsReady()
{
    return SN76489.Ready;
}

void SN76489_Shutdown()
{

}

void SN76489_Config(int mute, int boost, int volume, int feedback)
{
    SN76489_Context *p = &SN76489;

    p->Mute = mute;
    p->BoostNoise = boost;
    p->VolumeArray = volume;
    p->WhiteNoiseFeedback = feedback;
}

uint8 *SN76489_GetContextPtr()
{
    return (uint8 *) &SN76489;
}

int SN76489_GetContextSize(void)
{
    return sizeof(SN76489_Context);
}

void SN76489_Write(int data)
{
    SN76489_Context *p = &SN76489;

	if(data & 0x80)
	{
        /* Latch/data byte  %1 cc t dddd */
        p->LatchedRegister = ((data >> 4) & 0x07);
        p->Registers[p->LatchedRegister] =
            (p->Registers[p->LatchedRegister] & 0x3f0)          /* zero low 4 bits */
            | (data & 0xf);                                     /* and replace with data */
	}
	else
	{
        /* Data byte %0 - dddddd */
        if(!(p->LatchedRegister % 2) && (p->LatchedRegister < 5))
        {
            /* Tone register */
            p->Registers[p->LatchedRegister] =
                (p->Registers[p->LatchedRegister] & 0x00f)      /* zero high 6 bits */
                | ((data & 0x3f) << 4);                         /* and replace with data */
        }
		else
		{
            /* Other register */
            p->Registers[p->LatchedRegister] = data & 0x0f;     /* Replace with data */
        }
    }
    switch(p->LatchedRegister)
    {
	    case 0:
	    case 2:
        case 4: /* Tone channels */
            /* Zero frequency changed to 1 to avoid div/0 */
            if(p->Registers[p->LatchedRegister] == 0)
            {
                p->Registers[p->LatchedRegister] = 1;
            }
		    break;
        case 6: /* Noise */
            p->NoiseShiftRegister = NoiseInitialState;          /* reset shift register */
            p->NoiseFreq = 0x10 << (p->Registers[6] & 0x3);     /* set noise signal generator frequency */
		    break;
    }
}

void SN76489_GGStereoWrite(int data)
{
    SN76489_Context *p = &SN76489;
    p->PSGStereo = data;

    stereo0[0] = (p->PSGStereo >> 0 & 0x1);         // 1
    stereo0[1] = (p->PSGStereo >> (0 + 4) & 0x1);   // 1

    stereo1[0] = (p->PSGStereo >> 1 & 0x1);         // 1
    stereo1[1] = (p->PSGStereo >> (1 + 4) & 0x1);   // 1

    stereo2[0] = (p->PSGStereo >> 2 & 0x1);
    stereo2[1] = (p->PSGStereo >> (2 + 4) & 0x1);

    stereo3[0] = (p->PSGStereo >> 3 & 0x1);
    stereo3[1] = (p->PSGStereo >> (3 + 4) & 0x1);
}

void SN76489_Update(signed short **buffer, int length)
{
    SN76489_Context *p = &SN76489;
    int i, j;
    int dat;

    if(IS_GG)
    {
        for(j = 0; j < length; j++)
        {
            for(i = 0; i <= 2; ++i)
            {
                if(p->IntermediatePos[i] != LONG_MIN)
                {
                    p->Channels[i] = (int16) ((p->Mute >> i & 0x1) * PSGVolumeValues[p->VolumeArray][p->Registers[(i << 1) + 1]] * p->IntermediatePos[i] >> 16);
                }
                else
                {
                    p->Channels[i] = (int16) ((p->Mute >> i & 0x1) * PSGVolumeValues[p->VolumeArray][p->Registers[(i << 1) + 1]] * p->ToneFreqPos[i]);
                }
            }    
            p->Channels[3] = (int16) ((p->Mute >> 3 & 0x1) * PSGVolumeValues[p->VolumeArray][p->Registers[7]] * (p->NoiseShiftRegister & 0x1));
        
            if(p->BoostNoise) p->Channels[3] <<= 1;     /* Double noise volume to make some people happy */
        
            dat  = stereo0[0] * p->Channels[0];
            dat += stereo1[0] * p->Channels[1];
            dat += stereo2[0] * p->Channels[2];
            dat += stereo3[0] * p->Channels[3];
            buffer[0][j] = dat;

            dat  = stereo0[1] * p->Channels[0];
            dat += stereo1[1] * p->Channels[1];
            dat += stereo2[1] * p->Channels[2];
            dat += stereo3[1] * p->Channels[3];
            buffer[1][j] = dat;

            p->Clock += p->dClock;
            p->NumClocksForSample = (int) p->Clock;     /* truncates */
            p->Clock -= p->NumClocksForSample;          /* remove integer part */
    
            /* Decrement tone channel counters */
            p->ToneFreqVals[0] -= p->NumClocksForSample;
            p->ToneFreqVals[1] -= p->NumClocksForSample;
            p->ToneFreqVals[2] -= p->NumClocksForSample;
         
            /* Noise channel: match to tone2 or decrement its counter */
            if(p->NoiseFreq == 0x80) p->ToneFreqVals[3] = p->ToneFreqVals[2];
            else p->ToneFreqVals[3] -= p->NumClocksForSample;
    
            /* Tone channels: */
            for(i = 0; i <= 2; ++i)
            {
                if(p->ToneFreqVals[i] <= 0)
                {   /* If it gets below 0... */
                    if(p->Registers[i * 2] > PSG_CUTOFF)
                    {
                        /* Calculate how much of the sample is + and how much is -
                           Go to floating point and include the clock fraction for extreme accuracy :D
                           Store as long int, maybe it's faster? I'm not very good at this */
                        p->IntermediatePos[i] = (uint32) ((p->NumClocksForSample - p->Clock + (p->ToneFreqVals[i] << 1)) *
                                                          p->ToneFreqPos[i] / (p->NumClocksForSample + p->Clock) * 65536);
                        p->ToneFreqPos[i] = -p->ToneFreqPos[i]; // Flip the flip-flop
                    }
                    else
                    {
                        p->ToneFreqPos[i] = 1;          /* stuck value */
                        p->IntermediatePos[i] = LONG_MIN;
                    }
                    p->ToneFreqVals[i] += p->Registers[i * 2] * (p->NumClocksForSample / p->Registers[i * 2] + 1);
                }
                else
                {
                    p->IntermediatePos[i] = LONG_MIN;
                }
            }

            /* Noise channel */
            if(p->ToneFreqVals[3] <= 0)
            {   
                /* If it gets below 0... */
                p->ToneFreqPos[3] = -p->ToneFreqPos[3]; /* Flip the flip-flop */
                if(p->NoiseFreq != 0x80)                /* If not matching tone2, decrement counter */
                {
                    p->ToneFreqVals[3] += p->NoiseFreq * (p->NumClocksForSample / p->NoiseFreq + 1);
                }
                if(p->ToneFreqPos[3] == 1)
                {   
                    /* Only once per cycle... */
                    int Feedback;

                    if(p->Registers[6] & 0x4)
                    {   /* White noise */
                        /* Calculate parity of fed-back bits for feedback */
                        switch(p->WhiteNoiseFeedback)
                        {
                            /* Do some optimised calculations for common (known) feedback values */
                            case 0x0006:    /* SC-3000      %00000110 */
                            case 0x0009:    /* SMS, GG, MD  %00001001 */
                                /* If two bits fed back, I can do Feedback = (nsr & fb) && (nsr & fb ^ fb)
                                   since that's (one or more bits set) && (not all bits set) */
                                Feedback = ((p->NoiseShiftRegister & p->WhiteNoiseFeedback) &&
                                           ((p->NoiseShiftRegister & p->WhiteNoiseFeedback) ^ p->WhiteNoiseFeedback));
                                break;
                            case 0x8005:    /* BBC Micro */
                                /* fall through :P can't be bothered to think too much */
                            default:        /* Default handler for all other feedback values */
                                Feedback = p->NoiseShiftRegister & p->WhiteNoiseFeedback;
                                Feedback ^= Feedback >> 8;
                                Feedback ^= Feedback >> 4;
                                Feedback ^= Feedback >> 2;
                                Feedback ^= Feedback >> 1;
                                Feedback &= 1;
                                break;
                        }
                    }
                    else        /* Periodic noise */
                    {
                        Feedback = p->NoiseShiftRegister & 1;
                    }
                    p->NoiseShiftRegister = (p->NoiseShiftRegister >> 1) | (Feedback << 15);
                }
            }
        }
    }
    else
    {
        for(j = 0; j < length; j++)
        {
            for(i = 0; i <= 2; ++i)
            {
                if(p->IntermediatePos[i] != LONG_MIN)
                {
                    p->Channels[i] = (int16) ((p->Mute >> i & 0x1) * PSGVolumeValues[p->VolumeArray][p->Registers[(i << 1) + 1]] * p->IntermediatePos[i] >> 16);
                }
                else
                {
                    p->Channels[i] = (int16) ((p->Mute >> i & 0x1) * PSGVolumeValues[p->VolumeArray][p->Registers[(i << 1) + 1]] * p->ToneFreqPos[i]);
                }
            }    
            p->Channels[3] = (int16) ((p->Mute >> 3 & 0x1) * PSGVolumeValues[p->VolumeArray][p->Registers[7]] * (p->NoiseShiftRegister & 0x1));
        
            if(p->BoostNoise) p->Channels[3] <<= 1;     /* Double noise volume to make some people happy */

            dat = p->Channels[0];
            dat += p->Channels[1];
            dat += p->Channels[2];
            dat += p->Channels[3];
            buffer[0][j] = dat;
            buffer[1][j] = dat;

            p->Clock += p->dClock;
            p->NumClocksForSample = (int) p->Clock;     /* truncates */
            p->Clock -= p->NumClocksForSample;          /* remove integer part */
    
            /* Decrement tone channel counters */
            p->ToneFreqVals[0] -= p->NumClocksForSample;
            p->ToneFreqVals[1] -= p->NumClocksForSample;
            p->ToneFreqVals[2] -= p->NumClocksForSample;
         
            /* Noise channel: match to tone2 or decrement its counter */
            if(p->NoiseFreq == 0x80)
            {
                p->ToneFreqVals[3] = p->ToneFreqVals[2];
            }
            else
            {
                p->ToneFreqVals[3] -= p->NumClocksForSample;
            }
    
            /* Tone channels: */
            for(i = 0; i <= 2; ++i)
            {
                if(p->ToneFreqVals[i] <= 0)
                {   /* If it gets below 0... */
                    if(p->Registers[i * 2] > PSG_CUTOFF)
                    {
                        /* Calculate how much of the sample is + and how much is -
                           Go to floating point and include the clock fraction for extreme accuracy :D
                           Store as long int, maybe it's faster? I'm not very good at this */
                        p->IntermediatePos[i] = (uint32) ((p->NumClocksForSample - p->Clock + (p->ToneFreqVals[i] << 1)) *
                                                          p->ToneFreqPos[i] / (p->NumClocksForSample + p->Clock) * 65536);
                        p->ToneFreqPos[i] = -p->ToneFreqPos[i]; /* Flip the flip-flop */
                    }
                    else
                    {
                        p->ToneFreqPos[i] = 1;          /* stuck value */
                        p->IntermediatePos[i] = LONG_MIN;
                    }
                    p->ToneFreqVals[i] += p->Registers[i << 1] * (p->NumClocksForSample / p->Registers[i << 1] + 1);
                }
                else
                {
                    p->IntermediatePos[i] = LONG_MIN;
                }
            }

            /* Noise channel */
            if(p->ToneFreqVals[3] <= 0)
            {
                /* If it gets below 0... */
                p->ToneFreqPos[3] = -p->ToneFreqPos[3]; /* Flip the flip-flop */
                if(p->NoiseFreq != 0x80)                /* If not matching tone2, decrement counter */
                {
                    p->ToneFreqVals[3] += p->NoiseFreq * (p->NumClocksForSample / p->NoiseFreq + 1);
                }
                if(p->ToneFreqPos[3] == 1)
                {   
                    /* Only once per cycle... */
                    int Feedback;

                    if(p->Registers[6] & 0x4)
                    {   /* White noise */
                        /* Calculate parity of fed-back bits for feedback */
                        switch(p->WhiteNoiseFeedback)
                        {
                            /* Do some optimised calculations for common (known) feedback values */
                            case 0x0006:    /* SC-3000      %00000110 */
                            case 0x0009:    /* SMS, GG, MD  %00001001 */
                                /* If two bits fed back, I can do Feedback=(nsr & fb) && (nsr & fb ^ fb)
                                   since that's (one or more bits set) && (not all bits set) */
                                Feedback = ((p->NoiseShiftRegister & p->WhiteNoiseFeedback) &&
                                           ((p->NoiseShiftRegister & p->WhiteNoiseFeedback) ^ p->WhiteNoiseFeedback));
                                break;
                            case 0x8005:    /* BBC Micro */
                                /* fall through :P can't be bothered to think too much */
                            default:        /* Default handler for all other feedback values */
                                Feedback = p->NoiseShiftRegister & p->WhiteNoiseFeedback;
                                Feedback ^= Feedback >> 8;
                                Feedback ^= Feedback >> 4;
                                Feedback ^= Feedback >> 2;
                                Feedback ^= Feedback >> 1;
                                Feedback &= 1;
                                break;
                        }
                    }
                    else
                    {
                        /* Periodic noise */
                        Feedback = p->NoiseShiftRegister & 1;
                    }
                    p->NoiseShiftRegister = (p->NoiseShiftRegister >> 1) | (Feedback << 15);
                }
            }
        }
    }
}
