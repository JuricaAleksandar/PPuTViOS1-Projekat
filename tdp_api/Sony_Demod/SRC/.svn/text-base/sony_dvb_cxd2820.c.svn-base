/*------------------------------------------------------------------------------

 <dev:header>
    Copyright(c) 2009 Sony Corporation.

    $Revision: 2014 $
    $Author: sking $

</dev:header>

------------------------------------------------------------------------------*/

#include <sony_dvb_cxd2820.h>
#include <sony_dvb_demod_monitorT2.h>
#include "sony_dvb_stdlib.h"


/*------------------------------------------------------------------------------
  Defines
------------------------------------------------------------------------------*/
#ifndef CXD2820_TUNE_POLL_INTERVAL
#define CXD2820_TUNE_POLL_INTERVAL				10
#endif

/*------------------------------------------------------------------------------
  Static functions
------------------------------------------------------------------------------*/
static sony_dvb_result_t DoDemodTune (sony_dvb_cxd2820_t * pDriver, 
                                      sony_dvb_system_t system,
                                      uint8_t bwMHz);
static sony_dvb_result_t DoScan (sony_dvb_cxd2820_t * pDriver,
                                 sony_dvb_scan_params_t * pScanParams, 
                                 sony_dvb_scan_callback_t callBack) ;
static sony_dvb_result_t EnableBlindAcquisition (sony_dvb_cxd2820_t * pDriver,
                                                 sony_dvb_system_t system);

/**
 @brief DVB-T2 DVB-T2 PP2 Long Echo demodulator lock sequence.
*/
static sony_dvb_result_t DvbT2LongEchoSeqDemodLock (sony_dvb_cxd2820_t* pDriver);

/**
 @brief DVB-T2 DVB-T2 PP2 Long Echo Wait TS lock sequence.
*/
static sony_dvb_result_t DvbT2LongEchoSeqWaitTsLock (sony_dvb_cxd2820_t* pDriver);

/*------------------------------------------------------------------------------
  Implementation of public functions
------------------------------------------------------------------------------*/

sony_dvb_result_t dvb_cxd2820_Create (uint8_t dvbtI2CAddress,
                                      uint8_t dvbcI2CAddress,
                                      uint8_t dvbt2I2CAddress,
                                      sony_dvb_i2c_t * pDemodI2c,
                                      sony_dvb_demod_t * pDemod, 
                                      sony_dvb_tuner_t * pTuner, 
                                      sony_dvb_cxd2820_t * pDriver)
{

    sony_dvb_result_t result = SONY_DVB_OK;

    SONY_DVB_TRACE_ENTER ("dvb_cxd2820_Create");

    if ((!pDemod)  || (!pDemodI2c) || (!pDriver) || (!pTuner)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }
    
    sony_dvb_atomic_set(&(pDriver->cancel), 0);
    pDriver->pDemod = pDemod;
    sony_dvb_memset (pDriver->pDemod, 0, sizeof (sony_dvb_demod_t));
    pDriver->pDemod->i2cAddressT = dvbtI2CAddress;
    pDriver->pDemod->i2cAddressC = dvbcI2CAddress;
    pDriver->pDemod->i2cAddressT2 = dvbt2I2CAddress;
    pDriver->pDemod->pI2c = pDemodI2c;
    pDriver->pDemod->state = SONY_DVB_DEMOD_STATE_UNKNOWN ;
    pDriver->pTuner = pTuner;

    /* Default: Enabled T2 auto spectrum detect. */
    pDriver->t2AutoSpectrum = 1 ;

    SONY_DVB_TRACE_RETURN (result);
}

