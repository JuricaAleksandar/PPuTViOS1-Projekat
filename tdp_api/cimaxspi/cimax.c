/**
 * @mainpage CIMaX+ SPI Driver Copyright Notice
  @section copyright_notice Copyright notice � 2005 RT-RK LLC All rights reserved
  @n This software and related documentation (the "Software") are intellectual
  property owned by RT-RK and are copyright of RT-RK, unless specifically
  noted otherwise.
  @n Any use of the Software is permitted only pursuant to the terms of the license
  agreement, if any, which accompanies, is included with or applicable to the
  Software ("License Agreement") or upon express written consent of RT-RK. Any
  copying, reproduction or redistribution of the Software in whole or in part by
  any means not in accordance with the License Agreement or as agreed in writing
  by RT-RK is expressly prohibited.
  @n THE SOFTWARE IS WARRANTED, IF AT ALL, ONLY ACCORDING TO THE TERMS OF THE LICENSE
  AGREEMENT. EXCEPT AS WARRANTED IN THE LICENSE AGREEMENT THE SOFTWARE IS
  DELIVERED "AS IS" AND RT-RK HEREBY DISCLAIMS ALL WARRANTIES AND CONDITIONS
  WITH REGARD TO THE SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES AND CONDITIONS OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIT ENJOYMENT, TITLE AND
  NON-INFRINGEMENT OF ANY THIRD PARTY INTELLECTUAL PROPERTY OR OTHER RIGHTS WHICH
  MAY RESULT FROM THE USE OR THE INABILITY TO USE THE SOFTWARE.
  IN NO EVENT SHALL RT-RK BE LIABLE FOR INDIRECT, INCIDENTAL, CONSEQUENTIAL,
  PUNITIVE, SPECIAL OR OTHER DAMAGES WHATSOEVER INCLUDING WITHOUT LIMITATION,
  DAMAGES FOR LOSS OF BUSINESS PROFITS, BUSINESS INTERRUPTION, LOSS OF BUSINESS
  INFORMATION, AND THE LIKE, ARISING OUT OF OR RELATING TO THE USE OF OR THE
  INABILITY TO USE THE SOFTWARE, EVEN IF RT-RK HAS BEEN ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGES, EXCEPT PERSONAL INJURY OR DEATH RESULTING FROM
  RT-RK'S NEGLIGENCE.
 */
/**
 * @file     cimax.c
 *
 * @brief    CIMaX+ SPI driver API.
 *
 * @note     Copyright (C) 2010 RT-RK LLC 
 *           All Rights Reserved
 *
 * @author   RT-RK CIMaX+ team
 *
 * @date     14/02/2011
 */

/******************************************************************************
 * Includes
 ******************************************************************************/
#include <stdio.h>
#include <stdarg.h>
#include "cimax.h"
#include "cimax_spi_pio.h"
#include <fcntl.h>
#include <sys/ioctl.h>

/**************************************************************************//**
* @defgroup defines Defines
* @{
******************************************************************************/
/** CIMaX+ read thread stack size. */
#define CIMAX_READ_THREAD_STACK_SIZE      8192
/** CIMaX+ check INT period defined in milliseconds. */
#define CIMAX_CHECK_INT_PERIOD_MS         10
/** CIMaX+ register response timeout defined in milliseconds. */
#define CIMAX_REGISTER_RESPONSE_TIMEOUT_MS 400
/** CIMaX+ CAM response timeout defined in milliseconds. */
#define CIMAX_CAM_RESPONSE_TIMEOUT_MS      5000

/** CIMaX+ firmware start address. */
#define FW_START_ADDRESS                  0x8000
/** CIMaX+ firmware end address. */
#define FW_END_ADDRESS                    0xCFF9
/** CIMaX+ firmware vector table start address. */
#define FW_VECTOR_TABLE_START_ADDRESS     0xFFFA
/** CIMaX+ firmware vector table size. */
#define FW_VECTOR_TABLE_SIZE              6
/** CIMaX+ firmware packet size. */
#define FW_PACKET_SIZE                    128
/** CIMaX+ SPI firmware file name. */
#define FW_FILE_NAME                      "/bin/cimax_spidvb_211.bin"

#define DEVICE_GET_ALL_SIZE				256

#define CIMAX_DEVICE_RX_BUFFER_SIZE    4100

#define CIMAX_CI_HEADER_SIZE        4

/* Command tag */
#define CIMAX_COMMAND_OFFSET        0
#define CIMAX_SEL_MASK           0x80
#define CIMAX_TYP_MASK           0x40
#define CIMAX_CMD_MASK           0x3F
#define CIMAX_CMD_INIT           0x00
#define CIMAX_CMD_WRITE_REG      0x7F
#define CIMAX_CMD_READ_REG       0xFF
#define CIMAX_CMD_CAMRESET       0x01
#define CIMAX_CMD_GETCIS         0x02
#define CIMAX_CMD_WRITECOR       0x03
#define CIMAX_CMD_NEGOTIATE      0x04
#define CIMAX_CMD_WRITELPDU      0x05
#define CIMAX_CMD_READLPDU       0x06
#define CIMAX_CMD_WRITEEXT       0x07
#define CIMAX_CMD_READEXT        0x08
#define CIMAX_CMD_MCARD_WRITE    0x0a
#define CIMAX_CMD_GETREGSTATUS   0x0e

/* Status field */
#define CIMAX_STATUS_OFFSET         0
#define CIMAX_STATUS_BOOTOK      0x02
#define CIMAX_INITOK             0x0b
#define CIMAX_READ_REGOK         0x0c
#define CIMAX_WRITE_REGOK        0x0d
#define CIMAX_CAMRESETOK         0x00
#define CIMAX_CISOK              0x01
#define CIMAX_WRITECOROK         0x02
#define CIMAX_NEGOTIATEOK        0x03
#define CIMAX_WRITELPDUOK        0x04
#define CIMAX_READLPDUOK         0x06
#define CIMAX_WRITEEXTOK         0x07
#define CIMAX_READEXTOK          0x08
#define CIMAX_NO_CAM             0x09
#define CIMAX_NOK                0x0a
#define CIMAX_MCARD_WRITEOK      0x0f
#define CIMAX_WRITELPDUBUSY      0x14
#define CIMAX_PENDING_CMD        0x16
#define CIMAX_GETREGSTATUSOK     0x17


/* Counter value */
#define CIMAX_COUNTER_OFFSET        1

/* Length field */
#define CIMAX_LENGTH_MSB_OFFSET     2
#define CIMAX_LENGTH_LSB_OFFSET     3

/* Others */
#define CIMAX_DATA_OFFSET           4
#define CIMAX_CAMDET                0x05
#define CIMAX_DATAREADY             0x0e



/*=============================================================================
 * CIMaX+ commands
=============================================================================*/
/** CIMaX+ register header size. */
#define CIMAX_REGISTER_HEADER_SIZE        4
/** CIMaX+ register command position. */
#define CIMAX_REGISTER_CMD_POS            0
/** CIMaX+ register address most significant byte position. */
#define CIMAX_REGISTER_REG_ADDR_MSB_POS   1
/** CIMaX+ register address least significant byte position. */
#define CIMAX_REGISTER_REG_ADDR_LSB_POS   2
/** CIMaX+ register number of registers position. */
#define CIMAX_REGISTER_NUM_OF_REGS_POS    3

/** CIMaX+ register maximum payload size. */
#define CIMAX_REGISTER_MAX_PAYLOAD_SIZE   255

/** CIMaX+ register init request. */
#define CIMAX_REGISTER_INIT_REQ           0x00
/** CIMaX+ register write request. */
#define CIMAX_REGISTER_WRITE_REQ          0x7F
/** CIMaX+ register read request. */
#define CIMAX_REGISTER_READ_REQ           0xFF

/** CIMaX+ register init response OK. */
#define CIMAX_REGISTER_INIT_RESP_OK       0x4B
/** CIMaX+ register init response not OK. */
#define CIMAX_REGISTER_INIT_RESP_NOK      0xCB
/** CIMaX+ register write response OK. */
#define CIMAX_REGISTER_WRITE_RESP_OK      0x4D
/** CIMaX+ register write response not OK. */
#define CIMAX_REGISTER_WRITE_RESP_NOK     0xCD
/** CIMaX+ register read response OK. */
#define CIMAX_REGISTER_READ_RESP_OK       0x4C
/** CIMaX+ register read response not OK. */
#define CIMAX_REGISTER_READ_RESP_NOK      0xCC
/** CIMaX+* register command parse error response. */
#define CIMAX_REGISTER_CMD_PARSE_ERROR    0x51
/** CIMaX+* registr boot command not OK. */
#define CIMAX_REGISTER_BOOT_CMD_NOK       0x70

/*=============================================================================
 * CAM commands
=============================================================================*/
/** CIMaX+ CAM packet counter value. */
#define CIMAX_CAM_PACKET_COUNTER_VALUE    0x01

/** CIMaX+ CAM inserted value. */
#define CIMAX_CAM_INSERTED                0x01
/** CIMaX+ CAM extracted value. */
#define CIMAX_CAM_EXTRACTED               0x00

/** CIMaX+ CAM slot mask. */
#define CIMAX_CAM_SLOT_MASK               0x80
/** CIMaX+ CAM slot bit position. */
#define CIMAX_CAM_SLOT_BIT_POSITION       7
/** CIMaX+ CAM type command mask. */
#define CIMAX_CAM_TYP_CMD_MASK            0x7F         

/** CIMaX+ CAM header size. */
#define CIMAX_CAM_HEADER_SIZE             4
/** CIMaX+ CAM command or status position. */
#define CIMAX_CAM_COMMAND_OR_STATUS_POS   0
/** CIMaX+ CAM packet counter position. */
#define CIMAX_CAM_PACKET_COUNTER_POS      1
/** CIMaX+ CAM data length most significant byte position. */
#define CIMAX_CAM_DATA_LEN_MSB_POS        2
/** CIMaX+ CAM data length least significant byte position. */
#define CIMAX_CAM_DATA_LEN_LSB_POS        3
/** CIMaX+ CAM first byte of the data position. */
#define CIMAX_CAM_FIRST_DATA_BYTE_POS     4
/** CIMaX+ CAM second byte of the data position. */
#define CIMAX_CAM_SECOND_DATA_BYTE_POS    5

/** CIMaX+ CAM maximum payload size. */
#define CIMAX_CAM_MAX_PAYLOAD_SIZE        4096
/** CIMaX+ CAM maximum card information structure size. */
#define CIMAX_CAM_CIS_MAX_SIZE            512   /* To be checked  */
/** CIMaX+ CAM COR writing command payload size. */
#define CIMAX_CAM_COR_PAYLOAD_SIZE        5
/** CIMaX+ CAM COR additional payload size. */
#define CIMAX_CAM_COR_ADD_PAYLOAD_SIZE    2
/** CIMaX+ CAM buffer negotiation command payload size. */
#define CIMAX_CAM_NEGOTIATE_PAYLOAD_SIZE  2
/** CIMaX+ CAM detection response payload size. */
#define CIMAX_CAM_CAMDET_PAYLOAD_SIZE     1

/** CIMaX+ CAM COR address most significant byte position. */
#define CIMAX_CAM_COR_ADDR_MSB_POS        0
/** CIMaX+ CAM COR address least significant byte position. */
#define CIMAX_CAM_COR_ADDR_LSB_POS        1
/** CIMaX+ CAM COR coomand value position. */
#define CIMAX_CAM_COR_VALUE_POS           2
/** CIMaX+ CAM COR command last byte most significant byte position. */
#define CIMAX_CAM_COR_LAST_MSB_POS        3
/** CIMaX+ CAM COR command last byte least significant byte position. */
#define CIMAX_CAM_COR_LAST_LSB_POS        4

/** CIMaX+ CAM buffer negotiation command most significant byte position. */
#define CIMAX_CAM_NEGOTIATE_SIZE_MSB_POS  0
/** CIMaX+ CAM buffer negotiation command least significant byte position. */
#define CIMAX_CAM_NEGOTIATE_SIZE_LSB_POS  1

/** CIMaX+ CAMReset command. */
#define CIMAX_CAM_RESET_CMD               0x01
/** CIMaX+ GetCIS command. */
#define CIMAX_CAM_GET_CIS_CMD             0x02
/** CIMaX+ CAM WriteCOR command. */
#define CIMAX_CAM_WRITE_COR_CMD           0x03
/** CIMaX+ CAM Negotiate command. */
#define CIMAX_CAM_NEGOCIATE_CMD           0x04
/** CIMaX+ CAM WriteLPDU command. */
#define CIMAX_CAM_WRITE_LPDU_CMD          0x05
/** CIMaX+ CAM ReadLPDU command. */
#define CIMAX_CAM_READ_LPDU_CMD           0x06
/** CIMaX+ CAM GetEvt command. */
#define CIMAX_CAM_GET_EVT_CMD             0x0D
/** CIMaX+ CAM REGSTATUS command. */
#define CIMAX_CAM_REG_STATUS              0x0E

/** CIMaX+ CAM CAMReset A response. */
#define CIMAX_CAM_A_RESET_OK              0x40
/** CIMaX+ CAM CAMReset B response. */
#define CIMAX_CAM_B_RESET_OK              0xC0
/** CIMaX+ CAM GetCIS A response. */
#define CIMAX_CAM_A_GET_CIS_OK            0x41
/** CIMaX+ CAM GetCIS B response. */
#define CIMAX_CAM_B_GET_CIS_OK            0xC1
/** CIMaX+ CAM WriteCOR A response. */
#define CIMAX_CAM_A_WRITE_COR_OK          0x42
/** CIMaX+ CAM WriteCOR B response. */
#define CIMAX_CAM_B_WRITE_COR_OK          0xC2
/** CIMaX+ CAM Negotiate A response. */
#define CIMAX_CAM_A_NEGOTIATE_OK          0x43
/** CIMaX+ CAM Negotiate B response. */
#define CIMAX_CAM_B_NEGOTIATE_OK          0xC3
/** CIMaX+ CAM WriteLPDU A response. */
#define CIMAX_CAM_A_WRITE_LPDU_OK         0x44
/** CIMaX+ CAM WriteLPDU B response. */
#define CIMAX_CAM_B_WRITE_LPDU_OK         0xC4
/** CIMaX+ CAM CamDet A response. */
#define CIMAX_CAM_A_DET_OK                0x45
/** CIMaX+ CAM CamDet B response. */
#define CIMAX_CAM_B_DET_OK                0xC5
/** CIMaX+ CAM ReadLPDU A response. */
#define CIMAX_CAM_A_READ_LPDU_OK          0x46
/** CIMaX+ CAM ReadLPDU B OK. */
#define CIMAX_CAM_B_READ_LPDU_OK          0xC6
/** CIMaX+ CAM CAMNoCAM A response. */
#define CIMAX_CAM_A_NO_CAM                0x49
/** CIMaX+ CAM CAMNoCAM B response. */
#define CIMAX_CAM_B_NO_CAM                0xC9
/** CIMaX+ CAM CAMError A response. */
#define CIMAX_CAM_A_CAM_ERROR             0x4A
/** CIMaX+ CAM CAMError B response. */
#define CIMAX_CAM_B_CAM_ERROR             0xCA
/** CIMaX+ CAM DataReady A response. */
#define CIMAX_CAM_A_DATA_READY            0x4E
/** CIMaX+ CAM DataReady B response. */
#define CIMAX_CAM_B_DATA_READY            0xCE
/** CIMaX+ CAM StatusNoEvt response. */
#define CIMAX_CAM_NO_EVT                  0x55
/** CIMaX+ CAM CAMWriteBusy A response. */
#define CIMAX_CAM_A_WRITE_BUSY            0x54
/** CIMaX+ CAM CAMWriteBusy B response. */
#define CIMAX_CAM_B_WRITE_BUSY            0xD4
/** CIMaX+ CAM CMD_PENDING A response. */
#define CIMAX_CAM_A_CMD_PENDING           0x56
/** CIMaX+ CAM CMD_PENDING B response. */
#define CIMAX_CAM_B_CMD_PENDING           0xD6
/** CIMaX+ CAM REGSTATUSOK A response. */
#define CIMAX_CAM_A_REG_STATUS_OK         0x57
/** CIMaX+ CAM REGSTATUSOK B response. */
#define CIMAX_CAM_B_REG_STATUS_OK         0xD7

