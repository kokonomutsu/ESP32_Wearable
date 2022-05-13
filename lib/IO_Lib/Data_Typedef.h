#ifndef _IO_DATA_TYPEDEF_H_
#define _IO_DATA_TYPEDEF_H_

typedef enum _enumbool{eFALSE=0,eTRUE=!eFALSE} enumbool;

typedef struct
{
  	enumbool (*read)(void);
  	void(*write)(enumbool sta);
  	enumbool (*writeSta)(void);
}IO_Struct;

#endif