sony_dvb_result_t dvb_cxd2820_Initialize (sony_dvb_cxd2820_t * pDriver)
{
    sony_dvb_result_t result = SONY_DVB_OK;

    SONY_DVB_TRACE_ENTER ("dvb_cxd2820_Initialize");

    if ((!pDriver) || (!pDriver->pDemod)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    result = dvb_demod_Initialize (pDriver->pDemod);
    if (result == SONY_DVB_OK) {
        /* Initialize the tuner. */
#ifdef NuTune_Tuner
        result = pDriver->pTuner->Initialize (pDriver->pTuner);
#endif
    }

    SONY_DVB_TRACE_RETURN (result);
}

sony_dvb_result_t dvb_cxd2820_Finalize (sony_dvb_cxd2820_t * pDriver)
{
    sony_dvb_result_t result = SONY_DVB_OK;

    SONY_DVB_TRACE_ENTER ("dvb_cxd2820_Finalize");

    if ((!pDriver) || (!pDriver->pTuner)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }
#ifdef NuTune_Tuner
    result = pDriver->pTuner->Finalize (pDriver->pTuner);
#endif
    if (result == SONY_DVB_OK) {
        result = dvb_demod_Finalize (pDriver->pDemod);
    }

    SONY_DVB_TRACE_RETURN (result);
}

sony_dvb_result_t dvb_cxd2820_Sleep (sony_dvb_cxd2820_t * pDriver)
{
    sony_dvb_result_t result = SONY_DVB_OK;

    SONY_DVB_TRACE_ENTER ("dvb_cxd2820_Sleep");

    if ((!pDriver) || (!pDriver->pTuner)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }
#ifdef NuTune_Tuner
    result = pDriver->pTuner->Sleep (pDriver->pTuner);
#endif
    if (result == SONY_DVB_OK) {
        result = dvb_demod_Sleep (pDriver->pDemod);
    }

    return result;
}

static sony_dvb_result_t t2_WaitDemodLock (sony_dvb_cxd2820_t * pDriver)
{
    sony_dvb_result_t result = SONY_DVB_OK;
    sony_dvb_demod_lock_result_t lock = DEMOD_LOCK_RESULT_NOTDETECT;
    uint16_t index = 0;
    uint8_t specTrial = 0 ;
    uint8_t cont = 0 ;
    sony_dvb_demod_spectrum_sense_t sense = DVB_DEMOD_SPECTRUM_NORMAL ;

    SONY_DVB_TRACE_ENTER ("t2_WaitDemodLock");

    if ((!pDriver) || (!pDriver->pDemod)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* DVB-T2 only. */
    if (pDriver->pDemod->system != SONY_DVB_SYSTEM_DVBT2) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }
    
    do {
        cont = 0 ;
        /* Begin index = 1, so that time waited == index * CXD2820_TUNE_POLL_INTERVAL */
        for (index = 1; index <= (DVB_DEMOD_DVBT2_WAIT_LOCK / CXD2820_TUNE_POLL_INTERVAL); index++) {

            SONY_DVB_SLEEP (CXD2820_TUNE_POLL_INTERVAL);

            /* Check P1 detected after 300ms. */
            if (index == (DVB_DEMOD_DVBT2_P1_WAIT / CXD2820_TUNE_POLL_INTERVAL)) {
                
                /* Check Early P1 no detected. */
                result = dvb_demod_t2_CheckP1Lock (pDriver->pDemod, &lock);
                if (result != SONY_DVB_OK) {
                    SONY_DVB_TRACE_RETURN (result);
                }

                /* P1 not detected! */
                if (lock != DEMOD_LOCK_RESULT_LOCKED) {
                    if ((pDriver->t2AutoSpectrum) && (specTrial == 0)) {
                        /* Flip expected spectrum sense. */
                        result = dvb_demod_monitorT2_SpectrumSense (pDriver->pDemod, &sense);
                        if (result != SONY_DVB_OK) {
                            SONY_DVB_TRACE_RETURN (result);
                        }

                        result = dvb_demod_t2_SetSpectrumSense (pDriver->pDemod, sense == DVB_DEMOD_SPECTRUM_NORMAL ? DVB_DEMOD_SPECTRUM_INV : DVB_DEMOD_SPECTRUM_NORMAL );
                        if (result != SONY_DVB_OK) {
                            SONY_DVB_TRACE_RETURN (result);
                        }
                        specTrial++ ;
                        cont = 1 ;

                        /* Restart demodulator acquisition. */
                        result = dvb_demod_SoftReset (pDriver->pDemod);
                        if (result != SONY_DVB_OK) {
                            SONY_DVB_TRACE_RETURN (result);
                        }

                        /* Restart this waiting loop. Enters into do { } while() loop. */
                        break ;
                    } 
                    else if ((pDriver->t2AutoSpectrum) && (specTrial != 0)) {
                        /* Restore previous sense. */
                        result = dvb_demod_t2_SetSpectrumSense (pDriver->pDemod, sense);
                        if (result != SONY_DVB_OK) {
                            SONY_DVB_TRACE_RETURN (result);
                        }
                        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_UNLOCK);
                    }
                    else {
                        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_UNLOCK);
                    }
                }
            }

            /* Check for T2 P1 after 600ms. */
            if (index == (DVB_DEMOD_DVBT2_T2_P1_WAIT / CXD2820_TUNE_POLL_INTERVAL)) {
                
                /* Check Early P1 no detected. */
                result = dvb_demod_t2_CheckT2P1Lock (pDriver->pDemod, &lock);
                if (result != SONY_DVB_OK) {
                    SONY_DVB_TRACE_RETURN (result);
                }

                /* T2 P1 not detected! */
                if (lock != DEMOD_LOCK_RESULT_LOCKED) {
                    SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_UNLOCK);
                }
            }

            /* Check T2 OFDM core lock. */
            result = dvb_demod_CheckDemodLock (pDriver->pDemod, &lock);
            if (result != SONY_DVB_OK) {
                SONY_DVB_TRACE_RETURN (result);
            }

            switch (lock) {
            case DEMOD_LOCK_RESULT_LOCKED:
                SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
            case DEMOD_LOCK_RESULT_UNLOCKED:
                SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_UNLOCK);

            /* Intentional fall-through. */
            case DEMOD_LOCK_RESULT_NOTDETECT:
            default:
                break;              /* continue waiting... */
            }
        
            /* Check cancellation. */
            if (sony_dvb_atomic_read(&(pDriver->cancel)) != 0) {
                SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_CANCEL);
            }

            result = SONY_DVB_ERROR_TIMEOUT;
        }
    } while (cont);

    SONY_DVB_TRACE_RETURN (result);
}

