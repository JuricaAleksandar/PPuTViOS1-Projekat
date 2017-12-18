/*------------------------------------------------------------------------------

 <dev:header>
    Copyright(c) 2009 Sony Corporation.

    $Revision: 840 $
    $Author: sking $

</dev:header>

------------------------------------------------------------------------------*/

#ifndef SONY_CXD2820_EXAMPLE_I2C_GW_H
#define SONY_CXD2820_EXAMPLE_I2C_GW_H

#ifdef __cplusplus
extern "C" {
#endif
#include "..\..\include\sony_dvb_i2c.h"

#ifdef __cplusplus
}
#endif

/**
 @brief Creates an example of an I2C gateway interface. 
 
 @param hostI2C The I2C driver used to communicate with the gateway.
 @param gwAddress The gateway address (8-bit).
 @param gwSub The sub address of the gateway.
 @param gwI2C The gateway I2C driver. 
 
*/
void i2c_gw_Create (sony_dvb_i2c_t* hostI2C,
                    uint8_t gwAddress,
                    uint8_t gwSub,
                    sony_dvb_i2c_t * gwI2C);

#endif /* SONY_CXD2820_EXAMPLE_I2C_GW_H */
