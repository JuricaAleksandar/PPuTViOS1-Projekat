/*------------------------------------------------------------------------------

 <dev:header>
    Copyright(c) 2010 Sony Corporation.

    $Revision$
    $Author$

</dev:header>

------------------------------------------------------------------------------*/
/**
 @file    sony_ascot2s.c

      This file implements the Sony ASCOT2S DVB tuner driver.
      Based on CXD2822ER_spec_application-note_J_Detail_1.6.0_draft.pdf.
*/
/*----------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
 Includes
------------------------------------------------------------------------------*/

#include <sony_dvb.h>
#include <sony_dvb_i2c.h>
#include "sony_ascot2s.h"

/*------------------------------------------------------------------------------
 Defines
------------------------------------------------------------------------------*/
#define POLL_INTERVAL_MS            10      /**< Polling interval (in ms). */
#define XTAL_41MHZ                  41      /**< 41MHz XTAL. */
#define XTAL_4MHZ                   4       /**< 4MHz XTAL. */
#define ASCOT2S_XPON_WAIT_MS        90      /**< Wait for CPU idle in X_pon. 80ms + margin. */
#define ASCOT2S_XTUNE_WAIT_MS       20      /**< Wait for CPU idle in X_tune. 10ms + margin. */
#define ASCOT2S_RFAGC_EXTERN_SETTLING_MS   250  /**< RFAGC settling time after external RFAGC reset release. */
#define ASCOT2S_RFAGC_INTERN_SETTLING_MS   250  /**< RFAGC settling time after internal RFAGC reset release. */
#define ASCOT2S_MIN_FKHZ            42000   /**< 42MHz min RF frequency. */
#define ASCOT2S_MAX_FKHZ            880000  /**< 880MHz max RF frequency. */

/* Sony ASCOT2S register aliases. */
#define ASCOT2S_REG_REF_R           0x06
#define ASCOT2S_REG_REF_XOSC_SEL    0x07 
#define ASCOT2S_REG_KBW             0x08 
#define ASCOT2S_REG_RF_GAIN         0x0D
#define ASCOT2S_REG_CPU_STT         0x1A    /**< */
#define ASCOT2S_REG_CPU_ERR         0x1B    /**< */

/*------------------------------------------------------------------------------
 Statics
------------------------------------------------------------------------------*/

/**
 @brief Implements X_pon sequence.

        @note: Assumes that at least 50us has passed since 
        reset line is released.

 @param pTuner Tuner instance.
 
 @return SONY_DVB_OK if successful.
*/
static sony_dvb_result_t XPOn (sony_dvb_tuner_t * pTuner);

/**
 @brief Implements A_init (Analog tuning initialisation) sequence.

 @param pTuner Tuner instance.
 
 @return SONY_DVB_OK if successful.
*/
static sony_dvb_result_t AInit (sony_dvb_tuner_t * pTuner);

/**
 @brief Implements D_init (Digital tuning initialisation) sequence.

 @param pTuner Tuner instance.
 
 @return SONY_DVB_OK if successful.
*/
static sony_dvb_result_t DInit (sony_dvb_tuner_t * pTuner);

/**
 @brief Implements X_fin sequence.

 @param pTuner Tuner instance.
 
 @return SONY_DVB_OK if successful.
*/
static sony_dvb_result_t XFin (sony_dvb_tuner_t * pTuner);

/**
 @brief Implements X_tune sequence.

 @param pTuner Tuner instance.
 
 @return SONY_DVB_OK if successful.
*/
static sony_dvb_result_t XTune (sony_dvb_tuner_t * pTuner, 
                                uint32_t frequency, 
                                sony_dvb_system_t system,
                                uint8_t bandwidth);

static sony_dvb_result_t WaitCpuIdle (sony_dvb_tuner_t* pTuner,
                                      uint8_t timeoutMs);

