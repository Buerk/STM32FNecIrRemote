#include <stdlib.h>
#include <stdint.h>
#include "STM32F4xx.h"
#include "irda.h"



/*
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 *
 *   NEC + extended NEC
 *   -------------------------
 *
 *   frame: 1 start bit + 32 data bits + 1 stop bit
 *   data NEC:          8 address bits + 8 inverted address bits + 8 command bits + 8 inverted command bits
 *   data extended NEC: 16 address bits + 8 command bits + 8 inverted command bits
 *
 *   start bit:                           data "0":                 data "1":                 stop bit:
 *   -----------------_________           ------______              ------________________    ------______....
 *       9000us        4500us             560us  560us              560us    1690 us          560us
 *
 *
 *   Repetition frame:
 *
 *   -----------------_________------______  .... ~100ms Pause, then repeat
 *       9000us        2250us   560us
 *
 *---------------------------------------------------------------------------------------------------------------------------------------------------
*/

#ifdef SHORT_IRDA_TIMING_TYPE
	#define IRDA_TIMING_TYPE		uint8_t
	#define MAX_IRDA_TIMER_VALUE	0xFF
#else
	#define IRDA_TIMING_TYPE		uint16_t
	#define MAX_IRDA_TIMER_VALUE	0xFFFF
#endif

#define TIMEOUT_TIME_us			12000UL

#define NEC_HEADER_PULSE_us		9000UL
#define NEC_HEADER_PAUSE_us		4500UL
#define NEC_PULSE_LEN_us  		560UL
#define NEC_BIT_ZERO_PAUSE_us	560UL
#define NEC_BIT_ONE_PAUSE_us	1700UL
#define NEC_REPEAT_PAUSE_us		2250UL
#define NEC_REPEAT_TIMEOUT_us	300000UL

#define TOLERANCE 25

#define NEC_HEADER_PULSE_MIN_TICS  	((IRDA_TIMING_TYPE)((NEC_HEADER_PULSE_us * (100 - TOLERANCE)) / (100 * US_PER_TIC)))
#define NEC_HEADER_PULSE_MAX_TICS	((IRDA_TIMING_TYPE)((NEC_HEADER_PULSE_us * (100 + TOLERANCE)) / (100 * US_PER_TIC)))
#define NEC_HEADER_PAUSE_MIN_TICS	((IRDA_TIMING_TYPE)((NEC_HEADER_PAUSE_us * (100 - TOLERANCE)) / (100 * US_PER_TIC)))
#define NEC_HEADER_PAUSE_MAX_TICS	((IRDA_TIMING_TYPE)((NEC_HEADER_PAUSE_us * (100 + TOLERANCE)) / (100 * US_PER_TIC)))
#define NEC_PULSE_LEN_MIN_TICS		((IRDA_TIMING_TYPE)((NEC_PULSE_LEN_us * (100 - TOLERANCE)) / (100 * US_PER_TIC)))
#define NEC_PULSE_LEN_MAX_TICS		((IRDA_TIMING_TYPE)((NEC_PULSE_LEN_us * (100 + TOLERANCE)) / (100 * US_PER_TIC)))
#define NEC_BIT_ZERO_PAUSE_MIN_TICS	((IRDA_TIMING_TYPE)((NEC_BIT_ZERO_PAUSE_us * (100 - TOLERANCE)) / (100 * US_PER_TIC)))
#define NEC_BIT_ZERO_PAUSE_MAX_TICS	((IRDA_TIMING_TYPE)((NEC_BIT_ZERO_PAUSE_us * (100 + TOLERANCE)) / (100 * US_PER_TIC)))
#define NEC_BIT_ONE_PAUSE_MIN_TICS	((IRDA_TIMING_TYPE)((NEC_BIT_ONE_PAUSE_us * (100 - TOLERANCE)) / (100 * US_PER_TIC)))
#define NEC_BIT_ONE_PAUSE_MAX_TICS	((IRDA_TIMING_TYPE)((NEC_BIT_ONE_PAUSE_us * (100 + TOLERANCE)) / (100 * US_PER_TIC)))
#define NEC_REPEAT_PAUSE_MIN_TICS	((IRDA_TIMING_TYPE)((NEC_REPEAT_PAUSE_us * (100 - TOLERANCE)) / (100 * US_PER_TIC)))
#define NEC_REPEAT_PAUSE_MAX_TICS	((IRDA_TIMING_TYPE)((NEC_REPEAT_PAUSE_us * (100 + TOLERANCE)) / (100 * US_PER_TIC)))
#define NEC_REPEAT_TIMEOUT_TICS     ((uint16_t)(NEC_REPEAT_TIMEOUT_us/ US_PER_TIC))

