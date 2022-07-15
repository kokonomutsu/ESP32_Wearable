#include "Kernel_IO_function.h"

void vIO_Output(structIO_Manage_Output *pOutput, IO_Struct *pControl)
{
	if(pOutput->bCurrentProcess == eTRUE)
	{
		// Increase counter
		pOutput->uCycleCounter++;
		if(((pOutput->uCycleCounter)%(pOutput->uFrequency))==0)
		{

			if(pOutput->bFlagStart==eTRUE)
			{
				pOutput->bFlagStart = eFALSE;
				pControl->write(pOutput->bStartState);
				if(pOutput->uCountToggle != 0)
				{
					pOutput->uCountToggle--;
				}
			}
			else
			{
				if(pOutput->uCountToggle!=0)
				{
					pOutput->uCountToggle--;
					pControl->write((enumbool)(1 - (int)pControl->writeSta()));
				}
				else
				{
					//pControl->write(pOutput->bEndState);
					pOutput->bCurrentProcess = eFALSE;
				}
			}
		}
	}
}
//vIO_Output in 10ms task
//Change state: vIO_ConfigOutput(&pOPx, 1, 1, 1, 0);
//Toggle 500ms: vIO_ConfigOutput(&pOPx, 1, 2, 500/10, 0);

enumbool vIO_ConfigOutput(structIO_Manage_Output *pOutput, enumbool bStartState, uint32_t uCountToggle, uint32_t uFrequency, enumbool bFlagInterrupt)
{
	enumbool bFlagSetParaSucess = eFALSE;
	if(bFlagInterrupt==eTRUE)
	{
		pOutput->bCurrentProcess = eTRUE;

		pOutput->uFrequency 	= uFrequency;
		pOutput->uCountToggle 	= uCountToggle;
		pOutput->bStartState 	= bStartState;
		pOutput->bFlagStart		= eTRUE;

		pOutput->uCycleCounter	= uFrequency - 1;

		bFlagSetParaSucess		= eTRUE;
	}
	else
	{
		if(pOutput->bCurrentProcess == false)
		{
			pOutput->bCurrentProcess = eTRUE;

			pOutput->uFrequency 	= uFrequency;
			pOutput->uCountToggle 	= uCountToggle;
			pOutput->bStartState 	= bStartState;
			pOutput->bFlagStart		= eTRUE;

			pOutput->uCycleCounter	= uFrequency - 1;

			bFlagSetParaSucess		= eTRUE;
		}
	}
	return bFlagSetParaSucess;
}

#ifdef USE_BUTTON_IO
/* CYCLE 10 ms */
#define ANTI_NOISE_PRESS		2	/* 2 cycle of Button Process 20ms */
#define ANTI_NOISE_RELEASE		2	/* 2 cycle of Button Process 20ms */
#define TIME_SINGLE_PRESS		10	/* 10 cycle of Button Process 100ms */
#define TIME_LONG_PRESS_T1		200	/* 100 cycle of Button Process 1s */
#define TIME_LONG_PRESS_T2 		300	/* 200 cycle of Button Process ~ 2000ms = 2s */
#define TIME_HOLD_ON 			1000/* 1000 cycle of Button Process ~ 10000ms = 10s */
#define TIME_HOLD_OFF 			10	/* 10 cycle of Button Process ~ 100ms */
#define TIME_HOLD_OFF_LONG_T1	100	/* 100 cycle of Button Process ~ 1s */
#define TIME_HOLD_OFF_LONG_T2	200	/* 200 cycle of Button Process ~ 2s */
#define TIME_HOLD_OFF_LONG		400	/* 400 cycle of Button Process ~ 4s */

#define COUNT_TIME_SAMPLE		40

/* Static variable about time Press & Release */
static unsigned long 	uTimeCheck[NUMBER_IO_BUTTON_USE];
static unsigned long	uTimePress[NUMBER_IO_BUTTON_USE];
static unsigned char	bClickCount[NUMBER_IO_BUTTON_USE];
static unsigned long	bHoldOff[NUMBER_IO_BUTTON_USE];
static enumbool flag_start_sample[NUMBER_IO_BUTTON_USE];

