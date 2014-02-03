#ifndef IRDA_H
#define IRDA_H

// Receivepin
#define GET_IRDA_SIGNAL  (!(GPIOC->IDR & 0x0002))	// invert data for TSOP31238 (low active)

// Timing of receive function
#define US_PER_TIC   50


typedef struct
{
	volatile uint8_t newDat;
	volatile uint8_t repeat;

	volatile uint8_t adr;
	volatile uint8_t dat;

} IRDA_RECEIVE_DATA;			// Received data for data exchange

extern IRDA_RECEIVE_DATA irdaData;

extern void IRDA_Receive(void);	// Receive function

#endif
