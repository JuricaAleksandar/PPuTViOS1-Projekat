/*------------------------------------------------------------------------------

 <dev:header>
    Copyright(c) 2009 Sony Corporation.

    $Revision: 1.1.2.1.2.1 $
    $Author: chris.collis $

</dev:header>

------------------------------------------------------------------------------*/
/**
 @file    nutune_ft3114.c

      Implementation for the NuTune FT3114 driver.
      Implementation based on Preliminary data sheet FT-3114
      21/05/2009.
*/
/*----------------------------------------------------------------------------*/


/*------------------------------------------------------------------------------
 Includes
------------------------------------------------------------------------------*/

#include <sony_dvb.h>
#include "nutune_FT3114.h"

/*------------------------------------------------------------------------------
 Internal defines
------------------------------------------------------------------------------*/

#define PLL_TIMEOUT         50                  /* Timeout for PLL lock condition. */
#define PLL_POLL_INTERVAL   10                  /* Polling interval of PLL lock condition. */
#define PLL_RFAGC_WAIT      70                  /* Wait for RFAGC stabilisation. */

#define DVBT_REF_FREQ_HZ    166667              /* 166.667kHz */  
#define DVBC_REF_FREQ_HZ    62500               /* 62.5kHz */
#define FREQ_IF             36000               /* 36MHz */

/*
 NOTE: Frequencies provided in NuTune datasheet refer to the local 
       oscillator frequencies, not the input RF frequencies.
       fosc = fref + 36
       So, the following are adjusted to the RF frequency.
*/
#define FREQ_VL_1           (83000 - FREQ_IF)   /* fosc=83MHz */
#define FREQ_VL_2           (121000 - FREQ_IF)  /* fosc=121MHz */
#define FREQ_VL_3           (141000 - FREQ_IF)  /* fosc=141MHz */
#define FREQ_VL_4           (166000 - FREQ_IF)  /* fosc=166MHz */
#define FREQ_VL_5           (182000 - FREQ_IF)  /* fosc=182MHz */
#define FREQ_VH_1           (286000 - FREQ_IF)  /* fosc=286MHz */
#define FREQ_VH_2           (386000 - FREQ_IF)  /* fosc=386MHz */
#define FREQ_VH_3           (446000 - FREQ_IF)  /* fosc=446MHz */
#define FREQ_VH_4           (466000 - FREQ_IF)  /* fosc=466MHz */
#define FREQ_UH_1           (506000 - FREQ_IF)  /* fosc=506MHz */
#define FREQ_UH_2           (761000 - FREQ_IF)  /* fosc=761MHz */
#define FREQ_UH_3           (846000 - FREQ_IF)  /* fosc=846MHz */
#define FREQ_UH_4           (905000 - FREQ_IF)  /* fosc=905MHz */

#define FREQ_VL_LO          FREQ_VL_1
#define FREQ_VL_HI          FREQ_VL_5
#define FREQ_VH_HI          FREQ_VH_1
#define FREQ_UH_HI          FREQ_UH_4

/*------------------------------------------------------------------------------
 Functions
------------------------------------------------------------------------------*/

static sony_dvb_result_t WritePLL (sony_dvb_tuner_t * pTuner, const uint8_t * siTunerBytes, uint32_t size);
static sony_dvb_result_t WaitPLLLock (sony_dvb_tuner_t * pTuner);

sony_dvb_result_t nutune_ft3114_Create (uint8_t i2cAddress,
                                      uint32_t xtalFreq, 
                                      sony_dvb_i2c_t * pI2c, 
                                      uint32_t configFlags,
                                      void *user, 
                                      sony_dvb_tuner_t * pTuner)
{
    sony_dvb_result_t result = SONY_DVB_OK;

    SONY_DVB_TRACE_ENTER ("nutune_ft3114_Create");

    if ((pTuner == NULL) || (pI2c == NULL)) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    pTuner->Initialize = nutune_ft3114_Initialize;
    pTuner->Finalize = nutune_ft3114_Finalize;
    pTuner->Sleep = nutune_ft3114_Sleep;
    pTuner->Tune = nutune_ft3114_Tune;
    pTuner->system = SONY_DVB_SYSTEM_UNKNOWN;
    pTuner->bandWidth = 0;
    pTuner->frequency = 0;
    pTuner->i2cAddress = i2cAddress;
    pTuner->xtalFreq = xtalFreq;
    pTuner->pI2c = pI2c;
    pTuner->flags = configFlags ; /* Unused */
    pTuner->user = user;

    SONY_DVB_TRACE_RETURN (result);
}

