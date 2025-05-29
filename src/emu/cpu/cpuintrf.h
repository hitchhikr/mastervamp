#ifndef _CPUINTRF_H_
#define _CPUINTRF_H_

#include "osd_cpu.h"

/* Interrupt line constants */
enum
{
    /* Line states */
    CLEAR_LINE = 0,     /* Clear (a fired, held or pulsed) line */
    ASSERT_LINE,        /* Assert an interrupt immediately */
    HOLD_LINE,          /* Hold interrupt line until acknowledged */
    PULSE_LINE,         /* Pulse interrupt line for one instruction */

    /* Internal flags (not for use by drivers!) */
    INTERNAL_CLEAR_LINE = 100 + CLEAR_LINE,
    INTERNAL_ASSERT_LINE = 100 + ASSERT_LINE,

    /* Input lines */
    MAX_INPUT_LINES = 32+3,
    INPUT_LINE_IRQ0 = 0,
    INPUT_LINE_IRQ1 = 1,
    INPUT_LINE_IRQ2 = 2,
    INPUT_LINE_IRQ3 = 3,
    INPUT_LINE_IRQ4 = 4,
    INPUT_LINE_IRQ5 = 5,
    INPUT_LINE_IRQ6 = 6,
    INPUT_LINE_IRQ7 = 7,
    INPUT_LINE_IRQ8 = 8,
    INPUT_LINE_IRQ9 = 9,
    INPUT_LINE_NMI = MAX_INPUT_LINES - 3,

    /* Special input lines that are implemented in the core */
    INPUT_LINE_RESET = MAX_INPUT_LINES - 2,
    INPUT_LINE_HALT = MAX_INPUT_LINES - 1,

    /* Output lines */
    MAX_OUTPUT_LINES = 32
};

/* Daisy-chain link */
typedef struct
{
    void (*reset)(int);             /* Reset callback     */
    int  (*interrupt_entry)(int);   /* Entry callback     */
    void (*interrupt_reti)(int);    /* Reti callback      */
    int irq_param;                  /* Callback paramater */
}  Z80_DaisyChain;

#define Z80_MAXDAISY 4              /* Maximum of daisy chan device */

#define Z80_INT_REQ 0x01            /* Interrupt request mask       */
#define Z80_INT_IEO 0x02            /* Interrupt disable mask(IEO)  */

#define Z80_VECTOR(device,state) (((device) << 8) | (state))

#endif  /* _CPUINTRF_H_ */
