/*------------------------------------------------------------------------------

 <dev:header>
    Copyright(c) 2009 Sony Corporation.

    $Revision: 1778 $
    $Author: sking $

</dev:header>

------------------------------------------------------------------------------*/

#include "sony_dvb_demod_monitorT.h"
#include "sony_dvb_math.h"

#define MONITORT_PRERSBER_TIMEOUT              1000
#define MONITORT_PRERSBER_CHECK_PERIOD         10   /* Time in ms between each read of the demod status */

#define t_FreezeReg(pDemod) ((pDemod)->pI2c->WriteOneRegister ((pDemod)->pI2c, (pDemod)->i2cAddressT, 0x01, 0x01))
#define t_UnFreezeReg(pDemod) ((void)((pDemod)->pI2c->WriteOneRegister ((pDemod)->pI2c, (pDemod)->i2cAddressT, 0x01, 0x00))) 

/*------------------------------------------------------------------------------
 Static functions
------------------------------------------------------------------------------*/
static sony_dvb_result_t dvb_demod_monitorT_SNRReg (sony_dvb_demod_t * pDemod, uint16_t * pSNRReg) ;
static sony_dvb_result_t IsTPSLocked (sony_dvb_demod_t * pDemod)
{
    uint8_t sync = 0;
    uint8_t tslock = 0;
    sony_dvb_result_t result = SONY_DVB_OK;

    SONY_DVB_TRACE_ENTER ("IsTPSLocked");

    result = dvb_demod_monitorT_SyncStat (pDemod, &sync, &tslock);
    if (result != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (result);

    if (sync != 6)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_HW_STATE);

    SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
}

/*--------------------------------------------------------------------
  IP ID monitor

  sony_dvb_demod_t *pDemod      Instance of demod control struct
  uint8_t          *pID         IP ID
--------------------------------------------------------------------*/
sony_dvb_result_t dvb_demod_monitorT_IPID (sony_dvb_demod_t * pDemod, uint8_t * pID)
{
    uint8_t rdata = 0x00;

    SONY_DVB_TRACE_ENTER ("dvb_demod_monitorT_IPID");

    if (!pDemod || !pID)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);

    /* Software check is unnecessary */

    /* Set bank 0 */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x00, 0x00) != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);

    /* Read ip_id */
    if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x04, &rdata, 1) != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);

    *pID = rdata;

    SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
}

/*--------------------------------------------------------------------
  Sync state monitor

  sony_dvb_demod_t *pDemod      Instance of demod control struct
  uint8_t          *pSyncStat   Sync state (6: Lock)
  uint8_t          *pTSLockStat TS lock state (1: Lock)
--------------------------------------------------------------------*/
sony_dvb_result_t dvb_demod_monitorT_SyncStat (sony_dvb_demod_t * pDemod, uint8_t * pSyncStat, uint8_t * pTSLockStat)
{
    uint8_t rdata = 0x00;

    SONY_DVB_TRACE_ENTER ("dvb_demod_monitorT_SyncStat");

    if (!pDemod || !pSyncStat || !pTSLockStat)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);

    /* Software state check */
    if (pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) {
        /* This api is accepted in ACTIVE state only */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }
    if (pDemod->system != SONY_DVB_SYSTEM_DVBT) {
        /* Not DVB-T */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    /* Set bank 0 */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x00, 0x00) != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);

    /* Read syncstat0 */
    if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x10, &rdata, 1) != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);

    *pSyncStat = (uint8_t) (rdata & 0x07);

    /* Read fec_status */
    if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x73, &rdata, 1) != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);

    *pTSLockStat = (uint8_t) ((rdata & 0x08) ? 1 : 0);

    SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
}

