/****************************************************************************
* MONTAGE PROPRIETARY AND CONFIDENTIAL
* Montage Technology (Shanghai) Inc.
* All Rights Reserved
* --------------------------------------------------------------------------
*
* File:				mt_fe_example.c
*
* Current version:	0.00.00
*
* Description:		M88DS3103/DS3002B/DS300X IC Driver.
*
* Log:	Description				Version		Date		Author
*		---------------------------------------------------------------------
*
****************************************************************************/
#include <stdio.h> 
#include <math.h>
#include <unistd.h>
#include <stdint.h>
//#include "mt_fe_common_functions.h"
				
//#include "sttbx.h"

#ifndef uint
typedef unsigned int uint;
#endif

#include "mt_fe_def.h" 
#include "mt_fe_i2c.h"
#include "i2c.h"

#define QUEUE_ID 0x8888

extern MT_FE_TYPE	g_current_type;
 
//void os_get_mssage(OS_QUEUE QUEUE_ID, OS_MESSAGE message, U32 time_out);
//void os_send_mssage(OS_QUEUE QUEUE_ID, OS_MESSAGE message);


typedef enum _MT_FE_OS_STATE
{
	OS_TIMEOUT
}MT_FE_OS_STATE;


typedef enum _MT_FE_STATE
{
	MT_FE_STATE_IDLE
	,MT_FE_STATE_CONNETED
	,MT_FE_STATE_CONNETING
	,MT_FE_STATE_NO_SIGNAL
}MT_FE_STATE;


typedef enum _MT_FE_CMD
{
	MT_FE_CMD_CONNECT
	,MT_FE_CMD_GET_CHAN_INFO
	,MT_FE_CMD_GET_SIGNAL_INFO
	,MT_fE_CMD_SLEEP
	,MT_FE_CMD_WAKE_UP
	,MT_FE_CMD_DISEQC_SET_LNB
	,MT_FE_CMD_DISEQC_SEND_TONE_BURST
	,MT_FE_CMD_DISEQC_SEND_RECEIVE_MSG
	,MT_FE_CMD_BLIND_SCAN
	,MT_FE_CMD_CANCEL_BLIND_SCAN
}MT_FE_CMD;

typedef enum _MT_FE_RATATE_DIRECTION
{
	MT_FE_ROTATE_WEST
	,MT_FE_ROTATE_EAST
}MT_FE_RATATE_DIRECTION;


typedef enum _MT_FE_RATATE_MODE
{
	MT_FE_ROTATE_STEP
	,MT_FE_ROTATE_CONTINOUS
}MT_FE_RATATE_MODE;



