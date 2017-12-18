/*------------------------------------------------------------------------------

 <dev:header>
    Copyright(c) 2009 Sony Corporation.

    $Revision$
    $Author$

</dev:header>

------------------------------------------------------------------------------*/
/**
 @file    dvbtc_tuning.c

         This file provides an example of DVB-T/C tuning 
         and monitoring.
*/
/*----------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
 Includes
------------------------------------------------------------------------------*/
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif
#include "..\..\include\sony_dvb_cxd2820.h"
#include "..\..\include\sony_dvb_demod_monitor.h"
#include "..\..\include\sony_dvb_demod_monitorT.h"
#include "..\..\include\sony_dvb_demod_monitorC.h"
#include "..\i2c\i2c_feusb\drvi2c_feusb.h"      /* USB->I2C driver implementation. */
#include "..\tuner\sony_ascot2s.h"              /* Sony ASCOT2S tuner driver. */     
#include "sony_dvb_dbg.h"                       /* Debugging functions. */
#ifdef __cplusplus
}
#endif

#define SONY_DVB_DBG                1       /**< Define to enable debugging output from the example. */

/* #define TUNER_IFAGCPOS           1 */    /**< Define for IFAGC sense positive. */
/* #define TUNER_SPECTRUM_INV       1 */    /**< Define for spectrum inversion. */
#define TUNER_RFLVLMON_DISABLE      1       /**< Define to disable RF level monitoring. */
#define TUNER_SONY_ASCOT            1       /**< Define for Sony CXD2815 tuner. */

#define EX_DVBT_TUNING              1       /**< Define to enable DVB-T tuning example. */
#define EX_DVBC_TUNING              1       /**< Define to enable DVB-C tuning example. */

/*------------------------------------------------------------------------------
 Functions
 ------------------------------------------------------------------------------*/

/* Formatting/trace operations. */
static int g_traceEnable = 0;
static const char* FormatResult (sony_dvb_result_t result) ;
static const char* FormatTsLock (sony_dvb_demod_lock_result_t tsLockResult);

