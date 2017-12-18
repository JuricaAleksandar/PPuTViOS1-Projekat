/*------------------------------------------------------------------------------

 <dev:header>
    Copyright(c) 2009 Sony Corporation.

    $Revision: 1778 $
    $Author: sking $

</dev:header>

------------------------------------------------------------------------------*/

#include "sony_dvb_demod_monitorC.h"
#include "sony_dvb_demod_monitorT.h"
#include "sony_dvb_math.h"

#define MONITORC_PRERSBER_TIMEOUT              1000
#define MONITORC_PRERSBER_CHECK_PERIOD         10   /* Time in ms between each read of the demod status */

#define c_FreezeReg(pDemod) ((pDemod)->pI2c->WriteOneRegister ((pDemod)->pI2c, (pDemod)->i2cAddressC, 0x01, 0x01))
#define c_UnFreezeReg(pDemod) ((void)((pDemod)->pI2c->WriteOneRegister ((pDemod)->pI2c, (pDemod)->i2cAddressC, 0x01, 0x00)))  

/*------------------------------------------------------------------------------
 Static functions
------------------------------------------------------------------------------*/
static sony_dvb_result_t IsARLocked (sony_dvb_demod_t * pDemod)
{
    uint8_t ar = 0;
    uint8_t tslock = 0;
    sony_dvb_result_t result = SONY_DVB_OK;

    SONY_DVB_TRACE_ENTER ("IsARLocked");

    result = dvb_demod_monitorC_SyncStat (pDemod, &ar, &tslock);
    if (result != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (result);

    if (ar != 1)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_HW_STATE);

    SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
}

/*--------------------------------------------------------------------
  Sync state monitor

  sony_dvb_demod_t *pDemod           Instance of demod control struct
  uint8_t          *pARStat          AutoRecovery state (0: Not Completed, 1: Lock, 2: Timeout)
  uint8_t          *pTSLockStat      TS lock state (1: Lock)
--------------------------------------------------------------------*/
sony_dvb_result_t dvb_demod_monitorC_SyncStat (sony_dvb_demod_t * pDemod, uint8_t * pARStat, uint8_t * pTSLockStat)
{
    uint8_t rdata = 0x00;

    SONY_DVB_TRACE_ENTER ("dvb_demod_monitorC_SyncStat");

    if (!pDemod || !pARStat || !pTSLockStat)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);

    /* Software state check */
    if (pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) {
        /* This api is accepted in ACTIVE state only */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }
    if (pDemod->system != SONY_DVB_SYSTEM_DVBC) {
        /* Not DVB-C */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    /* Set bank 0 */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressC, 0x00, 0x00) != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);

    /* Read ar_status1 */
    if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressC, 0x88, &rdata, 1) != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);

    *pARStat = (uint8_t) (rdata & 0x03);    /* IREG_AR_TIMEOUT | IREG_AR_LOCK */

    /* Read fec_status */
    if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressC, 0x73, &rdata, 1) != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);

    *pTSLockStat = (uint8_t) ((rdata & 0x08) ? 1 : 0);

    SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
}

/*--------------------------------------------------------------------
  IF AGC output monitor

  sony_dvb_demod_t *pDemod      Instance of demod control struct
  int32_t          *pIFAGCOut   IF AGC output register (signed)
--------------------------------------------------------------------*/
sony_dvb_result_t dvb_demod_monitorC_IFAGCOut (sony_dvb_demod_t * pDemod, int32_t * pIFAGCOut)
{
    uint8_t rdata[2];

    SONY_DVB_TRACE_ENTER ("dvb_demod_monitorC_IFAGCOut");

    if (!pDemod || !pIFAGCOut)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);

    /* Software state check */
    if (pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) {
        /* This api is accepted in ACTIVE state only */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }
    if (pDemod->system != SONY_DVB_SYSTEM_DVBC) {
        /* Not DVB-C */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    /* Set bank 0 */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressC, 0x00, 0x00) != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);

    /* Read xgcinteg */
    if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressC, 0x49, rdata, 2) != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);

    *pIFAGCOut = dvb_Convert2SComplement (((rdata[0] & 0x03) << 8) | rdata[1], 10);

    SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
}

/*--------------------------------------------------------------------
  RF AGC output monitor

  sony_dvb_demod_t *pDemod      Instance of demod control struct
  uint32_t         *pRFAGCOut   RF AGC output register
--------------------------------------------------------------------*/
sony_dvb_result_t dvb_demod_monitorC_RFAGCOut (sony_dvb_demod_t * pDemod, uint32_t * pRFAGCOut)
{
    sony_dvb_result_t result = SONY_DVB_OK;

    SONY_DVB_TRACE_ENTER ("dvb_demod_monitorC_RFAGCOut");

    /* Same as DVB-T */
    result = dvb_demod_monitorT_RFAGCOut (pDemod, pRFAGCOut);

    SONY_DVB_TRACE_RETURN (result);
}

