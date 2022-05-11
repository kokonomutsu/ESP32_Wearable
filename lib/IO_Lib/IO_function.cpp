#include <Arduino.h>
#include "IO_function.h"

#ifdef USE_LED
    #ifdef USE_LED_1
		IO_Struct pLED1;
		static enumbool bStateLED1 = eFALSE;
		/* Init function */
		void LED1_Init(IO_Struct *pIO)
		{
            LED1_GPIO_INIT;
			/* Init pointer function */
			pIO->write=&LED1_Write;
			pIO->writeSta=&LED1_WriteStatus;
		}
		 /* write function */
		void LED1_Write(char BitVal)
		{
			if(BitVal==eTRUE)
			{
				LED1_GPIO_HIGH;
				bStateLED1 = eTRUE;
			}
			else
			{
				LED1_GPIO_LOW;
				bStateLED1 = eFALSE;
			}
		}
		/* Read write status function */
		enumbool LED1_WriteStatus(void)
		{
		  return bStateLED1;
		}
	#endif /*USE_LED_1*/
	#ifdef USE_LED_2
		IO_Struct pLED2;
		static enumbool bStateLED2 = eFALSE;
		/* Init function */
		void LED2_Init(IO_Struct *pIO)
		{
            LED2_GPIO_INIT;
			/* Init pointer function */
			pIO->write=&LED2_Write;
			pIO->writeSta=&LED2_WriteStatus;
		}
		 /* write function */
		void LED2_Write(char BitVal)
		{
			if(BitVal==eTRUE)
			{
				LED2_GPIO_HIGH;
				bStateLED2 = eTRUE;
			}
			else
			{
				LED2_GPIO_LOW;
				bStateLED2 = eFALSE;
			}
		}
		/* Read write status function */
		enumbool LED2_WriteStatus(void)
		{
		  return bStateLED2;
		}
	#endif /*USE_LED_2*/
	#ifdef USE_LED_3
		IO_Struct pLED3;
		static enumbool bStateLED3 = eFALSE;
		/* Init function */
		void LED3_Init(IO_Struct *pIO)
		{
            LED3_GPIO_INIT;
			/* Init pointer function */
			pIO->write=&LED3_Write;
			pIO->writeSta=&LED3_WriteStatus;
		}
		 /* write function */
		void LED3_Write(char BitVal)
		{
			if(BitVal==eTRUE)
			{
				LED3_GPIO_HIGH;
				bStateLED3 = eTRUE;
			}
			else
			{
				LED3_GPIO_LOW;
				bStateLED3 = eFALSE;
			}
		}
		/* Read write status function */
		enumbool LED3_WriteStatus(void)
		{
		  return bStateLED3;
		}
	#endif /*USE_LED_3*/

/*-------------------------------Init INPUT_IO_-------------------------------*/
#ifdef USE_BUTTON_IO
	#ifdef USE_BUTTON_IO_1
		/* Manage Variable */
		IO_Struct pBUT_1;
		/* Init function */
		void BUTTON1_Init(IO_Struct *pIO)
		{
            BT1_GPIO_INIT;
			/* Init pointer function */
			pIO->read=&BUTTON1_Read;
		}
			/* write function */
		char BUTTON1_Read(void)
		{
			return BT1_GPIO_VAL;
		}
	#endif /*USE_BUTTON_IO_1*/

	#ifdef USE_BUTTON_IO_2
		IO_Struct pBUT_2;
		void BUTTON2_Init(IO_Struct *pIO)
		{
            BT2_GPIO_INIT;
			pIO->read=&BUTTON2_Read;
		}
		char BUTTON2_Read(void)
		{
			return BT2_GPIO_VAL;
		}
	#endif /*USE_BUTTON_IO_2*/

	#ifdef USE_BUTTON_IO_3
        IO_Struct	pBUT_3;
        void BUTTON3_Init(IO_Struct *pIO)
        {
            BT3_GPIO_INIT;
            pIO->read=&BUTTON3_Read;
        }
        enumbool BUTTON3_Read(void)
        {
            return BT3_GPIO_VAL;
        }
	#endif /*USE_BUTTON_IO_3*/
#endif /*USE_BUTTON_IO*/

#endif