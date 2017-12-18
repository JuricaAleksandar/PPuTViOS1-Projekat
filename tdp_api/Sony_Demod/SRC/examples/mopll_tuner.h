/*------------------------------------------------------------------------------

 <dev:header>
    Copyright(c) 2009 Sony Corporation.

    $Revision$
    $Author$

</dev:header>

------------------------------------------------------------------------------*/
/**
 @file    mopll_tuner.h

          This file provides an example MOPLL RF tuner driver.

*/
/*----------------------------------------------------------------------------*/

#ifndef MOPLL_TUNER_H_
#define MOPLL_TUNER_H_

/*------------------------------------------------------------------------------
 Includes
------------------------------------------------------------------------------*/
#include <sony_dvb_tuner.h>

/*------------------------------------------------------------------------------
 Defines
------------------------------------------------------------------------------*/

#define SONY_DVB_TUNER_MOPLL_TUNER_ADDRESS		    0xC0        /**< 8bit form */
#define SONY_DVB_TUNER_MOPLL_DVBT_REF_FREQ_HZ       166667      /**< 166.667kHz */  
#define SONY_DVB_TUNER_MOPLL_DVBC_REF_FREQ_HZ       62500       /**< 62.5kHz */
#define SONY_DVB_TUNER_MOPLL_IF_MHZ                 36.0        /**< 36.0MHz */

/*------------------------------------------------------------------------------
 Functions
------------------------------------------------------------------------------*/

/**
 @brief Creates/sets up the MOPLL example tuner driver. 

    @note: Does not allocate any memory.
 
 @param i2cAddress The I2C address of the tuner, typically 
        ::SONY_DVB_TUNER_MOPLL_TUNER_ADDRESS.
 @param xtalFreq UNUSED. The crystal frequency in MHz (set to 4). 
 @param pI2c The I2C driver instance for the tuner driver to use.
 @param configFlags UNUSED.
 @param user User supplied data pointer.
 @param pTuner Pointer to struct to setup tuner driver.

 @param SONY_DVB_OK if successful and tuner driver setup.
*/
sony_dvb_result_t mopll_tuner_Create (uint8_t i2cAddress,
                                      uint32_t xtalFreq,
                                      sony_dvb_i2c_t * pI2c, 
                                      uint32_t configFlags, /* Unused. */
                                      void *user, 
                                      sony_dvb_tuner_t * pTuner);

/**
 @brief Initialize the tuner.

 @param pTuner Instance of the tuner driver.

 @return SONY_DVB_OK if successful.
*/
sony_dvb_result_t mopll_tuner_Initialize (sony_dvb_tuner_t * pTuner);

/**
 @brief Finalize the tuner.

 @param pTuner Instance of the tuner driver.

 @return SONY_DVB_OK if successful.
*/
sony_dvb_result_t mopll_tuner_Finalize (sony_dvb_tuner_t * pTuner);

/**
 @brief Perform RF tuning on tuner.
        After performing the tune, the application can read back 
        ::sony_dvb_tuner_t::frequency field to get the actual 
        tuner RF frequency. The actual tuner RF frequency is 
        dependent on the RF tuner step size.

 @param pTuner Instance of the tuner driver.
 @param frequency RF frequency to tune too (kHz)
 @param system The type of channel to tune too (DVB-T/C/T2).
 @param bandWidth The bandwidth of the channel in MHz.
        - DVB-C: 8MHz only.
        - DVB-T: 6, 7 or 8.
        - DVB-T2: 5, 6, 7 or 8.

 @return SONY_DVB_OK if successful.
*/
sony_dvb_result_t mopll_tuner_Tune (sony_dvb_tuner_t * pTuner, 
                                    uint32_t frequency, 
                                    sony_dvb_system_t system,
                                    uint8_t bandWidth);

/**
 @brief Sleep the tuner device (if supported).
 
 @param pTuner Instance of the tuner driver.

 @return SONY_DVB_OK if successful.
*/
sony_dvb_result_t mopll_tuner_Sleep (sony_dvb_tuner_t * pTuner);

#endif /* MOPLL_TUNER_H_ */
