/*------------------------------------------------------------------------------

 <dev:header>
    Copyright(c) 2009 Sony Corporation.

    $Revision: 1825 $
    $Author: sking $

</dev:header>

------------------------------------------------------------------------------*/

#include "sony_cxd2820.h"
#include "sony_dvb_math.h"
#include <sony_dvb_demod_monitorT2.h>
#include <sony_dvb_demod_monitorT.h>

#define t2_FreezeReg(pDemod) ((pDemod)->pI2c->WriteOneRegister ((pDemod)->pI2c, (pDemod)->i2cAddressT2, CXD2820R_B0_REGC_FREEZE, 0x01))
#define t2_UnFreezeReg(pDemod) ((void)((pDemod)->pI2c->WriteOneRegister ((pDemod)->pI2c, (pDemod)->i2cAddressT2, CXD2820R_B0_REGC_FREEZE, 0x00))) 

sony_dvb_result_t dvb_demod_monitorT2_CarrierOffset (sony_dvb_demod_t * pDemod, int32_t * pOffset)
{
    sony_dvb_result_t result = SONY_DVB_OK ;
    SONY_DVB_TRACE_ENTER ("dvb_demod_monitorT2_CarrierOffset");

    if ((!pDemod) || (!pOffset)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* Software state check */
    if (pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) {
        /* This api is accepted in ACTIVE state only */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    if (pDemod->system != SONY_DVB_SYSTEM_DVBT2) {
        /* Not DVB-T2 */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    /* Set bank 0x20 */

    /* Read crl_ctlval 27:0 */
    {
        uint8_t data[4];
        uint32_t ctlVal = 0;
        uint8_t syncState = 0 ;
        uint8_t tsLock = 0 ;

        /* Freeze registers */
        if (t2_FreezeReg(pDemod) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
        
        result = dvb_demod_monitorT2_SyncStat (pDemod, &syncState, &tsLock);
        if (result != SONY_DVB_OK) {
            t2_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        if (syncState != 6) {
            t2_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_HW_STATE);
        }

        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B0_REGC_BANK, CXD2820R_B20) != SONY_DVB_OK) {
            t2_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
        if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B20_CRCG_CRL_LOCK, data, sizeof (data)) != SONY_DVB_OK) {
            t2_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
        
        t2_UnFreezeReg(pDemod);

        /*
         * Format: s-3.30
         * Formula for converting this value to carrier frequency error is shown below
         * Carrier frequency error [Hz] (for BW=8MHz) = -((Register value in s-3.30) * Number of FFT points) / Tu (sec)
         * this is equivalent to the following equation for all FFT sizes
         * = -(IREG_CRCG_CTLVAL * (2^-30) * 64 ) / (7e-6) (All FFTsizes)
         * Carrier frequency error [Hz] (for BW=7MHz) = -(IREG_CRCG_CTLVAL * (2^-30) * 8  ) / (1e-6) (All FFTsizes)
         * Carrier frequency error [Hz] (for BW=6MHz) = -(IREG_CRCG_CTLVAL * (2^-30) * 48 ) / (7e-6) (All FFTsizes)
         * 
         * Or more generally: 
         * Offset [Hz] = -(IREG_CRCG_CTLVAL * (2^-30) * 8 * BW ) / (7e-6) (All FFTsizes)
         * 
         * Note: (2^-30 * 8 / 7e-6) = 1.064368657e-3
         * And: 1 /  (2^-30 * 8 / 7e-6) = 939.52 = 940
         * 
         */
        ctlVal = ((data[0] & 0x0F) << 24) | (data[1] << 16) | (data[2] << 8) | (data[3]);
        *pOffset = dvb_Convert2SComplement (ctlVal, 28);
        *pOffset = (-1 * ((*pOffset) * pDemod->bandWidth)) / 940;
    }

    /* Compensate for unexpected inverted spectrum. */
    {
        sony_dvb_demod_spectrum_sense_t currSense = DVB_DEMOD_SPECTRUM_NORMAL ;
        result = dvb_demod_monitorT2_SpectrumSense (pDemod, &currSense) ;
        if (result != SONY_DVB_OK) {
             SONY_DVB_TRACE_RETURN (result);
        }

        if (pDemod->confSense != currSense) {
            /* Sense different to expected. Invert calculation. */
            *pOffset *= -1 ;
        }
    }

    SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
}

sony_dvb_result_t dvb_demod_monitorT2_IFAGCOut (sony_dvb_demod_t * pDemod, uint32_t * pIFAGC)
{
    SONY_DVB_TRACE_ENTER ("dvb_demod_monitorT2_IFAGCOut");

    if ((!pDemod) || (!pIFAGC)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* Software state check */
    if (pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) {
        /* This api is accepted in ACTIVE state only */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    if (pDemod->system != SONY_DVB_SYSTEM_DVBT2) {
        /* Not DVB-T2 */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    /* Set bank 0x20 */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B0_REGC_BANK, CXD2820R_B20) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /* Read pir_agc_gain_1 and pir_agc_gain_2. */
    {
        uint8_t data[2];
        if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B20_PIR_AGC_GAIN_1, data, sizeof (data)) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        *pIFAGC = ((data[0] & 0x0F) << 8) | (data[1]);
    }

    SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
}

sony_dvb_result_t dvb_demod_monitorT2_RFAGCOut (sony_dvb_demod_t * pDemod, 
                                                uint32_t * pRFAGC)
{
    sony_dvb_result_t result = SONY_DVB_OK;

    SONY_DVB_TRACE_ENTER ("dvb_demod_monitorT2_RFAGCOut");

    /* Same as DVB-T */
    result = dvb_demod_monitorT_RFAGCOut (pDemod, pRFAGC);

    SONY_DVB_TRACE_RETURN (result);
}

sony_dvb_result_t dvb_demod_monitorT2_SNR (sony_dvb_demod_t * pDemod, 
                                           int32_t * pSNR)
{
    sony_dvb_result_t result = SONY_DVB_OK;

    SONY_DVB_TRACE_ENTER ("dvb_demod_monitorT2_SNR");

    if ((!pDemod) || (!pSNR)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* Software state check */
    if (pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) {
        /* This api is accepted in ACTIVE state only */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    if (pDemod->system != SONY_DVB_SYSTEM_DVBT2) {
        /* Not DVB-T2 */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    {
        uint16_t snr ;
        uint8_t syncState = 0 ;
        uint8_t tsLock = 0 ;
        uint8_t data[2] ;

        /* Freeze registers */
        if (t2_FreezeReg(pDemod) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
        
        result = dvb_demod_monitorT2_SyncStat (pDemod, &syncState, &tsLock);
        if (result != SONY_DVB_OK) {
            t2_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        /* Confirm sequence state == 6. */
        if (syncState != 6) {
            t2_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_HW_STATE);
        }

        /*
         SNR = 10 * log10 (IREG_SNMON_OD / 4 / 2 ) ;
         SNR = 10 * (log10 (IREG_SNMON_OD) - log10 (8))

         log10(8) * 100 = 90.308
        */
        /* Set bank 0x20 */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B0_REGC_BANK, CXD2820R_B20) != SONY_DVB_OK) {
            t2_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
        if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B20_SNMON_OD_11_8, data, sizeof (data)) != SONY_DVB_OK) {
            t2_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
        t2_UnFreezeReg(pDemod);

        snr = ((data[0] & 0x0F) << 8) | data[1] ;
        if (snr == 0) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_HW_STATE);
        }
        *pSNR = (10 * 10 * (int32_t)sony_dvb_log10 (snr) - 9031) ;
    }
    SONY_DVB_TRACE_RETURN (result);
}

sony_dvb_result_t dvb_demod_monitorT2_MER (sony_dvb_demod_t * pDemod, int32_t * pMER)
{
    sony_dvb_result_t result = SONY_DVB_OK;

    SONY_DVB_TRACE_ENTER ("dvb_demod_monitorT2_MER");

    if ((!pDemod) || (!pMER)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* Software state check */
    if (pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) {
        /* This api is accepted in ACTIVE state only */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    if (pDemod->system != SONY_DVB_SYSTEM_DVBT2) {
        /* Not DVB-T2 */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    {
        /*
         * IREG_OFDM_FFTSIZE[2:0] 0:2K, 1:8K, 2:4K, 3:1K, 4:16K, 5:32K
         * 
         * IREG_MER_DT[15:0]   Modulation Error Ratio. BitFormat u14.2 
         * 
         * MER[dB] = 10 * log10 ( IREG_MER_DT / 4 )
         *   
         */
        uint8_t data[2];
        uint32_t merdB;
        uint8_t syncState = 0 ;
        uint8_t tsLock = 0 ;

        /* Freeze registers. */
        if (t2_FreezeReg(pDemod) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        result = dvb_demod_monitorT2_SyncStat (pDemod, &syncState, &tsLock);
        if (result != SONY_DVB_OK) {
            t2_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
        
        /* Confirm sequence state == 6. */
        if (syncState != 6) {
            t2_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_HW_STATE);
        }

        /* Read MER */    
        /* Set bank 0x20 */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B0_REGC_BANK, CXD2820R_B20) != SONY_DVB_OK) {
            t2_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
        if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B20_MER_DT_1, data, sizeof (data)) != SONY_DVB_OK) {
            t2_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        /* Unfreeze registers. */
        t2_UnFreezeReg(pDemod);

        /* 
         * 10 * log10 ( IREG_MER_DT / 4 )
         * 
         * If we want dB x 1000, then
         * 
         * 1000 * 10 * (log10(IREG_MER_DT) - log10(4))
         * 
         * And: log10(4)*100 = 60.205
         */
        merdB = (data[0] << 8) | data[1] ;
        if (merdB == 0) {
            /* Reading invalid. */
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_HW_STATE);
        }
        merdB = sony_dvb_log10 (merdB);
        *pMER = 10 * 10 * (int32_t)merdB - 6021 ;
    }

    SONY_DVB_TRACE_RETURN (result);
}

sony_dvb_result_t dvb_demod_monitorT2_PreLDPCBER (sony_dvb_demod_t * pDemod, uint32_t * pBER)
{
    uint32_t bitError ;
    uint32_t period ;
    sony_dvb_result_t result = SONY_DVB_OK ;

    SONY_DVB_TRACE_ENTER ("dvb_demod_monitorT2_PreLDPCBER");
    
    if ((!pDemod) || (!pBER)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* Software state check */
    if (pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) {
        /* This api is accepted in ACTIVE state only */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    if (pDemod->system != SONY_DVB_SYSTEM_DVBT2) {
        /* Not DVB-T2 */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    /* Monitor the BER parameters. */
    result = dvb_demod_monitorT2_PreLDPCBERParam (pDemod, &bitError, &period);
    if (result != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN(result);
    }

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

        Therefore:
        BER = 1 / (Q + R/bitError)
    */
    {
        if (bitError == 0) {
            *pBER = 0 ;
            SONY_DVB_TRACE_RETURN(result);
        }
        *pBER = ((10 * 1000000) << 4) ;
        *pBER  /= (((period/bitError) << 4) + (((period%bitError) << 4) / bitError)) ;
    }

    SONY_DVB_TRACE_RETURN(result);
}

sony_dvb_result_t dvb_demod_monitorT2_PreLDPCBERParam (sony_dvb_demod_t * pDemod, 
                                                        uint32_t* pBitError,
                                                        uint32_t* pPeriod)
{
    sony_dvb_result_t result = SONY_DVB_OK;

    SONY_DVB_TRACE_ENTER ("dvb_demod_monitorT2_PreLDPCBERParam");

    if ((!pDemod) || (!pBitError) || (!pPeriod)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* Software state check */
    if (pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) {
        /* This api is accepted in ACTIVE state only */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    if (pDemod->system != SONY_DVB_SYSTEM_DVBT2) {
        /* Not DVB-T2 */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    /* Set bank 0x20 */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B0_REGC_BANK, CXD2820R_B20) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    {
        uint8_t data[4];
        if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B20_LBER_VALID, data, sizeof (data)) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        if (!(data[0] & 0x10)) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_HW_STATE);
        }

        *pBitError = ((data[0] & 0x0F) << 24) | (data[1] << 16) | (data[2] << 8) | data[3];

        /* Read measurement period. */
        if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B20_LBER_MES, data, 1) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        /* Period = 2^LBER_MES */
        *pPeriod = (1 << (data[0] & 0x0F)) ;

        /* Set Bank = 0x22. */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B0_REGC_BANK, CXD2820R_B22) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        /* Get the PLP type (Normal/Short). */
        if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B22_L1POST_PLP_FEC_TYPE, data, 1) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        if (((sony_dvb_dvbt2_plp_fec_t)(data[0] & 0x03)) == SONY_DVB_DVBT2_FEC_LDPC_16K) {
            /* Short. */
            *pPeriod *= 16200 ;
        } else {
            /* Normal. */ 
            *pPeriod *= 64800 ;
        }
    }

    SONY_DVB_TRACE_RETURN (result);
}

sony_dvb_result_t dvb_demod_monitorT2_PostBCHFER(sony_dvb_demod_t* pDemod,
                                                 uint32_t* pFER)
{
    uint32_t fecError ;
    uint32_t period ;
    sony_dvb_result_t result = SONY_DVB_OK ;

    SONY_DVB_TRACE_ENTER ("dvb_demod_monitorT2_PostBCHFER");
    
    if ((!pDemod) || (!pFER)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* Software state check */
    if (pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) {
        /* This api is accepted in ACTIVE state only */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    if (pDemod->system != SONY_DVB_SYSTEM_DVBT2) {
        /* Not DVB-T2 */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    /* Monitor the BER parameters. */
    result = dvb_demod_monitorT2_PostBCHFERParam (pDemod, &fecError, &period);
    if (result != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN(result);
    }
    
    /* Protect against /0. */
    if ((period == 0) || (fecError > period)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_HW_STATE);
    }

    /*
        FER = fecError / period ;

        Assume:
        Q = quotient = period / fecError ;
        R = remainder = period % fecError ;

        So:
        period = Q.fecError + R

        Therefore:
        FER = 1 / (Q + R/fecError)
    */
    {
        if (fecError == 0) {
            *pFER = 0 ;
            SONY_DVB_TRACE_RETURN(result);
        }
        *pFER = (uint32_t)((1000000u) << 12) ;
        *pFER  /= (((period/fecError) << 12) + (((period%fecError) << 12) / fecError)) ;
    }

    SONY_DVB_TRACE_RETURN(result);
}

sony_dvb_result_t dvb_demod_monitorT2_PostBCHFERParam(sony_dvb_demod_t* pDemod,
                                                      uint32_t* pFECError,
                                                      uint32_t* pPeriod)
{
    sony_dvb_result_t result = SONY_DVB_OK;

    SONY_DVB_TRACE_ENTER ("dvb_demod_monitorT2_PostBCHFERParam");

    if ((!pDemod) || (!pFECError) || (!pPeriod)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* Software state check */
    if (pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) {
        /* This api is accepted in ACTIVE state only */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    if (pDemod->system != SONY_DVB_SYSTEM_DVBT2) {
        /* Not DVB-T2 */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    /* Set bank 0x20 */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B0_REGC_BANK, CXD2820R_B20) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    {
        uint8_t data[3];
        if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B20_BER2_VALID, data, sizeof (data)) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
        if (!(data[0] & 0x01)) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_HW_STATE);
        }

        *pFECError = ((data[1] & 0x7F) << 8) | (data[2]) ;

        /* Read measurement period. */
        if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B20_BBER_MES, data, 1) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        /* Period = 2^BBER_MES */
        *pPeriod = (1 << (data[0] & 0x0F)) ;
    }

    SONY_DVB_TRACE_RETURN (result);
}

sony_dvb_result_t dvb_demod_monitorT2_L1Pre (sony_dvb_demod_t * pDemod, sony_dvb_dvbt2_l1pre_t * pL1Pre)
{
    sony_dvb_result_t result = SONY_DVB_OK;

    SONY_DVB_TRACE_ENTER ("dvb_demod_monitorT2_L1Pre");

    if ((!pDemod) || (!pL1Pre)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* Software state check */
    if (pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) {
        /* This api is accepted in ACTIVE state only */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    if (pDemod->system != SONY_DVB_SYSTEM_DVBT2) {
        /* Not DVB-T2 */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    {
        uint8_t data[37];
        uint8_t syncState = 0 ;
        uint8_t tsLock = 0 ;

        /* Freeze the register. */        
        if (t2_FreezeReg(pDemod) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }        
        
        result = dvb_demod_monitorT2_SyncStat (pDemod, &syncState, &tsLock);
        if (result != SONY_DVB_OK) {
            t2_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        /* Only valid after L1-pre locked. */
        if (syncState < 4) {
            t2_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_HW_STATE);
        }

        /* Set bank 0x22 */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B0_REGC_BANK, CXD2820R_B22) != SONY_DVB_OK) {
            t2_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B22_L1PRE_TYPE, data, sizeof (data)) != SONY_DVB_OK) {
            t2_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
        t2_UnFreezeReg(pDemod);

        /* Convert data to appropriate format. */
        pL1Pre->type = (sony_dvb_dvbt2_l1pre_type_t) data[0];
        pL1Pre->bwExt = data[1] & 0x01;
        pL1Pre->s1 = (sony_dvb_dvbt2_s1_t) (data[2] & 0x07);
        pL1Pre->s2 = data[3] & 0x0F;
        pL1Pre->l1Rep = data[4] & 0x01;
        pL1Pre->gi = (sony_dvb_dvbt2_guard_t) (data[5] & 0x07);
        pL1Pre->papr = (sony_dvb_dvbt2_papr_t) (data[6] & 0x0F);
        pL1Pre->mod = (sony_dvb_dvbt2_l1post_constell_t) (data[7] & 0x0F);
        pL1Pre->cr = (sony_dvb_dvbt2_l1post_cr_t) (data[8] & 0x03);
        pL1Pre->fec = (sony_dvb_dvbt2_l1post_fec_type_t) (data[9] & 0x03);
        pL1Pre->l1PostSize = (data[10] & 0x3) << 16;
        pL1Pre->l1PostSize |= (data[11]) << 8;
        pL1Pre->l1PostSize |= (data[12]);
        pL1Pre->l1PostInfoSize = (data[13] & 0x3) << 16;
        pL1Pre->l1PostInfoSize |= (data[14]) << 8;
        pL1Pre->l1PostInfoSize |= (data[15]);
        pL1Pre->pp = (sony_dvb_dvbt2_pp_t) (data[16] & 0x0F);
        pL1Pre->txIdAvailability = data[17];
        pL1Pre->cellId = (data[18] << 8);
        pL1Pre->cellId |= (data[19]);
        pL1Pre->networkId = (data[20] << 8);
        pL1Pre->networkId |= (data[21]);
        pL1Pre->systemId = (data[22] << 8);
        pL1Pre->systemId |= (data[23]);
        pL1Pre->numFrames = data[24];
        pL1Pre->numSymbols = (data[25] & 0x0F) << 8;
        pL1Pre->numSymbols |= data[26];
        pL1Pre->regen = data[27] & 0x07;
        pL1Pre->postExt = data[28] & 0x01;
        pL1Pre->numRfFreqs = data[29] & 0x07;
        pL1Pre->rfIdx = data[30] & 0x07;
        pL1Pre->resvd = (data[31] & 0x03) << 8;
        pL1Pre->resvd |= data[32];
        pL1Pre->crc32 = (data[33] << 24);
        pL1Pre->crc32 |= (data[34] << 16);
        pL1Pre->crc32 |= (data[35] << 8);
        pL1Pre->crc32 |= data[36];

        /* Get the FFT mode from the S2 signalling. */
        switch ((pL1Pre->s2 >> 1))
        {
        case SONY_DVB_DVBT2_S2_M8K_G_DVBT2:
            pL1Pre->fftMode = SONY_DVB_DVBT2_M8K;
            break ;
        case SONY_DVB_DVBT2_S2_M32K_G_DVBT2:
            pL1Pre->fftMode = SONY_DVB_DVBT2_M32K;
            break ;
        default:
            pL1Pre->fftMode = (sony_dvb_dvbt2_mode_t) (pL1Pre->s2 >> 1) ;
            break ;
        }

        /* Get the Mixed indicator (FEFs exist) from the S2 signalling. */
        pL1Pre->mixed = pL1Pre->s2 & 0x01 ;
    }

    SONY_DVB_TRACE_RETURN (result);
}

sony_dvb_result_t dvb_demod_monitorT2_Preset (sony_dvb_demod_t * pDemod, 
                                              sony_dvb_demod_t2_preset_t * pPresetInfo)
{
    SONY_DVB_TRACE_ENTER ("dvb_demod_monitorT2_Preset");

    if ((!pDemod) || (!pPresetInfo)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* Software state check */
    if (pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) {
        /* This api is accepted in ACTIVE state only */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    if (pDemod->system != SONY_DVB_SYSTEM_DVBT2) {
        /* Not DVB-T2 */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    {
        uint8_t data[5];
        uint8_t syncState = 0 ;
        uint8_t tsLock = 0 ;
        sony_dvb_result_t result = SONY_DVB_OK ;

        /* Freeze the registers. */
        if (t2_FreezeReg(pDemod) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        result = dvb_demod_monitorT2_SyncStat (pDemod, &syncState, &tsLock);
        if (result != SONY_DVB_OK) {
            t2_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        /* Only valid after L1-pre locked. */
        if (syncState < 4) {
            t2_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_HW_STATE);
        }
        
        /* Set bank 0x20 */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B0_REGC_BANK, CXD2820R_B20) != SONY_DVB_OK) {
            t2_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B20_OFDM_MIXED_MISO_FFTSZ, data, sizeof (data)) != SONY_DVB_OK) {
            t2_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        /* Unfreeze the registers. */
        t2_UnFreezeReg(pDemod);

        /* Convert */
        pPresetInfo->mixed = ((data[0] & 0x20) ? 1 : 0);
        pPresetInfo->s1 = (sony_dvb_dvbt2_s1_t) ((data[0] & 0x10) >> 4);
        pPresetInfo->mode = (sony_dvb_dvbt2_mode_t) (data[0] & 0x07);
        pPresetInfo->gi = (sony_dvb_dvbt2_guard_t) ((data[1] & 0x70) >> 4);
        pPresetInfo->pp = (sony_dvb_dvbt2_pp_t) (data[1] & 0x07);
        pPresetInfo->bwExt = (data[2] & 0x10) >> 4;
        pPresetInfo->papr = (sony_dvb_dvbt2_papr_t) (data[2] & 0x0F);
        pPresetInfo->numSymbols = (data[3] << 8) | data[4];
    }

    SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
}

sony_dvb_result_t dvb_demod_monitorT2_DataPLPs (sony_dvb_demod_t * pDemod, 
                                                uint8_t * pPLPIds, 
                                                uint8_t * pNumPLPs)
{
    SONY_DVB_TRACE_ENTER ("dvb_demod_monitorT2_DataPLPs");

    if ((!pDemod) || (!pPLPIds) || (!pNumPLPs)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* Software state check */
    if (pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) {
        /* This api is accepted in ACTIVE state only */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    if (pDemod->system != SONY_DVB_SYSTEM_DVBT2) {
        /* Not DVB-T2 */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    {
        uint8_t l1PostOK = 0 ;

        /* Set bank 0x22 */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B0_REGC_BANK, CXD2820R_B22) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        /* Freeze the registers. */
        if (t2_FreezeReg (pDemod) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
        
        if (pDemod->pI2c->ReadRegister(pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B22_L1POST_OK, &l1PostOK, 1) != SONY_DVB_OK) {
            t2_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        if (!(l1PostOK & 0x01)) {
            t2_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_HW_STATE);
        }
        
        /* Read the number of PLPs. */
        if (pDemod->pI2c->ReadRegister(pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B22_NUM_DATA_PLP_TS, pNumPLPs, 1) != SONY_DVB_OK) {
            t2_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        /* Invalid number of PLPs detected. May occur if multiple threads accessing device. */
        if (*pNumPLPs == 0) {
            t2_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_OTHER);
        }

        /* Read the PLPs from the device. */
        /* Set Bank 0x22 */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B0_REGC_BANK, CXD2820R_B22) != SONY_DVB_OK) {
            t2_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        /* Read up to PLP_ID127 */
        if (pDemod->pI2c->ReadRegister(pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B22_L1POST_PLP_ID0, pPLPIds, *pNumPLPs > 128 ? 128 : *pNumPLPs) != SONY_DVB_OK) {
            t2_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        if (*pNumPLPs > 128)
        {
            /* Set Bank 0x23 */
            if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B0_REGC_BANK, CXD2820R_B23) != SONY_DVB_OK) {
                t2_UnFreezeReg(pDemod);
                SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
            }

            /* Read the remaining data PLPs. */
            if (pDemod->pI2c->ReadRegister(pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B23_L1POST_PLP_ID128, pPLPIds + 128, *pNumPLPs - 128) != SONY_DVB_OK) {
                t2_UnFreezeReg(pDemod);
                SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
            }
        }

        t2_UnFreezeReg(pDemod);
    }
    SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
}

sony_dvb_result_t dvb_demod_monitorT2_ActivePLP (sony_dvb_demod_t * pDemod,
                                                 sony_dvb_dvbt2_plp_btype_t type, 
                                                 sony_dvb_dvbt2_plp_t * pPLPInfo)
{
    SONY_DVB_TRACE_ENTER ("dvb_demod_monitorT2_ActivePLP");

    if ((!pDemod) || (!pPLPInfo)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* Software state check */
    if (pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) {
        /* This api is accepted in ACTIVE state only */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    if (pDemod->system != SONY_DVB_SYSTEM_DVBT2) {
        /* Not DVB-T2 */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    {
        uint8_t data[20];
        uint8_t syncState = 0 ;
        uint8_t tsLock = 0 ;
        uint8_t addr = 0 ;
        uint8_t index = 0 ;
        sony_dvb_result_t result = SONY_DVB_OK ;

        /* Freeze the registers. */
        if (t2_FreezeReg(pDemod) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        /* Confirm SyncStat. */
        result = dvb_demod_monitorT2_SyncStat (pDemod, &syncState, &tsLock) ;
        if (result != SONY_DVB_OK) {
            t2_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        if (syncState < 5) {
            t2_UnFreezeReg(pDemod) ;
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_HW_STATE);
        }

        /* Set bank 0x22 */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B0_REGC_BANK, CXD2820R_B22) != SONY_DVB_OK) {
            t2_UnFreezeReg(pDemod) ;
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        /* Read contents of the L1-post for appropriate item. */
        if (type == SONY_DVB_DVBT2_PLP_COMMON) {
            addr = CXD2820R_B22_L1POST_PLP_ID_C ;
        } 
        else {
            addr = CXD2820R_B22_L1POST_PLP_ID ;
        }

        if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressT2, addr, data, sizeof(data)) != SONY_DVB_OK) {
            t2_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
        t2_UnFreezeReg(pDemod);

        /* If common, check for no common PLP, frame_interval == 0. */
        if ((type == SONY_DVB_DVBT2_PLP_COMMON) && (data[13] == 0)) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_HW_STATE);
        }

        pPLPInfo->id = data[index++] ;
        pPLPInfo->type = (sony_dvb_dvbt2_plp_type_t) (data[index++] & 0x07) ;
        pPLPInfo->payload = (sony_dvb_dvbt2_plp_payload_t) (data[index++] & 0x1F) ;
        pPLPInfo->ff = data[index++] & 0x01 ;
        pPLPInfo->firstRfIdx = data[index++] & 0x07 ;
        pPLPInfo->firstFrmIdx = data[index++] ;
        pPLPInfo->groupId = data[index++] ;
        pPLPInfo->plpCr = (sony_dvb_dvbt2_plp_code_rate_t) (data[index++] & 0x07) ;
        pPLPInfo->constell = (sony_dvb_dvbt2_plp_constell_t) (data[index++] & 0x07) ;
        pPLPInfo->rot = data[index++] & 0x01 ;
        pPLPInfo->fec = (sony_dvb_dvbt2_plp_fec_t) (data[index++] & 0x03) ;
        pPLPInfo->numBlocksMax = (uint16_t)((data[index++] & 0x03)) << 8 ;
        pPLPInfo->numBlocksMax |= data[index++] ;
        pPLPInfo->frmInt = data[index++] ;
        pPLPInfo->tilLen = data[index++] ;
        pPLPInfo->tilType = data[index++] & 0x01 ;

        /* Skip 0x77 for common PLP. */
        if (type == SONY_DVB_DVBT2_PLP_COMMON) {
            index++ ;
        }

        pPLPInfo->inBandFlag = data[index++] & 0x01 ;
        pPLPInfo->rsvd = data[index++] << 8 ;
        pPLPInfo->rsvd |= data[index++] ;
    }

    SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
}

sony_dvb_result_t dvb_demod_monitorT2_DataPLPError (sony_dvb_demod_t * pDemod, uint8_t * pPLPError)
{
    SONY_DVB_TRACE_ENTER ("dvb_demod_monitorT2_DataPLPError");

    if ((!pDemod) || (!pPLPError)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* Software state check */
    if (pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) {
        /* This api is accepted in ACTIVE state only */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    if (pDemod->system != SONY_DVB_SYSTEM_DVBT2) {
        /* Not DVB-T2 */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    {
        uint8_t data ;        
        uint8_t syncState = 0 ;
        uint8_t tsLock = 0 ;     
        sony_dvb_result_t result = SONY_DVB_OK ;

        /* Freeze the registers. */
        if (t2_FreezeReg(pDemod) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        /* Confirm SyncStat. */
        result = dvb_demod_monitorT2_SyncStat (pDemod, &syncState, &tsLock) ;
        if (result != SONY_DVB_OK) {
            t2_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        if (syncState < 5) {
            t2_UnFreezeReg(pDemod) ;
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_HW_STATE);
        }
        
        /* Set bank 0x22 */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B0_REGC_BANK, CXD2820R_B22) != SONY_DVB_OK) {
            t2_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B22_PLP_SEL_ERR, &data, 1) != SONY_DVB_OK) {
            t2_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
        t2_UnFreezeReg(pDemod);

        *pPLPError = data & 0x01;
    }

    SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
}

sony_dvb_result_t dvb_demod_monitorT2_L1Change (sony_dvb_demod_t * pDemod, uint8_t * pL1Change)
{
    SONY_DVB_TRACE_ENTER ("dvb_demod_monitorT2_L1Change");

    if ((!pDemod) || (!pL1Change)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* Software state check */
    if (pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) {
        /* This api is accepted in ACTIVE state only */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    if (pDemod->system != SONY_DVB_SYSTEM_DVBT2) {
        /* Not DVB-T2 */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    {  
        uint8_t data ;        
        uint8_t syncState = 0 ;
        uint8_t tsLock = 0 ;
        sony_dvb_result_t result = SONY_DVB_OK ;

        /* Freeze the registers. */
        if (t2_FreezeReg(pDemod) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        result = dvb_demod_monitorT2_SyncStat (pDemod, &syncState, &tsLock);
        if (result != SONY_DVB_OK) {
            t2_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        /* Only valid after L1-post decoded at least once. */
        if (syncState < 5) {
            t2_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_HW_STATE);
        }

        /* Set bank 0x22 */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B0_REGC_BANK, CXD2820R_B22) != SONY_DVB_OK) {
            t2_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B22_L1_CHANGE_FLAG, &data, sizeof (data)) != SONY_DVB_OK) {
            t2_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        *pL1Change = data & 0x01;
        if (*pL1Change) {
            /* Only clear if set, otherwise may miss indicator. */
            if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B22_L1_CHANGE_FLAG_CLR, 0x01) != SONY_DVB_OK) {
                t2_UnFreezeReg(pDemod);
                SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
            }
        }
        t2_UnFreezeReg(pDemod);
    }

    SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
}

sony_dvb_result_t dvb_demod_monitorT2_SyncStat (sony_dvb_demod_t * pDemod, 
                                                uint8_t * pSyncStat, 
                                                uint8_t * pTSLockStat) 
{
    SONY_DVB_TRACE_ENTER ("dvb_demod_monitorT2_SyncStat");

    if ((!pDemod) || (!pSyncStat) || (!pTSLockStat)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* Software state check */
    if (pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) {
        /* This api is accepted in ACTIVE state only */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    if (pDemod->system != SONY_DVB_SYSTEM_DVBT2) {
        /* Not DVB-T2 */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    /* Set bank 0x20 */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B0_REGC_BANK, CXD2820R_B20) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    {
        uint8_t data;
        if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B20_SP_TS_LOCK, &data, sizeof (data)) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
        *pSyncStat = data & 0x07 ;
        *pTSLockStat = (data & 0x20) >> 5;
    }

    SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
}

sony_dvb_result_t dvb_demod_monitorT2_SpectrumSense (sony_dvb_demod_t* pDemod,
                                                     sony_dvb_demod_spectrum_sense_t* pSense)
{
    SONY_DVB_TRACE_ENTER ("dvb_demod_monitorT2_SpectrumSense");

    if ((!pDemod) || (!pSense)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* Software state check */
    if (pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) {
        /* This api is accepted in ACTIVE state only */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    if (pDemod->system != SONY_DVB_SYSTEM_DVBT2) {
        /* Not DVB-T2 */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    {
        uint8_t data = 0 ;

        /* Set bank 0x20 */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B0_REGC_BANK, CXD2820R_B20) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        /* Read OREG_DNCNV_SRVS (bit 4) */
        if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B20_TDA_INVERT, &data, sizeof(data)) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        *pSense = (data & 0x10) ? DVB_DEMOD_SPECTRUM_INV : DVB_DEMOD_SPECTRUM_NORMAL ;
    }

    SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
}

sony_dvb_result_t dvb_demod_monitorT2_L1Post (sony_dvb_demod_t* pDemod,
                                              sony_dvb_dvbt2_l1post_t* pL1Post)
{
     SONY_DVB_TRACE_ENTER ("dvb_demod_monitorT2_L1Post");

    if ((!pDemod) || (!pL1Post)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* Software state check */
    if (pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) {
        /* This api is accepted in ACTIVE state only */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    if (pDemod->system != SONY_DVB_SYSTEM_DVBT2) {
        /* Not DVB-T2 */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    {  
        uint8_t data[15] ;        
        uint8_t l1PostOK = 0 ;

        /* Freeze the register. */        
        if (t2_FreezeReg(pDemod) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
        
        /* Set bank 0x22 */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B0_REGC_BANK, CXD2820R_B22) != SONY_DVB_OK) {
            t2_UnFreezeReg(pDemod) ;
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        if (pDemod->pI2c->ReadRegister(pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B22_L1POST_OK, &l1PostOK, 1) != SONY_DVB_OK) {
            t2_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        if (!(l1PostOK & 0x01)) {
            t2_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_HW_STATE);
        }

        /* Read L1-post data. */
        if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B22_L1POST_SUB_SLICES_PER_FRAME_1, data, sizeof (data)) != SONY_DVB_OK) {
            t2_UnFreezeReg(pDemod) ;
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
        
        /* Unfreeze registers. */
        t2_UnFreezeReg(pDemod) ;

        /* Convert data. */
        pL1Post->subSlicesPerFrame  = (data[0] & 0x7F) << 8 ;
        pL1Post->subSlicesPerFrame |= data[1] ;
        pL1Post->numPLPs = data[2] ;
        pL1Post->numAux = data[3] & 0x0F ;
        pL1Post->auxConfigRFU = data[4] ;
        pL1Post->rfIdx = data[5] & 0x07 ;
        pL1Post->freq  = data[6] << 24 ;
        pL1Post->freq |= data[7] << 16 ;
        pL1Post->freq |= data[8] << 8 ;
        pL1Post->freq |= data[9] ;
        pL1Post->fefType = data[10] & 0x0F ;
        pL1Post->fefLength  = (data[11] & 0x3F) << 16 ;
        pL1Post->fefLength |= data[12] << 8 ;
        pL1Post->fefLength |= data[13]  ;
        pL1Post->fefInterval = data[14] ;
    }

    SONY_DVB_TRACE_RETURN (SONY_DVB_OK);
}

sony_dvb_result_t dvb_demod_monitorT2_PreBCHBERParam(sony_dvb_demod_t* pDemod,
                                                     uint32_t* pBitError,
                                                     uint32_t* pPeriod)
{

    sony_dvb_result_t result = SONY_DVB_OK;

    SONY_DVB_TRACE_ENTER ("dvb_demod_monitorT2_PreBCHBERParam");

    if ((!pDemod) || (!pBitError) || (!pPeriod)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* Software state check */
    if (pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) {
        /* This api is accepted in ACTIVE state only */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    if (pDemod->system != SONY_DVB_SYSTEM_DVBT2) {
        /* Not DVB-T2 */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    {
        uint8_t data[3];
        sony_dvb_dvbt2_plp_fec_t plpFecType = SONY_DVB_DVBT2_FEC_LDPC_16K ;
        sony_dvb_dvbt2_plp_code_rate_t plpCr = SONY_DVB_DVBT2_R1_2 ; 
        static const uint16_t nBCHBitsLookup[SONY_DVB_DVBT2_FEC_RSVD1][SONY_DVB_DVBT2_R_RSVD1] = 
        {
            /* R1_2    R3_5    R2_3   R3_4    R4_5    R5_6 */ 
            {  7200,  9720,  10800,  11880,  12600,  13320 },  /* Short */
            { 32400, 38880,  43200,  48600,  51840,  54000 }   /* Normal */
        };

        /* Freeze registers */
        if (t2_FreezeReg(pDemod) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        /* Set bank 0x20 */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B0_REGC_BANK, CXD2820R_B20) != SONY_DVB_OK) {
            t2_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
        
        if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B20_BER2_VALID, data, 1) != SONY_DVB_OK) {
            t2_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        if (!(data[0] & 0x01)) {
            t2_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_HW_STATE);
        }

        /* Read measurement period. */
        if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B20_BBER_MES, data, 1) != SONY_DVB_OK) {
            t2_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        /* Period = 2^BBER_MES */
        *pPeriod = (1 << (data[0] & 0x0F)) ;

        /* Set bank 0x24 */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B0_REGC_BANK, CXD2820R_B24) != SONY_DVB_OK) {
            t2_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
      
        if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B24_BER0_BITERR_1, data, sizeof (data)) != SONY_DVB_OK) {
            t2_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        *pBitError = ((data[0] & 0x3F) << 16) | (data[1] << 8) | data[2] ;

        /* Set Bank = 0x22. */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B0_REGC_BANK, CXD2820R_B22) != SONY_DVB_OK) {
            t2_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        /* Get the PLP FEC type (Normal/Short). */
        if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B22_L1POST_PLP_FEC_TYPE, data, 1) != SONY_DVB_OK) {
            t2_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
        plpFecType = (sony_dvb_dvbt2_plp_fec_t)(data[0] & 0x03) ;

        /* Get the PLP code rate. */
        if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B22_L1POST_PLP_COD, data, 1) != SONY_DVB_OK) {
            t2_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
        t2_UnFreezeReg(pDemod);
        plpCr = (sony_dvb_dvbt2_plp_code_rate_t) (data[0] & 0x07) ;

        /* Confirm FEC Type / Code Rate */
        if ((plpFecType > SONY_DVB_DVBT2_FEC_LDPC_64K) || (plpCr > SONY_DVB_DVBT2_R5_6)) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_HW_STATE);
        }

        *pPeriod *= nBCHBitsLookup[plpFecType][plpCr] ;
    }

    SONY_DVB_TRACE_RETURN (result);
}

sony_dvb_result_t dvb_demod_monitorT2_PreBCHBER(sony_dvb_demod_t* pDemod,
                                                uint32_t* pBER)
{
    uint32_t bitError ;
    uint32_t period ;
    sony_dvb_result_t result = SONY_DVB_OK ;

    SONY_DVB_TRACE_ENTER ("dvb_demod_monitorT2_PreBCHBER");
    
    if ((!pDemod) || (!pBER)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* Software state check */
    if (pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) {
        /* This api is accepted in ACTIVE state only */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    if (pDemod->system != SONY_DVB_SYSTEM_DVBT2) {
        /* Not DVB-T2 */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    /* Monitor the BER parameters. */
    result = dvb_demod_monitorT2_PreBCHBERParam (pDemod, &bitError, &period);
    if (result != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN(result);
    }

    /* Protect against /0. */
    if ((period == 0) || (bitError > period)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_HW_STATE);
    }

    /*
        BER = bitError / period ;

        Assume:
        Q = quotient = period / bitError ;
        R = remainder = period % bitError ;

        So:
        period = Q.bitError + R

        Therefore:
        BER = 1 / (Q + R/bitError)
    */
    {
        if (bitError == 0) {
            *pBER = 0 ;
            SONY_DVB_TRACE_RETURN(result);
        }
        *pBER = (uint32_t)((1000u * 1000u * 1000u) << 2) ;
        *pBER  /= (((period/bitError) << 2) + (((period%bitError) << 2) / bitError)) ;
    }

    SONY_DVB_TRACE_RETURN(result);

}


sony_dvb_result_t dvb_demod_monitorT2_SamplingOffset (sony_dvb_demod_t * pDemod, 
                                                      int32_t* pPPM)
{
    SONY_DVB_TRACE_ENTER ("dvb_demod_monitorT2_SamplingOffset");

    if ((!pDemod) || (!pPPM)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* Software state check */
    if (pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) {
        /* This api is accepted in ACTIVE state only */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    if (pDemod->system != SONY_DVB_SYSTEM_DVBT2) {
        /* Not DVB-T2 */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    {
        uint8_t data [5] ;
        uint32_t trlMon2 = 0;
        sony_dvb_result_t result = SONY_DVB_OK ;
        uint8_t syncState = 0 ;
        uint8_t tsLock = 0 ;
        
        /* Freeze registers */
        if (t2_FreezeReg(pDemod) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }        
        
        result = dvb_demod_monitorT2_SyncStat (pDemod, &syncState, &tsLock);
        if (result != SONY_DVB_OK) {
            t2_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        if (syncState != 6) {
            t2_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_HW_STATE);
        }

        /* Set bank 0x20 */
        if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B0_REGC_BANK, CXD2820R_B20) != SONY_DVB_OK) {
            t2_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B20_TRL_MON1, data, sizeof(data)) != SONY_DVB_OK) {
            t2_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
        
        /* Unfreeze registers. */
        t2_UnFreezeReg(pDemod);
        
        /* Ignore top bits - not significant. */
        trlMon2 =  data[1] << 24 ;
        trlMon2 |= data[2] << 16 ;
        trlMon2 |= data[3] << 8 ;
        trlMon2 |= data[4] ;

        switch (pDemod->bandWidth) {
            case 5:
                /* (1 / 0x1CB3333333) * 1e12 * 64 = 519.2 */
                /* Confirm offset range. */
                if ((data[0] & 0x7F) != 0x1C) {
                    SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_HW_STATE);
                }
                *pPPM = (int32_t)(trlMon2 - 0xB3333333) ;
                *pPPM = (*pPPM + 4) / 8 ;
                *pPPM *= 519 ;
                break  ;
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

sony_dvb_result_t dvb_demod_monitorT2_TsRate (sony_dvb_demod_t * pDemod, 
                                              uint32_t* pTsRateKbps)
{
    sony_dvb_result_t result = SONY_DVB_OK ;
    SONY_DVB_TRACE_ENTER ("dvb_demod_monitorT2_TsRate");

    if ((!pDemod) || (!pTsRateKbps)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG) ;
    }

    if (pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE) ;
    }

    if (pDemod->system != SONY_DVB_SYSTEM_DVBT2) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE) ;
    }

    {
        uint8_t data[4] ;
        uint32_t val = 0 ;
        uint8_t syncState = 0 ;
        uint8_t tsLock = 0 ;

        /* Freeze registers */
        if (t2_FreezeReg(pDemod) != SONY_DVB_OK) {
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }
        
        result = dvb_demod_monitorT2_SyncStat (pDemod, &syncState, &tsLock);
        if (result != SONY_DVB_OK) {
            t2_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
        }

        /* Check TS lock. */
        if (!tsLock) {
            t2_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_HW_STATE);
        }

        /* Set Bank 0x20. */
        if (pDemod->pI2c->WriteOneRegister(pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B0_REGC_BANK, CXD2820R_B20) != SONY_DVB_OK) {
            t2_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C) ;
        }

        /*  Read IREG_SP_RD_SMOOTH_DP[27:0] 

            TS Rate (bps) = (8 / (IREG_SP_RD_SMOOTH_DP * 2^-16)) * 41e6
                          = 8 * 2^16 * 41e6 / IREG_SP_RD_SMOOTH_DP
                          = 8 * 2^16 * 4100 * 1000 / IREG_SP_RD_SMOOTH_DP
                Note: 8 * 2^16 * 4100 = 2149580800

                Below math limited to accuracy (relative to demod register)
                of approx. 2kbpps (when > 40Mbps).
        */
        if (pDemod->pI2c->ReadRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B20_RD_SMOOTH_DP_27_24, data, sizeof(data)) != SONY_DVB_OK) {
            t2_UnFreezeReg(pDemod);
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C) ;
        }

        /* Unfreeze registers */
        t2_UnFreezeReg(pDemod);

        val =  (uint32_t)((data[0] & 0x0F) << 24) ;
        val |= (uint32_t) (data[1] << 16) ;
        val |= (uint32_t) (data[2] << 8) ;
        val |= (uint32_t)  data[3] ;
        val = ((val + 5)/10) ;  /* /10 */

        if (val == 0) {
            SONY_DVB_TRACE_RETURN(SONY_DVB_ERROR_HW_STATE);
        }
        *pTsRateKbps  = (2149580800u + val / 2) / val;
    }
    SONY_DVB_TRACE_RETURN (result) ;
}

static void window_Init (sony_dvb_int32_window_t* pWindow, uint8_t length)
{
    if (!pWindow)
        return ;

    pWindow->length = length ;
    pWindow->count = 0 ;
    pWindow->index = 0 ;
}

static void window_Add (sony_dvb_int32_window_t* pWindow, int32_t value)
{
    if (!pWindow || (pWindow->length == 0))
        return ;
    if ((pWindow->index >= pWindow->length) || (pWindow->index >= SONY_DVB_MONITORT2_WINDOW_MAX_LENGTH))
        return ;

    pWindow->window[pWindow->index] = value ;
    pWindow->index = (pWindow->index + 1) % pWindow->length ;

    /* Clip the window count to the length. */
    pWindow->count = pWindow->count >= pWindow->length ? pWindow->count : pWindow->count + 1 ;
}

static int32_t window_GetMax (sony_dvb_int32_window_t* pWindow)
{
    int32_t v = -32767 ;
    uint16_t i = 0 ;

    if (!pWindow)
        return v;

    /* Protect against out of range index. */
    if (pWindow->count > SONY_DVB_MONITORT2_WINDOW_MAX_LENGTH)
        return v ;

    for (i = 0 ; i < pWindow->count ; i++) {
        if (pWindow->window[i] > v) {
            v = pWindow->window[i] ;
        }
    }
    return v ;
}

static int32_t window_GetTotal (sony_dvb_int32_window_t* pWindow)
{
    int32_t t = 0 ;
    uint16_t i = 0 ;

    if (!pWindow)
        return t;

    /* Protect against out of range index. */
    if (pWindow->count > SONY_DVB_MONITORT2_WINDOW_MAX_LENGTH)
        return t ;

    for (i = 0 ; i < pWindow->count ; i++) {
        t += pWindow->window[i] ;
    }
    return t ;
}

static int32_t window_GetAverage (sony_dvb_int32_window_t* pWindow)
{
    int32_t total = 0 ;

    if (!pWindow)
        return 0;
    
    if (pWindow->count > 0) {
        total = window_GetTotal (pWindow) ;
        total = total >= 0 ? (total + pWindow->count / 2) : (total - pWindow->count / 2) ;
        return total / pWindow->count ;
    }
    return 0 ;
}

sony_dvb_result_t dvb_demod_monitorT2_FilteredMER (sony_dvb_demod_t* pDemod,
                                                   sony_dvb_demod_mer_filter_t* pMerFilter,
                                                   int32_t* pInstMER,
                                                   int32_t* pMER)
{
    sony_dvb_result_t result = SONY_DVB_OK ;
    int32_t max = 0 ;
    int32_t avg = 0 ;
    int32_t val = 0 ;
    SONY_DVB_TRACE_ENTER ("dvb_demod_monitorT2_FilteredMER");

    if ((!pDemod) || (!pMerFilter) || (!pInstMER) || (!pMER)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }
    if ((pMerFilter->peakWindow.length == 0) || (pMerFilter->sampleWindow.length == 0)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* Software state check */
    if (pDemod->state != SONY_DVB_DEMOD_STATE_ACTIVE) {
        /* This api is accepted in ACTIVE state only */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    if (pDemod->system != SONY_DVB_SYSTEM_DVBT2) {
        /* Not DVB-T2 */
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_SW_STATE);
    }

    /* Set bank 0x20. */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B0_REGC_BANK, CXD2820R_B20) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /* Set the number of symbols the MER averages over. 1 = one value per symbol. */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B20_MER_CTL2, 0x01) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /* Take multiple samples to fill up filters quickly. */
    {
        uint8_t i = 0;
        for (i = 0 ; i < SONY_DVB_MONITORT2_MER_COUNT ; i++) {

            result = dvb_demod_monitorT2_MER (pDemod, pInstMER);
            if (result != SONY_DVB_OK) {
                (void) pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B0_REGC_BANK, CXD2820R_B20);
                (void) pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B20_MER_CTL2, 0x14);
                SONY_DVB_TRACE_RETURN (result);
            }

            /* Add the filter to the samples. */
            window_Add (&(pMerFilter->sampleWindow), *pInstMER);
            if (pMerFilter->sampleWindow.count == pMerFilter->sampleWindow.length) {
                
                /* Assume normal distribution and max lies at 99.999% (3 x var) */
                /* Rounding error in val calculation not significant. */
                max = window_GetMax (&(pMerFilter->sampleWindow)) ;
                avg = window_GetAverage (&(pMerFilter->sampleWindow)) ;
                val = ((max - avg) / 2) + avg ;
                window_Add (&(pMerFilter->peakWindow), val);

                /* Re-initialise sample window. */
                window_Init(&(pMerFilter->sampleWindow), SONY_DVB_MONITORT2_MER_SAMPLE_WLENGTH);
            }
        }
    }

    /* Set bank 0x20. */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B0_REGC_BANK, CXD2820R_B20) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    /* Set the number of symbols the MER averages over. 20 (0x14) = default value. */
    if (pDemod->pI2c->WriteOneRegister (pDemod->pI2c, pDemod->i2cAddressT2, CXD2820R_B20_MER_CTL2, 0x14) != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_I2C);
    }

    if (pMerFilter->peakWindow.count != 0) {
        *pMER = window_GetAverage (&(pMerFilter->peakWindow)) ;
    }
    else {
        /* Rounding error in val calculation not significant. */
        max = window_GetMax (&(pMerFilter->sampleWindow)) ;
        avg = window_GetAverage (&(pMerFilter->sampleWindow)) ;
        val = ((max - avg) / 2) + avg ;
        *pMER = val ;
    }

    SONY_DVB_TRACE_RETURN (result);
}

sony_dvb_result_t dvb_demod_monitorT2_InitMERFilter (sony_dvb_demod_mer_filter_t* pMerFilter)
{
    sony_dvb_result_t result = SONY_DVB_OK ;
    SONY_DVB_TRACE_ENTER ("dvb_demod_monitorT2_InitMERFilter");

    if ((!pMerFilter)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* Initial both windows. */
    window_Init (&(pMerFilter->peakWindow), SONY_DVB_MONITORT2_MER_PEAK_WLENGTH);
    window_Init (&(pMerFilter->sampleWindow), SONY_DVB_MONITORT2_MER_SAMPLE_WLENGTH);

    SONY_DVB_TRACE_RETURN (result);
}