/*=============================================================================
 * CAMError status
=============================================================================*/
/** CIMaX+ CAM Not Ready. */
#define CAMERROR_RESET                    0x0101
/** CIMaX+ CAM CIS buffer not allocate. */
#define CAMERROR_CIS_BUFFER               0x0201
/** CIMaX+ CAM CIS bad size (> 512 bytes). */
#define CAMERROR_CIS_BAD_SIZE             0x0202
/** CIMaX+ CAM CAM Not Activated. */
#define CAMERROR_CIS_CAM_NOT_ACTIVATED    0x0203
/** CIMaX+ CAM CAM not ready after software reset during write COR process. */
#define CAMERROR_COR_CAM_NOT_READY        0x0301
/** CIMaX+ CAM Checking of COR Value failed. */
#define CAMERROR_COR_VALUE_CHECKING       0x0302
/** CIMaX+ CAM CAM not responding after SR bit is raised during Buffer negociation phase. */
#define CAMERROR_NEGOTIATE_NOT_RESPONDING 0x0401
/** CIMaX+ CAM CAM buffer size length <> 2. */
#define CAMERROR_NEGOTIATE_SIZE_LENGTH    0x0402
/** CIMaX+ CAM CAM not ready to accept new buffer size negociate during Buffer negociation phase. */
#define CAMERROR_NEGOTIATE_NOT_READY      0x0403
/** CIMaX+ CAM LPDU not available. */
#define CAMERROR_LPDU_NOT_AVAILABLE       0x0601
/** @}*/

/*=============================================================================
 * PINMUX defines
=============================================================================*/

#define PINMUX_IOCTL_READ           0x5100
#define PINMUX_IOCTL_WRITE          0x5101
#define PINMUX_IOCTL_SETMOD         0x5102
#define PINMUX_IOCTL_GETMOD         0x5103
#define PINMUX_IOCTL_PRINTSTATUS    0x5104

#define SOC_PINMUX                  0
#define SM_PINMUX                   1

#define PINMUX_DEVICE "/dev/pinmux"

/*=============================================================================
 * GPIO defines
=============================================================================*/

#define GPIO_DEVICE "/dev/gpio"

#define GPIO_IOCTL_READ                 0x1111
#define GPIO_IOCTL_WRITE                0x1112
#define GPIO_IOCTL_SET                  0x1113
#define GPIO_IOCTL_GET                  0x1114
#define GPIO_IOCTL_INIT_IRQ             0x2222
#define GPIO_IOCTL_EXIT_IRQ             0x2223
#define GPIO_IOCTL_ENABLE_IRQ   0x2224
#define GPIO_IOCTL_DISABLE_IRQ  0x2225
#define GPIO_IOCTL_CLEAR_IRQ    0x2226
#define GPIO_IOCTL_READ_IRQ             0x2227

#define SM_GPIO_IOCTL_READ                      0x3333
#define SM_GPIO_IOCTL_WRITE                     0x3334
#define SM_GPIO_IOCTL_SET                       0x3335
#define SM_GPIO_IOCTL_GET                       0x3336
#define SM_GPIO_IOCTL_INIT_IRQ          0x4444
#define SM_GPIO_IOCTL_EXIT_IRQ          0x4445
#define SM_GPIO_IOCTL_ENABLE_IRQ        0x4446
#define SM_GPIO_IOCTL_DISABLE_IRQ       0x4447
#define SM_GPIO_IOCTL_CLEAR_IRQ         0x4448
#define SM_GPIO_IOCTL_READ_IRQ          0x4449

#define GPIO_MODE_DATA_IN               1
#define GPIO_MODE_DATA_OUT              2
#define GPIO_MODE_IRQ_LOWLEVEL  3
#define GPIO_MODE_IRQ_HIGHLEVEL 4
#define GPIO_MODE_IRQ_RISEEDGE  5
#define GPIO_MODE_IRQ_FALLEDGE  6


/******************************************************************************
 *  Typedefs
 ******************************************************************************/

/**
 * @enum regOperation_t
 * @brief An enumeration to represent supported registers operations
 */
typedef enum {
    /** Read register. */
    REG_OP_READ,
    /** Write register. */
    REG_OP_WRITE,
    /** Read register until some bits are set. */
    REG_OP_WAIT_TO_BE_SET,
    /** Read register until some bits are cleared. */
    REG_OP_WAIT_TO_BE_CLEARED,
    /** Read register until it's value is not equal to defined. */
    REG_OP_WAIT_EQUAL,
    /** Perform logical AND over register. */
    REG_OP_LOGICAL_AND,
    /** Perform logical OR over register. */
    REG_OP_LOGICAL_OR,
    /** Wait timeout in miliseconds. */
    REG_OP_WAIT
} regOperation_t;

/**
 * @struct regSettings_t
 * @brief The structure to represent register settings.
 */
typedef struct {
    /** CIMaX+ register address. */
    uint16 reg;
    /** CIMaX+ register value. */
    uint16 val;
    /** CIMaX+ register operation. */
    regOperation_t op;
} regSettings_t;

/**
 * @struct cimaxContext_t
 * @brief The structure to represent driver context.
 */
typedef struct {
    /** Active state of CIMaX+ INT pin. */
    uint8 intActiveState;
    /** Read thread handle. */
    osThread_t* hReadThread;
    /** Thread stop indicator. */
    BOOL bStopReadThread;
    /** Protects accessing CIMaX+ INT pin from different threads. */
    osMutex_t* hIntMutex;
    /** Holds information about CAM presence. */
    BOOL camInserted[CIMAX_CAM_MAX_SLOT_NUM];
    /** Holds information about CAM data ready. */
    BOOL camDataReady[CIMAX_CAM_MAX_SLOT_NUM];
    /** CIMaX+ response buffer. */
    uint8 responseBuff[CIMAX_CAM_HEADER_SIZE +
            CIMAX_CAM_MAX_PAYLOAD_SIZE];
    /** Holds information about interrupt/polling mode. */
    BOOL irqEnabled[CIMAX_CAM_MAX_SLOT_NUM];
} cimaxContext_t;

/**
 * @brief
 *   Device module structure.
 */
typedef struct {
    void *pstDevice;
    /**< pointer to opened device */
    uint8 u8DeviceIndex;
    /**< index of opened device in device list */
    uint8 pu8AllDevice[DEVICE_GET_ALL_SIZE];
    /**< device list */
    uint8 u8NumDevice;
    /**< number of devices */
} CimaxUsbDeviceModule;


/**************************************************************************//**
* @defgroup globals Global variables
******************************************************************************/

/* GPIO device file descriptor */
int sFd;

static FILE *spFile;

static CimaxUsbDeviceModule CIMAX_DeviceModule;
static CaModuleDetectionCb CIMAX_DetectionCb;

static DataToReadCb CIMAX_DataToReadCb;


static unsigned char CIMAX_RxBuffer[CIMAX_DEVICE_RX_BUFFER_SIZE];
static BOOL CIMAX_CAM_Inserted[CIMAX_CAM_MAX_SLOT_NUM] = {FALSE, FALSE};
static BOOL CIMAX_CAM_TSPassThrough[CIMAX_CAM_MAX_SLOT_NUM] = {FALSE, FALSE};

static volatile int doPolling = 0;


/** 
 * @ingroup globals
 * An instance of SPI register settings structure. 
 */

/*
 * Cimax configuration:
 * Parallel input 0 - CA Module B - Serial output 1
 */
static regSettings_t spiRegSettings[] = {

    {IN_SEL, 0x00, REG_OP_WRITE}, // Close TS input.
    {OUT_SEL, 0x00, REG_OP_WRITE}, // Close TS output.
    {FIFO_CTRL, 0x0f, REG_OP_WRITE}, // Reset TS FIFO.
    {SYNC_RTV_CTRL, 0x0f, REG_OP_WRITE},

    {0x0000, 200, REG_OP_WAIT}, /** Wait 200 miliseconds. */

    {0x0049, 0x15, REG_OP_WRITE}, //CkMan_Select Select 108MHz Serial 0 and Serial 1 clock.
    {0x0048, 0x6F, REG_OP_WRITE}, //CkMan_Config Config Serial 0 and Serial 1 clock enable.

    {0x0224, 0x00, REG_OP_WRITE}, //S2P_CH1_CTRL S2P CH1 (enable & rising edge & reset.)

    {0x0228, 0x22, REG_OP_WRITE}, //S2P_CH1_CTRL S2P CH1 (enable & rising edge & reset.)
    {0x0227, 0x00, REG_OP_WRITE}, //S2P_CH1_CTRL S2P CH1 (enable & rising edge & reset.)
    {0x022A, 0x18, REG_OP_WRITE}, //P2S_CH1_CTRL P2S CH1 (MOSTRT aligned on one byte & internal clock & enable & reset).

    /** CAM power. */
    {GPIO0_DATA_OUT, 0x00, REG_OP_WRITE},
    {CFG_2, 0x00, REG_OP_WRITE}, /** Unlock CFG. */
    {CFG_1, 0x00, REG_OP_WRITE}, /** 1) DVB/CI/CI+/SCARD 2slot. */
    {GPIO0_DFT, 0x00, REG_OP_WRITE}, /** 2) Set the Default "power off" state such as VCC_MODA=VCC_MODB=VPPx_MODA=VPPx_MODB='Z'. */
    {GPIO0_MASK_DATA, 0x07, REG_OP_WRITE}, /** 3) Set GPIO3 as external power switch driver. */
    {GPIO0_DATA_OUT, 0x01, REG_OP_WRITE}, /** 4) Set "power on" state (VCC=VPP1=VPP2= 5V). */
    {CFG_2, 0x01, REG_OP_WRITE}, /** 5) Lock config. */
    {GPIO0_DIR, 0x07, REG_OP_WRITE}, /** 6) Write in the GPIO0_DIR_REG: defines the GPIOs, which are used to drive the external power switch, in output mode. */
    {CFG_1, 0x20, REG_OP_WAIT_TO_BE_SET}, /** 7) Check VCCENable. */
    {0x0000, 400, REG_OP_WAIT}, /** Wait 200 miliseconds. */
    {CFG_1, 0x08, REG_OP_LOGICAL_OR}, /** 8) Set & wait for PcmciaOutputEnable. */
    {CFG_1, 0x08, REG_OP_WAIT_TO_BE_SET},
    {0x0000, 400, REG_OP_WAIT}, /** Wait 200 miliseconds. */

    /** Set router CAM. */
    {ROUTER_CAM_CH, 0x08, REG_OP_WRITE}, // source for ch1 = mod B
    {ROUTER_CAM_MOD, 0x10, REG_OP_WRITE}, // source for mod B = ch0

    {0x0000, 200, REG_OP_WAIT}, /** Wait 200 miliseconds. */
    {0x022F, 0x18, REG_OP_WRITE}, //ROUTER_CAM_CH
    {0x0230, 0x00, REG_OP_WRITE}, //ROUTER_CAM_MOD

    {0x022E, 0x04, REG_OP_WRITE}, //OUT_SEL CH0 => Serial 1 Out (P2S0), CH1 => Serial 2 Out (P2S1)

    {0x022D, 0x01, REG_OP_WRITE}, //IN_SEL Parallel In => CH0, Serial 2 In => CH1.
};



/* spi reset universal configuration */
static regSettings_t spiRegSettingsReset[] = {

    //{0x0000, 200, REG_OP_WAIT}, /** Wait 200 miliseconds. */

    {0x0222, 0x40, REG_OP_WRITE}, // USB2TS0_RDL
    {0x0223, 0x40, REG_OP_WRITE}, // USB2TS1_RDL    
    {0x0221, 0x0F, REG_OP_WRITE}, // USB2TS_CTRL   
    {0x022D, 0x22, REG_OP_WRITE}, // IN_SEL  

    //{0x0000, 200, REG_OP_WAIT}, /** Wait 200 miliseconds. */

    {0x0231, 0x0F, REG_OP_WRITE}, // FIFO_CTRL 

    //{0x0000, 200, REG_OP_WAIT}, /** Wait 200 miliseconds. */
    {0x022A, 0x01, REG_OP_WRITE}, // P2S_CH1_CTRL
    {0x022A, 0x18, REG_OP_WRITE}, // P2S_CH1_CTRL


    {0x02CB, 0x7F, REG_OP_WRITE}, // RESET_CLK_SWITCH_0
    {0x02CC, 0x0F, REG_OP_WRITE}, // RESET_CLK_SWITCH_1 
    {0x0221, 0x05, REG_OP_WRITE}, // USB2TS_CTRL  
    {0x022D, 0x01, REG_OP_WRITE} // IN_SEL     

    //{0x0000, 200, REG_OP_WAIT} /** Wait 200 miliseconds. */
};


/** 
 * @ingroup globals
 * An instance of CIMaX+ context structure.
 */
static cimaxContext_t cimaxContext_g;

/******************************************************************************
 *    Static functions
 ******************************************************************************/
static int32 downloadFW(void);
static int32 downloadCfg(void);
static int32 initFW(void);
static uint16 computeBistRom(uint8 *tab, int32 size, uint16 fwSign);
static uint8 checkBistRom(int32 startAddr, int32 endAddr, int32 signature);
static int32 asyncEventsThreadFunction(void *params);
static int32 accessCAM(uint8 slotId, uint8 cmd, uint8 *bufferOut, uint16 numOfBytes);
static BOOL responseCamNoCam(uint8 slotId);
static BOOL responseCamError(uint8 slotId);
static BOOL responseCamWriteBusy(uint8 slotId);
static BOOL responseCamCmdPending(uint8 slotId);
static int32 getResponse(uint32 timeout);
#ifdef _DEBUG
static void printData(char* functionName, uint8 *buffer, int32 numOfBytes);
#endif 