/*--------------------------------------------------------------------
  IF AGC output monitor

  sony_dvb_demod_t *pDemod      Instance of demod control struct
  uint32_t         *pIFAGCOut   IF AGC output register
--------------------------------------------------------------------*/
sony_dvb_result_t dvb_demod_monitorT_IFAGCOut (sony_dvb_demod_t * pDemod, uint32_t * pIFAGCOut)
{
    uint8_t rdata[2];

    SONY_DVB_TRACE_ENTER ("dvb_demod_monitorT_IFAGCOut");

    if (!pDemod || !pIFAGCOut)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);

    /* Software state check */
    if (pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) {
        /* This api is accepted in ACTIVE state only */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }
    if (pDemod->system != SONY_DVB_SYSTEM_DVBT) {
        /* Not DVB-T */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    /* Set bank 0 */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x00, 0x00) != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);

    /* Read pir_agc_gain */
    if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x26, rdata, 2) != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);

    *pIFAGCOut = ((rdata[0] & 0x0F) << 8) | rdata[1];

    SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
}

/*--------------------------------------------------------------------
  RF AGC output monitor

  sony_dvb_demod_t *pDemod      Instance of demod control struct
  uint32_t         *pRFAGCOut   RF AGC output register
--------------------------------------------------------------------*/
sony_dvb_result_t dvb_demod_monitorT_RFAGCOut (sony_dvb_demod_t * pDemod, uint32_t * pRFAGCOut)
{
    uint8_t rdata[2];

    SONY_DVB_TRACE_ENTER ("dvb_demod_monitorT_RFAGCOut");

    if (!pDemod || !pRFAGCOut)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);

    /* Software state check */
    if (pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) {
        /* This api is accepted in ACTIVE state only */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }
    /* This function is called from DVB-C/T2 mode */

    /* Set bank 0 */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x00, 0x00) != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);

    /* Read ramreg_ramon_adcdata */
    if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressT, 0xDF, rdata, 2) != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);

    *pRFAGCOut = (rdata[0] << 2) | (rdata[1] & 0x03);

    SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
}

/*--------------------------------------------------------------------
  MODE/GUARD monitor

  sony_dvb_demod_t            *pDemod   Instance of demod control struct
  sony_dvb_demod_dvbt_mode_t  *pMode    Mode estimation result
  sony_dvb_demod_dvbt_guard_t *pGuard   Guard interval estimation result
--------------------------------------------------------------------*/
sony_dvb_result_t dvb_demod_monitorT_ModeGuard (sony_dvb_demod_t * pDemod,
                                                sony_dvb_demod_dvbt_mode_t * pMode, sony_dvb_demod_dvbt_guard_t * pGuard)
{
    uint8_t rdata = 0x00;

    SONY_DVB_TRACE_ENTER ("dvb_demod_monitorT_ModeGuard");

    if (!pDemod || !pMode || !pGuard)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);

    /* Software state check */
    if (pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) {
        /* This api is accepted in ACTIVE state only */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }
    if (pDemod->system != SONY_DVB_SYSTEM_DVBT) {
        /* Not DVB-T */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }
    
    /* Freeze registers */
    if (t_FreezeReg(pDemod) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /* TPS Lock check */
    {
        sony_dvb_result_t result = IsTPSLocked (pDemod);
        if (result != SONY_DVB_OK) {
            t_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (result);
        }
    }

    /* Set bank 0 */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x00, 0x00) != SONY_DVB_OK) {
        t_UnFreezeReg(pDemod);
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /* Read syncstat0 */
    if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x40, &rdata, 1) != SONY_DVB_OK) {
        t_UnFreezeReg(pDemod);
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    t_UnFreezeReg(pDemod);

    *pMode = (sony_dvb_demod_dvbt_mode_t) ((rdata >> 2) & 0x03);
    *pGuard = (sony_dvb_demod_dvbt_guard_t) (rdata & 0x03);

    SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
}

