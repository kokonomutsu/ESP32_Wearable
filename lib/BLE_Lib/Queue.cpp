/*
 * Queue.c
 *
 *  Created on: Oct 29, 2021
 *  Author: DThree
 */
/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/


#include "Queue.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
/****************************************************************************
 *
 * NAME:       vQueue_Init
 *
 * DESCRIPTION:
 *
 * PARAMETERS: Name     RW  Usage
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
void vQueue_Init(tsQueue *psQueue, unsigned char *pau8Buffer, unsigned int u32Length)
{

	/* Initialise the event queue */
	psQueue->u32ReadPtr = 0;
	psQueue->u32WritePtr = 0;
	psQueue->u32Length = u32Length;
	psQueue->pau8Buffer = pau8Buffer;

}

/****************************************************************************
 *
 * NAME:       vQueue_Flush
 *
 * DESCRIPTION:
 *
 * PARAMETERS: Name     RW  Usage
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
void vQueue_Flush(tsQueue *psQueue)
{

	/* Initialise the event queue */
	psQueue->u32ReadPtr = 0;
	psQueue->u32WritePtr = 0;

}

/****************************************************************************
 *
 * NAME:       bPPP_QueueWrite
 *
 * DESCRIPTION:
 *
 * PARAMETERS: Name     RW  Usage
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
bool bQueue_Write(tsQueue *psQueue, unsigned char u8Item)
{
	/* Make a copy of the write pointer */
	unsigned long u32NewWritePtr = psQueue->u32WritePtr;

	u32NewWritePtr++;
	if(u32NewWritePtr == psQueue->u32Length)
	{
		u32NewWritePtr = 0;
	}

	/* If new incremented pointer is same as read pointer, queue is full */
	if(u32NewWritePtr == psQueue->u32ReadPtr)
	{
		return(true);
	}

	psQueue->pau8Buffer[psQueue->u32WritePtr] = u8Item;	/* Add item to queue */
	psQueue->u32WritePtr = u32NewWritePtr;				/* Write new pointer */

	return(false);
}

/****************************************************************************
 *
 * NAME:       bPPP_QueueRead
 *
 * DESCRIPTION:
 *
 * PARAMETERS: Name     RW  Usage
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
bool bQueue_Read(tsQueue *psQueue, unsigned char *pu8Item)
{

	/* If pointers are same, nothing in the queue */
	if(psQueue->u32ReadPtr == psQueue->u32WritePtr)
	{
		return(false);
	}

	/* Read an event from the queue */
	*pu8Item = psQueue->pau8Buffer[psQueue->u32ReadPtr++];

	if(psQueue->u32ReadPtr == psQueue->u32Length)
	{
		psQueue->u32ReadPtr = 0;
	}

	return(true);

}

/****************************************************************************
 *
 * NAME:       bQueue_IsEmpty
 *
 * DESCRIPTION:
 *
 * PARAMETERS: Name     RW  Usage
 *
 * RETURNS:
 * bool:	TRUE if the queue is empty
 *			FALSE if it contains data
 *
 ****************************************************************************/
bool bQueue_IsEmpty(tsQueue *psQueue)
{

	/* If pointers are same, nothing in the queue */
	if(psQueue->u32ReadPtr == psQueue->u32WritePtr)
	{
		return(true);
	}
	else
	{
		return(false);
	}

}


/****************************************************************************
 *
 * NAME:       bQueue_IsFull
 *
 * DESCRIPTION:
 *	Checks if the queue is full or not
 *
 * PARAMETERS: 	Name     		RW  Usage
 *
 * RETURNS:
 * bool:	TRUE if the queue is full
 *			FALSE if it contains data
 *
 ****************************************************************************/
bool bQueue_IsFull(tsQueue *psQueue)
{

	/* Queue can only ever hold u32Length -1 bytes max */
	if(u32Queue_GetDepth(psQueue) == psQueue->u32Length - 1)
	{
		return(true);
	}
	else
	{
		return(false);
	}

}


/****************************************************************************
 *
 * NAME:       u32Queue_GetDepth
 *
 * DESCRIPTION:
 *	Returns the number of bytes in the queue
 *
 * PARAMETERS: 	Name     		RW  Usage
 *
 * RETURNS:
 *	unsigned long:		Number of bytes in the queue
 *
 ****************************************************************************/
unsigned int u32Queue_GetDepth(tsQueue *psQueue)
{

	/* If pointers are same, nothing in the queue */
	if(psQueue->u32ReadPtr == psQueue->u32WritePtr)
	{
		return(0);
	}


	if(psQueue->u32WritePtr > psQueue->u32ReadPtr)
	{
		return(psQueue->u32WritePtr - psQueue->u32ReadPtr);
	}
	else
	{
		return(psQueue->u32Length - (psQueue->u32ReadPtr - psQueue->u32WritePtr));
	}

}


/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/
/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/