#define TIMEOUT_TIME_TICS			((IRDA_TIMING_TYPE)(TIMEOUT_TIME_us/ US_PER_TIC))

#define IRPULSE 1
#define IRPAUSE 0

#define EVENT_NO_EVENT 			0
#define EVENT_MEASURED_PAUSE	1
#define EVENT_MEASURED_PULSE	2

#define FALSE 0
#define TRUE (!FALSE)

typedef enum
{
	IRDA_IDLE,
	IRDA_PULSE
} IRDA_STATE;

typedef enum
{
	IRDA_RECEIVE_FROM_IDLE,
	IRDA_RECEIVE_HEADER_PULSE,
	IRDA_RECEIVE_HEADER_PAUSE,
	IRDA_RECEIVE_REPETITION,
	IRDA_RECEIVE_DATA_PULSE,
	IRDA_RECEIVE_DATA_PAUSE,
	IRDA_RECEIVE_FINISHED
} IRDA_DECODE_STATE;

typedef struct
{
	// Data for receive function:
	IRDA_TIMING_TYPE timer;
	IRDA_STATE state;

	// Data for decode function:
	IRDA_DECODE_STATE decodestate;
	uint16_t frameTimer;

	uint8_t dat_cnt;
	// receive operating buffers:
	uint32_t data;
	uint8_t adr;
	uint8_t nadr;
	uint8_t dat;
	uint8_t ndat;
} IRDA;


IRDA_RECEIVE_DATA irdaData;
static IRDA irda;


// ------------------------------------------------------------------------------------------------
#ifdef IRDA_LOG

#define RAWLOG_LEN 70
IRDA_TIMING_TYPE rawlog[RAWLOG_LEN];
uint8_t rawidx = 0;

static void IRDA_logger(IRDA_TIMING_TYPE time, uint8_t event)
{
	if (time >= TIMEOUT_TIME_TICS)
	{
		rawidx = 0;
	}
	if (rawidx < RAWLOG_LEN)
	{
		rawlog[rawidx] = time;
		rawidx++;
	}
}
#endif

// ------------------------------------------------------------------------------------------------