int main (int argc, char* argv[]) {
    
    static const uint32_t dvbtFreqkHz = 682000 ;
    static const uint32_t dvbcFreqkHz = 730000 ;

    sony_dvb_result_t result = SONY_DVB_OK ;
    sony_dvb_result_t tuneResult = SONY_DVB_OK ;
    sony_dvb_cxd2820_t cxd2820 ;
    sony_dvb_demod_t demod  ;
    sony_dvb_tuner_t tuner ;
    sony_dvb_i2c_t demodI2C ;
    sony_dvb_i2c_t tunerI2C ;
    sony_dvb_tune_params_t tuneParams ;
    sony_dvb_i2c_feusb_t feusb ;
    uint8_t i = 0 ;

    argc = argc ;
    argv = argv ;

    /*------------------------------------------------------------------------------
     Setup / Initialisation
    ------------------------------------------------------------------------------*/
    
    /* Setup I2C interfaces. */
    tunerI2C.gwAddress = SONY_DVB_CXD2820_DVBT_ADDRESS ;
    tunerI2C.gwSub = 0x09;  /* Connected via demod I2C gateway function. */
    tunerI2C.Read = drvi2c_feusb_ReadGw ;
    tunerI2C.Write = drvi2c_feusb_WriteGw ;
    tunerI2C.ReadRegister = dvb_i2c_CommonReadRegister;
    tunerI2C.WriteRegister = dvb_i2c_CommonWriteRegister;
    tunerI2C.WriteOneRegister = dvb_i2c_CommonWriteOneRegister;
    tunerI2C.user = (void*) &feusb ;

    /* Setup demod I2C interfaces. */
    demodI2C.gwAddress = 0x00 ;
    demodI2C.gwSub = 0x00;  /* N/A */
    demodI2C.Read = drvi2c_feusb_Read ;
    demodI2C.Write = drvi2c_feusb_Write ;
    demodI2C.ReadRegister = dvb_i2c_CommonReadRegister;
    demodI2C.WriteRegister = dvb_i2c_CommonWriteRegister;
    demodI2C.WriteOneRegister = dvb_i2c_CommonWriteOneRegister;
    demodI2C.user = (void*) &feusb ;

    /* Setup tuner. */
    result = sony_ascot2s_Create (SONY_ASCOT2S_ADDRESS, 4, &tunerI2C, 0, NULL, &tuner);
    if (result != SONY_DVB_OK) {
        printf ("Error: Unable to create Sony ASCOT2S tuner driver. (status=%d, %s)\r\n", result, FormatResult(result));
        return -1 ;
    }

    /* Setup demodulator. */
    result =  dvb_cxd2820_Create (SONY_DVB_CXD2820_DVBT_ADDRESS,
                                  SONY_DVB_CXD2820_DVBC_ADDRESS,
                                  SONY_DVB_CXD2820_DVBT2_ADDRESS,
                                 &demodI2C, 
                                 &demod, 
                                 &tuner, 
                                 &cxd2820);

    /* Configure demod driver for Sony ASCOT2S tuner IF. */
    demod.iffreq_config.config_DVBT6 = DVB_DEMOD_MAKE_IFFREQ_CONFIG_DVBT (3.8);
    demod.iffreq_config.config_DVBT7 = DVB_DEMOD_MAKE_IFFREQ_CONFIG_DVBT (4.5);
    demod.iffreq_config.config_DVBT8 = DVB_DEMOD_MAKE_IFFREQ_CONFIG_DVBT (5.0);
    demod.iffreq_config.config_DVBC = DVB_DEMOD_MAKE_IFFREQ_CONFIG_DVBC (5.1);

    /* T2 ITB setup. */
    demod.iffreq_config.config_DVBT2_5 = DVB_DEMOD_MAKE_IFFREQ_CONFIG_DVBT2 (4.0);
    demod.iffreq_config.config_DVBT2_6 = DVB_DEMOD_MAKE_IFFREQ_CONFIG_DVBT2 (4.0);
    demod.iffreq_config.config_DVBT2_7 = DVB_DEMOD_MAKE_IFFREQ_CONFIG_DVBT2 (4.35);
    demod.iffreq_config.config_DVBT2_8 = DVB_DEMOD_MAKE_IFFREQ_CONFIG_DVBT2 (4.85); 
    
    /* Initialize the I2C */
    result = drvi2c_feusb_Initialize (&feusb, NULL);
    if (result != SONY_DVB_OK) {
        printf ("Error: Unable to initialise FEUSB I2C driver. (status=%d, %s)\r\n", result, FormatResult(result));
        return -1 ;
    }

    /* Initialize the system. */
    result = dvb_cxd2820_Initialize (&cxd2820);
    if (result != SONY_DVB_OK) {
        printf ("Error: Unable to initialise the CXD2820 driver. (status=%d, %s)\r\n", result, FormatResult(result));
        return -1 ;
    }

    /* IFAGC setup. Modify to suit connected tuner. */
    /* IFAGC positive, value = 0. */
#ifdef TUNER_IFAGCPOS
    result = dvb_demod_SetConfig (&demod, DEMOD_CONFIG_IFAGCNEG, 0);
    if (result != SONY_DVB_OK) {
        printf ("Error: Unable to configure IFAGCNEG. (status=%d, %s)\r\n", result, FormatResult(result));
        return -1 ;
    }
#endif

    /* Spectrum Inversion setup. Modify to suit connected tuner. */
    /* Spectrum inverted, value = 1. */
#ifdef TUNER_SPECTRUM_INV
    result = dvb_demod_SetConfig (&demod, DEMOD_CONFIG_SPECTRUM_INV, 1);
    if (result != SONY_DVB_OK) {
        printf ("Error: Unable to configure SPECTRUM_INV. (status=%d, %s)\r\n", result, FormatResult(result));
        return -1 ;
    }
#endif

    /* RF level monitoring (RFAIN/RFAGC) enable/disable. */
    /* Default is enabled. 1: Enable, 0: Disable. */
#ifdef TUNER_RFLVLMON_DISABLE
    result = dvb_demod_SetConfig (&demod, DEMOD_CONFIG_RFLVMON_ENABLE, 0);
    if (result != SONY_DVB_OK) {
        printf ("Error: Unable to configure RFLVMON_ENABLE. (status=%d, %s)\r\n", result, FormatResult(result));
        return -1 ;
    }
#endif

    /* Sony CXD2815 tuner setup. Set to 1 ONLY if Sony CXD2815 tuner is connected. */
#ifdef TUNER_SONY_ASCOT
    demod.is_ascot = 1 ;
#endif

    /*------------------------------------------------------------------------------
     Tune - normal "zapping" on known DVB-T channel.

            Implements a carrier offset compensation mechanism to ensure 
            that large carrier offsets can be handled where the offset is too
            large for the demodulator to achieve TS lock or for optimal performance.

    ------------------------------------------------------------------------------*/
#ifdef EX_DVBT_TUNING
    /* Perform tune (waits for TS lock) */
    tuneParams.bwMHz = 8 ;
    tuneParams.centreFreqkHz = dvbtFreqkHz ;
    tuneParams.system = SONY_DVB_SYSTEM_DVBT ;
    tuneParams.tParams.usePresets = 0 ;
    tuneParams.tParams.profile = DVBT_PROFILE_HP ;

    printf ("Tune to DVB-T signal at %lukHz.\r\n", tuneParams.centreFreqkHz);
    tuneResult = dvb_cxd2820_Tune(&cxd2820, &tuneParams);

    /* Carrier offset compensation. */
    if ((tuneResult == SONY_DVB_ERROR_TIMEOUT) || 
        (tuneResult == SONY_DVB_OK)) {
        int32_t offsetkHz = 0 ;
        uint32_t stepkHz = (SONY_ASCOT2S_OFFSET_CUTOFF_HZ + 500) / 1000;
        
        /* Monitor carrier offset. */
        result = dvb_demod_monitor_CarrierOffset(cxd2820.pDemod, &offsetkHz);
        if (result != SONY_DVB_OK) {
            printf ("Error: Unable to monitor T carrier offset. (status=%d, %s)\r\n", result, FormatResult(result));
            return -1 ;
        }
        
        printf ("DVB-T carrier offset detected. Centre=%lukHz, offset=%ldkHz, step=%lukHz.\r\n", tuneParams.centreFreqkHz, offsetkHz, stepkHz);

        /* Carrier recovery loop locked (demod locked), compensate for the offset and retry tuning. */
        stepkHz = (stepkHz + 1)/2 ;
        if ((uint32_t) abs (offsetkHz) > stepkHz) {
            /* Tuners have only a fixed frequency step size (stepkHz), */
            /* therefore we must query the tuner driver to get the actual */
            /* centre frequency set by the tuner. */
            tuneParams.centreFreqkHz = (uint32_t)((int32_t)cxd2820.pTuner->frequency + offsetkHz) ;
            tuneParams.t2Params.tuneResult = 0 ;    /* Clear result. */
            
            printf ("Re-tuning to compensate offset. New centre=%lukHz.\r\n", tuneParams.centreFreqkHz);
            tuneResult = dvb_cxd2820_Tune(&cxd2820, &tuneParams);
        }
    }
    if (tuneResult != SONY_DVB_OK) {
        printf ("Error: Unable to get TS lock DVB-T signal at %lukHz. (status=%d, %s)\r\n", tuneParams.centreFreqkHz, tuneResult, FormatResult(tuneResult));
        return -1 ;
    }
    printf ("TS locked to DVB-T signal at %lukHz.\r\n", tuneParams.centreFreqkHz);

#ifdef SONY_DVB_DBG
    /* Debugging print. Allow monitors to settle. */
    SONY_DVB_SLEEP(1000);
    sony_dvb_dbg_monitor (cxd2820.pDemod);
#endif

    for (i = 0 ; i < 10 ; i++) {

        /* Monitor Pre-RS (Reed-Solomon) BER / Pre-Viterbi BER. */
        {
            uint32_t ber = 0 ;
            result = dvb_demod_monitorT_PreRSBER (cxd2820.pDemod, &ber);
            if (result == SONY_DVB_OK) {
                printf ("DVB-T Pre RS BER=%lu\r\n", ber);
            }
        }

        {
            uint32_t ber = 0 ;
            result = dvb_demod_monitorT_PreViterbiBER (cxd2820.pDemod, &ber);
            if (result == SONY_DVB_OK) {
                printf ("DVB-T Pre Viterbi BER=%lu\r\n", ber);
            }
        }       
        
        {
            sony_dvb_demod_lock_result_t tsLock = DEMOD_LOCK_RESULT_NOTDETECT ;
            result = dvb_demod_monitor_TSLock (cxd2820.pDemod, &tsLock);
            if (result == SONY_DVB_OK) {
                printf ("DVB-T TS lock=%s\r\n", FormatTsLock (tsLock));
            }
        }

        SONY_DVB_SLEEP (1000);
    }
#endif

    /*------------------------------------------------------------------------------
     Tune - normal "zapping" on known DVB-C channel.
    ------------------------------------------------------------------------------*/
#ifdef EX_DVBC_TUNING
    tuneParams.bwMHz = 8 ;
    tuneParams.centreFreqkHz = dvbcFreqkHz ;
    tuneParams.system = SONY_DVB_SYSTEM_DVBC ;
    
    printf ("Tune to DVB-C signal at %lukHz.\r\n", tuneParams.centreFreqkHz);
    result = dvb_cxd2820_Tune(&cxd2820, &tuneParams);
    if (result != SONY_DVB_OK) {
        printf ("Error: Unable to get TS lock on DVB-C signal at %lukHz. (status=%d, %s)\r\n", dvbcFreqkHz, result, FormatResult(result));
        return -1 ;
    }

    {
        int32_t offset ;
        result = dvb_demod_monitorC_CarrierOffset(cxd2820.pDemod, &offset);
        if (result == SONY_DVB_OK) {
            printf ("DVB-C TS lock achieved at %lukHz, offset=%ldkHz.\r\n", dvbcFreqkHz, offset);
        }
    }

#ifdef SONY_DVB_DBG
    /* Debugging print. Allow monitors to settle. */
    SONY_DVB_SLEEP(1000);
    sony_dvb_dbg_monitor (cxd2820.pDemod);
#endif

    /*------------------------------------------------------------------------------
     DVB-C Monitor - Example monitoring functions (10 seconds).
    ------------------------------------------------------------------------------*/

    for (i = 0 ; i < 10 ; i++) {

        /* Monitor Pre-RS (Reed-Solomon) BER. */
        {
            uint32_t ber = 0 ;
            result = dvb_demod_monitorC_PreRSBER (cxd2820.pDemod, &ber);
            if (result == SONY_DVB_OK) {
                printf ("DVB-C Pre RS BER=%lu\r\n", ber);
            }
        }       

        {
            sony_dvb_demod_lock_result_t tsLock = DEMOD_LOCK_RESULT_NOTDETECT ;
            result = dvb_demod_monitor_TSLock (cxd2820.pDemod, &tsLock);
            if (result == SONY_DVB_OK) {
                printf ("DVB-C TS lock=%s\r\n", FormatTsLock (tsLock));
            }
        }

        SONY_DVB_SLEEP (1000);
    }
#endif

    /*------------------------------------------------------------------------------
     Closing / Shutdown
    ------------------------------------------------------------------------------*/
    
    /* Finalize the device. */
    result = dvb_cxd2820_Finalize (&cxd2820);
    if (result != SONY_DVB_OK) {
        printf ("Error: Unable to finalise the CXD2820 driver. (status=%d, %s)\r\n", result, FormatResult(result));
        return -1 ;
    }

    /* Finalise the I2C */
    result = drvi2c_feusb_Finalize (&feusb);
    if (result != SONY_DVB_OK) {
        printf ("Error: Unable to finalize FEUSB I2C driver. (status=%d, %s)\r\n", result, FormatResult(result));
        return -1 ;
    }

    return 0 ;
}

