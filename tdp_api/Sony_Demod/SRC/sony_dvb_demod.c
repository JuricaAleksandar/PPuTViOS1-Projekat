/*------------------------------------------------------------------------------

 <dev:header>
    Copyright(c) 2009 Sony Corporation.

    $Revision: 2014 $
    $Author: sking $

</dev:header>

------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------

  CXD2820 hardware state chart
                                   PowerOn
                                      |
                                      |
                                      V 
                              +---------------+
                              | Shutdown-ULP  |
             OSCENBN=H ------>|               |
             (From any state) +---------------+
                                      |
                                      | OSCENBN=L, RESETN=L 
                                      V
              +-----------------------------------------------+
              |                     Sleep                     |
              |                                               |
              +-----------------------------------------------+
                  |       ^        |       ^      |        ^
                  |       |        |       |      |        |
                SL2NOT  NOT2SL   SL2NOG  NOG2SL  SL2NOC  NOC2SL
                  |       |        |       |      |        |
                  V       |        V       |      V        |
              +-----------------------------------------------+
              |    Normal-T    |   Normal-T2   |    Normal-C  |
              |   (DVB-T)      |   (DVB-T2)    |   (DVB-C)    |
              +-----------------------------------------------+


          SL2NOT : Sleep      -> Normal-T
          NOT2SL : Normal-T   -> Sleep
          SL2NOG : Sleep      -> Normal-T2
          NOG2SL : Normal-T2  -> Sleep
          SL2NOC : Sleep      -> Normal-C
          NOC2SL : Normal-C   -> Sleep

------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------

  CXD2817 hardware state chart
                                   PowerOn
                                      |
                                      |
                                      V 
               +---------------------------------------+
               | OSCENBN=H                             | OSCENBN=L, RESETN=L 
               V                                       V 
        +---------------+  OSCENBN=L, RESETN=L  +---------------+
        | Shutdown-ULP  |---------------------->|  Shutdown-LP  |
        |               |                       |               |
        +---------------+                       +---------------+
               ^                                    |      ^     
               |  OSCENBN=H                         |      |      
               |  (From any state)                SD2SL   SL2SD 
                                                    |      | 
                                                    V      | 
              +-----------------------------------------------+
              |                     Sleep                     |
              |                                               |
              +-----------------------------------------------+
                  |       ^                       |        ^
                  |       |                       |        |
                SL2NOT  NOT2SL                   SL2NOC  NOC2SL
                  |       |                       |        |
                  V       |                       V        |
              +-----------------------------------------------+
              |    Normal-T            |   Normal-C           |
              |   (DVB-T)              |   (DVB-C)            |
              +-----------------------------------------------+
          
          SD2SL  : Shutdown-LP -> Sleep
          SL2SD  : Sleep       -> Shutdown-LP
          SL2NOT : Sleep       -> Normal-T
          NOT2SL : Normal-T    -> Sleep
          SL2NOC : Sleep       -> Normal-C
          NOC2SL : Normal-C    -> Sleep

------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
  Demodulator settings based on:
  CXD2820 ES1.0: StandardIF Register usage v1.15.
  CXD2820 ES1.0: Ascot Register usage v1.25.
  CXD2820 ES1.1: EN Register usage v1.28.
------------------------------------------------------------------------------*/
#include "sony_cxd2820.h"
#include <sony_dvb_demod.h>
#include <sony_dvb_demod_monitor.h>
#include <sony_dvb_demod_monitorT2.h>

#define IO_PORT_DEFAULT_SETTING     0x08

/*------------------------------------------------------------------------------
  DVB-T2 register freeze/unfreeze
------------------------------------------------------------------------------*/
#define t2_FreezeReg(pDemod) ((pDemod)->pI2c->WriteOneRegister ((pDemod)->pI2c, (pDemod)->i2cAddressT2, CXD2820R_B0_REGC_FREEZE, 0x01))
#define t2_UnFreezeReg(pDemod) ((void)((pDemod)->pI2c->WriteOneRegister ((pDemod)->pI2c, (pDemod)->i2cAddressT2, CXD2820R_B0_REGC_FREEZE, 0x00))) 

/*------------------------------------------------------------------------------
  Local types
------------------------------------------------------------------------------*/

/**
 @brief Defines for demod registers.
*/
typedef struct sony_dvb_demod_reg_t {
    uint8_t bank ;
    uint8_t subAddress ;
    uint8_t value ;
    uint8_t mask ;
} sony_dvb_demod_reg_t ;

/*------------------------------------------------------------------------------
  Static functions
------------------------------------------------------------------------------*/
static sony_dvb_result_t CheckTPSLock (sony_dvb_demod_t * pDemod, 
                                       sony_dvb_demod_lock_result_t * pLock);
static sony_dvb_result_t SD2SL (sony_dvb_demod_t * pDemod);
static sony_dvb_result_t SL2SD (sony_dvb_demod_t * pDemod);
static sony_dvb_result_t SL2NOT (sony_dvb_demod_t * pDemod);
static sony_dvb_result_t SL2NOT_BandSetting (sony_dvb_demod_t * pDemod);
static sony_dvb_result_t NOT2SL (sony_dvb_demod_t * pDemod);
static sony_dvb_result_t SL2NOC (sony_dvb_demod_t * pDemod);
static sony_dvb_result_t NOC2SL (sony_dvb_demod_t * pDemod);

/* T2 specific */
/**
 @brief Active T2 state (G) to Sleep.
*/ 
static sony_dvb_result_t NOG2SL (sony_dvb_demod_t * pDemod);
/**
 @brief Sleep to active T2 (G) state.
*/ 
static sony_dvb_result_t SL2NOG (sony_dvb_demod_t * pDemod);
/**
 @brief Sleep to active T2 (G) state: bandwidth configuration.
*/ 
static sony_dvb_result_t SL2NOG_BandSetting (sony_dvb_demod_t * pDemod);
static sony_dvb_result_t t2_CheckDemodLock (sony_dvb_demod_t * pDemod, 
                                            sony_dvb_demod_lock_result_t* pLock);

/**
 @brief Set the T2 demodulator for configuration items 
        not set by the T/C configuration.

 @param pDemod The demod instance.
 @param configId The configuration Id.
 @param value The value of the configuration item.

 @return SONY_DVB_OK if successful.
*/
static sony_dvb_result_t t2_SetConfig (sony_dvb_demod_t * pDemod, 
                                        sony_dvb_demod_config_id_t configId, 
                                        int32_t value);

/**
 @brief Function that writes an array of register values into the demodulator.
 
 @param pDemod The demod instance. 
 @param address The I2C address that the regs reside in.
 @param regs The registers to write.
 @param regCount The number of registers in the register count array.

 @return SONY_DVB_OK if all registers successfully written.
*/
static sony_dvb_result_t demod_WriteRegisters (sony_dvb_demod_t* pDemod,
                                               uint8_t address,
                                               const sony_dvb_demod_reg_t* regs,
                                               uint8_t regCount);

/**
 @brief Function that is used to prepare for new acquisition to DVB-T2 
        channel.
 
 @param pDemod The demod instance. 

 @return SONY_DVB_OK if successful.
*/
static sony_dvb_result_t t2_PrepareAcquisition (sony_dvb_demod_t* pDemod);

/**
 @brief Function used to set the OREG_CRL_OPEN register.

 @param pDemod The demod instance. 
 @param crlOpen The OREG_CRL_OPEN register value to write.

 @return SONY_DVB_OK if successful.
*/
static sony_dvb_result_t t2_SetCrlOpen (sony_dvb_demod_t* pDemod, 
                                        uint8_t crlOpen);

/**
 @brief Function used for initialising device for DVB-T2 Long Echo sequence or
        returning device to default state on disabling DVB-T2 Long Echo sequence.

 @param pDemod The demod instance. 

 @return SONY_DVB_OK if successful.
*/
static sony_dvb_result_t le_seq_Init (sony_dvb_demod_t* pDemod);

/*------------------------------------------------------------------------------
  Implementation of public functions
------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------
  Initialize demod

  Initialize the demod chip and the demod struct.
  The demod chip is set to "Sleep" state.

  NOTE: I2C addresses and pI2c must be set before this function calls.

  sony_dvb_demod_t *pDemod   Instance of demod control struct
--------------------------------------------------------------------*/
sony_dvb_result_t dvb_demod_Initialize (sony_dvb_demod_t * pDemod)
{
    sony_dvb_result_t result = SONY_DVB_OK;

    SONY_DVB_TRACE_ENTER ("dvb_demod_Initialize");

    if (!pDemod)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);

    /* Initialize sony_dvb_demod_t */
    pDemod->state = SONY_DVB_DEMOD_STATE_UNKNOWN;
    pDemod->system = SONY_DVB_SYSTEM_UNKNOWN;
    pDemod->bandWidth = 0;

    pDemod->dvbt_scannmode = 0; /* OFF is default */
    pDemod->enable_rflvmon = 1; /* ON is default */
    pDemod->confSense = DVB_DEMOD_SPECTRUM_NORMAL; /* NORMAL default */
    pDemod->ioHiZ = IO_PORT_DEFAULT_SETTING ; /* RFAGC Hi-Z by default. */
    pDemod->t2LongEchoPP2 = 0; /* Default: Disabled DVB-T2 Long Echo sequence. */
    
    result = dvb_demod_monitor_ChipID (pDemod, &(pDemod->chipId)) ;
    if (result != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (result);
    }

    /* HW Reset (set pinRESETN 'L') here */
    /* Handle SD2SL() transition for CXD2817 device. */
    if (pDemod->chipId == SONY_DVB_CXD2817) {
        result = SD2SL (pDemod);
    }
    if (result == SONY_DVB_OK) {
        pDemod->state = SONY_DVB_DEMOD_STATE_SLEEP;
    }
    else {
        pDemod->state = SONY_DVB_DEMOD_STATE_INVALID;
    }

    SONY_DVB_TRACE_RETURN (result);
}

/*--------------------------------------------------------------------
  Finalize demod

  Finalize the demod chip. The demod chip is set to "ShutDown" state.
  Call dvb_demod_Initialize if re-activate demod.

  sony_dvb_demod_t *pDemod   Instance of demod control struct
--------------------------------------------------------------------*/
sony_dvb_result_t dvb_demod_Finalize (sony_dvb_demod_t * pDemod)
{
    sony_dvb_result_t result = SONY_DVB_OK;

    SONY_DVB_TRACE_ENTER ("dvb_demod_Finalize");

    if (!pDemod)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);

    if (pDemod->state == SONY_DVB_DEMOD_STATE_ACTIVE) {
        result = dvb_demod_Sleep (pDemod);
        if (result != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (result);
        }
    }

    /* Handle SL2SD() transition for CXD2817 device. */
    if (pDemod->chipId == SONY_DVB_CXD2817) {
        result = SL2SD (pDemod);
    }
    if (result == SONY_DVB_OK) {
        pDemod->state = SONY_DVB_DEMOD_STATE_SHUTDOWN;
    }
    else {
        pDemod->state = SONY_DVB_DEMOD_STATE_INVALID;
    }

    SONY_DVB_TRACE_RETURN (result);
}

/*--------------------------------------------------------------------
  Activate demod

  Set the demod chip to DVB-T2/T/C state.

  sony_dvb_demod_t  *pDemod     Instance of demod control struct
  sony_dvb_system_t system      System enum value
  uint8_t           bandWidth   Bandwidth(MHz) 6or7or8(DVB-T), 8(DVB-C)
--------------------------------------------------------------------*/
sony_dvb_result_t dvb_demod_Tune (sony_dvb_demod_t * pDemod, 
                                  sony_dvb_system_t system, 
                                  uint8_t bandWidth)
{
    sony_dvb_result_t result = SONY_DVB_OK;

    SONY_DVB_TRACE_ENTER ("dvb_demod_Tune");
    if (!pDemod) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* Argument check */
    if (system == SONY_DVB_SYSTEM_DVBT) {
        if ((bandWidth != 6) && (bandWidth != 7) && (bandWidth != 8)) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_NOSUPPORT);   /* Not supported bandwidth */
        }
    }
    else if (system == SONY_DVB_SYSTEM_DVBC) {
        if (bandWidth != 8) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG); /* Not supported bandwidth */
        }
    }
    else if (system == SONY_DVB_SYSTEM_DVBT2) {
        if ((bandWidth != 5) && (bandWidth != 6) && (bandWidth != 7) && (bandWidth != 8)) {
            /* Not supported bandwidth */
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_NOSUPPORT);
        }
    }
    else {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG); /* Unknown system */
    }

    /* Software state check */
    if ((pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE)
        && (pDemod->state != SONY_DVB_DEMOD_STATE_SLEEP)) {
        /* This api is accepted in ACTIVE or SLEEP states only */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }
    
    /* Device type check. */
    if ((pDemod->chipId == SONY_DVB_CXD2817) && (system == SONY_DVB_SYSTEM_DVBT2)) {
        /* CXD2817 device does not support DVB-T2. */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_NOSUPPORT);
    }

    /* Enable Acquisition. */
    if (system != pDemod->system) {
        /* System changed ! */
        /* NOTE: In SLEEP state, pDemod->system is SONY_DVB_SYSTEM_UNKNOWN,
         * so this line will be executed. */
        if (pDemod->state == SONY_DVB_DEMOD_STATE_ACTIVE) {
            /* Set chip to Sleep state */
            result = dvb_demod_Sleep (pDemod);
            if (result != SONY_DVB_OK) {
                SONY_DVB_TRACE_RETURN (result);
            }
        }

        /* Set new system/bandWidth */
        pDemod->system = system;
        pDemod->bandWidth = bandWidth;
        switch (pDemod->system) {
        case SONY_DVB_SYSTEM_DVBT:
            result = SL2NOT (pDemod);
            break;
        case SONY_DVB_SYSTEM_DVBC:
            result = SL2NOC (pDemod);
            break;
        case SONY_DVB_SYSTEM_DVBT2:
            result = SL2NOG (pDemod);
            break;

        /* Intentional fall through. */
        case SONY_DVB_SYSTEM_UNKNOWN:
        default:
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG); /* Unknown system */
        }
    }
    else {
        if (bandWidth != pDemod->bandWidth) {
            /* BandWidth changed ! (DVB-T/T2) */
            pDemod->system = system;
            pDemod->bandWidth = bandWidth;
            switch (pDemod->system) {
            case SONY_DVB_SYSTEM_DVBT:
                result = SL2NOT_BandSetting (pDemod);
                break;
            case SONY_DVB_SYSTEM_DVBT2:
                result = SL2NOG_BandSetting (pDemod);
                break;
            
            /* Intentional fall through. */
            case SONY_DVB_SYSTEM_DVBC:
            case SONY_DVB_SYSTEM_UNKNOWN:
            default:
                SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_OTHER);
            }
        }

        /* Prepare DVB-T2 demodulator for acquisition. */
        if (pDemod->system == SONY_DVB_SYSTEM_DVBT2) {
            result = t2_PrepareAcquisition (pDemod);
            if (result != SONY_DVB_OK) {
                SONY_DVB_TRACE_RETURN (result);
            }
            
            if (!pDemod->t2LongEchoPP2) {
                /* Soft reset is performed in DVB-T2 Long Echo sequence. */
                result = dvb_demod_SoftReset (pDemod);
            }
        }
        else {
            /* Start acquisition. */
            result = dvb_demod_SoftReset (pDemod);
        }
    }

    if (result != SONY_DVB_OK) {
        pDemod->state = SONY_DVB_DEMOD_STATE_INVALID;
    }

    SONY_DVB_TRACE_RETURN (result);
}

/*--------------------------------------------------------------------
  Sleep demod

  Set the demod chip to "Sleep" state.

  sony_dvb_demod_t *pDemod   Instance of demod control struct
--------------------------------------------------------------------*/
sony_dvb_result_t dvb_demod_Sleep (sony_dvb_demod_t * pDemod)
{
    SONY_DVB_TRACE_ENTER ("dvb_demod_Sleep");

    if (!pDemod)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);

    /* Software state check */
    if (pDemod->state == SONY_DVB_DEMOD_STATE_ACTIVE) {
        sony_dvb_result_t result = SONY_DVB_OK;

        switch (pDemod->system) {
        case SONY_DVB_SYSTEM_DVBT2:
            result = NOG2SL(pDemod);
            break ;
        case SONY_DVB_SYSTEM_DVBT:
            result = NOT2SL (pDemod);
            break;
        case SONY_DVB_SYSTEM_DVBC:
            result = NOC2SL (pDemod);
            break;

         /* Intentional fall through. */
         case SONY_DVB_SYSTEM_UNKNOWN:
         default:
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
        }

        if (result == SONY_DVB_OK) {
            pDemod->state = SONY_DVB_DEMOD_STATE_SLEEP;
            pDemod->system = SONY_DVB_SYSTEM_UNKNOWN;
            pDemod->bandWidth = 0;
        }
        else {
            pDemod->state = SONY_DVB_DEMOD_STATE_INVALID;   /* Should be finalized ... */
            pDemod->system = SONY_DVB_SYSTEM_UNKNOWN;
            pDemod->bandWidth = 0;
        }
        SONY_DVB_TRACE_RETURN (result);
    }
    else if (pDemod->state == SONY_DVB_DEMOD_STATE_SLEEP) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_OK);    /* Nothing to do */
    }
    else {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }
}


/*--------------------------------------------------------------------
  TPS Lock Check static function
  (Called by dvb_demod_CheckDemodLock/dvb_demod_CheckTSLock)
  NOTE: This function must be called 80ms after soft reset.
--------------------------------------------------------------------*/
static sony_dvb_result_t CheckTPSLock (sony_dvb_demod_t * pDemod, 
                                       sony_dvb_demod_lock_result_t * pLock)
{
    SONY_DVB_TRACE_ENTER ("CheckTPSLock");

    if (!pDemod || !pLock)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);

    if (pDemod->state == SONY_DVB_DEMOD_STATE_ACTIVE) {
        switch (pDemod->system) {
        case SONY_DVB_SYSTEM_DVBT:
        {
            uint8_t rdata = 0;

            /* Set bank 0 */
            if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x00, 0x00) != SONY_DVB_OK)
                SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);

            if (pDemod->dvbt_scannmode) {   /* Unlock flag check while scanning */
                /* Register freeze */
                if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x01, 0x01) != SONY_DVB_OK)
                    SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);

                /* Read syncstat0 */
                if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x10, &rdata, 1) != SONY_DVB_OK)
                    SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
                pDemod->dvbt_lock_info.IREG_SEQ_OSTATE = (uint8_t) (rdata & 0x07);

                /* Read argd_state */
                if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x14, &rdata, 1) != SONY_DVB_OK)
                    SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
                pDemod->dvbt_lock_info.IREG_ARGD_STATE = (uint8_t) (rdata & 0x0F);

                /* Set bank 0x0A */
                if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x00, 0x0A) != SONY_DVB_OK)
                    SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);

                /* Read IREG_COSNE_COK */
                if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x14, &rdata, 1) != SONY_DVB_OK)
                    SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
                pDemod->dvbt_lock_info.IREG_COSNE_COK = (rdata & 0x10) ? 1 : 0;

                /* Set bank 0 */
                if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x00, 0x00) != SONY_DVB_OK)
                    SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);

                /* Register unfreeze */
                if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x01, 0x00) != SONY_DVB_OK)
                    SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);

                /* Check Lock/Unlock */
                if (pDemod->dvbt_lock_info.IREG_SEQ_OSTATE >= 6) {
                    *pLock = DEMOD_LOCK_RESULT_LOCKED;
                }
                else if ((pDemod->dvbt_lock_info.IREG_ARGD_STATE >= 4) && !pDemod->dvbt_lock_info.IREG_COSNE_COK) {
                    *pLock = DEMOD_LOCK_RESULT_UNLOCKED;
                }
                else {
                    *pLock = DEMOD_LOCK_RESULT_NOTDETECT;
                }
            }
            else {
                /* Read syncstat0 */
                if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x10, &rdata, 1) != SONY_DVB_OK)
                    SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
                pDemod->dvbt_lock_info.IREG_SEQ_OSTATE = (uint8_t) (rdata & 0x07);

                /* Check Lock or not */
                if (pDemod->dvbt_lock_info.IREG_SEQ_OSTATE >= 6) {
                    *pLock = DEMOD_LOCK_RESULT_LOCKED;
                }
                else {
                    *pLock = DEMOD_LOCK_RESULT_NOTDETECT;
                }
            }
            SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
        }
        case SONY_DVB_SYSTEM_DVBC:
        {
            *pLock = DEMOD_LOCK_RESULT_UNLOCKED;
            SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
        }
        /* Intentional fall through. */
        case SONY_DVB_SYSTEM_DVBT2:
        case SONY_DVB_SYSTEM_UNKNOWN:
        default:
            *pLock = DEMOD_LOCK_RESULT_UNLOCKED;
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
        }
    }
    else {
        *pLock = DEMOD_LOCK_RESULT_UNLOCKED;
        SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
    }
}