/*------------------------------------------------------------------------------
 Initialise the tuner. No operation required.
------------------------------------------------------------------------------*/
sony_dvb_result_t nutune_ft3114_Initialize (sony_dvb_tuner_t * pTuner)
{
    sony_dvb_result_t result = SONY_DVB_OK;
    SONY_DVB_TRACE_ENTER ("nutune_ft3114_Initialize");
    /* Removes compiler warnings. */
    pTuner = pTuner ;
    SONY_DVB_TRACE_RETURN (result);
}

sony_dvb_result_t nutune_ft3114_Finalize (sony_dvb_tuner_t * pTuner)
{
    sony_dvb_result_t result = SONY_DVB_OK;
    SONY_DVB_TRACE_ENTER ("nutune_ft3114_Finalize");
    /* Removes compiler warnings. */
    pTuner = pTuner ;
    SONY_DVB_TRACE_RETURN (result);
}

/*------------------------------------------------------------------------------
 Setup the channel bandwidth.
 Tune to the specified frequency.
 Wait for lock.

------------------------------------------------------------------------------*/
sony_dvb_result_t nutune_ft3114_Tune (sony_dvb_tuner_t * pTuner, 
                                      uint32_t frequency, 
                                      sony_dvb_system_t system,
                                      uint8_t bandWidth)
{
    sony_dvb_result_t result = SONY_DVB_OK;
    uint8_t data[5] ;
    SONY_DVB_TRACE_ENTER ("nutune_ft3114_Tune");
		printf("\nnutune_ft3114_Tune  frequency %d system %d bandWidth %d\n",frequency,system,bandWidth);
    if (!pTuner) {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* Set current tuning parameter to struct */
    pTuner->system = system;
    pTuner->bandWidth = bandWidth;

    /*
     Write Format: 
     [ADB] [DB1] [DB2] [CB1] [BB] [CB2]
     ADB: Address Byte
     DB1/2: Data Bytes

            N = fosc / fref ;

            fosc = fchannel + 36MHz

            fref = 166.67kHz (DVB-T/T2)
            fref = 62.5kHz (DVB-C)

            N = (fchannel + 36) / 166.67kHz

            36/166.67 = 216
            36/62.5 = 576

            DVB-T: N = fchannel / 166.667 + 216
            DVB-C: N = fcannel / 62.5 + 576

     CB1: Control Byte 1
        1 0 ATP2 ATP1 ATP0 RS2 RS1 RS0

     BB: Bandswitch Byte
        CP1 CP0 0 P5 BS4 BS3 BS2 BS1

     CB2: Control Byte 2
        1 1 ATC STBY 0 0 1 XTO
    */

    data[2] = 0x80 ;    /* Mask, 166.667kHz */
    data[3] = 0x00 ;    
    data[4] = 0xE3 ;    /* Mask: XTO=Disabled, Standby=OFF, ATC=1 */
    {
        /* Setup ref divider. */
        uint16_t n = 0x00 ;
        
        switch (system)
        {
        /* Intentional fall through. */
        case SONY_DVB_SYSTEM_DVBT:
        case SONY_DVB_SYSTEM_DVBT2:
            n = (uint16_t)((frequency * 6) / 1000 + 216) ;
            pTuner->frequency = (uint32_t) (((uint32_t)n)* DVBT_REF_FREQ_HZ - 36 * 1000 * 1000) / 1000 ;
            
            break ;
        case SONY_DVB_SYSTEM_DVBC:
            n = (uint16_t)((frequency * 16) / 1000 + 576) ;
            pTuner->frequency = (uint32_t) (((uint32_t)n) * DVBC_REF_FREQ_HZ - 36 * 1000 * 1000) / 1000 ;
            data[2] |= 0x03 ;   /* 62.5kHz */
            break ;
        default:
            SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
            /* break ; prevent warnings */
        }

        data[0] = (uint8_t)((n >> 8) & 0x7F) ;
        data[1] = (uint8_t)(n & 0xFF) ;
    }

    /* Setup RFAGC takeover point. 109dBuV */
    /* @todo: Get DVB-C value. */
    data[2] |= (0x01 << 3) ;

    /* Bandswitch */
    if ((frequency >= FREQ_VL_LO) && (frequency <= FREQ_VL_HI)) {
        data[3] |= (0x01) ;
    }
    else if ((frequency > FREQ_VL_HI) && (frequency <= FREQ_VH_HI)) {
        data[3] |= (0x02) ;
    }
    else if ((frequency > FREQ_VH_HI) && (frequency <= FREQ_UH_HI)) {
        data[3] |= (0x08) ;
    }
    else {
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
    }

    /* Charge Pump */
    if ( ((frequency > FREQ_VL_2) && (frequency <= FREQ_VL_3)) ||
         ((frequency > FREQ_VH_1) && (frequency <= FREQ_VH_2)) ||
         ((frequency > FREQ_UH_1) && (frequency <= FREQ_UH_2)) ) {
        data[3] |= (0x01 << 6) ;
    }
    else if ( ((frequency > FREQ_VL_3) && (frequency <= FREQ_VL_4)) ||
         ((frequency > FREQ_VH_2) && (frequency <= FREQ_VH_3)) ||
         ((frequency > FREQ_UH_2) && (frequency <= FREQ_UH_3)) ) {
        data[3] |= (0x02 << 6) ;
    } 
    else if ( ((frequency > FREQ_VL_4) && (frequency <= FREQ_VL_5)) ||
         ((frequency > FREQ_VH_3) && (frequency <= FREQ_VH_4)) ||
         ((frequency > FREQ_UH_3) && (frequency <= FREQ_UH_4)) ) {
        data[3] |= (0x03 << 6) ;
    }
    
    /* Setup bandwidth */
    switch (bandWidth)
    {
    /* Intentional fall through. */
    case 5:
    case 6:
    case 7:
        data[3] |= 0x10 ;
        break ;
    case 8: break ;
    default:
        SONY_DVB_TRACE_RETURN (SONY_DVB_ERROR_ARG);
        /* break ; prevent warnings */
    }

    result = WritePLL (pTuner, data, sizeof(data));
    if (result != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (result);
    }

    /* Wait for PLL lock indication. */
    result = WaitPLLLock (pTuner);
    if (result != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (result);
    }

    /* Wait for RFAGC stabilisation. */
    SONY_DVB_SLEEP (PLL_RFAGC_WAIT);

    /* Re-send, but with the high RFAGC current OFF. */
    data[4] &= ~0x20 ;
    result = WritePLL (pTuner, data, sizeof(data));
    if (result != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (result);
    } 
    
    /* Wait for PLL lock indication. */
    result = WaitPLLLock (pTuner);
    if (result != SONY_DVB_OK) {
        SONY_DVB_TRACE_RETURN (result);
    }

    SONY_DVB_TRACE_RETURN (result);
}

/*------------------------------------------------------------------------------
 Sleep the tuner module.
------------------------------------------------------------------------------*/
sony_dvb_result_t nutune_ft3114_Sleep (sony_dvb_tuner_t * pTuner)
{
    sony_dvb_result_t result = SONY_DVB_OK;

    SONY_DVB_TRACE_ENTER ("nutune_ft3114_Sleep");
    /* Removes compiler warnings. */
    pTuner = pTuner ;
    SONY_DVB_TRACE_RETURN (result);
}

sony_dvb_result_t WritePLL (sony_dvb_tuner_t * pTuner, const uint8_t * siTunerBytes, uint32_t size)
{
    return pTuner->pI2c->Write (pTuner->pI2c, pTuner->i2cAddress, (uint8_t*)siTunerBytes, size, (SONY_I2C_START_EN | SONY_I2C_STOP_EN));
}

sony_dvb_result_t WaitPLLLock (sony_dvb_tuner_t * pTuner)
{
    sony_dvb_result_t result = SONY_DVB_OK;
#if 0
    uint8_t i = 0 ;
    uint8_t data = 0 ;
#endif

    SONY_DVB_TRACE_ENTER ("WaitPLLLock");

#if 0
    for (i = 0 ; i < PLL_TIMEOUT / PLL_POLL_INTERVAL ; i++) {

        result = pTuner->pI2c->Read (pTuner->pI2c, pTuner->i2cAddress, &data, 1, (SONY_I2C_START_EN | SONY_I2C_STOP_EN)) ;
        SONY_DVB_TRACE2 ("WaitPLLLock : I2CRead result %d\r\n",result);
        if (result != SONY_DVB_OK) {
            break ;
        }

        if (data & 0x40) {
            break ;
        }
        result = SONY_DVB_ERROR_TIMEOUT ;
        SONY_DVB_SLEEP (PL0_POLL_INTERVAL);
    }
#else
    SONY_DVB_SLEEP (PLL_TIMEOUT);
#endif
    SONY_DVB_TRACE_RETURN (result);
}