/**************************************************************************//**
* @defgroup CIMaX_Globals CIMaX+ Global Functions Definitions
******************************************************************************/

BOOL CimaxUsbCmdWrite(void* pDevice, uint8* pu8TxData, uint32 u32TxDataLength, uint8* pu8RxData, uint32* pu32RxDataLength) {

    printf("CimaxUsbCmdWrite function not implemented\n");

    return 0;
}

/*
 * Pinmux functions
 */



/**
 * @ingroup     CIMaX_Globals
 *
 * @brief       CIMaX+ SPI driver initialization:
 *              - Initialization of SPI and PIO interface, 
 *              - Initialization of read thread and synchronisation resources,
 *              - FW download,
 *              - Configuration download,
 *              - Start FW execution.
 *
 * @param       none  
 *
 * @return      CIMAX_NO_ERROR if successfull, CIMAX_ERROR if failure.
 */
int32 CIMAX_Init(CaModuleDetectionCb pstDetectCb, DataToReadCb pstDataToReadCb) {
    spiInitParams_t spiInitParams;
    uint32 i = 0;
    uint8 bootStatus = 0;

    /* Initialize SPI interface used for communication with CIMaX+. */
    spiInitParams.clock = CIMAX_SPI_CLOCK_1MHZ;
    spiInitParams.mode = 0;
    if (CIMAX_SPI_Init(&spiInitParams) != CIMAX_NO_ERROR) {
        printf("%s: SPI init failed.\n", __FUNCTION__);
        fflush(stdout);
        return CIMAX_ERROR;
    }

    /* Initialize PIO interface used for communication with CIMaX+. */
    if (CIMAX_PIO_Init() != CIMAX_NO_ERROR) {
        printf("%s: PIO init failed.\n", __FUNCTION__);
        CIMAX_Term();
        return CIMAX_ERROR;
    }

    /* Set default CIMaX+ INT pin active state. */
    cimaxContext_g.intActiveState = 0;
    for (i = 0; i < CIMAX_CAM_MAX_SLOT_NUM; i++) {
        cimaxContext_g.camInserted[i] = FALSE;
        cimaxContext_g.camDataReady[i] = FALSE;
        cimaxContext_g.irqEnabled[i] = TRUE;
    }

    /* create resources for synchronization. */
    cimaxContext_g.hIntMutex = OS_MutexCreate();
    if (cimaxContext_g.hIntMutex == NULL) {
        printf("%s: hIntMutex init failed.\n", __FUNCTION__);
        CIMAX_Term();
        return CIMAX_ERROR;
    }

    /* Read Boot status. In case FW already loaded no need to load again*/
    CIMAX_ReadRegister(BOOT_Status, &bootStatus, 1);

    printf("CIMAX BOOT STATUS CHECK : %s\n", (bootStatus == CIMAX_STATUS_BOOTOK) ? "PASS" : "FAIL(--> FW download)");

    if(bootStatus != CIMAX_STATUS_BOOTOK)
    {

       printf("Set pinmux for TSI and SPI\n");
       /* TSI pinmux */
       set_pinmux(0,6, 1);
       set_pinmux(0,7, 1);

       // set_pinmux for ICS
       set_pinmux(0,2, 1);

       /* GPIO 28 enable (Cimax reset pin) */
       set_pinmux(0,16, 0);

       // cimax is in not reset mode
       //set_gpio(28, 1);

       printf("Pinmux is set\n");

       printf("Reset Cimax\n");
       set_gpio(28, 0);

       printf("Reset finished\n");
       usleep(100000);
       set_gpio(28, 1);
       usleep(100000);

       int fwTries = 5;

       do {
           fwTries--;
           if (downloadFW() == CIMAX_NO_ERROR) {
               break;
           }

           printf("Reset Cimax\n");
           set_gpio(28, 0);

           printf("Reset finished\n");
           usleep(100000);
           set_gpio(28, 1);
           usleep(100000);

       } while (fwTries);

       if (fwTries == 0) {
           CIMAX_Term();
           return CIMAX_ERROR;
       }
    }

    int cfgTries = 5;

    printf("Download Config\n");

    do {
        cfgTries--;
        if (downloadCfg() == CIMAX_NO_ERROR) {
            break;
        }
        printf("Retry cfg download\n");
    } while (cfgTries);

    if (cfgTries == 0) {
        printf("Download Config FAILED\n");
        CIMAX_Term();
        return CIMAX_ERROR;
    }


    /* Create thread for handling async events. */
    cimaxContext_g.hReadThread = OS_ThreadCreate(&asyncEventsThreadFunction, NULL, CIMAX_READ_THREAD_STACK_SIZE);
    if (cimaxContext_g.hReadThread == NULL) {
        printf("%s: hReadThread init failed.\n", __FUNCTION__);
        CIMAX_Term();
        return CIMAX_ERROR;
    }

    CIMAX_DetectionCb = pstDetectCb;
    CIMAX_DataToReadCb = pstDataToReadCb;

    printf("%s: OK.\n", __FUNCTION__);

    return CIMAX_NO_ERROR;
}

/**
 * @ingroup     CIMaX_Globals
 *
 * @brief       Terminate resources used by CIMaX+ SPI driver.
 *
 * @param       none  
 *
 * @return      CIMAX_NO_ERROR if successfull, CIMAX_ERROR if failure.
 */
int32 CIMAX_Term(void) {
    int32 status = CIMAX_NO_ERROR;

    /* Stop read thread. */
    cimaxContext_g.bStopReadThread = TRUE;
    OS_Sleep(100);
    if (cimaxContext_g.hReadThread != NULL) {
        OS_ThreadDelete(cimaxContext_g.hReadThread);
    }

    /* Delete resources for synchronization. */
    if (cimaxContext_g.hIntMutex != NULL) {
        OS_MutexDelete(cimaxContext_g.hIntMutex);
    }

    /* Terminate SPI and PIO interfaces. */
    CIMAX_SPI_Term();
    CIMAX_PIO_Term();

    memset(&cimaxContext_g, 0, sizeof (cimaxContext_t));

    printf("%s OK.\n", __FUNCTION__);

    return status;
}

/******************************************************************************
 * @brief
 *	 CAM Detect event
 *
 * @param  slotId
 *	   CAM slot id.
 *	   event EVT_INSERTED/EVT_REMOVED
 * @param  result
 *	   Result, true if CAM is detected.
 *
 * @return
 *	   CIMAX_NO_ERROR if successfull, error code if failure.
 ******************************************************************************/
int32 CIMAX_CAMDetect(uint8 slotId, EVT eEvt) {
    if (slotId >= CIMAX_CAM_MAX_SLOT_NUM) {
        printf("CIMAX_CAMDetected: Invalid slot id (%d)\n", slotId);
        return (CIMAX_INVALID_SLOT_ID_ERROR);
    }

    if (eEvt == EVT_INSERTED) {
        CIMAX_CAM_Inserted[slotId] = TRUE;
    } else if (eEvt == EVT_REMOVED) {
        CIMAX_CAM_Inserted[slotId] = FALSE;
    }

    return (CIMAX_NO_ERROR);
}

/**
 * @ingroup     CIMaX_Globals
 *
 * @brief       CIMaX+ Write register.
 *
 * @param       addr        Register address.
 * @param       buffer      Buffer which holds data to be written.
 * @param       numOfBytes  Number of bytes to be written.
 *
 * @return      CIMAX_NO_ERROR if successfull, error code if failure.
 */
int32 CIMAX_WriteRegister(uint16 addr, uint8 *buffer, uint8 numOfBytes) {
    uint8 bufferOut[CIMAX_REGISTER_HEADER_SIZE + CIMAX_REGISTER_MAX_PAYLOAD_SIZE];
    int32 retVal = CIMAX_ERROR;

    /* Prepare command header. */
    bufferOut[CIMAX_REGISTER_CMD_POS] = CIMAX_REGISTER_WRITE_REQ;
    bufferOut[CIMAX_REGISTER_REG_ADDR_MSB_POS] = (addr >> 8) & 0xFF;
    bufferOut[CIMAX_REGISTER_REG_ADDR_LSB_POS] = addr & 0xFF;
    bufferOut[CIMAX_REGISTER_NUM_OF_REGS_POS] = numOfBytes;

    /* Prepare command payload. */
    memcpy(&bufferOut[CIMAX_REGISTER_HEADER_SIZE], buffer, numOfBytes);

    /* INT access protection. */
    OS_MutexLock(cimaxContext_g.hIntMutex);

    /* Send command to CIMaX+. */
    if (CIMAX_SPI_Write(bufferOut, numOfBytes + CIMAX_REGISTER_HEADER_SIZE) != CIMAX_NO_ERROR) {
        OS_MutexUnlock(cimaxContext_g.hIntMutex);
        return CIMAX_SPI_COMM_ERROR;
    }

    retVal = getResponse(CIMAX_REGISTER_RESPONSE_TIMEOUT_MS);
    if (retVal < 0) {
        OS_MutexUnlock(cimaxContext_g.hIntMutex);
        return retVal;
    }

    if ((cimaxContext_g.responseBuff[CIMAX_REGISTER_CMD_POS] != CIMAX_REGISTER_WRITE_RESP_OK) ||
            (cimaxContext_g.responseBuff[CIMAX_REGISTER_REG_ADDR_MSB_POS] != ((addr >> 8) & 0xff)) ||
            (cimaxContext_g.responseBuff[CIMAX_REGISTER_REG_ADDR_LSB_POS] != (addr & 0xff)) ||
            (cimaxContext_g.responseBuff[CIMAX_REGISTER_NUM_OF_REGS_POS] != 0x00)) {
        OS_MutexUnlock(cimaxContext_g.hIntMutex);
        return CIMAX_WRITE_RESP_ERROR;
    }

    OS_MutexUnlock(cimaxContext_g.hIntMutex);

    return CIMAX_NO_ERROR;
}

/**
 * @ingroup     CIMaX_Globals
 *
 * @brief       CIMaX+ Read register.
 *
 * @param       addr        Register address.
 * @param       buffer      Buffer for read data.
 * @param       numOfBytes  Number of bytes to be read.
 *
 * @return      CIMAX_NO_ERROR if successfull, error code if failure.
 */
int32 CIMAX_ReadRegister(uint16 addr, uint8 *buffer, uint8 numOfBytes) {
    uint8 bufferOut[CIMAX_REGISTER_HEADER_SIZE];
    int32 retVal = CIMAX_ERROR;

    /* Send read data request (only header). */
    bufferOut[CIMAX_REGISTER_CMD_POS] = CIMAX_REGISTER_READ_REQ;
    bufferOut[CIMAX_REGISTER_REG_ADDR_MSB_POS] = (addr >> 8) & 0xFF;
    bufferOut[CIMAX_REGISTER_REG_ADDR_LSB_POS] = addr & 0xFF;
    bufferOut[CIMAX_REGISTER_NUM_OF_REGS_POS] = numOfBytes;

    /* INT access protection. */
    OS_MutexLock(cimaxContext_g.hIntMutex);

    /* Send command to CIMaX+. */
    if (CIMAX_SPI_Write(bufferOut, CIMAX_REGISTER_HEADER_SIZE) != CIMAX_NO_ERROR) {
        OS_MutexUnlock(cimaxContext_g.hIntMutex);
        return CIMAX_SPI_COMM_ERROR;
    }

    retVal = getResponse(CIMAX_REGISTER_RESPONSE_TIMEOUT_MS);
    if (retVal < 0) {
        OS_MutexUnlock(cimaxContext_g.hIntMutex);
        return retVal;
    }

    if ((cimaxContext_g.responseBuff[CIMAX_REGISTER_CMD_POS] != CIMAX_REGISTER_READ_RESP_OK) ||
            (cimaxContext_g.responseBuff[CIMAX_REGISTER_REG_ADDR_MSB_POS] != ((addr >> 8)&0xff)) ||
            (cimaxContext_g.responseBuff[CIMAX_REGISTER_REG_ADDR_LSB_POS] != (addr & 0xff)) ||
            (cimaxContext_g.responseBuff[CIMAX_REGISTER_NUM_OF_REGS_POS] != numOfBytes) ||
            (retVal != (numOfBytes + CIMAX_REGISTER_HEADER_SIZE))) {
        OS_MutexUnlock(cimaxContext_g.hIntMutex);
        return CIMAX_READ_RESP_ERROR;
    }

    /* Copy read data to input buffer. */
    memcpy(buffer, &cimaxContext_g.responseBuff[CIMAX_REGISTER_HEADER_SIZE], numOfBytes);

    OS_MutexUnlock(cimaxContext_g.hIntMutex);

    return CIMAX_NO_ERROR;
}

/**
 * @ingroup     CIMaX_Globals
 *
 * @brief       Checks if CAM is inserted.
 *
 * @param       slotId   CAM slot id.
 * @param       result   Result, true if CAM is detected.
 *
 * @return      CIMAX_NO_ERROR if successfull, error code if failure.
 */
int32 CIMAX_CAMDetected(uint8 slotId, int32 *result) {
    if (slotId >= CIMAX_CAM_MAX_SLOT_NUM) {
        return CIMAX_INVALID_SLOT_ID_ERROR;
    }

    /* Initialize result. */
    *result = 0;
    /* TODO mutex. */
    if (cimaxContext_g.camInserted[slotId]) {
        *result = 1;
    }

    return CIMAX_NO_ERROR;
}

/**
 * @ingroup     CIMaX_Globals
 *
 * @brief       Checks if data from CAM are ready.
 *
 * @param       slotId   CAM slot id.
 * @param       result   Result, true if data is available.
 *
 * @return      CIMAX_NO_ERROR if successfull, error code if failure.
 */
