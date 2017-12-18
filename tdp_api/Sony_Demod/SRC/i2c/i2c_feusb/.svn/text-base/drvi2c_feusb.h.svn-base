/*------------------------------------------------------------------------------

 <dev:header>
    Copyright(c) 2009 Sony Corporation.

    $Revision: 1535 $
    $Author: sking $

</dev:header>

------------------------------------------------------------------------------*/


#ifndef DRVI2C_FEUSB_H
#define DRVI2C_FEUSB_H

#include <sony_dvb.h>
#include <sony_dvb_i2c.h>
#include "i2c_listener.h"

// Bridge of I2C code.

typedef struct sony_dvb_i2c_feusb_t {
    void* handle ;
    i2c_listener_t* pListener ;
    void* user ;
} sony_dvb_i2c_feusb_t ;

sony_dvb_result_t drvi2c_feusb_Read (sony_dvb_i2c_t * pI2c, uint8_t deviceAddress, uint8_t subAddress, uint8_t * pData,
                                     uint32_t size, uint8_t mode);

sony_dvb_result_t drvi2c_feusb_Write (sony_dvb_i2c_t * pI2c, uint8_t deviceAddress, const uint8_t * pData,
                                      uint32_t size, uint8_t mode);

sony_dvb_result_t drvi2c_feusb_ReadGw (sony_dvb_i2c_t * pI2c, uint8_t deviceAddress, uint8_t subAddress, uint8_t * pData,
                                       uint32_t size, uint8_t mode);

sony_dvb_result_t drvi2c_feusb_WriteGw (sony_dvb_i2c_t * pI2c, uint8_t deviceAddress, const uint8_t * pData,
                                        uint32_t size, uint8_t mode); 

sony_dvb_result_t drvi2c_feusb_ReadRegister (sony_dvb_i2c_t * pI2c, 
                                             uint8_t deviceAddress, 
                                             uint8_t subAddress, 
                                             uint8_t * pData,
                                             uint32_t size);
sony_dvb_result_t drvi2c_feusb_Initialize (sony_dvb_i2c_feusb_t* pFeusb, i2c_listener_t* pListener);
sony_dvb_result_t drvi2c_feusb_Finalize (sony_dvb_i2c_feusb_t* pFeusb);
sony_dvb_result_t drvi2c_feusb_IsOpen (sony_dvb_i2c_feusb_t* pFeusb, uint8_t* pOpen);

#endif // DRVI2C_FEUSB_H 