/*--------------------------------------------------------------------
  Constellation monitor

  sony_dvb_demod_t                    *pDemod Instance of demod control struct
  sony_dvb_demod_dvbc_constellation_t *pQAM   Constellation
--------------------------------------------------------------------*/
sony_dvb_result_t dvb_demod_monitorC_QAM (sony_dvb_demod_t * pDemod, sony_dvb_demod_dvbc_constellation_t * pQAM)
{
    uint8_t rdata = 0x00;

    SONY_DVB_TRACE_ENTER ("dvb_demod_monitorC_QAM");

    if (!pDemod || !pQAM)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);

    /* Software state check */
    if (pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) {
        /* This api is accepted in ACTIVE state only */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }
    if (pDemod->system != SONY_DVB_SYSTEM_DVBC) {
        /* Not DVB-C */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    /* Freeze registers */
    if (c_FreezeReg(pDemod) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /* AR lock check */
    {
        sony_dvb_result_t result = IsARLocked (pDemod);
        if (result != SONY_DVB_OK) {
            c_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (result);
        }
    }

    /* Set bank 0 */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressC, 0x00, 0x00) != SONY_DVB_OK) {
        c_UnFreezeReg(pDemod);
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /* Read detectedqam */
    if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressC, 0x19, &rdata, 1) != SONY_DVB_OK) {
        c_UnFreezeReg(pDemod);
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    c_UnFreezeReg(pDemod);

    *pQAM = (sony_dvb_demod_dvbc_constellation_t) (rdata & 0x07);

    SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
}

/*--------------------------------------------------------------------
  Symbol rate monitor

  sony_dvb_demod_t *pDemod           Instance of demod control struct
  uint32_t         *pSymRate         Symbol rate (kSymbol/sec)
--------------------------------------------------------------------*/
sony_dvb_result_t dvb_demod_monitorC_SymbolRate (sony_dvb_demod_t * pDemod, uint32_t * pSymRate)
{
    uint8_t rdata[2];

    SONY_DVB_TRACE_ENTER ("dvb_demod_monitorC_SymbolRate");

    if (!pDemod || !pSymRate)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);

    /* Software state check */
    if (pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) {
        /* This api is accepted in ACTIVE state only */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }
    if (pDemod->system != SONY_DVB_SYSTEM_DVBC) {
        /* Not DVB-C */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    /* Freeze registers */
    if (c_FreezeReg(pDemod) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /* AR lock check */
    {
        sony_dvb_result_t result = IsARLocked (pDemod);
        if (result != SONY_DVB_OK) {
            c_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (result);
        }
    }

    /* Set bank 0 */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressC, 0x00, 0x00) != SONY_DVB_OK) {
        c_UnFreezeReg(pDemod);
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /* Read dtectdsymrt */
    if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressC, 0x1A, rdata, 2) != SONY_DVB_OK) {
        c_UnFreezeReg(pDemod);
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    c_UnFreezeReg(pDemod);

    *pSymRate = (((((rdata[0] & 0x0F) << 8) | rdata[1]) * 5) + 1) / 2;  /* 0.0025 * 1000 */

    SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
}

/*--------------------------------------------------------------------
  Carrier offset monitor

  sony_dvb_demod_t *pDemod      Instance of demod control struct
  int32_t          *pOffset     Carrier offset value(kHz)
--------------------------------------------------------------------*/
sony_dvb_result_t dvb_demod_monitorC_CarrierOffset (sony_dvb_demod_t * pDemod, int32_t * pOffset)
{
    uint8_t rdata[2];

    SONY_DVB_TRACE_ENTER ("dvb_demod_monitorC_CarrierOffset");

    if (!pDemod || !pOffset)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);

    /* Software state check */
    if (pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) {
        /* This api is accepted in ACTIVE state only */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }
    if (pDemod->system != SONY_DVB_SYSTEM_DVBC) {
        /* Not DVB-C */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    /* Freeze registers */
    if (c_FreezeReg(pDemod) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /* AR lock check */
    {
        sony_dvb_result_t result = IsARLocked (pDemod);
        if (result != SONY_DVB_OK) {
            c_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (result);
        }
    }

    /* Set bank 0 */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressC, 0x00, 0x00) != SONY_DVB_OK) {
        c_UnFreezeReg(pDemod);
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /* Read freq_ost */
    if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressC, 0x15, rdata, 2) != SONY_DVB_OK) {
        c_UnFreezeReg(pDemod);
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    c_UnFreezeReg(pDemod);

    /* 14bit signed value to 32bit signed value */
    *pOffset = dvb_Convert2SComplement (((rdata[0] & 0x3F) << 8) | rdata[1], 14);
    *pOffset = *pOffset * 41000 / 16384;    /* ADC Clock(41MHz) / 16384 */

    /* Compensate offset for inverting tuner achitecture. */
    if (pDemod->confSense == DVB_DEMOD_SPECTRUM_INV) {
        *pOffset *= -1;
    }

    SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
}

