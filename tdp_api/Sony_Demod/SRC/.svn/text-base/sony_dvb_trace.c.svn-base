
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "../include/sony_dvb.h"

#include "mmac/types.h"
#include "mmac/debug.h"
#include "mmac/api/iic.h"

#ifdef __cplusplus
}
#endif


/*------------------------------------------------------------------------------
  Sample implementation of trace log function
  These functions are declared in sony_dvb.h
------------------------------------------------------------------------------*/
static const char *g_callStack[32768];    /* To output function name in TRACE_RETURN log */
static int g_callStackTop = 0;

const char* dvb_trace_format_result (sony_dvb_result_t result)
{
    char* pErrorName = "UNKNOWN" ;
    switch (result) {
    case SONY_DVB_OK:
        pErrorName = "OK";
        break;
    case SONY_DVB_ERROR_TIMEOUT:
        pErrorName = "ERROR_TIMEOUT";
        break;
    case SONY_DVB_ERROR_UNLOCK:
        pErrorName = "ERROR_UNLOCK";
        break;
    case SONY_DVB_ERROR_CANCEL:
        pErrorName = "ERROR_CANCEL";
        break;
    case SONY_DVB_ERROR_ARG:
        pErrorName = "ERROR_ARG";
        break;
    case SONY_DVB_ERROR_I2C:
        pErrorName = "ERROR_I2C";
        break;
    case SONY_DVB_ERROR_SW_STATE:
        pErrorName = "ERROR_SW_STATE";
        break;
    case SONY_DVB_ERROR_HW_STATE:
        pErrorName = "ERROR_HW_STATE";
        break;
    case SONY_DVB_ERROR_RANGE:
        pErrorName = "ERROR_RANGE";
        break;
    case SONY_DVB_ERROR_NOSUPPORT:
        pErrorName = "ERROR_NOSUPPORT";
        break;
    case SONY_DVB_ERROR_OTHER:
        pErrorName = "ERROR_OTHER";
        break;
    default:
        pErrorName = "ERROR_UNKNOWN";
        break;
    }
    return pErrorName ;
}

void dvb_trace_hexdump (const uint8_t *addr, int len)
{
    while (len)
    {
        SONY_DVB_TRACE2("%2.2x ",*addr++);
        --len;
    }
    SONY_DVB_TRACE1("\r\n");
}

void dvb_trace_log_enter (const char *funcname, const char *filename, unsigned int linenum)
{
    const char *pFileNameTop = NULL;
    
    //MMAC_DEBUG_Print (MMAC_DEBUG_MSG,"dvb_trace_log_enter : funcname %x filename %x\r\n",funcname,filename);
    if (!funcname || !filename)
        return;

    /*
     * <PORTING>
     * NOTE: strrchr(filename, '\\')+1 can get file name from full path string.
     * If in linux system, '/' will be used instead of '\\'.
     */
    pFileNameTop = strrchr (filename, '\\');
    if (pFileNameTop) {
        pFileNameTop += 1;
    }
    else {
        pFileNameTop = filename;    /* Cannot find '\\' */
    }

    MMAC_DEBUG_Print (MMAC_DEBUG_MSG,"TRACE_ENTER: %s (%s, line %d)\r\n", funcname, pFileNameTop, linenum);

    if (g_callStackTop >= (int)(sizeof (g_callStack) / sizeof (g_callStack[0]) - 1)) {
        MMAC_DEBUG_Print (MMAC_DEBUG_MSG,"ERROR: Call Stack Overflow...\r\n");
        return;
    }
    g_callStack[g_callStackTop] = funcname;
    g_callStackTop++;
}

void dvb_trace_log_return (sony_dvb_result_t result, const char *filename, unsigned int linenum)
{
    const char *pErrorName = NULL;
    const char *pFuncName = NULL;
    const char *pFileNameTop = NULL;

    if (g_callStackTop > 0) {
        g_callStackTop--;
        pFuncName = g_callStack[g_callStackTop];
    }
    else {
        MMAC_DEBUG_Print (MMAC_DEBUG_MSG,"ERROR: Call Stack Underflow...\r\n");
        pFuncName = "Unknown Func";
    }

    switch (result) {
    case SONY_DVB_OK:
    case SONY_DVB_ERROR_TIMEOUT:
    case SONY_DVB_ERROR_UNLOCK:
    case SONY_DVB_ERROR_CANCEL:
        break;
    default:
        break;
    }

    pErrorName = dvb_trace_format_result (result);

    pFileNameTop = strrchr (filename, '\\');
    if (pFileNameTop) {
        pFileNameTop += 1;
    }
    else {
        pFileNameTop = filename;    /* Cannot find '\\' */
    }

    MMAC_DEBUG_Print (MMAC_DEBUG_MSG,"TRACE_RETURN: %s (%s) (%s, line %d)\r\n\n", pErrorName, pFuncName, pFileNameTop, linenum);
}

