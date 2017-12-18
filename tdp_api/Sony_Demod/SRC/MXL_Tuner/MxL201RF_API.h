/*
 
 Driver APIs for MxL201RF Tuner
 
 Copyright, Maxlinear, Inc.
 All Rights Reserved
 
 File Name:      MxL201RF_API.h

 
 */
#ifndef __MxL201RF_API_H
#define __MxL201RF_API_H

#include "MxL201RF_Common.h"


/******************************************************************************
**
**  Name: MxL_Set_Register
**
**  Description:    Write one register to MxL201RF
**
**  Parameters:    	myTuner				- Pointer to MxL201RF_TunerConfigS
**					RegAddr				- Register address to be written
**					RegData				- Data to be written
**
**  Returns:        MxL_ERR_MSG			- MxL_OK if success	
**										- MxL_ERR_SET_REG if fail
**
******************************************************************************/
MxL_ERR_MSG MxL_Set_Register(MxL201RF_TunerConfigS* myTuner, UINT8 RegAddr, UINT8 RegData);

/******************************************************************************
**
**  Name: MxL_Get_Register
**
**  Description:    Read one register from MxL201RF
**
**  Parameters:    	myTuner				- Pointer to MxL201RF_TunerConfigS
**					RegAddr				- Register address to be read
**					RegData				- Pointer to register read
**
**  Returns:        MxL_ERR_MSG			- MxL_OK if success	
**										- MxL_ERR_GET_REG if fail
**
******************************************************************************/
MxL_ERR_MSG MxL_Get_Register(MxL201RF_TunerConfigS* myTuner, UINT8 RegAddr, UINT8 *RegData);

/******************************************************************************
**
**  Name: MxL_Tuner_Init
**
**  Description:    MxL201RF Initialization
**
**  Parameters:    	myTuner				- Pointer to MxL201RF_TunerConfigS
**
**  Returns:        MxL_ERR_MSG			- MxL_OK if success	
**										- MxL_ERR_INIT if fail
**
******************************************************************************/
MxL_ERR_MSG MxL_Tuner_Init(MxL201RF_TunerConfigS* );

/******************************************************************************
**
**  Name: MxL_Tuner_RFTune
**
**  Description:    Frequency tunning for channel
**
**  Parameters:    	myTuner				- Pointer to MxL201RF_TunerConfigS
**					RF_Freq_Hz			- RF Frequency in Hz
**					BWMHz				- Bandwidth 6, 7 or 8 MHz
**
**  Returns:        MxL_ERR_MSG			- MxL_OK if success	
**										- MxL_ERR_RFTUNE if fail
**
******************************************************************************/
MxL_ERR_MSG MxL_Tuner_RFTune(MxL201RF_TunerConfigS*, UINT32 RF_Freq_Hz, MxL201RF_BW_MHz BWMHz);		

/******************************************************************************
**
**  Name: MxL_Soft_Reset
**
**  Description:    Software Reset the MxL201RF Tuner
**
**  Parameters:    	myTuner				- Pointer to MxL201RF_TunerConfigS
**
**  Returns:        MxL_ERR_MSG			- MxL_OK if success	
**										- MxL_ERR_OTHERS if fail
**
******************************************************************************/
MxL_ERR_MSG MxL_Soft_Reset(MxL201RF_TunerConfigS*);

/******************************************************************************
**
**  Name: MxL_Standby
**
**  Description:    Enter Standby Mode
**
**  Parameters:    	myTuner				- Pointer to MxL201RF_TunerConfigS
**
**  Returns:        MxL_ERR_MSG			- MxL_OK if success	
**										- MxL_ERR_OTHERS if fail
**
******************************************************************************/
MxL_ERR_MSG MxL_Stand_By(MxL201RF_TunerConfigS*);