int main_old(void)
{
	int message;
	S8			snr, strength;
	U8			switch_port_no, horizontal_polarization, connect_time;
	U8			direction, rotate_mode, lock_timeout_cnt;
	S32			ret_time;
	MT_FE_STATE	state = MT_FE_STATE_IDLE;
	U32			cnt;
	U32			freq_MHz;
	U32			sym_rate_KSs;
	U32			total_packags, err_packags;
	BOOL		lnb_on_off, on_off_22k, envelop_mode;
	MT_FE_LNB_VOLTAGE	choice_13v_18v;
	MT_FE_LOCK_STATE lock_state;
	MT_FE_CHAN_INFO_DVBS2 chan_info;
	MT_FE_TYPE  dvbs_type;
	MT_FE_DiSEqC_MSG msg;
	MT_FE_DiSEqC_TONE_BURST mode;
	MT_FE_BAND_TYPE			band;
	MT_FE_LNB_LOCAL_OSC 	local_osc;
	MT_FE_BS_TP_INFO		bs;
	U32						begin_freqMHz, end_freqMHz;

	MT_FE_RET	ret = MtFeErr_Ok;


	// init I2C
	I2C_Init();
	
	
	/*alloc the memeory to store the scanned and locked TP info*/
	MT_FE_TP_INFO blindscaninfo[1000] = {0};

	void (*callback)(void);

	/*initialize the parameter*/
	bs.p_tp_info	 = blindscaninfo;
	bs.tp_max_num	 = 1000;


	connect_time = 0;


	mt_fe_dmd_ds3k_init();


	while (1)
	{
		// test
		//ret = os_get_mssage(QUEUE_ID, message, 100)	/*	timeout = 100ms	*/

//		if (ret_time != OS_TIMEOUT)
		{
			printf(" Prvi switch \n");
			/*App can use the state to show "no signal" and "connecting status" info bar.*/
			switch (state)
			{
				case MT_FE_STATE_NO_SIGNAL:
				case MT_FE_STATE_CONNETING:
					mt_fe_dmd_ds3k_get_lock_state(&lock_state);


					if (lock_state == MtFeLockState_Locked)
					{
						/* change state */
						state = MT_FE_STATE_CONNETED;

						/*
							TODO:
								send message to APP, that demodulater has Locked !

								call function 'mt_fe_dmd_get_chan_info_code_dd2k' to
								get the chan_info_code, and send it to APP.
						*/
						mt_fe_dmd_ds3k_get_channel_info(&chan_info);


						cnt = 0;
						connect_time = 0;

						break;
					}
					else if ((lock_state == MtFeLockState_Unlocked)||(lock_state == MtFeLockState_Waiting))
					{
						cnt++;
					}


					if(dvbs_type == MtFeType_DvbS)
					{
						if(sym_rate_KSs >= 2000)		lock_timeout_cnt = 10;	//1s
						else							lock_timeout_cnt = 30;	//3s
					}
					else if(dvbs_type == MtFeType_DvbS2)
					{
						if(sym_rate_KSs >= 10000)		lock_timeout_cnt = 8;  //800ms
						else if(sym_rate_KSs >= 5000)	lock_timeout_cnt = 15; //1.5s
						else if(sym_rate_KSs >= 2000)	lock_timeout_cnt = 30; //3s
						else							lock_timeout_cnt = 60; //6s
					}
					else if(dvbs_type == MtFeType_DvbS_Unknown)
					{
						if(sym_rate_KSs >= 10000)		lock_timeout_cnt = 13; //1.3s
						else if(sym_rate_KSs >= 4000)	lock_timeout_cnt = 32; //3.2s
						else if(sym_rate_KSs >= 2000)	lock_timeout_cnt = 35; //3.5s
						else							lock_timeout_cnt = 85; //8.5s
					}


					if (cnt > lock_timeout_cnt)
					{
						/*
							TODO:
								send message to APP, that demodulater Unlock !
						*/


						/*	re-connect	*/
						if(connect_time == 0)
						{
							ret = mt_fe_dmd_ds3k_connect(freq_MHz, sym_rate_KSs, dvbs_type);
						}
						else if(connect_time == 1)
						{
							ret = mt_fe_dmd_ds3k_connect(freq_MHz-2, sym_rate_KSs, dvbs_type);
						}
						else if(connect_time == 2)
						{
							ret = mt_fe_dmd_ds3k_connect(freq_MHz+2, sym_rate_KSs, dvbs_type);
						}

						if(ret == MtFeErr_Ok)
						{
							if(connect_time >= 3)
							{
								state = MT_FE_STATE_NO_SIGNAL;
								connect_time =0;
							}
							else
							{
								connect_time++;
							}
							/*	if unlock, reset cnt  */
							cnt = 0;
						}
					}
					break;

				case MT_FE_STATE_CONNETED:
					mt_fe_dmd_ds3k_get_lock_state(&lock_state);

					if (lock_state == MtFeLockState_Locked)
						cnt = 0;
					else
						cnt ++;

					if (cnt > 10)
					{
						/*
							TODO:
								send message to APP, that demodulater Drop !
						*/


						/*	re-connect	*/
						ret = mt_fe_dmd_ds3k_connect(freq_MHz, sym_rate_KSs, dvbs_type);
						if(ret == MtFeErr_Ok)
						{
							/*	change state	*/
							state = MT_FE_STATE_CONNETING;

							/*	if drop, reset chan_info_code	*/
							cnt = 0;
							connect_time++;
						}
					}
					break;

				case MT_FE_STATE_IDLE:
				default:
					break;
			}

			continue;
		}

        printf(" Drugi switch \n");
		switch(message)
		{
			case MT_FE_CMD_CONNECT:
				/*
				Get frequency and symbol rate and dvb_type from app.

				1.  frequency------- UNIT: MHz
				2.  symbol rate----- UNIT: KSs
				3.  dvbs_type-------- may be the three types:

						"MtFeType_DvbS"
						"MtFeType_DvbS2"
						"MtFeType_DvbS_Unknown"

					If you do not know the modulator mode, you can set the dvbs_type as "MtFeType_DvbS_Unknown".
				It will try DVBS and DVBS2 mode automatically.*/
				freq_MHz       = 1500;
				sym_rate_KSs   = 27500;
				dvbs_type	   = MtFeType_DvbS_Unknown;

				ret = mt_fe_dmd_ds3k_connect(freq_MHz, sym_rate_KSs, dvbs_type);
				if(ret != MtFeErr_Ok)
				{
					// connect operation failed
					state = MT_FE_STATE_IDLE;
					cnt = 0;
					connect_time = 0;
				}
				else
				{
					state = MT_FE_STATE_CONNETING;
					cnt   = 0;

					connect_time++;
				}

				break;

			case MT_FE_CMD_BLIND_SCAN:
				/*
				Get parametres from app to do blind scan.

				1.  begin_freqMHz-------   UNIT: MHz
				2.  end_freqMHz  -------   UNIT: MHz
				3.  local_osc    -------   two types:

						"MtFeLNB_Single_Local_OSC" --> single osc
						"MtFeLNB_Dual_Local_OSC"   --> dual osc


				4.	band	     -------  two types:

						"MtFeBand_C" ---> c band
						"MtFeBand_Ku" ---> Ku band

				5.	callback     -------  function to do something(parse TS stream, eg.) after TP locked/unlocked*/
                
                printf(" Blind scan \n");

				begin_freqMHz	 = 1000;
				end_freqMHz		 = 1500;
				local_osc		 = MtFeLNB_Single_Local_OSC;
				band			 = MtFeBand_C;
				callback		 = NULL;
				

				/*Detail Scan: times = 2 or more; Normal Scan: times = 1;*/
				/*Segment Lock:    algorithm = MT_FE_BS_ALGORITHM_A;
				  Whole Lock Mode: algorithm = MT_FE_BS_ALGORITHM_B;*/
				bs.bs_times 	 = 2;		//recommdated blindscan times = MT_FE_BS_TIMES = 2
				bs.bs_algorithm	 = MT_FE_BS_ALGORITHM_A;


				/*register callback function to do something(parse TS stream, eg.)*/
				mt_fe_dmd_ds3k_register_notify(callback);

				/*save bs*/
				if(local_osc == MtFeLNB_Single_Local_OSC)
				{
					mt_fe_dmd_ds3k_set_LNB(MtFe_True, MtFe_False, MtFeLNB_18V, 0);
					mt_fe_dmd_ds3k_blindscan(begin_freqMHz, end_freqMHz, &bs);

					mt_fe_dmd_ds3k_set_LNB(MtFe_True, MtFe_False, MtFeLNB_13V, 0);
					mt_fe_dmd_ds3k_blindscan(begin_freqMHz, end_freqMHz, &bs);
				}
				else
				{
					if(band == MtFeBand_Ku)
					{
						mt_fe_dmd_ds3k_set_LNB(MtFe_True, MtFe_False, MtFeLNB_18V, 0);
						mt_fe_dmd_ds3k_blindscan(begin_freqMHz, end_freqMHz, &bs);

						mt_fe_dmd_ds3k_set_LNB(MtFe_True, MtFe_False, MtFeLNB_13V, 0);
						mt_fe_dmd_ds3k_blindscan(begin_freqMHz, end_freqMHz, &bs);

						mt_fe_dmd_ds3k_set_LNB(MtFe_True, MtFe_True, MtFeLNB_18V, 0);
						mt_fe_dmd_ds3k_blindscan(begin_freqMHz, end_freqMHz, &bs);

						mt_fe_dmd_ds3k_set_LNB(MtFe_True, MtFe_True, MtFeLNB_13V, 0);
						mt_fe_dmd_ds3k_blindscan(begin_freqMHz, end_freqMHz, &bs);
					}
					else
					{
						mt_fe_dmd_ds3k_set_LNB(MtFe_True, MtFe_False, MtFeLNB_18V, 0);
						mt_fe_dmd_ds3k_blindscan(begin_freqMHz, end_freqMHz, &bs);
					}
				}
				break;


			case MT_FE_CMD_CANCEL_BLIND_SCAN:
				/*set the flag to cancel blind scan process*/
				mt_fe_dmd_ds3k_blindscan_abort(MtFe_True);
				break;

			case MT_FE_CMD_GET_CHAN_INFO:
				/*
					TODO:
						call function 'mt_fe_dmd_ds3k_get_channel_info' to
						get the channel infomation, and send it to APP.
						pls see structure MT_FE_CHAN_INFO_DVBS2
				*/
				mt_fe_dmd_ds3k_get_channel_info(&chan_info);
				break;

			case MT_FE_CMD_GET_SIGNAL_INFO:
				/*
					TODO:
						1. call 'mt_fe_dmd_ds3k_get_snr' to get the signal quality,
						2. call 'mt_fe_dmd_ds3k_get_per' to get the error packages and total packages
							to calculate the per.
						3. call 'mt_fe_dmd_ds3k_get_strength' to get the signal strength

					You can use these APIs to send the signal infomatio to APP.
				*/

				mt_fe_dmd_ds3k_get_lock_state(&lock_state);

				if(lock_state == MtFeLockState_Locked)
				{
					mt_fe_dmd_ds3k_get_snr(&snr);
					mt_fe_dmd_ds3k_get_strength(&strength);

					/*In DVBS2 mode, get PER value; DVBS mode gets the BER value*/
					/*per/ber = err_packags/total_packags*/
					mt_fe_dmd_ds3k_get_per(&total_packags, &err_packags);
				}
				else
				{
					snr = 0;
					strength = 0;
					err_packags = 0;		/*error bit in DVBS mode; error packegs in DVBS2 mode*/
					total_packags = 0xffff;	/*total bits in DVBS mode; total packegs in DVBS2 mode*/
				}
				break;


			case MT_fE_CMD_SLEEP:
				/*
					Sleep DS300x.
				*/
				mt_fe_dmd_ds3k_sleep();
				break;

			case MT_FE_CMD_WAKE_UP:
				/*
					Wake up DS300x and reconnect TP.
				*/
				mt_fe_dmd_ds3k_wake_up();
				mt_fe_dmd_ds3k_connect(freq_MHz, sym_rate_KSs, dvbs_type);

				break;


			case MT_FE_CMD_DISEQC_SET_LNB:
				/*
					TODO:
						You can call API 'mt_fe_dmd_ds3k_set_LNB' to set LNB on and select the LNB voltage
					according to the designed circuit.*/

				lnb_on_off 	= MtFe_True;
				on_off_22k 	= MtFe_True;
				choice_13v_18v = MtFeLNB_18V;
				envelop_mode	= MtFe_False;

				mt_fe_dmd_ds3k_set_LNB(lnb_on_off, on_off_22k, choice_13v_18v, envelop_mode);

				break;


			case MT_FE_CMD_DISEQC_SEND_TONE_BURST:

				mode = MtFeDiSEqCToneBurst_Moulated;
				envelop_mode = MtFe_False;

				mt_fe_dmd_ds3k_DiSEqC_send_tone_burst(mode, envelop_mode);
				break;


			case MT_FE_CMD_DISEQC_SEND_RECEIVE_MSG:

				/* 1. DiSEqC switch control application example */
#if BUILD_FIX
				switch_port_no 			= ??; 	/*switch port*/
				horizontal_polarization = ??; 	/*polarization voltage*/
				on_off_22k				= ??;   /*22k signal on/off*/
				envelop_mode			= ??;
#endif //BUILD_FIX

				/* STEP1: LNB on, 22K off, 18V/13V */
				mt_fe_dmd_ds3k_set_LNB(MtFe_True, MtFe_False, MtFeLNB_18V/MtFeLNB_13V, 0);

				/* STEP2: Send DiSEqC Message */
				/*maybe the one or more message byte(s) should be modified with some DiSEqC switches*/
				msg.data_send[0] = 0xE0; /*Framing Byte*/
				msg.data_send[1] = 0x10; /*Address Byte*/
				msg.data_send[2] = 0x38; /*Command Byte*/
				msg.data_send[3] = 0xF0 + (switch_port_no - 1)*4; /*Data Byte*/
				msg.size_send = 4;

				if(horizontal_polarization == MtFeLNB_18V)  msg.data_send[3] |= 0x02; /*Bit 1 set 1*/
				if(on_off_22k == MtFe_True)					msg.data_send[3] |= 0x01; /*Bit 0 set 1*/
				/*only send data to device*/
				msg.is_enable_receive = FALSE;
				msg.is_envelop_mode = FALSE;
				mt_fe_dmd_ds3k_DiSEqC_send_receive_msg(&msg);


				/*STEP3: LNB on, 22K on, 18V/13V, Maybe this step is not necessary if needn't set 22K signal on*/
				/*LNB on, 22K on, 18V/13V*/
				mt_fe_dmd_ds3k_set_LNB(MtFe_True, MtFe_True, MtFeLNB_18V/MtFeLNB_13V, 0);

				/*STEP4: Connect the TP*/
				mt_fe_dmd_ds3k_connect(freq_MHz, sym_rate_KSs, dvbs_type);






				/* 2. DiSEqC Motor Rotation control application*/

				direction 	= MT_FE_ROTATE_EAST ? 0x68 : 0x69; /*rotate direction*/
				rotate_mode	= MT_FE_ROTATE_STEP ? 0xFF : 0x00; /*rotate mode*/

				/*2.1 Motor Rotation Start*/
				/* maybe the one or more message byte(s) should be modified with some DiSEqC motors*/
				msg.data_send[0] = 0xE0; 		/*Framing Byte*/
				msg.data_send[1] = 0x31; 		/*Address Byte*/
				msg.data_send[2] = direction; 	/*Command Byte1*/
				msg.data_send[3] = rotate_mode; /*Command Byte2*/
				msg.size_send = 4;

				/*only send data to device*/
				msg.is_enable_receive = FALSE;
				msg.is_envelop_mode = FALSE;

				mt_fe_dmd_ds3k_DiSEqC_send_receive_msg(&msg);


				/*2.2 Motor Rotation Stop*/
				/* maybe the one or more message byte(s) should be modified with some DiSEqC motors*/
				msg.data_send[0] = 0xE0; 		/*Framing Byte*/
				msg.data_send[1] = 0x31; 		/*Address Byte*/
				msg.data_send[2] = 0x60; 		/*Command Byte*/
				msg.size_send = 3;

				/*only send data to device*/
				msg.is_enable_receive = FALSE;
				msg.is_envelop_mode = FALSE;

				mt_fe_dmd_ds3k_DiSEqC_send_receive_msg(&msg);


				break;


			default:
				break;
		}
	}
}