/*------------------------------------------------------------------------------
  Sample implementation of trace log function
  These functions are declared in sony_dvb.h
------------------------------------------------------------------------------*/
static const char *g_callStack[256];    /* To output function name in TRACE_RETURN log */
static int g_callStackTop = 0;

void dvb_trace_log_enter (const char *funcname, const char *filename, unsigned int linenum)
{
    if (!funcname || !filename)
        return;

    if (g_traceEnable) {
        /*
         * PORTING
         * NOTE: strrchr(filename, '\\')+1 can get file name from full path string.
         * If in linux system, '/' will be used instead of '\\'.
         */
        const char *pFileNameTop = NULL;
        pFileNameTop = strrchr (filename, '\\');
        if (pFileNameTop) {
            pFileNameTop += 1;
        }
        else {
            pFileNameTop = filename;    /* Cannot find '\\' */
        }

        printf ("TRACE_ENTER: %s (%s, line %u)\n", funcname, pFileNameTop, linenum);
    }
    if (g_callStackTop >= (int)(sizeof (g_callStack) / sizeof (g_callStack[0]) - 1)) {
        printf ("ERROR: Call Stack Overflow...\n");
        return;
    }
    g_callStack[g_callStackTop] = funcname;
    g_callStackTop++;
}

void dvb_trace_log_return (sony_dvb_result_t result, const char *filename, unsigned int linenum)
{
    const char *pErrorName = NULL;
    const char *pFuncName = NULL;
    const char *pFileNameTop = NULL;

    if (!filename)
        return ;

    if (g_callStackTop > 0) {
        g_callStackTop--;
        pFuncName = g_callStack[g_callStackTop];
    }
    else {
        printf ("ERROR: Call Stack Underflow...\n");
        pFuncName = "Unknown Func";
    }

    switch (result) {
    case SONY_DVB_OK:
    case SONY_DVB_ERROR_TIMEOUT:
    case SONY_DVB_ERROR_UNLOCK:
    case SONY_DVB_ERROR_CANCEL:
        /* Only print log if FATAL error is occured */
        if (!g_traceEnable) {
            return;
        }
        break;
    case SONY_DVB_ERROR_ARG:
    case SONY_DVB_ERROR_I2C:
    case SONY_DVB_ERROR_SW_STATE:
    case SONY_DVB_ERROR_HW_STATE:
    case SONY_DVB_ERROR_RANGE:
    case SONY_DVB_ERROR_NOSUPPORT:
    case SONY_DVB_ERROR_OTHER:
    default:
        break;
    }

    pErrorName = FormatResult (result);

    pFileNameTop = strrchr (filename, '\\');
    if (pFileNameTop) {
        pFileNameTop += 1;
    }
    else {
        pFileNameTop = filename;    /* Cannot find '\\' */
    }

    printf ("TRACE_RETURN: %s (%s) (%s, line %u)\n", pErrorName, pFuncName, pFileNameTop, linenum);
}