/******************************************************************************
**
**  Name: MxL_Wakeup
**
**  Description:    Wakeup from Standby Mode (Note: after wake up, please call RF_Tune again)
**
**  Parameters:    	myTuner				- Pointer to MxL201RF_TunerConfigS
**
**  Returns:        MxL_ERR_MSG			- MxL_OK if success	
**										- MxL_ERR_OTHERS if fail
**
******************************************************************************/
MxL_ERR_MSG MxL_Wake_Up(MxL201RF_TunerConfigS*);

/******************************************************************************
**
**  Name: MxL_Enable_LT
**
**  Description:    Enable/Disable the on-chip loop-thru
**
**  Parameters:    	myTuner				- Pointer to MxL201RF_TunerConfigS
**					En_LT				- 0: disable loop-thru, otherwise enable loop-thru
**
**  Returns:        MxL_ERR_MSG			- MxL_OK if success	
**										- MxL_ERR_SET_REG if fail
**
******************************************************************************/
MxL_ERR_MSG MxL_Enable_LT(MxL201RF_TunerConfigS* myTuner, UINT8 En_LT);

/******************************************************************************
**
**  Name: MxL_Check_ChipVersion
**
**  Description:    Return the MxL201RF Chip ID
**
**  Parameters:    	myTuner				- Pointer to MxL201RF_TunerConfigS
**					myChipVersion		- MxL_ChipVersion
**  Returns:        MxL_ERR_MSG			
**
******************************************************************************/
MxL_ERR_MSG MxL_Check_ChipVersion(MxL201RF_TunerConfigS*, MxL201RF_ChipVersion*);

/******************************************************************************
**
**  Name: MxL_RFSynth_Lock_Status
**
**  Description:    RF synthesizer lock status of MxL201RF
**
**  Parameters:    	myTuner				- Pointer to MxL201RF_TunerConfigS
**					isLock				- Pointer to Lock Status
**
**  Returns:        MxL_ERR_MSG			- MxL_OK if success	
**										- MxL_ERR_OTHERS if fail
**
******************************************************************************/
MxL_ERR_MSG MxL_RFSynth_Lock_Status(MxL201RF_TunerConfigS* , BOOL* isLock);

/******************************************************************************
**
**  Name: MxL_REFSynth_Lock_Status
**
**  Description:    REF synthesizer lock status of MxL201RF
**
**  Parameters:    	myTuner				- Pointer to MxL201RF_TunerConfigS
**					isLock				- Pointer to Lock Status
**
**  Returns:        MxL_ERR_MSG			- MxL_OK if success	
**										- MxL_ERR_OTHERS if fail	
**
******************************************************************************/
MxL_ERR_MSG MxL_REFSynth_Lock_Status(MxL201RF_TunerConfigS* , BOOL* isLock);

/******************************************************************************
**
**  Name: MxL_Check_RF_Input_Power
**
**  Description:    Reads the RF Input power
**
**  Parameters:    	myTuner				- Pointer to MxL201RF_TunerConfigS
**					RF_Input_Level		- Pointer to return value, RF input level
**
**  Returns:        MxL_ERR_MSG			- MxL_OK if success	
**										- MxL_ERR_OTHERS if fail	
**
******************************************************************************/
MxL_ERR_MSG MxL_Check_RF_Input_Power(MxL201RF_TunerConfigS* , REAL32* RF_Input_Level);

/******************************************************************************
**
**  Name: MxL_Input_Power_LUT_Reg_Write
**
**  Description:    Overwrite the registers used by LUT for input power reporting
**
**  Parameters:    	myTuner				- Pointer to MxL201RF_TunerConfigS
**					LUT_Index			- Sub-address index of LUT to be written
					LUT_Val				- Value to be written to LUT
**
**  Returns:        MxL_ERR_MSG			- MxL_OK if success	
**										- MxL_ERR_OTHERS if fail	
**
******************************************************************************/
MxL_ERR_MSG MxL_Input_Power_LUT_Reg_Write(MxL201RF_TunerConfigS* , UINT8 LUT_Index, UINT8 LUT_Val);

#endif //__MxL201RF_API_H