/*--------------------------------------------------------------------
  Check Demod Lock

  Check lock state of demod. (no wait)
  Integration part Tune function call this function to decide "lock" or not.
  Which state is considered as "lock" is system dependent.
  (DVB-T2: OFDM lock (P1/L1-pre/L1-post), DVB-T : TPS Lock, DVB-C : TS Lock)

  sony_dvb_demod_t               *pDemod   Instance of demod control struct
  sony_dvb_demod_lock_result_t   *pLock    Demod lock state
--------------------------------------------------------------------*/
sony_dvb_result_t dvb_demod_CheckDemodLock (sony_dvb_demod_t * pDemod, 
                                            sony_dvb_demod_lock_result_t * pLock)
{
    SONY_DVB_TRACE_ENTER ("dvb_demod_CheckDemodLock");

    if (!pDemod || !pLock)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);

    if (pDemod->state == SONY_DVB_DEMOD_STATE_ACTIVE) {
        switch (pDemod->system) {
        case SONY_DVB_SYSTEM_DVBT:
        {
            /* DVB-T, Demod lock == TPS Lock.*/
            sony_dvb_result_t result = SONY_DVB_OK;
            result = CheckTPSLock (pDemod, pLock);
            SONY_DVB_TRACE_RETURN (result);
        }
        case SONY_DVB_SYSTEM_DVBC:
        {
            /* DVB-C, Demod lock == TS Lock. */
            sony_dvb_result_t result = SONY_DVB_OK;
            result = dvb_demod_CheckTSLock (pDemod, pLock);
            SONY_DVB_TRACE_RETURN (result);
        }
        case SONY_DVB_SYSTEM_DVBT2:
        {
            /* DVB-T2, Demod lock == OFDM core lock. */
            sony_dvb_result_t result = SONY_DVB_OK;
            result = t2_CheckDemodLock (pDemod, pLock);
            SONY_DVB_TRACE_RETURN (result);
        }

        /* Intentional fall through. */
        case SONY_DVB_SYSTEM_UNKNOWN:
        default:
            *pLock = DEMOD_LOCK_RESULT_UNLOCKED;
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
        }
    }
    else {
        *pLock = DEMOD_LOCK_RESULT_UNLOCKED;
        SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
    }
}

/*--------------------------------------------------------------------
  Check TS Lock

  Check TS lock state of demod. (no wait)

  sony_dvb_demod_t               *pDemod   Instance of demod control struct
  sony_dvb_demod_lock_result_t   *pLock    TS lock state
--------------------------------------------------------------------*/
sony_dvb_result_t dvb_demod_CheckTSLock (sony_dvb_demod_t * pDemod, 
                                         sony_dvb_demod_lock_result_t * pLock)
{
    SONY_DVB_TRACE_ENTER ("dvb_demod_CheckTSLock");

    if (!pDemod || !pLock)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);

    /* Software state check */
    if (pDemod->state == SONY_DVB_DEMOD_STATE_ACTIVE) {
        switch (pDemod->system) {
        case SONY_DVB_SYSTEM_DVBT:
        {
            sony_dvb_result_t result = SONY_DVB_OK;
            uint8_t rdata = 0;

            /* TPS Lock check first */
            result = CheckTPSLock (pDemod, pLock);
            if (result != SONY_DVB_OK)
                SONY_DVB_TRACE_RETURN (result);
            if (*pLock != DEMOD_LOCK_RESULT_LOCKED)
                SONY_DVB_TRACE_RETURN (result);

            /* Set bank 0 */
            if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x00, 0x00) != SONY_DVB_OK)
                SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);

            /* Read fec_status */
            if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x73, &rdata, 1) != SONY_DVB_OK)
                SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);

            *pLock = (rdata & 0x08) ? DEMOD_LOCK_RESULT_LOCKED : DEMOD_LOCK_RESULT_NOTDETECT;
            SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
        }
        case SONY_DVB_SYSTEM_DVBC:
        {
            uint8_t rdata = 0;
            /* Set bank 0 */
            if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressC, 0x00, 0x00) != SONY_DVB_OK)
                SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);

            /* Read ar_status1 */
            if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressC, 0x88, &rdata, 1) != SONY_DVB_OK)
                SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);

            if ((rdata & 0x03) == 0x02) {
                /* AR unlocked */
                *pLock = DEMOD_LOCK_RESULT_UNLOCKED;
                SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
            }
            else if ((rdata & 0x03) != 0x01) {
                /* Not AR locked */
                *pLock = DEMOD_LOCK_RESULT_NOTDETECT;
                SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
            }

            /* Read fec_status */
            if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressC, 0x73, &rdata, 1) != SONY_DVB_OK)
                SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);

            *pLock = (rdata & 0x08) ? DEMOD_LOCK_RESULT_LOCKED : DEMOD_LOCK_RESULT_NOTDETECT;
            SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
        }
        case SONY_DVB_SYSTEM_DVBT2:
        {
            sony_dvb_result_t result = SONY_DVB_OK;
            uint8_t syncState = 0, tsLock = 0;

            result = dvb_demod_monitorT2_SyncStat (pDemod, &syncState, &tsLock);
            if (result != SONY_DVB_OK) {
                SONY_DVB_TRACE_RETURN (result);
            }
            /* Check DMD_OK (Demodulator Lock) & TS Lock. */
            *pLock = DEMOD_LOCK_RESULT_NOTDETECT;
            if ((syncState == 6) && (tsLock == 1)) {
                *pLock = DEMOD_LOCK_RESULT_LOCKED;
            }
       
            SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
        }

        /* Intentional fall through. */
        case SONY_DVB_SYSTEM_UNKNOWN:
        default:
            *pLock = DEMOD_LOCK_RESULT_UNLOCKED;
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
        }
    }
    else {
        *pLock = DEMOD_LOCK_RESULT_UNLOCKED;
        SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
    }
}

/*--------------------------------------------------------------------
  Configuration of the demod

  Set several parameters for the demod chip and this driver.
  Should be call this function after Initialization.

  sony_dvb_demod_t             *pDemod   Instance of demod control struct
  sony_dvb_SONY_DVB_DEMOD_id_t configId  Enum value defined in this file
  int32_t                      value     Value to be set
--------------------------------------------------------------------*/
sony_dvb_result_t dvb_demod_SetConfig (sony_dvb_demod_t * pDemod, 
                                       sony_dvb_demod_config_id_t configId, 
                                       int32_t value)
{
    uint8_t i2cAddressActive = 0x00;    /* Activated slave address (DVB-T or DVB-C) */
    sony_dvb_result_t result = SONY_DVB_OK ;
    SONY_DVB_TRACE_ENTER ("dvb_demod_SetConfig");

    if (!pDemod)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);

    /* Software state check */
    if (pDemod->state == SONY_DVB_DEMOD_STATE_ACTIVE) {
        if (pDemod->system == SONY_DVB_SYSTEM_DVBC) {
            i2cAddressActive = pDemod->i2cAddressC;
        }
        else if (pDemod->system == SONY_DVB_SYSTEM_DVBT2) {
            i2cAddressActive = pDemod->i2cAddressT2;
        }
        else {
            i2cAddressActive = pDemod->i2cAddressT;
        }
    }
    else if (pDemod->state == SONY_DVB_DEMOD_STATE_SLEEP) {
        i2cAddressActive = pDemod->i2cAddressT;
    }
    else {
        /* This api is accepted in ACTIVE or SLEEP states only */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    switch (configId) {
        /* ======================== DVB-T and DVB-C ============================= */
    case DEMOD_CONFIG_PARALLEL_SEL:
        /* OREG_TSIF_PARALLEL_SEL (Serial/Parallel output select) */
        /* This register can change only in SLEEP state */
        if (pDemod->state != SONY_DVB_DEMOD_STATE_SLEEP)
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
        /* Set bank 0x00 */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x00, 0x00) != SONY_DVB_OK)
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        /* Set OREG_TSIF_PARALLEL_SEL (bit 4) */
        if (dvb_i2c_SetRegisterBits (pDemod->pI2c, pDemod->i2cAddressT, 0x70,
                                     (uint8_t) (value ? 0x10 : 0x00), 0x10) != SONY_DVB_OK)
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        break;

    case DEMOD_CONFIG_SER_DATA_ON_MSB:
        /* OREG_TSIF_SER_DATA_ON_MSB (Bit order on serial mode) */
        /* Set bank 0x00 */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, i2cAddressActive, 0x00, 0x00) != SONY_DVB_OK)
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        /* Set OREG_TSIF_SER_DATA_ON_MSB (bit 5) */
        if (dvb_i2c_SetRegisterBits (pDemod->pI2c, i2cAddressActive, 0x70,
                                     (uint8_t) (value ? 0x20 : 0x00), 0x20) != SONY_DVB_OK)
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        break;

    case DEMOD_CONFIG_OUTPUT_SEL_MSB:
        /* OREG_TSIF_OUTPUT_SEL_MSB (Bit order on parallel mode) */
        /* Set bank 0x00 */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, i2cAddressActive, 0x00, 0x00) != SONY_DVB_OK)
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        /* Set OREG_TSIF_OUTPUT_SEL_MSB (bit 3) */
        if (dvb_i2c_SetRegisterBits (pDemod->pI2c, i2cAddressActive, 0x70,
                                     (uint8_t) (value ? 0x08 : 0x00), 0x08) != SONY_DVB_OK)
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        break;

    case DEMOD_CONFIG_TSVALID_ACTIVE_HI:
        /* OREG_TSIF_TSVALID_ACTIVE_HI */
        /* Set bank 0x00 */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, i2cAddressActive, 0x00, 0x00) != SONY_DVB_OK)
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        /* Set OREG_TSIF_TSVALID_ACTIVE_HI (bit 7) */
        if (dvb_i2c_SetRegisterBits (pDemod->pI2c, i2cAddressActive, 0x71,
                                     (uint8_t) (value ? 0x80 : 0x00), 0x80) != SONY_DVB_OK)
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        break;

    case DEMOD_CONFIG_TSSYNC_ACTIVE_HI:
        /* OREG_TSIF_TSSYNC_ACTIVE_HI */
        /* Set bank 0x00 */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, i2cAddressActive, 0x00, 0x00) != SONY_DVB_OK)
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        /* Set OREG_TSIF_TSSYNC_ACTIVE_HI (bit 6) */
        if (dvb_i2c_SetRegisterBits (pDemod->pI2c, i2cAddressActive, 0x71,
                                     (uint8_t) (value ? 0x40 : 0x00), 0x40) != SONY_DVB_OK)
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        break;

    case DEMOD_CONFIG_TSERR_ACTIVE_HI:
        /* OREG_TSIF_TSERR_ACTIVE_HI */
        /* Set bank 0x00 */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, i2cAddressActive, 0x00, 0x00) != SONY_DVB_OK)
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        /* Set OREG_TSIF_TSERR_ACTIVE_HI (bit 5) */
        if (dvb_i2c_SetRegisterBits (pDemod->pI2c, i2cAddressActive, 0x71,
                                     (uint8_t) (value ? 0x20 : 0x00), 0x20) != SONY_DVB_OK)
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        break;

    case DEMOD_CONFIG_LATCH_ON_POSEDGE:
        /* OREG_TSIF_LATCH_ON_POSEDGE (TS Clock invert) */
        /* This register can change only in SLEEP state */
        if (pDemod->state != SONY_DVB_DEMOD_STATE_SLEEP)
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
        /* Set bank 0x00 */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x00, 0x00) != SONY_DVB_OK)
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        /* Set OREG_TSIF_LATCH_ON_POSEDGE (bit 4) */
        if (dvb_i2c_SetRegisterBits (pDemod->pI2c, pDemod->i2cAddressT, 0x71,
                                     (uint8_t) (value ? 0x10 : 0x00), 0x10) != SONY_DVB_OK)
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        break;

    case DEMOD_CONFIG_TSCLK_CONT:
        /* OREG_TSIF_TSCLK_CONT (TS clock continuous/gated) */
        /* Set bank 0x00 */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, i2cAddressActive, 0x00, 0x00) != SONY_DVB_OK)
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        /* Set OREG_TSIF_TSCLK_CONT (bit 7) */
        if (dvb_i2c_SetRegisterBits (pDemod->pI2c, i2cAddressActive, 0x70,
                                     (uint8_t) (value ? 0x80 : 0x00), 0x80) != SONY_DVB_OK)
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        break;

    case DEMOD_CONFIG_IFAGCNEG:
        /* OCTL_IFAGCNEG */
        /* Set bank 0x00 */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x00, 0x00) != SONY_DVB_OK)
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        /* Set OCTL_IFAGCNEG (bit 6) */
        if (dvb_i2c_SetRegisterBits (pDemod->pI2c, pDemod->i2cAddressT, 0xCB,
                                     (uint8_t) (value ? 0x40 : 0x00), 0x40) != SONY_DVB_OK)
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        /* OREG_INVERTXGC */
        /* Set bank 0x00 */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressC, 0x00, 0x00) != SONY_DVB_OK)
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        /* Set OREG_INVERTXGC (bit 7) */
        if (dvb_i2c_SetRegisterBits (pDemod->pI2c, pDemod->i2cAddressC, 0x1F,
                                     (uint8_t) (value ? 0x80 : 0x00), 0x80) != SONY_DVB_OK)
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        break;

    case DEMOD_CONFIG_SPECTRUM_INV:
        /* Spectrum Inversion */
        
        /* Store the configured sense. */
        pDemod->confSense = value ? DVB_DEMOD_SPECTRUM_INV : DVB_DEMOD_SPECTRUM_NORMAL ;

        /* Set bank 0x00 */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x00, 0x00) != SONY_DVB_OK)
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        /* Set OREG_DNCNV_SRVS (bit 4) */
        if (dvb_i2c_SetRegisterBits (pDemod->pI2c, pDemod->i2cAddressT, 0xB5,
                                     (uint8_t) (value ? 0x10 : 0x00), 0x10) != SONY_DVB_OK)
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);

        /* Set bank 0x00 */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressC, 0x00, 0x00) != SONY_DVB_OK)
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        /* Set OREG_INV_SPECTRUM (bit 1) (0: Invert, 1: Not invert) */
        /* NOTE: DVB-C spectrum inversion setup/monitor register logic is opposite to DVB-T. */
        if (dvb_i2c_SetRegisterBits (pDemod->pI2c, pDemod->i2cAddressC, 0x21,
                                     (uint8_t) (value ? 0x00 : 0x02), 0x02) != SONY_DVB_OK)
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        break;

    case DEMOD_CONFIG_RFLVMON_ENABLE:
        /* RF level monitor enable/disable */
        /* This setting can change only in SLEEP state */
        if (pDemod->state != SONY_DVB_DEMOD_STATE_SLEEP)
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
        pDemod->enable_rflvmon = value ? 1 : 0;
        break;

    case DEMOD_CONFIG_RFAGC_ENABLE:
        /* RFAGC PWM output enable/disable. */
        /* This setting can change only in SLEEP state */
        if (pDemod->state != SONY_DVB_DEMOD_STATE_SLEEP) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
        }

        /* Set bank 0 */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, CXD2820R_B0_REGC_BANK, CXD2820R_B0) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
        
        /* Update IO port Hi-Z setting. */
        pDemod->ioHiZ = value ? pDemod->ioHiZ & ~0x08 : pDemod->ioHiZ | 0x08 ;
        if (value) {
            /* Enable RFAGC PWM function. */
            if (dvb_i2c_SetRegisterBits (pDemod->pI2c, pDemod->i2cAddressT, 0x89, 0x10, 0x30) != SONY_DVB_OK) {
                SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
            }
        }

        /* Enable RFAGC PWM output. */
        if (dvb_i2c_SetRegisterBits (pDemod->pI2c, pDemod->i2cAddressT, 0xBD, value ? 0x00 : 0x02, 0x02) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
                    
        /* Set bank 1 */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, CXD2820R_B0_REGC_BANK, 0x01) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
        /* DVB-C: Manual RFAGC PWM control is default. */
        /* DVB-T: Manual RFAGC PWM control (OCTL_RFAGCMANUAL) */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x39, value ? 0x01 : 0x00) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
        break ;

    case DEMOD_CONFIG_SCR_ENABLE:
        /* OREG_TSIF_SCR_ENABLE (TS smoothing ON/OFF. OFF is not recommended!) */
        /* This register can change only in SLEEP state */
        if (pDemod->state != SONY_DVB_DEMOD_STATE_SLEEP)
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
        /* Set bank 0x00 */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x00, 0x00) != SONY_DVB_OK)
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        /* Set OREG_TSIF_SCR_ENABLE (bit 0) */
        if (dvb_i2c_SetRegisterBits (pDemod->pI2c, pDemod->i2cAddressT, 0xD7,
                                     (uint8_t) (value ? 0x01 : 0x00), 0x01) != SONY_DVB_OK)
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        break;

    case DEMOD_CONFIG_TSIF_SDPR:
        /* Argument checking. */
        if ((value <= 0) || (value > SONY_CXD2820_MAX_TS_CLK_KHZ)) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_RANGE);
        }

        {
            uint32_t val = 0x00 ;
            uint8_t data[2] ;
            uint32_t uvalue = (uint32_t) value ;

            /* Set Bank 0x00. */
            if (pDemod->pI2c->WriteOneRegister(pDemod->pI2c, i2cAddressActive, CXD2820R_B0_REGC_BANK, CXD2820R_B0) != SONY_DVB_OK) {
                SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C) ;
            }
        
            /* 
                OREG_SP_WR_SMOOTH_DP = (41(MHz) / TSCLK(MHz)) * 2^8 ; 
                Notes: 41000 * 100 * (2^8) = 1049600000
            */
            val = (1049600000u + uvalue/ 2) / uvalue ;
            val = (val + 50) / 100 ;
            data[0] = (uint8_t) ((val & 0xFF00) >> 8) ;
            data[1] = (uint8_t) (val & 0xFF) ;

            if (pDemod->pI2c->WriteRegister(pDemod->pI2c, i2cAddressActive, 0xE4, data, sizeof(data)) != SONY_DVB_OK) {
                SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C) ;
            }
        }
        break;

    case DEMOD_CONFIG_TS_AUTO_RATE_ENABLE:
        /* Set bank 0x00 */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, i2cAddressActive, 0x00, 0x00) != SONY_DVB_OK)
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        
        /* Set OREG_TSIF_SCR_DP_AUTO */
        if (dvb_i2c_SetRegisterBits(pDemod->pI2c, i2cAddressActive, 0xD7, value ? 0x02 : 0x00, 0x02) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C) ;
        }
        break;

        /* ============================= DVB-T ================================== */
    case DEMOD_CONFIG_DVBT_LPSELECT:
        /* OREG_LPSELECT (Hierarchical Modulation select) */
        /* Set bank 0x04 */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x00, 0x04) != SONY_DVB_OK)
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        /* Set OREG_LPSELECT (bit 0) */
        if (dvb_i2c_SetRegisterBits (pDemod->pI2c, pDemod->i2cAddressT, 0x10,
                                     (uint8_t) ((value == (int32_t)DVBT_PROFILE_LP) ? 0x01 : 0x00), 0x01) != SONY_DVB_OK)
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        /* Set bank 0x00 */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x00, 0x00) != SONY_DVB_OK)
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        break;

    case DEMOD_CONFIG_DVBT_FORCE_MODEGI:
        /* OREG_FORCE_MODEGI (Mode/GI estimation enable (Auto or Manual)) */
        /* Set bank 0x00 */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x00, 0x00) != SONY_DVB_OK)
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        if (value) {
            /* Enable : Set OREG_FORCE_MODEGI (bit 4) */
            if (dvb_i2c_SetRegisterBits (pDemod->pI2c, pDemod->i2cAddressT, 0xD3, 0x10, 0x10) != SONY_DVB_OK)
                SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
        else {
            /* Disable -> OREG_MODE/OREG_GI must be zero! */
            if (dvb_i2c_SetRegisterBits (pDemod->pI2c, pDemod->i2cAddressT, 0xD3, 0x00, 0x1F) != SONY_DVB_OK)
                SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
        break;

    case DEMOD_CONFIG_DVBT_MODE:
        /* OREG_MODE (For manaul Mode/GI setting) */
        /* Set bank 0x00 */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x00, 0x00) != SONY_DVB_OK)
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        /* Set OREG_MODE (bit 3:2) */
        if (dvb_i2c_SetRegisterBits (pDemod->pI2c, pDemod->i2cAddressT, 0xD3,
                                     (uint8_t) (((uint32_t) (value) & 0x03) << 2), 0x0C) != SONY_DVB_OK)
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        break;

    case DEMOD_CONFIG_DVBT_GI:
        /* OREG_GI (For manual Mode/GI setting) */
        /* Set bank 0x00 */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x00, 0x00) != SONY_DVB_OK)
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        /* Set OREG_GI (bit 1:0) */
        if (dvb_i2c_SetRegisterBits (pDemod->pI2c, pDemod->i2cAddressT, 0xD3,
                                     (uint8_t) ((uint32_t) (value) & 0x03), 0x03) != SONY_DVB_OK)
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        break;

    case DEMOD_CONFIG_DVBT_SCANMODE:
        pDemod->dvbt_scannmode = value ? 1 : 0;
        break;

        /* ============================= DVB-C ================================== */
    case DEMOD_CONFIG_DVBC_SCANMODE:
        /* OREG_AR_SCANNING (Scan mode select) */
        /* Set bank 0x00 */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressC, 0x00, 0x00) != SONY_DVB_OK)
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        /* Set OREG_AR_SCANNING (bit 5) */
        if (dvb_i2c_SetRegisterBits (pDemod->pI2c, pDemod->i2cAddressC, 0x86,
                                     (uint8_t) (value ? 0x20 : 0x00), 0x20) != SONY_DVB_OK)
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        break;
    
        /* ============================= DVB-T2 ================================== */

    case DEMOD_CONFIG_DVBT2_PP2_LONG_ECHO:
        /* Handled in t2_SetConfig. */
        break ;
        /* ============================= All Modes =============================== */
    case DEMOD_CONFIG_SHARED_IF:
        pDemod->sharedIf = value ? 0x01 : 0x00 ;
        break;
    default:
        /* Unknown ID */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* Shadow settings for T2 (CXD2820 only). */
    if (pDemod->chipId != SONY_DVB_CXD2817) {
        result = t2_SetConfig (pDemod, configId, value);
    }

    SONY_DVB_TRACE_RETURN (result);
}

