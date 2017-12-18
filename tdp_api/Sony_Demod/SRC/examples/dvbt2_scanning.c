/*------------------------------------------------------------------------------

 <dev:header>
    Copyright(c) 2009 Sony Corporation.

    $Revision$
    $Author$

</dev:header>

------------------------------------------------------------------------------*/
/**
 @file    dvbt2_tuning.c

         This file provides an example of DVB-T/T2 scanning.
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
#include "..\i2c\i2c_feusb\drvi2c_feusb.h"      /* USB->I2C driver implementation. */
#include "..\tuner\sony_ascot2s.h"              /* Sony ASCOT2S tuner driver. */
#ifdef __cplusplus
}
#endif

/* #define TUNER_IFAGCPOS           1 */    /**< Define for IFAGC sense positive. */
/* #define TUNER_SPECTRUM_INV       1 */    /**< Define for spectrum inversion. */
#define TUNER_RFLVLMON_DISABLE      1       /**< Define to disable RF level monitoring. */
#define TUNER_SONY_ASCOT            1       /**< Define for Sony CXD2815 tuner. */

/*------------------------------------------------------------------------------
 Functions
 ------------------------------------------------------------------------------*/

/* Formatting/trace operations. */
static int g_traceEnable = 0;
static const char* FormatResult (sony_dvb_result_t result) ;
static const char* FormatSystem (sony_dvb_system_t system);

/*------------------------------------------------------------------------------
 Function: Example Scanning Callback
 ------------------------------------------------------------------------------*/

/**
 @brief Example scanning call-back function that implements 
        carrier offset compensation for DVB-T/T2/C.
        It also provides an example scan progress indicator.

        Carrier offset compensation allows the demodulator 
        to lock to signals that have large enough carrier offsets
        as to prevent TS lock.
        Carrier offset compensation is important to real world tuning
        and scanning and is highly recommended.

 @param pDriver The driver instance.
 @param pResult The scan result.

*/
static void example_scan_callback (sony_dvb_cxd2820_t * pDriver, 
                                   sony_dvb_scan_result_t * pResult)
{
    sony_dvb_result_t result = SONY_DVB_OK ;
    int32_t offsetkHz = 0 ;
    uint32_t stepkHz = 0 ;
    uint32_t compFreqkHz = 0 ;
    uint32_t percComplete = 0 ;
    
    if (!pDriver || !pResult)
        return ;

    /* Print % complete. */
    if (pResult->scanParams->endFrequencykHz > pResult->scanParams->startFrequencykHz) {    
        percComplete = (pResult->centreFreqkHz - pResult->scanParams->startFrequencykHz) * 100 / 
            (pResult->scanParams->endFrequencykHz - pResult->scanParams->startFrequencykHz);
        printf ("Scan: %lu%%.\r", percComplete);
    }

    /* Found channel. */
    if (pResult->tuneResult == SONY_DVB_OK) {
        printf ("\r\n");

        /* Check carrier offset to see if it should be compensated. */
        result = dvb_demod_monitor_CarrierOffset (pDriver->pDemod, &offsetkHz);
        if (result != SONY_DVB_OK) {
            printf ("Error: Unable to monitor carrier offset. (status=%d, %s)\r\n", result, FormatResult(result));
            return ;
        }

        /* Get step size of tuner in current system (T/T2/C). */
        switch (pResult->system)
        {
            /* Intentional fall-through. */
            case SONY_DVB_SYSTEM_DVBT:
            case SONY_DVB_SYSTEM_DVBT2:
                stepkHz = (SONY_ASCOT2S_OFFSET_CUTOFF_HZ + 500) / 1000 ;
                break ;

            case SONY_DVB_SYSTEM_DVBC:
                stepkHz = (SONY_ASCOT2S_OFFSET_CUTOFF_HZ + 500) / 1000 ;
                break ;
            case SONY_DVB_SYSTEM_UNKNOWN:
            default:
                printf ("Error: Unknown system.\r\n");
                return ;
        }
    
        /* Display channel. */
        printf ("Found %s channel at %lukHz, offset=%ldkHz, step=%lukHz.\r\n", FormatSystem (pResult->system), pDriver->pTuner->frequency, offsetkHz, stepkHz);
    
        /* Calculate compensated centre frequency. */
        /* Tuners have only a fixed frequency step size (stepkHz), */
        /* therefore we must query the tuner driver to get the actual */
        /* centre frequency set by the tuner. */
        compFreqkHz = (uint32_t)((int32_t)pDriver->pTuner->frequency + offsetkHz) ;
        stepkHz = (stepkHz + 1)/2 ;

        /* Check to see if centre frequency compensation needed. */
        if ((uint32_t) abs (offsetkHz) > stepkHz) {
            /* Compensate, re-tune. */
            printf ("Re-tuning to compensate offset. New centre=%lukHz.\r\n", compFreqkHz);
            result = dvb_cxd2820_BlindTune (pDriver, pResult->system, compFreqkHz, pResult->scanParams->bwMHz);
            if (result != SONY_DVB_OK) {
                printf ("Error: Unable to blind tune to %s signal at %lukHz. (status=%d, %s)\r\n", FormatSystem (pResult->system), compFreqkHz, result, FormatResult(result));
                return ;
            }
        }

        /* Wait for TS lock. */
        result = dvb_cxd2820_WaitTSLock (pDriver);        
        if (result != SONY_DVB_OK) {
            /* TS not locked! */
            printf ("Error: Unable to achieve TS lock to %s signal at %lukHz. (status=%d, %s)\r\n", FormatSystem (pResult->system), pDriver->pTuner->frequency, result, FormatResult(result));
            return ;
        }
        
        printf ("TS locked to %s signal at %lukHz. \r\n", FormatSystem (pResult->system), pDriver->pTuner->frequency);
    }
}

