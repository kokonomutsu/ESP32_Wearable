#ifndef _IO_DATA_TYPEDEF_H_
#define _IO_DATA_TYPEDEF_H_

typedef enum _enumbool{eFALSE=0,eTRUE=!eFALSE} enumbool;

typedef struct
{
  	char (*read)(void);
  	void(*write)(char sta);
  	enumbool (*writeSta)(void);
}IO_Struct;

#endif