static sony_dvb_result_t WaitDemodLock (sony_dvb_cxd2820_t * pDriver)
{
    sony_dvb_result_t result = SONY_DVB_OK;
    sony_dvb_demod_lock_result_t lock = DEMOD_LOCK_RESULT_NOTDETECT;
    uint16_t index = 0;
    uint16_t timeout = 0;

    SONY_DVB_TRACE_ENTER ("WaitDemodLock");

    switch (pDriver->pDemod->system) {
    
    case SONY_DVB_SYSTEM_DVBT2:
        /* DVB-T2: Check OFDM core lock. */
        if (pDriver->pDemod->t2LongEchoPP2) {
            result = DvbT2LongEchoSeqDemodLock (pDriver);
        }
        else {
            result = t2_WaitDemodLock (pDriver);
        }
        SONY_DVB_TRACE_RETURN (result);
    
    case SONY_DVB_SYSTEM_DVBC:
        /* DVB-C: Wait TS lock timeout. */
        timeout = DVB_DEMOD_DVBC_WAIT_TS_LOCK;
        break;

    case SONY_DVB_SYSTEM_DVBT:
        /* DVB-T: Wait for early unlock valid. */
        SONY_DVB_SLEEP (DVB_DEMOD_DVBT_EARLY_UNLOCK_WAIT);
        timeout = DVB_DEMOD_DVBT_WAIT_LOCK;
        break ;

    /* Intentional fall-through. */
    case SONY_DVB_SYSTEM_UNKNOWN:
    default:
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_OTHER);
    }

    /* Wait for demod lock */
    for (index = 0; index < (timeout / CXD2820_TUNE_POLL_INTERVAL); index++) {

        SONY_DVB_SLEEP (CXD2820_TUNE_POLL_INTERVAL);

        result = dvb_demod_CheckDemodLock (pDriver->pDemod, &lock);
        if (result != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (result);
        }

        switch (lock) {
        case DEMOD_LOCK_RESULT_LOCKED:
            SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
        case DEMOD_LOCK_RESULT_UNLOCKED:
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_UNLOCK);

        /* Intentional fall-through. */
        case DEMOD_LOCK_RESULT_NOTDETECT:
        default:
            break;              /* continue waiting... */
        }
    
        /* Check cancellation. */
        if (sony_dvb_atomic_read(&(pDriver->cancel)) != 0) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_CANCEL);
        }

        result = SONY_DVB_ERROR_TIMEOUT;
    }

    SONY_DVB_TRACE_RETURN (result);
}