/*------------------------------------------------------------------------------
 Functions
------------------------------------------------------------------------------*/

sony_dvb_result_t sony_ascot2s_Create (uint8_t i2cAddress,
                                       uint32_t xtalFreq, 
                                       sony_dvb_i2c_t * pI2c,
                                       uint32_t configFlags,
                                       void *user, 
                                       sony_dvb_tuner_t * pTuner)
{
/*
    sony_dvb_result_t result = SONY_DVB_OK;
    SONY_DVB_TRACE_ENTER ("sony_ascot2s_Create");

    if ((!pTuner) || (!pI2c)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    if ((xtalFreq != XTAL_41MHZ) && (xtalFreq != XTAL_4MHZ)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_NOSUPPORT);
    }

    pTuner->Initialize = sony_ascot2s_Initialize;
    pTuner->Finalize = sony_ascot2s_Finalize;
    pTuner->Sleep = sony_ascot2s_Sleep;
    pTuner->Tune = sony_ascot2s_Tune;
    pTuner->system = SONY_DVB_SYSTEM_UNKNOWN;
    pTuner->frequency = 0;
    pTuner->bandWidth = 0;
    pTuner->i2cAddress = i2cAddress;
    pTuner->xtalFreq = xtalFreq;
    pTuner->pI2c = pI2c;
    pTuner->flags = configFlags ;
    pTuner->user = user;
*/
    sony_dvb_result_t result = SONY_DVB_OK;
    if ((!pTuner) || (!pI2c)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    //pTuner->Initialize = ;
    //pTuner->Finalize =  ;
    //pTuner->Sleep =  ;
    //pTuner->Tune =  ;
    pTuner->system = SONY_DVB_SYSTEM_UNKNOWN;
    pTuner->frequency = 0;
    pTuner->bandWidth = 0;
    pTuner->i2cAddress = i2cAddress;
    pTuner->xtalFreq = xtalFreq;
    pTuner->pI2c = pI2c;
    pTuner->flags = configFlags ;
    pTuner->user = user;


	
    SONY_DVB_TRACE_RETURN (result);
}

/*------------------------------------------------------------------------------
 Initialise the tuner. 
------------------------------------------------------------------------------*/
sony_dvb_result_t sony_ascot2s_Initialize (sony_dvb_tuner_t * pTuner)
{
    sony_dvb_result_t result = SONY_DVB_OK;
    SONY_DVB_TRACE_ENTER ("sony_ascot2s_Initialize");
    result = XPOn (pTuner);
    /* Device is in "Power Save" state. */
    pTuner->system = SONY_DVB_SYSTEM_UNKNOWN;
    pTuner->frequency = 0;
    pTuner->bandWidth = 0;
    SONY_DVB_TRACE_RETURN (result);
}

/*------------------------------------------------------------------------------
 Set tuner into Power Save mode.
------------------------------------------------------------------------------*/
sony_dvb_result_t sony_ascot2s_Finalize (sony_dvb_tuner_t * pTuner)
{
    sony_dvb_result_t result = SONY_DVB_OK;
    SONY_DVB_TRACE_ENTER ("sony_ascot2s_Finalize");
    result = XFin (pTuner);
    /* Device is in "Power Save" state. */
    pTuner->system = SONY_DVB_SYSTEM_UNKNOWN;
    pTuner->frequency = 0;
    pTuner->bandWidth = 0;
    SONY_DVB_TRACE_RETURN (result);
}

/*------------------------------------------------------------------------------
 Setup the channel bandwidth.
 Tune to the specified frequency.
 Wait for lock.

------------------------------------------------------------------------------*/
sony_dvb_result_t sony_ascot2s_Tune (sony_dvb_tuner_t * pTuner,
                                     uint32_t frequency,
                                     sony_dvb_system_t system,
                                     uint8_t bandWidth)
{
    
    sony_dvb_result_t result = SONY_DVB_OK ;
    SONY_DVB_TRACE_ENTER ("sony_ascot2s_Tune");

    if (!pTuner) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    switch (system)
    {
        case SONY_DVB_SYSTEM_DVBC:
            if (pTuner->system != system) {
                /* Analog system initialisation. */
                result = AInit (pTuner);
            }
            break ;

        /* Intentional fall-through. */
        case SONY_DVB_SYSTEM_DVBT:
        case SONY_DVB_SYSTEM_DVBT2:
            if ((pTuner->system == SONY_DVB_SYSTEM_UNKNOWN) ||
                (pTuner->system == SONY_DVB_SYSTEM_DVBC)) {
                /* Digital system initialisation. */
                result = DInit (pTuner);
            }
            break ;

        /* Intentional fall-through. */
        case SONY_DVB_SYSTEM_UNKNOWN:
        default:
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    if (result != SONY_DVB_OK) {
        pTuner->system = SONY_DVB_SYSTEM_UNKNOWN;
        pTuner->frequency = 0;
        pTuner->bandWidth = 0;
        SONY_DVB_TRACE_RETURN (result);
    }

    /* Tune: X_tune. */
    result = XTune (pTuner, frequency, system, bandWidth);
    if (result == SONY_DVB_OK) {
        /* Assign current values. */
        pTuner->system = system;
        pTuner->frequency = frequency;
        pTuner->bandWidth = bandWidth;
    }

    SONY_DVB_TRACE_RETURN (result);
}

/*------------------------------------------------------------------------------
 Sleep the tuner module.
------------------------------------------------------------------------------*/
sony_dvb_result_t sony_ascot2s_Sleep (sony_dvb_tuner_t * pTuner)
{
    sony_dvb_result_t result = SONY_DVB_OK ;
    SONY_DVB_TRACE_ENTER ("sony_ascot2s_Sleep");
    result = XFin (pTuner);
    /* Device is in "Power Save" state. */
    pTuner->system = SONY_DVB_SYSTEM_UNKNOWN;
    pTuner->frequency = 0;
    pTuner->bandWidth = 0;
    SONY_DVB_TRACE_RETURN (result);
}

/*------------------------------------------------------------------------------
 RF level monitor.
------------------------------------------------------------------------------*/
sony_dvb_result_t sony_ascot2s_monitor_RFLevel (sony_dvb_tuner_t * pTuner, 
                                                uint32_t ifAgc,
                                                uint8_t rfAinValid,
                                                uint32_t rfAin,
                                                int32_t* pRFLeveldB)
{
    SONY_DVB_TRACE_ENTER ("sony_ascot2s_monitor_RFLevel");
    
    if ((!pTuner) || (!pRFLeveldB)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* Protect against overflow. IFAGC is unsigned 12-bit. */
    if (ifAgc > 0xFFF) {        
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* 
     RFAIN not enabled:
     ==================
     1st order:
     RF (dB) = 0.0374 * IFAGC - 101.52
      
     2nd order:
     RF (dB) = -5.139E-6 * IFAGC^2 + 0.05014969962 * IFAGC - 106.59564776988
    */
    if (!rfAinValid)
    {
        *pRFLeveldB = -51 * ifAgc * ifAgc + 501497 * ifAgc - 1065956478 ;
        *pRFLeveldB = (*pRFLeveldB < 0) ? *pRFLeveldB - 5000 : *pRFLeveldB + 5000 ;
        *pRFLeveldB /= 10000 ;
    }
    else {
        /* unused. */
        rfAin = rfAin ;
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_NOSUPPORT);
    }

    SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
}

/*------------------------------------------------------------------------------
 X_pon
------------------------------------------------------------------------------*/
static sony_dvb_result_t XPOn (sony_dvb_tuner_t * pTuner)
{
    sony_dvb_result_t result = SONY_DVB_OK ;
    SONY_DVB_TRACE_ENTER ("XPOn");

    if ((!pTuner) || (!pTuner->pI2c)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* IF signal output suppression. Power save setting on Analog block. */
    if (pTuner->pI2c->WriteOneRegister (pTuner->pI2c, pTuner->i2cAddress, 0x14, 0x00) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /* XTAL configuration. */
    if (pTuner->pI2c->WriteOneRegister (pTuner->pI2c, pTuner->i2cAddress, 0x01, (uint8_t)pTuner->xtalFreq) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /* 41MHz special setting (external input or XTAL). */
    if (pTuner->xtalFreq == XTAL_41MHZ) {
        if (pTuner->pI2c->WriteOneRegister (pTuner->pI2c, pTuner->i2cAddress, 0x44, 0x07) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
    }

    /* Boot CPU. */
    if (pTuner->pI2c->WriteOneRegister (pTuner->pI2c, pTuner->i2cAddress, 0x04, 0x40) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }
    
    if (pTuner->flags & SONY_ASCOT2S_CONFIG_RFAGC_EXTERNAL) {
        /* Using external RFAGC (Overload) circuit. */
        if (pTuner->pI2c->WriteOneRegister (pTuner->pI2c, pTuner->i2cAddress, 0x21, 0x1C) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
    }
    else {
        /* Using internal RFAGC (Overload) circuit. */
        if (pTuner->pI2c->WriteOneRegister (pTuner->pI2c, pTuner->i2cAddress, 0x21, 0x18) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
    }

    {
        /* MIX_TOL: System-L (Slow), !System-L (4k) */
        uint8_t data = 0x00 ;
        if (!(pTuner->flags & SONY_ASCOT2S_CONFIG_SECAM_REGION)) {
            /* MIX_TOL !System-L, set default 4k.  */
            data |= 0x10 ;
        }
        if (dvb_i2c_SetRegisterBits (pTuner->pI2c, pTuner->i2cAddress, 0x0C, data, 0x30) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
    }
    
    /* Wait CPU idle. */
    result = WaitCpuIdle (pTuner, ASCOT2S_XPON_WAIT_MS);
    if (result != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (result);
    }

    /* Check CPU error. */
    {
        uint8_t data = 0 ;
        if (pTuner->pI2c->ReadRegister (pTuner->pI2c, pTuner->i2cAddress, ASCOT2S_REG_CPU_ERR, &data, 1) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C); 
        }

        if (data != 0x00) {
            /* CPU error. */
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_OTHER);
        }
    }

    /* XOSC_SEL. Max. of DVB-C/T/T2 drive current for given XTAL config. */
    /* XOSC_SEL: (41MHz = 400uA, 4MHz = 100uA) */
    {
        uint8_t data = (pTuner->xtalFreq == XTAL_41MHZ) ? 0x10 : 0x04 ;
        if (pTuner->flags & SONY_ASCOT2S_CONFIG_EXT_REF) {
            data = 0x00 ; /* Oscillator stopped. */
        }
        if (dvb_i2c_SetRegisterBits (pTuner->pI2c, pTuner->i2cAddress, ASCOT2S_REG_REF_XOSC_SEL, data, 0x1F) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
    }

    /* Power save mode. */
    if (pTuner->pI2c->WriteOneRegister (pTuner->pI2c, pTuner->i2cAddress, 0x14, 0x00) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /* Stop PLL. */
    if (pTuner->pI2c->WriteOneRegister (pTuner->pI2c, pTuner->i2cAddress, 0x15, 0x04) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    SONY_DVB_TRACE_RETURN (result);  
}

/*------------------------------------------------------------------------------
 X_fin
------------------------------------------------------------------------------*/
static sony_dvb_result_t XFin (sony_dvb_tuner_t * pTuner)
{
    sony_dvb_result_t result = SONY_DVB_OK ;
    SONY_DVB_TRACE_ENTER ("XFin");

    if ((!pTuner) || (!pTuner->pI2c)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* Enable Power Save and Disable PLL. */
    if (pTuner->pI2c->WriteOneRegister (pTuner->pI2c, pTuner->i2cAddress, 0x14, 0x00) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }    
    if (pTuner->pI2c->WriteOneRegister (pTuner->pI2c, pTuner->i2cAddress, 0x15, 0x04) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }
    SONY_DVB_TRACE_RETURN (result);
}

/*------------------------------------------------------------------------------
 A_init
------------------------------------------------------------------------------*/
static sony_dvb_result_t AInit (sony_dvb_tuner_t * pTuner)
{
    sony_dvb_result_t result = SONY_DVB_OK ;
    SONY_DVB_TRACE_ENTER ("AInit");

    if ((!pTuner) || (!pTuner->pI2c)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* Disable Power Save and Enable PLL. */
    if (pTuner->pI2c->WriteOneRegister (pTuner->pI2c, pTuner->i2cAddress, 0x14, 0xFB) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }
    if (pTuner->pI2c->WriteOneRegister (pTuner->pI2c, pTuner->i2cAddress, 0x15, 0x0F) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /* Select IFOUT and IFAGC output/input. */
    {
        uint8_t data = 0x00 ; /* Default: IFOUT1, IFAGC1 */
        if (pTuner->flags & SONY_ASCOT2S_CONFIG_IFOUT_2) {
            data |= 0x04 ;  /* IFOUTP2/IFOUTN2: 1b */
        }
        if (pTuner->flags & SONY_ASCOT2S_CONFIG_IFAGC_2) {
            data |= 0x08 ;  /* AGC2: 01b */
        }
        if (dvb_i2c_SetRegisterBits (pTuner->pI2c, pTuner->i2cAddress, 0x05, data, 0x1C) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
    }

    /* REF_R: Depends on Xtal. */ 
    if (pTuner->pI2c->WriteOneRegister (pTuner->pI2c, pTuner->i2cAddress, 
            ASCOT2S_REG_REF_R, (uint8_t)(pTuner->xtalFreq == XTAL_41MHZ ? 40 : pTuner->xtalFreq)) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /* XOSC_SEL: (41MHz = 400uA, 4MHz = 50uA) */
    {
        uint8_t data = (pTuner->xtalFreq == XTAL_41MHZ) ? 0x10 : 0x02 ;
        if (pTuner->flags & SONY_ASCOT2S_CONFIG_EXT_REF) {
            data = 0x00 ; /* Oscillator stopped. */
        }
        if (dvb_i2c_SetRegisterBits (pTuner->pI2c, pTuner->i2cAddress, ASCOT2S_REG_REF_XOSC_SEL, data, 0x1F) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
    }

    /* KBW: Wc = 1.5kHz */
    if (pTuner->pI2c->WriteOneRegister (pTuner->pI2c, pTuner->i2cAddress, ASCOT2S_REG_KBW, 0x02) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /* C2_BANK, R2_BANK, R2_RANGE, ORDER: C2=400pF, R2=30K, R2_RANGE=0, ORDER=0 */
    if (dvb_i2c_SetRegisterBits (pTuner->pI2c, pTuner->i2cAddress, 0x0B, 0x0F, 0x3F) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /* MIX_OLL: 2.0Vp-p */
    if (dvb_i2c_SetRegisterBits (pTuner->pI2c, pTuner->i2cAddress, 0x0C, 0x02, 0x07) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /* RF_GAIN: High RF Gain. */
    if (dvb_i2c_SetRegisterBits (pTuner->pI2c, pTuner->i2cAddress, ASCOT2S_REG_RF_GAIN, 0x20, 0x3F) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /* IF_BPF_GC: +2dB BPF */
    if (dvb_i2c_SetRegisterBits (pTuner->pI2c, pTuner->i2cAddress, 0x0E, 0x02, 0x07) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    SONY_DVB_TRACE_RETURN (result);
}

/*------------------------------------------------------------------------------
 D_init
------------------------------------------------------------------------------*/
static sony_dvb_result_t DInit (sony_dvb_tuner_t * pTuner)
{
    sony_dvb_result_t result = SONY_DVB_OK ;
    SONY_DVB_TRACE_ENTER ("DInit");

    if ((!pTuner) || (!pTuner->pI2c)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* Disable Power Save and Enable PLL. */
    if (pTuner->pI2c->WriteOneRegister (pTuner->pI2c, pTuner->i2cAddress, 0x14, 0xFB) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }    
    if (pTuner->pI2c->WriteOneRegister (pTuner->pI2c, pTuner->i2cAddress, 0x15, 0x0F) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /* Select IFOUT and IFAGC output/input. */
    {
        uint8_t data = 0x00 ; /* Default: IFOUT1, IFAGC1 */
        if (pTuner->flags & SONY_ASCOT2S_CONFIG_IFOUT_2) {
            data |= 0x04 ;  /* IFOUTP2/IFOUTN2: 1b */
        }
        if (pTuner->flags & SONY_ASCOT2S_CONFIG_IFAGC_2) {
            data |= 0x08 ;  /* AGC2: 01b */
        }
        if (dvb_i2c_SetRegisterBits (pTuner->pI2c, pTuner->i2cAddress, 0x05, data, 0x1C) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
    }

    /* REF_R: Depends on Xtal. */ 
    if (pTuner->pI2c->WriteOneRegister (pTuner->pI2c, pTuner->i2cAddress, 
            ASCOT2S_REG_REF_R, (uint8_t)(pTuner->xtalFreq == XTAL_41MHZ ? 10 : 1)) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /* XOSC_SEL: (41MHz = 400uA, 4MHz = 100uA) */
    {
        uint8_t data = (pTuner->xtalFreq == XTAL_41MHZ) ? 0x10 : 0x04 ;
        if (pTuner->flags & SONY_ASCOT2S_CONFIG_EXT_REF) {
            data = 0x00 ; /* Oscillator stopped. */
        }
        if (dvb_i2c_SetRegisterBits (pTuner->pI2c, pTuner->i2cAddress, ASCOT2S_REG_REF_XOSC_SEL, data, 0x1F) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
    }

    /* KBW: Wc = 30kHz (14 = 0x0E) */
    if (pTuner->pI2c->WriteOneRegister (pTuner->pI2c, pTuner->i2cAddress, ASCOT2S_REG_KBW, 14) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /* C2_BANK, R2_BANK, R2_RANGE, ORDER: C2=100pF, R2=10K, R2_RANGE=0, ORDER=1 */
    if (dvb_i2c_SetRegisterBits (pTuner->pI2c, pTuner->i2cAddress, 0x0B, 0x25, 0x3F) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /* MIX_OLL: 2.5Vp-p */
    if (dvb_i2c_SetRegisterBits (pTuner->pI2c, pTuner->i2cAddress, 0x0C, 0x03, 0x07) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /* RF_GAIN: High RF Gain. */
    if (dvb_i2c_SetRegisterBits (pTuner->pI2c, pTuner->i2cAddress, ASCOT2S_REG_RF_GAIN, 0x20, 0x3F) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /* IF_BPF_GC: +8dB BPF */
    if (dvb_i2c_SetRegisterBits (pTuner->pI2c, pTuner->i2cAddress, 0x0E, 0x05, 0x07) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    SONY_DVB_TRACE_RETURN (result);
}

/*------------------------------------------------------------------------------
 X_tune
------------------------------------------------------------------------------*/
static sony_dvb_result_t XTune (sony_dvb_tuner_t * pTuner, 
                                uint32_t frequency, 
                                sony_dvb_system_t system,
                                uint8_t bandwidth)
{
    sony_dvb_result_t result = SONY_DVB_OK ;
    SONY_DVB_TRACE_ENTER ("XTune");

    if ((!pTuner) || (!pTuner->pI2c)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    if ((frequency < ASCOT2S_MIN_FKHZ) || (frequency > ASCOT2S_MAX_FKHZ)) {
        SONY_DVB_TRACE_RETURN(SONY_DVB_ERROR_RANGE);
    }

    /* Hold reset of RFAGC circuit. */
    if (pTuner->flags & SONY_ASCOT2S_CONFIG_RFAGC_EXTERNAL) {
        /* External RFAGC (overload) circuit. */
        if (pTuner->pI2c->WriteOneRegister (pTuner->pI2c, pTuner->i2cAddress, 0x3F, 0x01) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
    }
    else {
        /* Internal RFAGC (overload) circuit. */
        if (pTuner->pI2c->WriteOneRegister (pTuner->pI2c, pTuner->i2cAddress, 0x21, 0x1C) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
        if (pTuner->pI2c->WriteOneRegister (pTuner->pI2c, pTuner->i2cAddress, 0x3F, 0x01) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
    }

    /*
                    CHAN    BW       IF  
      DVB-T         8MHz    8.0MHz   5.0MHz
      DVB-T         7MHz    7.0MHz   4.5MHz
      DVB-T         6MHz    5.8MHz   3.8MHz
      DVB-T2        8MHz    8.0MHz   4.85MHz
      DVB-T2        7MHz    7.0MHz   4.35MHz
      DVB-T2        6MHz    6.0MHz   4.0MHz
      DVB-T2        5MHz    6.0MHz   4.0MHz
      DVB-C         ----    8.3MHz   5.1MHz

    */

    {
        uint8_t bwOffset = 0 ;
        uint8_t ifOffset = 0 ;
        uint8_t data[5] ;
        
        data[0] = (uint8_t)(frequency & 0xFF) ;
        data[1] = (uint8_t)((frequency & 0xFF00) >> 8) ;
        data[2] = (uint8_t)((frequency & 0x0F0000) >> 16) ;
        data[3] = 0xFF ; /* VCO calibration. */
        data[4] = 0xFF ; /* Enable Analog block. */

        switch (system)
        {
            case SONY_DVB_SYSTEM_DVBC:
            {
                switch (bandwidth)
                {
                    case 8:
                        /* +300kHz BW offset */
                        /* +100kHz IF offset */
                        /* 8MHz BW */
                        ifOffset = 0x10 ;
                        bwOffset = 0x06 ;
                        data[2] |= 0x20 ;
                        break ;
                    default:
                        SONY_DVB_TRACE_RETURN(SONY_DVB_ERROR_NOSUPPORT);
                }
            }
            break ;

            case SONY_DVB_SYSTEM_DVBT:
            {
                switch (bandwidth)
                {
                    case 6:
                        /* 00b */
                        /* -200kHz IF offset, -200kHz BW offset. */
                        bwOffset = 0x1C ;
                        ifOffset = 0xE0 ;
                        break ;
                    case 7:
                        /* 01b */
                        /* 0 IF offset, 0 BW offset. */
                        data[2] |= 0x10 ;
                        break ;
                    case 8:
                        /* 10b */
                        /* 0 IF offset, 0 BW offset. */
                        data[2] |= 0x20 ;
                        break ;
                    default:
                        SONY_DVB_TRACE_RETURN(SONY_DVB_ERROR_NOSUPPORT);
                }
            }
            break ;

            case SONY_DVB_SYSTEM_DVBT2:
            {
                switch (bandwidth)
                {
                    /* Intentional fall-through. */
                    case 5:
                    case 6:
                        /* 00b */
                        /* 0 IF offset, 0 BW offset. */
                        break ;
                    case 7:
                        /* 01b */
                        /* -150kHz IF offset, 0 BW offset. */
                        data[2] |= 0x10 ;
                        ifOffset = 0xE8 ;
                        break ;
                    case 8:
                        /* 10b */
                        /* -150kHz IF offset, 0 BW offset. */
                        data[2] |= 0x20 ;
                        ifOffset = 0xE8 ;
                        break ;
                    default:
                        SONY_DVB_TRACE_RETURN(SONY_DVB_ERROR_NOSUPPORT);
                }
            }
            break ;

            /* Intentional fall-through. */
            case SONY_DVB_SYSTEM_UNKNOWN:
            default:
                SONY_DVB_TRACE_RETURN(SONY_DVB_ERROR_NOSUPPORT);
        }

        /* Band centre frequency. Do nothing. */
      
        /* 0x0E: FIF_OFFSET: IF offset. */
        if (dvb_i2c_SetRegisterBits (pTuner->pI2c, pTuner->i2cAddress, 0x0E, ifOffset, 0xF8) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        /* 0x0F: BW_OFFSET: BW offset. */
        if (dvb_i2c_SetRegisterBits (pTuner->pI2c, pTuner->i2cAddress, 0x0F, bwOffset, 0x1F) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        /* Tune. */
        if (pTuner->pI2c->WriteRegister(pTuner->pI2c, pTuner->i2cAddress, 0x10, data, sizeof(data)) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN(SONY_DVB_ERROR_I2C);
        }

        /* Wait CPU idle. */
        result = WaitCpuIdle (pTuner, ASCOT2S_XTUNE_WAIT_MS);
        if (result != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (result);
        }

        if (pTuner->flags & SONY_ASCOT2S_CONFIG_RFAGC_EXTERNAL) {
            /* Release reset of external RFAGC(Overload) circuit. */
            if (pTuner->pI2c->WriteOneRegister (pTuner->pI2c, pTuner->i2cAddress, 0x3F, 0x00) != SONY_DVB_OK) {
                SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
            }

            /* Wait for RFAGC settling time. */
            SONY_DVB_SLEEP (ASCOT2S_RFAGC_EXTERN_SETTLING_MS);
        }
        else {
            /* Release reset of internal RFAGC(Overload) circuit. */
            if (pTuner->pI2c->WriteOneRegister (pTuner->pI2c, pTuner->i2cAddress, 0x21, 0x18) != SONY_DVB_OK) {
                SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
            }
            if (pTuner->pI2c->WriteOneRegister (pTuner->pI2c, pTuner->i2cAddress, 0x3F, 0x00) != SONY_DVB_OK) {
                SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
            }

            /* Wait for RFAGC settling time. */
            SONY_DVB_SLEEP (ASCOT2S_RFAGC_INTERN_SETTLING_MS);
        }
    }

    SONY_DVB_TRACE_RETURN (result);
}

static sony_dvb_result_t WaitCpuIdle (sony_dvb_tuner_t* pTuner, 
                                      uint8_t timeoutMs)
{
    sony_dvb_result_t result = SONY_DVB_OK ;
    SONY_DVB_TRACE_ENTER ("WaitCpuIdle");

    if ((!pTuner) || (!pTuner->pI2c)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    {
        uint8_t i = 0 ;
        uint8_t data = 0x00 ;
        for (i = 0 ; i < (timeoutMs + POLL_INTERVAL_MS/2) / POLL_INTERVAL_MS ; i++) {
            
            SONY_DVB_SLEEP (POLL_INTERVAL_MS);
            
            if (pTuner->pI2c->ReadRegister (pTuner->pI2c, pTuner->i2cAddress, ASCOT2S_REG_CPU_STT, &data, 1) != SONY_DVB_OK) {
                SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
            }

            if (data == 0x00) {
                result = SONY_DVB_OK ;
                break ;
            }
            result = SONY_DVB_ERROR_TIMEOUT ;
        }
    }

    SONY_DVB_TRACE_RETURN (result);
}