int32 CIMAX_DataAvailable(uint8 slotId, int32 *result) {
    uint8 bufferOut[CIMAX_CAM_HEADER_SIZE];
    int32 responseSize = 0;
    uint8 camStatus = CIMAX_CAM_A_REG_STATUS_OK;

    printf("CIMAX_DataAvailable is not implemented\n");

#if 0    

    if (slotId >= CIMAX_CAM_MAX_SLOT_NUM) {
        return CIMAX_INVALID_SLOT_ID_ERROR;
    }

    /* Initialize result. */
    *result = 0;

    if (cimaxContext_g.irqEnabled[slotId] == TRUE) {
        /* TODO mutex. */
        if (cimaxContext_g.camDataReady[slotId] == TRUE) {
            *result = 1;
        }
    } else {
        /* INT access protection. */
        OS_MutexLock(cimaxContext_g.hIntMutex);

        responseSize = accessCAM(slotId, CIMAX_CAM_REG_STATUS, bufferOut, 0);
        if (responseSize < 0) {
            OS_MutexUnlock(cimaxContext_g.hIntMutex);
            return responseSize;
        }

        /* Check if response is valid. */
        if (slotId != CIMAX_CAM_SLOT_A_ID) {
            camStatus = CIMAX_CAM_B_REG_STATUS_OK;
        }

        if ((cimaxContext_g.responseBuff[CIMAX_CAM_COMMAND_OR_STATUS_POS] != camStatus) ||
                (cimaxContext_g.responseBuff[CIMAX_CAM_PACKET_COUNTER_POS] != CIMAX_CAM_PACKET_COUNTER_VALUE) ||
                (cimaxContext_g.responseBuff[CIMAX_CAM_DATA_LEN_MSB_POS] != 0x00) ||
                (cimaxContext_g.responseBuff[CIMAX_CAM_DATA_LEN_LSB_POS] != 0x01)) {
            printf("%s: Invalid response received!\n", __FUNCTION__);
            OS_MutexUnlock(cimaxContext_g.hIntMutex);
            return CIMAX_CAM_RESPONSE_ERROR;
        }

        /* Extract DA bit (bit 7) from status register. */
        *result = (cimaxContext_g.responseBuff[CIMAX_CAM_FIRST_DATA_BYTE_POS] & 0x80) >> 7;

        OS_MutexUnlock(cimaxContext_g.hIntMutex);

    }
#endif
    return CIMAX_NO_ERROR;
}

/**
 * @ingroup     CIMaX_Globals
 *
 * @brief       Reset the module.
 *
 * @param       slotId   CAM slot id.
 *
 * @return      CIMAX_NO_ERROR if successfull, error code if failure.
 */
int32 CIMAX_CAMReset(uint8 slotId) {
    uint8 bufferOut[CIMAX_CAM_HEADER_SIZE];
    int32 responseSize = 0;
    uint8 camStatus = CIMAX_CAM_A_RESET_OK;

    /* Reset data ready local variable. */
    cimaxContext_g.camDataReady[slotId] = FALSE;
    cimaxContext_g.irqEnabled[slotId] = TRUE;

    if (slotId >= CIMAX_CAM_MAX_SLOT_NUM) {
        return CIMAX_INVALID_SLOT_ID_ERROR;
    }

    /* INT access protection. */
    OS_MutexLock(cimaxContext_g.hIntMutex);

    responseSize = accessCAM(slotId, CIMAX_CAM_RESET_CMD, bufferOut, 0);
    if (responseSize < 0) {
        OS_MutexUnlock(cimaxContext_g.hIntMutex);
        return responseSize;
    }

    /* Check if response is valid. */
    if (slotId != CIMAX_CAM_SLOT_A_ID) {
        camStatus = CIMAX_CAM_B_RESET_OK;
    }

    if ((cimaxContext_g.responseBuff[CIMAX_CAM_COMMAND_OR_STATUS_POS] != camStatus) ||
            (cimaxContext_g.responseBuff[CIMAX_CAM_PACKET_COUNTER_POS] != CIMAX_CAM_PACKET_COUNTER_VALUE) ||
            (cimaxContext_g.responseBuff[CIMAX_CAM_DATA_LEN_MSB_POS] != 0x00) ||
            (cimaxContext_g.responseBuff[CIMAX_CAM_DATA_LEN_LSB_POS] != 0x00)) {
        printf("%s: Invalid response received!\n", __FUNCTION__);
        OS_MutexUnlock(cimaxContext_g.hIntMutex);
        return CIMAX_CAM_RESPONSE_ERROR;
    }

    OS_MutexUnlock(cimaxContext_g.hIntMutex);

    return CIMAX_NO_ERROR;
}

/**
 * @ingroup     CIMaX_Globals
 *
 * @brief       Gets CIS of CAM module.
 *
 * @param       slotId   CAM slot id.
 * @param       cisBuff  Buffer for storing read CIS.
 *
 * @return      CIMAX_NO_ERROR if successfull, error code if failure.
 */
int32 CIMAX_GetCIS(uint8 slotId, uint8 *cisBuff) {
    uint8 bufferOut[CIMAX_CAM_HEADER_SIZE];
    int32 responseSize = 0;
    uint16 dataSize = 0;
    uint8 camStatus = CIMAX_CAM_A_GET_CIS_OK;

    if (slotId >= CIMAX_CAM_MAX_SLOT_NUM) {
        return CIMAX_INVALID_SLOT_ID_ERROR;
    }

    /* INT access protection. */
    OS_MutexLock(cimaxContext_g.hIntMutex);

    responseSize = accessCAM(slotId, CIMAX_CAM_GET_CIS_CMD, bufferOut, 0);
    if (responseSize < 0) {
        OS_MutexUnlock(cimaxContext_g.hIntMutex);
        return responseSize;
    }

    /* Check if response is valid. */
    if (slotId != CIMAX_CAM_SLOT_A_ID) {
        camStatus = CIMAX_CAM_B_GET_CIS_OK;
    }

    if ((cimaxContext_g.responseBuff[CIMAX_CAM_COMMAND_OR_STATUS_POS] != camStatus) ||
            (cimaxContext_g.responseBuff[CIMAX_CAM_PACKET_COUNTER_POS] != CIMAX_CAM_PACKET_COUNTER_VALUE)) {
        printf("%s: Invalid response received!\n", __FUNCTION__);
        OS_MutexUnlock(cimaxContext_g.hIntMutex);
        return CIMAX_CAM_RESPONSE_ERROR;
    }

    dataSize = cimaxContext_g.responseBuff[CIMAX_CAM_DATA_LEN_MSB_POS];
    dataSize = (dataSize << 8);
    dataSize = dataSize | cimaxContext_g.responseBuff[CIMAX_CAM_DATA_LEN_LSB_POS];

    memcpy(cisBuff, &cimaxContext_g.responseBuff[CIMAX_CAM_HEADER_SIZE], dataSize);

    OS_MutexUnlock(cimaxContext_g.hIntMutex);

    return dataSize;
}

/**
 * @ingroup     CIMaX_Globals
 *
 * @brief       Write COR value to CAM module.
 *
 * @param       slotId   CAM slot id.
 * @param       addr     COR register address.
 * @param       buffer  Buffer which holds COR data.
 * @param (opt) irqEn    interrupt/polling mode
 * @param (opt) timeOut  WriteCOR pending timeout value
 * @param       NULL     mandatory
 *
 * @return      CIMAX_NO_ERROR if successfull, error code if failure.
 */
int32 CIMAX_WriteCor(uint8 slotId, uint16 addr, const uint8 *buffer, ...) {
    uint8 bufferOut[CIMAX_CAM_HEADER_SIZE + CIMAX_CAM_COR_PAYLOAD_SIZE + CIMAX_CAM_COR_ADD_PAYLOAD_SIZE];
    int32 responseSize = 0;
    uint8 camStatus = CIMAX_CAM_A_WRITE_COR_OK;
    va_list arguments;
    uint8 inputArgument, payloadSize;
    BOOL irqEnable = FALSE;

    /* Initial payload size. */
    payloadSize = CIMAX_CAM_COR_PAYLOAD_SIZE;

    //   va_start(arguments, buffer);
    //
    //   /* Parsing arguments. */
    //   while ((inputArgument = va_arg(arguments, uint8)) != NULL)
    //   {
    //      if (inputArgument == 0x41)
    //         irqEnable = TRUE;
    //      bufferOut[payloadSize] = inputArgument;
    //      payloadSize += 1;
    //   }
    //
    //   va_end(arguments);

    if (slotId >= CIMAX_CAM_MAX_SLOT_NUM) {
        return CIMAX_INVALID_SLOT_ID_ERROR;
    }

    /* INT access protection. */
    OS_MutexLock(cimaxContext_g.hIntMutex);

    bufferOut[CIMAX_CAM_COR_ADDR_MSB_POS] = addr >> 8;
    bufferOut[CIMAX_CAM_COR_ADDR_LSB_POS] = addr & 0x00FF;
    bufferOut[CIMAX_CAM_COR_VALUE_POS] = buffer[0];
    bufferOut[CIMAX_CAM_COR_LAST_MSB_POS] = 0x00;
    bufferOut[CIMAX_CAM_COR_LAST_LSB_POS] = 0x00;
    bufferOut[CIMAX_CAM_COR_LAST_LSB_POS + 1] = 0x40;

    responseSize = accessCAM(slotId, CIMAX_CAM_WRITE_COR_CMD, bufferOut, payloadSize + 1);
    if (responseSize < 0) {
        OS_MutexUnlock(cimaxContext_g.hIntMutex);
        return responseSize;
    }

    /* Check if response is valid. */
    if (slotId != CIMAX_CAM_SLOT_A_ID) {
        camStatus = CIMAX_CAM_B_WRITE_COR_OK;
    }

    if ((cimaxContext_g.responseBuff[CIMAX_CAM_COMMAND_OR_STATUS_POS] == camStatus) &&
            (cimaxContext_g.responseBuff[CIMAX_CAM_PACKET_COUNTER_POS] == CIMAX_CAM_PACKET_COUNTER_VALUE) &&
            (cimaxContext_g.responseBuff[CIMAX_CAM_DATA_LEN_MSB_POS] == 0x00) &&
            (cimaxContext_g.responseBuff[CIMAX_CAM_DATA_LEN_LSB_POS] == 0x00)) {
        OS_MutexUnlock(cimaxContext_g.hIntMutex);
        cimaxContext_g.irqEnabled[slotId] = irqEnable;
        return CIMAX_NO_ERROR;
    }

    printf("%s: Invalid response received!\n", __FUNCTION__);

    OS_MutexUnlock(cimaxContext_g.hIntMutex);

    return CIMAX_CAM_RESPONSE_ERROR;
}

/******************************************************************************
 * @brief
 *   Read the content of Status Register.
 *
 * @param  slotId
 *     CAM slot id.
 * @param  mask
 *     Mask to find bit value in register.
 * @param  result
 *     Result, true if bit is set.
 *
 * @return
 *     CIMAX_NO_ERROR if successfull, error code if failure.
 ******************************************************************************/
int32 CIMAX_ReadStatus(uint8 slotId, uint8 mask, int32 *result) {

    unsigned long u32Size = 0;
    unsigned char pu8Cmd[CIMAX_CI_HEADER_SIZE];
    int32 res = 0;

    uint8 bufferOut[CIMAX_CAM_HEADER_SIZE];
    int32 responseSize = 0;
    uint8 camStatus = CIMAX_CAM_A_REG_STATUS_OK;
    int32 dataSize = 0;


    if (slotId >= CIMAX_CAM_MAX_SLOT_NUM) {
        printf("CIMAX_ReadStatus: Invalid slot id (%d)\n", slotId);
        return (CIMAX_INVALID_SLOT_ID_ERROR);
    }

    if (result == NULL) {
        printf("CIMAX_ReadStatus: Invalid parameter\n");
        return (CIMAX_ERROR);
    }

    pu8Cmd[CIMAX_COMMAND_OFFSET] = CIMAX_CMD_GETREGSTATUS;
    if (slotId == 1)
        pu8Cmd[CIMAX_COMMAND_OFFSET] |= CIMAX_SEL_MASK;

    pu8Cmd[CIMAX_COUNTER_OFFSET] = 1;
    pu8Cmd[CIMAX_LENGTH_MSB_OFFSET] = 0;
    pu8Cmd[CIMAX_LENGTH_LSB_OFFSET] = 0;
    u32Size = CIMAX_DEVICE_RX_BUFFER_SIZE;


    /* INT access protection. */
    OS_MutexLock(cimaxContext_g.hIntMutex);

    responseSize = accessCAM(slotId, CIMAX_CAM_REG_STATUS, bufferOut, 0);
    if (responseSize < 0) {
        OS_MutexUnlock(cimaxContext_g.hIntMutex);
        return responseSize;
    }

    /* Check if response is valid. */
    if (slotId != CIMAX_CAM_SLOT_A_ID) {
        camStatus = CIMAX_CAM_B_REG_STATUS_OK;
    }

    /*
        if ((cimaxContext_g.responseBuff[CIMAX_CAM_COMMAND_OR_STATUS_POS] == 0xc4)) {
            printf("response 0xc4 in read status skip msg\n");
            printf("mask = 0x%x\n", mask);
            if (mask == 0x80) {
     *result = 0;
            } else if (mask == 0x40) {
     *result = 1;
            } else {
     *result = 0;
            }

            OS_MutexUnlock(cimaxContext_g.hIntMutex);
            return 0;
        }
     */

    if ((cimaxContext_g.responseBuff[CIMAX_CAM_COMMAND_OR_STATUS_POS] != camStatus) ||
            (cimaxContext_g.responseBuff[CIMAX_CAM_PACKET_COUNTER_POS] != CIMAX_CAM_PACKET_COUNTER_VALUE)) {
        printf("%s: Invalid response received!\n", __FUNCTION__);

        printf("cimaxContext_g.responseBuff[0] = 0x%x\n", cimaxContext_g.responseBuff[0]);
        printf("cimaxContext_g.responseBuff[1] = 0x%x\n", cimaxContext_g.responseBuff[1]);
        printf("cimaxContext_g.responseBuff[2] = 0x%x\n", cimaxContext_g.responseBuff[2]);
        printf("cimaxContext_g.responseBuff[3] = 0x%x\n", cimaxContext_g.responseBuff[3]);

        OS_MutexUnlock(cimaxContext_g.hIntMutex);
        return CIMAX_CAM_RESPONSE_ERROR;
    }


    dataSize = cimaxContext_g.responseBuff[CIMAX_CAM_DATA_LEN_MSB_POS];
    dataSize = (dataSize << 8);
    dataSize = dataSize | cimaxContext_g.responseBuff[CIMAX_CAM_DATA_LEN_LSB_POS];


    //memcpy(buffer, &cimaxContext_g.responseBuff[CIMAX_CAM_HEADER_SIZE], dataSize);


    if (cimaxContext_g.responseBuff[CIMAX_STATUS_OFFSET] & CIMAX_TYP_MASK) {
        /* status */
        switch (cimaxContext_g.responseBuff[CIMAX_STATUS_OFFSET] & CIMAX_CMD_MASK) {
            case CIMAX_GETREGSTATUSOK:
            {
                if ((cimaxContext_g.responseBuff[4] & mask) == mask) {
                    //printf("Read status, mask = 0x%x result = 1\n", mask);
                    *result = 1;
                } else {
                    //printf("Read status, mask = 0x%x result = 0\n", mask);
                    *result = 0;
                }
                res = CIMAX_NO_ERROR;
                break;
            }

            case CIMAX_NO_CAM:
            {
                printf("CIMAX_ReadStatus(%d): CIMAX_NO_CAM\n", slotId);
                if (CIMAX_DetectionCb != NULL && cimaxContext_g.camInserted[slotId] == TRUE) {
                    cimaxContext_g.camInserted[slotId] == FALSE;
                    CIMAX_DetectionCb(slotId, EVT_REMOVED, NULL);
                }
                //res = CIMAX_NO_CAM_ERROR;
                res = CIMAX_NO_ERROR;
                break;
            }

            case CIMAX_PENDING_CMD:
            {
                printf("CIMAX_ReadStatus(%d): CIMAX_PENDING_CMD\n", slotId);
                res = CIMAX_CAM_CMD_PENDING;
                break;
            }

            case CIMAX_DATAREADY:
            {
                if (mask == 0x80) {
                    printf("CIMAX_ReadStatus(%d,DA): status DataReady\n", slotId);
                    *result = 1;
                } else if (mask == 0x40) {
                    printf("CIMAX_ReadStatus(%d,FR): status DataReady\n", slotId);
                    *result = 0;
                } else if (mask == 0x10) {
                    printf("CIMAX_ReadStatus(%d,IIR): status DataReady\n", slotId);
                    *result = 0;
                } else {
                    *result = 0;
                }
                res = CIMAX_NO_ERROR;
                break;
            }

            default:
            {
                printf("CIMAX_ReadStatus(%d): Unknown status (%d)\n", slotId, cimaxContext_g.responseBuff[CIMAX_STATUS_OFFSET] & CIMAX_CMD_MASK);
                res = CIMAX_ERROR;
                break;
            }
        }
    }

    /* Reset data ready flag. */
    //cimaxContext_g.camDataReady[slotId] = FALSE;

    OS_MutexUnlock(cimaxContext_g.hIntMutex);

    return res;

}