/*--------------------------------------------------------------------
  Carrier offset monitor

  sony_dvb_demod_t *pDemod      Instance of demod control struct
  int32_t          *pOffset     Carrier offset value(kHz)
--------------------------------------------------------------------*/
sony_dvb_result_t dvb_demod_monitorT_CarrierOffset (sony_dvb_demod_t * pDemod, int32_t * pOffset)
{
    uint8_t rdata[4];

    SONY_DVB_TRACE_ENTER ("dvb_demod_monitorT_CarrierOffset");

    if (!pDemod || !pOffset)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);

    /* Software state check */
    if (pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) {
        /* This api is accepted in ACTIVE state only */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }
    if (pDemod->system != SONY_DVB_SYSTEM_DVBT) {
        /* Not DVB-T */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    /* Freeze registers */
    if (t_FreezeReg(pDemod) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /* TPS Lock check */
    {
        sony_dvb_result_t result = IsTPSLocked (pDemod);
        if (result != SONY_DVB_OK) {
            t_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (result);
        }
    }

    /* Set bank 0 */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x00, 0x00) != SONY_DVB_OK) {
        t_UnFreezeReg(pDemod);
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /* Read crcg_ctlval */
    if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x4C, rdata, 4) != SONY_DVB_OK) {
        t_UnFreezeReg(pDemod);
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    t_UnFreezeReg(pDemod);

    /* 29bit signed value to 32bit signed value */
    *pOffset = -dvb_Convert2SComplement (((rdata[0] & 0x1F) << 24) | (rdata[1] << 16) | (rdata[2] << 8) | rdata[3], 29);

    switch (pDemod->bandWidth) {
    case 6:
        *pOffset /= 39147;
        break;
    case 7:
        *pOffset /= 33554;
        break;
    case 8:
        *pOffset /= 29360;
        break;
    default:
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

     /* Compensate for inverted spectrum. */
    {
        sony_dvb_demod_spectrum_sense_t detSense = DVB_DEMOD_SPECTRUM_NORMAL ;

        sony_dvb_result_t result = dvb_demod_monitorT_SpectrumSense(pDemod, &detSense);
        if (result != SONY_DVB_OK)
            SONY_DVB_TRACE_RETURN (result);

        /* Inverted RF spectrum with non-inverting tuner architecture. */
        if (detSense == DVB_DEMOD_SPECTRUM_INV) {
            *pOffset *= -1 ;
        }
    }

    SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
}

/*--------------------------------------------------------------------
  MER monitor

  sony_dvb_demod_t *pDemod      Instance of demod control struct
  int32_t          *pCNValue    C/N estimation value (dBx1000)

  int32_t          *pMER        MER value (dBx1000)

  uint32_t         *pMERDT      MER_DT register value
--------------------------------------------------------------------*/
sony_dvb_result_t dvb_demod_monitorT_CNValue (sony_dvb_demod_t * pDemod, int32_t *pCNValue)
{
    sony_dvb_result_t result = SONY_DVB_OK;
    uint32_t merdt = 0;

    SONY_DVB_TRACE_ENTER ("dvb_demod_monitorT_CNValue");

    if (!pDemod || !pCNValue)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);

    /* Software state check */
    if (pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) {
        /* This api is accepted in ACTIVE state only */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }
    if (pDemod->system != SONY_DVB_SYSTEM_DVBT) {
        /* Not DVB-T */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    result = dvb_demod_monitorT_MERDT (pDemod, &merdt);
    if (result != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (result);

    if (merdt == 0) {
        /* log10 function will return INF */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_HW_STATE);
    }

    if (merdt >= 32000) {
        merdt = 31999;
    }
    
    /* 
     C/N = 10 * log10 (MERDT/(32000 - MERDT)) + 33.10
         = 10 * log10 (MERDT) - log10 (32000 - MERDT) + 33.10
     */
    *pCNValue = 10 * 10 * ((int32_t)sony_dvb_log10 (merdt) - (int32_t)sony_dvb_log10 (32000 - merdt)) + 33100 ;

    SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
}

sony_dvb_result_t dvb_demod_monitorT_MER (sony_dvb_demod_t * pDemod, int32_t *pMER)
{
    sony_dvb_result_t result = SONY_DVB_OK;
    uint32_t merdt = 0;
    sony_dvb_demod_dvbt_mode_t mode = DVBT_MODE_2K;
    sony_dvb_demod_dvbt_guard_t guard = DVBT_GUARD_1_32;

    SONY_DVB_TRACE_ENTER ("dvb_demod_monitorT_MER");

    if (!pDemod || !pMER)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);

    /* Software state check */
    if (pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) {
        /* This api is accepted in ACTIVE state only */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }
    if (pDemod->system != SONY_DVB_SYSTEM_DVBT) {
        /* Not DVB-T */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    result = dvb_demod_monitorT_MERDT (pDemod, &merdt);
    if (result != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (result);

    if (merdt == 0) {
        /* log function will return INF */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_HW_STATE);
    }

    result = dvb_demod_monitorT_ModeGuard (pDemod, &mode, &guard);
    if (result != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (result);
    
    /*
     2k:
      MER = 10 * log10 ( ( (merdt/16) / 1705 ) * 1660.0 + 45.0 * 16.0 / 9.0)) ;
          = 10 * ( log10 (merdt) - log10 (16) - log10 (1705) + log10 (1740)) ; 
          = 10 * ( log10 (merdt) - 1.2041 - 3.2201 + 3.2405 )
          = 10 * ( log10 (merdt) - 1.19529 )

     8k:
      MER = 10 * log10 ( ( (merdt/16) / 6817 ) * (6440.0 + 177.0 * 16.0 / 9.0)) ;
          = 10 * ( log10 (merdt) - 1.208109 )

    */
    *pMER = 10 * 10 * ((int32_t)sony_dvb_log10 (merdt)) ;
    switch (mode) {
    case DVBT_MODE_2K:
        *pMER -= 11953 ;
        break;
    case DVBT_MODE_8K:
        *pMER -= 12081 ;
        break;

    /* Intentional fall through. */
    case DVBT_MODE_RESERVED_2:
    case DVBT_MODE_RESERVED_3:
    default:
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_HW_STATE);
    }

    SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
}

sony_dvb_result_t dvb_demod_monitorT_MERDT (sony_dvb_demod_t * pDemod, uint32_t * pMERDT)
{
    uint8_t rdata[2];

    SONY_DVB_TRACE_ENTER ("dvb_demod_monitorT_MERDT");

    if (!pDemod || !pMERDT)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);

    /* Software state check */
    if (pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) {
        /* This api is accepted in ACTIVE state only */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }
    if (pDemod->system != SONY_DVB_SYSTEM_DVBT) {
        /* Not DVB-T */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    /* Freeze registers */
    if (t_FreezeReg(pDemod) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /* TPS Lock check */
    {
        sony_dvb_result_t result = IsTPSLocked (pDemod);
        if (result != SONY_DVB_OK) {
            t_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (result);
        }
    }

    /* Set bank 0 */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x00, 0x00) != SONY_DVB_OK) {
        t_UnFreezeReg(pDemod);
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /* Read mer_dt */
    if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x2C, rdata, 2) != SONY_DVB_OK) {
        t_UnFreezeReg(pDemod);
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    t_UnFreezeReg(pDemod);

    *pMERDT = (rdata[0] << 8) | rdata[1];

    SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
}