sony_dvb_result_t dvb_cxd2820_BlindTune (sony_dvb_cxd2820_t * pDriver,
                                         sony_dvb_system_t system,
                                         uint32_t centreFreqkHz, 
                                         uint8_t bwMHz)
{
    sony_dvb_result_t result = SONY_DVB_OK;

    SONY_DVB_TRACE_ENTER ("dvb_cxd2820_BlindTune");

    /* Argument verification. */
    if ((!pDriver) || (!pDriver->pDemod) || (!pDriver->pTuner)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* Device check. */
    if ((pDriver->pDemod->chipId == SONY_DVB_CXD2817) && 
        (system == SONY_DVB_SYSTEM_DVBT2)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_NOSUPPORT);
    }

    /* Perform RF tuning. */
#ifdef NuTune_Tuner
    result = pDriver->pTuner->Tune (pDriver->pTuner, centreFreqkHz, system, bwMHz);
#endif
    if (result != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (result);
    }

    /* Ensure demod is configured correctly for blind acquisition. */
    result = EnableBlindAcquisition (pDriver, system);    
    if (result != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (result);
    }
    
    /* Perform demodulator tuning operation. */
    result = DoDemodTune (pDriver, system, bwMHz);
    
    /* Clear cancellation flag. */
    sony_dvb_atomic_set(&(pDriver->cancel), 0);

    SONY_DVB_TRACE_RETURN (result);
}

sony_dvb_result_t dvb_cxd2820_Tune (sony_dvb_cxd2820_t * pDriver, sony_dvb_tune_params_t * pTuneParams)
{
    sony_dvb_result_t result = SONY_DVB_OK;
    uint8_t usePresets = 0 ;

    SONY_DVB_TRACE_ENTER ("dvb_cxd2820_Tune");

    /* Argument verification. */
    if ((!pDriver) || (!pDriver->pTuner) || (!pTuneParams) || (!pDriver->pDemod)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }
    
    /* Device check. */
    if ((pDriver->pDemod->chipId == SONY_DVB_CXD2817) && (pTuneParams->system == SONY_DVB_SYSTEM_DVBT2)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_NOSUPPORT);
    }

    /* Perform RF tuning. */
#ifdef NuTune_Tuner
    result = pDriver->pTuner->Tune (pDriver->pTuner, pTuneParams->centreFreqkHz, pTuneParams->system, pTuneParams->bwMHz);
#endif
    if (result != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (result);
    }

    /* Setup any required preset information. */
    switch (pTuneParams->system) {
    case SONY_DVB_SYSTEM_DVBC:
        break;
    case SONY_DVB_SYSTEM_DVBT:
        usePresets = pTuneParams->tParams.usePresets ;
        /* Check DVB-T Preset tuning parameters. */
        if (usePresets) {
            result = dvb_demod_SetConfig (pDriver->pDemod, DEMOD_CONFIG_DVBT_FORCE_MODEGI, 1);
            if (result != SONY_DVB_OK) {
                SONY_DVB_TRACE_RETURN (result);
            }
            result = dvb_demod_SetConfig (pDriver->pDemod, DEMOD_CONFIG_DVBT_MODE, (int32_t) pTuneParams->tParams.mode);
            if (result != SONY_DVB_OK) {
                SONY_DVB_TRACE_RETURN (result);
            }
            result = dvb_demod_SetConfig (pDriver->pDemod, DEMOD_CONFIG_DVBT_GI, (int32_t) pTuneParams->tParams.gi);
        }
        else {
            result = dvb_demod_SetConfig (pDriver->pDemod, DEMOD_CONFIG_DVBT_FORCE_MODEGI, 0);
        }

        if (result != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (result);
        }

        /* Select appropriate HP/LP stream. */
        result = dvb_demod_SetConfig (pDriver->pDemod, DEMOD_CONFIG_DVBT_LPSELECT, (int32_t) pTuneParams->tParams.profile);

        break;

    case SONY_DVB_SYSTEM_DVBT2:

        pTuneParams->t2Params.tuneResult = 0x00;

        /* Configure for non-auto PLP selection. */
        result = dvb_demod_t2_SetPLPConfig (pDriver->pDemod, 0x01, pTuneParams->t2Params.dataPLPId);
        if (result != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (result);
        }

        break;

    /* Intentional fall through. */
    case SONY_DVB_SYSTEM_UNKNOWN:
    default:
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    if (result != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (result);
    }

    /* Wait for demodulator lock. */
    result = DoDemodTune (pDriver, pTuneParams->system, pTuneParams->bwMHz);
    if (result == SONY_DVB_OK) {

        /* Wait TS lock (non DVB-C modes) */
        if (pTuneParams->system != SONY_DVB_SYSTEM_DVBC) {
            result = dvb_cxd2820_WaitTSLock (pDriver);
        }

        /* Check the T2 portion */
        if (pTuneParams->system == SONY_DVB_SYSTEM_DVBT2) {
            sony_dvb_result_t localResult = SONY_DVB_OK;
            uint8_t data = 0;

            localResult = dvb_demod_monitorT2_DataPLPError (pDriver->pDemod, &data);
            if ((localResult == SONY_DVB_OK) && (data)) {
                pTuneParams->t2Params.tuneResult |= DVB_DEMOD_DVBT2_TUNE_RESULT_DATA_PLP_NOT_FOUND;
            }
        }
    }

    /* Clear cancellation flag. */
    sony_dvb_atomic_set(&(pDriver->cancel), 0);

    SONY_DVB_TRACE_RETURN (result);
}