/******************************************************************************
 * @brief
 *   TS PassThrough the module or not.
 *
 * @param  slotId
 *     CAM slot id.
 *     value 0=passthrough 1=bypass
 *
 * @return
 *     CIMAX_NO_ERROR if successfull, error code if failure.
 ******************************************************************************/
int32 CIMAX_CAMPassThrough(uint8 slotId, uint8 value) {
    unsigned long u32Size = 0;
    unsigned char pu8Cmd[CIMAX_CI_HEADER_SIZE + 1];
    int32 res = 0;
    uint8 val_mod_reg = 0;
    uint8 val_ch_reg = 0;
    int currAddr = 0;
    unsigned char cmdValue = 0;

    printf("CIMAX_CAMPassThrough slot[%d] = 0x%x\n", slotId, value);

    if (slotId >= CIMAX_CAM_MAX_SLOT_NUM) {
        printf("CIMAX_CAMReset: Invalid slot id (%d)\n", slotId);
        return (CIMAX_INVALID_SLOT_ID_ERROR);
    }

    if (value == 0) {
        CIMAX_CAM_TSPassThrough[slotId] = TRUE;
    } else {
        CIMAX_CAM_TSPassThrough[slotId] = FALSE;
    }

    if ((CIMAX_CAM_Inserted[0] == FALSE) && (CIMAX_CAM_Inserted[1] == FALSE)) {
        val_mod_reg = 0x00;
        val_ch_reg = 0x18;
    } else if ((CIMAX_CAM_Inserted[0] == TRUE) && (CIMAX_CAM_Inserted[1] == FALSE)) {
        if (CIMAX_CAM_TSPassThrough[0] == TRUE) {
            val_mod_reg = 0x01;
            val_ch_reg = 0x58;
        } else {
            val_mod_reg = 0x01;
            val_ch_reg = 0x18;
        }
    } else if ((CIMAX_CAM_Inserted[0] == FALSE) && (CIMAX_CAM_Inserted[1] == TRUE)) {
        if (CIMAX_CAM_TSPassThrough[1] == TRUE) {
            val_mod_reg = 0x10;
            val_ch_reg = 0x08;
        } else {
            val_mod_reg = 0x10;
            val_ch_reg = 0x18;
        }
    } else /*if ((CIMAX_CAM_Inserted[0] == TRUE) && (CIMAX_CAM_Inserted[1] == TRUE))*/ {
        if ((CIMAX_CAM_TSPassThrough[0] == TRUE) && (CIMAX_CAM_TSPassThrough[1] == TRUE)) {
            val_mod_reg = 0x51;
            val_ch_reg = 0x08;
        } else if ((CIMAX_CAM_TSPassThrough[0] == TRUE) && (CIMAX_CAM_TSPassThrough[1] == FALSE)) {
            val_mod_reg = 0x11;
            val_ch_reg = 0x58;
        } else if ((CIMAX_CAM_TSPassThrough[0] == FALSE) && (CIMAX_CAM_TSPassThrough[1] == TRUE)) {
            val_mod_reg = 0x11;
            val_ch_reg = 0x08;
        } else /*if ((CIMAX_CAM_TSPassThrough[0] == FALSE) && (CIMAX_CAM_TSPassThrough[1] == FALSE))*/ {
            val_mod_reg = 0x11;
            val_ch_reg = 0x18;
        }
    }

    if (res == 0) {
        pu8Cmd[0] = CIMAX_CMD_WRITE_REG;
        pu8Cmd[1] = 0x02;
        pu8Cmd[2] = 0x30;
        pu8Cmd[3] = 1;
        pu8Cmd[4] = val_mod_reg;
        u32Size = CIMAX_DEVICE_RX_BUFFER_SIZE;

        currAddr = pu8Cmd[1];
        currAddr = currAddr << 8 | pu8Cmd[2];

        cmdValue = val_mod_reg;

        if (CIMAX_WriteRegister(currAddr, &cmdValue, 1) < 0) {
            printf("CIMAX_CAMPassThrough read reg fails\n");
            res = -1;
        } else {
            printf("CIMAX_CAMPassThrough OK addr = 0x%x val = 0x%x\n", currAddr, cmdValue);
        }
    }

    if (res == 0) {
        pu8Cmd[0] = CIMAX_CMD_WRITE_REG;
        pu8Cmd[1] = 0x02;
        pu8Cmd[2] = 0x2F;
        pu8Cmd[3] = 1;
        pu8Cmd[4] = val_ch_reg;
        u32Size = CIMAX_DEVICE_RX_BUFFER_SIZE;

        currAddr = pu8Cmd[1];
        currAddr = currAddr << 8 | pu8Cmd[2];

        cmdValue = val_ch_reg;

        if (CIMAX_WriteRegister(currAddr, &cmdValue, 1) < 0) {
            printf("CIMAX_CAMPassThrough read reg fails\n");
            res = -1;
        } else {
            printf("CIMAX_CAMPassThrough OK addr = 0x%x val = 0x%x\n", currAddr, cmdValue);
        }
    }

    printf("CIMAX routing TS %s slot=%d set MOD_REG=0x%x CH_REG=0x%x\n", (value == 0) ? "passthrough" : "bypass", slotId, val_mod_reg, val_ch_reg);

    return (res);
}

/**
 * @ingroup     CIMaX_Globals
 *
 * @brief       Buffer negotiation.
 *
 * @param       slotId      CAM slot id.
 * @param       bufferSize  Suggested buffer size.
 *
 * @return      Negotiated buffer size if the operation was successful, error code otherwise.
 */
int32 CIMAX_Negotiate(uint8 slotId, uint16 bufferSize, uint8 *bufferNegotiate) {
    uint8 bufferOut[CIMAX_CAM_HEADER_SIZE + CIMAX_CAM_NEGOTIATE_PAYLOAD_SIZE];
    int32 responseSize = 0;
    uint16 negotiatedBufferSize = 0;
    uint8 camStatus = CIMAX_CAM_A_NEGOTIATE_OK;

    memset(bufferNegotiate, 0, 2 * sizeof (unsigned char));

    if (slotId >= CIMAX_CAM_MAX_SLOT_NUM) {
        return CIMAX_INVALID_SLOT_ID_ERROR;
    }

    /* INT access protection. */
    OS_MutexLock(cimaxContext_g.hIntMutex);

    bufferOut[CIMAX_CAM_NEGOTIATE_SIZE_MSB_POS] = bufferSize >> 8;
    bufferOut[CIMAX_CAM_NEGOTIATE_SIZE_LSB_POS] = bufferSize & 0x00FF;

    responseSize = accessCAM(slotId, CIMAX_CAM_NEGOCIATE_CMD, bufferOut, CIMAX_CAM_NEGOTIATE_PAYLOAD_SIZE);
    if (responseSize < 0) {
        OS_MutexUnlock(cimaxContext_g.hIntMutex);
        return responseSize;
    }

    /* Check if response is valid. */
    if (slotId != CIMAX_CAM_SLOT_A_ID) {
        camStatus = CIMAX_CAM_B_NEGOTIATE_OK;
    }

    /* Check response */
    if ((cimaxContext_g.responseBuff[CIMAX_CAM_COMMAND_OR_STATUS_POS] == camStatus) &&
            (cimaxContext_g.responseBuff[CIMAX_CAM_PACKET_COUNTER_POS] == CIMAX_CAM_PACKET_COUNTER_VALUE) &&
            (cimaxContext_g.responseBuff[CIMAX_CAM_DATA_LEN_MSB_POS] == 0x00) &&
            (cimaxContext_g.responseBuff[CIMAX_CAM_DATA_LEN_LSB_POS] == 0x02)) {

        bufferNegotiate[0] = cimaxContext_g.responseBuff[CIMAX_CI_HEADER_SIZE];
        bufferNegotiate[1] = cimaxContext_g.responseBuff[CIMAX_CI_HEADER_SIZE + 1];

        OS_MutexUnlock(cimaxContext_g.hIntMutex);
        return CIMAX_NO_ERROR;
    }

    printf("%s: Invalid response received!\n", __FUNCTION__);

    OS_MutexUnlock(cimaxContext_g.hIntMutex);
    return CIMAX_CAM_RESPONSE_ERROR;
}

/**
 * @ingroup     CIMaX_Globals
 *
 * @brief       Write LPDU.
 *
 * @param       slotId      CAM slot id.
 * @param       buffer      Buffer with data to be written.
 * @param       numOfBytes  Number of bytes to be written.
 *
 * @return      CIMAX_NO_ERROR if successfull, error code if failure.
 */
int32 CIMAX_WriteLPDU(uint8 slotId, uint8 *buffer, uint16 numOfBytes) {
    int32 responseSize = 0;
    uint8 camStatus = CIMAX_CAM_A_WRITE_LPDU_OK;

    if (slotId >= CIMAX_CAM_MAX_SLOT_NUM) {
        return CIMAX_INVALID_SLOT_ID_ERROR;
    }

    /* INT access protection. */
    OS_MutexLock(cimaxContext_g.hIntMutex);

    responseSize = accessCAM(slotId, CIMAX_CAM_WRITE_LPDU_CMD, buffer, numOfBytes);
    if (responseSize < 0) {
        OS_MutexUnlock(cimaxContext_g.hIntMutex);
        return responseSize;
    }

    /* Check if response is valid. */
    if (slotId != CIMAX_CAM_SLOT_A_ID) {
        camStatus = CIMAX_CAM_B_WRITE_LPDU_OK;
    }

    if ((cimaxContext_g.responseBuff[CIMAX_CAM_COMMAND_OR_STATUS_POS] == camStatus) &&
            (cimaxContext_g.responseBuff[CIMAX_CAM_PACKET_COUNTER_POS] == CIMAX_CAM_PACKET_COUNTER_VALUE) &&
            (cimaxContext_g.responseBuff[CIMAX_CAM_DATA_LEN_MSB_POS] == 0x00) &&
            (cimaxContext_g.responseBuff[CIMAX_CAM_DATA_LEN_LSB_POS] == 0x00)) {
        OS_MutexUnlock(cimaxContext_g.hIntMutex);
        return CIMAX_NO_ERROR;
    }

    printf("%s: Invalid response received!\n", __FUNCTION__);

    OS_MutexUnlock(cimaxContext_g.hIntMutex);

    return CIMAX_CAM_RESPONSE_ERROR;
}

/**
 * @ingroup     CIMaX_Globals
 *
 * @brief       Read LPDU.
 *
 * @param       slotId      CAM slot id.
 * @param       buffer      Buffer for data to be read.
 * @param       numOfBytes  Number of bytes to be read.
 *
 * @return      Number of data read if successfull, error code if failure.
 */
int32 CIMAX_ReadLPDU(uint8 slotId, uint8 *buffer, uint16 numOfBytes) {
    uint8 bufferOut[CIMAX_CAM_HEADER_SIZE];
    int32 responseSize = 0;
    int32 dataSize = 0;
    uint8 camStatus = CIMAX_CAM_A_READ_LPDU_OK;

    if (slotId >= CIMAX_CAM_MAX_SLOT_NUM) {
        return CIMAX_INVALID_SLOT_ID_ERROR;
    }

    /* INT access protection. */
    OS_MutexLock(cimaxContext_g.hIntMutex);

    responseSize = accessCAM(slotId, CIMAX_CAM_READ_LPDU_CMD, bufferOut, 0);
    if (responseSize < 0) {
        OS_MutexUnlock(cimaxContext_g.hIntMutex);
        printf("ReadLPDU error, responseSize = %d\n", responseSize);
        return responseSize;
    }

    /* Check if response is valid. */
    if (slotId != CIMAX_CAM_SLOT_A_ID) {
        camStatus = CIMAX_CAM_B_READ_LPDU_OK;
    }

    if ((cimaxContext_g.responseBuff[CIMAX_CAM_COMMAND_OR_STATUS_POS] != camStatus) ||
            (cimaxContext_g.responseBuff[CIMAX_CAM_PACKET_COUNTER_POS] != CIMAX_CAM_PACKET_COUNTER_VALUE)) {
        printf("%s: Invalid response received!\n", __FUNCTION__);
        OS_MutexUnlock(cimaxContext_g.hIntMutex);
        return CIMAX_CAM_RESPONSE_ERROR;
    }

    dataSize = cimaxContext_g.responseBuff[CIMAX_CAM_DATA_LEN_MSB_POS];
    dataSize = (dataSize << 8);
    dataSize = dataSize | cimaxContext_g.responseBuff[CIMAX_CAM_DATA_LEN_LSB_POS];

    memcpy(buffer, &cimaxContext_g.responseBuff[CIMAX_CAM_HEADER_SIZE], dataSize);

    /* Reset data ready flag. */
    //cimaxContext_g.camDataReady[slotId] = FALSE;
    //CIMAX_DataToReadCb(slotId, 0);

    OS_MutexUnlock(cimaxContext_g.hIntMutex);

    return dataSize;
}

/******************************************************************************
 *  Internal helpers
 ******************************************************************************/

/**
 * @ingroup     CIMaX_Globals
 *
 * @brief       Read thread function. It detects active CIMaX+ INT pin and reads response.
 *
 * @param       params      Thread parameters.
 *
 * @return      CIMAX_NO_ERROR if successfull, CIMAX_ERROR if failure.
 */
