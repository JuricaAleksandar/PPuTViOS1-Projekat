/*------------------------------------------------------------------------------

 <dev:header>
    Copyright(c) 2009 Sony Corporation.

    $Revision$
    $Author$

</dev:header>

------------------------------------------------------------------------------*/
/**
 @file    sony_dvb_dbg.c

      Implementation for an example MOPLL tuner.
*/
/*----------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
 Includes
------------------------------------------------------------------------------*/

#include <stdio.h>
#include <sony_dvb.h>
#include <sony_dvb_demod_monitor.h>
#include <sony_dvb_demod_monitorT.h>
#include <sony_dvb_demod_monitorT2.h>
#include <sony_dvb_demod_monitorC.h>
#include "sony_dvb_dbg.h"

/*------------------------------------------------------------------------------
 Statics
------------------------------------------------------------------------------*/

static const char* gBool[] = 
    { "False", "True" } ;
static const char* gUnknown = "Unknown" ;
static const char* gSystem[] = 
    { "Unknown", "DVB-C", "DVB-T", "DVB-T2" } ;
static const char* gSpectrumSense[] = 
    { "Normal", "Inverted" } ;
static const char* gDrvState[] = 
    { "Unknown", "Shutdown", "Sleep", "Active", "Invalid" } ;
static const char* gDrvLockResult[] = 
    { "Not Detected", "Locked", "Unlocked" } ;

static FILE *fp_debug_log;
static const char* gCXD2820ES11 = "CXD2820 ES1.1" ;
static const char* gCXD2820ES10 = "CXD2820 ES1.0" ;
static const char* gCXD2817 = "CXD2817" ;
static const char* gCXD2820TS = "CXD2820 TS" ;

static const char* gDivider = "-------------------------------------------------" ;
static const char* gCatChannelInformation = " Channel Information ";
static const char* gCatChannelParameters = " Channel Parameters ";
static const char* gCatDriverInformation= " Driver Information ";

/*------------------------------------------------------------------------------
 DVB-T2
 ------------------------------------------------------------------------------*/
static const char* gDvbT2Gi[] = 
    { "1/32", "1/16", "1/8", "1/4", "1/128", "19/128", "19/256" } ;
static const char* gDvbT2Mode[] = 
    { "2K", "8K", "4K", "1K", "16K", "32K" } ;
static const char* gDvbT2Papr[] = 
    { "None", "ACE", "TR", "TR-ACE" } ;
static const char* gDvbT2L1PostConstell[] = 
    { "BPSK", "QPSK", "16-QAM", "64-QAM" } ;
static const char* gDvbT2L1PostCodeRate[] = 
    { "1/2" } ;
static const char* gDvbT2L1PostFEC[] = 
    { "16K LDPC" } ;
static const char* gDvbT2PlpCodeRate[] = 
    { "1/2", "3/5", "2/3", "3/4", "4/5", "5/6" } ;
static const char* gDvbT2PlpConstell[] = 
    { "QPSK", "16-QAM", "64-QAM", "256-QAM" } ;
static const char* gDvbT2PlpType[] = 
    { "Common", "Data 1", "Data 2" } ;
static const char* gDvbT2PlpFEC[] = 
    { "16K LDPC", "64K LDPC" } ;

/*------------------------------------------------------------------------------
 DVB-T
------------------------------------------------------------------------------*/
static const char* gDvbTConstell[] = 
    { "QPSK", "16-QAM", "64-QAM" };
static const char* gDvbTHier[] = 
    { "Non", "A1", "A2", "A4" };
static const char* gDvbTCodeRate[] = 
    { "1/2", "2/3", "3/4", "5/6", "7/8" } ;
static const char* gDvbTGi[] = 
    { "1/32", "1/16", "1/8", "1/4" } ;
static const char* gDvbTMode[] = 
    { "2K", "8K" } ;

/*------------------------------------------------------------------------------
 DVB-C
------------------------------------------------------------------------------*/
static const char* gDvbCConstell[] = 
    { "16-QAM", "32-QAM", "64-QAM", "128-QAM", "256-QAM" };