/*------------------------------------------------------------------------------
  Implementation of static functions
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
  CXD2817R specific functions
------------------------------------------------------------------------------*/

static sony_dvb_result_t SD2SL (sony_dvb_demod_t * pDemod)
{
    SONY_DVB_TRACE_ENTER ("SD2SL");

    if (!pDemod)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);

    /* Set bank 0 */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x00, 0x00) != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);

    /* Set pll_rst_cnt (Reset Release | PLL Active) */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x7E, 0x02) != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);

    /* Wait */
    SONY_DVB_SLEEP (10);

    /* Set clkreg_ck_a_en (Always Clock Enable) */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x7F, 0x01) != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);

    SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
}

static sony_dvb_result_t SL2SD (sony_dvb_demod_t * pDemod)
{
    SONY_DVB_TRACE_ENTER ("SL2SD");

    if (!pDemod)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);

    /* Set bank 0 */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x00, 0x00) != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);

    /* Set clkreg_ck_a_en (Always Clock Disable) */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x7F, 0x00) != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);

    /* Set pll_rst_cnt (Reset Release | PLL PowerDown) */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x7E, 0x03) != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);

    SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
}

/*------------------------------------------------------------------------------
  Common functions
------------------------------------------------------------------------------*/

static sony_dvb_result_t SL2NOT (sony_dvb_demod_t * pDemod)
{
    sony_dvb_result_t result = SONY_DVB_OK ;
    SONY_DVB_TRACE_ENTER ("SL2NOT");

    if (!pDemod)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);

    /* Set bank 0 */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x00, 0x00) != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);

    /* Set actreg_dvbmode (DVB-T mode) */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x80, 0x00) != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);

    /* Set clkreg_ck_en *Burst write is forbidden* */
    if (pDemod->enable_rflvmon) {
        /* (OREG_CK_RM_EN | OREG_CK_T_EN | OREG_CK_B_EN) */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x81, 0x13) != SONY_DVB_OK)
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }
    else {
        /* (OREG_CK_T_EN | OREG_CK_B_EN) */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x81, 0x03) != SONY_DVB_OK)
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /* Set adcreg_adc_en (OREG_ADC_AREGEN | OREG_ADC_ADCEN | OREG_ADC_CLKGENEN) */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x85, 0x07) != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);

    /* Set ramreg_ramon_stb (SSM Enable) */
    if (pDemod->enable_rflvmon) {
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x88, 0x02) != SONY_DVB_OK)
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /* Write parameters for DVB-T depends on bandwidth */
    {
        result = SL2NOT_BandSetting (pDemod);
        if (result != SONY_DVB_OK)
            SONY_DVB_TRACE_RETURN (result);
    }

    /* Set bank 0 */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x00, 0x00) != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);

    /* Enable output (o_reg_port_hiz) */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0xFF, pDemod->ioHiZ) != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);

    /* Soft Reset. */
    result = dvb_demod_SoftReset(pDemod);
    if (result != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (result);
    }

    SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
}

static sony_dvb_result_t SL2NOT_BandSetting (sony_dvb_demod_t * pDemod)
{
    SONY_DVB_TRACE_ENTER ("SL2NOT_BandSetting");

    if (!pDemod)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);

    /* Set bank 0 */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x00, 0x00) != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);

    if (pDemod->is_ascot) {
        /* Set itb_gdeq_en (Bit 0) */
        if (dvb_i2c_SetRegisterBits (pDemod->pI2c, pDemod->i2cAddressT, 0xA5, 0x01, 0x01) != SONY_DVB_OK)   /* ON */
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        /* ADC setting (Bit 6:5) */
        if (dvb_i2c_SetRegisterBits (pDemod->pI2c, pDemod->i2cAddressT, 0x82, 0x40, 0x60) != SONY_DVB_OK)
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }
    else {
        /* Set itb_gdeq_en (Bit 0) */
        if (dvb_i2c_SetRegisterBits (pDemod->pI2c, pDemod->i2cAddressT, 0xA5, 0x00, 0x01) != SONY_DVB_OK)   /* OFF */
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        /* ADC setting (Bit 6:5) */
        if (dvb_i2c_SetRegisterBits (pDemod->pI2c, pDemod->i2cAddressT, 0x82, 0x20, 0x60) != SONY_DVB_OK)
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }
    /*
     * <ACI Filter setting (non-Ascot tuners)>
     * slave    bank    addr    bit     default     value      name
     * ----------------------------------------------------------------------------------
     * <SLV-T>   00h     C2h    [7:0]     8'h00      8'h11    2'b00,OREG_CAS_ACIFLT1_EN[1:0],2'b00,OREG_CAS_ACIFLT2_EN[1:0]
     */
    if(!pDemod->is_ascot) {
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0xC2, 0x11) != SONY_DVB_OK)
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /*
     * <BBAGC TARGET level setting>
     * slave    bank    addr    bit     default     value      name
     * ----------------------------------------------------------------------------------
     * <SLV-T>   01h     6Ah    [7:0]     8'h50      8'h50      OREG_ITB_DAGC_TRGT[7:0]
     */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x00, 0x01) != SONY_DVB_OK)  /* Bank 01 */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x6A, 0x50) != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    
    /*
     * <SW reset without IFAGC reset>
     * slave    bank    addr    bit     default     value      name
     * ----------------------------------------------------------------------------------
     * <SLV-T>   01h     48h    [4]       8'h20      8'h30      OCTL_AGC_NO_SWRST
     */
    {
        uint8_t data = pDemod->is_ascot ? 0x10 : 0x00 ;

        if (dvb_i2c_SetRegisterBits (pDemod->pI2c, pDemod->i2cAddressT, 0x48, data, 0x10) != SONY_DVB_OK)
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /*
     * <Pre RS BER monitor resolution setting>
     * slave    bank    addr    bit     default     value      name
     * ------------------------------------------------------------------------------
     * <SLV-T>   04h     27h    [6:0]     7'h09     7'h41     OREG_BER_ERRPKT_BITECNT[6:0]
     */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x00, 0x04) != SONY_DVB_OK)  /* Bank 04 */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x27, 0x41) != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
            
    /* Bank 0x00 */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x00, 0x00) != SONY_DVB_OK)  /* Bank 00 */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);


    switch (pDemod->bandWidth) {
    case 6:
        /*
         * <ACI Filter setting(for ASCOT)>
         * slave    bank    addr    bit     default     value      name
         * ----------------------------------------------------------------------------------
         * <SLV-T>   00h     C2h    [7:0]     8'h00      8'h13    2'b00,OREG_CAS_ACIFLT1_EN[1:0],2'b00,OREG_CAS_ACIFLT2_EN[1:0]
         */
        if(pDemod->is_ascot) {
            if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0xC2, 0x13) != SONY_DVB_OK)
                SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
        /*
         * <Timing Recovery setting>
         * slave    bank    addr    bit     default     value      name
         * ----------------------------------------------------------------------------------
         * <SLV-T>   00h     9Fh    [5:0]     6'h11      8'h17      OREG_TRCG_NOMINALRATE[37:32]
         * <SLV-T>   00h     A0h    [7:0]     8'hF0      8'hEA      OREG_TRCG_NOMINALRATE[31:24]
         * <SLV-T>   00h     A1h    [7:0]     8'h00      8'hAA      OREG_TRCG_NOMINALRATE[23:16]
         * <SLV-T>   00h     A2h    [7:0]     8'h00      8'hAA      OREG_TRCG_NOMINALRATE[15:8]
         * <SLV-T>   00h     A3h    [7:0]     8'h00      8'hAA      OREG_TRCG_NOMINALRATE[7:0]
         */
        {
            const uint8_t data[] = { 0x17, 0xEA, 0xAA, 0xAA, 0xAA };
            if (pDemod->pI2c->WriteRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x9F, data, sizeof (data)) != SONY_DVB_OK)
                SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
        /*
         * <ASCOT group delay eq setting (for ASCOT)>
         * slave    bank    addr    bit     default     value      name
         * ----------------------------------------------------------------------------------
         * <SLV-T>   00h     A6h    [7:0]     8'h1E      8'h19      OREG_ITB_COEF01[7:0]
         * <SLV-T>   00h     A7h    [7:0]     8'h1D      8'h24      OREG_ITB_COEF02[7:0]
         * <SLV-T>   00h     A8h    [7:0]     8'h29      8'h2B      OREG_ITB_COEF11[7:0]
         * <SLV-T>   00h     A9h    [7:0]     8'hC9      8'hB7      OREG_ITB_COEF12[7:0]
         * <SLV-T>   00h     AAh    [7:0]     8'h2A      8'h2C      OREG_ITB_COEF21[7:0]
         * <SLV-T>   00h     ABh    [7:0]     8'hBA      8'hAC      OREG_ITB_COEF22[7:0]
         * <SLV-T>   00h     ACh    [7:0]     8'h29      8'h29      OREG_ITB_COEF31[7:0]
         * <SLV-T>   00h     ADh    [7:0]     8'hAD      8'hA6      OREG_ITB_COEF32[7:0]
         * <SLV-T>   00h     AEh    [7:0]     8'h29      8'h2A      OREG_ITB_COEF41[7:0]
         * <SLV-T>   00h     AFh    [7:0]     8'hA4      8'h9F      OREG_ITB_COEF42[7:0]
         * <SLV-T>   00h     B0h    [7:0]     8'h29      8'h2A      OREG_ITB_COEF51[7:0]
         * <SLV-T>   00h     B1h    [7:0]     8'h9A      8'h99      OREG_ITB_COEF52[7:0]
         * <SLV-T>   00h     B2h    [7:0]     8'h28      8'h2A      OREG_ITB_COEF61[7:0]
         * <SLV-T>   00h     B3h    [7:0]     8'h9E      8'h9B      OREG_ITB_COEF62[7:0]
         */
        if (pDemod->is_ascot) {
            const uint8_t data[] = { 0x19, 0x24, 0x2B, 0xB7, 0x2C, 0xAC, 0x29, 0xA6, 0x2A, 0x9F, 0x2A, 0x99, 0x2A, 0x9B };
            if (pDemod->pI2c->WriteRegister (pDemod->pI2c, pDemod->i2cAddressT, 0xA6, data, sizeof (data)) != SONY_DVB_OK)
                SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
        /*
         * <IF freq setting>
         * slave    bank    addr    bit     default     value      name
         * ---------------------------------------------------------------------------------
         * <SLV-T>   00h     B6h    [7:0]     8'h1F   user define   OREG_DNCNV_LOFRQ[23:16]
         * <SLV-T>   00h     B7h    [7:0]     8'h38   user define   OREG_DNCNV_LOFRQ[15:8]
         * <SLV-T>   00h     B8h    [7:0]     8'h32   user define   OREG_DNCNV_LOFRQ[7:0]
         */
        {
            uint8_t data[3];
            data[0] = (uint8_t) ((pDemod->iffreq_config.config_DVBT6 >> 16) & 0xFF);
            data[1] = (uint8_t) ((pDemod->iffreq_config.config_DVBT6 >> 8) & 0xFF);
            data[2] = (uint8_t) (pDemod->iffreq_config.config_DVBT6 & 0xFF);
            if (pDemod->pI2c->WriteRegister (pDemod->pI2c, pDemod->i2cAddressT, 0xB6, data, sizeof (data)) != SONY_DVB_OK)
                SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        /*
         * <TSIF setting>
         * slave    bank    addr    bit     default     value      name
         * ----------------------------------------------------------------------------------
         * <SLV-T>   00h     D7h    [7:6]     2'b00      2'b10      OREG_CHANNEL_WIDTH[1:0]
         */
        if (dvb_i2c_SetRegisterBits (pDemod->pI2c, pDemod->i2cAddressT, 0xD7, 0x80, 0xC0) != SONY_DVB_OK)
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        /*
         * <Demod internal delay setting>
         * slave    bank    addr    bit     default     value      name
         * ----------------------------------------------------------------------------------
         * <SLV-T>   00h     D9h    [5:0]     6'h01      6'h1F      OREG_CDRB_GTDOFST[13:8]
         * <SLV-T>   00h     DAh    [7:0]     8'hE0      8'hDC      OREG_CDRB_GTDOFST[7:0]
         */
        {
            const uint8_t data[] = { 0x1F, 0xDC };
            if (pDemod->pI2c->WriteRegister (pDemod->pI2c, pDemod->i2cAddressT, 0xD9, data, sizeof (data)) != SONY_DVB_OK)
                SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        if (pDemod->is_ascot) {
            
            /*
             * <Notch filter setting(for ASCOT)>
             * slave    Bank    Addr    Bit    default    Value          Name
             * ---------------------------------------------------------------------------------
             * <SLV-T>   07h     38h    [1:0]    8'h00      8'h00      OREG_CAS_CCIFLT2_EN_CW2[1:0]
             * <SLV-T>   07h     39h    [1:0]    8'h03      8'h02      OREG_CAS_CWSEQ_ON,OREG_CAS_CWSEQ_ON2
             * <SLV-T>   07h     3Ch    [3:0]    8'h05      8'h05      OREG_CAS_CCIFLT2_MU_CW[3:0]
             * <SLV-T>   07h     3Dh    [3:0]    8'h05      8'h05      OREG_CAS_CCIFLT2_MU_CW2[3:0]
             * <SLV-T>   07h     44h    [7:0]    8'h00      8'h91      OREG_CAS_CCIFLT2_FREQ_CW2[15:8]
             * <SLV-T>   07h     45h    [7:0]    8'h00      8'hA0      OREG_CAS_CCIFLT2_FREQ_CW2[7:0]
             */
            if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x00, 0x07) != SONY_DVB_OK)  /* Bank 07 */
                SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
            {
                const uint8_t data[] = { 0x00, 0x02 } ;
                if (pDemod->pI2c->WriteRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x38, data, sizeof(data)) != SONY_DVB_OK)
                    SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
            }
            {
                const uint8_t data[] = { 0x05, 0x05 } ;
                if (pDemod->pI2c->WriteRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x3C, data, sizeof(data)) != SONY_DVB_OK)
                    SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
            }
            {
                const uint8_t data[] = { 0x91, 0xA0 } ;
                if (pDemod->pI2c->WriteRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x44, data, sizeof(data)) != SONY_DVB_OK)
                    SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
            }
        }
        break;

    case 7:
        /*
         * <ACI Filter setting(for ASCOT)>
         * slave    bank    addr    bit     default     value      name
         * ----------------------------------------------------------------------------------
         * <SLV-T>   00h     C2h    [7:0]     8'h00      8'h13    2'b00,OREG_CAS_ACIFLT1_EN[1:0],2'b00,OREG_CAS_ACIFLT2_EN[1:0]
         */
        if(pDemod->is_ascot) {
            if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0xC2, 0x13) != SONY_DVB_OK)
                SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        /*
         * <Timing Recovery setting>
         * slave    bank    addr    bit     default     value      name
         * ----------------------------------------------------------------------------------
         * <SLV-T>   00h     9Fh    [5:0]     6'h11      8'h14      OREG_TRCG_NOMINALRATE[37:32]
         * <SLV-T>   00h     A0h    [7:0]     8'hF0      8'h80      OREG_TRCG_NOMINALRATE[31:24]
         * <SLV-T>   00h     A1h    [7:0]     8'h00      8'h00      OREG_TRCG_NOMINALRATE[23:16]
         * <SLV-T>   00h     A2h    [7:0]     8'h00      8'h00      OREG_TRCG_NOMINALRATE[15:8]
         * <SLV-T>   00h     A3h    [7:0]     8'h00      8'h00      OREG_TRCG_NOMINALRATE[7:0]
         */
        {
            const uint8_t data[] = { 0x14, 0x80, 0x00, 0x00, 0x00 };
            if (pDemod->pI2c->WriteRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x9F, data, sizeof (data)) != SONY_DVB_OK)
                SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
        /*
         * <ASCOT group delay eq setting (for ASCOT)>
         * slave    bank    addr    bit     default     value      name
         * ----------------------------------------------------------------------------------
         * <SLV-T>   00h     A6h    [7:0]     8'h1E      8'h1B      OREG_ITB_COEF01[7:0]
         * <SLV-T>   00h     A7h    [7:0]     8'h1D      8'h22      OREG_ITB_COEF02[7:0]
         * <SLV-T>   00h     A8h    [7:0]     8'h29      8'h2B      OREG_ITB_COEF11[7:0]
         * <SLV-T>   00h     A9h    [7:0]     8'hC9      8'hC1      OREG_ITB_COEF12[7:0]
         * <SLV-T>   00h     AAh    [7:0]     8'h2A      8'h2C      OREG_ITB_COEF21[7:0]
         * <SLV-T>   00h     ABh    [7:0]     8'hBA      8'hB3      OREG_ITB_COEF22[7:0]
         * <SLV-T>   00h     ACh    [7:0]     8'h29      8'h2B      OREG_ITB_COEF31[7:0]
         * <SLV-T>   00h     ADh    [7:0]     8'hAD      8'hA9      OREG_ITB_COEF32[7:0]
         * <SLV-T>   00h     AEh    [7:0]     8'h29      8'h2B      OREG_ITB_COEF41[7:0]
         * <SLV-T>   00h     AFh    [7:0]     8'hA4      8'hA0      OREG_ITB_COEF42[7:0]
         * <SLV-T>   00h     B0h    [7:0]     8'h29      8'h2B      OREG_ITB_COEF51[7:0]
         * <SLV-T>   00h     B1h    [7:0]     8'h9A      8'h97      OREG_ITB_COEF52[7:0]
         * <SLV-T>   00h     B2h    [7:0]     8'h28      8'h2B      OREG_ITB_COEF61[7:0]
         * <SLV-T>   00h     B3h    [7:0]     8'h9E      8'h9B      OREG_ITB_COEF62[7:0]
         */
        if (pDemod->is_ascot) {
            const uint8_t data[] = { 0x1B, 0x22, 0x2B, 0xC1, 0x2C, 0xB3, 0x2B, 0xA9, 0x2B, 0xA0, 0x2B, 0x97, 0x2B, 0x9B };
            if (pDemod->pI2c->WriteRegister (pDemod->pI2c, pDemod->i2cAddressT, 0xA6, data, sizeof (data)) != SONY_DVB_OK)
                SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
        /*
         * <IF freq setting>
         * slave    bank    addr    bit     default     value      name
         * ---------------------------------------------------------------------------------
         * <SLV-T>   00h     B6h    [7:0]     8'h1F   user define   OREG_DNCNV_LOFRQ[23:16]
         * <SLV-T>   00h     B7h    [7:0]     8'h38   user define   OREG_DNCNV_LOFRQ[15:8]
         * <SLV-T>   00h     B8h    [7:0]     8'h32   user define   OREG_DNCNV_LOFRQ[7:0]
         */
        {
            uint8_t data[3];
            data[0] = (uint8_t) ((pDemod->iffreq_config.config_DVBT7 >> 16) & 0xFF);
            data[1] = (uint8_t) ((pDemod->iffreq_config.config_DVBT7 >> 8) & 0xFF);
            data[2] = (uint8_t) (pDemod->iffreq_config.config_DVBT7 & 0xFF);
            if (pDemod->pI2c->WriteRegister (pDemod->pI2c, pDemod->i2cAddressT, 0xB6, data, sizeof (data)) != SONY_DVB_OK)
                SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        /*
         * <TSIF setting>
         * slave    bank    addr    bit     default     value      name
         * ----------------------------------------------------------------------------------
         * <SLV-T>   00h     D7h    [7:6]     2'b00      2'b01      OREG_CHANNEL_WIDTH[1:0]
         */
        if (dvb_i2c_SetRegisterBits (pDemod->pI2c, pDemod->i2cAddressT, 0xD7, 0x40, 0xC0) != SONY_DVB_OK)
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        /*
         * <Demod internal delay setting>
         * slave    bank    addr    bit     default     value      name
         * ----------------------------------------------------------------------------------
         * <SLV-T>   00h     D9h    [5:0]     6'h01      6'h12      OREG_CDRB_GTDOFST[13:8]
         * <SLV-T>   00h     DAh    [7:0]     8'hE0      8'hF8      OREG_CDRB_GTDOFST[7:0]
         */
        {
            const uint8_t data[] = { 0x12, 0xF8 };
            if (pDemod->pI2c->WriteRegister (pDemod->pI2c, pDemod->i2cAddressT, 0xD9, data, sizeof (data)) != SONY_DVB_OK)
                SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        if (pDemod->is_ascot) {
            
            /*
             * <Notch filter setting(for ASCOT)>
             * slave    Bank    Addr    Bit    default    Value          Name
             * ---------------------------------------------------------------------------------
             * <SLV-T>   07h     38h    [1:0]    8'h00      8'h01      OREG_CAS_CCIFLT2_EN_CW2[1:0]
             * <SLV-T>   07h     39h    [1:0]    8'h03      8'h02      OREG_CAS_CWSEQ_ON,OREG_CAS_CWSEQ_ON2
             * <SLV-T>   07h     3Ch    [3:0]    8'h05      8'h03      OREG_CAS_CCIFLT2_MU_CW[3:0]
             * <SLV-T>   07h     3Dh    [3:0]    8'h05      8'h01      OREG_CAS_CCIFLT2_MU_CW2[3:0]
             * <SLV-T>   07h     44h    [7:0]    8'h00      8'h97      OREG_CAS_CCIFLT2_FREQ_CW2[15:8]
             * <SLV-T>   07h     45h    [7:0]    8'h00      8'h00      OREG_CAS_CCIFLT2_FREQ_CW2[7:0]
             */
            if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x00, 0x07) != SONY_DVB_OK)  /* Bank 07 */
                SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
            {
                const uint8_t data[] = { 0x01, 0x02 } ;
                if (pDemod->pI2c->WriteRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x38, data, sizeof(data)) != SONY_DVB_OK)
                    SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
            }
            {
                const uint8_t data[] = { 0x03, 0x01 } ;
                if (pDemod->pI2c->WriteRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x3C, data, sizeof(data)) != SONY_DVB_OK)
                    SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
            }
            {
                const uint8_t data[] = { 0x97, 0x00 } ;
                if (pDemod->pI2c->WriteRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x44, data, sizeof(data)) != SONY_DVB_OK)
                    SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
            }
        }

        break;

    case 8:
        /*
         * <ACI Filter setting(for ASCOT 8MHz)>
         * slave    bank    addr    bit     default     value      name
         * ----------------------------------------------------------------------------------
         * <SLV-T>   00h     C2h    [7:0]     8'h00      8'h11    2'b00,OREG_CAS_ACIFLT1_EN[1:0],2'b00,OREG_CAS_ACIFLT2_EN[1:0]
         */
        if(pDemod->is_ascot) {
            if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0xC2, 0x11) != SONY_DVB_OK)
                SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
        /*
         * <Timing Recovery setting>
         * slave    bank    addr    bit     default     value      name
         * ----------------------------------------------------------------------------------
         * <SLV-T>   00h     9Fh    [5:0]     6'h11      8'h11      OREG_TRCG_NOMINALRATE[37:32]
         * <SLV-T>   00h     A0h    [7:0]     8'hF0      8'hF0      OREG_TRCG_NOMINALRATE[31:24]
         * <SLV-T>   00h     A1h    [7:0]     8'h00      8'h00      OREG_TRCG_NOMINALRATE[23:16]
         * <SLV-T>   00h     A2h    [7:0]     8'h00      8'h00      OREG_TRCG_NOMINALRATE[15:8]
         * <SLV-T>   00h     A3h    [7:0]     8'h00      8'h00      OREG_TRCG_NOMINALRATE[7:0]
         */
        {
            const uint8_t data[] = { 0x11, 0xF0, 0x00, 0x00, 0x00 };
            if (pDemod->pI2c->WriteRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x9F, data, sizeof (data)) != SONY_DVB_OK)
                SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
        /*
         * <ASCOT group delay eq setting (for ASCOT)>
         * slave    bank    addr    bit     default     value      name
         * ----------------------------------------------------------------------------------
         * <SLV-T>   00h     A6h    [7:0]     8'h1E      8'h1E      OREG_ITB_COEF01[7:0]
         * <SLV-T>   00h     A7h    [7:0]     8'h1D      8'h1D      OREG_ITB_COEF02[7:0]
         * <SLV-T>   00h     A8h    [7:0]     8'h29      8'h29      OREG_ITB_COEF11[7:0]
         * <SLV-T>   00h     A9h    [7:0]     8'hC9      8'hC9      OREG_ITB_COEF12[7:0]
         * <SLV-T>   00h     AAh    [7:0]     8'h2A      8'h2A      OREG_ITB_COEF21[7:0]
         * <SLV-T>   00h     ABh    [7:0]     8'hBA      8'hBA      OREG_ITB_COEF22[7:0]
         * <SLV-T>   00h     ACh    [7:0]     8'h29      8'h29      OREG_ITB_COEF31[7:0]
         * <SLV-T>   00h     ADh    [7:0]     8'hAD      8'hAD      OREG_ITB_COEF32[7:0]
         * <SLV-T>   00h     AEh    [7:0]     8'h29      8'h29      OREG_ITB_COEF41[7:0]
         * <SLV-T>   00h     AFh    [7:0]     8'hA4      8'hA4      OREG_ITB_COEF42[7:0]
         * <SLV-T>   00h     B0h    [7:0]     8'h29      8'h29      OREG_ITB_COEF51[7:0]
         * <SLV-T>   00h     B1h    [7:0]     8'h9A      8'h9A      OREG_ITB_COEF52[7:0]
         * <SLV-T>   00h     B2h    [7:0]     8'h28      8'h28      OREG_ITB_COEF61[7:0]
         * <SLV-T>   00h     B3h    [7:0]     8'h9E      8'h9E      OREG_ITB_COEF62[7:0]
         */
        if (pDemod->is_ascot) {
            const uint8_t data[] = { 0x1E, 0x1D, 0x29, 0xC9, 0x2A, 0xBA, 0x29, 0xAD, 0x29, 0xA4, 0x29, 0x9A, 0x28, 0x9E };
            if (pDemod->pI2c->WriteRegister (pDemod->pI2c, pDemod->i2cAddressT, 0xA6, data, sizeof (data)) != SONY_DVB_OK)
                SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
        /*
         * <IF freq setting>
         * slave    bank    addr    bit     default     value      name
         * ---------------------------------------------------------------------------------
         * <SLV-T>   00h     B6h    [7:0]     8'h1F   user define   OREG_DNCNV_LOFRQ[23:16]
         * <SLV-T>   00h     B7h    [7:0]     8'h38   user define   OREG_DNCNV_LOFRQ[15:8]
         * <SLV-T>   00h     B8h    [7:0]     8'h32   user define   OREG_DNCNV_LOFRQ[7:0]
         */
        {
            uint8_t data[3];
            data[0] = (uint8_t) ((pDemod->iffreq_config.config_DVBT8 >> 16) & 0xFF);
            data[1] = (uint8_t) ((pDemod->iffreq_config.config_DVBT8 >> 8) & 0xFF);
            data[2] = (uint8_t) (pDemod->iffreq_config.config_DVBT8 & 0xFF);
            if (pDemod->pI2c->WriteRegister (pDemod->pI2c, pDemod->i2cAddressT, 0xB6, data, sizeof (data)) != SONY_DVB_OK)
                SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        /*
         * <TSIF setting>
         * slave    bank    addr    bit     default     value      name
         * ----------------------------------------------------------------------------------
         * <SLV-T>   00h     D7h    [7:6]     2'b00      2'b00      OREG_CHANNEL_WIDTH[1:0]
         */
        if (dvb_i2c_SetRegisterBits (pDemod->pI2c, pDemod->i2cAddressT, 0xD7, 0x00, 0xC0) != SONY_DVB_OK)
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        /*
         * <Demod internal delay setting>
         * slave    bank    addr    bit     default     value      name
         * ----------------------------------------------------------------------------------
         * <SLV-T>   00h     D9h    [5:0]     6'h01      6'h01      OREG_CDRB_GTDOFST[13:8]
         * <SLV-T>   00h     DAh    [7:0]     8'hE0      8'hE0      OREG_CDRB_GTDOFST[7:0]
         */
        {
            const uint8_t data[] = { 0x01, 0xE0 };
            if (pDemod->pI2c->WriteRegister (pDemod->pI2c, pDemod->i2cAddressT, 0xD9, data, sizeof (data)) != SONY_DVB_OK)
                SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        if (pDemod->is_ascot) {

            /*
             * <Notch filter setting(for ASCOT)>
             * slave    Bank    Addr    Bit    default    Value          Name
             * ---------------------------------------------------------------------------------
             * <SLV-T>   07h     38h    [1:0]    8'h00      8'h01      OREG_CAS_CCIFLT2_EN_CW2[1:0]
             * <SLV-T>   07h     39h    [1:0]    8'h03      8'h02      OREG_CAS_CWSEQ_ON,OREG_CAS_CWSEQ_ON2
             * <SLV-T>   07h     3Ch    [3:0]    8'h05      8'h05      OREG_CAS_CCIFLT2_MU_CW[3:0]
             * <SLV-T>   07h     3Dh    [3:0]    8'h05      8'h05      OREG_CAS_CCIFLT2_MU_CW2[3:0]
             * <SLV-T>   07h     44h    [7:0]    8'h00      8'h91      OREG_CAS_CCIFLT2_FREQ_CW2[15:8]
             * <SLV-T>   07h     45h    [7:0]    8'h00      8'hA0      OREG_CAS_CCIFLT2_FREQ_CW2[7:0]
             */
            if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x00, 0x07) != SONY_DVB_OK)  /* Bank 07 */
                SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
            
            {
                const uint8_t data[] = { 0x01, 0x02 } ;
                if (pDemod->pI2c->WriteRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x38, data, sizeof(data)) != SONY_DVB_OK)
                    SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
            }
            {
                const uint8_t data[] = { 0x05, 0x05 } ;
                if (pDemod->pI2c->WriteRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x3C, data, sizeof(data)) != SONY_DVB_OK)
                    SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
            }
            {
                const uint8_t data[] = { 0x91, 0xA0 } ;
                if (pDemod->pI2c->WriteRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x44, data, sizeof(data)) != SONY_DVB_OK)
                    SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
            }
        }

        break;
    default:
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_OTHER);
    }

    SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
}