MT_FE_RET _mt_fe_ds3k_get_quality(MT_FE_TYPE dtv_type, U32 *signal_quality)
{
	U8 npow1, npow2, spow1, tmp_val;
	U32 tmp;

	S16 cnt = 10;		// calculate total 10 times to smooth the result

	double npow = 0.0, spow = 0.0, snr_val = 0.0, val = 0.0;


	switch(dtv_type)
	{
		case MtFeType_DvbS2:
			while(cnt > 0)
			{
				_mt_fe_dmd_get_reg(0x8C, &npow1);
				_mt_fe_dmd_get_reg(0x8D, &npow2);
				npow += (((npow1 & 0x3F) + (npow2 << 6)) >> 2);

				_mt_fe_dmd_get_reg(0x8E, &spow1);
				spow += ((spow1 * spow1) >> 1);

				cnt --;
			}

			npow /= 10.0;
			spow /= 10.0;

			if(spow == 0)
			{
				snr_val = 0;
			}
			else if(npow == 0)
			{
				snr_val = 22;
			}
			else			//	limit the snr_val valid range to [-3.0, 22.0]
			{
				snr_val = (10 * log10((spow / npow)));

				if(snr_val > 22.0)
					snr_val = 22.0;

				if(snr_val < -3.0)
					snr_val = -3.0;
			}

			*signal_quality = (U32)((snr_val + 3) * 100 / (22 + 3));

			break;

		case MtFeType_DvbS:
			tmp = 0;
			while(cnt > 0)
			{
				_mt_fe_dmd_get_reg(0xFF, &tmp_val);
				tmp += tmp_val;
				cnt--;
			}

			val = tmp / 80.0;

			if(val > 0)
			{
				snr_val = log(val) / log(10) * 10;

				if(snr_val < 4.0)
					snr_val = 4.0;

				if(snr_val > 12.0)
					snr_val = 12.0;


				*signal_quality = (U32)((snr_val - 4) * 100 / 8);
			}
			else
			{
				*signal_quality = 0;
			}
			break;

		default:
			break;
	}

	return MtFeErr_Ok;
}

