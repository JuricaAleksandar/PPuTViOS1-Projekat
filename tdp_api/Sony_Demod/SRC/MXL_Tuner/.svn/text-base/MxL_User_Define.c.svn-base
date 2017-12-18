/*
 
 Driver APIs for MxL201RF Tuner
 
 Copyright, Maxlinear, Inc.
 All Rights Reserved
 
 File Name:      MxL_User_Define.c

 */
#define TWSI_BUS_ID 0
#define TOTAL_TWSI	4

#include "MxL_User_Define.h"
//#include "MxL201RF_Common.h"
#ifndef uint
#ifndef ANDROID
typedef unsigned int uint;
#endif
#endif

#include "i2c.h"
#if 0
#include "../../Common/USB_I2C/usb_i2c.h"
#endif

//static UINT8 i2c_sem = 0;
//const UINT8  sem_sleeptime = 50;

#if 0
#include "OSAL.h"
extern OSAL_HANDLE_T hdlSemaphore;
#endif


//CRITICAL_SECTION cs;

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//																		   //
//					I2C Functions (implement by customer)				   //
//																		   //
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//

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
UINT32 MxL_I2C_Write(UINT8 DeviceAddr, UINT8* pArray, UINT32 count)
{

	UINT32 i;
	UINT8 GOOD[2];
	#ifdef MxL_I2C_DBG
	char msgbuf[256];
	#endif

	#ifdef MxL_I2C_DBG
	for(i=0; i<count; i+=2)
	{

		sprintf(msgbuf, "[I2C Write  ]: DevAddr %x Addr %x, Val %x count %d\n",DeviceAddr, *(pArray+i), *(pArray+i+1),count);
		printf("%s\n", msgbuf);

	}
	#endif

	unsigned char *pwbuf = pArray;
	unsigned char prbuf[256];
	int rnum = 0;
	int wnum = count;

   for(i=0; i<count; i+=2)
	{
	   GOOD[0] = *(pArray + i);
	   GOOD[1] = *(pArray + i + 1);
       wnum = 2;
       rnum = 0;

	   if (TWSI_writeread_bytes(TWSI_BUS_ID, DeviceAddr, 0, GOOD, wnum, prbuf, rnum, 0) != 0) 
	   {
		   printf("[I2C Write  ]:	ERROR\n");
		   return -1;
	   }
	}
//BAKO
	MxL_Delay(0);
	/*if (TWSI_writeread_bytes(TWSI_BUS_ID, DeviceAddr, 0, pwbuf, wnum, prbuf, rnum, 0) != 0) 
	{
		printf("[I2C Write  ]:	ERROR\n");
		return -1;
	}*/
	
	return 0;	
}

/******************************************************************************
**
**  Name: MxL_I2C_WriteGW
**
**  Description:    I2C write operations
**
**  Parameters:    	
**					DeviceAddr	- MxL201RF Device address
**					GatewayAddr    - Gateway address
**					TunerAddr		- Tuner address					
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
UINT32 MxL_I2C_WriteGW(UINT8 DeviceAddr, UINT8 GatewayAddr, UINT8 TunerAddr ,UINT8* pArray, UINT32 count)
{
	UINT8 *DataArray = (UINT8*)pArray;
	UINT32 i;
	UINT8 result = 0;

	#ifdef MxL_I2C_DBG
	char msgbuf[256];
	#endif
	
	/*for(i=0; i<count; i+=2)
	{
	#ifdef MxL_I2C_DBG
		//sprintf(msgbuf, "[I2C WriteGW]: DevAddr %2x  TunerAddr %2x GW %x Addr %2x, Val %2x count %2d",DeviceAddr,TunerAddr,GatewayAddr, *(DataArray+i), *(DataArray+i+1),count);
		//printf("%s\n", msgbuf);
	#endif	
	}*/
    unsigned char uart_array[4];
	unsigned char pwbuf[256];
	unsigned char *prbuf = NULL;
	int rnum = 0;
	int wnum = count + 2;

    pwbuf[0] = GatewayAddr;
    pwbuf[1] = TunerAddr<<1;
    for(i=0; i < count; i++)
       pwbuf[i + 2] = pArray[i];

	#ifdef MxL_I2C_DBG
    printf("[I2C WriteGW]:[start]");
    for(i=0; i < wnum; i++)
		printf("0x%x ", pwbuf[i]);  

    printf("[stop]\n"); 
   #endif
	
