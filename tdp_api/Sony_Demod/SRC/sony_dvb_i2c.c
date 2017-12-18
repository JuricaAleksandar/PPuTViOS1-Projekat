/*------------------------------------------------------------------------------

 <dev:header>
    Copyright(c) 2009 Sony Corporation.

    $Revision: 838 $
    $Author: sking $

</dev:header>

------------------------------------------------------------------------------*/

#include <sony_dvb_i2c.h>
#include "sony_dvb_stdlib.h"    /* for memcpy */
/*#include "tuner\MxL_User_Define.h"*/

#define BURST_WRITE_MAX 128     /* Max length of burst write */

sony_dvb_result_t dvb_i2c_CommonReadRegister (sony_dvb_i2c_t * pI2c, uint8_t deviceAddress, uint8_t subAddress, uint8_t * pData,
                                              uint32_t size)
{
    sony_dvb_result_t result = SONY_DVB_OK;

    SONY_DVB_TRACE_I2C_ENTER ("dvb_i2c_CommonReadRegister");

    if (!pI2c) {
        SONY_DVB_TRACE_I2C_RETURN (SONY_DVB_ERROR_ARG);
    }

//	UINT8 Data;
//	&Data =  pData;
	

    result = pI2c->Read (pI2c, deviceAddress, subAddress, pData, size, SONY_I2C_START_EN | SONY_I2C_STOP_EN);
    //result = (sony_dvb_result_t) MxL_I2C_Read((UINT8) deviceAddress, (UINT8) subAddress, pData);

    //result = pI2c->Write (pI2c, deviceAddress, &subAddress, 1, SONY_I2C_START_EN);
    /*if (result == SONY_DVB_OK) {
        result = pI2c->Read (pI2c, deviceAddress, pData, size, SONY_I2C_START_EN | SONY_I2C_STOP_EN);
    }*/
    

    SONY_DVB_TRACE_I2C_RETURN (result);
}

sony_dvb_result_t dvb_i2c_CommonWriteRegister (sony_dvb_i2c_t * pI2c, uint8_t deviceAddress, uint8_t subAddress,
                                               const uint8_t * pData, uint32_t size)
{
    sony_dvb_result_t result = SONY_DVB_OK;
    uint8_t buffer[BURST_WRITE_MAX + 1];

    SONY_DVB_TRACE_I2C_ENTER ("dvb_i2c_CommonWriteRegister");

    if (!pI2c) {
        SONY_DVB_TRACE_I2C_RETURN (SONY_DVB_ERROR_ARG);
    }
    if (size > BURST_WRITE_MAX) {
        /* Buffer is too small... */
        SONY_DVB_TRACE_I2C_RETURN (SONY_DVB_ERROR_ARG);
    }

    buffer[0] = subAddress;	
    sony_dvb_memcpy (&(buffer[1]), pData, size);
    /*for (i=0; i<size;i++)
    {
    	buffer[i*2] 	= subAddress+i;
    	buffer[i*2+1]	= *(pData+i);	
    }*/
	//    sony_dvb_memcpy (&(buffer[1]), pData, size);



	/* send the new buffer */
	result = pI2c->Write (pI2c, deviceAddress, buffer, size, SONY_I2C_START_EN | SONY_I2C_STOP_EN);
	//	result = (sony_dvb_result_t)MxL_I2C_Write((UINT8)deviceAddress, buffer, (UINT8) (size*2));	
	SONY_DVB_TRACE_I2C_RETURN (result);
}

sony_dvb_result_t dvb_i2c_CommonWriteOneRegister (sony_dvb_i2c_t * pI2c, uint8_t deviceAddress, uint8_t subAddress,
                                                  uint8_t data)
{
    sony_dvb_result_t result = SONY_DVB_OK;

    SONY_DVB_TRACE_I2C_ENTER ("dvb_i2c_CommonWriteOneRegister");

    if (!pI2c) {
        SONY_DVB_TRACE_I2C_RETURN (SONY_DVB_ERROR_ARG);
    }
    result = pI2c->WriteRegister (pI2c, deviceAddress, subAddress, &data, 1);
    SONY_DVB_TRACE_I2C_RETURN (result);
}

/* For Read-Modify-Write */
//dvb_i2c_SetRegisterBits (pDemod->pI2c, pDemod->i2cAddressT, 0xA5, 0x01, 0x01)
//dvb_i2c_SetRegisterBits (pDemod->pI2c, pDemod->i2cAddressT, 0x82, 0x40, 0x60)
sony_dvb_result_t dvb_i2c_SetRegisterBits (sony_dvb_i2c_t * pI2c, uint8_t deviceAddress, uint8_t subAddress, uint8_t data,
                                           uint8_t mask)
{
    sony_dvb_result_t result = SONY_DVB_OK;

    SONY_DVB_TRACE_I2C_ENTER ("dvb_i2c_SetRegisterBits");

    if (!pI2c) {
        SONY_DVB_TRACE_I2C_RETURN (SONY_DVB_ERROR_ARG);
    }
    if (mask == 0x00) {
        /* Nothing to do */
        SONY_DVB_TRACE_I2C_RETURN (SONY_DVB_OK);
    }

    if (mask != 0xFF) {
        uint8_t rdata = 0x00;
        result = pI2c->ReadRegister (pI2c, deviceAddress, subAddress, &rdata, 1);
        if (result != SONY_DVB_OK)
            SONY_DVB_TRACE_I2C_RETURN (result);
        data = (data & mask) | (rdata & (mask ^ 0xFF));
    }

    result = pI2c->WriteOneRegister (pI2c, deviceAddress, subAddress, data);
    SONY_DVB_TRACE_I2C_RETURN (result);
}
