/*------------------------------------------------------------------------------

 <dev:header>
    Copyright(c) 2009 Sony Corporation.

    $Revision: 1663 $
    $Author: sking $

</dev:header>

------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
 Includes
------------------------------------------------------------------------------*/
#include <sony_dvb_demod_monitor.h>
#include <sony_dvb_demod_monitorT.h>
#include <sony_dvb_demod_monitorT2.h>
#include <sony_dvb_demod_monitorC.h>
#include "sony_dvb_math.h"
#include "sony_cxd2820.h"

/*------------------------------------------------------------------------------
 Defines
------------------------------------------------------------------------------*/
#define SONY_DVB_DVBT2_SQI_MER_MAX_DB1000   31500       /**< 31dB max MER. */
#define SONY_DVB_DVBT2_SQI_CNR_MAX_DB1000   24500       /**< 25dB max CNR. */
#define SONY_DVB_DVBT2_SQI_CNR_MARG_DB1000  1500        /**< XdB CNR margin. */

/*------------------------------------------------------------------------------
 Statics
------------------------------------------------------------------------------*/

/**
 @brief The list of DVB-T Nordig Profile 1
        C/N values in dBx1000.
*/
static const int32_t nordigDVBTdB1000[3][5] = 
{   
    /* 1/2,   2/3,   3/4,   5/6,   7/8 */
    {  5100,  6900,  7900,  8900,  9700  },  /* QPSK */
    {  10800, 13100, 14600, 15600, 16000 },  /* 16-QAM */
    {  16500, 18700, 20200, 21600, 22500 }   /* 64-QAM */
};

/**
 @brief Measured MER at picture fail point (FER > 0) with margin.
        MER values in dBx1000.
*/
static const int32_t merDVBT2dB1000[4][6] = 
{
    /*   1/2,   3/5,   2/3,   3/4,    4/5,    5/6 */
    {      0,  1200,  2200,  3400,   3900,    4400 },  /* QPSK */
    {   5500,  7000,  8000,  9000,  10000,   10800 },  /* 16-QAM */
    {   9800, 11500, 12700, 14200,  15300,   15800 },  /* 64-QAM */
    {  13600, 15800, 17000, 19000,  20100,   20900 },  /* 256-QAM */
};

/**
 @brief Nordig DVB-T2 SNR limits.
*/
static const int32_t snrDVBT2dB1000[4][6] = 
{
    /*   1/2,   3/5,   2/3,   3/4,    4/5,    5/6 */
    {   2500,  3800,  4600,  5600,   6200,    6700 },  /* QPSK */
    {   7400,  9100, 10300,  9700,  12400,   13000 },  /* 16-QAM */
    {  11300, 13500, 15000, 16800,  17800,   17700 },  /* 64-QAM */
    {  14600, 17500, 19600, 22000,  23700,   24600 },  /* 256-QAM */
};

/**
 @brief Monitor the DVB-T quality metric. Based on Nordig SQI 
        equation.
 
 @param pDemod The demodulator instance.
 @param pQuality The quality as a percentage (0-100).

 @return SONY_DVB_OK if successful and pQuality valid.
*/
static sony_dvb_result_t dvb_demod_monitorT_Quality (sony_dvb_demod_t* pDemod, 
                                                     uint8_t* pQuality) ;

/**
 @brief Monitor the DVB-T2 quality metric. 

 @param pDemod The demodulator instance.
 @param pMerFilter The MER filter to be used for the DVB-T2 MER measurement.
                   NULL if not used.
 @param pQuality The quality as a percentage (0-100).
 
 @return SONY_DVB_OK if successful and pQuality valid.
*/
static sony_dvb_result_t dvb_demod_monitorT2_Quality (sony_dvb_demod_t* pDemod, 
                                                      sony_dvb_demod_mer_filter_t* pMerFilter,
                                                      uint8_t* pQuality) ;

