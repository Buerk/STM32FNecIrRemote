#include <stdio.h>
#include "STM32F4xx.h"
#include "led.h"
#include "irda.h"

volatile uint32_t usTicks;                      /* counts 1ms timeTicks       */
/*----------------------------------------------------------------------------
  SysTick_Handler
 *----------------------------------------------------------------------------*/
void SysTick_Handler(void)
{
	if (usTicks < 99)
		LED_On(1);
	else if (usTicks == 500000)
		LED_Off(1);

	if (usTicks > 1000000)
		usTicks = 0;

	usTicks += 50;
	IRDA_Receive();
}


void IRDA_Init(void)
{
	RCC->AHB1ENR  |= ((1UL <<  2) );              /* Enable GPIOC clock         */

	GPIOC->MODER    &= ~((3UL << 2*1)  );         /* PC.1 is input              */
	GPIOC->OSPEEDR  &= ~((3UL << 2*1)  );         /* PC.1 is 50MHz Fast Speed   */
	GPIOC->OSPEEDR  |=  ((2UL << 2*1)  );
	GPIOC->PUPDR    &= ~((3UL << 2*1)  );         /* PC.1 is no Pull up         */
}

int main(void)
{
	SystemInit();
	SystemCoreClockUpdate();                      /* Get Core Clock Frequency   */
	IRDA_Init();

	LED_Init();
	LED_On(0);

	if (SysTick_Config(SystemCoreClock / 20000))	// must match US_PER_TIC
	{ /* SysTick 100 usec interrupts  */
		while (1)
			;                                  /* Capture error              */
	}

	while(1)
    {
    	if (irdaData.newDat)
    	{
    		irdaData.newDat = 0;  // Set Breakpoint and examine...
    		// ...Data: irdaData.adr, irdaData.dat
    	}
    }
}