/*--------------------------------------------------------------------
  SNR monitor

  sony_dvb_demod_t *pDemod      Instance of demod control struct
  int32_t          *pSNR        SNR(dBx1000)

  uint16_t         *pSNRReg     IREG_SNMON_OD register value
--------------------------------------------------------------------*/
sony_dvb_result_t dvb_demod_monitorT_SNR (sony_dvb_demod_t * pDemod, int32_t *pSNR)
{
    sony_dvb_result_t result = SONY_DVB_OK;
    uint16_t reg = 0;

    SONY_DVB_TRACE_ENTER ("dvb_demod_monitorT_SNR");

    if (!pDemod || !pSNR)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);

    /* Software state check */
    if (pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) {
        /* This api is accepted in ACTIVE state only */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }
    if (pDemod->system != SONY_DVB_SYSTEM_DVBT) {
        /* Not DVB-T */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    result = dvb_demod_monitorT_SNRReg (pDemod, &reg);
    if (result != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (result);

    if (reg == 0) {
        /* log10 function will return INF */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_HW_STATE);
    }
    
    /*
      SNR = 10 * log10 (reg / 8)
          = 10 * log10 (reg) - log10(8)

      Where: 
          log10(8) = 0.90308
    */
    *pSNR = 10 * 10 * (int32_t)sony_dvb_log10 (reg) ;
    *pSNR -= 9031 ;

    SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
}

