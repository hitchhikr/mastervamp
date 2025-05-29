#ifndef _SHARED_H_
#define _SHARED_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>
#include <math.h>
#include <limits.h>

#define ALIGN
typedef unsigned char uint8;
typedef unsigned short uint16;

#include "types.h"
#include "cpu/z80.h"
#include "sms.h"
#include "pio.h"
#include "cpu/memz80.h"
#include "vdp.h"
#include "render.h"
#include "tms.h"
#include "sn76489.h"
#include "sound.h"
#include "system.h"

#include "state.h"
#include "loadrom.h"

#endif /* _SHARED_H_ */