int32 asyncEventsThreadFunction(void *params) {
    uint8 bufferOut[CIMAX_CAM_HEADER_SIZE];
    uint8 slotId;
    uint8 intState;

    printf("%s: thread for handling asynchronous events created.\n", __FUNCTION__);
    while (!cimaxContext_g.bStopReadThread) {
        /* INT access protection. */
        OS_MutexLock(cimaxContext_g.hIntMutex);


        /* Read INT Pin */
        galois_gpio_data_t gpio_data;
        galois_gpio_data_t * pgpio_data = &gpio_data;

        gpio_data.port = 0; // gpio 0
        gpio_data.mode = 1;
        gpio_data.data = 1;

        gpio_read(pgpio_data);

        /* If CIMaX+ INT pin is inactive, do nothing. */
        if (pgpio_data->data != 0) {
            OS_MutexUnlock(cimaxContext_g.hIntMutex);
            OS_Sleep(2);
            continue;
        }

        /* Prepare GetEvt command header. */
        bufferOut[CIMAX_REGISTER_CMD_POS] = CIMAX_CAM_GET_EVT_CMD;
        bufferOut[CIMAX_REGISTER_REG_ADDR_MSB_POS] = 0x01;
        bufferOut[CIMAX_REGISTER_REG_ADDR_LSB_POS] = 0x00;
        bufferOut[CIMAX_REGISTER_NUM_OF_REGS_POS] = 0x00;

        /* Send command to CIMaX+ */
        if (CIMAX_SPI_Write(bufferOut, CIMAX_REGISTER_HEADER_SIZE) != CIMAX_NO_ERROR) {
            OS_MutexUnlock(cimaxContext_g.hIntMutex);
            continue;
        }

        if (getResponse(CIMAX_CAM_RESPONSE_TIMEOUT_MS) < 0) {
            printf("%s: Invalid GetEvt response length.\n", __FUNCTION__);
            OS_MutexUnlock(cimaxContext_g.hIntMutex);
            continue;
        }

        switch (cimaxContext_g.responseBuff[CIMAX_REGISTER_CMD_POS] & CIMAX_CAM_TYP_CMD_MASK) {
            case CIMAX_CAM_A_DET_OK: /* or CIMAX_CAM_B_DET_OK */
                /* Process CamDet message */
                slotId = CIMAX_CAM_SLOT_A_ID;
                if (cimaxContext_g.responseBuff[CIMAX_REGISTER_CMD_POS] & CIMAX_CAM_SLOT_MASK) {
                    slotId++; /* = CIMAX_CAM_SLOT_B_ID */
                }
                printf("%s: CamDet message received : 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x.\n",
                        __FUNCTION__, cimaxContext_g.responseBuff[CIMAX_CAM_COMMAND_OR_STATUS_POS],
                        cimaxContext_g.responseBuff[CIMAX_CAM_PACKET_COUNTER_POS],
                        cimaxContext_g.responseBuff[CIMAX_CAM_DATA_LEN_MSB_POS],
                        cimaxContext_g.responseBuff[CIMAX_CAM_DATA_LEN_LSB_POS],
                        cimaxContext_g.responseBuff[CIMAX_CAM_FIRST_DATA_BYTE_POS]);
                if (cimaxContext_g.responseBuff[CIMAX_CAM_FIRST_DATA_BYTE_POS] == CIMAX_CAM_INSERTED) {
                    /* CAM is inserted callback */
                    printf("CAM %d is inserted\n", slotId);
                    cimaxContext_g.camInserted[slotId] = TRUE;
                    CIMAX_DetectionCb(slotId, EVT_INSERTED, NULL);
                } else {
                    printf("CAM %d is removed\n", slotId);
                    cimaxContext_g.camInserted[slotId] = FALSE;
                    CIMAX_DetectionCb(slotId, EVT_REMOVED, NULL);
                }
                break;
            case CIMAX_CAM_A_DATA_READY: /* or CIMAX_CAM_B_DATA_READY */
                /* Process DataReady message */
                slotId = CIMAX_CAM_SLOT_A_ID;
                if (cimaxContext_g.responseBuff[CIMAX_REGISTER_CMD_POS] & CIMAX_CAM_SLOT_MASK) {
                    slotId++; /* = CIMAX_CAM_SLOT_B_ID */
                }
                cimaxContext_g.camDataReady[slotId] = TRUE; /* TODO. */

                //printf("Callback Data ready\n");
                CIMAX_DataToReadCb(slotId, NULL);

                printf("%s: DataReady message received : 0x%02x 0x%02x 0x%02x 0x%02x.\n",
                        __FUNCTION__, cimaxContext_g.responseBuff[CIMAX_CAM_COMMAND_OR_STATUS_POS],
                        cimaxContext_g.responseBuff[CIMAX_CAM_PACKET_COUNTER_POS],
                        cimaxContext_g.responseBuff[CIMAX_CAM_DATA_LEN_MSB_POS],
                        cimaxContext_g.responseBuff[CIMAX_CAM_DATA_LEN_LSB_POS]);
                break;
            case CIMAX_CAM_NO_EVT:
                /* Process NoEvt message. */
                printf("%s: NoEvt message received : 0x%02x 0x%02x 0x%02x 0x%02x.\n",
                        __FUNCTION__, cimaxContext_g.responseBuff[CIMAX_CAM_COMMAND_OR_STATUS_POS],
                        cimaxContext_g.responseBuff[CIMAX_CAM_PACKET_COUNTER_POS],
                        cimaxContext_g.responseBuff[CIMAX_CAM_DATA_LEN_MSB_POS],
                        cimaxContext_g.responseBuff[CIMAX_CAM_DATA_LEN_LSB_POS]);
                break;
            default:
                printf("%s: ***** INVALID message received :0x%02x 0x%02x 0x%02x 0x%02x. *****\n",
                        __FUNCTION__, cimaxContext_g.responseBuff[CIMAX_CAM_COMMAND_OR_STATUS_POS],
                        cimaxContext_g.responseBuff[CIMAX_CAM_PACKET_COUNTER_POS],
                        cimaxContext_g.responseBuff[CIMAX_CAM_DATA_LEN_MSB_POS],
                        cimaxContext_g.responseBuff[CIMAX_CAM_DATA_LEN_LSB_POS]);
                /* cimaxContext_g.bStopReadThread = TRUE; */
                break;
        } /* switch (readBuffer[CIMAX_REGISTER_CMD_POS] & CIMAX_CAM_TYP_CMD_MASK) */
        OS_MutexUnlock(cimaxContext_g.hIntMutex);
    } /* while */

    return CIMAX_NO_ERROR;
}

/**************************************************************************//**
* @defgroup CIMaX_Statics CIMaX+ Static Functions Definitions
******************************************************************************/

/**
 * @ingroup     CIMaX_Statics
 *
 * @brief       Downloads FW to CIMaX+ from file.
 *
 * @param       none
 *
 * @return      CIMAX_NO_ERROR if successfull, error code if failure.
 */
int32 downloadFW(void) {
    osFile_t* fd;
    uint16 currAddr = FW_START_ADDRESS;
    uint8 packetSize = 0;
    uint32 cnt = 0;
    uint8 bufferIn[FW_PACKET_SIZE + CIMAX_REGISTER_HEADER_SIZE];
    uint8 bufferOut[FW_PACKET_SIZE + CIMAX_REGISTER_HEADER_SIZE];
    uint16 FWSign = 0;

    printf("Download CIMaX+ FW from %s \t:  ", FW_FILE_NAME);

    /* Open CIMaX+ FW file. */
    fd = OS_FileOpen(FW_FILE_NAME, "rb");
    if (fd == NULL) {
        printf("%s: Opening FW file failed.\n", __FUNCTION__);
        return CIMAX_ERROR;
    }

    /* Position to FW start address inside CIMaX+ FW file. */
    OS_FileSeek(fd, FW_START_ADDRESS, OS_SEEK_SET);

    do {
        /* Calculate packet size for sending to CIMaX+. */
        packetSize = (uint8) ((currAddr <= (FW_END_ADDRESS + 1 - FW_PACKET_SIZE)) ? (FW_PACKET_SIZE) : (FW_END_ADDRESS + 1 - currAddr));

        /* Fill output buffer with data from CIMaX+ FW file. */
        if ((OS_FileRead(bufferOut, packetSize, fd)) != packetSize) {
            printf("%s: reading FW file failed!\n", __FUNCTION__);
            return CIMAX_ERROR;
        }

        /* Compute BistRom. */
        FWSign = computeBistRom(bufferOut, packetSize, FWSign);

        /* Write data to CIMaX+. */
        if (CIMAX_WriteRegister(currAddr, bufferOut, packetSize) < 0) {
            /* CIMaX+ write error. */
            printf("%s: write packet failed!\n", __FUNCTION__);
            return CIMAX_ERROR;
        }

        /*  Read data previously written to CIMaX+. */
        if (CIMAX_ReadRegister(currAddr, bufferIn, packetSize) < 0) {
            /* CIMaX+ read error */
            printf("%s: read packet failed!\n", __FUNCTION__);
            return CIMAX_ERROR;
        }

        /* At the end, compare written and read data. */
        if (memcmp(bufferOut, bufferIn, packetSize) != 0) {
            printf("%s: read/write data mismatch error at address 0x%02x!\n", __FUNCTION__, currAddr);
            return CIMAX_ERROR;
        }
        currAddr += packetSize;

        /* Show progress. */
        printf((cnt++ % 2) ? ("\b\\") : ("\b/"));

    } while (currAddr < (FW_END_ADDRESS + 1));

    printf("\bOK.\n");

    printf("Load vector table \t\t\t\t:  ");

    /* Position to vector table start address inside CIMaX+ FW file. */
    OS_FileSeek(fd, FW_VECTOR_TABLE_START_ADDRESS, OS_SEEK_SET);

    /* Set current address. */
    currAddr = FW_VECTOR_TABLE_START_ADDRESS;

    /* Set packet size for vector table. */
    packetSize = FW_VECTOR_TABLE_SIZE;

    /* Fill output buffer with data from CIMaX+ FW file. */
    if ((OS_FileRead(bufferOut, packetSize, fd)) != packetSize) {
        printf("%s: reading FW file failed!\n", __FUNCTION__);
        return CIMAX_ERROR;
    }

    /* Compute BistRom. */
    FWSign = computeBistRom(bufferOut, packetSize, FWSign);

    /* Write data to CIMaX+. */
    if (CIMAX_WriteRegister(currAddr, bufferOut, packetSize) < 0) {
        /* CIMaX+ write error. */
        printf("%s: write packet failed!\n", __FUNCTION__);
        return CIMAX_ERROR;
    }

    /* Read data previously written to CIMaX+. */
    if (CIMAX_ReadRegister(currAddr, bufferIn, packetSize) < 0) {
        /* CIMaX+ read error. */
        printf("%s: read packet failed!\n", __FUNCTION__);
        return CIMAX_ERROR;
    }

    /* At the end, compare written and read data. */
    if (memcmp(bufferOut, bufferIn, packetSize) != 0) {
        printf("%s: read/write data mismatch error at address 0x%02x!\n", __FUNCTION__, currAddr);
        return CIMAX_ERROR;
    }

    printf("\bOK.\n");

    printf("Checking BistRom \t\t\t\t:  ");

    /* Check BistRom */
    if (checkBistRom(FW_START_ADDRESS, FW_END_ADDRESS, FWSign) != 0x02) {
        printf("%s: checking BistRom failed!\n", __FUNCTION__);
        return CIMAX_ERROR;
    }

    printf("\bOK.\n");

    printf("Start FW execution \t\t\t\t:  ");
    /* Init command - indicates the end of boot sequence. */
    if (initFW() != CIMAX_NO_ERROR) {
        printf("%s: init FW failed!\n", __FUNCTION__);
        return CIMAX_ERROR;
    }

    printf("\bOK.\n");

    /* Close CIMaX+ FW file. */
    OS_FileClose(fd);

    return CIMAX_NO_ERROR;
}

/**
 * @ingroup     CIMaX_Statics
 *
 * @brief       Starts CIMaX+ FW execution (sends INIT command).
 *
 * @param       none
 *
 * @return      CIMAX_NO_ERROR if successfull, error code if failure.
 */
int32 initFW(void) {
    uint8 bufferOut[CIMAX_REGISTER_HEADER_SIZE];
    int32 retVal = CIMAX_ERROR;

    /* Prepare command header. */
    memset(bufferOut, 0, sizeof (bufferOut));

    /* INT access protection. */
    OS_MutexLock(cimaxContext_g.hIntMutex);

    /* Send command to CIMaX+. */
    if (CIMAX_SPI_Write(bufferOut, CIMAX_REGISTER_HEADER_SIZE) != CIMAX_NO_ERROR) {
        OS_MutexUnlock(cimaxContext_g.hIntMutex);
        return CIMAX_SPI_COMM_ERROR;
    }

    retVal = getResponse(CIMAX_REGISTER_RESPONSE_TIMEOUT_MS);
    if (retVal < 0) {
        OS_MutexUnlock(cimaxContext_g.hIntMutex);
        return retVal;
    }

    /* Check response. */
    if ((cimaxContext_g.responseBuff[CIMAX_REGISTER_CMD_POS] == CIMAX_REGISTER_INIT_RESP_OK) &&
            (cimaxContext_g.responseBuff[CIMAX_REGISTER_REG_ADDR_MSB_POS] == 0x00) &&
            (cimaxContext_g.responseBuff[CIMAX_REGISTER_REG_ADDR_LSB_POS] == 0x00) &&
            (cimaxContext_g.responseBuff[CIMAX_REGISTER_NUM_OF_REGS_POS] == 0x00)) {
        OS_MutexUnlock(cimaxContext_g.hIntMutex);
        return CIMAX_NO_ERROR;
    }

    OS_MutexUnlock(cimaxContext_g.hIntMutex);

    return CIMAX_INIT_RESP_ERROR;
}

/**
 * @ingroup     CIMaX_Statics
 *
 * @brief       Downloads configuration (register settings) defined in spiRegSettings array. 
 *
 * @param       none
 *
 * @return      CIMAX_NO_ERROR if successfull, CIMAX_ERROR if failure.
 */