sony_dvb_result_t dvb_cxd2820_WaitTSLock (sony_dvb_cxd2820_t * pDriver)
{
    sony_dvb_result_t result = SONY_DVB_OK;
    sony_dvb_demod_lock_result_t lock = DEMOD_LOCK_RESULT_NOTDETECT;
    uint16_t index = 0;
    uint16_t timeout = 0;

    SONY_DVB_TRACE_ENTER ("dvb_cxd2820_WaitTSLock");

    /* Argument verification. */
    if ((!pDriver) || (!pDriver->pDemod)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    switch (pDriver->pDemod->system) {
    case SONY_DVB_SYSTEM_DVBC:
        timeout = DVB_DEMOD_DVBC_WAIT_TS_LOCK;
        break;
    case SONY_DVB_SYSTEM_DVBT2:

        /* DVB-T2 Long Echo sequence check. */
        if (pDriver->pDemod->t2LongEchoPP2) {
            result = DvbT2LongEchoSeqWaitTsLock (pDriver);
            SONY_DVB_TRACE_RETURN(result);
        }
        timeout = DVB_DEMOD_DVBT2_WAIT_TS_LOCK;
        break;
    case SONY_DVB_SYSTEM_DVBT:
        timeout = DVB_DEMOD_DVBT_WAIT_TS_LOCK;
        break;

    /* Intentional fall-through. */
    case SONY_DVB_SYSTEM_UNKNOWN:
    default:
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* Wait for TS lock */
    for (index = 0; index < (timeout / CXD2820_TUNE_POLL_INTERVAL); index++) {

        result = dvb_demod_CheckTSLock (pDriver->pDemod, &lock);
        if (result != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (result);
        }

        if ((pDriver->pDemod->system == SONY_DVB_SYSTEM_DVBT2) && 
            (lock == DEMOD_LOCK_RESULT_LOCKED)) {
            /* DVB-T2 TS locked, attempt to optimise the Transport Stream output. */
            result = dvb_demod_t2_OptimiseTSOutput (pDriver->pDemod) ;
            if (result == SONY_DVB_ERROR_HW_STATE) {
                /* TS has gone out of lock, continue waiting. */
                lock = DEMOD_LOCK_RESULT_NOTDETECT ;
            }
            else if (result != SONY_DVB_OK) {
                /* Unexpected error. */
                SONY_DVB_TRACE_RETURN (result);
            }
        }

        switch (lock) {
        case DEMOD_LOCK_RESULT_LOCKED:
            SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
        case DEMOD_LOCK_RESULT_UNLOCKED:
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_UNLOCK);

        /* Intentional fall-through. */
        case DEMOD_LOCK_RESULT_NOTDETECT:
        default:
            break;              /* continue waiting... */
        }

        /* Check cancellation. */
        if (sony_dvb_atomic_read(&(pDriver->cancel)) != 0) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_CANCEL);
        }
        
        SONY_DVB_SLEEP (CXD2820_TUNE_POLL_INTERVAL);

        result = SONY_DVB_ERROR_TIMEOUT;
    }

    SONY_DVB_TRACE_RETURN (result);
}

