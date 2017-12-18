/*------------------------------------------------------------------------------

 <dev:header>
    Copyright(c) 2009 Sony Corporation.

    $Revision: 1535 $
    $Author: sking $

</dev:header>

------------------------------------------------------------------------------*/
#ifndef I2C_FEUSB_H
#define I2C_FEUSB_H

#include <sony_dvb.h>
#include <windows.h>

#define I2C_FEUSB_OK            0
#define I2C_FEUSB_ERROR_ARG     1
#define I2C_FEUSB_ERROR_ACK     2
#define I2C_FEUSB_ERROR_ACCESS  3
#define I2C_FEUSB_ERROR_OTHER   4

#ifndef BOOL
//typedef int BOOL;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* I2C slave address must be in 8bit form! (8'bxxxxxxx0) */

    int i2c_feusb_Read (HANDLE handle, uint8_t deviceAddress, uint8_t * pData, uint32_t numberOfBytes);
    int i2c_feusb_Write (HANDLE handle, uint8_t deviceAddress, const uint8_t * pData, uint32_t numberOfBytes);
    int i2c_feusb_ReadGw (HANDLE handle, uint8_t deviceAddress, uint8_t * pData, uint32_t numberOfBytes, uint8_t gwAddress, uint8_t gwSub);
    int i2c_feusb_WriteGw (HANDLE handle, uint8_t deviceAddress, const uint8_t * pData, uint32_t numberOfBytes,
                           uint8_t gwAddress, uint8_t gwSub);
    int i2c_feusb_ReadRegister (HANDLE handle, uint8_t deviceAddress, uint8_t subAddress, uint8_t* pData, uint32_t numberOfBytes);
    int i2c_feusb_Initialize (HANDLE* handle);
    int i2c_feusb_Initialize (HANDLE* handle);
    int i2c_feusb_Finalize (HANDLE handle);

#ifdef __cplusplus
}                               /* extern "C" */
#endif
#endif                          /* I2C_FEUSB_H */