int32 downloadCfg(void) {
    uint32 cnt;
    uint8 buf[CIMAX_REGISTER_MAX_PAYLOAD_SIZE];

    printf("Download CIMaX+ configuration(register settings): ");

    for (cnt = 0; cnt < sizeof (spiRegSettings) / sizeof (regSettings_t); cnt++) {
        switch (spiRegSettings[cnt].op) {
            case REG_OP_READ:
                /* Read register. */
                if (CIMAX_ReadRegister(spiRegSettings[cnt].reg, buf, 1) < 0) {
                    /* CIMaX+ read error. */
                    printf("FAILED at REG_OP_READ operation.\n");
                    return CIMAX_ERROR;
                }
                //printf("Read operation: register 0x%04x = 0x%02x.\n", spiRegSettings[cnt].reg, buf[0]);
                break;
            case REG_OP_WRITE:
                /* Write register. */
                if (CIMAX_WriteRegister(spiRegSettings[cnt].reg, (uint8 *) & spiRegSettings[cnt].val, 1) < 0) {
                    /* CIMaX+ write error. */
                    printf("FAILED at REG_OP_WRITE operation.\n");
                    return CIMAX_ERROR;
                }
                //printf("Write operation: register 0x%04x = 0x%02x.\n", spiRegSettings[cnt].reg, spiRegSettings[cnt].val);
                break;
            case REG_OP_WAIT_TO_BE_SET:
                do {
                    if (CIMAX_ReadRegister(spiRegSettings[cnt].reg, buf, 1) < 0) {
                        /* CIMaX+ read error. */
                        printf("FAILED at REG_OP_WAIT_TO_BE_SET operation.\n");
                        return CIMAX_ERROR;
                    }
                } while ((buf[0] & spiRegSettings[cnt].val) != spiRegSettings[cnt].val);
                //printf("Wait register to be set operation: register 0x%04x = 0x%02x.\n", spiRegSettings[cnt].reg, buf[0]);
                break;
            case REG_OP_WAIT_TO_BE_CLEARED:
                do {
                    if (CIMAX_ReadRegister(spiRegSettings[cnt].reg, buf, 1) < 0) {
                        /* CIMaX+ read error. */
                        printf("FAILED at REG_OP_WAIT_TO_BE_CLEARED operation.\n");
                        return CIMAX_ERROR;
                    }
                } while ((buf[0] & spiRegSettings[cnt].val) != 0);
                //printf("Wait register to be cleared operation: register 0x%04x = 0x%02x.\n", spiRegSettings[cnt].reg, buf[0]);
                break;
            case REG_OP_WAIT_EQUAL:
                do {
                    if (CIMAX_ReadRegister(spiRegSettings[cnt].reg, buf, 1) < 0) {
                        /* CIMaX+ read error. */
                        printf("FAILED at REG_OP_WAIT_EQUAL operation.\n");
                        return CIMAX_ERROR;
                    }
                } while (buf[0] != spiRegSettings[cnt].val);
                //printf("Wait register to be equal operation: register 0x%04x = 0x%02x.\n", spiRegSettings[cnt].reg, buf[0]);
                break;
            case REG_OP_LOGICAL_AND:
                if (CIMAX_ReadRegister(spiRegSettings[cnt].reg, buf, 1) < 0) {
                    /* CIMaX+ read error. */
                    printf("FAILED at REG_OP_LOGICAL_AND operation (reading).\n");
                    return CIMAX_ERROR;
                }
                buf[0] &= spiRegSettings[cnt].val;
                if (CIMAX_WriteRegister(spiRegSettings[cnt].reg, buf, 1) < 0) {
                    /* CIMaX+ write error. */
                    printf("FAILED at REG_OP_LOGICAL_AND operation (writing).\n");
                    return CIMAX_ERROR;
                }
                //printf("Logical AND operation: register 0x%04x = 0x%02x.\n", spiRegSettings[cnt].reg, buf[0]);
                break;
            case REG_OP_LOGICAL_OR:
                if (CIMAX_ReadRegister(spiRegSettings[cnt].reg, buf, 1) < 0) {
                    /* CIMaX+ read error. */
                    printf("FAILED at REG_OP_LOGICAL_OR operation (reading).\n");
                    return CIMAX_ERROR;
                }
                buf[0] |= spiRegSettings[cnt].val;
                if (CIMAX_WriteRegister(spiRegSettings[cnt].reg, buf, 1) < 0) {
                    /* CIMaX+ write error. */
                    printf("FAILED at REG_OP_LOGICAL_AND operation (writing).\n");
                    return CIMAX_ERROR;
                }
                //printf("Logical OR operation: register 0x%04x = 0x%02x.\n", spiRegSettings[cnt].reg, buf[0]);
                break;
            case REG_OP_WAIT:
                //printf("Wait for %d ms.\n", spiRegSettings[cnt].val);
                OS_Sleep(spiRegSettings[cnt].val);
                break;
            default:
                printf("\nInvalid operation 0x%02x!\n", spiRegSettings[cnt].op);
                return CIMAX_ERROR;
                break;
        }
    }
    printf("OK.\n");
    return CIMAX_NO_ERROR;
}

/**
 * @ingroup     CIMaX_Statics
 *
 * @brief       Computes bistrom value.
 *
 * @param       tab      Buffer which holds data for bistrom calculation.
 * @param       size     Number of bytes to be processed.
 * @param       FWSign   Initial value of bistrom.
 *
 * @return      Calculated bistrom value.
 */
uint16 computeBistRom(uint8 *tab, int32 size, uint16 FWSign) {
    int32 k;
    int32 i;
    uint16 mySign;

    for (k = 0; k < size; k++) {
        mySign = tab[k]&0x01;

        for (i = 0; i < 16; i++)
            if (0x88B7 & (1 << i))
                mySign ^= (FWSign >> i)&0x01;

        mySign |= ((FWSign << 1)^(tab[ k ]))&0x00FE;
        mySign |= (FWSign << 1) & 0x00FF00;

        FWSign = mySign;
    }

    return FWSign;
}

/**
 * @ingroup     CIMaX_Statics
 *
 * @brief       Checks bistrom value.
 *
 * @param       startAddr   Start address for bistrom calculation.
 * @param       endAddr     End address for bistrom calculation.
 * @param       signature   Initial value of bistrom.
 *
 * @return      Boot status if the operation was successful, error code otherwise.
 */
uint8 checkBistRom(int32 startAddr, int32 endAddr, int32 signature) {
    uint8 Val[2];

    /* Write "Flash" Size. */
    Val[0] = (uint8) ((0xD000 - startAddr)&0x00ff);
    Val[1] = (uint8) ((0xD000 - startAddr) >> 8);

    if (CIMAX_WriteRegister(0x008D, Val, 2) != 0)
        return CIMAX_ERROR;

    /* Write Signature. */
    Val[0] = (uint8) (signature & 0x00ff);
    Val[1] = (uint8) (signature >> 8);

    if (CIMAX_WriteRegister(0x0080, Val, 2) != 0)
        return CIMAX_ERROR;

    /* Launch BistRom [(D000-flashSize)..CFF9]+[FFFA..FFFF] computation. */
    Val[0] = 0x0F;
    if (CIMAX_WriteRegister(0x0082, Val, 1) != 0)
        return CIMAX_ERROR;

    /* Read Signature. */
    if (CIMAX_ReadRegister(BISTROM_SignatureLSB, Val, 2) != 0)
        return CIMAX_ERROR;

    /* Read Boot status. */
    if (CIMAX_ReadRegister(BOOT_Status, Val, 1) != 0)
        return CIMAX_ERROR;

    return Val[0];
}

#define CIMAX_MAX_MESSAGE_SIZE_WITH_HEADER 256
#define CIMAX_MAX_MESSAGE_SIZE_NO_HEADER 251 // 255(max size) - 4(header) = 251

/**
 * @ingroup     CIMaX_Statics
 *
 * @brief       CAM messages handling.
 *
 * @param       slotId         CAM identification.
 * @param       cmd            Command identification.
 * @param       bufferOut      Write buffer.
 * @param       numOfBytes     Number of bytes to write.
 *
 * @return      CIMAX_NO_ERROR if successfull, error code if failure.
 */
int32 accessCAM(uint8 slotId, uint8 cmd, uint8 *bufferOut, uint16 numOfBytes) {
    uint8 buf[CIMAX_CAM_HEADER_SIZE + CIMAX_CAM_MAX_PAYLOAD_SIZE];
    int32 retVal = CIMAX_ERROR;
    int counter = 0;
    int numOfBytesLeft = 0;
    int bytesWritten = 0;
    int i = 0;

    if (slotId != CIMAX_CAM_SLOT_A_ID) {
        cmd = cmd | CIMAX_CAM_SLOT_MASK;
    }

    /* Prepare command header. */
    buf[CIMAX_CAM_COMMAND_OR_STATUS_POS] = cmd;
    buf[CIMAX_CAM_PACKET_COUNTER_POS] = CIMAX_CAM_PACKET_COUNTER_VALUE;
    buf[CIMAX_CAM_DATA_LEN_MSB_POS] = (numOfBytes >> 8) & 0xFF;
    buf[CIMAX_CAM_DATA_LEN_LSB_POS] = numOfBytes & 0xFF;

    /* Prepare command payload. */
    memcpy(&buf[CIMAX_REGISTER_HEADER_SIZE], bufferOut, numOfBytes);

    if (numOfBytes > CIMAX_MAX_MESSAGE_SIZE_WITH_HEADER) {

        //printData(__FUNCTION__, buf, 4 + numOfBytes);

        printf("Sending large message to SPI bytes = %d\n", numOfBytes);

        for (counter = 0; counter < (numOfBytes / CIMAX_MAX_MESSAGE_SIZE_WITH_HEADER); counter++) {

            if (CIMAX_SPI_Write(buf + (counter * CIMAX_MAX_MESSAGE_SIZE_WITH_HEADER), CIMAX_MAX_MESSAGE_SIZE_WITH_HEADER) != CIMAX_NO_ERROR) {
                printf("Error while writing in loop\n");
                return CIMAX_SPI_COMM_ERROR;
            }
            bytesWritten += CIMAX_MAX_MESSAGE_SIZE_WITH_HEADER;

        }

        numOfBytesLeft = numOfBytes - counter * CIMAX_MAX_MESSAGE_SIZE_WITH_HEADER;
        printf("numOfBytesLeft = %d\n", numOfBytesLeft);

        if (numOfBytesLeft > 0) {

            //printf("Write remaining bytes = %d\n", numOfBytesLeft);
            //printf("write from 0x%x\n", *(buf + (counter * CIMAX_MAX_MESSAGE_SIZE_WITH_HEADER)));

            if (CIMAX_SPI_Write(buf + counter * CIMAX_MAX_MESSAGE_SIZE_WITH_HEADER, numOfBytesLeft + 4) != CIMAX_NO_ERROR) {
                printf("Error while writing bytes left\n");
                return CIMAX_SPI_COMM_ERROR;
            }
        }

        /*
                printf("all is written, send read status to trigger int pin\n");

                unsigned char tempBuff[4];

                tempBuff[0] = 0x8e;
                tempBuff[1] = 0x01;
                tempBuff[2] = 0x00;
                tempBuff[3] = 0x00;

                if (CIMAX_SPI_Write(tempBuff, 4) != CIMAX_NO_ERROR) {
                    printf("Error while writing temp bytes\n");
                    return CIMAX_SPI_COMM_ERROR;
                }
         */

        //return CIMAX_NO_ERROR;

    } else {

        /* Send command to CAM. */
        if (CIMAX_SPI_Write(buf, numOfBytes + CIMAX_REGISTER_HEADER_SIZE) != CIMAX_NO_ERROR) {
            return CIMAX_SPI_COMM_ERROR;
        }

    }



    retVal = getResponse(CIMAX_CAM_RESPONSE_TIMEOUT_MS);
    if (retVal < 0) {
        printf("retVal error = %d\n", retVal);
        return retVal;
    }

    //printData("Response:", cimaxContext_g.responseBuff, 4);

    if ((slotId << CIMAX_CAM_SLOT_BIT_POSITION) != (cimaxContext_g.responseBuff[CIMAX_CAM_COMMAND_OR_STATUS_POS] & CIMAX_CAM_SLOT_MASK)) {
        printf("%s: ***** slot MISMATCH (%d instead of %d)! *****\n", __FUNCTION__,
                (cimaxContext_g.responseBuff[CIMAX_CAM_COMMAND_OR_STATUS_POS] >> CIMAX_CAM_SLOT_BIT_POSITION), slotId);
    }

    /* Check response from CIMax+. */
    if (responseCamNoCam(slotId) == TRUE) {
        printf("%s: CAMNoCAM %d received!\n", __FUNCTION__, slotId);
        return CIMAX_NO_CAM_ERROR;
    }

    if (responseCamError(slotId) == TRUE) {
        printf("%s: CAMError %d received!\n", __FUNCTION__, slotId);
        return CIMAX_CAM_ERROR;
    }

    if (responseCamWriteBusy(slotId) == TRUE) {
        printf("%s: CAMWriteBusy %d received!\n", __FUNCTION__, slotId);
        return CIMAX_CAM_WRITE_BUSY_ERROR;
    }

    if (responseCamCmdPending(slotId) == TRUE) {
        printf("%s: CAMCmdPending %d received!\n", __FUNCTION__, slotId);
        return CIMAX_CAM_CMD_PENDING;
    }

    return retVal;
}

/**
 * @ingroup     CIMaX_Statics
 *
 * @brief       Checks if CIMaX+ response is CAMNoCam message.
 *
 * @param       slotId   CAM slot id.
 *
 * @return      TRUE if response is CAMNoCam, otherwise FALSE.
 */
BOOL responseCamNoCam(uint8 slotId) {
    uint8 camStatus = CIMAX_CAM_A_NO_CAM;

    /* Check if response is valid. */
    if (slotId != CIMAX_CAM_SLOT_A_ID) {
        camStatus = CIMAX_CAM_B_NO_CAM;
    }

    if ((cimaxContext_g.responseBuff[CIMAX_CAM_COMMAND_OR_STATUS_POS] == camStatus) &&
            (cimaxContext_g.responseBuff[CIMAX_CAM_PACKET_COUNTER_POS] == CIMAX_CAM_PACKET_COUNTER_VALUE) &&
            (cimaxContext_g.responseBuff[CIMAX_CAM_DATA_LEN_MSB_POS] == 0x00) &&
            (cimaxContext_g.responseBuff[CIMAX_CAM_DATA_LEN_LSB_POS] == 0x00)) {
        /* Reset CAM detection variable. */
        if (cimaxContext_g.responseBuff[CIMAX_CAM_COMMAND_OR_STATUS_POS] & CIMAX_CAM_SLOT_MASK) {
            cimaxContext_g.camInserted[CIMAX_CAM_SLOT_B_ID] = FALSE;
        } else {
            cimaxContext_g.camInserted[CIMAX_CAM_SLOT_A_ID] = FALSE;
        }
        return TRUE;
    }
    return FALSE;
}

/**
 * @ingroup     CIMaX_Statics
 *
 * @brief       Checks if CIMaX+ response is CAMError message.
 *
 * @param       slotId   CAM slot id.
 *
 * @return      TRUE if response is CAMError, otherwise FALSE.
 */