/*------------------------------------------------------------------------------
 Functions
------------------------------------------------------------------------------*/

static void PrintUint (const char* name, const uint32_t val)
{
          //printf ("%-14lu = %s\r\n", val, name);
 	  fprintf(fp_debug_log,"%s = %lu\n",name,val);	  	     
}
static void PrintInt (const char* name, const int32_t val)
{
    	 //printf ("%-14ld = %s\r\n", val, name);
         fprintf(fp_debug_log,"%s = %ld\n",name,val);	 
    
}
static void PrintString (const char* name, const char* val)
{
    	  //printf ("%-14s = %s\r\n", val, name);
          fprintf(fp_debug_log,"%s = %s\n",name,val);	 	  
    
}
static void PrintStringSimple (const char* st)
{
    	  //printf ("%s\r\n", st);
          fprintf(fp_debug_log,"%s\n",st);	  
	  
    
}

static const char* FormatChipId (sony_dvb_demod_chip_id_t chipId)
{
    switch (chipId)
    {
    case SONY_DVB_CXD2817:
        return gCXD2817 ;
    case SONY_DVB_CXD2820_TS:
        return gCXD2820TS ;
    case SONY_DVB_CXD2820_ES1_0:
        return gCXD2820ES10 ;
    case SONY_DVB_CXD2820_ES1_1:
        return gCXD2820ES11 ;

    case SONY_DVB_CXD2820_IP:
    default:
        return gUnknown ;
    }
}

static void MonitorCommon (sony_dvb_demod_t* pDemod)
{
    sony_dvb_result_t result = SONY_DVB_OK ;

    PrintStringSimple (gDivider);
    PrintStringSimple (gDivider);
    PrintStringSimple (gCatChannelInformation);
    PrintStringSimple (gDivider);
    PrintStringSimple (gDivider);

    {
        int32_t offset = 0 ;
        result = dvb_demod_monitor_CarrierOffset (pDemod, &offset);
        if (result == SONY_DVB_OK)
            PrintInt ("Offset(kHz)", offset);
    }

    {
        uint32_t ifagc = 0 ;
        result = dvb_demod_monitor_IFAGCOut (pDemod, &ifagc);
        if (result == SONY_DVB_OK)
            PrintUint ("IFAGC", ifagc);
    }

    {
        uint32_t rfagc = 0 ;
        result = dvb_demod_monitor_RFAGCOut (pDemod, &rfagc);
        if (result == SONY_DVB_OK)
            PrintUint ("RFAGC", rfagc);
    }

    {
        sony_dvb_demod_lock_result_t lock ;
        result = dvb_demod_monitor_TSLock (pDemod, &lock);
        if (result == SONY_DVB_OK)
            PrintString ("TS Lock", gDrvLockResult[(uint8_t)lock]);
    }
}

static void MonitorDemodDriver (sony_dvb_demod_t* pDemod)
{
    PrintStringSimple (gDivider);
    PrintStringSimple (gDivider);
    PrintStringSimple (gCatDriverInformation);
    PrintStringSimple (gDivider);
    PrintStringSimple (gDivider);

    PrintString ("State", gDrvState[pDemod->state]);
    PrintString ("System", gSystem[pDemod->system]);
    PrintUint ("Bandwidth (MHz)", pDemod->bandWidth);
    PrintUint ("DVB-T I2C", pDemod->i2cAddressT);
    PrintUint ("DVB-T2 I2C", pDemod->i2cAddressT2);
    PrintUint ("DVB-C I2C", pDemod->i2cAddressC);
    PrintString ("Ascot", gBool[pDemod->is_ascot != 0 ? 1 : 0]);
    PrintString ("RF Level", gBool[pDemod->enable_rflvmon != 0 ? 1 : 0]);
    PrintString ("DVB-T Scan Mode", gBool[pDemod->dvbt_scannmode != 0 ? 1 : 0]);
    PrintString ("Chip", FormatChipId (pDemod->chipId));
    PrintString ("Spectrum Sense", gSpectrumSense [pDemod->confSense]);
    PrintUint ("IO Hi-Z", pDemod->ioHiZ);
}
 