void DS3000_Tunerset(FREQ, BAUD)
{
   MT_FE_RET res = mt_fe_dmd_ds3k_connect(FREQ, BAUD, MtFeType_DvbS);
   printf(" DS3000_Tunerset = %d \n", res); 
}

int DS3000_SignalQuality(void)
{
	U32 signal_quality;

	_mt_fe_ds3k_get_quality(g_current_type,&signal_quality);
	return signal_quality;


}


int DS3000_SignalLevel(void)
{
	S8 signal_strength;

	//signal_strength = GetSignalStrength();
	mt_fe_dmd_ds3k_get_strength(&signal_strength);
	return signal_strength;
	

}
	


int DS3000_LockCheck(void) 
{
	MT_FE_LOCK_STATE lock_state;


	mt_fe_dmd_ds3k_get_lock_state(&lock_state);
	
	if(lock_state == MtFeLockState_Locked)
	{
		printf(" [DS3000] Lock OK \n");
		return 1;
	}
	else
	{
		printf(" [DS3000] UnLock \n");
		return 0;
	}

	
}



void DS3000_Init(void)
{
	MT_FE_RET res = mt_fe_dmd_ds3k_init();
    printf(" DS3000_Init = %d \n", res);    
}



void _mt_sleep(U32 ms)
{
	usleep(ms);
}