static sony_dvb_result_t dvb_demod_monitorT_SNRReg (sony_dvb_demod_t * pDemod, uint16_t * pSNRReg)
{
    uint8_t rdata[2];

    SONY_DVB_TRACE_ENTER ("dvb_demod_monitorT_SNRReg");

    if (!pDemod || !pSNRReg)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);

    /* Software state check */
    if (pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) {
        /* This api is accepted in ACTIVE state only */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }
    if (pDemod->system != SONY_DVB_SYSTEM_DVBT) {
        /* Not DVB-T */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    /* Freeze registers */
    if (t_FreezeReg(pDemod) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /* TPS Lock check */
    {
        sony_dvb_result_t result = IsTPSLocked (pDemod);
        if (result != SONY_DVB_OK) {
            t_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (result);
        }
    }

    /* Set bank 0 */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x00, 0x00) != SONY_DVB_OK) {
        t_UnFreezeReg(pDemod);
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /* Read snmon_od */
    if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x28, rdata, 2) != SONY_DVB_OK) {
        t_UnFreezeReg(pDemod);
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    t_UnFreezeReg(pDemod);

    *pSNRReg = ((rdata[0] & 0x1F) << 8) | rdata[1];

    SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
}

/*--------------------------------------------------------------------
  Pre Viterbi BER monitor

  sony_dvb_demod_t *pDemod      Instance of demod control struct
  uint32_t         *pBER        BER value (Pre viterbi decoder)

  uint32_t         *pBitError   Bit error count
  uint32_t         *pPeriod     Period(bit) of BER measurement
--------------------------------------------------------------------*/
sony_dvb_result_t dvb_demod_monitorT_PreViterbiBER (sony_dvb_demod_t * pDemod, uint32_t *pBER)
{
    sony_dvb_result_t result = SONY_DVB_OK;
    uint32_t bitError = 0;
    uint32_t period = 0;

    SONY_DVB_TRACE_ENTER ("dvb_demod_monitorT_PreViterbiBER");

    if (!pDemod || !pBER)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);

    /* Software state check */
    if (pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) {
        /* This api is accepted in ACTIVE state only */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }
    if (pDemod->system != SONY_DVB_SYSTEM_DVBT) {
        /* Not DVB-T */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    result = dvb_demod_monitorT_PreViterbiBitError (pDemod, &bitError, &period);
    if (result != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (result);

    /* Protect against /0. */
    if ((period == 0) || (bitError > period)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_HW_STATE);
    }
    
    /*
        BER = bitError / period ;

        Assume:
        Q = quotient = period / bitError ;
        R = remainder = periond % bitError ;

        So:
        period = Q.bitError + R
    */    
    {
        if (bitError == 0) {
            *pBER = 0 ;
            SONY_DVB_TRACE_RETURN(result);
        }
        *pBER = ((10 * 1000000) << 4) ;
        *pBER  /= (((period/bitError) << 4) + (((period%bitError) << 4) / bitError)) ;
    }

    SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
}

sony_dvb_result_t dvb_demod_monitorT_PreViterbiBitError (sony_dvb_demod_t * pDemod, uint32_t * pBitError, uint32_t * pPeriod)
{
    uint8_t rdata[2];

    SONY_DVB_TRACE_ENTER ("dvb_demod_monitorT_PreViterbiBitError");

    if (!pDemod || !pBitError || !pPeriod)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);

    /* Software state check */
    if (pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) {
        /* This api is accepted in ACTIVE state only */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }
    if (pDemod->system != SONY_DVB_SYSTEM_DVBT) {
        /* Not DVB-T */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    /* Freeze registers */
    if (t_FreezeReg(pDemod) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /* TPS Lock check */
    {
        sony_dvb_result_t result = IsTPSLocked (pDemod);
        if (result != SONY_DVB_OK) {
            t_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (result);
        }
    }

    /* Set bank 0 */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x00, 0x00) != SONY_DVB_OK) {
        t_UnFreezeReg(pDemod);
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /* Read vber_biecnt */
    if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x22, rdata, 2) != SONY_DVB_OK) {
        t_UnFreezeReg(pDemod);
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    *pBitError = (rdata[0] << 8) | rdata[1];

    /* Read vber_period */
    if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x6F, rdata, 1) != SONY_DVB_OK) {
        t_UnFreezeReg(pDemod);
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    t_UnFreezeReg(pDemod);

    *pPeriod = 0x1000 << (rdata[0] & 0x07);

    SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
}