sony_dvb_result_t dvb_demod_monitor_IPID (sony_dvb_demod_t * pDemod, uint8_t * pIPID)
{
    sony_dvb_result_t result = SONY_DVB_OK;

    SONY_DVB_TRACE_ENTER ("dvb_demod_monitor_IPID");

    if ((!pDemod) || (!pIPID)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    result = dvb_demod_monitorT_IPID (pDemod, pIPID);

    SONY_DVB_TRACE_RETURN (result);
}

sony_dvb_result_t dvb_demod_monitor_TSLock (sony_dvb_demod_t * pDemod, sony_dvb_demod_lock_result_t * pLock)
{
    sony_dvb_result_t result = SONY_DVB_OK;

    SONY_DVB_TRACE_ENTER ("dvb_demod_monitor_TSLock");

    if ((!pDemod) || (!pLock)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    result = dvb_demod_CheckTSLock (pDemod, pLock);

    SONY_DVB_TRACE_RETURN (result);
}

sony_dvb_result_t dvb_demod_monitor_CarrierOffset (sony_dvb_demod_t * pDemod, 
                                                   int32_t * pOffset)
{
    sony_dvb_result_t result = SONY_DVB_OK;

    SONY_DVB_TRACE_ENTER ("dvb_demod_monitor_CarrierOffset");

    if ((!pDemod) || (!pOffset)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    switch (pDemod->system) {
    case SONY_DVB_SYSTEM_DVBT2:
        result = dvb_demod_monitorT2_CarrierOffset(pDemod, pOffset);
        *pOffset = (*pOffset >= 0 ) ? (*pOffset + 500) / 1000 : (*pOffset - 500) / 1000 ;
        break ;
    case SONY_DVB_SYSTEM_DVBT:
        result = dvb_demod_monitorT_CarrierOffset(pDemod, pOffset);
        break ;
    case SONY_DVB_SYSTEM_DVBC:
        result = dvb_demod_monitorC_CarrierOffset(pDemod, pOffset);
        break ;
    
    /* Intentional fall through. */
    case SONY_DVB_SYSTEM_UNKNOWN:
    default:
        result = SONY_DVB_ERROR_ARG ;
    }

    SONY_DVB_TRACE_RETURN (result);
}

sony_dvb_result_t dvb_demod_monitor_RFAGCOut (sony_dvb_demod_t * pDemod,
                                              uint32_t * pRFAGC)
{
    sony_dvb_result_t result = SONY_DVB_OK;
    SONY_DVB_TRACE_ENTER ("dvb_demod_monitor_RFAGCOut");

    if ((!pDemod) || (!pRFAGC)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    result = dvb_demod_monitorT_RFAGCOut(pDemod, pRFAGC);

    SONY_DVB_TRACE_RETURN (result);
}

sony_dvb_result_t dvb_demod_monitor_IFAGCOut (sony_dvb_demod_t * pDemod, 
                                              uint32_t * pIFAGC)
{
    sony_dvb_result_t result = SONY_DVB_OK;
    int32_t cIFAGCOut = 0 ;

    SONY_DVB_TRACE_ENTER ("dvb_demod_monitor_IFAGCOut");

    if ((!pDemod) || (!pIFAGC)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    switch (pDemod->system) {
    case SONY_DVB_SYSTEM_DVBT2:
        result = dvb_demod_monitorT2_IFAGCOut(pDemod, pIFAGC);
        break ;
    case SONY_DVB_SYSTEM_DVBT:
        result = dvb_demod_monitorT_IFAGCOut(pDemod, pIFAGC);
        break ;
    case SONY_DVB_SYSTEM_DVBC:
        result = dvb_demod_monitorC_IFAGCOut(pDemod, &cIFAGCOut);
        /* Scale to DVB-T/T2 IFAGC levels. */
        *pIFAGC = ((uint32_t)(cIFAGCOut + 512)) << 2 ; 
        break ;
    /* Intentional fall through. */
    case SONY_DVB_SYSTEM_UNKNOWN:
    default:
        result = SONY_DVB_ERROR_ARG ;
    }

    SONY_DVB_TRACE_RETURN (result);
}

sony_dvb_result_t dvb_demod_monitor_ChipID (sony_dvb_demod_t * pDemod, 
                                            sony_dvb_demod_chip_id_t * pChipId)
{
    uint8_t data = 0 ;
    SONY_DVB_TRACE_ENTER ("dvb_demod_monitor_ChipID");

    if ((!pDemod) || (!pChipId)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* Set bank 0x00 */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT, CXD2820R_B0_REGC_BANK, CXD2820R_B0) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }
    
    /* Read Chip ID reg. */
    if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressT, CXD2820R_B0_CHIP_ID_T, &data, 1) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }
    *pChipId = (sony_dvb_demod_chip_id_t) data ;

    SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
}

sony_dvb_result_t dvb_demod_monitor_Quality (sony_dvb_demod_t* pDemod, 
                                             uint8_t* pQuality)
{
    sony_dvb_result_t result = SONY_DVB_OK;
    SONY_DVB_TRACE_ENTER ("dvb_demod_monitor_Quality");

    if ((!pDemod) || (!pQuality)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* Interface only available when demod is active. */
    if (pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    switch (pDemod->system) {
    case SONY_DVB_SYSTEM_DVBT:
        result = dvb_demod_monitorT_Quality (pDemod, pQuality);
        break ;
    case SONY_DVB_SYSTEM_DVBT2: 
        result = dvb_demod_monitorT2_Quality (pDemod, NULL, pQuality);
        break ;
    
    /* Intentional fall-through. */
    case SONY_DVB_SYSTEM_DVBC: 
    case SONY_DVB_SYSTEM_UNKNOWN:
    default:
        result = SONY_DVB_ERROR_NOSUPPORT ;
        break ;
    }

    SONY_DVB_TRACE_RETURN (result);
}

static sony_dvb_result_t dvb_demod_monitorT_Quality (sony_dvb_demod_t* pDemod, 
                                                     uint8_t* pQuality)
{
    sony_dvb_result_t result = SONY_DVB_OK;
    sony_dvb_demod_tpsinfo_t tps ;
    uint32_t ber = 0 ;
    int32_t cn = 0 ;
    int32_t cnRel = 0 ;
    int32_t berSQI = 0 ;
    int32_t tmp = 0 ;

    SONY_DVB_TRACE_ENTER ("dvb_demod_monitorT_Quality");

    if ((!pDemod) || (!pQuality)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /*
        SQI (Signal Quality Indicator) given by:
        SQI = (((C/Nrel - 3) / 10) + 1) * BER_SQI 
        BER_SQI = 20*log10(1/BER) - 40
        
        Re-arrange for ease of calculation:
        BER_SQI = 20 * (log10(1) - log10(BER)) - 40

        If BER in units of 1e-7
        BER_SQI = 20 * (log10(1) - (log10(BER) - log10(1e7)) - 40

        BER_SQI = log10(1e7) - 40 - log10(BER)
        BER_SQI = -33 - 20*log10 (BER)
    */

    /* Monitor TPS for Modulation / Code Rate */
    result = dvb_demod_monitorT_TPSInfo(pDemod, &tps);
    if (result != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (result);
    }

    /* Get Pre-RS (Post-Viterbi) BER. */
    result = dvb_demod_monitorT_PreRSBER(pDemod, &ber);
    if (result != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (result);
    }
    
    /* Get CNR value. */
    result = dvb_demod_monitorT_CNValue(pDemod, &cn);
    if (result != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (result);
    }

    /* Ensure correct TPS values. */
    if ((tps.constellation >= DVBT_CONSTELLATION_RESERVED_3) || 
        (tps.rateHP >= DVBT_CODERATE_RESERVED_5)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_OTHER);
    }

    /* Calculate. */

    /* Handle 0%. */
    cnRel = cn - nordigDVBTdB1000[tps.constellation][tps.rateHP];
    if (cnRel < -7 * 1000) {
        *pQuality = 0 ;
        SONY_DVB_TRACE_RETURN (result);
    }

    /* Calculate BER_SQI and clip. BER_SQI (1000x) */
    if (ber == 0) {
        berSQI = 100 * 1000 ;
    }
    else {
        berSQI = (int32_t)(10 * sony_dvb_log10(ber)) ;
        berSQI = 20 * (7 * 1000 - (berSQI) ) - 40 * 1000 ;
        if (berSQI < 0) {
            berSQI = 0 ;
        }
    }

    /* Round up for rounding errors. */
    if (cnRel >= 3 * 1000) {
        *pQuality = (uint8_t) ((berSQI + 500) / 1000) ;
    }
    else {
        tmp = ( ((cnRel - (3*1000)) / 10) + 1000 ) ;
        *pQuality = (uint8_t)( ( (tmp * berSQI) + (5 * 1000 * 1000/10) ) / (1000 * 1000) ) & 0xFF ;
    }

    if (*pQuality > 100) {
        *pQuality = 100 ;
    }

    SONY_DVB_TRACE_RETURN (result);
}

static sony_dvb_result_t dvb_demod_monitorT2_Quality (sony_dvb_demod_t* pDemod, 
                                                      sony_dvb_demod_mer_filter_t* pMerFilter,
                                                      uint8_t* pQuality)
{
    sony_dvb_result_t result = SONY_DVB_OK;
    int32_t mer = 0 ;
    int32_t merTmp = 0 ;
    int32_t snr = 0 ;
    int32_t snrTmp = 0 ;
    uint32_t fer = 0 ;
    sony_dvb_dvbt2_plp_t plpInfo ;

    SONY_DVB_TRACE_ENTER ("dvb_demod_monitorT2_Quality");

    if ((!pDemod) || (!pQuality)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* Get Post BCH FER. Check if > 0. */
    result = dvb_demod_monitorT2_PostBCHFER(pDemod, &fer) ;
    if (result == SONY_DVB_OK) {
        if (fer > 0) {
            *pQuality = 0 ;
            SONY_DVB_TRACE_RETURN (result);
        }
    }

    /* Get Active PLP information. */
    result = dvb_demod_monitorT2_ActivePLP (pDemod, SONY_DVB_DVBT2_PLP_DATA, &plpInfo);
    if (result != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (result);
    }

    /* Ensure correct plp info. */
    if ((plpInfo.plpCr > SONY_DVB_DVBT2_R5_6) || 
        (plpInfo.constell > SONY_DVB_DVBT2_QAM256)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_OTHER);
    }

    /* Get MER. */
    if (pMerFilter != NULL) {
        /* Filtered MER. */
        result = dvb_demod_monitorT2_FilteredMER(pDemod, pMerFilter, &merTmp, &mer);
    }
    else {
        result = dvb_demod_monitorT2_MER(pDemod, &mer);
    }
    
    if (result != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (result);
    }

    /* Get SNR */
    result = dvb_demod_monitorT2_SNR(pDemod, &snr);
    if (result != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (result);
    }

    /*
        FER != 0: SQI = 0.
        FER == 0: SQI = 
            (((MER - MERmin) / (MERmax - MERmin)) * 100)/2  +
            (((CNR - (CNrel-CNmarg)) / (CNmax - (CNrel-CNmarg))) * 100)/2 ;
    */
    merTmp = (mer - merDVBT2dB1000[plpInfo.constell][plpInfo.plpCr]) * 100 ;
    merTmp /= (SONY_DVB_DVBT2_SQI_MER_MAX_DB1000 - merDVBT2dB1000[plpInfo.constell][plpInfo.plpCr]);

    if (merTmp > 100) {
        merTmp = 100 ;
    }
    else if (merTmp < 0) {
        merTmp = 0 ;
    }

    snrTmp = (snr - (snrDVBT2dB1000[plpInfo.constell][plpInfo.plpCr] - SONY_DVB_DVBT2_SQI_CNR_MARG_DB1000)) * 100 ;
    snrTmp /= (SONY_DVB_DVBT2_SQI_CNR_MAX_DB1000 - 
              (snrDVBT2dB1000[plpInfo.constell][plpInfo.plpCr] - SONY_DVB_DVBT2_SQI_CNR_MARG_DB1000));
    
    if (snrTmp > 100) {
        snrTmp = 100 ;
    }
    else if (snrTmp < 0) {
        snrTmp = 0 ;
    }

    merTmp = (merTmp + snrTmp)/2 ;
    
    if (merTmp > 100) {
        *pQuality = 100 ;
    }
    else {
        *pQuality = (uint8_t) merTmp ;
    }
    
    SONY_DVB_TRACE_RETURN (result);
}

sony_dvb_result_t dvb_demod_monitor_QualityV2 (sony_dvb_demod_t* pDemod, 
                                               sony_dvb_demod_mer_filter_t* pMerFilter,
                                               uint8_t* pQuality)
{
    sony_dvb_result_t result = SONY_DVB_OK;
    SONY_DVB_TRACE_ENTER ("dvb_demod_monitor_QualityV2");

    if ((!pDemod) || (!pQuality)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* Do not check pMerFilter == NULL, OK if bypassing filtered MER. */

    /* Interface only available when demod is active. */
    if (pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    switch (pDemod->system) {
    case SONY_DVB_SYSTEM_DVBT:
        result = dvb_demod_monitorT_Quality (pDemod, pQuality);
        break ;
    case SONY_DVB_SYSTEM_DVBT2: 
        result = dvb_demod_monitorT2_Quality (pDemod, pMerFilter, pQuality);
        break ;
    
    /* Intentional fall-through. */
    case SONY_DVB_SYSTEM_DVBC: 
    case SONY_DVB_SYSTEM_UNKNOWN:
    default:
        result = SONY_DVB_ERROR_NOSUPPORT ;
        break ;
    }

    SONY_DVB_TRACE_RETURN (result);
}