void DS3000_I2C_Test(void)
{
	U8 val;

	printf("[DS3000_Init] Test Menu \n");
	
//	_mt_fe_dmd_set_reg(0x61, 0x87);
//	_mt_fe_dmd_get_reg(0x61,&val);
//	printf("[_mt_fe_dmd_get_reg]  Reg[0x61]= 0x%02x \n",val);

	
//	_mt_fe_tn_set_reg(0x02, 0x77);
//	_mt_fe_tn_get_reg(0x02,&val);
//	printf("[_mt_fe_tn_get_reg]  Reg[0x02]= 0x%02x \n",val);
                          
	_mt_fe_tn_get_reg(0x15,&val);                           
	printf("[_mt_fe_tn_get_reg]  Reg[0x15]= 0x%02x \n",val);

                         
	_mt_fe_tn_get_reg(0x62,&val);                           
	printf("[_mt_fe_tn_get_reg]  Reg[0x62]= 0x%02x \n",val);

	_mt_fe_dmd_get_reg(0x0d,&val);                           
	printf("[_mt_fe_tn_get_reg]  Reg[0x0d]= 0x%02x \n",val);

	_mt_fe_dmd_get_reg(0xd1,&val);                          
	printf("[_mt_fe_tn_get_reg]  Reg[0xd1]= 0x%02x \n",val);//	_mt_fe_tn_set_reg(0x00, 0x03); 

//	_mt_sleep(2);                       
//	_mt_fe_tn_get_reg(0x00,&val);                          
//	printf("[_mt_fe_tn_set_reg]  Reg[0x00]= 0x%02x \n",val);
//
//	_mt_fe_tn_set_reg(0x05, 0xa0); 
//	_mt_sleep(2);                       
//	_mt_fe_tn_get_reg(0x05,&val); 
//	printf("[_mt_fe_tn_set_reg]  Reg[0x05]= 0x%02x \n",val);
	
}