/*--------------------------------------------------------------------
  Pre RS BER monitor

  sony_dvb_demod_t *pDemod      Instance of demod control struct
  uint32_t         *pBER        BER value (Pre reed solomon decoder)

  uint32_t         *pBitError   Bit error count
  uint32_t         *pPeriod     Period(bit) of BER measurement
--------------------------------------------------------------------*/
sony_dvb_result_t dvb_demod_monitorT_PreRSBER (sony_dvb_demod_t * pDemod, uint32_t *pBER)
{
    sony_dvb_result_t result = SONY_DVB_OK;
    uint32_t bitError = 0;
    uint32_t period = 0;

    SONY_DVB_TRACE_ENTER ("dvb_demod_monitorT_PreRSBER");

    if (!pDemod || !pBER)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);

    /* Software state check */
    if (pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) {
        /* This api is accepted in ACTIVE state only */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }
    if (pDemod->system != SONY_DVB_SYSTEM_DVBT) {
        /* Not DVB-T */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    result = dvb_demod_monitorT_PreRSBitError (pDemod, &bitError, &period);
    if (result != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (result);

    /* Protect against /0. */
    if ((period == 0) || (bitError > period)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_HW_STATE);
    }

    /*
        BER = bitError / period ;

        Assume:
        Q = quotient = period / bitError ;
        R = remainder = periond % bitError ;

        So:
        period = Q.bitError + R
    */
    {
        if (bitError == 0) {
            *pBER = 0 ;
            SONY_DVB_TRACE_RETURN(result);
        }
        *pBER = ((10 * 1000000) << 4) ;
        *pBER  /= (((period/bitError) << 4) + (((period%bitError) << 4) / bitError)) ;
    }

    SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
}

sony_dvb_result_t dvb_demod_monitorT_PreRSBitError (sony_dvb_demod_t * pDemod, uint32_t * pBitError, uint32_t * pPeriod)
{
    uint8_t rdata[3];
    int index = 0;

    SONY_DVB_TRACE_ENTER ("dvb_demod_monitorT_PreRSBitError");

    if (!pDemod || !pBitError || !pPeriod)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);

    /* Software state check */
    if (pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) {
        /* This api is accepted in ACTIVE state only */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }
    if (pDemod->system != SONY_DVB_SYSTEM_DVBT) {
        /* Not DVB-T */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    /* TPS Lock check */
    {
        sony_dvb_result_t result = IsTPSLocked (pDemod);
        if (result != SONY_DVB_OK)
            SONY_DVB_TRACE_RETURN (result);
    }

    /* Set bank 0 */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x00, 0x00) != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);

    /* Set ber_set (Bit error count start) */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x79, 0x01) != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);

    rdata[0] = rdata[1] = rdata[2] = 0;
    for (index = 0; index < (MONITORT_PRERSBER_TIMEOUT / MONITORT_PRERSBER_CHECK_PERIOD); index++) {
        SONY_DVB_SLEEP (MONITORT_PRERSBER_CHECK_PERIOD);
        /* Read ber_estimate */
        if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x76, rdata, 3) != SONY_DVB_OK)
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        /* Check IREG_BER_RDY bit (bit 7) */
        if (rdata[2] & 0x80)
            break;
    }

    /* Check IREG_BER_RDY bit (bit 7) */
    if ((rdata[2] & 0x80) == 0)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_HW_STATE);    /* Not ready... */

    *pBitError = ((rdata[2] & 0x0F) << 16) | (rdata[1] << 8) | rdata[0];

    /* Read ber_period */
    if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x72, rdata, 1) != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);

    *pPeriod = (1 << (rdata[0] & 0x1F)) * 204 * 8;

    SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
}