sony_dvb_result_t dvb_cxd2820_Cancel (sony_dvb_cxd2820_t * pDriver)
{
    SONY_DVB_TRACE_ENTER ("dvb_cxd2820_Cancel");

    /* Argument verification. */
    if ((!pDriver) || (!pDriver->pDemod)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* Set the cancellation flag. */
    sony_dvb_atomic_set(&(pDriver->cancel), 1);

    SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
}

sony_dvb_result_t dvb_cxd2820_Scan (sony_dvb_cxd2820_t * pDriver,
                                    sony_dvb_scan_params_t * pScanParams, 
                                    sony_dvb_scan_callback_t callBack)
{
    sony_dvb_result_t result = SONY_DVB_OK ;
    sony_dvb_result_t r = SONY_DVB_OK ;     /* Used for return codes AFTER scan completed. */

    SONY_DVB_TRACE_ENTER ("dvb_cxd2820_Scan");

    /* Argument verification. */
    if ((!pDriver) || (!pDriver->pDemod) || (!pScanParams)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* Ensure the scan parameters are valid. */
    if (pScanParams->endFrequencykHz < pScanParams->startFrequencykHz) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }
    if (pScanParams->stepFrequencykHz == 0) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* Device check. */
    if (pDriver->pDemod->chipId == SONY_DVB_CXD2817) {
        if ( (pScanParams->system == SONY_DVB_SYSTEM_DVBT2) || 
            ((pScanParams->system  == SONY_DVB_SYSTEM_DVBT) && (pScanParams->multiple))) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_NOSUPPORT);
        }
    }

    /* Configure driver/device for scanning */
    switch (pScanParams->system)
    {
        /* For DVB-T2 scan, assume DVB-T is scanned also. */
        case SONY_DVB_SYSTEM_DVBT:
        case SONY_DVB_SYSTEM_DVBT2:
            result = dvb_demod_SetConfig(pDriver->pDemod, DEMOD_CONFIG_DVBT_SCANMODE, 1);
            if (result != SONY_DVB_OK) {
                SONY_DVB_TRACE_RETURN(result);
            }
            result = EnableBlindAcquisition (pDriver, SONY_DVB_SYSTEM_DVBT);
            if (result != SONY_DVB_OK) {
                SONY_DVB_TRACE_RETURN(result);
            }

            /* Device check. */
            if (pDriver->pDemod->chipId != SONY_DVB_CXD2817) {
                result = EnableBlindAcquisition (pDriver, SONY_DVB_SYSTEM_DVBT2);
            }
            break ;
        case SONY_DVB_SYSTEM_DVBC:
            result = dvb_demod_SetConfig(pDriver->pDemod, DEMOD_CONFIG_DVBC_SCANMODE, 1);
            break ;

        /* Intentional fall through */
        case SONY_DVB_SYSTEM_UNKNOWN:
        default:
            SONY_DVB_TRACE_RETURN(SONY_DVB_ERROR_ARG);
    }
    if (result != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN(result);
    }

    /* Perform scan. */
    result = DoScan (pDriver, pScanParams, callBack) ;

    /* Configure driver device for normal operation. */
    switch (pScanParams->system)
    {
        /* For DVB-T2 scan, assume DVB-T is scanned also. */
        case SONY_DVB_SYSTEM_DVBT:
        case SONY_DVB_SYSTEM_DVBT2:
            r = dvb_demod_SetConfig(pDriver->pDemod, DEMOD_CONFIG_DVBT_SCANMODE, 0) ;
            if (r != SONY_DVB_OK) {
                SONY_DVB_TRACE_RETURN(r);
            }
            break ;
        case SONY_DVB_SYSTEM_DVBC:
            r = dvb_demod_SetConfig(pDriver->pDemod, DEMOD_CONFIG_DVBC_SCANMODE, 0);
            if (r != SONY_DVB_OK) {
                SONY_DVB_TRACE_RETURN(r);
            }
            break ;

        /* Intentational fall through. */
        case SONY_DVB_SYSTEM_UNKNOWN:
        default:
            SONY_DVB_TRACE_RETURN(SONY_DVB_ERROR_ARG);
    }
    
    /* Clear cancellation flag. */
    sony_dvb_atomic_set(&(pDriver->cancel), 0);

    SONY_DVB_TRACE_RETURN (result);
}

