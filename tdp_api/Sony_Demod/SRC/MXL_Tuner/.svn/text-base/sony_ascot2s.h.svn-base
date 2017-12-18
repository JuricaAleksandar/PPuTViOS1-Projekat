/*------------------------------------------------------------------------------

 <dev:header>
    Copyright(c) 2010 Sony Corporation.

    $Revision$
    $Author$

</dev:header>

------------------------------------------------------------------------------*/
/**

 @file    sony_ascot2s.h

          This file provides the Sony ASCOT2S DVB tuner driver.
*/
/*----------------------------------------------------------------------------*/
#ifndef SONY_ASCOT2S_H_
#define SONY_ASCOT2S_H_

/*------------------------------------------------------------------------------
 Includes
------------------------------------------------------------------------------*/
#include <sony_dvb_tuner.h>

/*------------------------------------------------------------------------------
 Defines
------------------------------------------------------------------------------*/

/* Tuner I2C address */
#define SONY_ASCOT2S_ADDRESS		        0xC0        /**< Tuner I2C address (8bit). */

/**
 @brief ASCOT2S RF step size is <1kHz, so this sets the cutoff frequency 
        in the detected carrier offset to retune.
*/
#define SONY_ASCOT2S_OFFSET_CUTOFF_HZ       50000

/**
 @brief Set if using external XTAL (ref input). e.g If sharing 41MHz XTAL with demodulator.
        Apply as mask to sony_dvb_tuner_t::flags.
*/
#define SONY_ASCOT2S_CONFIG_EXT_REF         0x80000000

/**
 @brief Set if system is used in SECAM region. 
        Apply as mask to sony_dvb_tuner_t::flags. 
*/
#define SONY_ASCOT2S_CONFIG_SECAM_REGION    0x40000000

/**
 @brief Set if system uses an external RFAGC(overload) circuit. By default, an internal 
        RFAGC circuit is used and is recommended.
        Apply as mask to sony_dvb_tuner_t::flags.
*/
#define SONY_ASCOT2S_CONFIG_RFAGC_EXTERNAL  0x20000000

/**
 @brief Set if system uses IFOUTP2/IFOUTN2, instead of IFOUTP1/IFOUTN1 (default).
        Apply as mask to sony_dvb_tuner_t::flags.
*/
#define SONY_ASCOT2S_CONFIG_IFOUT_2         0x10000000

/**
 @brief Set if system uses the AGC2, instead of AGC1 (default).
        Apply as mask to sony_dvb_tuner_t::flags.
*/
#define SONY_ASCOT2S_CONFIG_IFAGC_2         0x08000000

/*------------------------------------------------------------------------------
 Functions
------------------------------------------------------------------------------*/

/**
 @brief Creates an instance of the ASCOT2S tuner driver. 
        Assumes the following system configuration:
        - Digital IF:    IFOUT1.
        - Digital IFAGC: IFAGC1.
 
 @param i2cAddress The I2C address of the ASCOT2S device.
        Typically 0xC0.
 @param xtalFreq The crystal frequency of the tuner (MHz). 
        Supports 4MHz or 41MHz.
 @param pI2c The I2C driver that the tuner driver will use for 
        communication.
 @param configFlags See ::SONY_ASCOT2S_CONFIG_EXT_REF,  
        ::SONY_ASCOT2S_CONFIG_SECAM_REGION and ::SONY_ASCOT2S_CONFIG_RFAGC_EXTERNAL.
 @param user User data.
 @param pTuner The tuner driver instance to create. Memory 
        must have been allocated for this instance before 
        creation.
 
 @return SONY_DVB_OK if successful.
*/
sony_dvb_result_t sony_ascot2s_Create (uint8_t i2cAddress,
                                       uint32_t xtalFreq, 
                                       sony_dvb_i2c_t * pI2c, 
                                       uint32_t configFlags,
                                       void *user, 
                                       sony_dvb_tuner_t * pTuner);

/**
 @brief Initialize the tuner into "Power Save" state.
        Uses the X_pon sequence.

 @param pTuner Instance of the tuner driver.

 @return SONY_DVB_OK if successful.
*/
sony_dvb_result_t sony_ascot2s_Initialize (sony_dvb_tuner_t * pTuner);

/**
 @brief Finalize the driver and places into a power state ("Power Save"). 
        Uses the X_fin sequence.

 @param pTuner Instance of the tuner driver.

 @return SONY_DVB_OK if successful.
*/
sony_dvb_result_t sony_ascot2s_Finalize (sony_dvb_tuner_t * pTuner);

/**
 @brief Tune the ASCOT2S tuner to the specified system, frequency 
        and bandwidth.
        Uses a combination of sequences:
        - A_init
        - D_init
        - X_tune

 @param pTuner Instance of the tuner driver.
 @param frequency RF frequency to tune too (kHz).
 @param system The type of channel to tune too (DVB-T/C/T2).
 @param bandWidth The bandwidth of the channel (MHz).
        - DVB-C: 8MHz only.
        - DVB-T: 6, 7 or 8.
        - DVB-T2: 5, 6, 7 or 8.

 @return SONY_DVB_OK if successful.
*/
sony_dvb_result_t sony_ascot2s_Tune (sony_dvb_tuner_t * pTuner,
                                     uint32_t frequency, 
                                     sony_dvb_system_t system,
                                     uint8_t bandWidth);

/**
 @brief Sleep the ASCOT2S tuner. 
        Places the tuner in a low power state ("Power Save").
        Uses the X_fin sequence.

 @param pTuner Instance of the tuner driver.

 @return SONY_DVB_OK if successful.
*/
sony_dvb_result_t sony_ascot2s_Sleep (sony_dvb_tuner_t * pTuner);

/**
 @brief Monitors the RF signal level in dBx1000 at the 
        input of the tuner.

 @param pTuner Instance of the tuner driver.
 @param ifAgc IFAGC level as read from the demodulator.
 @param rfAinValid Indicates whether or not the RFAIN value 
        is valid and should be used for the RF level calculation.
 @param rfAin The RFAIN level (10-bit) read from the demodulator.
 @param pRFLeveldB The RF level at the tuner input in dBx1000.

 @return SONY_DVB_OK if pRFLeveldB is valid.
*/
sony_dvb_result_t sony_ascot2s_monitor_RFLevel (sony_dvb_tuner_t * pTuner,
                                                uint32_t ifAgc,
                                                uint8_t rfAinValid,
                                                uint32_t rfAin,
                                                int32_t* pRFLeveldB);

#endif /* SONY_ASCOT2S_H_ */