void sony_dvb_dbg_monitor (sony_dvb_demod_t* pDemod)
{
    if (!pDemod) {
        PrintStringSimple ("Invalid param.");
        return ;
    }

	fp_debug_log = fopen("debug_log.txt","w");     /* open file pointer */

    switch (pDemod->system)
    {
        case SONY_DVB_SYSTEM_DVBT:
            sony_dvb_dbg_monitorT (pDemod);
            break ;
        case SONY_DVB_SYSTEM_DVBT2:
            sony_dvb_dbg_monitorT2 (pDemod);
            break ;

        case SONY_DVB_SYSTEM_DVBC:
            sony_dvb_dbg_monitorC (pDemod);
            break ;

        case SONY_DVB_SYSTEM_UNKNOWN:
        default: 
            PrintStringSimple ("Unknown system.");
            break ;
    }
	fclose(fp_debug_log);	
}

void sony_dvb_dbg_monitorT (sony_dvb_demod_t* pDemod)
{
    sony_dvb_result_t result = SONY_DVB_OK ;


	
    if (!pDemod) {
        PrintStringSimple ("Invalid param.");
        return ;
    }
    PrintStringSimple ("DVBT debug message");
    MonitorCommon (pDemod);

    /* Additional channel information. */
    {
        uint8_t syncState = 0 ;
        uint8_t tsLock = 0 ;
        result = dvb_demod_monitorT_SyncStat (pDemod, &syncState, &tsLock);
        if (result == SONY_DVB_OK)
        	{
            PrintUint ("Demod State", syncState);
        	}

    }
    
    {
        int32_t mer = 0 ;
        result = dvb_demod_monitorT_MER (pDemod, &mer);
        if (result == SONY_DVB_OK)
            PrintInt ("MER (dBx1e3)", mer);
    }

    {
        int32_t snr = 0 ;
        result = dvb_demod_monitorT_SNR (pDemod, &snr);
        if (result == SONY_DVB_OK)
            PrintInt ("SNR (dBx1e3)", snr);
    }

    {
        uint32_t ber = 0 ;
        result = dvb_demod_monitorT_PreViterbiBER (pDemod, &ber);
        if (result == SONY_DVB_OK) 
            PrintUint ("Pre-Viterbi BER (1e7)", ber);
    }
    {
        uint32_t ber = 0;
        result = dvb_demod_monitorT_PreRSBER (pDemod, &ber);
        if (result == SONY_DVB_OK)
            PrintUint ("Pre-RS BER (1e7)", ber);
    }

    {
        uint32_t errors = 0 ;
        result = dvb_demod_monitorT_RSError (pDemod, &errors);
        if (result == SONY_DVB_OK) 
            PrintUint("RS Errors", errors);
    }
    
    {
        int32_t ppm = 0 ;
        result = dvb_demod_monitorT_SamplingOffset (pDemod, &ppm);
        if (result == SONY_DVB_OK)
            PrintInt ("PPM (ppmx1e2)", ppm);
    }

    /* Channel parameters. */
    PrintStringSimple (gDivider);
    PrintStringSimple (gDivider);
    PrintStringSimple (gCatChannelParameters);
    PrintStringSimple (gDivider);
    PrintStringSimple (gDivider);
    {
        sony_dvb_demod_tpsinfo_t tps ;

        PrintStringSimple (gDivider);
        PrintStringSimple ("TPS");
        PrintStringSimple (gDivider);

        result = dvb_demod_monitorT_TPSInfo (pDemod, &tps);
        if (result == SONY_DVB_OK) {
            PrintString ("Mode", gDvbTMode[tps.mode]);
            PrintString ("GI", gDvbTGi[tps.guard]);
            PrintString ("Constellation", gDvbTConstell[tps.constellation]);
            PrintString ("Hier.", gDvbTHier[tps.hierarchy]);
            PrintString ("HP code rate", gDvbTCodeRate[tps.rateHP]);
            PrintString ("LP code rate", gDvbTCodeRate[tps.rateLP]);
            PrintUint("Cell ID", tps.cellID);
        }
        PrintStringSimple (gDivider);
    }

    {
        sony_dvb_demod_dvbt_mode_t mode ;
        sony_dvb_demod_dvbt_guard_t guard ;
        result = dvb_demod_monitorT_ModeGuard (pDemod, &mode, &guard);
        if (result == SONY_DVB_OK) {
            PrintString("Det. Mode", gDvbTMode[(uint8_t)mode]);
            PrintString("Det. Guard", gDvbTGi[(uint8_t)guard]);
        }
    }
    
    {
        sony_dvb_demod_spectrum_sense_t sense ;
        result = dvb_demod_monitorT_SpectrumSense (pDemod, &sense);
        if (result == SONY_DVB_OK)
            PrintString("Spectrum Sense", gSpectrumSense[(uint8_t)sense]);
    }

    MonitorDemodDriver (pDemod);


}

