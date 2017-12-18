/*------------------------------------------------------------------------------

 <dev:header>
    Copyright(c) 2009 Sony Corporation.

    $Revision$
    $Author$

</dev:header>

------------------------------------------------------------------------------*/

#ifndef SONY_CXD2820_I2C_LISTENER_H
#define SONY_CXD2820_I2C_LISTENER_H

#include "sony_dvb_i2c.h"

typedef struct i2c_listener_t i2c_listener_t ;

typedef void (*sony_dvb_i2c_listener_log_t) (i2c_listener_t* listener, char* buf) ;

/**
 @brief Creates an example of an I2C listener.
 
 @param logger The I2C logger function pointer.
 @param enable 0: indicates not enabled, 1 indicates enabled.
 @param user A user data pointer.
 
 @return NULL if unable to allocate memory. Otherwise instance is
         returned.
*/
i2c_listener_t* i2c_listener_Create (sony_dvb_i2c_listener_log_t logger,
                                     uint8_t enable,
                                     void* user);

void i2c_listener_Destroy (i2c_listener_t* pListener);

/**
 @brief Enable/Disable I2C listener support.
*/
void i2c_listener_Enable (i2c_listener_t * pListener, 
                          uint8_t enable);

/**
 @brief Set the logging callback function.
*/
void i2c_listener_SetLogger (i2c_listener_t * pListener, 
                             sony_dvb_i2c_listener_log_t logger);

void* i2c_listener_GetUserPointer (i2c_listener_t * pListener);


void i2c_listener_LogRead (i2c_listener_t * pListener, 
                           sony_dvb_i2c_t * pI2c,
                            uint8_t deviceAddress, 
                            uint8_t * pData, 
                            uint32_t size,
                            uint8_t mode);

void i2c_listener_LogReadGw (i2c_listener_t * pListener,
                             sony_dvb_i2c_t * pI2c,
                             uint8_t deviceAddress, 
                             uint8_t * pData, 
                             uint32_t size,
                             uint8_t mode);

void i2c_listener_LogWrite (i2c_listener_t * pListener,
                            sony_dvb_i2c_t * pI2c, 
                            uint8_t deviceAddress, 
                            const uint8_t * pData, 
                            uint32_t size,
                            uint8_t mode);

void i2c_listener_LogWriteGw (i2c_listener_t * pListener,
                            sony_dvb_i2c_t * pI2c, 
                            uint8_t deviceAddress, 
                            const uint8_t * pData, 
                            uint32_t size,
                            uint8_t mode);

#endif /* SONY_CXD2820_I2C_LISTENER_H */