static sony_dvb_result_t NOT2SL (sony_dvb_demod_t * pDemod)
{
    
    SONY_DVB_TRACE_ENTER ("NOT2SL");

    if (!pDemod)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);

    /* Set bank 0 */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x00, 0x00) != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);

    /* Disable output (o_reg_port_hiz) */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0xFF, 0x1F) != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);

    /* Set adcreg_adc_en (All disable) */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x85, (pDemod->sharedIf ? 0x06 : 0x00)) != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);

    /* Set ramreg_ramon_stb (SSM Disable) */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x88, 0x01) != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);

    /* Set clkreg_ck_en (Disable) */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x81, 0x00) != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);

    /* Set actreg_dvbmode (DVB-T mode) */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x80, 0x00) != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);

    SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
}

static sony_dvb_result_t SL2NOC (sony_dvb_demod_t * pDemod)
{
    sony_dvb_result_t result = SONY_DVB_OK ;
    SONY_DVB_TRACE_ENTER ("SL2NOC");

    if (!pDemod)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);

    /* Set bank 0 */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x00, 0x00) != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);

    /* Set actreg_dvbmode (DVB-C mode) */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x80, 0x01) != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);

    /* Set clkreg_ck_en *Burst write is forbidden* */
    if (pDemod->enable_rflvmon) {
        /* (OREG_CK_RM_EN | OREG_CK_C_EN | OREG_CK_B_EN) */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x81, 0x15) != SONY_DVB_OK)
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }
    else {
        /* (OREG_CK_C_EN | OREG_CK_B_EN) */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x81, 0x05) != SONY_DVB_OK)
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /* Set adcreg_adc_en (OREG_ADC_AREGEN | OREG_ADC_ADCEN | OREG_ADC_CLKGENEN) */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x85, 0x07) != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);

    /* Set ramreg_ramon_stb (SSM Enable) */
    if (pDemod->enable_rflvmon) {
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x88, 0x02) != SONY_DVB_OK)
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /* DVB-C parameter settings --------------------------------------------- */

    /* Set bank 0 */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressC, 0x00, 0x00) != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);

    if (pDemod->is_ascot) {
        /* Set itb_gdeq_en (Bit 0) */
        if (dvb_i2c_SetRegisterBits (pDemod->pI2c, pDemod->i2cAddressC, 0xA5, 0x01, 0x01) != SONY_DVB_OK)   /* ON */
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        /* ADC setting (To DVB-T/System slave address!) (Bit 6:5) */
        if (dvb_i2c_SetRegisterBits (pDemod->pI2c, pDemod->i2cAddressT, 0x82, 0x40, 0x60) != SONY_DVB_OK)
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }
    else {
        /* Set itb_gdeq_en (Bit 0) */
        if (dvb_i2c_SetRegisterBits (pDemod->pI2c, pDemod->i2cAddressC, 0xA5, 0x00, 0x01) != SONY_DVB_OK)   /* OFF */
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        /* ADC setting (To DVB-T/System slave address!) (Bit 6:5) */
        if (dvb_i2c_SetRegisterBits (pDemod->pI2c, pDemod->i2cAddressT, 0x82, 0x20, 0x60) != SONY_DVB_OK)
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /*
     * <ASCOT group delay eq setting (for ASCOT)>
     * slave    bank    addr    bit     default     value      name
     * ---------------------------------------------------------------------------------
     * <SLV-C>   00h     A6h    [7:0]     8'h1E      8'h27      OREG_ITB_COEF01[7:0]
     * <SLV-C>   00h     A7h    [7:0]     8'h1D      8'hD9      OREG_ITB_COEF02[7:0]
     * <SLV-C>   00h     A8h    [7:0]     8'h29      8'h27      OREG_ITB_COEF11[7:0]
     * <SLV-C>   00h     A9h    [7:0]     8'hC9      8'hC5      OREG_ITB_COEF12[7:0]
     * <SLV-C>   00h     AAh    [7:0]     8'h2A      8'h25      OREG_ITB_COEF21[7:0]
     * <SLV-C>   00h     ABh    [7:0]     8'hBA      8'hBB      OREG_ITB_COEF22[7:0]
     * <SLV-C>   00h     ACh    [7:0]     8'h29      8'h24      OREG_ITB_COEF31[7:0]
     * <SLV-C>   00h     ADh    [7:0]     8'hAD      8'hB1      OREG_ITB_COEF32[7:0]
     * <SLV-C>   00h     AEh    [7:0]     8'h29      8'h24      OREG_ITB_COEF41[7:0]
     * <SLV-C>   00h     AFh    [7:0]     8'hA4      8'hA9      OREG_ITB_COEF42[7:0]
     * <SLV-C>   00h     B0h    [7:0]     8'h29      8'h23      OREG_ITB_COEF51[7:0]
     * <SLV-C>   00h     B1h    [7:0]     8'h9A      8'hA4      OREG_ITB_COEF52[7:0]
     * <SLV-C>   00h     B2h    [7:0]     8'h28      8'h23      OREG_ITB_COEF61[7:0]
     * <SLV-C>   00h     B3h    [7:0]     8'h9E      8'hA2      OREG_ITB_COEF62[7:0]
     */
    if (pDemod->is_ascot) {
        const uint8_t data[] = { 0x27, 0xD9, 0x27, 0xC5, 0x25, 0xBB, 0x24, 0xB1, 0x24, 0xA9, 0x23, 0xA4, 0x23, 0xA2 };
        if (pDemod->pI2c->WriteRegister (pDemod->pI2c, pDemod->i2cAddressC, 0xA6, data, sizeof (data)) != SONY_DVB_OK)
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }
    /*
     * <IF freq setting>
     * slave    bank    addr    bit     default     value      name
     * ---------------------------------------------------------------------------------
     * <SLV-C>   00h     42h    [5:0]     6'h38    user define  OREG_ITB_DWNCONVER_FRQENCY[13:8]
     * <SLV-C>   00h     43h    [7:0]     8'h32    user define  OREG_ITB_DWNCONVER_FRQENCY[7:0]
     */
    {
        uint8_t data[2];
        data[0] = (uint8_t) ((pDemod->iffreq_config.config_DVBC >> 8) & 0x3F);
        data[1] = (uint8_t) (pDemod->iffreq_config.config_DVBC & 0xFF);
        if (pDemod->pI2c->WriteRegister (pDemod->pI2c, pDemod->i2cAddressC, 0x42, data, sizeof (data)) != SONY_DVB_OK)
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /*
     * <IFAGC TARGET level setting (for ASCOT)>
     * slave    bank    addr    bit     default     value      name
     * ----------------------------------------------------------------------------------
     * <SLV-C>   00h     53h    [7:0]     8'h69      8'h48      OREG_XGC_TARGET_LEVEL[7:0]
     */
    if (pDemod->is_ascot) {
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressC, 0x53, 0x48) != SONY_DVB_OK)
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /*
     * <BBAGC TARGET level setting>
     * slave    bank    addr    bit     default     value      name
     * ----------------------------------------------------------------------------------
     * <SLV-C>   01h     6Ah    [7:0]     8'h50      8'h48      OREG_ITB_DAGC_TRGT[7:0]
     */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressC, 0x00, 0x01) != SONY_DVB_OK)  /* Bank 01 */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressC, 0x6A, 0x48) != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);

    /*
     * <Pre RS BER monitor resolution setting>
     * slave    bank    addr    bit     default     value      name
     * ------------------------------------------------------------------------------
     * <SLV-C>   04h     27h    [6:0]     7'h09     7'h41     OREG_BER_ERRPKT_BITECNT[6:0]
     */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressC, 0x00, 0x04) != SONY_DVB_OK)  /* Bank 04 */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressC, 0x27, 0x41) != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);

    /*
     * <Auto Recovery enhancement setting>
     * slave    bank    addr    bit     default     value      name
     * ----------------------------------------------------------------------------------
     * <SLV-C>   00h     20h     [2:0]     3'h01      3'h06     OREG_AGCLOCKPERIOD[2:0]
     * <SLV-C>   00h     59h     [7:0]     8'h40      8'h50     OREG_XGC_LKTHR[7:0]
     * <SLV-C>   00h     87h     [5:2]     4'h02      4'h03     OREG_AR_TRIAL_NUM[3:0]
     * <SLV-C>   00h     8Bh     [7:0]     8'h05      8'h07     OREG_AR_AGC_TIMER[7:0]
     */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressC, 0x00, 0x00) != SONY_DVB_OK)  /* Bank 00 */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    if (dvb_i2c_SetRegisterBits (pDemod->pI2c, pDemod->i2cAddressC, 0x20, 0x06, 0x07) != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressC, 0x59, 0x50) != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
  
    if (dvb_i2c_SetRegisterBits (pDemod->pI2c, pDemod->i2cAddressC, 0x87, 0x0C, 0x3C) != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressC, 0x8B, 0x07) != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);

    /* DVB-C parameter settings (end) --------------------------------------- */

    /* Enable output (o_reg_port_hiz) */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0xFF, pDemod->ioHiZ) != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);

    /* Soft Reset. */
    result = dvb_demod_SoftReset(pDemod);
    if (result != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (result);
    }

    SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
}

static sony_dvb_result_t NOC2SL (sony_dvb_demod_t * pDemod)
{
    sony_dvb_result_t result = SONY_DVB_OK;

    SONY_DVB_TRACE_ENTER ("NOC2SL");

    /* Same as NOT2SL */
    result = NOT2SL (pDemod);
    SONY_DVB_TRACE_RETURN (result);
}

