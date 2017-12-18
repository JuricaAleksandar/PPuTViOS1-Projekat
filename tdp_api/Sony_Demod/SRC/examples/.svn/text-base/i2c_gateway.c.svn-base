/*------------------------------------------------------------------------------

 <dev:header>
    Copyright(c) 2009 Sony Corporation.

    $Revision$
    $Author$

</dev:header>

------------------------------------------------------------------------------*/
/**
 @file    i2c_gateway.c

        This file provides an example implementation of the 
        sony_dvb_i2c_t::Read and sony_dvb_i2c_t::Write function
        pointers for tuners connected through the demodulator 
        I2C gateway function.

        This implementation uses a sony_dvb_i2c_t instance 
        as the underlying I2C driver, then adds the neccessary 
        payloads and calls to I2C to allow for gateway I2C 
        communication.

        \verbatim
        +--------+          +--------------+         +---------+
        | HOST   |  I2C     | CXD2820R     |  I2C    | Tuner   |
        |        |<========>| I2C GW (0x09)|<=======>|         |
        +--------+          +--------------+         +---------+
        \endverbatim

        I2C Gateway Access
        ==================
         \verbatim
        The Read gateway function pointer should:
        {S} [Addr] [GW Sub] [TunerAddr+Read] {SR} [Addr+Read] [Read0] [Read1] ... [ReadN] {P}

        The Write gateway function pointer should:
        {S} [Addr] [GW Sub] [TunerAddr] [Data0] [Data1] ... [DataN] {P}

        Where:
        {S} : Start condition
        {P} : Stop condition
        {SR}: Start repeat condition
        \endverbatim
*/
/*----------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
 Includes
------------------------------------------------------------------------------*/
#include "i2c_gateway.h"
#include <string.h>
#define MAX_I2C_WRITE   256

/*------------------------------------------------------------------------------
 Functions
 ------------------------------------------------------------------------------*/

static sony_dvb_result_t i2c_gw_Read (sony_dvb_i2c_t * pI2c, 
                                       uint8_t deviceAddress, 
                                       uint8_t * pData, 
                                       uint32_t size,
                                       uint8_t mode)
{
    sony_dvb_i2c_t* hostI2C = (sony_dvb_i2c_t*) pI2c->user ;
    uint8_t data[2] ;
    
    /* Remove compiler warnings. */
    mode = SONY_I2C_START_EN | SONY_I2C_STOP_EN ;

    /* Check arguments */
    if ((!pI2c) || (!pData)) {
        return SONY_DVB_ERROR_ARG;
    }
    
    /* {S} [Addr] [GW Sub] [TunerAddr+Read] */
    data[0] = pI2c->gwSub ;
    data[1] = deviceAddress ;
    if (hostI2C->Write (hostI2C, pI2c->gwAddress,  data, sizeof(data), SONY_I2C_START_EN)
        != SONY_DVB_OK) {
        return SONY_DVB_ERROR_I2C ;
    }

    /* {SR} [Addr+Read] [Read0] [Read1] ... [ReadN] {P} */
    return hostI2C->Read (hostI2C, pI2c->gwAddress, pData, size, mode);
}

static sony_dvb_result_t i2c_gw_Write (sony_dvb_i2c_t* pI2c, 
                                        uint8_t deviceAddress, 
                                        const uint8_t * pData, 
                                        uint32_t size,
                                        uint8_t mode)
{
    uint8_t data[MAX_I2C_WRITE] ;
    uint8_t* tmpData = data ; 
    sony_dvb_i2c_t* hostI2C = (sony_dvb_i2c_t*) pI2c->user ;
    mode = SONY_I2C_START_EN | SONY_I2C_STOP_EN ;

    /* Check arguments */
    if ((!pI2c) || (!pData)) {
        return SONY_DVB_ERROR_ARG;
    }

    /* Protect against overflow: GW Sub Adress (1) + Tuner address (1) */
    if (size > (MAX_I2C_WRITE - (1 + 1))) {
        return SONY_DVB_ERROR_ARG;
    }

    /* Payload: [GWSub] [TunerAddress] [Data0] ... [DataN] */
    *(tmpData++) = pI2c->gwSub ;
    *(tmpData++) = deviceAddress ;
    memcpy (tmpData, pData, size);

    /* Perform real I2C write. */
    return hostI2C->Write (hostI2C, 
        pI2c->gwAddress, 
        data,
        size + 2, 
        mode);
}

void i2c_gw_Create (sony_dvb_i2c_t* hostI2C,
                    uint8_t gwAddress,
                    uint8_t gwSub,
                    sony_dvb_i2c_t * gwI2C)
{
    gwI2C->user = (void*) hostI2C ;
    gwI2C->gwAddress = gwAddress ;
    gwI2C->gwSub = gwSub ;
    gwI2C->Read = i2c_gw_Read ;
    gwI2C->Write = i2c_gw_Write ;
    gwI2C->ReadRegister = dvb_i2c_CommonReadRegister ;
    gwI2C->WriteRegister = dvb_i2c_CommonWriteRegister ;
    gwI2C->WriteOneRegister = dvb_i2c_CommonWriteOneRegister ;
}
