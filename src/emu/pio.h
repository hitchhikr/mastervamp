#ifndef _PIO_H_
#define _PIO_H_

#define SIO_TXFL (1 << 0)   /* 1 = Transmit buffer full */
#define SIO_RXRD (1 << 1)   /* 1 = Receive buffer full */
#define SIO_FRER (1 << 2)   /* 1 = Framing error occured */

#define MAX_DEVICE 2
#define DEVICE_D0 (1 << 0)
#define DEVICE_D1 (1 << 1)
#define DEVICE_D2 (1 << 2)
#define DEVICE_D3 (1 << 3)
#define DEVICE_TL (1 << 4)
#define DEVICE_TR (1 << 5)
#define DEVICE_TH (1 << 6)
#define DEVICE_ALL (DEVICE_D0 | DEVICE_D1 | DEVICE_D2 | DEVICE_D3 | DEVICE_TL | DEVICE_TR | DEVICE_TH)

#define PIN_LVL_LO 0        /* Pin outputs 0V */
#define PIN_LVL_HI 1        /* Pin outputs +5V */
#define PIN_DIR_OUT 0       /* Pin is is an active driving output */
#define PIN_DIR_IN 1        /* Pin is in active-low input mode */

enum
{
    PORT_A = 0,             /* I/O port A */
    PORT_B = 1              /* I/O port B */
};

enum
{
    DEVICE_NONE = 0,        /* No peripheral */
    DEVICE_PAD2B = 1,       /* Standard 2-button digital joystick/gamepad */
    DEVICE_PADDLE = 2,      /* Paddle controller; rotary dial with fire button */
    DEVICE_LIGHTGUN = 3,    /* Sega Light Phaser; Light Gun with trigger button */
    DEVICE_SPORTSPAD = 4    /* Sports Pad controller; analog stick with 2 buttons */
};

typedef struct
{
    uint8 tr_level[2];      /* TR pin output level */
    uint8 th_level[2];      /* TH pin output level */
    uint8 tr_dir[2];        /* TR pin direction */
    uint8 th_dir[2];        /* TH pin direction */
} io_state;

/* Global variables */
extern io_state io_lut[2][256];
extern io_state *io_current;

/* Function prototypes */
void pio_init(void);
void pio_reset(void);
void pio_shutdown(void);

void pio_ctrl_w(uint8 data);
uint8 pio_port_r(uint8 offset);
void sio_w(uint8 offset, int data);
uint8 sio_r(uint8 offset);
extern uint8 coleco_pio_r(uint8 port);

#endif /* _PIO_H_ */