int main(void)
{
	int message;
	S8			snr, strength;
	U8			switch_port_no, horizontal_polarization, connect_time;
	U8			direction, rotate_mode, lock_timeout_cnt;
	S32			ret_time;
	MT_FE_STATE	state = MT_FE_STATE_IDLE;
	U32			cnt;
	U32			freq_MHz;
	U32			sym_rate_KSs;
	U32			total_packags, err_packags;
	BOOL		lnb_on_off, on_off_22k, envelop_mode;
	MT_FE_LNB_VOLTAGE	choice_13v_18v;
	MT_FE_LOCK_STATE lock_state;
	MT_FE_CHAN_INFO_DVBS2 chan_info;
	MT_FE_TYPE  dvbs_type;
	MT_FE_DiSEqC_MSG msg;
	MT_FE_DiSEqC_TONE_BURST mode;
	MT_FE_BAND_TYPE			band;
	MT_FE_LNB_LOCAL_OSC 	local_osc;
	MT_FE_BS_TP_INFO		bs;
	U32						begin_freqMHz, end_freqMHz;

	MT_FE_RET	ret = MtFeErr_Ok;


	// init I2C
	I2C_Init(); 
	
     // i2c test
//    DS3000_I2C_Test	();
    
	/*alloc the memeory to store the scanned and locked TP info*/
	MT_FE_TP_INFO blindscaninfo[1000] = {0};

	void (*callback)(void);

	/*initialize the parameter*/
	bs.p_tp_info	 = blindscaninfo;
	bs.tp_max_num	 = 1000;


	connect_time = 0;

    printf("tuner init \n");
	MT_FE_RET res = mt_fe_dmd_ds3k_init();
	printf(" DS3000_Init = %d \n", res);

    printf("tuner set freq \n");
    DS3000_Tunerset(1150, 28125);
    
    _mt_sleep(200); 
    printf("tuner lock check \n");
    
    while (DS3000_LockCheck()== 0)
    {
        printf("tuner is loocking... \n");
    }
    
    int i = DS3000_SignalQuality();   
    printf("DS3000_SignalQuality  = %d \n",i);
    
    int j = DS3000_SignalLevel();   
    printf("DS3000_SignalLevel  = %d \n",j);
    
    
    mt_fe_dmd_ds3k_get_channel_info(&chan_info); 
    printf("channel_info  type= %d  \n",chan_info.type);   
    printf("channel_info  mod_mode= %d  \n",chan_info.mod_mode);  
    printf("channel_info  roll_off= %d  \n",chan_info.roll_off);   
    printf("channel_info  code_rate= %d  \n",chan_info.code_rate); 
    printf("channel_info  is_pilot_on= %d  \n",chan_info.is_pilot_on);   
    printf("channel_info  is_spectrum_inv= %d  \n",chan_info.is_spectrum_inv);   
  
    return 0;
} 
  
  
  