#ifndef _IO_FUNCTION_H_
#define _IO_FUNCTION_H_

#include "IO_config.h"
#include "Data_Typedef.h"

/**************OUTPUT************************/
#ifdef USE_LED
    #ifdef USE_LED_1
		void LED1_Init(IO_Struct *pIO);
		void LED1_Write(char BitVal);
		enumbool LED1_WriteStatus(void);
		extern IO_Struct pLED1;
	#endif /*USE_LED_1*/
	#ifdef USE_LED_2
		void LED2_Init(IO_Struct *pIO);
		void LED2_Write(char BitVal);
		enumbool LED2_WriteStatus(void);
		extern IO_Struct pLED2;
	#endif /*USE_LED_2*/
	#ifdef USE_LED_3
		void LED3_Init(IO_Struct *pIO);
		void LED3_Write(char BitVal);
		enumbool LED3_WriteStatus(void);
		extern IO_Struct pLED3;
	#endif /*USE_LED_3*/
#endif

/*----------------------------------------------------------------------------*/
#ifdef USE_BUTTON_IO
	#ifdef USE_BUTTON_IO_1
		void BUTTON1_Init(IO_Struct *pIO);
		char BUTTON1_Read(void);
		extern IO_Struct pBUT_1;
	#endif /*USE_BUTTON_IO_1*/

	#ifdef USE_BUTTON_IO_2
		void BUTTON2_Init(IO_Struct *pIO);
		char BUTTON2_Read(void);
		extern IO_Struct pBUT_2;
	#endif /*USE_BUTTON_IO_2*/

	#ifdef USE_BUTTON_IO_3
		void BUTTON3_Init(IO_Struct *pIO);
		enumbool BUTTON3_Read(void);
		extern IO_Struct pBUT_3;
	#endif /*USE_BUTTON_IO_3*/

#endif /*USE_BUTTON_IO*/

#endif