int main (int argc, char* argv[]) {
    
    static const uint32_t startFreqkHz = 626000 ;
    static const uint32_t endFreqkHz = 706000 ;

    sony_dvb_result_t result = SONY_DVB_OK ;
    sony_dvb_cxd2820_t cxd2820 ;
    sony_dvb_demod_t demod  ;
    sony_dvb_tuner_t tuner ;
    sony_dvb_i2c_t demodI2C ;
    sony_dvb_i2c_t tunerI2C ;
    sony_dvb_i2c_feusb_t feusb ;
    sony_dvb_scan_params_t scanParams ;

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
     Scan - DVB-T/T2 multiple standard scan.
    ------------------------------------------------------------------------------*/
  
    scanParams.system = SONY_DVB_SYSTEM_DVBT ;  /* First attempted system is DVB-T. */
    scanParams.startFrequencykHz = startFreqkHz ;
    scanParams.endFrequencykHz = endFreqkHz ;
    scanParams.stepFrequencykHz = 8000 ;        /* 8MHz step. */
    scanParams.bwMHz = 8 ;                      /* 8MHz BW. */
    scanParams.multiple = 1 ;                   /* Search for DVB-T2 also. */
    result = dvb_cxd2820_Scan (&cxd2820, &scanParams, example_scan_callback);
    if (result != SONY_DVB_OK) {
        printf ("Error: Unable to scan. (status=%d, %s)\r\n", result, FormatResult(result));
        return -1 ;
    }

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

static const char* FormatSystem (sony_dvb_system_t sys)
{
    char* pName = "Unknown" ;
    switch (sys) 
    {
    case SONY_DVB_SYSTEM_DVBC:
        pName = "DVB-C" ;
        break ;
    case SONY_DVB_SYSTEM_DVBT:
        pName = "DVB-T" ;
        break ;
    case SONY_DVB_SYSTEM_DVBT2:
        pName = "DVB-T2" ;
        break ;
    case SONY_DVB_SYSTEM_UNKNOWN:
    default: break ;
    }
    return pName ;
}