static sony_dvb_result_t DoScan (sony_dvb_cxd2820_t * pDriver,
                                 sony_dvb_scan_params_t * pScanParams,
                                 sony_dvb_scan_callback_t callBack)
{
    sony_dvb_result_t result = SONY_DVB_OK;
    sony_dvb_scan_result_t scanResult ;
    sony_dvb_system_t systems[] = { SONY_DVB_SYSTEM_UNKNOWN, SONY_DVB_SYSTEM_UNKNOWN, SONY_DVB_SYSTEM_UNKNOWN };
    uint8_t i = 0 ;

    SONY_DVB_TRACE_ENTER ("DoScan");
    systems[0] = pScanParams->system ;

    /* Setup mixed system scanning, only applies to T/T2 scanning. */
    if (pScanParams->multiple != 0) {
        systems[1] = (pScanParams->system == SONY_DVB_SYSTEM_DVBT2) ? SONY_DVB_SYSTEM_DVBT : SONY_DVB_SYSTEM_DVBT2 ;
    }

    scanResult.centreFreqkHz = pScanParams->startFrequencykHz ;
    scanResult.scanParams = pScanParams ;

    while (scanResult.centreFreqkHz <= pScanParams->endFrequencykHz) {
        
        /* Perform RF tuning. */
        result = pDriver->pTuner->Tune (pDriver->pTuner, scanResult.centreFreqkHz, pScanParams->system, pScanParams->bwMHz);
        if (result != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (result);
        }

        /* Attempt blind tune (Set RF frequency). Handle mixed scanning. */
        i = 0 ;
        while (systems[i] != SONY_DVB_SYSTEM_UNKNOWN) {

            /* Get current system. */
            scanResult.system = systems[i++] ;

            /* Perform RF tuning. */
            result = pDriver->pTuner->Tune (pDriver->pTuner, scanResult.centreFreqkHz, scanResult.system, pScanParams->bwMHz);
            if (result != SONY_DVB_OK) {
                SONY_DVB_TRACE_RETURN (result);
            }

            scanResult.tuneResult = DoDemodTune (pDriver, scanResult.system, pScanParams->bwMHz);
            switch (scanResult.tuneResult) {
                /* Intentional fall-through. */
                case SONY_DVB_OK:
                case SONY_DVB_ERROR_UNLOCK:
                case SONY_DVB_ERROR_TIMEOUT:
                    /* Notify caller. */
                    if (callBack) {
                        callBack (pDriver, &scanResult);
                    }
                    break ;
                
                /* Intentional fall through. */
                case SONY_DVB_ERROR_ARG:
                case SONY_DVB_ERROR_I2C:
                case SONY_DVB_ERROR_SW_STATE:
                case SONY_DVB_ERROR_HW_STATE:
                case SONY_DVB_ERROR_RANGE:
                case SONY_DVB_ERROR_NOSUPPORT:
                case SONY_DVB_ERROR_CANCEL:
                case SONY_DVB_ERROR_OTHER: 
                default:
                    /* Serious error occurred -> cancel operation. */
                    SONY_DVB_TRACE_RETURN (scanResult.tuneResult);
            }
        }
        scanResult.centreFreqkHz += pScanParams->stepFrequencykHz ;
        
        /* Check cancellation. */
        if (sony_dvb_atomic_read(&(pDriver->cancel)) != 0) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_CANCEL);
        }
    }
    
    SONY_DVB_TRACE_RETURN (result);
}

static sony_dvb_result_t DoDemodTune (sony_dvb_cxd2820_t * pDriver,
                                      sony_dvb_system_t system,
                                      uint8_t bwMHz)
{
    sony_dvb_result_t result = SONY_DVB_OK ;
    SONY_DVB_TRACE_ENTER ("DoDemodTune");

    /* Enable acquisition on the demodulator. */
    result = dvb_demod_Tune (pDriver->pDemod, system, bwMHz);
    if (result != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (result);
    }

    /* Wait for lock */
    result = WaitDemodLock (pDriver);
    SONY_DVB_TRACE_RETURN (result);
}

sony_dvb_result_t dvb_cxd2820_SetT2AutoSpectrumDetection (sony_dvb_cxd2820_t* pDriver,
                                                          uint8_t enable)
{
    SONY_DVB_TRACE_ENTER ("dvb_cxd2820_SetT2AutoSpectrumDetection");
    if ((!pDriver)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }
    pDriver->t2AutoSpectrum = enable ;
    SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
}