/*--------------------------------------------------------------------
  RS error monitor

  sony_dvb_demod_t *pDemod      Instance of demod control struct
  uint32_t         *pRSError    RS error count
--------------------------------------------------------------------*/
sony_dvb_result_t dvb_demod_monitorT_RSError (sony_dvb_demod_t * pDemod, uint32_t * pRSError)
{
    uint8_t rdata[2];

    SONY_DVB_TRACE_ENTER ("dvb_demod_monitorT_RSError");

    if (!pDemod || !pRSError)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);

    /* Software state check */
    if (pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) {
        /* This api is accepted in ACTIVE state only */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }
    if (pDemod->system != SONY_DVB_SYSTEM_DVBT) {
        /* Not DVB-T */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    /* Freeze registers */
    if (t_FreezeReg(pDemod) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /* TPS Lock check */
    {
        sony_dvb_result_t result = IsTPSLocked (pDemod);
        if (result != SONY_DVB_OK) {
            t_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (result);
        }
    }

    /* Set bank 0 */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x00, 0x00) != SONY_DVB_OK) {
        t_UnFreezeReg(pDemod);
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /* Read ber_cwrjctcnt */
    if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressT, 0xEA, rdata, 2) != SONY_DVB_OK) {
        t_UnFreezeReg(pDemod);
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    t_UnFreezeReg(pDemod);

    *pRSError = (rdata[0] << 8) | rdata[1];

    SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
}

/*--------------------------------------------------------------------
  TPS information monitor

  sony_dvb_demod_t          *pDemod  Instance of demod control struct
  sony_dvb_demod_tpsinfo_t  *pInfo   TPS information struct
--------------------------------------------------------------------*/
sony_dvb_result_t dvb_demod_monitorT_TPSInfo (sony_dvb_demod_t * pDemod, sony_dvb_demod_tpsinfo_t * pInfo)
{
    uint8_t rdata[7];

    SONY_DVB_TRACE_ENTER ("dvb_demod_monitorT_TPSInfo");

    if (!pDemod || !pInfo)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);

    /* Software state check */
    if (pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) {
        /* This api is accepted in ACTIVE state only */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }
    if (pDemod->system != SONY_DVB_SYSTEM_DVBT) {
        /* Not DVB-T */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    /* Freeze registers */
    if (t_FreezeReg(pDemod) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /* TPS Lock check */
    {
        sony_dvb_result_t result = IsTPSLocked (pDemod);
        if (result != SONY_DVB_OK) {
            t_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (result);
        }
    }

    /* Set bank 0 */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x00, 0x00) != SONY_DVB_OK) {
        t_UnFreezeReg(pDemod);
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /* Read tps_info_mon */
    if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x2F, rdata, 7) != SONY_DVB_OK) {
        t_UnFreezeReg(pDemod);
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }
    
    t_UnFreezeReg(pDemod);

    pInfo->constellation = (sony_dvb_demod_dvbt_constellation_t) ((rdata[0] >> 6) & 0x03);  /* 0x2F, bit[7:6] */
    pInfo->hierarchy = (sony_dvb_demod_dvbt_hierarchy_t) ((rdata[0] >> 3) & 0x07);  /* 0x2F, bit[5:3] */
    pInfo->rateHP = (sony_dvb_demod_dvbt_coderate_t) (rdata[0] & 0x07); /* 0x2F, bit[2:0] */
    pInfo->rateLP = (sony_dvb_demod_dvbt_coderate_t) ((rdata[1] >> 5) & 0x07);  /* 0x30, bit[7:5] */
    pInfo->guard = (sony_dvb_demod_dvbt_guard_t) ((rdata[1] >> 3) & 0x03);  /* 0x30, bit[4:3] */
    pInfo->mode = (sony_dvb_demod_dvbt_mode_t) ((rdata[1] >> 1) & 0x03);    /* 0x30, bit[2:1] */
    pInfo->cellID = (uint16_t) ((rdata[3] << 8) | rdata[4]);    /* 0x32, 0x33 */

    SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
}

