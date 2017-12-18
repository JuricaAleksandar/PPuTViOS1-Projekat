#include "mt_fe_def.h"

typedef enum
{
	MT_EX_PSTNR_STOP		 = 0x60,		/* Stop positioner movement */
	MT_EX_PSTNR_LIMIT_OFF	 = 0x63,		/* Disable limits */
	MT_EX_PSTNR_LIMIT_E		 = 0x66,		/* Set east limit */
	MT_EX_PSTNR_LIMIT_W		 = 0x67,		/* Set west limit */
	MT_EX_PSTNR_DRV_E		 = 0x68,		/* Drive motor east */
	MT_EX_PSTNR_DRV_W		 = 0x69,		/* Drive motor west */
	MT_EX_PSTNR_STORE_NN	 = 0x6A,		/* Store satellite position and enable limits */
	MT_EX_PSTNR_GOTO_NN		 = 0x6B,		/* Drive Motor to Satellite Position nn */
	MT_EX_PSTNR_GOTO_ANG	 = 0x6E,		/* Goto Angular Position */
	MT_EX_PSTNR_RECALCULATE	 = 0x6F,		/* Re-calculate */
}MT_Example_Positioner_CMD;

typedef enum
{
	MT_EX_PSTER_DRV_CONTINUE	 = 0,		/* Drive positioner continuously */
	MT_EX_PSTER_DRV_TIMEOUT,				/* Drive positioner with time out in s, 1 ~ 127 */
	MT_EX_PSTER_DRV_STEPS,					/* Drive positioner with steps, 1 ~ 128 */
}MT_Example_Positioner_Drive_Mode;



MT_FE_RET MtExamplePositionControl(MT_Example_Positioner_CMD cmd, U32 param)
{
	MT_FE_DiSEqC_MSG	msg;

	MT_Example_Positioner_Drive_Mode mode = (param >> 24) & 0xFF;
											/* defines the positioner drive mode */

	U8 data = (U8)param & 0xFF;				/* defines the positioner data, for example NN, steps or timeout time */


	msg.data_send[0] = 0xE0;
	msg.data_send[1] = 0x31;
	msg.data_send[2] = cmd;
	msg.size_send = 4;
	msg.is_enable_receive = FALSE;

	switch(cmd)
	{
		case MT_EX_PSTNR_STOP:
		case MT_EX_PSTNR_LIMIT_OFF:
		case MT_EX_PSTNR_LIMIT_E:
		case MT_EX_PSTNR_LIMIT_W:
			msg.size_send = 3;
			break;

		case MT_EX_PSTNR_DRV_E:
		case MT_EX_PSTNR_DRV_W:
			if(mode == MT_EX_PSTER_DRV_CONTINUE)
			{
				msg.data_send[3] = 0;
			}
			else if(mode == MT_EX_PSTER_DRV_TIMEOUT)
			{
				/* time out is 1 ~ 127 s */
				msg.data_send[3] = data & 0x7F;
			}
			else if(mode == MT_EX_PSTER_DRV_STEPS)
			{
				/* step is 1 ~ 128, data_send is 0x80 ~ 0xFF */
				msg.data_send[3] = (~data) + 1;
			}
			else
			{
				/* default use continuesly mode */
				msg.data_send[3] = 0;
			}
			break;

		case MT_EX_PSTNR_RECALCULATE:
			msg.data_send[3] = data;
			break;

		case MT_EX_PSTNR_STORE_NN:
		case MT_EX_PSTNR_GOTO_NN:
			msg.data_send[3] = data;
			break;

		case MT_EX_PSTNR_GOTO_ANG:
			msg.size_send = 5;
			if(param == 0)
			{
				msg.data_send[3] = 0xE0;
				msg.data_send[4] = 0x00;
			}
			else
			{
				msg.data_send[3] = (param >> 8) & 0xFF;		// the param defines the degree
				msg.data_send[4] = (param & 0xFF);
			}
			break;

		default:
			msg.data_send[3] = data;
			break;
	}

	msg.is_envelop_mode = FALSE;

	mt_fe_dmd_ds3k_DiSEqC_send_receive_msg(&msg);

    return MtFeErr_Ok;
}


MT_FE_RET ODU_Channel_change(U8 UB, U8 Bank, U16 UB_freq_MHz, U16 channel_freq_MHz, U16 *real_freq_MHz)
{
	MT_FE_RET	ret = MtFeErr_Ok;

	MT_FE_DiSEqC_MSG msg;

	U16 Channel_T;
	U8	Step_size = 4;	/* 4MHz */

	*real_freq_MHz = 0;

	if(UB > 7)
	{
		return MtFeErr_Param;
	}
	if(Bank > 7)
	{
		return MtFeErr_Param;
	}

	Channel_T = (channel_freq_MHz + UB_freq_MHz)/Step_size - 350;

	msg.data_send[0] = 0xE0;
	msg.data_send[1] = 0x10;		/* or 0x00, or 0x11 */
	msg.data_send[2] = 0x5A;
	msg.data_send[3] = (UB << 5) & 0xE0;
	msg.data_send[3] += (Bank << 2) & 0x1C;
	msg.data_send[3] += (Channel_T >> 8) & 0x03;
	msg.data_send[4] = Channel_T & 0xFF;

	msg.size_send = 5;
	msg.is_enable_receive = FALSE;
	msg.is_envelop_mode = FALSE;

	ret = mt_fe_dmd_ds3k_DiSEqC_send_receive_msg(&msg);
	if(ret != MtFeErr_Ok)
	{
		return ret;
	}

	*real_freq_MHz = channel_freq_MHz;
	*real_freq_MHz += UB_freq_MHz;
	*real_freq_MHz /= 4;
	*real_freq_MHz *= 4;
	*real_freq_MHz -= UB_freq_MHz;


	_mt_sleep(400);


	return MtFeErr_Ok;
}

MT_FE_RET ODU_UBxSignal_ON(void)
{
	MT_FE_RET ret = MtFeErr_Ok;

	MT_FE_DiSEqC_MSG msg;

	msg.data_send[0] = 0xE0;
	msg.data_send[1] = 0x10;		/* or 0x00, or 0x11 */
	msg.data_send[2] = 0x5B;
	msg.data_send[3] = 0x00;
	msg.data_send[4] = 0x00;

	msg.size_send = 5;
	msg.is_enable_receive = FALSE;
	msg.is_envelop_mode = FALSE;

	ret = mt_fe_dmd_ds3k_DiSEqC_send_receive_msg(&msg);

	return ret;
}

MT_FE_RET ODU_PowerOFF(U8 UB)
{
	MT_FE_RET ret = MtFeErr_Ok;

	MT_FE_DiSEqC_MSG msg;

	if(UB > 7)
	{
		return MtFeErr_Param;
	}

	msg.data_send[0] = 0xE0;
	msg.data_send[1] = 0x10;		/* or 0x00, or 0x11 */
	msg.data_send[2] = 0x5A;
	msg.data_send[3] = (UB << 5) & 0xE0;
	msg.data_send[4] = 0x00;

	msg.size_send = 5;
	msg.is_enable_receive = FALSE;
	msg.is_envelop_mode = FALSE;

	ret = mt_fe_dmd_ds3k_DiSEqC_send_receive_msg(&msg);

	return ret;
}