static sony_dvb_result_t EnableBlindAcquisition (sony_dvb_cxd2820_t * pDriver,
                                                 sony_dvb_system_t system)
{
    sony_dvb_result_t result = SONY_DVB_OK ;

    SONY_DVB_TRACE_ENTER ("EnableBlindAcquisition");
    if ((!pDriver) || (!pDriver->pDemod)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }
    
    /* Configure the demodulator for blind acquisition in specified system. */
    switch (system) {
    case SONY_DVB_SYSTEM_DVBC:
        break;
    case SONY_DVB_SYSTEM_DVBT:

        /* Disable Forced Mode/GI */
        result = dvb_demod_SetConfig (pDriver->pDemod, DEMOD_CONFIG_DVBT_FORCE_MODEGI, 0);
        if (result != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (result);
        }

        /* Force HP Selection. */
        result = dvb_demod_SetConfig (pDriver->pDemod, DEMOD_CONFIG_DVBT_LPSELECT, (int32_t)DVBT_PROFILE_HP);
        break;
    case SONY_DVB_SYSTEM_DVBT2:

        /* Enable Auto PLP selection. */
        result = dvb_demod_t2_SetPLPConfig (pDriver->pDemod, 0x00, 0x00);
        if (result != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (result);
        }
        break;

    /* Intentional fall through. */
    case SONY_DVB_SYSTEM_UNKNOWN:
    default:
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    SONY_DVB_TRACE_RETURN (result);
}

static void DmdLockSeqSleep (sony_dvb_le_seq_t* pSeq, uint32_t ms)
{
    /* Unused. */
    pSeq = pSeq ;

    SONY_DVB_SLEEP(ms);
}

static sony_dvb_result_t DvbT2LongEchoSeqDemodLock (sony_dvb_cxd2820_t* pDriver)
{
    sony_dvb_result_t result = SONY_DVB_OK ;

    /* Setup sequencer with start values. */
    sony_dvb_le_seq_t seq = { 
        NULL,                                   /* pDemod */
        SONY_DVB_LE_SEQ_INIT,               	/* state */
        0,                                      /* autoSpectrumDetect */
        CXD2820_TUNE_POLL_INTERVAL,             /* pollInterval */
        0,                                      /* waitTime */
        0,                                      /* totalWaitTime */
        0,                                      /* spectTrial */
        0,                                      /* gThetaPOfst */
        0,                                      /* gThetaMOfst */
        0,                                      /* gThetaNoOfst */
        0,                                      /* unlockDetDone */
        0,                                      /* crlOpenTrial */
        0,                                      /* complete */
        DmdLockSeqSleep                         /* Sleep */
    };

    SONY_DVB_TRACE_ENTER ("DvbT2LongEchoSeqDemodLock");

    if (!pDriver) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* Assign demod into sequencer. */
    seq.pDemod = pDriver->pDemod ;
    seq.autoSpectrumDetect = pDriver->t2AutoSpectrum ;

    /* Run sequencer. */
    while ((!seq.complete) && (result == SONY_DVB_OK)) {

        /* Turn crank on sequencer. */
        result = dvb_demod_le_SeqWaitDemodLock(&seq);

        /* Sanity check on overall wait time. */
        if ((result == SONY_DVB_OK) && (seq.totalWaitTime > 30*1000)) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_TIMEOUT);
        }

        /* Check cancellation. */
        if (sony_dvb_atomic_read(&(pDriver->cancel)) != 0) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_CANCEL);
        }
    }

    SONY_DVB_TRACE_RETURN (result);
}

static void TsLockSeqSleep (sony_dvb_le_seq_ts_lock_t* pSeq, uint32_t ms)
{    
    /* Unused. */
    pSeq = pSeq ;

    SONY_DVB_SLEEP(ms);
}

static sony_dvb_result_t DvbT2LongEchoSeqWaitTsLock (sony_dvb_cxd2820_t* pDriver)
{
    sony_dvb_result_t result = SONY_DVB_OK ;

    /* Setup sequencer with start values. */
    sony_dvb_le_seq_ts_lock_t seq = { 
        NULL,                                   /* pDemod. */
        CXD2820_TUNE_POLL_INTERVAL,             /* pollInterval */
        0,                                      /* waitTime */
        0,                                      /* complete */
        TsLockSeqSleep                          /* Sleep */
    };

    SONY_DVB_TRACE_ENTER ("DvbT2LongEchoSeqWaitTsLock");

    if (!pDriver) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* Assign demod into sequencer. */
    seq.pDemod = pDriver->pDemod ;

    /* Run sequencer. */
    while ((!seq.complete) && (result == SONY_DVB_OK)) {

        /* Turn crank on sequencer. */
        result = dvb_demod_le_SeqWaitTsLock(&seq);

        /* Sanity check on overall wait time. */
        if ((result == SONY_DVB_OK) && (seq.waitTime > 20*1000)) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_TIMEOUT);
        }

        /* Check cancellation. */
        if (sony_dvb_atomic_read(&(pDriver->cancel)) != 0) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_CANCEL);
        }
    }

    SONY_DVB_TRACE_RETURN (result);
}
