/*------------------------------------------------------------------------------

 <dev:header>
    Copyright(c) 2009 Sony Corporation.

    $Revision$
    $Author$

</dev:header>

------------------------------------------------------------------------------*/
/**
 @file    i2c_listener.c

        This file provides an example logging I2C driver implementation.
        Accesses to the I2C driver are logged as ASCII text to the attached
        logging function (without line endings).
*/
/*----------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
 Includes
------------------------------------------------------------------------------*/
#include "i2c_listener.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define MAX_LOG_BUF 1024 

#if defined _MSC_VER
/* MS Windows safe snprintf and strncat */
#define sony_dvb_snprintf sprintf_s 
#define sony_dvb_strncat(dest,dest_len,src,src_len) strncat_s((dest),(dest_len),(src),(src_len)) 
#else
/* Other safe snprintf and strncat */
#define sony_dvb_snprintf snprintf 
#define sony_dvb_strncat(dest,dest_len,src,src_len) strncat((dest),(src),(src_len))
#endif

/**
 @brief Internal structure used for logging.
*/
struct i2c_listener_t {
    uint8_t enable ;
    sony_dvb_i2c_listener_log_t logger ;
    uint8_t currentI2CAddress ;
    char buf[MAX_LOG_BUF] ;
    void* user ;
} ;

/*------------------------------------------------------------------------------
 Functions
 ------------------------------------------------------------------------------*/

static void appendChar (i2c_listener_t* listener, uint8_t c)
{
	// [Oliver] mark the print function of SONY because it is not referenced.
    //		char temp[5] ;
    //		sony_dvb_snprintf (temp, sizeof(temp), " h%.2x", c);
    //		sony_dvb_strncat (listener->buf, MAX_LOG_BUF, temp, sizeof(temp));
}

static void checkI2CAddress (i2c_listener_t* listener, uint8_t deviceAddress)
{
    if (deviceAddress != listener->currentI2CAddress) {
		// [Oliver] mark the print function of SONY because it is not referenced.
		//		sony_dvb_snprintf (listener->buf, MAX_LOG_BUF, "sid"); 
        appendChar (listener, deviceAddress >> 1);
        listener->currentI2CAddress = deviceAddress ;
        if (listener->logger) {
            listener->logger (listener, listener->buf);
        }
    }
}

static void i2c_listener_LogWriteAll (i2c_listener_t * pListener,
                            sony_dvb_i2c_t * pI2c, 
                            uint8_t deviceAddress, 
                            const uint8_t * pData, 
                            uint32_t size,
                            uint8_t mode,
                            uint8_t gw,
                            uint8_t readWrite)
{

    mode = mode ;

    if (pListener->enable) {
        uint32_t i = 0 ;

        /* Check I2C address changes. */
        if (gw != 0) {
            checkI2CAddress (pListener, pI2c->gwAddress);
        }
        else {
            checkI2CAddress (pListener, deviceAddress);
        }

//        sony_dvb_snprintf (pListener->buf, MAX_LOG_BUF, "w bb");
        if (gw != 0) {
            appendChar (pListener, pI2c->gwSub);
            /* For a write for read accesses, do not pre-pend the device address. */
            if (!readWrite) {
                appendChar (pListener, deviceAddress);
            }
        }

        /* Print the data. */
        for (i = 0 ; i < size; i++) {
            appendChar (pListener, pData[i]);
        }
        if (pListener->logger) {
            pListener->logger (pListener, pListener->buf);
        }
    }
}

static void i2c_listener_LogReadAll (i2c_listener_t * pListener, 
                             sony_dvb_i2c_t * pI2c,
                            uint8_t deviceAddress, 
                            uint8_t * pData, 
                            uint32_t size,
                            uint8_t mode,
                            uint8_t gw)
{
    if (pListener->enable) {
        uint32_t i = 0 ;
        if (gw != 0) {
            checkI2CAddress (pListener, pI2c->gwAddress);
        }
        else  {
            checkI2CAddress (pListener, deviceAddress);
        }
        
        if (gw != 0) {
            uint8_t data = deviceAddress | 0x01 ;
            /* Writes the read address into the gateway. */
            i2c_listener_LogWriteAll (pListener, pI2c, deviceAddress, &data, sizeof(data), mode, gw, 1);
        }
        
//        sony_dvb_snprintf (pListener->buf, MAX_LOG_BUF, "r bb");

        for (i = 0 ; i < size ; i++) {
            appendChar (pListener, pData[i]);
        }

        if (pListener->logger) {
            pListener->logger (pListener, pListener->buf);
        }
    }
}

void i2c_listener_LogRead (i2c_listener_t * pListener, 
                           sony_dvb_i2c_t * pI2c,
                            uint8_t deviceAddress, 
                            uint8_t * pData, 
                            uint32_t size,
                            uint8_t mode)
{
    i2c_listener_LogReadAll (pListener, pI2c, deviceAddress, pData, size, mode, 0);
}

void i2c_listener_LogReadGw (i2c_listener_t * pListener,
                             sony_dvb_i2c_t * pI2c,
                             uint8_t deviceAddress, 
                             uint8_t * pData, 
                             uint32_t size,
                             uint8_t mode)
{
    i2c_listener_LogReadAll (pListener, pI2c, deviceAddress, pData, size, mode, 1);
}

void i2c_listener_LogWrite (i2c_listener_t * pListener,
                            sony_dvb_i2c_t * pI2c, 
                            uint8_t deviceAddress, 
                            const uint8_t * pData, 
                            uint32_t size,
                            uint8_t mode)
{    
    i2c_listener_LogWriteAll (pListener, pI2c, deviceAddress, pData, size, mode, 0, 0);
}

void i2c_listener_LogWriteGw (i2c_listener_t * pListener,
                            sony_dvb_i2c_t * pI2c, 
                            uint8_t deviceAddress, 
                            const uint8_t * pData, 
                            uint32_t size,
                            uint8_t mode)
{
    i2c_listener_LogWriteAll (pListener, pI2c, deviceAddress, pData, size, mode, 1, 0);
}

i2c_listener_t* i2c_listener_Create (sony_dvb_i2c_listener_log_t logger,
                                     uint8_t enable,
                                     void* user)
{
    i2c_listener_t* pListener = (i2c_listener_t*) malloc (sizeof(i2c_listener_t));
    if (pListener == NULL) {
        return pListener ;
    }
    
    /* Configure listener. */
    pListener->enable = enable ;
    pListener->logger = logger ;
    pListener->currentI2CAddress = 0 ;
    memset (pListener->buf, 0, MAX_LOG_BUF);
    pListener->user = user ;

    return pListener ;
}

void i2c_listener_Destroy (i2c_listener_t * pListener)
{
    if (pListener != NULL) {
        free(pListener);
    }
}

void i2c_listener_Enable (i2c_listener_t * pListener, 
                          uint8_t enable)
{
    pListener->enable = enable ;
}

void i2c_listener_SetLogger (i2c_listener_t * pListener, 
                             sony_dvb_i2c_listener_log_t logger)
{
    pListener->logger = logger ;
}

void* i2c_listener_GetUserPointer (i2c_listener_t * pListener)
{
    if (pListener)
    {
        return pListener->user ;
    }
    return NULL ;
}