BOOL responseCamError(uint8 slotId) {
    uint8 camStatus = CIMAX_CAM_A_CAM_ERROR;
    uint16 receivedErrorNumber;

    /* Check if response is valid. */
    if (slotId != CIMAX_CAM_SLOT_A_ID) {
        camStatus = CIMAX_CAM_B_CAM_ERROR;
    }

    if ((cimaxContext_g.responseBuff[CIMAX_CAM_COMMAND_OR_STATUS_POS] == camStatus) &&
            (cimaxContext_g.responseBuff[CIMAX_CAM_PACKET_COUNTER_POS] == CIMAX_CAM_PACKET_COUNTER_VALUE) &&
            (cimaxContext_g.responseBuff[CIMAX_CAM_DATA_LEN_MSB_POS] == 0x00) &&
            (cimaxContext_g.responseBuff[CIMAX_CAM_DATA_LEN_LSB_POS] == 0x02)) {
        receivedErrorNumber = cimaxContext_g.responseBuff[CIMAX_CAM_FIRST_DATA_BYTE_POS] & 0x0F;
        receivedErrorNumber = receivedErrorNumber << 8;
        receivedErrorNumber |= cimaxContext_g.responseBuff[CIMAX_CAM_SECOND_DATA_BYTE_POS];

        printf("%s: CAMError %d received: ", __FUNCTION__, slotId);

        switch (receivedErrorNumber) {
            case CAMERROR_RESET:
                printf("CAM not ready!\n");
                break;
            case CAMERROR_CIS_BUFFER:
                printf("CIS buffer not allocated!\n");
                break;
            case CAMERROR_CIS_BAD_SIZE:
                printf("Bad CIS size (> 512 bytes)!\n");
                break;
            case CAMERROR_CIS_CAM_NOT_ACTIVATED:
                printf("CAM not activated!\n");
                break;
            case CAMERROR_COR_CAM_NOT_READY:
                printf("CAM not ready after software reset during write COR process!\n");
                break;
            case CAMERROR_COR_VALUE_CHECKING:
                printf("Checking of COR value failed!\n");
                break;
            case CAMERROR_NEGOTIATE_NOT_RESPONDING:
                printf("CAM not responding after SR bit is raised during buffer negociation phase!\n");
                break;
            case CAMERROR_NEGOTIATE_SIZE_LENGTH:
                printf("CAM buffer size length <> 2!\n");
                break;
            case CAMERROR_NEGOTIATE_NOT_READY:
                printf("CAM not ready to accept new buffer size negotiate during buffer negociation phase!\n");
                break;
            case CAMERROR_LPDU_NOT_AVAILABLE:
                printf("LPDU not available!\n");
                break;
            default:
                printf("unknown error 0x%04x!\n", receivedErrorNumber);
                break;
        }

        return TRUE;
    }
    return FALSE;
}

/**
 * @ingroup     CIMaX_Statics
 *
 * @brief       Checks if CIMaX+ response is CAMWriteBusy message.
 *
 * @param       slotId   CAM slot id.
 *
 * @return      TRUE if response is CAMWriteBusy, otherwise FALSE.
 */
BOOL responseCamWriteBusy(uint8 slotId) {
    uint8 camStatus = CIMAX_CAM_A_WRITE_BUSY;

    /* Check if response is valid. */
    if (slotId != CIMAX_CAM_SLOT_A_ID) {
        camStatus = CIMAX_CAM_B_WRITE_BUSY;
    }

    if ((cimaxContext_g.responseBuff[CIMAX_CAM_COMMAND_OR_STATUS_POS] == camStatus) &&
            (cimaxContext_g.responseBuff[CIMAX_CAM_PACKET_COUNTER_POS] == CIMAX_CAM_PACKET_COUNTER_VALUE) &&
            (cimaxContext_g.responseBuff[CIMAX_CAM_DATA_LEN_MSB_POS] == 0x00) &&
            (cimaxContext_g.responseBuff[CIMAX_CAM_DATA_LEN_LSB_POS] == 0x00)) {
        return TRUE;
    }
    return FALSE;
}

/**
 * @ingroup     CIMaX_Statics
 *
 * @brief       Checks if CIMaX+ response is CAMCmdPending message.
 *
 * @param       slotId   CAM slot id.
 *
 * @return      TRUE if response is CAMCmdPending, otherwise FALSE.
 */
BOOL responseCamCmdPending(uint8 slotId) {
    uint8 camStatus = CIMAX_CAM_A_CMD_PENDING;

    /* Check if response is valid. */
    if (slotId != CIMAX_CAM_SLOT_A_ID) {
        camStatus = CIMAX_CAM_B_CMD_PENDING;
    }

    if ((cimaxContext_g.responseBuff[CIMAX_CAM_COMMAND_OR_STATUS_POS] == camStatus) &&
            (cimaxContext_g.responseBuff[CIMAX_CAM_PACKET_COUNTER_POS] == CIMAX_CAM_PACKET_COUNTER_VALUE) &&
            (cimaxContext_g.responseBuff[CIMAX_CAM_DATA_LEN_MSB_POS] == 0x00) &&
            (cimaxContext_g.responseBuff[CIMAX_CAM_DATA_LEN_LSB_POS] == 0x01)) {
        return TRUE;
    }
    return FALSE;
}

void printData(char* functionName, uint8 *buffer, int32 numOfBytes) {
    int32 i = 0;

    printf("\n%s[%i]: ", functionName, numOfBytes);
    for (i = 0; i < numOfBytes; i++) {
        printf("%02x ", buffer[i]);
        fflush(stdout);
    }

    printf("\n");
}

/**
 * @ingroup     CIMaX_Statics
 *
 * @brief       Waits for CIMaX+ INT pin to be active and reads response.
 *
 * @param       timeout     Timeout in miliseconds to wait for CIMaX+ response.
 *
 * @return      CIMAX_NO_ERROR if successfull, error code if failure.
 */
int32 getResponse(uint32 timeout) {
    int32 numOfBytesRead = 0;
    uint32 count = 0;
    int32 retVal = CIMAX_ERROR;
    uint16 dataLen = 0;

    timeout = timeout / CIMAX_CHECK_INT_PERIOD_MS;

    galois_gpio_data_t gpio_data;
    galois_gpio_data_t * pgpio_data = &gpio_data;

    gpio_data.port = 0; // gpio 0
    gpio_data.mode = 1;
    gpio_data.data = 1;


    do {
        gpio_read(pgpio_data);
        OS_Sleep(CIMAX_CHECK_INT_PERIOD_MS);

        count++;
        if (count > timeout) {
            return CIMAX_TIMEOUT_ERROR;
            printf("Timeout error\n");
        }

        /*
                if (pgpio_data->data == 1) {
                    printf("Cimax: waiting for gpio[0] (interrupt pin)\n");
                }
         */

    } while (pgpio_data->data == 1);


    /* Get response header. */
    numOfBytesRead = CIMAX_SPI_Read(cimaxContext_g.responseBuff, CIMAX_REGISTER_HEADER_SIZE);

    //printData("HEADER", cimaxContext_g.responseBuff, numOfBytesRead);


    /* Check if all bytes of response header are read. */
    if (numOfBytesRead != CIMAX_REGISTER_HEADER_SIZE) {
        printf("%s: header reading error!\n", __FUNCTION__);
        return CIMAX_SPI_COMM_ERROR;
    }

    /* Parse header. */
    switch (cimaxContext_g.responseBuff[CIMAX_REGISTER_CMD_POS] & CIMAX_CAM_TYP_CMD_MASK) {
        case CIMAX_REGISTER_INIT_RESP_OK: /* Fall through. */
        case CIMAX_REGISTER_INIT_RESP_NOK: /* Fall through. */
        case CIMAX_REGISTER_WRITE_RESP_OK: /* Fall through. */
        case CIMAX_REGISTER_WRITE_RESP_NOK: /* Fall through. */
        case CIMAX_REGISTER_READ_RESP_OK: /* Fall through. */
        case CIMAX_REGISTER_READ_RESP_NOK: /* Fall through. */
        case CIMAX_REGISTER_CMD_PARSE_ERROR:

            /* Read register response payload if needed. */
            if (cimaxContext_g.responseBuff[CIMAX_REGISTER_NUM_OF_REGS_POS] > 0) {
                numOfBytesRead += CIMAX_SPI_Read(&cimaxContext_g.responseBuff[CIMAX_REGISTER_HEADER_SIZE], cimaxContext_g.responseBuff[CIMAX_REGISTER_NUM_OF_REGS_POS]);

                /* Check if all bytes of response are read. */
                if (numOfBytesRead != CIMAX_REGISTER_HEADER_SIZE + cimaxContext_g.responseBuff[CIMAX_REGISTER_NUM_OF_REGS_POS]) {
                    printf("%s: register response payload reading error!\n", __FUNCTION__);
                    return CIMAX_SPI_COMM_ERROR;
                }
            }
            retVal = numOfBytesRead;
            break;
        case CIMAX_CAM_A_RESET_OK: /* or CIMAX_CAM_B_RESET_OK */
        case CIMAX_CAM_A_GET_CIS_OK: /* or CIMAX_CAM_B_GET_CIS_OK */
        case CIMAX_CAM_A_WRITE_COR_OK: /* or CIMAX_CAM_B_WRITE_COR_OK */
        case CIMAX_CAM_A_NEGOTIATE_OK: /* or CIMAX_CAM_B_NEGOTIATE_OK */
        case CIMAX_CAM_A_WRITE_LPDU_OK: /* or CIMAX_CAM_B_WRITE_LPDU_OK */
        case CIMAX_CAM_A_READ_LPDU_OK: /* or CIMAX_CAM_B_READ_LPDU_OK */
        case CIMAX_CAM_A_NO_CAM: /* or CIMAX_CAM_B_NO_CAM */
        case CIMAX_CAM_A_CAM_ERROR: /* or CIMAX_CAM_B_CAM_ERROR */
        case CIMAX_CAM_A_WRITE_BUSY: /* or CIMAX_CAM_B_WRITE_BUSY */
        case CIMAX_CAM_A_DET_OK: /* or CIMAX_CAM_B_DET_OK */
        case CIMAX_CAM_A_DATA_READY: /* or CIMAX_CAM_B_DATA_READY */
        case CIMAX_CAM_A_CMD_PENDING: /* or CIMAX_CAM_B_CMD_PENDING */
        case CIMAX_CAM_A_REG_STATUS_OK: /* or CIMAX_CAM_B_REG_STATUS_OK */
        case CIMAX_CAM_NO_EVT:
            /* Get data length for CAM response. */
            dataLen = cimaxContext_g.responseBuff[CIMAX_CAM_DATA_LEN_MSB_POS];
            dataLen = (dataLen << 8);
            dataLen = dataLen | cimaxContext_g.responseBuff[CIMAX_CAM_DATA_LEN_LSB_POS];

            /* Read CAM response payload if needed. */
            if (dataLen > 0) {
                numOfBytesRead += CIMAX_SPI_Read(&cimaxContext_g.responseBuff[CIMAX_CAM_HEADER_SIZE], dataLen);
                /* Check if all bytes of response are read. */
                if (numOfBytesRead != CIMAX_REGISTER_HEADER_SIZE + dataLen) {
                    printf("%s: CAM response payload reading error!\n", __FUNCTION__);
                    return CIMAX_SPI_COMM_ERROR;
                }
            }
            retVal = numOfBytesRead;
            break;

            //case CIMAX_CAM_A_DET_OK:            /* or CIMAX_CAM_B_DET_OK */
            //case CIMAX_CAM_A_DATA_READY:        /* or CIMAX_CAM_B_DATA_READY */
            //case CIMAX_CAM_NO_EVT:
            //   retVal = numOfBytesRead;
            //   break;   
        default:
            printf("%s: ***** invalid response!:0x%02x 0x%02x 0x%02x 0x%02x. *****\n",
                    __FUNCTION__, cimaxContext_g.responseBuff[CIMAX_CAM_COMMAND_OR_STATUS_POS],
                    cimaxContext_g.responseBuff[CIMAX_CAM_PACKET_COUNTER_POS],
                    cimaxContext_g.responseBuff[CIMAX_CAM_DATA_LEN_MSB_POS],
                    cimaxContext_g.responseBuff[CIMAX_CAM_DATA_LEN_LSB_POS]);
            return CIMAX_ERROR;
            break;
    } /* switch (responseBuff[CIMAX_REGISTER_CMD_POS] & CIMAX_CAM_TYP_CMD_MASK) */

    return retVal;
}

int32 CIMAX_ResetPatch() {
    printf("Reset Cimax FIFO buffers\n");

    uint32 cnt;
    uint8 buf[CIMAX_REGISTER_MAX_PAYLOAD_SIZE];

    printf("Download CIMaX+ configuration(register settings): ");

    for (cnt = 0; cnt < sizeof (spiRegSettingsReset) / sizeof (regSettings_t); cnt++) {
        switch (spiRegSettingsReset[cnt].op) {
            case REG_OP_WRITE:
                /* Write register. */
                if (CIMAX_WriteRegister(spiRegSettingsReset[cnt].reg, (uint8 *) & spiRegSettingsReset[cnt].val, 1) < 0) {
                    /* CIMaX+ write error. */
                    printf("FAILED at REG_OP_WRITE operation.\n");
                    return CIMAX_ERROR;
                }
                //printf("Write operation: register 0x%04x = 0x%02x.\n", spiRegSettings[cnt].reg, spiRegSettings[cnt].val);
                break;
            case REG_OP_WAIT:
                //printf("Wait for %d ms.\n", spiRegSettings[cnt].val);
                OS_Sleep(spiRegSettingsReset[cnt].val);
                break;
            default:
                printf("\nInvalid operation 0x%02x!\n", spiRegSettingsReset[cnt].op);
                return CIMAX_ERROR;
                break;
        }
    }
    printf("OK.\n");
    return CIMAX_NO_ERROR;



#if 0
    unsigned int cnt;
    unsigned char buf[CIMAX_REGISTER_MAX_PAYLOAD_SIZE];
    unsigned char txBuffer[5];
    unsigned char rxBuffer[4];
    int receivedLength = 0;

    printf("Download CIMaX+ configuration(register settings): ");


    CimaxSpiProtect(1);


    for (cnt = 0; cnt < sizeof (spiRegSettingsReset) / sizeof (regSettings_t); cnt++) {

        printf("spiRegSettings[%d].op = %0X, val = 0x%0X\n", cnt, spiRegSettingsReset[cnt].op, (unsigned char *) spiRegSettingsReset[cnt].val);

        txBuffer[0] = CIMAX_REGISTER_WRITE_REQ;
        txBuffer[1] = spiRegSettingsReset[cnt].reg >> 8 & 0xFF;
        txBuffer[2] = spiRegSettingsReset[cnt].reg & 0xFF;
        txBuffer[3] = 0x01;
        txBuffer[4] = (unsigned char *) spiRegSettingsReset[cnt].val;

        switch (spiRegSettingsReset[cnt].op) {
            case REG_OP_WRITE:

                CimaxUsbCmdWriteUnprotected(NULL, txBuffer, 5, &rxBuffer, &receivedLength);

                if ((receivedLength != 4) || (rxBuffer[0] != 0x4D)) {
                    printf("FAILED at REG_OP_WRITE operation.\n");
                    printf("Error receiving response while cimax reset\n");
                    CimaxSpiProtect(0);
                    return S_FALSE;
                }
                break;

            case REG_OP_WAIT:
                //printf("Wait for %d ms.\n", spiRegSettings[cnt].val);
                usleep(spiRegSettingsReset[cnt].val);
                break;

            default:
                printf("\nInvalid operation 0x%02x!\n", spiRegSettingsReset[cnt].op);
                CimaxSpiProtect(0);
                return S_FALSE;
                break;
        }
    }

    CimaxSpiProtect(0);

    printf("OK.\n");
    return 0;

#endif    

}