static void MonitorT2Plp (sony_dvb_dvbt2_plp_t* plp)
{
    PrintUint("Id", plp->id);
    PrintString("Type", gDvbT2PlpType[plp->type]);
    PrintUint("Group Id", plp->groupId);
    PrintString("Constell", gDvbT2PlpConstell[plp->constell]);
    PrintString("Code Rate", gDvbT2PlpCodeRate[plp->plpCr]);
    PrintString("Rotated", gBool[plp->rot != 0 ? 1 : 0]);
    PrintString("FEC", gDvbT2PlpFEC[plp->fec]);
    PrintUint("Num Blocks", plp->numBlocksMax);
    PrintUint("Frame Interval", plp->frmInt);
    PrintUint("Time Interleaver Length", plp->tilLen);
    PrintUint("Time Interleaver Type", plp->tilType);
    PrintString("In-band Signalling", gBool[plp->inBandFlag != 0 ? 1: 0]);
}

void sony_dvb_dbg_monitorT2 (sony_dvb_demod_t* pDemod)
{
    sony_dvb_result_t result = SONY_DVB_OK ;
    uint8_t numPlps = 0 ;

    if (!pDemod) {
        PrintStringSimple ("Invalid param.");
        return ;
    }
    PrintStringSimple ("DVBT2 debug message");
    MonitorCommon (pDemod);

    /* Additional channel condition. */
    {
        uint8_t syncState = 0 ;
        uint8_t tsLock = 0 ;
        result = dvb_demod_monitorT2_SyncStat (pDemod, &syncState, &tsLock);
        if (result == SONY_DVB_OK)
            PrintUint ("Demod State", syncState);

    }

    {
        int32_t mer = 0 ;
        result = dvb_demod_monitorT2_MER (pDemod, &mer);
        if (result == SONY_DVB_OK)
            PrintInt("MER (dBx1e3)", mer);
    }

    {
        int32_t snr = 0 ;
        result = dvb_demod_monitorT2_SNR (pDemod, &snr);
        if (result == SONY_DVB_OK)
            PrintInt("SNR (dBx1e3)", snr);
    }

    {
        uint32_t ber = 0 ;
        result = dvb_demod_monitorT2_PreLDPCBER (pDemod, &ber);
        if (result == SONY_DVB_OK)
            PrintUint("Pre LDPC BER (1e7)", ber);
    }

    {
        uint32_t fer = 0 ;
        result = dvb_demod_monitorT2_PostBCHFER (pDemod, &fer);
        if (result == SONY_DVB_OK)
            PrintUint("Post BCH FER (1e6)", fer);
    }
    {
        uint32_t ber = 0 ;
        result = dvb_demod_monitorT2_PreBCHBER (pDemod, &ber);
        if (result == SONY_DVB_OK)
            PrintUint ("Pre BCH BER (1e9)", ber);
    }
    {
        int32_t ppm = 0 ;
        result = dvb_demod_monitorT2_SamplingOffset (pDemod, &ppm);
        if (result == SONY_DVB_OK)
            PrintInt ("PPM (ppmx1e2)", ppm);
    }

    /* Channel parameters. */
    PrintStringSimple (gDivider);
    PrintStringSimple (gDivider);
    PrintStringSimple (gCatChannelParameters);
    PrintStringSimple (gDivider);
    PrintStringSimple (gDivider);

    {
        sony_dvb_dvbt2_l1pre_t l1Pre ;

        PrintStringSimple (gDivider);
        PrintStringSimple ("L1 pre");
        PrintStringSimple (gDivider);

        result = dvb_demod_monitorT2_L1Pre (pDemod, &l1Pre);
        if (result == SONY_DVB_OK) {
            PrintString ("BW Extended", gBool[l1Pre.bwExt != 0 ? 1 : 0]);
            PrintUint ("S1", (uint8_t) l1Pre.s1);
            PrintUint ("S2", (uint8_t) l1Pre.s2);
            PrintString ("Mixed", gBool[l1Pre.mixed != 0 ? 1 : 0]);
            PrintString ("FFT Mode", gDvbT2Mode[l1Pre.fftMode]);
            PrintUint("L1 Repeat", l1Pre.l1Rep);
            PrintString ("GI", gDvbT2Gi[l1Pre.gi]);
            PrintString ("PAPR", gDvbT2Papr[l1Pre.papr]);
            PrintString ("L1-post Constell", gDvbT2L1PostConstell[l1Pre.mod]);
            PrintString ("L1-post Code Rate", gDvbT2L1PostCodeRate[l1Pre.cr]);
            PrintString ("L1-post FEC", gDvbT2L1PostFEC[l1Pre.fec]);
            PrintUint ("L1-post Size", l1Pre.l1PostSize);
            PrintUint ("PP", (uint8_t)l1Pre.pp + 1);
            PrintUint ("TX ID Avail.", l1Pre.txIdAvailability);
            PrintUint ("Cell ID", l1Pre.cellId);
            PrintUint ("Network ID", l1Pre.networkId);
            PrintUint ("System ID", l1Pre.systemId);
            PrintUint ("Number Frames", l1Pre.numFrames);
            PrintUint ("Number Symbols", l1Pre.numSymbols);
            PrintUint ("Regen", l1Pre.regen);
            PrintUint ("Post Ext", l1Pre.postExt);
            PrintUint ("Number RF Freqs", l1Pre.numRfFreqs);
            PrintUint ("RF Index", l1Pre.rfIdx);
        }
        PrintStringSimple (gDivider);
    }

    {
        uint8_t plps[256] ;
        result = dvb_demod_monitorT2_DataPLPs (pDemod, plps, &numPlps);
        if (result == SONY_DVB_OK)
            PrintUint ("# PLPs", numPlps);
        else 
            numPlps = 0 ;
    }

    {
        sony_dvb_dvbt2_plp_t plp ;

        PrintStringSimple (gDivider);
        PrintStringSimple ("Active Data PLP");
        PrintStringSimple (gDivider);

        result = dvb_demod_monitorT2_ActivePLP (pDemod, SONY_DVB_DVBT2_PLP_DATA, &plp);
        if (result == SONY_DVB_OK)
            MonitorT2Plp (&plp);

        PrintStringSimple (gDivider);
    }

    {
        sony_dvb_dvbt2_plp_t plp ;

        if (numPlps > 1) {
          
            /* Print the common PLP if available. */
            PrintStringSimple (gDivider);
            PrintStringSimple ("Active Common PLP");
            PrintStringSimple (gDivider);
            result = dvb_demod_monitorT2_ActivePLP (pDemod, SONY_DVB_DVBT2_PLP_COMMON, &plp);
            if (result == SONY_DVB_OK)
                MonitorT2Plp (&plp);

            PrintStringSimple (gDivider);
        }
    }

    {
        sony_dvb_dvbt2_l1post_t l1Post ;

        PrintStringSimple (gDivider);
        PrintStringSimple ("L1 Post");
        PrintStringSimple (gDivider);

        result = dvb_demod_monitorT2_L1Post(pDemod, &l1Post);
        if (result == SONY_DVB_OK) {
            /* L1-post */
            PrintUint ("Sub Slices Per Frame", l1Post.subSlicesPerFrame);
            PrintUint ("#PLPs", l1Post.numPLPs);
            PrintUint ("#Aux", l1Post.numAux);
            PrintUint ("Aux Config", l1Post.auxConfigRFU);
            PrintUint ("RF Index", l1Post.rfIdx);
            PrintUint ("Frequency (Hz)", l1Post.freq);
            PrintUint ("FEF Type", l1Post.fefType);
            PrintUint ("FEF Length", l1Post.fefLength);
            PrintUint ("FEF Interval", l1Post.fefInterval);
        }
        PrintStringSimple (gDivider);
    }

    {
        sony_dvb_demod_spectrum_sense_t sense ;
        result = dvb_demod_monitorT2_SpectrumSense (pDemod, &sense);
        if (result == SONY_DVB_OK)
            PrintString("Spectrum Sense", gSpectrumSense[(uint8_t)sense]);
    }

    MonitorDemodDriver (pDemod);
}

