/*------------------------------------------------------------------------------

 <dev:header>
    Copyright(c) 2009 Sony Corporation.

    $Revision: 1.1.2.1 $
    $Author: paul.serridge $

</dev:header>

------------------------------------------------------------------------------*/
/**

 @file    nutune_ft3114.h

          This file provides the NuTune FT3114 driver implementation 
          definitions.

*/
/*----------------------------------------------------------------------------*/

#ifndef NUTUNE_FT3114_H_
#define NUTUNE_FT3114_H_

/*------------------------------------------------------------------------------
 Includes
------------------------------------------------------------------------------*/
#include <sony_dvb_tuner.h>

/*------------------------------------------------------------------------------
 Defines
------------------------------------------------------------------------------*/

/* Tuner I2C address */
#define SONY_DVB_TUNER_NUTUNE_FT3114_ADDRESS		    0xC0 /* 8bit form */

/*------------------------------------------------------------------------------
 Functions
------------------------------------------------------------------------------*/

sony_dvb_result_t nutune_ft3114_Create (uint8_t i2cAddress,
                                      uint32_t xtalFreq,
                                      sony_dvb_i2c_t * pI2c, 
                                      uint32_t configFlags, /* Unused. */
                                      void *user, 
                                      sony_dvb_tuner_t * pTuner);

sony_dvb_result_t nutune_ft3114_Initialize (sony_dvb_tuner_t * pTuner);
sony_dvb_result_t nutune_ft3114_Finalize (sony_dvb_tuner_t * pTuner);
sony_dvb_result_t nutune_ft3114_Tune (sony_dvb_tuner_t * pTuner, 
                                    uint32_t frequency, 
                                    sony_dvb_system_t system,
                                    uint8_t bandWidth);
sony_dvb_result_t nutune_ft3114_Sleep (sony_dvb_tuner_t * pTuner);


#endif /* NUTUNE_FT3114_H_ */