static void IRDA_Decode(IRDA_TIMING_TYPE time, uint8_t event)
{
	uint8_t dat, ndat, adr, nadr;

	switch (irda.decodestate)
	{
		case IRDA_RECEIVE_FROM_IDLE:
		default:
			if (event == EVENT_MEASURED_PAUSE)
			{	// only one time synchronization to correct event (rising edge) necessary. Then the other events are automatically correct due to this state machine
				irda.decodestate = IRDA_RECEIVE_HEADER_PULSE;
			}
			break;

		case IRDA_RECEIVE_HEADER_PULSE:
			if (time >= NEC_HEADER_PULSE_MIN_TICS && time <= NEC_HEADER_PULSE_MAX_TICS)
				irda.decodestate = IRDA_RECEIVE_HEADER_PAUSE;
			else
				irda.decodestate = IRDA_RECEIVE_FROM_IDLE;
			break;

		case IRDA_RECEIVE_HEADER_PAUSE:
			if (time >= NEC_HEADER_PAUSE_MIN_TICS && time <= NEC_HEADER_PAUSE_MAX_TICS)
			{
				irda.dat_cnt = 0;
				irda.decodestate = IRDA_RECEIVE_DATA_PULSE;
			}
			else if (time >= NEC_REPEAT_PAUSE_MIN_TICS && time <= NEC_REPEAT_PAUSE_MAX_TICS && irda.frameTimer < NEC_REPEAT_TIMEOUT_TICS)
			{	// repetition only allowed if not too long since last reception
				irda.decodestate = IRDA_RECEIVE_REPETITION;
			}
			else
			{
				irda.decodestate = IRDA_RECEIVE_FROM_IDLE;	// wrong time: wait for starting pulse
			}
			break;

		case IRDA_RECEIVE_DATA_PULSE:
			if (time >= NEC_PULSE_LEN_MIN_TICS && time <= NEC_PULSE_LEN_MAX_TICS)
				irda.decodestate = IRDA_RECEIVE_DATA_PAUSE;
			else
				irda.decodestate = IRDA_RECEIVE_FROM_IDLE;
			break;

		case IRDA_RECEIVE_DATA_PAUSE:
			irda.data <<= 1;
			if (time >= NEC_BIT_ZERO_PAUSE_MIN_TICS && time <= NEC_BIT_ZERO_PAUSE_MAX_TICS)
			{
				irda.decodestate = IRDA_RECEIVE_DATA_PULSE;
			}
			else if (time >= NEC_BIT_ONE_PAUSE_MIN_TICS && time <= NEC_BIT_ONE_PAUSE_MAX_TICS)
			{
				irda.decodestate = IRDA_RECEIVE_DATA_PULSE;
				irda.data |= 1;
			}
			else
				irda.decodestate = IRDA_RECEIVE_FROM_IDLE;

			irda.dat_cnt++;

			if (irda.dat_cnt >= 32)
				irda.decodestate = IRDA_RECEIVE_FINISHED;
			break;

		case IRDA_RECEIVE_FINISHED:
			if (time >= NEC_PULSE_LEN_MIN_TICS && time <= NEC_PULSE_LEN_MAX_TICS)
			{
				adr = irda.data >> 24;
				nadr = irda.data >> 16;
				dat = irda.data >> 8;
				ndat = irda.data;
				if ((adr ^ nadr) == 0xFF && (dat ^ ndat) == 0xFF)
				{
					irdaData.adr = adr;
					irdaData.dat = dat;
					irdaData.repeat = 0;
					irdaData.newDat = 1;
					irda.frameTimer = 0;
				}
			}
			irda.decodestate = IRDA_RECEIVE_FROM_IDLE;
			break;

		case IRDA_RECEIVE_REPETITION:
			if (time >= NEC_PULSE_LEN_MIN_TICS && time <= NEC_PULSE_LEN_MAX_TICS)
			{
				irdaData.repeat = 1;
				irdaData.newDat = 1;
				irda.frameTimer = 0;
			}
			irda.decodestate = IRDA_RECEIVE_FROM_IDLE;
			break;
	}
}

void IRDA_Receive(void)
{
	uint8_t irdat;
	IRDA_TIMING_TYPE time;			// measured time
	uint8_t event = EVENT_NO_EVENT;

	if (GET_IRDA_SIGNAL)
		irdat = IRPULSE;
	else
		irdat = IRPAUSE;

	if (irda.timer < MAX_IRDA_TIMER_VALUE)
		irda.timer++;

	if (irda.frameTimer < 0xFFFF)
		irda.frameTimer++;

	switch(irda.state)
	{
		case IRDA_IDLE:
		default:
			if (irdat == IRPULSE)
			{	// IR modulation activity detected
				event = EVENT_MEASURED_PAUSE;
				time = irda.timer;
				irda.timer = 0;
				irda.state = IRDA_PULSE;
			}
			break;

		case IRDA_PULSE:
			if (irdat == IRPAUSE)
			{	// IR modulation no longer active
				event = EVENT_MEASURED_PULSE;
				time = irda.timer;
				irda.timer = 0;
				irda.state = IRDA_IDLE;
			}
			break;
	}

	//-------------------------------------
	if (event != EVENT_NO_EVENT)
	{
		IRDA_Decode(time, event);
#ifdef IRDA_LOG
		IRDA_logger(time, event);
#endif
	}
}