sony_dvb_result_t dvb_demod_SoftReset (sony_dvb_demod_t * pDemod)
{
    SONY_DVB_TRACE_ENTER ("dvb_demod_SoftReset");

    if (!pDemod)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);

    /* Software state check */
    if ((pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE)
        && (pDemod->state != SONY_DVB_DEMOD_STATE_SLEEP)) {
        /* This api is accepted in ACTIVE or SLEEP states only */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    /* Set bank 0 */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x00, 0x00) != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);

    /* Soft reset (rstgen_hostrst) */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0xFE, 0x01) != SONY_DVB_OK) {
        pDemod->state = SONY_DVB_DEMOD_STATE_INVALID;
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /* For DVB-T2 and ES1.1, reset WR_SMOOTH_SCALE to default (0x0FFF) */
    if ((pDemod->system == SONY_DVB_SYSTEM_DVBT2) && (pDemod->chipId == SONY_DVB_CXD2820_ES1_1)) {
        static const uint8_t data[] = { 0x0F, 0xFF } ; 
        /* Bank 0x20. */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B0_REGC_BANK, CXD2820R_B20) != SONY_DVB_OK) {
            pDemod->state = SONY_DVB_DEMOD_STATE_INVALID;
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
        if (pDemod->pI2c->WriteRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B20_WR_SMOOTH_SCALE_11_8, data, sizeof (data)) != SONY_DVB_OK) {
            pDemod->state = SONY_DVB_DEMOD_STATE_INVALID;
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
    }
    pDemod->state = SONY_DVB_DEMOD_STATE_ACTIVE;
    SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
}

/*--------------------------------------------------------------------
  GPIO functions

  GPIO enable/disable and read/write

  sony_dvb_demod_t           *pDemod    Instance of demod control struct
  int                        id         GPIO number (0 or 1 or 2 (CXD2820 Only))
  int                        is_enable  Set enable (1) or disable (0)
  int                        is_write   Read (0) or Write (1)
  int                        *pValue    Output value (0 or 1)
  int                        value      Input value (0 or 1)
--------------------------------------------------------------------*/
sony_dvb_result_t dvb_demod_GPIOSetting (sony_dvb_demod_t * pDemod, 
                                         int id, 
                                         int is_enable, 
                                         int is_write)
{
    SONY_DVB_TRACE_ENTER ("dvb_demod_GPIOSetting");

    if (!pDemod)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);

    /* Set bank 0 */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x00, 0x00) != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    switch (id) {
    case 0:                    /* GPIO 0 */
        /* Set OREG_INTRPT_IOMODE (bit 7:6) */
        if (dvb_i2c_SetRegisterBits (pDemod->pI2c, pDemod->i2cAddressT, 0x89,
                                     (uint8_t) (is_enable ? 0x80 : 0x40), 0xC0) != SONY_DVB_OK)
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        if (is_enable) {
            /* OREG_GPIEN (bit 3) */
            if (dvb_i2c_SetRegisterBits (pDemod->pI2c, pDemod->i2cAddressT, 0x8E,
                                         (uint8_t) (is_write ? 0x00 : 0x08), 0x08) != SONY_DVB_OK)
                SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
        break;
    case 1:                    /* GPIO 1 */
        /* Set OREG_INTRPT_IOMODE (bit 5:4) */
        if (dvb_i2c_SetRegisterBits (pDemod->pI2c, pDemod->i2cAddressT, 0x89,
                                     (uint8_t) (is_enable ? 0x20 : 0x10), 0x30) != SONY_DVB_OK)
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        if (is_enable) {
            /* OREG_GPIEN (bit 4) */
            if (dvb_i2c_SetRegisterBits (pDemod->pI2c, pDemod->i2cAddressT, 0x8E,
                                         (uint8_t) (is_write ? 0x00 : 0x10), 0x10) != SONY_DVB_OK)
                SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
        break;
    case 2:                   /* GPIO 2 */
        
        /* Device type check. */
        if (pDemod->chipId == SONY_DVB_CXD2817) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_NOSUPPORT);
        }

        /* Set OREG_INTRPT_IOMODE (bit 3:2) */
        /* GPIO or TSERR */
        if (dvb_i2c_SetRegisterBits (pDemod->pI2c, pDemod->i2cAddressT, 0x89,
                                     (uint8_t) (is_enable ? 0x08 : 0x04), 0x0C) != SONY_DVB_OK)
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        
        if (is_enable) {
            /* OREG_GPIEN (bit 5) */
            if (dvb_i2c_SetRegisterBits (pDemod->pI2c, pDemod->i2cAddressT, 0x8E,
                                         (uint8_t) (is_write ? 0x00 : 0x20), 0x20) != SONY_DVB_OK)
                SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
        break;

    default:
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
}

sony_dvb_result_t dvb_demod_GPIORead (sony_dvb_demod_t * pDemod, 
                                      int id, 
                                      int *pValue)
{
    uint8_t rdata = 0x00;

    SONY_DVB_TRACE_ENTER ("dvb_demod_GPIORead");

    if (!pDemod || !pValue)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);

    /* Set bank 0 */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x00, 0x00) != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);

    /* Read o_reg_gpi */
    if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x8F, &rdata, 1) != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);

    switch (id) {
    case 0:
        *pValue = (rdata & 0x01) ? 1 : 0;
        break;
    case 1:
        *pValue = (rdata & 0x02) ? 1 : 0;
        break;
    case 2:

        /* Device type check. */
        if (pDemod->chipId == SONY_DVB_CXD2817) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_NOSUPPORT);
        }

        *pValue = (rdata & 0x04) ? 1 : 0;
        break ;
    default:
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
}

sony_dvb_result_t dvb_demod_GPIOWrite (sony_dvb_demod_t * pDemod, 
                                       int id, 
                                       int value)
{
    SONY_DVB_TRACE_ENTER ("dvb_demod_GPIOWrite");

    if (!pDemod)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);

    /* Set bank 0 */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x00, 0x00) != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);

    switch (id) {
    case 0:
        /* OREG_GPO (bit 0) */
        if (dvb_i2c_SetRegisterBits (pDemod->pI2c, pDemod->i2cAddressT, 0x8E,
                                     (uint8_t) (value ? 0x01 : 0x00), 0x01) != SONY_DVB_OK)
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        break;
    case 1:
        /* OREG_GPO (bit 1) */
        if (dvb_i2c_SetRegisterBits (pDemod->pI2c, pDemod->i2cAddressT, 0x8E,
                                     (uint8_t) (value ? 0x02 : 0x00), 0x02) != SONY_DVB_OK)
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        break;
    case 2:

        /* Device type check. */
        if (pDemod->chipId == SONY_DVB_CXD2817) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_NOSUPPORT);
        }

        /* OREG_GPO (bit 2) */
        if (dvb_i2c_SetRegisterBits (pDemod->pI2c, pDemod->i2cAddressT, 0x8E,
                                     (uint8_t) (value ? 0x04 : 0x00), 0x04) != SONY_DVB_OK)
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        break;
    default:
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
}

/*--------------------------------------------------------------------
  DVB-T2 Functionality.
--------------------------------------------------------------------*/

/**
 @brief CXD2820 ES1.0 recommended settings.
*/
static const sony_dvb_demod_reg_t gCXD2820ES1_0_T2Settings[] = 
{
    /*  Bank    Sub Address                         Value   Mask */
    {   0x20,   CXD2820R_B20_P1DET_S1_OK_THR,       0x0A,   0xFF}, /* P1 S1 detect optimisation. */
    {   0x20,   CXD2820R_B20_P1DET_S2_OK_THR,       0x0A,   0xFF}, /* P1 S2 detect optimisation.. */

    {   0x20,   CXD2820R_B20_SEQ_L1PRE_WTIME_15_8,  0x1A,   0xFF}, /* L1 pre wait time optimisation. */
    {   0x20,   CXD2820R_B20_SEQ_L1POST_WTIME_15_8, 0x50,   0xFF}, /* L1 post wait time optimisation. */

    {   0x25,   CXD2820R_B25_SP_TTO_CORRECT_MIN,    0x07,   0x0F}, /* Stream processor optimisation. */
    {   0x25,   CXD2820R_B25_SP_ISCR_CNT_CORRECT_MODE, 0x03,   0x03}, /* Stream processor optimisation. */

    {   0x2A,   CXD2820R_B2A_CSI_ALB_TRGT_U,        0x00,   0xFF}, /* 2path optimisation. */
    {   0x2A,   CXD2820R_B2A_CSI_ALB_TRGT_L,        0x34,   0xFF}, /* 2path optimisation. */

    {   0x27,   CXD2820R_B27_NEW_FRM_THR,           0x14,   0xFF}, /* FEF optimisation. */
    {   0x3F,   CXD2820R_B3F_OREG_OYOBIA,           0x00,   0xFF}, /* FEF optimisation. */
    {   0x3F,   CXD2820R_B3F_OREG_OYOBIB,           0x02,   0xFF}, /* FEF optimisation. */
    {   0x3F,   CXD2820R_B3F_OREG_OYOBIC,           0x01,   0xFF}, /* FEF optimisation. */

    {   0x3F,   CXD2820R_B3F_OREG_FYOBIC,           0x0C,   0xFF}, /* M-PLP TS output optimisation. */

    {   0x27,   CXD2820R_B27_TITP_SETTING0,         0x22,   0x07}, /* Doppler synchronous time optimisation. */
    {   0x27,   CXD2820R_B27_TITP_SETTING1,         0x30,   0xE0}, /* Doppler synchronous time optimisation. */

    {   0x27,   CXD2820R_B27_NEW_CAS_DAGC_PARAM,    0x05,   0x18}, /* CAS Flutter optimisation. */
    {   0x2A,   CXD2820R_B2A_CAS_CCIFILT_ACTCOND,   0x06,   0x07}, /* CAS Flutter optimisation. */

    {   0x20,   CXD2820R_B20_LBER_MES,              0x18,   0x10}, /* Multiple PLP BER measurement optimisation. */
    {   0x20,   CXD2820R_B20_BBER_MES,              0x18,   0x10}  /* Multiple PLP BER measurement optimisation. */
};


/**
 @brief CXD2820 ES1.1 recommended settings.
*/
static const sony_dvb_demod_reg_t gCXD2820ES1_1_T2Settings[] = 
{
    /*  Bank    Sub Address                         Value   Mask */
    {   0x20,   CXD2820R_B20_P1DET_S1_OK_THR,       0x0A,   0xFF}, /* P1 S1 detect optimisation. */
    {   0x20,   CXD2820R_B20_P1DET_S2_OK_THR,       0x0A,   0xFF}, /* P1 S2 detect optimisation.. */

    {   0x20,   CXD2820R_B20_SEQ_L1PRE_WTIME_15_8,  0x1A,   0xFF}, /* L1 pre wait time optimisation. */
    {   0x20,   CXD2820R_B20_SEQ_L1POST_WTIME_15_8, 0x50,   0xFF}, /* L1 post wait time optimisation. */

    {   0x25,   CXD2820R_B25_SP_TTO_CORRECT_MIN,    0x07,   0x0F}, /* Stream processor optimisation. */
    {   0x25,   CXD2820R_B25_SP_ISCR_CNT_CORRECT_MODE, 0x03,   0x03}, /* Stream processor optimisation. */

    {   0x2A,   CXD2820R_B2A_CSI_ALB_TRGT_U,        0x00,   0xFF}, /* 2path optimisation. */
    {   0x2A,   CXD2820R_B2A_CSI_ALB_TRGT_L,        0x34,   0xFF}, /* 2path optimisation. */

    {   0x27,   CXD2820R_B27_NEW_FRM_THR,           0x14,   0xFF}, /* FEF optimisation. */
    {   0x3F,   CXD2820R_B3F_OREG_OYOBIA,           0x0D,   0xFF}, /* FEF optimisation. */
    {   0x3F,   CXD2820R_B3F_OREG_OYOBIB,           0x02,   0xFF}, /* FEF optimisation. */
    {   0x3F,   CXD2820R_B3F_OREG_OYOBIC,           0x01,   0xFF}, /* FEF optimisation. */

    {   0x3F,   CXD2820R_B3F_OREG_FYOBIC,           0x2C,   0xFF}, /* M-PLP TS output optimisation. */
    {   0x3F,   CXD2820R_B3F_OREG_SYOBIB,           0x13,   0xFF}, /* M-PLP TS output optimisation. */
    {   0x3F,   CXD2820R_B3F_OREG_SYOBIC,           0x01,   0xFF}, /* M-PLP TS output optimisation. */
    {   0x3F,   CXD2820R_B3F_OREG_SYOBID,           0x00,   0xFF}, /* M-PLP TS output optimisation. */
    
    {   0x27,   CXD2820R_B27_CRCG_FGFACT0,          0x0F,   0x0F}, /* CRL optimisation. */
    {   0x27,   CXD2820R_B27_CRCG_FGFACT1,          0x0F,   0x0F}, /* CRL optimisation. */
    {   0x27,   CXD2820R_B27_CRCG_FGFACT2,          0x0F,   0x0F}, /* CRL optimisation. */
    {   0x27,   CXD2820R_B27_CRCG_FGFACT3,          0x0F,   0x0F}, /* CRL optimisation. */

    {   0x27,   CXD2820R_B27_TITP_SETTING0,         0x22,   0x07}, /* Doppler synchronous time optimisation. */
    {   0x27,   CXD2820R_B27_TITP_SETTING1,         0x30,   0xE0}, /* Doppler synchronous time optimisation. */

    {   0x27,   CXD2820R_B27_NEW_CAS_DAGC_PARAM,    0x15,   0x18}, /* CAS Flutter optimisation. */
    {   0x2A,   CXD2820R_B2A_CAS_CCIFILT_ACTCOND,   0x06,   0x07}, /* CAS Flutter optimisation. */

    {   0x20,   CXD2820R_B20_LBER_MES,              0x18,   0x10}, /* Multiple PLP BER measurement optimisation. */
    {   0x20,   CXD2820R_B20_BBER_MES,              0x18,   0x10}, /* Multiple PLP BER measurement optimisation. */

    {   0x20,   CXD2820R_B20_P1DET_DET_THR,         0x2A,   0xFF}  /* 3path optimisation. */
};

static sony_dvb_result_t SL2NOG (sony_dvb_demod_t * pDemod)
{
    sony_dvb_result_t result = SONY_DVB_OK;

    SONY_DVB_TRACE_ENTER ("SL2NOG");

    /* Argument verification. */
    if (!pDemod) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* Set bank 0 */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, 0x00, 0x00) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /* Set dvbmode (DVB-T2 mode) */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B0_DVBMODE, 0x02) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /* Enable RF level monitoring (if requested) */
    if (pDemod->enable_rflvmon) {
        /* (OREG_CK_RM_EN | OREG_CK_G_EN) */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B0_CLKREG_CK_EN, (0x20 | 0x10)) !=
            SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
    }
    else {
        /* (OREG_CK_G_EN) */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B0_CLKREG_CK_EN, 0x20) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
    }

    /* Set adcreg_adc_en (OREG_ADC_AREGEN | OREG_ADC_ADCEN | OREG_ADC_CLKGENEN) */
    if (dvb_i2c_SetRegisterBits (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B0_ADCREG_ADC_EN, 0x07, 0x07) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /* Set ramreg_ramon_stb (SSM Enable) */
    if (pDemod->enable_rflvmon) {
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B0_RAMREG_RAMON_STB, 0x02) !=  SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
    }

    /* Set bank 0x20 */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, 0x00, 0x20) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /* Enable eDRAM : OREG_DRAM_ENABLE */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B20_DRAM_ENABLE, 0x01) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /* Setup band setting. */
    result = SL2NOG_BandSetting (pDemod);
    if (result != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (result);
    }

    /* Only apply settings to non-TS devices. */
    if (pDemod->chipId == SONY_DVB_CXD2820_ES1_0) {
        result = demod_WriteRegisters (pDemod, pDemod->i2cAddressT2, gCXD2820ES1_0_T2Settings, SONY_DVB_ARRAY_SIZE(gCXD2820ES1_0_T2Settings));
        if (result != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (result);
        }
    }
    else if ((pDemod->chipId == SONY_DVB_CXD2820_ES1_1) || 
             (pDemod->chipId == SONY_DVB_CXD2820_IP)) {
        result = demod_WriteRegisters (pDemod, pDemod->i2cAddressT2, gCXD2820ES1_1_T2Settings, SONY_DVB_ARRAY_SIZE(gCXD2820ES1_1_T2Settings));
        if (result != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (result);
        }
    }

    /* Update CAS filter setting if not Ascot. */
    if (!pDemod->is_ascot) {

        /* Set Bank 0x20. */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B0_REGC_BANK, CXD2820R_B20) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        /* OREG_CAS_ACIFLT2_EN[1:0] = 3 */
        if (dvb_i2c_SetRegisterBits (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B20_CAS_PARAMS1, 0x03, 0x03) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
    }

    /* Set bank 0x0 */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, 0x00, 0x00) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /* Enable TS output. */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B0_O_REG_PORT_HIZ, pDemod->ioHiZ) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /* Prepare for T2 acquisition. */
    result = t2_PrepareAcquisition (pDemod);
    if (result != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (result);
    }

    /* Soft Reset. DVB-T2 Long Echo sequence includes soft reset. */
    if (!pDemod->t2LongEchoPP2) {
        result = dvb_demod_SoftReset(pDemod);
        if (result != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (result);
        }
    }

    SONY_DVB_TRACE_RETURN (result);
}

static sony_dvb_result_t NOG2SL (sony_dvb_demod_t * pDemod)
{
        
    SONY_DVB_TRACE_ENTER ("NOG2SL");

    /* Argument verification. */
    if (!pDemod) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* Set bank 0x0. */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, 0x00, 0x00) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /* Disable TS output. (0=Output, 1=Hi-Z) */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B0_O_REG_PORT_HIZ, 0x1F) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /* ADC Stop. Shared IF may be enabled */
    if (dvb_i2c_SetRegisterBits (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B0_ADCREG_ADC_EN, (pDemod->sharedIf ? 0x06 : 0x00), 0x07) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /* Set ramreg_ramon_stb (SSM Disable) */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B0_RAMREG_RAMON_STB, 0x01) !=  SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /* Set bank 0x20. */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, 0x00, 0x20) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }
    
    /* EDRAM stop. */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B20_DRAM_ENABLE, 0x00) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /* Set bank 0x00 */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, 0x00, 0x00) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /* Disable DVB-T2 clock. */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B0_CLKREG_CK_EN, 0x00) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /* Stop DVB-T2 auto recovery. */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B0_DVBMODE, 0x00) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /* Update state. */
    pDemod->state = SONY_DVB_DEMOD_STATE_SLEEP;

    SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
}

