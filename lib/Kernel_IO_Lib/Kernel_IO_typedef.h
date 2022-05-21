#ifndef _KERNEL_IO_TYPEDEF_
#define _KERNEL_IO_TYPEDEF_

#include "IO_config.h"
#include "Data_Typedef.h"

typedef struct{
	unsigned long	uFrequency;
	unsigned long	uCountToggle;
	unsigned long	uCycleCounter;
	enumbool	    bStartState;
	//enumbool	    bEndState;
	enumbool	    bCurrentProcess;
	enumbool	    bFlagStart;
}structIO_Manage_Output;

#ifdef USE_BUTTON_IO
/* Enum button state */
typedef enum
{
    eButtonPress			= 1,
    eButtonRelease 			= 2,
	eButtonLongPressT1		= 3,
	eButtonLongPressT2		= 4,
	eButtonHoldOn			= 5,
	eButtonDoubleClick		= 6,
	eButtonSingleClick		= 7,
	eButtonTripleClick		= 8,
	eButtonHoldOff			= 9,
    eButtonHoldOffLongT1	= 10,
    eButtonHoldOffLongT2	= 11,
    eButtonHoldOffLong  	= 12,
	eButtonStateUN			= 0xff,
}eButtonState;
/* Enum button index */
typedef enum
{
    eButton1			= 0,
    eButton2 			= 1,
    eButton3			= 2,
	eButton4			= 3,
	eButton5			= 4,
	eButtonUN			= 0xff,
}eIndexButton;
/* Struct manage button */
typedef struct{
    eButtonState	bButtonState[(int)NUMBER_IO_BUTTON_USE];/* Button state */
	unsigned long	bButtonTime[(int)NUMBER_IO_BUTTON_USE];/* Time to hole */
	enumbool 		bFlagNewButton;
}structIO_Button;
#endif

#endif