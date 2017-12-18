#define TWSI_BUS_ID 0
#define TOTAL_TWSI	4


#include "nutune_FT3114_I2C.h"

#ifndef uint
#ifndef ANDROID
typedef unsigned int uint;
#endif
#endif
#include "crules.h"
#include "i2c.h"
//#define NuTune_I2C_DBG

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//																		   //
//					I2C Functions (implement by customer)				   //
//																		   //
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//

/******************************************************************************
**
**  Name: NuTune_I2C_Write
**
**  Description:    I2C write operations
**
**  Parameters:    	
**					DeviceAddr	- NuTune Device address
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
uint32_t NuTune_I2C_Write(uint8_t DeviceAddr, uint8_t* pArray, uint32_t count)
{

	uint32_t i;
	uint8_t GOOD[2];
	unsigned char *pwbuf = pArray;
	unsigned char prbuf[256];
	int rnum = 0;
	int wnum = count;

	#ifdef NuTune_I2C_DBG
	char msgbuf[256];
	#endif

	#ifdef NuTune_I2C_DBG
	for(i=0; i<count; i+=2)
	{
		sprintf(msgbuf, "[I2C Write  ]: DevAddr %x Addr %x, Val %x count %d\n",DeviceAddr, *(pArray+i), *(pArray+i+1),count);
		printf("%s\n", msgbuf);
	}
	#endif

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
	NuTune_Delay(0);

	return 0;	
}

/******************************************************************************
**
**  Name: NuTune_I2C_WriteGW
**
**  Description:    I2C write operations
**
**  Parameters:    	
**					DeviceAddr	- NuTune Device address
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
uint32_t NuTune_I2C_WriteGW(uint8_t DeviceAddr, uint8_t GatewayAddr, uint8_t TunerAddr ,uint8_t* pArray, uint32_t count)
{
	uint8_t *DataArray = (uint8_t*)pArray;
	uint32_t i;
	uint8_t result = 0;

 	unsigned char uart_array[4];
	unsigned char pwbuf[256];
	unsigned char *prbuf = NULL;
	int rnum = 0;
	int wnum = count + 2;

	#ifdef NuTune_I2C_DBG
	char msgbuf[256];
	#endif


	for(i=0; i<count; i+=2)
	{
	#ifdef NuTune_I2C_DBG
		sprintf(msgbuf, "[I2C WriteGW]: DevAddr %2x  TunerAddr %2x GW %x Addr %2x, Val %2x count %2d",DeviceAddr,TunerAddr,GatewayAddr, *(DataArray+i), *(DataArray+i+1),count);
		printf("%s\n", msgbuf);
	#endif	
	}
   
    pwbuf[0] = GatewayAddr;
    pwbuf[1] = TunerAddr;
    for(i=0; i < count; i++)
       pwbuf[i + 2] = pArray[i];

	#ifdef NuTune_I2C_DBG
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
**  Name: NuTune_I2C_Read
**
**  Description:    I2C read operations
**
**  Parameters:    	
**					DeviceAddr	- NuTune Device address
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
uint32_t NuTune_I2C_Read(uint8_t DeviceAddr, uint8_t Addr, uint8_t* mData)
{

	uint32_t Status = 0;
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

	#ifdef NuTune_I2C_DBG
	sprintf(msgbuf, "[I2C Read   ]: DevAddr %x, Sub %x Value %x", DeviceAddr, Addr, mData[0]);
	printf("%s\n", msgbuf);	
	#endif
	

	return Status;

}

/******************************************************************************
**
**  Name: NuTune_I2C_ReadGW
**
**  Description:    I2C read operations
**
**  Parameters:    	
**					DeviceAddr	- NuTune Device address
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
uint32_t NuTune_I2C_ReadGW(uint8_t DeviceAddr, uint8_t GatewayAddr, uint8_t TunerAddr , uint8_t RegAddr, uint8_t* mData, uint32_t count)
{

	uint32_t Status = 0;
	char msgbuf[256];
	unsigned char pwbuf[10];
	int rnum ;
	int wnum ;
   uint8_t* ret_buf = mData;


	#ifdef NuTune_I2C_DBG
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

	#ifdef NuTune_I2C_DBG
		sprintf(msgbuf, "	[I2C ReadGW]: 0x%x \r\n", mData[0]);
		printf("%s\n", msgbuf);		
	#endif
	return Status;
}

/******************************************************************************
**
**  Name: NuTune_Delay
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

void NuTune_Delay(uint32_t mSec)
{
   usleep(mSec);
   return;
}