static const char* FormatResult (sony_dvb_result_t result)
{
    char* pErrorName = "UNKNOWN" ;
    switch (result) {
    case SONY_DVB_OK:
        pErrorName = "OK";
        break;
    case SONY_DVB_ERROR_TIMEOUT:
        pErrorName = "ERROR_TIMEOUT";
        break;
    case SONY_DVB_ERROR_UNLOCK:
        pErrorName = "ERROR_UNLOCK";
        break;
    case SONY_DVB_ERROR_CANCEL:
        pErrorName = "ERROR_CANCEL";
        break;
    case SONY_DVB_ERROR_ARG:
        pErrorName = "ERROR_ARG";
        break;
    case SONY_DVB_ERROR_I2C:
        pErrorName = "ERROR_I2C";
        break;
    case SONY_DVB_ERROR_SW_STATE:
        pErrorName = "ERROR_SW_STATE";
        break;
    case SONY_DVB_ERROR_HW_STATE:
        pErrorName = "ERROR_HW_STATE";
        break;
    case SONY_DVB_ERROR_RANGE:
        pErrorName = "ERROR_RANGE";
        break;
    case SONY_DVB_ERROR_NOSUPPORT:
        pErrorName = "ERROR_NOSUPPORT";
        break;
    case SONY_DVB_ERROR_OTHER:
        pErrorName = "ERROR_OTHER";
        break;
    default:
        pErrorName = "ERROR_UNKNOWN";
        break;
    }
    return pErrorName ;
}

static const char* FormatTsLock (sony_dvb_demod_lock_result_t tsLockResult)
{
    char* pName = "Unknown" ;
    switch (tsLockResult) 
    {
    case DEMOD_LOCK_RESULT_NOTDETECT:
        pName = "Not detected" ;
        break ;
    case DEMOD_LOCK_RESULT_LOCKED:
        pName = "Locked" ;
        break ;
    case DEMOD_LOCK_RESULT_UNLOCKED:
        pName = "Unlocked" ;
        break ;
    default: break ;
    }
    return pName ;
}