#if 0
	for(i=0; i<count; i+=2)
	{
      uart_array[0] = GatewayAddr;
      uart_array[1] = TunerAddr<<1;
      uart_array[2] = *(DataArray+i);
      uart_array[3] = *(DataArray+i + 1);
     wnum = 4;
     rnum = 0;
	   printf("[I2C WriteGW  ]:	Addr: 0x%x    Val: 0x%x \n", *(DataArray+i), *(DataArray+i + 1));

      if (TWSI_writeread_bytes(TWSI_BUS_ID, DeviceAddr, 0, uart_array, wnum, prbuf, rnum, 0) != 0) 
	  {
		  printf("[I2C WriteGW  ]:	ERROR\n");
		  return -1;
	  }	
	}
#endif

	if (TWSI_writeread_bytes(TWSI_BUS_ID, DeviceAddr, 0, pwbuf, wnum, prbuf, rnum, 0) != 0) 
	{
		printf("[I2C WriteGW  ]:	ERROR\n");
		return -1;
	}
    
    return result;	

}


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
UINT32 MxL_I2C_Read(UINT8 DeviceAddr, UINT8 Addr, UINT8* mData)
{

	UINT32 Status = 0;
	char msgbuf[256];
	unsigned char pwbuf[256];
	int rnum ;
	int wnum ;
    uint8_t* ret_buf = mData;


	pwbuf[0] = Addr; 
	rnum = 1;
	wnum = 1;
	if (TWSI_writeread_bytes(TWSI_BUS_ID, DeviceAddr, 0, pwbuf, wnum, ret_buf, rnum, 0) != 0) 
	{
		printf("Write subaddress failed. \n");
		return -1;
	}

	#ifdef MxL_I2C_DBG
	sprintf(msgbuf, "[I2C Read   ]: DevAddr %x, Sub %x Value %x", DeviceAddr, Addr, mData[0]);
	printf("%s\n", msgbuf);	
	#endif
	

	return Status;

}

/******************************************************************************
**
**  Name: MxL_I2C_ReadGW
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
UINT32 MxL_I2C_ReadGW(UINT8 DeviceAddr, UINT8 GatewayAddr, UINT8 TunerAddr , UINT8 RegAddr, UINT8* mData, UINT32 count)
{

	UINT32 Status = 0;
	char msgbuf[256];
	unsigned char pwbuf[10];
	int rnum ;
	int wnum ;
   uint8_t* ret_buf = mData;


	#ifdef MxL_I2C_DBG
	sprintf(msgbuf, "[I2C ReadGW ]: DevAddr %2x, TunerAddr %2x Sub %2x Reg %2x \n", DeviceAddr,TunerAddr, TunerAddr, RegAddr);
	printf("%s\n", msgbuf);		
	#endif
	
	// set register address
   memset(pwbuf,0x00,10);
   pwbuf[0] = GatewayAddr;
   pwbuf[1] = (TunerAddr<<1);// + 1;
	pwbuf[2] = 0xFB;
	pwbuf[3] = RegAddr;
	rnum = 0;
	wnum = 4;
	if (TWSI_writeread_bytes(TWSI_BUS_ID, DeviceAddr, 0, pwbuf, wnum, ret_buf, rnum, 0) != 0) 
	{
		printf("[I2C ReadGW]: Write subaddress failed. \n");
		return -1;
	}
   memset(pwbuf,0x00,10);
   rnum = count;
	wnum = 2;
   pwbuf[0] = GatewayAddr;
   pwbuf[1] = (TunerAddr<<1) + 1;
	//pwbuf[2] = RegAddr;

	if (TWSI_writeread_bytes(TWSI_BUS_ID, DeviceAddr, 0, pwbuf, wnum, ret_buf, rnum, 0) != 0) 

	{
		printf("[I2C ReadGW]: Write subaddress failed. \n");
		return -1;
	}

	#ifdef MxL_I2C_DBG
		sprintf(msgbuf, "	[I2C ReadGW]: 0x%x \r\n", mData[0]);
		printf("%s\n", msgbuf);		
	#endif
	return Status;
}

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

void MxL_Delay(UINT32 mSec)
{
   usleep(mSec);
   return;
}