static sony_dvb_result_t SL2NOG_BandSetting (sony_dvb_demod_t * pDemod)
{
    sony_dvb_result_t result = SONY_DVB_OK;

    SONY_DVB_TRACE_ENTER ("SL2NOG_BandSetting");

    /* Argument verification. */
    if (!pDemod) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* Set bank 0x00 */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, 0x00, 0x00) != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);

    if (pDemod->is_ascot) {
        /* ADC Gain setting (Bit 6:5) - 1.4Vp-p */
        if (dvb_i2c_SetRegisterBits (pDemod->pI2c, pDemod->i2cAddressT2, 0x82, 0x40, 0x60) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
    }
    else {
        /* ADC Gain setting (Bit 6:5) - 1.0Vp-p */
        if (dvb_i2c_SetRegisterBits (pDemod->pI2c, pDemod->i2cAddressT2, 0x82, 0x20, 0x60) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
    }
    
    /*
     * <SW reset without IFAGC reset (ASCOT only)>
     *  slave    Bank    Addr    Bit    default     Value       Name
     * <SLV-T>   21h     48h    [4]       8'h20      8'h30      OCTL_AGC_NO_SWRST
    */
    {
        uint8_t data = pDemod->is_ascot ? 0x10 : 0x00;

	    /* Set bank 0x21 */
	    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, 0x00, 0x21) != SONY_DVB_OK) {
	        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
	    }
        if (dvb_i2c_SetRegisterBits (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B21_PIR_AGC_CTL0, data, 0x10) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
		}
    }

    /* Set bank 0x20 */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, 0x00, 0x20) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    if (pDemod->is_ascot) {
        /* Set itb_gdeq_en (Bit 0) - GDEQ ON */
        if (dvb_i2c_SetRegisterBits (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B20_ITB_GDEQ_EN, 0x01, 0x01) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
    }
    else {
        /* Set itb_gdeq_en (Bit 0) - GDEQ OFF  */
        if (dvb_i2c_SetRegisterBits (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B20_ITB_GDEQ_EN, 0x00, 0x01) != SONY_DVB_OK) { 
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
    }

    switch (pDemod->bandWidth) {
        case SONY_DVB_DVBT2_BW_5_MHZ:
        {
            /* TRL settings for 5MHz. */
            {
                static const uint8_t data[] = { 0x1C, 0xB3, 0x33, 0x33, 0x33 };
                if (pDemod->pI2c->WriteRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B20_TRL_NOMINALRATE_37_32, data,
                                   sizeof (data)) != SONY_DVB_OK) {
                    SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
                }
            }

            /* Group delay equalizer settings for ASCOT tuner only for 5MHz. */
            if (pDemod->is_ascot) {
                static const uint8_t data[] =
                    { 0x19, 0x24, 0x2B, 0xB7, 0x2C, 0xAC, 0x29, 0xA6, 0x2A, 0x9F, 0x2A, 0x99, 0x2A, 0x9B };
                /* xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xb0, 0xb1, 0xb2, 0xb3 */
                if (pDemod->pI2c->WriteRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B20_ITB_GDEQ_CF_01, data,
                                                 sizeof (data)) != SONY_DVB_OK) {
                    SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
                }
            }

            /* DNC (Down converter) frequency settings for 5MHz. */
            {
                uint8_t data[3];
                data[0] = (uint8_t) ((pDemod->iffreq_config.config_DVBT2_5 >> 16) & 0xFF);
                data[1] = (uint8_t) ((pDemod->iffreq_config.config_DVBT2_5 >> 8) & 0xFF);
                data[2] = (uint8_t) ((pDemod->iffreq_config.config_DVBT2_5) & 0xFF);
                if (pDemod->pI2c->WriteRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B20_TDA_DNCNV_FREQ2, data,
                                                 sizeof (data)) != SONY_DVB_OK) {
                    SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
                }
            }

            /* TSIF setting for 5MHz. */
            if (dvb_i2c_SetRegisterBits (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B20_TSIF_SM_CTL, 0xC0, 0xC0) != SONY_DVB_OK) {
                SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
            }
        }
        break;

        case SONY_DVB_DVBT2_BW_6_MHZ:
        {
            /* TRL settings for 6MHz. */
            {
                static const uint8_t data[] = { 0x17, 0xEA, 0xAA, 0xAA, 0xAA };
                if (pDemod->pI2c->WriteRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B20_TRL_NOMINALRATE_37_32, data, sizeof (data)) != SONY_DVB_OK) {
                    SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
                }
            }

            /* Group delay equalizer settings for ASCOT tuner only. */
            if (pDemod->is_ascot) {
                static const uint8_t data[] =
                    { 0x19, 0x24, 0x2B, 0xB7, 0x2C, 0xAC, 0x29, 0xA6, 0x2A, 0x9F, 0x2A, 0x99, 0x2A, 0x9B };
                    /* xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xb0, 0xb1, 0xb2, 0xb3 */
                if (pDemod->pI2c->WriteRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B20_ITB_GDEQ_CF_01, data, sizeof (data)) != SONY_DVB_OK) {
                    SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
                }
            }

            /* DNC (Down converter) frequency settings for 6MHz. */
            {
                uint8_t data[3];
                data[0] = (uint8_t) ((pDemod->iffreq_config.config_DVBT2_6 >> 16) & 0xFF);
                data[1] = (uint8_t) ((pDemod->iffreq_config.config_DVBT2_6 >> 8) & 0xFF);
                data[2] = (uint8_t) ((pDemod->iffreq_config.config_DVBT2_6) & 0xFF);
                if (pDemod->pI2c->WriteRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B20_TDA_DNCNV_FREQ2, data, sizeof (data)) != SONY_DVB_OK) {
                    SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
                }
            }

            /* TSIF setting for 6MHz. */
            if (dvb_i2c_SetRegisterBits (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B20_TSIF_SM_CTL, 0x80, 0xC0) != SONY_DVB_OK) {
                SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
            }
        }
        break;

        case SONY_DVB_DVBT2_BW_7_MHZ:
        {
            /* TRL settings for 7MHz. */
            {
                static const uint8_t data[] = { 0x14, 0x80, 0x00, 0x00, 0x00 };
                if (pDemod->pI2c->WriteRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B20_TRL_NOMINALRATE_37_32, data, sizeof (data)) != SONY_DVB_OK) {
                    SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
                }
            }

            /* Group delay equalizer settings for ASCOT tuner only. */
            if (pDemod->is_ascot) {
                static const uint8_t data[] =
                    { 0x1B, 0x22, 0x2B, 0xC1, 0x2C, 0xB3, 0x2B, 0xA9, 0x2B, 0xA0, 0x2B, 0x97, 0x2B, 0x9B };
                    /* xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xb0, 0xb1, 0xb2, 0xb3 */
                if (pDemod->pI2c->WriteRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B20_ITB_GDEQ_CF_01, data, sizeof (data)) != SONY_DVB_OK) {
                    SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
                }
            }

            /* DNC (Down converter) frequency settings for 7MHz. */
            {
                uint8_t data[3];
                data[0] = (uint8_t) ((pDemod->iffreq_config.config_DVBT2_7 >> 16) & 0xFF);
                data[1] = (uint8_t) ((pDemod->iffreq_config.config_DVBT2_7 >> 8) & 0xFF);
                data[2] = (uint8_t) ((pDemod->iffreq_config.config_DVBT2_7) & 0xFF);
                if (pDemod->pI2c->WriteRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B20_TDA_DNCNV_FREQ2, data,
                                                 sizeof (data)) != SONY_DVB_OK) {
                    SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
                }
            }

            /* TSIF setting for 7MHz. */
            if (dvb_i2c_SetRegisterBits (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B20_TSIF_SM_CTL, 0x40, 0xC0) != SONY_DVB_OK) {
                SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
            }
        }
        break;

        case SONY_DVB_DVBT2_BW_8_MHZ:
        {
            /* TRL settings for 8MHz. */
            {
                static const uint8_t data[] = { 0x11, 0xF0, 0x00, 0x00, 0x00 };
                if (pDemod->pI2c->WriteRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B20_TRL_NOMINALRATE_37_32, data,
                                                 sizeof (data)) != SONY_DVB_OK) {
                    SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
                }
            }

            /* Group delay equalizer settings for ASCOT tuner only. */
            if (pDemod->is_ascot) {
                static const uint8_t data[] =
                    { 0x1E, 0x1D, 0x29, 0xC9, 0x2A, 0xBA, 0x29, 0xAD, 0x29, 0xA4, 0x29, 0x9A, 0x28, 0x9E };
                    /*0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xb0, 0xb1, 0xb2, 0xb3 */
                if (pDemod->pI2c->WriteRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B20_ITB_GDEQ_CF_01, data,
                                                 sizeof (data)) != SONY_DVB_OK) {
                    SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
                }
            }

            /* DNC (Down converter) frequency settings for 8MHz. */
            {
                uint8_t data[3];
                data[0] = (uint8_t) ((pDemod->iffreq_config.config_DVBT2_8 >> 16) & 0xFF);
                data[1] = (uint8_t) ((pDemod->iffreq_config.config_DVBT2_8 >> 8) & 0xFF);
                data[2] = (uint8_t) ((pDemod->iffreq_config.config_DVBT2_8) & 0xFF);
                if (pDemod->pI2c->WriteRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B20_TDA_DNCNV_FREQ2, data,
                    sizeof (data)) != SONY_DVB_OK) {
                    SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
                }
            }

            /* TSIF setting for 8MHz. */
            if (dvb_i2c_SetRegisterBits (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B20_TSIF_SM_CTL, 0x00, 0xC0) != SONY_DVB_OK) {
                SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
            }
        }
        break;

        default:
            result = SONY_DVB_ERROR_ARG;
            break;
    }

    SONY_DVB_TRACE_RETURN (result);
}

static sony_dvb_result_t t2_CheckDemodLock (sony_dvb_demod_t * pDemod, 
                                            sony_dvb_demod_lock_result_t * pLock)
{
    sony_dvb_result_t result = SONY_DVB_OK;
    uint8_t data;

    SONY_DVB_TRACE_ENTER ("t2_CheckDemodLock");

    if ((!pDemod) || (!pLock)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* Set bank 0x20 */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, 0x00, CXD2820R_B20) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B20_SP_TS_LOCK, &data, 1) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }
    
    /* Demod lock, sync state == 6. */
    *pLock = ((data & 0x07) == 6) ? DEMOD_LOCK_RESULT_LOCKED : DEMOD_LOCK_RESULT_NOTDETECT ;

    SONY_DVB_TRACE_RETURN (result);
}

sony_dvb_result_t dvb_demod_t2_CheckP1Lock (sony_dvb_demod_t * pDemod, 
                                            sony_dvb_demod_lock_result_t * pLock)
{
    sony_dvb_result_t result = SONY_DVB_OK;

    SONY_DVB_TRACE_ENTER ("dvb_demod_t2_CheckP1Lock");
    if ((!pDemod) || (!pLock)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* Software state check */
    if (pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) {
        /* This api is accepted in ACTIVE state only */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    if (pDemod->system != SONY_DVB_SYSTEM_DVBT2) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }
    {
        /* P1 early detect. */
        uint8_t p1DetLock = 0 ;
        uint8_t seqMaxState = 0 ;

        /* Set Bank 0x20 */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, 0x00, CXD2820R_B20) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
        if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B20_P1DET_STATE, &p1DetLock, 1) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
        if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B20_SEQ_NG_OCCURRED_MAX_STATE, &seqMaxState, 1) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
        
        *pLock = (((p1DetLock & 0x10) == 0) && ((seqMaxState & 0x07) <= 2)) ? 
            DEMOD_LOCK_RESULT_NOTDETECT : DEMOD_LOCK_RESULT_LOCKED  ;
    }

    SONY_DVB_TRACE_RETURN (result);
}

sony_dvb_result_t dvb_demod_t2_CheckT2P1Lock (sony_dvb_demod_t * pDemod, 
                                              sony_dvb_demod_lock_result_t * pLock)
{
    sony_dvb_result_t result = SONY_DVB_OK;
    uint8_t data;

    SONY_DVB_TRACE_ENTER ("dvb_demod_t2_CheckT2P1Lock");

    if ((!pDemod) || (!pLock)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }    
    
    /* Software state check */
    if (pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) {
        /* This api is accepted in ACTIVE state only */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    if (pDemod->system != SONY_DVB_SYSTEM_DVBT2) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* Set bank 0x20 */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, 0x00, CXD2820R_B20) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B20_SEQ_NG_OCCURRED_MAX_STATE, &data, 1) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }
    
    /* Detect T2 P1 if detected at least once since last soft reset. */
    *pLock = ((data & 0x07) <= 2) ? DEMOD_LOCK_RESULT_NOTDETECT : DEMOD_LOCK_RESULT_LOCKED ;

    SONY_DVB_TRACE_RETURN (result);
}

sony_dvb_result_t dvb_demod_t2_SetPLPConfig (sony_dvb_demod_t * pDemod, 
                                             uint8_t autoPLP,
                                             uint8_t plpId)
{
    sony_dvb_result_t result = SONY_DVB_OK;

    SONY_DVB_TRACE_ENTER ("dvb_demod_t2_SetPLPConfig");
    if ((!pDemod)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* Software state check */
    if ((pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE)
        && (pDemod->state != SONY_DVB_DEMOD_STATE_SLEEP)) {
        /* This api is accepted in ACTIVE or SLEEP states only */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }    
    
    /* Device type check. */
    if (pDemod->chipId == SONY_DVB_CXD2817) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_NOSUPPORT);
    }

    /* Argument check */
    if (autoPLP >= 3) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* Set Bank 0x23 */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B0_REGC_BANK, CXD2820R_B23) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    if (autoPLP != 0) {
        /* Manual PLP selection mode. Set the data PLP Id. */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B23_FP_PLP_ID, plpId) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
    }

    /* Auto PLP select (Scanning mode = 0x00). Data PLP select = 0x01. OREGD_FP_PLP_AUTO_SEL[1:0] */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B23_FP_PLP_AUTO_SEL, autoPLP & 0x03) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    SONY_DVB_TRACE_RETURN (result);
}

static sony_dvb_result_t t2_SetConfig (sony_dvb_demod_t * pDemod, 
                                       sony_dvb_demod_config_id_t configId, 
                                       int32_t value)
{
    sony_dvb_result_t result = SONY_DVB_OK ;

    SONY_DVB_TRACE_ENTER ("t2_SetConfig");

    if (!pDemod)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);

    switch (configId) {
        /* ======================== DVB-T2 ============================= */
    case DEMOD_CONFIG_PARALLEL_SEL:

        /* gcsp_params: OREG_SP_PARALLEL_SEL (Serial/Parallel output select) */
        /* This register can change only in SLEEP state */
        if (pDemod->state != SONY_DVB_DEMOD_STATE_SLEEP) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
        }

        /* Set bank 0x20 */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B0_REGC_BANK, CXD2820R_B20) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        /* Set OREG_TSIF_PARALLEL_SEL (bit 4) */
        if (dvb_i2c_SetRegisterBits (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B20_GCSP_PARAMS,
            (uint8_t) (value ? 0x10 : 0x00), 0x10) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
        break;

    case DEMOD_CONFIG_SER_DATA_ON_MSB:

        /* gcsp_params: OREG_SP_SER_DATA_ON_MSB (Bit order on serial mode) */
        /* Set bank 0x20 */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B0_REGC_BANK, CXD2820R_B20) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        /* Set OREG_TSIF_SER_DATA_ON_MSB (bit 5) */
        if (dvb_i2c_SetRegisterBits (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B20_GCSP_PARAMS,
            (uint8_t) (value ? 0x20 : 0x00), 0x20) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
        break;

    case DEMOD_CONFIG_OUTPUT_SEL_MSB:
        /* gcsp_params: OREG_SP_OUTPUT_SEL_MSB (Bit order on serial mode) */
        /* Set bank 0x20 */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B0_REGC_BANK, CXD2820R_B20) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        /* Set OREG_TSIF_OUTPUT_SEL_MSB (bit 3) */
        if (dvb_i2c_SetRegisterBits (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B20_GCSP_PARAMS,
            (uint8_t) (value ? 0x08 : 0x00), 0x08) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
        break;

    case DEMOD_CONFIG_TSVALID_ACTIVE_HI:
        /* bb_params: OREG_SP_TSVALID_ACTIVE_HI */
        /* Set bank 0x20 */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B0_REGC_BANK, CXD2820R_B20) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        /* Set OREG_TSIF_TSVALID_ACTIVE_HI (bit 7) */
        if (dvb_i2c_SetRegisterBits (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B20_BB_PARAMS,
            (uint8_t) (value ? 0x80 : 0x00), 0x80) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
        break;

    case DEMOD_CONFIG_LATCH_ON_POSEDGE:
        /* bb_params: OREG_SP_LATCH_ON_POSEDGE (TS Clock invert) */
        /* This register can change only in SLEEP state */
        if (pDemod->state != SONY_DVB_DEMOD_STATE_SLEEP) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
        }

        /* Set bank 0x20 */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B0_REGC_BANK, CXD2820R_B20) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
        /* Set OREG_TSIF_LATCH_ON_POSEDGE (bit 6) */
        if (dvb_i2c_SetRegisterBits (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B20_BB_PARAMS,
            (uint8_t) (value ? 0x40 : 0x00), 0x40) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
        break;

    case DEMOD_CONFIG_TSCLK_CONT:
        /* bb_params: OREG_SP_TSCLK_ACTIVATE bit 0 (TS Clock continuous or gated) */

        /* Set bank 0x20 */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B0_REGC_BANK, CXD2820R_B20) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
        /* Set OREG_SP_TSCLK_ACTIVATE (bit 0) */
        if (dvb_i2c_SetRegisterBits (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B20_BB_PARAMS,
            (uint8_t) (value ? 0x01 : 0x00), 0x01) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
        break;

    case DEMOD_CONFIG_IFAGCNEG:

        /* Set bank 0x20 */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B0_REGC_BANK, CXD2820R_B20) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        /* Set OCTL_IFAGCNEG (bit 6) */
        if (dvb_i2c_SetRegisterBits (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B20_PIR_AGC_CTL1,
            (uint8_t) (value ? 0x40 : 0x00), 0x40) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
        break;

    case DEMOD_CONFIG_SPECTRUM_INV:
        {
            sony_dvb_demod_spectrum_sense_t sense = (value ? DVB_DEMOD_SPECTRUM_INV : DVB_DEMOD_SPECTRUM_NORMAL) ; 
            result = dvb_demod_t2_SetSpectrumSense (pDemod, sense);
            if (result != SONY_DVB_OK) {
                SONY_DVB_TRACE_RETURN(result);
            }
        }
        break ;

    case DEMOD_CONFIG_RFAGC_ENABLE:

        /* Set bank 0x21 */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B0_REGC_BANK, CXD2820R_B21) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }           
        
        /* Set OCTL_RFAGCMANUAL. */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B21_RFAGC_RFAGCMANUAL, (value ? 0x01 : 0x00)) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        /* Set bank 0x20 */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B0_REGC_BANK, CXD2820R_B20) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        /* Enable RFAGC PWM output (bit 1). */
        if (dvb_i2c_SetRegisterBits(pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B20_RIDAPG_DISABLE, value ? 0x00 : 0x02, 0x02) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }   

        break ;

    case DEMOD_CONFIG_TSIF_SDPR:
        /* Argument checking. */
        if ((value <= 0) || (value > SONY_CXD2820_MAX_TS_CLK_KHZ)) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_RANGE);
        }

        /* Write OREG_SP_WR_SMOOTH_DP[27:0] */
        {
            uint32_t val = 0x00 ;
            uint8_t data[4] ;
            uint32_t uvalue = (uint32_t) value ;

            /* Set Bank 0x20. */
            if (pDemod->pI2c->WriteOneRegister(pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B0_REGC_BANK, CXD2820R_B20) != SONY_DVB_OK) {
                SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C) ;
            }
            
            /* 
                OREG_SP_WR_SMOOTH_DP = (41(MHz) / TSCLK(MHz)) * 2^16 ; 
                Notes: 41000 * (2^16) = 2686976000
            */
            val = (2686976000u + uvalue / 2) / uvalue ;
            data[0] = (uint8_t) ((val & 0x0F000000) >> 24) ;
            data[1] = (uint8_t) ((val & 0xFF0000) >> 16) ;
            data[2] = (uint8_t) ((val & 0xFF00) >>  8);
            data[3] = (uint8_t)  (val & 0xFF) ;

            if (pDemod->pI2c->WriteRegister(pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B20_WR_SMOOTH_DP_27_24, data, sizeof(data)) != SONY_DVB_OK) {
                SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C) ;
            }
        }
        break ;

    case DEMOD_CONFIG_TS_AUTO_RATE_ENABLE:
        /* Set bank 0x20 */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B0_REGC_BANK, CXD2820R_B20) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
        /* Set OREG_AUTO_RATE_EN */
        if (dvb_i2c_SetRegisterBits(pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B20_TSIF_SM_CTL, value ? 0x02 : 0x00, 0x02) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C) ;
        }
        break ;

    case DEMOD_CONFIG_DVBT2_PP2_LONG_ECHO:
        
        /* If using DVB-T2 Long Echo sequence and changing to not DVB-T2 Long Echo sequence.  */
        /* Then reset defaults. */
        if (pDemod->t2LongEchoPP2 && !value) {
            result = le_seq_Init (pDemod);
            if (result != SONY_DVB_OK) {
                SONY_DVB_TRACE_RETURN (result);
            }
        }
        pDemod->t2LongEchoPP2 = (uint8_t) (value ? 1 : 0) ;
        break ;

    /* Intentional fall-through */
    case DEMOD_CONFIG_RFLVMON_ENABLE:
    case DEMOD_CONFIG_SCR_ENABLE:
    case DEMOD_CONFIG_TSSYNC_ACTIVE_HI:
    case DEMOD_CONFIG_TSERR_ACTIVE_HI:
    case DEMOD_CONFIG_SHARED_IF:
        /* ============================= DVB-T ================================== */
    case DEMOD_CONFIG_DVBT_LPSELECT:
    case DEMOD_CONFIG_DVBT_FORCE_MODEGI:
    case DEMOD_CONFIG_DVBT_MODE:
    case DEMOD_CONFIG_DVBT_GI:
    case DEMOD_CONFIG_DVBT_SCANMODE:

        /* ============================= DVB-C ================================== */
    case DEMOD_CONFIG_DVBC_SCANMODE:
        break ;

    default:
        /* Unknown ID */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
}

static sony_dvb_result_t demod_WriteRegisters (sony_dvb_demod_t* pDemod,
                                               uint8_t address,
                                               const sony_dvb_demod_reg_t* regs,
                                               uint8_t regCount)
{
    uint8_t i = 0 ;
    uint8_t currentBank = 0xFF ;

    SONY_DVB_TRACE_ENTER ("demod_WriteRegisters");
    if (!pDemod || !regs)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);

    for (i = 0 ; i < regCount ; i++) {

        /* Handle new banks. */
        if (currentBank != regs[i].bank) {
            currentBank = regs[i].bank ;
            if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, address, CXD2820R_B0_REGC_BANK, currentBank) != SONY_DVB_OK) {
                SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
            }
        }

        /* Write the register values. */
        if (regs[i].mask != 0xFF) {
            if (dvb_i2c_SetRegisterBits (pDemod->pI2c, address, regs[i].subAddress, regs[i].value, regs[i].mask) != SONY_DVB_OK) {
                SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
            }
        }
        else {
            if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, address, regs[i].subAddress, regs[i].value) != SONY_DVB_OK) {
                SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
            }
        }
    }
    SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
}