/*--------------------------------------------------------------------
  Spectrum sense monitor

  sony_dvb_demod_t *pDemod      Instance of demod control struct
  int              *pSense   Spectrum is inverted or not(0: normal, 1: inverted)
--------------------------------------------------------------------*/
sony_dvb_result_t dvb_demod_monitorC_SpectrumSense (sony_dvb_demod_t * pDemod, sony_dvb_demod_spectrum_sense_t *pSense)
{
    uint8_t rdata = 0x00;

    SONY_DVB_TRACE_ENTER ("dvb_demod_monitorC_SpectrumSense");

    if (!pDemod || !pSense)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);

    /* Software state check */
    if (pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) {
        /* This api is accepted in ACTIVE state only */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }
    if (pDemod->system != SONY_DVB_SYSTEM_DVBC) {
        /* Not DVB-C */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    /* Freeze registers */
    if (c_FreezeReg(pDemod) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /* AR lock check */
    {
        sony_dvb_result_t result = IsARLocked (pDemod);
        if (result != SONY_DVB_OK) {
            c_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (result);
        }
    }

    /* Set bank 0 */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressC, 0x00, 0x00) != SONY_DVB_OK) {
        c_UnFreezeReg(pDemod);
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /* Read detectedqam */
    if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressC, 0x19, &rdata, 1) != SONY_DVB_OK) {
        c_UnFreezeReg(pDemod);
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    c_UnFreezeReg(pDemod);

    /* NOTE: DVB-C spectrum inversion setup/monitor register logic is opposite to DVB-T. */
    *pSense = (rdata & 0x80) ? DVB_DEMOD_SPECTRUM_NORMAL : DVB_DEMOD_SPECTRUM_INV ;

    SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
}

/*--------------------------------------------------------------------
  SNR monitor

  sony_dvb_demod_t *pDemod      Instance of demod control struct
  int32_t           *pSNR       SNR(dB x 1000)

  uint8_t          *pSNRReg     IREG_SNR_ESTIMATE register value
--------------------------------------------------------------------*/
sony_dvb_result_t dvb_demod_monitorC_SNR (sony_dvb_demod_t * pDemod, int32_t *pSNR)
{
    sony_dvb_demod_dvbc_constellation_t qam = DVBC_CONSTELLATION_16QAM;
    uint8_t reg = 0x00;
    sony_dvb_result_t result = SONY_DVB_OK;

    SONY_DVB_TRACE_ENTER ("dvb_demod_monitorC_SNR");

    if (!pDemod || !pSNR)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);

    /* Software state check */
    if (pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) {
        /* This api is accepted in ACTIVE state only */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }
    if (pDemod->system != SONY_DVB_SYSTEM_DVBC) {
        /* Not DVB-C */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    result = dvb_demod_monitorC_QAM (pDemod, &qam); /* Get constellation */
    if (result != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (result);

    result = dvb_demod_monitorC_SNRReg (pDemod, &reg);
    if (result != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (result);

    if (reg == 0) {
        /* log function will return INF */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_HW_STATE);
    }

    switch (qam) {
    case DVBC_CONSTELLATION_16QAM:
    case DVBC_CONSTELLATION_64QAM:
    case DVBC_CONSTELLATION_256QAM:
        *pSNR = -95 * (int32_t)sony_dvb_log (reg) + 63017 ;
        break;
    case DVBC_CONSTELLATION_32QAM:
    case DVBC_CONSTELLATION_128QAM:
        *pSNR = (-875 * (int32_t)sony_dvb_log (reg) + 566735) / 10 ;
        break;
    default:
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_HW_STATE);    /* Unknown value... */
    }

    SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
}

sony_dvb_result_t dvb_demod_monitorC_SNRReg (sony_dvb_demod_t * pDemod, uint8_t * pSNRReg)
{
    uint8_t rdata = 0x00;

    SONY_DVB_TRACE_ENTER ("dvb_demod_monitorC_SNRReg");

    if (!pDemod || !pSNRReg)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);

    /* Software state check */
    if (pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) {
        /* This api is accepted in ACTIVE state only */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }
    if (pDemod->system != SONY_DVB_SYSTEM_DVBC) {
        /* Not DVB-C */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    /* Freeze registers */
    if (c_FreezeReg(pDemod) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /* AR lock check */
    {
        sony_dvb_result_t result = IsARLocked (pDemod);
        if (result != SONY_DVB_OK) {
            c_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (result);
        }
    }

    /* Set bank 0 */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressC, 0x00, 0x00) != SONY_DVB_OK) {
        c_UnFreezeReg(pDemod);
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /* Read snrestimate */
    if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressC, 0x4D, &rdata, 1) != SONY_DVB_OK) {
        c_UnFreezeReg(pDemod);
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    c_UnFreezeReg(pDemod);

    *pSNRReg = rdata;

    SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
}

