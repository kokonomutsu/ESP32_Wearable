#ifndef	_IO_Kernel_H_
#define _IO_Kernel_H_

#include "Kernel_IO_typedef.h"
#include <stdint.h>

void vIO_Output(structIO_Manage_Output *pOutput, IO_Struct *pControl);
enumbool vIO_ConfigOutput(structIO_Manage_Output *pOutput, enumbool bStartState, uint32_t uCountToggle, uint32_t uFrequency, enumbool bFlagInterrupt);

#ifdef USE_BUTTON_IO
	//void vResetButonValue(eIndexButton bIndex);
	void vGetIOButtonValue(eIndexButton bIndex, enumbool InputState, structIO_Button *OldValue, structIO_Button *NewValue);
#endif	

#endif