sony_dvb_result_t dvb_demod_monitorT_SpectrumSense (sony_dvb_demod_t* pDemod,
                                                    sony_dvb_demod_spectrum_sense_t* pSense)
{
   SONY_DVB_TRACE_ENTER ("dvb_demod_monitorT_SpectrumSense");

    if (!pDemod || !pSense)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);

    /* Software state check */
    if (pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) {
        /* This api is accepted in ACTIVE state only */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }
    if (pDemod->system != SONY_DVB_SYSTEM_DVBT) {
        /* Not DVB-T */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    {
        uint8_t data = 0 ;

        /* Set bank 0x07 */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x00, 0x07) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressT, 0xC6, &data, sizeof(data)) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        *pSense = (data & 0x01) ? DVB_DEMOD_SPECTRUM_INV : DVB_DEMOD_SPECTRUM_NORMAL ;
    }

    SONY_DVB_TRACE_RETURN (SONY_DVB_OK);

}

sony_dvb_result_t dvb_demod_monitorT_SamplingOffset (sony_dvb_demod_t * pDemod, 
                                                     int32_t* pPPM)
{
    SONY_DVB_TRACE_ENTER ("dvb_demod_monitorT_SamplingOffset");

    if ((!pDemod) || (!pPPM)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* Software state check */
    if (pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) {
        /* This api is accepted in ACTIVE state only */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    if (pDemod->system != SONY_DVB_SYSTEM_DVBT) {
        /* Not DVB-T */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    /* Freeze registers */
    if (t_FreezeReg(pDemod) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /* TPS Lock check */
    {
        sony_dvb_result_t result = IsTPSLocked (pDemod);
        if (result != SONY_DVB_OK) {
            t_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (result);
        }
    }

    {
        uint8_t data [5] ;
        uint32_t trlMon2 = 0;

        /* Set bank 0x00 */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x00, 0x00) != SONY_DVB_OK) {
            t_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        /* Read IREG_TRL_CTLVAL_S */
        if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressT, 0x52, data, sizeof(data)) != SONY_DVB_OK) {
            t_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        t_UnFreezeReg(pDemod);

        /* Ignore top bits - not significant. */
        trlMon2 =  data[1] << 24 ;
        trlMon2 |= data[2] << 16 ;
        trlMon2 |= data[3] << 8 ;
        trlMon2 |= data[4] ;

        switch (pDemod->bandWidth) {
            case 6:
                /* (1 / 0x17EAAAAAAA) * 1e12 * 64 = 623.0 */
                /* Confirm offset range. */
                if ((data[0] & 0x7F) != 0x17) {
                    SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_HW_STATE);
                }
                *pPPM = (int32_t)(trlMon2 - 0xEAAAAAAA) ;
                *pPPM = (*pPPM + 4) / 8 ;
                *pPPM *= 623 ;
                break ;
            case 7:
                /* (1 / 0x1480000000 ) * 1e12 * 64 = 726.9 */
                /* Confirm offset range. */
                if ((data[0] & 0x7F) != 0x14) {
                    SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_HW_STATE);
                }
                *pPPM = (int32_t)(trlMon2 - 0x80000000) ;
                *pPPM = (*pPPM + 4) / 8 ;
                *pPPM *= 727 ;
                break ;
            case 8:
                /* (1 / 0x11F0000000) * 1e12 * 64 = 830.7 */
                /* Confirm offset range. */
                if ((data[0] & 0x7F) != 0x11) {
                    SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_HW_STATE);
                }
                *pPPM = (int32_t)(trlMon2 - 0xF0000000) ;
                *pPPM = (*pPPM + 4) / 8 ;
                *pPPM *= 831 ;
                break ;
            default:
                SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_OTHER);
        }

        *pPPM = (*pPPM + (5000 * 8)) / (10000 * 8) ;
    }

    SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
}
