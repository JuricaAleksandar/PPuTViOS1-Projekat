/*

 User defined Data Types and API Functions for MxL201RF Tuner
 
 Copyright, Maxlinear, Inc.
 All Rights Reserved
 
 File Name:      MxL201RF_API.c
 Date Created:  Jan. 20, 2009
 Version:    5.1.6
*/


#ifndef __MxL_USER_DEFINE_H
#define __MxL_USER_DEFINE_H


/******************************************************************************
    User-Defined Types (Typedefs)
******************************************************************************/
//#include <windows.h>
#include <stdio.h>
//#include <process.h>


//#define MxL_I2C_DBG 1
//#define SONY_MXL_2TUNER_ONLY 1
#if 0
static HANDLE hMutex;
#endif

#define MxL210RF_I2C_ADDRESS_PATH1	99
#define MxL210RF_I2C_ADDRESS_PATH2	96
#define MxL210RF_SONYCXD2820_GW_ADDR 0x09
#define SONY2820R_ADDR_NULL		0
#define MIN_RF_FREQ	44
#define MAX_RF_FREQ	1000


typedef unsigned char  UINT8;
typedef unsigned short UINT16;
typedef unsigned int   UINT32;
typedef char           SINT8;
typedef short          SINT16;
typedef int            SINT32;
typedef float          REAL32;



// BOOL is defined in windows.h
/*
typedef enum
{
  TRUE = 1,
  FALSE = 0
} BOOL;
*/


/******************************************************************************
**
**  Name: MxL_I2C_Write
**
**  Description:    I2C write operations
**
**  Parameters:    	
**					DeviceAddr	- MxL201RF Device address
**					pArray		- Write data array pointer
**					count		- total number of array
**
**  Returns:        0 if success
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   12-16-2007   khuang initial release.
**
******************************************************************************/
UINT32 MxL_I2C_Write(UINT8 DeviceAddr, UINT8* pArray, UINT32 count);

/******************************************************************************
**
**  Name: MxL_I2C_Write
**
**  Description:    I2C write operations
**
**  Parameters:    	
**					DeviceAddr	- MxL201RF Device address
**					pArray		- Write data array pointer
**					count		- total number of array
**
**  Returns:        0 if success
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   12-16-2007   khuang initial release.
**
******************************************************************************/
UINT32 MxL_I2C_WriteGW(UINT8 DeviceAddr, UINT8 GatewayAddr, UINT8 TunerAddr, UINT8* pArray, UINT32 count);


/******************************************************************************
**
**  Name: MxL_I2C_Read
**
**  Description:    I2C read operations
**
**  Parameters:    	
**					DeviceAddr	- MxL201RF Device address
**					Addr		- register address for read
**					*Data		- data return
**
**  Returns:        0 if success
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   12-16-2007   khuang initial release.
**
******************************************************************************/
UINT32 MxL_I2C_Read(UINT8 DeviceAddr, UINT8 Addr, UINT8* mData);

/******************************************************************************
**
**  Name: MxL_I2C_ReadGW
**
**  Description:    I2C read operations
**
**  Parameters:    	
**					DeviceAddr	- MxL201RF Device address
**					Addr			- register address for read
**					*Data		- data return
**
**  Returns:        0 if success
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   12-16-2007   khuang initial release.
**
******************************************************************************/


UINT32 MxL_I2C_ReadGW(UINT8 DeviceAddr, UINT8 GatewayAddr, UINT8 TunerAddr , UINT8 RegAddr, UINT8* mData, UINT32 count);


/******************************************************************************
**
**  Name: MxL_Delay
**
**  Description:    Delay function in milli-second
**
**  Parameters:    	
**					mSec		- milli-second to delay
**
**  Returns:        0
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   12-16-2007   khuang initial release.
**
******************************************************************************/
void MxL_Delay(UINT32 mSec);

# endif //__MxL_USER_DEFINE_H