static sony_dvb_result_t monitorT2_TSRate (sony_dvb_demod_t * pDemod, 
                                           uint32_t* pTSRate)
{

    SONY_DVB_TRACE_ENTER ("monitorT2_TSRate");
    if ((!pDemod) || (!pTSRate)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* Software state check */
    if (pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) {
        /* This api is accepted in ACTIVE state only */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    /* Check the running system. */
    if (pDemod->system != SONY_DVB_SYSTEM_DVBT2) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    {
        uint8_t data[4] ;

        /* Set bank 0x25 */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B0_REGC_BANK, CXD2820R_B25) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        /* Read registers. IREG_SP_RATEP. */
        if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B25_SP_RATEP_LOCK, data, sizeof (data)) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
        
        if (!(data[0] & 0x01)) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_HW_STATE);
        }

        *pTSRate = (data[1] & 0x0F) << 16 ;
        *pTSRate |= data[2] << 8 ;
        *pTSRate |= data[3] ;
    }

    SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
}
        
#define SONY_CXD2820_T2_FRAME_LEN_RATE_ADJUST   300U

static sony_dvb_result_t t2_es1_0_OptimiseTSOutput (sony_dvb_demod_t * pDemod)
{
    sony_dvb_result_t result = SONY_DVB_OK;

    SONY_DVB_TRACE_ENTER ("t2_es1_0_OptimiseTSOutput");
    if ((!pDemod)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    {
        /* 
            if ( IREG_OFDM_MIXED == 0 ) {
                OREG_SYOBIB = 8'h00
                {OREG_SYOBIC[2:0],OREG_SYOBID[7:0]} = 11'd300
            } else {
                if ( T2_frame_length - 300 > 0 ) {
                    OREG_SYOBIB = 8'h03
                    {OREG_SYOBIC[2:0],OREG_SYOBID[7:0]} = T2_frame_length - 11'd300
                } else {
                    OREG_SYOBIB = 8'h03
                    {OREG_SYOBIC[2:0],OREG_SYOBID[7:0]} = 11'd0
                }
            }
            
            Where:
            T2_frame_length = {2048+FFT_SIZE*(1+GI)*(NDSYM+NP2)}/Ratep/N_TI
            T2_frame_length = (2048 + (FFT_SIZE + FFT_SIZE*GI)*(NDSYM+NP2))/Ratep/N_TI

        */
        sony_dvb_demod_t2_preset_t preset ;
        sony_dvb_dvbt2_plp_t activePLP ;
        uint32_t tsRate = 0 ;
        uint32_t frameLen = 0 ;
        uint8_t numP2 = 0 ;
        uint8_t nTi = 0 ;
        uint8_t syobid = 0x00 ;
        uint8_t syobic = 0x00 ;
        uint8_t syobib = 0x00 ;
        static const uint16_t fftLookup[] = { 2048, 8192, 4096, 1024, 16384, 32768 };
        static const struct { uint8_t num ; uint16_t denom ; } giLookup[] =
        {
           /* 1/32    1/16    1/8    1/4    1/128    19/128    19/256  */
            { 1,32}, {1,16}, {1,8}, {1,4}, {1,128}, {19,128}, {19,256}
        } ;
        
        /* Get the preset information. */
        result = dvb_demod_monitorT2_Preset (pDemod, &preset);
        if (result != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (result);
        }

        /* Confirm Mode/GI */
        if ((preset.mode > SONY_DVB_DVBT2_M32K) || 
            (preset.gi > SONY_DVB_DVBT2_G19_256)) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_RANGE);
        }

        if (!preset.mixed) {
            /* !Mixed */
            syobib = 0x00 ;
            frameLen = SONY_CXD2820_T2_FRAME_LEN_RATE_ADJUST ;
        } 
        else {
            /* Mixed */

            /* Get the active PLP information. */
            result = dvb_demod_monitorT2_ActivePLP (pDemod, SONY_DVB_DVBT2_PLP_DATA, &activePLP);
            if (result != SONY_DVB_OK) {
                SONY_DVB_TRACE_RETURN (result);
            }

            /* Get the TS rate. */
            result = monitorT2_TSRate (pDemod, &tsRate) ;
            if (result != SONY_DVB_OK) {
                SONY_DVB_TRACE_RETURN (result);
            }

            /* Protect against /0, set to 1. */
            if (tsRate == 0) {
                tsRate = 1 ;
            }

            /* Get number of P2 symbols. */
            {
                /* Set bank 0x28 */
                if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B0_REGC_BANK, CXD2820R_B28) != SONY_DVB_OK) {
                    SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
                }

                /* Read registers. IREG_OFDM_NP2 */
                if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B28_IREG_OFDM_NP2, &numP2, sizeof (numP2)) != SONY_DVB_OK) {
                    SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
                }

                numP2 &= 0x1F ;
            }

            /* Calculate the frame length. */
            frameLen = fftLookup[(uint8_t)preset.mode] + 
                (fftLookup[(uint8_t)preset.mode] * giLookup[(uint8_t)preset.gi].num + giLookup[(uint8_t)preset.gi].denom / 2) / giLookup[(uint8_t)preset.gi].denom ;
            frameLen *= (preset.numSymbols + numP2) ;
            frameLen += 2048 ;
            frameLen = (frameLen + tsRate/2) / tsRate ;
            nTi = activePLP.tilLen > 0 ? activePLP.tilLen : 1 ;
            frameLen = (frameLen + nTi / 2) / nTi ;
            syobib = 0x03 ;
            if (frameLen > SONY_CXD2820_T2_FRAME_LEN_RATE_ADJUST) {
                frameLen -= SONY_CXD2820_T2_FRAME_LEN_RATE_ADJUST ;
            }
            else {
                frameLen = 0 ;
            }
        }
        syobic = (uint8_t)(frameLen >> 8) & 0x07 ;
        syobid = (uint8_t)(frameLen & 0xFF) ;

        /* Set bank 0x25 */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B0_REGC_BANK, CXD2820R_B25) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, 0x51, syobib) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        } 
        if (dvb_i2c_SetRegisterBits (pDemod->pI2c, pDemod->i2cAddressT2, 0x52, syobic, 0x07) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, 0x53, syobid) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
    }
    SONY_DVB_TRACE_RETURN (result);
}

static sony_dvb_result_t t2_es1_1_OptimiseTSOutput (sony_dvb_demod_t * pDemod)
{
    sony_dvb_result_t result = SONY_DVB_OK;

    SONY_DVB_TRACE_ENTER ("t2_es1_1_OptimiseTSOutput");
    if ((!pDemod)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* Optimise TS output rate for ES1.1 device. */
    {
        /* 
           set changed_scale = false
           while(!changed_scale) {
              if ( changed_scale == false && IREG_SP_TSLOCK == 1 ) {
                 set changed_scale = true
                 if ( IREG_SYOBIA[7] == 0 ) {
                    OREG_SP_WR_SCALE = 16'h0FFF
                 } else {
                    OREG_SP_WR_SCALE = 16'h0000
                 }
              }
           }
        */
        uint8_t syobia = 0 ;
        sony_dvb_demod_lock_result_t lock ;    
        uint8_t data[2]  ;
        
        /* Freeze Registers */
        result = t2_FreezeReg(pDemod);
        if (result != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (result);
        }

        /* Confirm TS lock. */
        result = dvb_demod_CheckTSLock (pDemod, &lock);
        if (result != SONY_DVB_OK) {
            t2_UnFreezeReg(pDemod) ;
            SONY_DVB_TRACE_RETURN (result);
        }

        if (lock != DEMOD_LOCK_RESULT_LOCKED) {
            t2_UnFreezeReg(pDemod) ;
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_HW_STATE);
        }

        /* Set bank 0x3F */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B0_REGC_BANK, CXD2820R_B3F) != SONY_DVB_OK) {
            t2_UnFreezeReg(pDemod) ;
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B3F_IREG_SYOBIA, &syobia, 1) != SONY_DVB_OK) {
            t2_UnFreezeReg(pDemod) ;
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        /* Unfreeze registers. */
        t2_UnFreezeReg(pDemod) ;

        if ((syobia & 0x80) == 0) {
            data[0] = 0x0F ;
            data[1] = 0xFF ;
        }
        else {
            data[0] = 0x00 ;
            data[1] = 0x00 ;
        }

        /* Set bank 0x20 */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B0_REGC_BANK, CXD2820R_B20) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
        if (pDemod->pI2c->WriteRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B20_WR_SMOOTH_SCALE_11_8, data, sizeof (data)) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
    }

    SONY_DVB_TRACE_RETURN (result);
}

sony_dvb_result_t dvb_demod_t2_OptimiseTSOutput (sony_dvb_demod_t * pDemod)
{
   sony_dvb_result_t result = SONY_DVB_OK;

    SONY_DVB_TRACE_ENTER ("dvb_demod_t2_OptimiseTSOutput");
    if ((!pDemod)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* Software state check */
    if (pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) {
        /* This api is accepted in ACTIVE state only */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    /* Check the running system. */
    if (pDemod->system != SONY_DVB_SYSTEM_DVBT2) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }
    
    /* Optimise TS Output Rate */
    switch (pDemod->chipId)
    {
        case SONY_DVB_CXD2820_ES1_0:
            result = t2_es1_0_OptimiseTSOutput (pDemod);
            break ;
    
        /* Intentional fall-through */
        case SONY_DVB_CXD2820_IP:
        case SONY_DVB_CXD2820_ES1_1:
            result = t2_es1_1_OptimiseTSOutput (pDemod);
            break ;
        
        /* CXD2817 catch. Should never be executed as system never = DVBT2. */
        case SONY_DVB_CXD2817:
            result = SONY_DVB_ERROR_NOSUPPORT ;
            break ;

        /* Intentional fall-through */
        case SONY_DVB_CXD2820_TS:
        default: 
            /* Do nothing. */
            break ;
    }

    SONY_DVB_TRACE_RETURN (result);
}

sony_dvb_result_t dvb_demod_t2_SetSpectrumSense (sony_dvb_demod_t* pDemod, 
                                                 sony_dvb_demod_spectrum_sense_t sense)
{

    SONY_DVB_TRACE_ENTER ("dvb_demod_t2_SetSpectrumSense");
    
    if ((!pDemod)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* Software state check */
    if ((pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) && 
        (pDemod->state != SONY_DVB_DEMOD_STATE_SLEEP)){
        /* This api is accepted in ACTIVE/ASLEEP state */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    /* Device type check. */
    if (pDemod->chipId == SONY_DVB_CXD2817) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_NOSUPPORT);
    }

    {
        /* Set bank 0x20 */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B0_REGC_BANK, CXD2820R_B20) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        /* Set OREG_DNCNV_SRVS (bit 4) */
        if (dvb_i2c_SetRegisterBits (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B20_TDA_INVERT, 
                (uint8_t) (sense == DVB_DEMOD_SPECTRUM_INV ? 0x10 : 0x00), 0x10) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
    }
    SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
}

sony_dvb_result_t dvb_demod_RFAGCWrite (sony_dvb_demod_t* pDemod, uint16_t val)
{
    SONY_DVB_TRACE_ENTER ("dvb_demod_RFAGCWrite");
    
    if ((!pDemod)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* Software state check */
    if (pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) {
        /* This api is accepted in ACTIVE state only */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }
    
    switch (pDemod->system)
    {
        case SONY_DVB_SYSTEM_DVBC:
            
            /* Set bank 0x00 */
            if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressC, CXD2820R_B0_REGC_BANK, 0x00) != SONY_DVB_OK) {
                SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
            }

            {
                uint16_t conv ;

                /* Convert to signed int and then 2's complement. */
                int16_t ival = (int16_t)(val - (0xFFF >> 1)) - 1 ;
                ival /= 4 ;
                conv = ival >= 0 ? (uint16_t) ival : (uint16_t)(ival + 0x3FF + 1);
                {
                    uint8_t data[2] ;
                    data[0] = (uint8_t)((conv >> 8) & 0x03) ;
                    data[1] = (uint8_t)(conv & 0xFF) ;

                    /* Write OREG_RFAGC_LEVEL[9:0] */
                    if (pDemod->pI2c->WriteRegister (pDemod->pI2c, pDemod->i2cAddressC, 0x4B, data, sizeof(data)) != SONY_DVB_OK) {
                        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
                    }
                }
            }
            break ;

        case SONY_DVB_SYSTEM_DVBT:

            /* Set bank 0x01 */
            if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, CXD2820R_B0_REGC_BANK, 0x01) != SONY_DVB_OK) {
                SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
            }

            /* Write OCTL_RFAGCMANUAL_VAL[11:0] */
            {
                uint8_t data[2] ;
                data[0] = (uint8_t)((val >> 8) & 0x0F) ;
                data[1] = (uint8_t)(val & 0xFF) ;

                if (pDemod->pI2c->WriteRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x3A, data, sizeof(data)) != SONY_DVB_OK) {
                    SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
                }
            }
            break ;

        case SONY_DVB_SYSTEM_DVBT2:

            /* Set bank 0x21 */
            if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B0_REGC_BANK, CXD2820R_B21) != SONY_DVB_OK) {
                SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
            }

            /* Write OCTL_RFAGCMANUAL_VAL[11:0] */
            {
                uint8_t data[2] ;
                data[0] = (uint8_t)((val >> 8) & 0x0F) ;
                data[1] = (uint8_t)(val & 0xFF) ;

                if (pDemod->pI2c->WriteRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B21_RFAGC_MAN_11_8, data, sizeof(data)) != SONY_DVB_OK) {
                    SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
                }
            }
            break ;

        /* Intentional fall-through. */
        case SONY_DVB_SYSTEM_UNKNOWN:
        default:
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_OTHER);
    }
    SONY_DVB_TRACE_RETURN (SONY_DVB_OK);

}

static sony_dvb_result_t t2_PrepareAcquisition (sony_dvb_demod_t* pDemod)
{
    static const uint8_t data[] = { 0x0F, 0xFF } ;
    SONY_DVB_TRACE_ENTER ("t2_PrepareAcquisition");
    
    if ((!pDemod)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* Software state check. */
    if ((pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) && 
        (pDemod->state != SONY_DVB_DEMOD_STATE_SLEEP)) {
        /* This api is accepted in ACTIVE/SLEEP states only. */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }
    
    if ((pDemod->chipId == SONY_DVB_CXD2820_ES1_1) || (pDemod->chipId == SONY_DVB_CXD2820_IP)) {
        /* Set bank 0x20 */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B0_REGC_BANK, CXD2820R_B20) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        /* Write OREG_SP_WR_SCALE = 12'hFFF. (Default) */
        if (pDemod->pI2c->WriteRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B20_WR_SMOOTH_SCALE_11_8, data, sizeof (data)) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
    }

    SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
}

static sony_dvb_result_t t2_SetCrlOpen (sony_dvb_demod_t* pDemod, 
                                        uint8_t crlOpen)
{
    SONY_DVB_TRACE_ENTER ("t2_SetCrlOpen");
    
    if (!pDemod) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* Set Bank 0x27. */
    if (pDemod->pI2c->WriteOneRegister(pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B0_REGC_BANK, CXD2820R_B27) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN(SONY_DVB_ERROR_I2C);
    }

    /* Set OREG_CRL_OPEN. */
    if (pDemod->pI2c->WriteOneRegister(pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B27_CRL_OPEN, crlOpen ? 0x01 : 0x00) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN(SONY_DVB_ERROR_I2C);
    }

    SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
}

/*------------------------------------------------------------------------------
  DVB-T2 Long Echo acquisition sequence.
  Based on CXD2820R_driver_sequence_flow.pdf v1.14
------------------------------------------------------------------------------*/

/**
 @brief State function pointer type definition.
*/
typedef sony_dvb_result_t (*dvb_demod_le_seq_func_t) (sony_dvb_le_seq_t* pSeq);

/**
 @brief DVB-T2 PP2 Long Echo sequence ::sony_dvb_le_seq_state_t::SONY_DVB_LE_SEQ_INIT.

 @param pSeq The sequence instance.

 @return SONY_DVB_OK if successful.
*/
static sony_dvb_result_t t2_le_SeqInit (sony_dvb_le_seq_t* pSeq);

/**
 @brief DVB-T2 PP2 Long Echo sequence ::sony_dvb_le_seq_state_t::SONY_DVB_LE_SEQ_START.

 @param pSeq The sequence instance.

 @return SONY_DVB_OK if successful.
*/
static sony_dvb_result_t t2_le_SeqStart (sony_dvb_le_seq_t* pSeq);

/**
 @brief DVB-T2 PP2 Long Echo sequence ::sony_dvb_le_seq_state_t::SONY_DVB_LE_SEQ_WAIT_P1.

 @param pSeq The sequence instance.

 @return SONY_DVB_OK if successful.
*/
static sony_dvb_result_t t2_le_SeqWaitP1 (sony_dvb_le_seq_t* pSeq);

/**
 @brief DVB-T2 PP2 Long Echo sequence ::sony_dvb_le_seq_state_t::SONY_DVB_LE_SEQ_WAIT_LOCK.

 @param pSeq The sequence instance.

 @return SONY_DVB_OK if successful.
*/
static sony_dvb_result_t t2_le_SeqWaitDmdLock (sony_dvb_le_seq_t* pSeq);

/**
 @brief DVB-T2 PP2 Long Echo sequence ::sony_dvb_le_seq_state_t::SONY_DVB_LE_SEQ_TIMING_TRIAL.

 @param pSeq The sequence instance.

 @return SONY_DVB_OK if successful.
*/
static sony_dvb_result_t t2_le_SeqTimingTrial (sony_dvb_le_seq_t* pSeq);

/**
 @brief DVB-T2 PP2 Long Echo sequence ::sony_dvb_le_seq_state_t::SONY_DVB_LE_SEQ_CRL_OPEN_TRIAL.

 @param pSeq The sequence instance.

 @return SONY_DVB_OK if successful.
*/
static sony_dvb_result_t t2_le_SeqCrlOpenTrial (sony_dvb_le_seq_t* pSeq);

/**
 @brief DVB-T2 PP2 Long Echo sequence ::sony_dvb_le_seq_state_t::SONY_DVB_LE_SEQ_SPECTRUM_TRIAL.

 @param pSeq The sequence instance.

 @return SONY_DVB_OK if successful.
*/
static sony_dvb_result_t t2_le_SeqSpectrumTrial (sony_dvb_le_seq_t* pSeq);

sony_dvb_result_t dvb_demod_le_SeqWaitDemodLock (sony_dvb_le_seq_t* pSeq)
{
    sony_dvb_result_t result = SONY_DVB_OK ;

    /* Setup function pointer table for each sequence element. */
    dvb_demod_le_seq_func_t funcs[SONY_DVB_LE_SEQ_UNKNOWN] = { 
        t2_le_SeqInit,               /* SONY_DVB_LE_SEQ_INIT */
        t2_le_SeqStart,              /* SONY_DVB_LE_SEQ_START */
        t2_le_SeqWaitP1,             /* SONY_DVB_LE_SEQ_WAIT_P1 */
        t2_le_SeqWaitDmdLock,        /* SONY_DVB_LE_SEQ_WAIT_LOCK */
        t2_le_SeqCrlOpenTrial,       /* SONY_DVB_LE_SEQ_CRL_OPEN_TRIAL */
        t2_le_SeqTimingTrial,        /* SONY_DVB_LE_SEQ_TIMING_TRIAL */
        t2_le_SeqSpectrumTrial       /* SONY_DVB_LE_SEQ_SPECTRUM_TRIAL */
    };

    SONY_DVB_TRACE_ENTER ("dvb_demod_le_SeqWaitDemodLock");
    if ((!pSeq)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* Ensure FSM state has not been corrupted. */
    if (pSeq->state >= SONY_DVB_LE_SEQ_UNKNOWN) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_OTHER);
    }

    /* Invoke sequence element. */
    result = funcs[pSeq->state] (pSeq);

    SONY_DVB_TRACE_RETURN (result);
}