/**		Reset Button Value		**
void vResetButonValue(eIndexButton bIndex)
{
	uTimeCheck[bIndex]=0;
	bClickCount[bIndex]=0;
	uTimePress[bIndex]=0;
	bHoldOff[bIndex]=0;
}
*/
void vGetIOButtonValue(eIndexButton bIndex, enumbool InputState, structIO_Button *OldValue, structIO_Button *NewValue)
{
/* Start get new state */
	/* eFALSE button press, eTRUE button release*/
	if(InputState == eTRUE)
	{
		/* Base on the last state, caculate to change state */
		switch(OldValue->bButtonState[bIndex])
		{
			case eButtonPress:
				if(bHoldOff[bIndex]>=(int)ANTI_NOISE_RELEASE)
					NewValue->bButtonState[bIndex] = eButtonRelease;
				break;
			case eButtonSingleClick:
				if(bHoldOff[bIndex]>=(int)ANTI_NOISE_RELEASE)
					NewValue->bButtonState[bIndex] = eButtonRelease;
				break;
			case eButtonDoubleClick:
				if(bHoldOff[bIndex]>=(int)ANTI_NOISE_RELEASE)
					NewValue->bButtonState[bIndex] = eButtonRelease;
				break;
			case eButtonTripleClick:
				if(bHoldOff[bIndex]>=(int)ANTI_NOISE_RELEASE)
					NewValue->bButtonState[bIndex] = eButtonRelease;
				break;
			case eButtonLongPressT1:
				if(bHoldOff[bIndex]>=(int)ANTI_NOISE_RELEASE)
					NewValue->bButtonState[bIndex] = eButtonRelease;
				break;
			case eButtonLongPressT2:
				if(bHoldOff[bIndex]>=(int)ANTI_NOISE_RELEASE)
					NewValue->bButtonState[bIndex] = eButtonRelease;
				break;
			case eButtonHoldOn:
				if(bHoldOff[bIndex]>=(int)ANTI_NOISE_RELEASE)
					NewValue->bButtonState[bIndex] = eButtonRelease;
				break;
			case eButtonRelease:
				if(bHoldOff[bIndex]>=(int)TIME_HOLD_OFF)
					NewValue->bButtonState[bIndex] = eButtonHoldOff;
				break;
			case eButtonHoldOff:
				if(bHoldOff[bIndex]>=(int)TIME_HOLD_OFF_LONG_T1)
					NewValue->bButtonState[bIndex] = eButtonHoldOffLongT1;
				break;
			case eButtonHoldOffLongT1:
				if(bHoldOff[bIndex]>=(int)TIME_HOLD_OFF_LONG_T2)
					NewValue->bButtonState[bIndex] = eButtonHoldOffLongT2;
				break;
			case eButtonHoldOffLongT2:
				if(bHoldOff[bIndex]>=(int)TIME_HOLD_OFF_LONG)
					NewValue->bButtonState[bIndex] = eButtonHoldOffLong;
				break;
			case eButtonHoldOffLong:
				break;
		}
		/* Increase time Hold Off */
		if(bHoldOff[bIndex]<(int)TIME_HOLD_OFF_LONG){bHoldOff[bIndex]++;}
		/* Reset Time counter */
		uTimePress[bIndex] = 0;
		NewValue->bButtonTime[bIndex] = 0;
	}
	/* Button press */
	else if(InputState == eFALSE)
	{
		/* Base on the last state, caculate to change state */
		switch(OldValue->bButtonState[bIndex])
		{
			case eButtonRelease:
				bHoldOff[bIndex]=0;
				if(uTimeCheck[bIndex]>=(int)ANTI_NOISE_PRESS)
				{
					NewValue->bButtonState[bIndex] = eButtonPress;
					bClickCount[bIndex]++;
				}
				flag_start_sample[bIndex] = eTRUE;
				break;
			case eButtonHoldOff:
				NewValue->bButtonState[bIndex] = eButtonRelease;
				break;
			case eButtonHoldOffLongT1:
				NewValue->bButtonState[bIndex] = eButtonRelease;
				break;
			case eButtonHoldOffLongT2:
				NewValue->bButtonState[bIndex] = eButtonRelease;
				break;
			case eButtonHoldOffLong:
				NewValue->bButtonState[bIndex] = eButtonRelease;
				break;
			}
		/* Increase counter */
		++uTimePress[bIndex];
	}

	/* Time End period */
	if(flag_start_sample[bIndex]==eTRUE)
		uTimeCheck[bIndex]++;
	if(uTimeCheck[bIndex]>=(int)COUNT_TIME_SAMPLE)/* 10*20=200ms */
	{
		if((bClickCount[bIndex]==1)&&(uTimePress[bIndex]<=(int)TIME_SINGLE_PRESS))
			NewValue->bButtonState[bIndex] = eButtonSingleClick;
		if(bClickCount[bIndex]==2)
			NewValue->bButtonState[bIndex] = eButtonDoubleClick;
		if(bClickCount[bIndex]==3)
			NewValue->bButtonState[bIndex] = eButtonTripleClick;
		/* Reset counter */
		bClickCount[bIndex] = 0;
		/* Reset */
		uTimeCheck[bIndex] = 0;
		/* Set flag sample */
		flag_start_sample[bIndex] = eFALSE;
	}
	/* Change mode */
	if(uTimePress[bIndex]>=(int)TIME_HOLD_ON)
		NewValue->bButtonState[bIndex] = eButtonHoldOn;
	else if(uTimePress[bIndex]>=TIME_LONG_PRESS_T2)
		NewValue->bButtonState[bIndex] = eButtonLongPressT2;
	else if(uTimePress[bIndex]>=TIME_LONG_PRESS_T1)
		NewValue->bButtonState[bIndex] = eButtonLongPressT1;
	/* Update button press time */
	NewValue->bButtonTime[bIndex] = uTimePress[bIndex];
}
#endif