/*--------------------------------------------------------------------
  Pre RS BER monitor

  sony_dvb_demod_t *pDemod      Instance of demod control struct
  uint32_t         *pBER        BER value (Pre reed solomon decoder)

  uint32_t         *pBitError   Bit error count
  uint32_t         *pPeriod     Period(bit) of BER measurement
--------------------------------------------------------------------*/
sony_dvb_result_t dvb_demod_monitorC_PreRSBER (sony_dvb_demod_t * pDemod, uint32_t *pBER)
{
    sony_dvb_result_t result = SONY_DVB_OK;
    uint32_t bitError = 0;
    uint32_t period = 0;

    SONY_DVB_TRACE_ENTER ("dvb_demod_monitorC_PreRSBER");

    if (!pDemod || !pBER)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);

    /* Software state check */
    if (pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) {
        /* This api is accepted in ACTIVE state only */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }
    if (pDemod->system != SONY_DVB_SYSTEM_DVBC) {
        /* Not DVB-T */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    result = dvb_demod_monitorC_PreRSBitError (pDemod, &bitError, &period);
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

sony_dvb_result_t dvb_demod_monitorC_PreRSBitError (sony_dvb_demod_t * pDemod, uint32_t * pBitError, uint32_t * pPeriod)
{
    uint8_t rdata[3];
    int index = 0;

    SONY_DVB_TRACE_ENTER ("dvb_demod_monitorC_PreRSBitError");

    if (!pDemod || !pBitError || !pPeriod)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);

    /* Software state check */
    if (pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) {
        /* This api is accepted in ACTIVE state only */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }
    if (pDemod->system != SONY_DVB_SYSTEM_DVBC) {
        /* Not DVB-T */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    /* AR lock check */
    {
        sony_dvb_result_t result = IsARLocked (pDemod);
        if (result != SONY_DVB_OK)
            SONY_DVB_TRACE_RETURN (result);
    }

    /* Set bank 0 */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressC, 0x00, 0x00) != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);

    /* Set ber_set (Bit error count start) */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressC, 0x79, 0x01) != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);

    rdata[0] = rdata[1] = rdata[2] = 0;
    for (index = 0; index < (MONITORC_PRERSBER_TIMEOUT / MONITORC_PRERSBER_CHECK_PERIOD); index++) {
        SONY_DVB_SLEEP (MONITORC_PRERSBER_CHECK_PERIOD);
        /* Read ber_estimate */
        if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressC, 0x76, rdata, 3) != SONY_DVB_OK)
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
    if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressC, 0x72, rdata, 1) != SONY_DVB_OK)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);

    *pPeriod = (1 << (rdata[0] & 0x1F)) * 204 * 8;

    SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
}

/*--------------------------------------------------------------------
  RS error monitor

  sony_dvb_demod_t *pDemod      Instance of demod control struct
  uint32_t         *pRSError    RS error count
--------------------------------------------------------------------*/
sony_dvb_result_t dvb_demod_monitorC_RSError (sony_dvb_demod_t * pDemod, uint32_t * pRSError)
{
    uint8_t rdata[2];

    SONY_DVB_TRACE_ENTER ("dvb_demod_monitorC_RSError");

    if (!pDemod || !pRSError)
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);

    /* Software state check */
    if (pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) {
        /* This api is accepted in ACTIVE state only */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }
    if (pDemod->system != SONY_DVB_SYSTEM_DVBC) {
        /* Not DVB-C */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    /* Freeze registers */
    if (c_FreezeReg(pDemod) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /* AR lock check */
    {
        sony_dvb_result_t result = IsARLocked (pDemod);
        if (result != SONY_DVB_OK) {
            c_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (result);
        }
    }

    /* Set bank 0 */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressC, 0x00, 0x00) != SONY_DVB_OK) {
        c_UnFreezeReg(pDemod);
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /* Read ber_cwrjctcnt */
    if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressC, 0xEA, rdata, 2) != SONY_DVB_OK) {
        c_UnFreezeReg(pDemod);
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }
    c_UnFreezeReg(pDemod);

    *pRSError = (rdata[0] << 8) | rdata[1];

    SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
}