sony_dvb_result_t dvb_demod_le_SeqWaitTsLock (sony_dvb_le_seq_ts_lock_t* pSeq)
{
    sony_dvb_result_t result = SONY_DVB_OK ;

    SONY_DVB_TRACE_ENTER ("dvb_demod_le_SeqWaitTsLock");
    if ((!pSeq) || (!pSeq->pDemod) || (!pSeq->Sleep) || (pSeq->pollInterval == 0)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* Software state check */
    if (pSeq->pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) {
        /* This api is accepted in ACTIVE state only */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    /* Check the running system. */
    if (pSeq->pDemod->system != SONY_DVB_SYSTEM_DVBT2) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    {
        sony_dvb_demod_lock_result_t lock = DEMOD_LOCK_RESULT_NOTDETECT;
        uint8_t data = 0;

        result = dvb_demod_CheckTSLock (pSeq->pDemod, &lock);
        if (result != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (result);
        }

        if (lock == DEMOD_LOCK_RESULT_LOCKED) {
            
            /* DVB-T2 TS locked, attempt to optimise the Transport Stream output. */
            result = dvb_demod_t2_OptimiseTSOutput (pSeq->pDemod) ;
            if (result == SONY_DVB_ERROR_HW_STATE) {
                /* TS has gone out of lock, continue waiting. */
                result = SONY_DVB_OK ;
                lock = DEMOD_LOCK_RESULT_NOTDETECT ;
            }
            else if (result != SONY_DVB_OK) {
                /* Unexpected error. */
                SONY_DVB_TRACE_RETURN (result);
            }
        }

        if (lock == DEMOD_LOCK_RESULT_LOCKED) {

            /* Set Bank 0x27. */
            if (pSeq->pDemod->pI2c->WriteOneRegister(pSeq->pDemod->pI2c, pSeq->pDemod->i2cAddressT2, CXD2820R_B0_REGC_BANK, CXD2820R_B27) != SONY_DVB_OK) {
                SONY_DVB_TRACE_RETURN(SONY_DVB_ERROR_I2C);
            }

            /* Read OREG_SYR_ACTMODE. */
            if (pSeq->pDemod->pI2c->ReadRegister(pSeq->pDemod->pI2c, pSeq->pDemod->i2cAddressT2, CXD2820R_B27_SYR_ACTMODE, &data, 1) != SONY_DVB_OK) {
                SONY_DVB_TRACE_RETURN(SONY_DVB_ERROR_I2C);
            }

            if ((data & 0x07) == 3) {
                /* Set OREG_SYR_ACTMODE = 7. */
                if (pSeq->pDemod->pI2c->WriteOneRegister(pSeq->pDemod->pI2c, pSeq->pDemod->i2cAddressT2, CXD2820R_B27_SYR_ACTMODE, 0x07) != SONY_DVB_OK) {
                    SONY_DVB_TRACE_RETURN(SONY_DVB_ERROR_I2C);
                }
            }

            /* Complete sequence. TS locked and optimisation performed. */
            pSeq->complete = 1 ;
            SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
        }   
        else {
            if (pSeq->waitTime == DVB_DEMOD_DVBT2_LE_WAIT_SYR) {
                /* Set Bank 0x27. */
                if (pSeq->pDemod->pI2c->WriteOneRegister(pSeq->pDemod->pI2c, pSeq->pDemod->i2cAddressT2, CXD2820R_B0_REGC_BANK, CXD2820R_B27) != SONY_DVB_OK) {
                    SONY_DVB_TRACE_RETURN(SONY_DVB_ERROR_I2C);
                }

                /* Set OREG_SYR_ACTMODE = 3. */
                if (pSeq->pDemod->pI2c->WriteOneRegister(pSeq->pDemod->pI2c, pSeq->pDemod->i2cAddressT2, CXD2820R_B27_SYR_ACTMODE, 0x03) != SONY_DVB_OK) {
                    SONY_DVB_TRACE_RETURN(SONY_DVB_ERROR_I2C);
                }
            }

            /* Check max wait time. */
            if (pSeq->waitTime > DVB_DEMOD_DVBT2_WAIT_TS_LOCK) {
                pSeq->complete = 1 ;
                SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_TIMEOUT);
            }
        }

        /* Perform wait. */
        pSeq->Sleep (pSeq, pSeq->pollInterval) ;

        /* Increment run wait time. */
        pSeq->waitTime += pSeq->pollInterval ;
    }

    SONY_DVB_TRACE_RETURN (result);
}

static sony_dvb_result_t le_seq_Init (sony_dvb_demod_t* pDemod)
{
    sony_dvb_result_t result = SONY_DVB_OK ;

    SONY_DVB_TRACE_ENTER ("le_seq_Init");
    if (!pDemod) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

   /* Set OREG_CRL_OPEN = 0. (Default) */
    result = t2_SetCrlOpen (pDemod, 0);
    if (result != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (result);
    }

    {
        static const uint8_t data[] = { 0x00, 0x00 } ;

        /* Set Bank 0x27. */
        if (pDemod->pI2c->WriteOneRegister(pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B0_REGC_BANK, CXD2820R_B27) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN(SONY_DVB_ERROR_I2C);
        }

        /* Set OREG_GTHETA_OFST0/1 = 0. (Default) */
        if (pDemod->pI2c->WriteRegister(pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B27_GTHETA_OFST0, data, sizeof(data)) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN(SONY_DVB_ERROR_I2C);
        }

        /* Set OREG_SYR_ACTMODE = 7. (Default) */
        if (pDemod->pI2c->WriteOneRegister(pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B27_SYR_ACTMODE, 0x07) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN(SONY_DVB_ERROR_I2C);
        }

        /* Set OREG_FCS_ALPHA_U = 3. Default) */
        if (dvb_i2c_SetRegisterBits(pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B27_FCS_ALPHA, 0x30, 0xF0) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN(SONY_DVB_ERROR_I2C);
        }
    }

    SONY_DVB_TRACE_RETURN (result);
}

static sony_dvb_result_t t2_le_SeqInit (sony_dvb_le_seq_t* pSeq)
{
    sony_dvb_result_t result = SONY_DVB_OK ;

    SONY_DVB_TRACE_ENTER ("t2_le_SeqInit");
    if ((!pSeq) || (!pSeq->pDemod)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* Check the running system. */
    if (pSeq->pDemod->system != SONY_DVB_SYSTEM_DVBT2) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    /* Clear sequencer flags. */
    pSeq->crlOpenTrial = 0;
    pSeq->gThetaMOfst = 0;
    pSeq->gThetaPOfst = 0;
    pSeq->gThetaNoOfst = 0;
    pSeq->spectTrial = 0;
    pSeq->totalWaitTime = 0;
    pSeq->unlockDetDone = 0;
    pSeq->waitTime = 0;
    pSeq->complete = 0;

    result = le_seq_Init (pSeq->pDemod);
    pSeq->state = SONY_DVB_LE_SEQ_START ;
    
    SONY_DVB_TRACE_RETURN (result);
}

static sony_dvb_result_t t2_le_SeqStart (sony_dvb_le_seq_t* pSeq)
{
    sony_dvb_result_t result = SONY_DVB_OK ;

    SONY_DVB_TRACE_ENTER ("t2_le_SeqWaitStart");
    if ((!pSeq) || (!pSeq->pDemod)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* Software state check */
    if ((pSeq->pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) && 
        (pSeq->pDemod->state != SONY_DVB_DEMOD_STATE_SLEEP)){
        /* This api is accepted in ACTIVE/SLEEP states */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    /* Check the running system. */
    if (pSeq->pDemod->system != SONY_DVB_SYSTEM_DVBT2) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    pSeq->waitTime = 0;

    /* (Re)Start acquisition. */
    result = dvb_demod_SoftReset (pSeq->pDemod);
    if (result != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (result);
    }

    /* Assign next sequence state. */
    pSeq->state = SONY_DVB_LE_SEQ_WAIT_P1;

    SONY_DVB_TRACE_RETURN (result);
}

static sony_dvb_result_t t2_le_SeqWaitP1 (sony_dvb_le_seq_t* pSeq)
{
    sony_dvb_result_t result = SONY_DVB_OK ;
    sony_dvb_demod_lock_result_t lock = DEMOD_LOCK_RESULT_NOTDETECT ;

    SONY_DVB_TRACE_ENTER ("t2_le_SeqWaitP1");
    if ((!pSeq) || (!pSeq->pDemod) || (!pSeq->Sleep) || (pSeq->pollInterval == 0)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* Software state check */
    if (pSeq->pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) {
        /* This api is accepted in ACTIVE state only */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    /* Check the running system. */
    if (pSeq->pDemod->system != SONY_DVB_SYSTEM_DVBT2) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    /* Perform wait. */
    pSeq->Sleep (pSeq, pSeq->pollInterval) ;

    /* Increment run wait time. */
    pSeq->waitTime += pSeq->pollInterval ;

    /* Increment overall wait time. */
    pSeq->totalWaitTime += pSeq->pollInterval ;

    /* Check if need to do unlock checks. */
    if (!pSeq->unlockDetDone) {
        
        /* Check for T2/non-T2 P1 symbol. */
        if (pSeq->waitTime == DVB_DEMOD_DVBT2_P1_WAIT) {

            /* Check for any P1. */
            result = dvb_demod_t2_CheckP1Lock (pSeq->pDemod, &lock);
            if (result != SONY_DVB_OK) {
                SONY_DVB_TRACE_RETURN (result);
            }

            /* P1 detected. */
            if (lock == DEMOD_LOCK_RESULT_LOCKED) {
                
                if (pSeq->crlOpenTrial) {

                    /* OREG_CRL_OPEN = 0. */
                    result = t2_SetCrlOpen (pSeq->pDemod, 0);
                    if (result != SONY_DVB_OK) {
                        SONY_DVB_TRACE_RETURN (result);
                    }
                    
                    pSeq->unlockDetDone = 1;
                    pSeq->state = SONY_DVB_LE_SEQ_START ;
                }
                else {
                    pSeq->state = SONY_DVB_LE_SEQ_WAIT_LOCK ;
                }
            }
            else {
                /* No P1 detected, trial CRL open. */
                pSeq->state = SONY_DVB_LE_SEQ_CRL_OPEN_TRIAL ;
            }

            SONY_DVB_TRACE_RETURN (result);
        }

        /* Check for T2 P1 lock only. */
        if (pSeq->waitTime == DVB_DEMOD_DVBT2_T2_P1_WAIT) {

            result = dvb_demod_t2_CheckT2P1Lock (pSeq->pDemod, &lock);
            if (result != SONY_DVB_OK) {
                SONY_DVB_TRACE_RETURN (result);
            }

            /* Indicate UNLOCK if no P1 (no T2). */
            if (lock != DEMOD_LOCK_RESULT_LOCKED) {
                pSeq->complete = 1;
                SONY_DVB_TRACE_RETURN(SONY_DVB_ERROR_UNLOCK);
            }
        }
    }

    /* Assign next sequence state. */
    pSeq->state = SONY_DVB_LE_SEQ_WAIT_LOCK ;

    SONY_DVB_TRACE_RETURN (result);
}

static sony_dvb_result_t t2_le_SeqWaitDmdLock (sony_dvb_le_seq_t* pSeq)
{
    sony_dvb_result_t result = SONY_DVB_OK ;
    uint8_t tsLock = 0;
    uint8_t dmdState = 0;
    uint8_t fcsAlpha = 0;

    SONY_DVB_TRACE_ENTER ("t2_le_SeqWaitDmdLock");
    if ((!pSeq) || (!pSeq->pDemod)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* Software state check */
    if (pSeq->pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) {
        /* This api is accepted in ACTIVE state only */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    /* Check the running system. */
    if (pSeq->pDemod->system != SONY_DVB_SYSTEM_DVBT2) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    result = dvb_demod_monitorT2_SyncStat (pSeq->pDemod, &(dmdState), &(tsLock));
    if (result != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (result);
    }

    if (dmdState == 6) {

        /* Set Bank 0x27. */
        if (pSeq->pDemod->pI2c->WriteOneRegister(pSeq->pDemod->pI2c, pSeq->pDemod->i2cAddressT2, CXD2820R_B0_REGC_BANK, CXD2820R_B27) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN(SONY_DVB_ERROR_I2C);
        }

        /* Read OREG_FCS_ALPHA_U. */
        if (pSeq->pDemod->pI2c->ReadRegister(pSeq->pDemod->pI2c, pSeq->pDemod->i2cAddressT2, CXD2820R_B27_FCS_ALPHA, &fcsAlpha, 1) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN(SONY_DVB_ERROR_I2C);
        }
        
        if ((fcsAlpha & 0xF0) == 0) {
            if (pSeq->pDemod->pI2c->WriteOneRegister(pSeq->pDemod->pI2c, pSeq->pDemod->i2cAddressT2, CXD2820R_B27_FCS_ALPHA, fcsAlpha | 0x30) != SONY_DVB_OK) {
                SONY_DVB_TRACE_RETURN(SONY_DVB_ERROR_I2C);
            }
        }

        /* Demodulator locked. */
        pSeq->complete = 1;
        SONY_DVB_TRACE_RETURN (result);
    }

    if (pSeq->gThetaNoOfst) {
        if (pSeq->waitTime > DVB_DEMOD_DVBT2_LE_WAIT_NO_OFST) {
            /* Error - timeout. */
            pSeq->complete = 1;
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_TIMEOUT);
        }
    }
    else {
        if (pSeq->waitTime > DVB_DEMOD_DVBT2_WAIT_LOCK) {
            /* Error - timeout. */
            pSeq->complete = 1;
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_TIMEOUT);
        }
    }

    /* Check L1 pre. */
    {
        uint8_t l1PreToCnt = 0 ;
        uint8_t seqMaxState = 0 ;

        /* Set Bank 0x20. */
        if (pSeq->pDemod->pI2c->WriteOneRegister(pSeq->pDemod->pI2c, pSeq->pDemod->i2cAddressT2, CXD2820R_B0_REGC_BANK, CXD2820R_B20) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN(SONY_DVB_ERROR_I2C);
        }

        /* Read IREG_SEQ_L1PRE_TO_CNT */
        if (pSeq->pDemod->pI2c->ReadRegister(pSeq->pDemod->pI2c, pSeq->pDemod->i2cAddressT2, CXD2820R_B20_SEQ_L1PRE_TO_CNT, &l1PreToCnt, 1) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN(SONY_DVB_ERROR_I2C);
        }

        if (l1PreToCnt >= 3) {

            /* Read IREG_SEQ_MAX_STATE */
            if (pSeq->pDemod->pI2c->ReadRegister(pSeq->pDemod->pI2c, pSeq->pDemod->i2cAddressT2, CXD2820R_B20_SEQ_NG_OCCURRED_MAX_STATE, &seqMaxState, 1) != SONY_DVB_OK) {
                SONY_DVB_TRACE_RETURN(SONY_DVB_ERROR_I2C);
            }

            /* Check if move to timing trial state. */
            if ((seqMaxState & 0x07) <= 3) {
                pSeq->state = SONY_DVB_LE_SEQ_TIMING_TRIAL;
                SONY_DVB_TRACE_RETURN(SONY_DVB_OK);
            }
        }

        pSeq->state = SONY_DVB_LE_SEQ_WAIT_P1;
    }

    SONY_DVB_TRACE_RETURN (result);
}

static sony_dvb_result_t t2_le_SeqTimingTrial (sony_dvb_le_seq_t* pSeq)
{
    sony_dvb_result_t result = SONY_DVB_OK ;
    uint8_t data[] = { 0, 0 };
    uint8_t syrActMode = 0x03;      /* Set OREG_SYR_ACTMODE = 3. */
    uint8_t fcsAlphaU = 0x00;       /* Set OREG_FCS_ALPHA_U = 0. */

    SONY_DVB_TRACE_ENTER ("t2_le_SeqTimingTrial");
    if ((!pSeq) || (!pSeq->pDemod)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* Software state check */
    if (pSeq->pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) {
        /* This api is accepted in ACTIVE state only */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    /* Check the running system. */
    if (pSeq->pDemod->system != SONY_DVB_SYSTEM_DVBT2) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    /* 
        offset = 250/(4096/2) * 16384 
        offset = 2000
    */
    if (pSeq->gThetaPOfst) {
        if (pSeq->gThetaMOfst) {
            if (pSeq->gThetaNoOfst) {

                /* Move to wait for P1 detection without adjustment. */
                pSeq->state = SONY_DVB_LE_SEQ_WAIT_P1;
                SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
            }
            else {
                /* OREG_GTHETA_OFST = 0us */
                data[0] = 0x00;
                data[1] = 0x00;
                fcsAlphaU = 0x30 ;      /* OREG_FCS_ALPHA_U = 3 */
                syrActMode = 0x07 ;     /* OREG_SYR_ACTMODE = 7 */
                pSeq->gThetaNoOfst = 1 ;
            }
        }
        else {
            /* OREG_GTHETA_OFST = -offset (-2000) -250us */
            data[0] = 0x78;
            data[1] = 0x30;
            pSeq->gThetaMOfst = 1 ;
        }
    }
    else {
        /* OREG_GTHETA_OFST = +offset (2000) +250us  */
        data[0] = 0x07;
        data[1] = 0xD0;
        pSeq->gThetaPOfst = 1;
    }
    pSeq->unlockDetDone = 1; 

    /* Set Bank 0x27. */
    if (pSeq->pDemod->pI2c->WriteOneRegister(pSeq->pDemod->pI2c, pSeq->pDemod->i2cAddressT2, CXD2820R_B0_REGC_BANK, CXD2820R_B27) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN(SONY_DVB_ERROR_I2C);
    }
    
    /* Set GTHETA_OFST0/1. */
    if (pSeq->pDemod->pI2c->WriteRegister(pSeq->pDemod->pI2c, pSeq->pDemod->i2cAddressT2, CXD2820R_B27_GTHETA_OFST0, data, sizeof(data)) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN(SONY_DVB_ERROR_I2C);
    }
    
    if (pSeq->pDemod->pI2c->WriteOneRegister(pSeq->pDemod->pI2c, pSeq->pDemod->i2cAddressT2, CXD2820R_B27_SYR_ACTMODE, syrActMode) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN(SONY_DVB_ERROR_I2C);
    }
    
    if (dvb_i2c_SetRegisterBits(pSeq->pDemod->pI2c, pSeq->pDemod->i2cAddressT2, CXD2820R_B27_FCS_ALPHA, fcsAlphaU, 0xF0) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN(SONY_DVB_ERROR_I2C);
    }

    pSeq->state = SONY_DVB_LE_SEQ_START;

    SONY_DVB_TRACE_RETURN (result);
}

static sony_dvb_result_t t2_le_SeqCrlOpenTrial (sony_dvb_le_seq_t* pSeq)
{
    sony_dvb_result_t result = SONY_DVB_OK ;
    uint8_t data = 0 ;

    SONY_DVB_TRACE_ENTER ("t2_le_SeqCrlOpenTrial");
    if ((!pSeq) || (!pSeq->pDemod)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* Software state check */
    if (pSeq->pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) {
        /* This api is accepted in ACTIVE state only */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    /* Check the running system. */
    if (pSeq->pDemod->system != SONY_DVB_SYSTEM_DVBT2) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    if (pSeq->crlOpenTrial) {

        result = t2_SetCrlOpen (pSeq->pDemod, 0);
        if (result != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (result);
        }

        pSeq->crlOpenTrial = 0;
        pSeq->state = SONY_DVB_LE_SEQ_SPECTRUM_TRIAL;
    }
    else {
        
        /* Set Bank 0x20. */
        if (pSeq->pDemod->pI2c->WriteOneRegister(pSeq->pDemod->pI2c, pSeq->pDemod->i2cAddressT2, CXD2820R_B0_REGC_BANK, CXD2820R_B20) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN(SONY_DVB_ERROR_I2C);
        }

        /* Read IREG_ENP1DET. */
        if (pSeq->pDemod->pI2c->ReadRegister(pSeq->pDemod->pI2c, pSeq->pDemod->i2cAddressT2, CXD2820R_B20_P1DET_STATE, &data, 1) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN(SONY_DVB_ERROR_I2C);
        }
        
        if ((data & 0x40) != 0) {
            /* No P1 detected. */
            pSeq->state = SONY_DVB_LE_SEQ_SPECTRUM_TRIAL;
        }
        else {
            /* P1 detected. */
            /* OREG_CRL_OPEN = 1 */
            result = t2_SetCrlOpen (pSeq->pDemod, 1);
            if (result != SONY_DVB_OK) {
                SONY_DVB_TRACE_RETURN (result);
            }

            pSeq->crlOpenTrial = 1;
            pSeq->state = SONY_DVB_LE_SEQ_START;
        }
    }

    SONY_DVB_TRACE_RETURN (result);
}

static sony_dvb_result_t t2_le_SeqSpectrumTrial (sony_dvb_le_seq_t* pSeq)
{
    sony_dvb_result_t result = SONY_DVB_OK ;
    sony_dvb_demod_spectrum_sense_t sense = DVB_DEMOD_SPECTRUM_NORMAL;

    SONY_DVB_TRACE_ENTER ("t2_le_SeqSpectrumTrial");
    if ((!pSeq) || (!pSeq->pDemod)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* Software state check */
    if (pSeq->pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) {
        /* This api is accepted in ACTIVE state only */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    /* Check the running system. */
    if (pSeq->pDemod->system != SONY_DVB_SYSTEM_DVBT2) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    /* Check to see if user requested auto spectrum detection. */
    if (!pSeq->autoSpectrumDetect) {
        pSeq->complete = 1;
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_UNLOCK);
    }

    /* Flip expected spectrum sense. */
    result = dvb_demod_monitorT2_SpectrumSense (pSeq->pDemod, &sense);
    if (result != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (result);
    }

    result = dvb_demod_t2_SetSpectrumSense (pSeq->pDemod, sense == DVB_DEMOD_SPECTRUM_NORMAL ? DVB_DEMOD_SPECTRUM_INV : DVB_DEMOD_SPECTRUM_NORMAL );
    if (result != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (result);
    }
    
    if (pSeq->spectTrial == 1) {
        pSeq->complete = 1;
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_UNLOCK);
    }
    pSeq->spectTrial = 1;
    pSeq->state = SONY_DVB_LE_SEQ_START;

    SONY_DVB_TRACE_RETURN (result);
}
