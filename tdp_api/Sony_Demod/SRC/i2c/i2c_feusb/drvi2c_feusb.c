/*------------------------------------------------------------------------------

 <dev:header>
    Copyright(c) 2009 Sony Corporation.

    $Revision: 1535 $
    $Author: sking $

</dev:header>

------------------------------------------------------------------------------*/



#include "drvi2c_feusb.h"
#include "i2c_listener.h"



#ifdef MXL_Tuner
#define MxL210RF_I2C_ADDRESS_PATH1	99
#include "MxL_User_Define.h"
#endif

#ifdef NuTune_Tuner
#define NUTUNE_I2C_ADDRESS_PATH1	0x6C
#include "nutune_FT3114_I2C.h"
#endif

sony_dvb_result_t drvi2c_feusb_Read (sony_dvb_i2c_t * pI2c, uint8_t deviceAddress, uint8_t subAddress,uint8_t * pData, uint32_t size, uint8_t mode)
{
    sony_dvb_result_t result = SONY_DVB_OK;
    int i;
	
    for(i=0;i<(size);i++)
    {
#ifdef MXL_Tuner
    	result = (sony_dvb_result_t) MxL_I2C_Read((uint8_t) deviceAddress, (uint8_t) (subAddress+i), &pData[i]);
#endif

#ifdef NuTune_Tuner
    	result = (sony_dvb_result_t) NuTune_I2C_Read((uint8_t) deviceAddress, (uint8_t) (subAddress+i), &pData[i]);
#endif
    }

	
    return result;

}

sony_dvb_result_t drvi2c_feusb_Write (sony_dvb_i2c_t * pI2c, uint8_t deviceAddress, const uint8_t * pData,
                                      uint32_t size, uint8_t mode)
{

    sony_dvb_result_t result = SONY_DVB_OK;
    uint8_t buffer[128 + 1];
    uint8_t i;

    mode = mode;  // remove warning msg

    for (i=0; i<size;i++)
    {
    	buffer[i*2] 	= pData[0]+i;
    	buffer[i*2+1]	= *(pData+ i + 1);	
    }
#ifdef MXL_Tuner
	result = (sony_dvb_result_t)MxL_I2C_Write((uint8_t)deviceAddress, buffer, (uint8_t) (size*2));
#endif
#ifdef NuTune_Tuner
	result = (sony_dvb_result_t)NuTune_I2C_Write((uint8_t)deviceAddress, buffer, (uint8_t) (size*2));
#endif

    return	result;
	
}

// For gateway read access
sony_dvb_result_t drvi2c_feusb_ReadGw (sony_dvb_i2c_t * pI2c, uint8_t deviceAddress, uint8_t subAddress, uint8_t * pData,
                                       uint32_t size, uint8_t mode)
{    
    sony_dvb_result_t result = SONY_DVB_OK;

	mode = mode;
#ifdef MXL_Tuner
    result =  (sony_dvb_result_t)MxL_I2C_ReadGW(deviceAddress, pI2c->gwSub , MxL210RF_I2C_ADDRESS_PATH1 , subAddress, pData, size);
#endif

#ifdef NuTune_Tuner
	 result =  (sony_dvb_result_t)NuTune_I2C_ReadGW(deviceAddress, pI2c->gwSub ,NUTUNE_I2C_ADDRESS_PATH1 , subAddress, pData, size);
#endif

	return result;

 
}

// For gateway write access
sony_dvb_result_t drvi2c_feusb_WriteGw (sony_dvb_i2c_t * pI2c, uint8_t deviceAddress, const uint8_t * pData,
                                        uint32_t size, uint8_t mode)
{
    sony_dvb_result_t result = SONY_DVB_OK;
    uint8_t buffer[128 + 1];
    uint8_t i;

    mode = mode;  // remove warning msg

#ifdef MXL_Tuner
    for (i=0; i<size;i++)
    {
    	buffer[i*2] 	= pData[0]+i;
    	buffer[i*2+1]	= *(pData+ i + 1);	
    }
 	 result =  (sony_dvb_result_t)MxL_I2C_WriteGW(deviceAddress, pI2c->gwSub , MxL210RF_I2C_ADDRESS_PATH1 ,buffer, (uint8_t) (size*2));
#endif

#ifdef NuTune_Tuner
	memset(buffer,0,128);
	memcpy(buffer,pData,size);
   result =  (sony_dvb_result_t)NuTune_I2C_WriteGW(NUTUNE_I2C_ADDRESS_PATH1, pI2c->gwSub ,deviceAddress,buffer, (uint8_t) (size));

#endif

	return result;

	
}