void sony_dvb_dbg_monitorC (sony_dvb_demod_t* pDemod)
{
    sony_dvb_result_t result = SONY_DVB_OK ;

    if (!pDemod) {
        PrintStringSimple ("Invalid param.");
        return ;
    }

    PrintStringSimple ("DVBC debug message");
    MonitorCommon (pDemod);

    /* Additional channel information. */
    {
        uint8_t arState = 0 ;
        uint8_t tsLock = 0 ;
        result = dvb_demod_monitorC_SyncStat (pDemod, &arState, &tsLock);
        if (result == SONY_DVB_OK)
            PrintUint("Demod State", arState);
    }
    {
        int32_t snr = 0;
        result = dvb_demod_monitorC_SNR (pDemod, &snr);
        if (result == SONY_DVB_OK)
            PrintInt ("SNR (dBx1e3)", snr);
    }
    {
        uint32_t ber = 0 ;
        result = dvb_demod_monitorC_PreRSBER (pDemod, &ber);
        if (result == SONY_DVB_OK)
            PrintUint ("Pre RS BER (1e7)", ber);
    }
    {
        uint32_t errors = 0 ;
        result = dvb_demod_monitorC_RSError (pDemod, &errors);
        if (result == SONY_DVB_OK)
            PrintUint ("RS Errors", errors);
    }

    /* Channel parameters. */
    PrintStringSimple (gDivider);
    PrintStringSimple (gDivider);
    PrintStringSimple (gCatChannelParameters);
    PrintStringSimple (gDivider);
    PrintStringSimple (gDivider);

    {
        sony_dvb_demod_dvbc_constellation_t qam ;
        result = dvb_demod_monitorC_QAM (pDemod, &qam);
        if (result == SONY_DVB_OK)
            PrintString("Constell", gDvbCConstell[(uint8_t)qam]);
    }
    {
        uint32_t symbolRate = 0 ;
        result = dvb_demod_monitorC_SymbolRate (pDemod, &symbolRate);
        if (result == SONY_DVB_OK)
            PrintUint("Symbol Rate (kSps)", symbolRate); 
    }

    {
        sony_dvb_demod_spectrum_sense_t sense ;
        result = dvb_demod_monitorC_SpectrumSense (pDemod, &sense);
        if (result == SONY_DVB_OK)
            PrintString("Spectrum Sense", gSpectrumSense[(uint8_t)sense]);
    }

    MonitorDemodDriver (pDemod);
}
