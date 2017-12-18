#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <malloc.h>
#include <stdlib.h>


#include "spi_bus.h"

#define MV_SPI_PERR(fmt, ...) printf("[%s %d] E: "fmt"\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define MV_SPI_PINF(fmt, ...) printf("[%s %d] I: "fmt"\n", __FILE__, __LINE__, ##__VA_ARGS__)

#define MV_SPI_MASTERID			1		// SPI Master ID
#define MV_SPI_SLAVEID			2		// SPI Slave ID


#define MV_SPI_START_ADDR 		0x0		// SPI Flash Start Address
#define MV_SPI_HEADER_ADDR 		(MV_SPI_START_ADDR + 0x400000)	// 4M-bytes Offset for Image Header Adderss
#define MV_SPI_MAX_ADDR			0xFFFFFF	// 16MB

#define MV_SPI_FLASH_BLKSIZE	0x10000		// 64K-bytes Block Size
#define MV_SPI_FLASH_SECSIZE	0x1000		// 4K-bytes Sector Size
#define MV_SPI_FLASH_PGSIZE		128		// 128B	Page Size

#define MV_SPI_DEVICE_NAME		"/dev/spi1"
#define MV_SPI_BACKUP_FILE		"/home/galois_rwdata/SPI_Flash_Image_Backup"	// Backup File Path

// Instructions For SPI Flash 
#define MV_SPI_FLASH_WREN		0x06
#define MV_SPI_FLASH_WRDI		0x04
#define MV_SPI_FLASH_RDSR		0x05
#define MV_SPI_FLASH_WRSR		0x01
#define MV_SPI_FLASH_READ		0x03
#define MV_SPI_FLASH_FREAD		0x0B
#define MV_SPI_FLASH_PP			0x02
#define MV_SPI_FLASH_SE			0xD8
#define MV_SPI_FLASH_BE			0xC7
#define MV_SPI_FLASH_DP			0xB9
#define MV_SPI_FLASH_RES		0xAB
#define MV_SPI_FLASH_RDID		0x9F
// Masks For Flash Status
#define MV_SPI_FLASH_WIP		0x01


#define S_OK 		0x0000
#define S_FALSE 	0x0001

/**
 * @defgroup CIMaX_Register_Map CIMaX+ Register Map
 * @{
 */
#define BUFFIN_CFG                        0x0000
#define BUFFIN_ADDR_LSB                   0x0001
#define BUFFIN_ADDR_MSB                   0x0002
#define BUFFIN_DATA                       0x0003
#define BUFFOUT_CFG                       0x0004
#define BUFFOUT_ADDR_LSB                  0x0005
#define BUFFOUT_ADDR_MSB                  0x0006
#define BUFFOUT_DATA                      0x0007
#define BOOT_Key                          0x0008   
#define BOOT_Status                       0x0009   
#define BOOT_Test                         0x000A   
#define usb2_0_irq_mask                   0x0010   
#define usb2_0_status                     0x0011   
#define usb2_0_rx                         0x0012   
#define usb2_0_tx                         0x0013   
#define SPI_Slave_Ctrl                    0x0018   
#define SPI_Slave_Status                  0x0019   
#define SPI_Slave_Rx                      0x001A   
#define SPI_Slave_Tx                      0x001B   
#define SPI_Slave_Mask                    0x001C   
#define UCSG_Ctrl                         0x0020   
#define UCSG_Status                       0x0021   
#define UCSG_RxData                       0x0022   
#define UCSG_TxData                       0x0023   
#define PCtrl_Ctrl                        0x0028   
#define PCtrl_Status                      0x0029   
#define PCtrl_NbByte_LSB                  0x002A   
#define PCtrl_NbByte_MSB                  0x002B   
#define SPI_Master_Ctl                    0x0030   
#define SPI_Master_NCS                    0x0031   
#define SPI_Master_Status                 0x0032   
#define SPI_Master_TxBuf                  0x0033   
#define SPI_Master_RxBuf                  0x0034   
#define BISTRAM_Ctl                       0x0038   
#define BISTRAM_Bank                      0x0039   
#define BISTRAM_Pat                       0x003A   
#define BISTRAM_SM                        0x003B   
#define BISTRAM_AddrLSB                   0x003C   
#define BISTROM_Config                    0x0040   
#define BISTROM_SignatureLSB              0x0041   
#define BISTROM_SignatureMSB              0x0042   
#define BISTROM_StartAddrLSB              0x0043   
#define BISTROM_StartAddrMSB              0x0043   
#define BISTROM_StopAddrLSB               0x0043   
#define BISTROM_StopAddrMSB               0x0043   
#define CkMan_Config                      0x0048   
#define CkMan_Select                      0x0049   
#define CkMan_Test                        0x004A   
#define Revision_Number                   0x004B   
#define ResMan_Config                     0x0050   
#define ResMan_Status                     0x0051   
#define ResMan_WD                         0x0052   
#define ResMan_WD_MSB                     0x0053   
#define CPU_Test                          0x0060   
#define IrqMan_Config0                    0x0068   
#define IrqMan_Config1                    0x0069   
#define IrqMan_Irq0                       0x006A   
#define IrqMan_NMI                        0x006B   
#define IrqMan_SleepKey                   0x006C   
#define Tim_Config                        0x0070   
#define Tim_Value_LSB                     0x0071   
#define Tim_Value_MSB                     0x0072   
#define Tim_Comp_LSB                      0x0073
#define Tim_Comp_MSB                      0x0074
#define TI_Config                         0x0076
#define TI_Data                           0x0077
#define TI_Reg0                           0x0078
#define TI_Reg1                           0x0079
#define TI_Reg2                           0x007A
#define TI_Reg3                           0x007B
#define TI_Reg4                           0x007C
#define TI_ROM1                           0x007D
#define TI_ROM2                           0x007E
#define TI_ROM3                           0x007F
#define DVBCI_START_ADDR                  0x0100
#define DVBCI_END_ADDR                    0x017F
#define DATA                              0x0180
//#define CTRL                              0x0181
#define QB_HOST                           0x0182
#define LEN_HOST_LSB                      0x0183
#define LEN_HOST_MSB                      0x0184
#define FIFO_TX_TH_LSB                    0x0185
#define FIFO_TX_TH_MSB                    0x0186
#define FIFO_TX_D_NB_LSB                  0x0187
#define FIFO_TX_D_NB_MSB                  0x0188
#define QB_MOD_CURR                       0x0189
#define LEN_MOD_CURR_LSB                  0x018A
#define LEN_MOD_CURR_MSB                  0x018B
#define QB_MOD                            0x018C
#define LEN_MOD_LSB                       0x018D
#define LEN_MOD_MSB                       0x018E
#define FIFO_RX_TH_LSB                    0x018F
#define FIFO_RX_TH_MSB                    0x0190
#define FIFO_RX_D_NB_LSB                  0x0191
#define FIFO_RX_D_NB_MSB                  0x0192
#define IT_STATUS_0                       0x0193
#define IT_STATUS_1                       0x0194
#define IT_MASK_0                         0x0195
#define IT_MASK_1                         0x0196
#define IT_HOST_PIN_CFG                   0x0200
#define CFG_0                             0x0201
#define CFG_1                             0x0202
#define CFG_2                             0x0203
#define IT_HOST                           0x0204
#define MOD_IT_STATUS                     0x0205
#define MOD_IT_MASK                       0x0206
#define MOD_CTRL_A                        0x0207
#define MOD_CTRL_B                        0x0208
#define DEST_SEL                          0x0209
#define CAM_MSB_ADD                       0x020A
#define GPIO0_DIR                         0x020B
#define GPIO0_DATA_IN                     0x020C
#define GPIO0_DATA_OUT                    0x020D
#define GPIO0_STATUS                      0x020E
#define GPIO0_IT_MASK                     0x020F
#define GPIO0_DFT                         0x0210
#define GPIO0_MASK_DATA                   0x0211
#define GPIO1_DIR                         0x0212
#define GPIO1_DATA_IN                     0x0213
#define GPIO1_DATA_OUT                    0x0214
#define GPIO1_STATUS                      0x0215
#define GPIO1_IT_MASK                     0x0216
#define MEM_ACC_TIME_A                    0x0217
#define MEM_ACC_TIME_B                    0x0218
#define IO_ACC_TIME_A                     0x0219
#define IO_ACC_TIME_B                     0x021A
#define EXT_CH_ACC_TIME_A                 0x021B
#define EXT_CH_ACC_TIME_B                 0x021C
#define PAR_IF_0                          0x021D
#define PAR_IF_1                          0x021E
#define PAR_IF_CTRL                       0x021F
#define PCK_LENGTH                        0x0220
#define USB2TS_CTRL                       0x0221
#define USB2TS0_RDL                       0x0222
#define USB2TS1_RDL                       0x0223
#define TS2USB_CTRL                       0x0224
#define TSOUT_PAR_CTRL                    0x0225
#define TSOUT_PAR_CLK_SEL                 0x0226
#define S2P_CH0_CTRL                      0x0227
#define S2P_CH1_CTRL                      0x0228
#define P2S_CH0_CTRL                      0x0229
#define P2S_CH1_CTRL                      0x022A
#define TS_IT_STATUS                      0x022B
#define TS_IT_MASK                        0x022C
#define IN_SEL                            0x022D
#define OUT_SEL                           0x022E
#define ROUTER_CAM_CH                     0x022F
#define ROUTER_CAM_MOD                    0x0230
#define FIFO_CTRL                         0x0231
#define FIFO1_2_STATUS                    0x0232
#define FIFO3_4_STATUS                    0x0233
#define GAP_REMOVER_CH0_CTRL              0x0234
#define GAP_REMOVER_CH1_CTRL              0x0235
#define SYNC_RTV_CTRL                     0x0236
#define SYNC_RTV_CH0_SYNC_NB              0x0237
#define SYNC_RTV_CH0_PATTERN              0x0238
#define SYNC_RTV_CH1_SYNC_NB              0x0239
#define SYNC_RTV_CH1_PATTERN              0x023A
#define SYNC_RTV_OFFSET_PATT              0x023B
#define CTRL_FILTER                       0x023D
#define PID_EN_FILTER_CH0                 0x023E
#define PID_EN_FILTER_CH1                 0x023F
#define PID_LSB_FILTER_CH0_0              0x0240
#define PID_MSB_FILTER_CH0_0              0x0241
#define PID_LSB_FILTER_CH0_1              0x0242
#define PID_MSB_FILTER_CH0_1              0x0243
#define PID_LSB_FILTER_CH0_2              0x0244
#define PID_MSB_FILTER_CH0_2              0x0245
#define PID_LSB_FILTER_CH0_3              0x0246
#define PID_MSB_FILTER_CH0_3              0x0247
#define PID_LSB_FILTER_CH0_4              0x0248
#define PID_MSB_FILTER_CH0_4              0x0249
#define PID_LSB_FILTER_CH0_5              0x024A
#define PID_MSB_FILTER_CH0_5              0x024B
#define PID_LSB_FILTER_CH0_6              0x024C
#define PID_MSB_FILTER_CH0_6              0x024D
#define PID_LSB_FILTER_CH0_7              0x024E
#define PID_MSB_FILTER_CH0_7              0x024F
#define PID_LSB_FILTER_CH1_0              0x0260
#define PID_MSB_FILTER_CH1_0              0x0261
#define PID_LSB_FILTER_CH1_1              0x0262
#define PID_MSB_FILTER_CH1_1              0x0263
#define PID_LSB_FILTER_CH1_2              0x0264
#define PID_MSB_FILTER_CH1_2              0x0265
#define PID_LSB_FILTER_CH1_3              0x0266
#define PID_MSB_FILTER_CH1_3              0x0267
#define PID_LSB_FILTER_CH1_4              0x0268
#define PID_MSB_FILTER_CH1_4              0x0269
#define PID_LSB_FILTER_CH1_5              0x026A
#define PID_MSB_FILTER_CH1_5              0x026B
#define PID_LSB_FILTER_CH1_6              0x026C
#define PID_MSB_FILTER_CH1_6              0x026D
#define PID_LSB_FILTER_CH1_7              0x026E
#define PID_MSB_FILTER_CH1_7              0x026F
#define PID_OLD_LSB_REMAPPER_0            0x0280
#define PID_OLD_MSB_REMAPPER_0            0x0281
#define PID_OLD_LSB_REMAPPER_1            0x0282
#define PID_OLD_MSB_REMAPPER_1            0x0283
#define PID_OLD_LSB_REMAPPER_2            0x0284
#define PID_OLD_MSB_REMAPPER_2            0x0285
#define PID_OLD_LSB_REMAPPER_3            0x0286
#define PID_OLD_MSB_REMAPPER_3            0x0287
#define PID_OLD_LSB_REMAPPER_4            0x0288
#define PID_OLD_MSB_REMAPPER_4            0x0289
#define PID_OLD_LSB_REMAPPER_5            0x028A
#define PID_OLD_MSB_REMAPPER_5            0x028B
#define PID_OLD_LSB_REMAPPER_6            0x028C
#define PID_OLD_MSB_REMAPPER_6            0x028D
#define PID_OLD_LSB_REMAPPER_7            0x028E
#define PID_OLD_MSB_REMAPPER_7            0x028F
#define PID_NEW_LSB_REMAPPER_0            0x02A0
#define PID_NEW_MSB_REMAPPER_0            0x02A1
#define PID_NEW_LSB_REMAPPER_1            0x02A2
#define PID_NEW_MSB_REMAPPER_1            0x02A3
#define PID_NEW_LSB_REMAPPER_2            0x02A4
#define PID_NEW_MSB_REMAPPER_2            0x02A5
#define PID_NEW_LSB_REMAPPER_3            0x02A6
#define PID_NEW_MSB_REMAPPER_3            0x02A7
#define PID_NEW_LSB_REMAPPER_4            0x02A8
#define PID_NEW_MSB_REMAPPER_4            0x02A9
#define PID_NEW_LSB_REMAPPER_5            0x02AA
#define PID_NEW_MSB_REMAPPER_5            0x02AB
#define PID_NEW_LSB_REMAPPER_6            0x02AC
#define PID_NEW_MSB_REMAPPER_6            0x02AD
#define PID_NEW_LSB_REMAPPER_7            0x02AE
#define PID_NEW_MSB_REMAPPER_7            0x02AF
#define MERGER_DIV_MICLK                  0x02C0
#define PID_AND_SYNC_REMAPPER_CTRL        0x02C1
#define PID_EN_REMAPPER                   0x02C2
#define SYNC_SYMBOL                       0x02C3
#define PID_AND_SYNC_REMAPPER_INV_CTRL    0x02C4
#define BITRATE_CH0_LSB                   0x02C5
#define BITRATE_CH0_MSB                   0x02C6
#define BITRATE_CH1_LSB                   0x02C7
#define BITRATE_CH1_MSB                   0x02C8
#define STATUS_CLK_SWITCH_0               0x02C9
#define STATUS_CLK_SWITCH_1               0x02CA
#define RESET_CLK_SWITCH_0                0x02CB
#define RESET_CLK_SWITCH_1                0x02CC
#define PAD_DRVSTR_CTRL                   0x02CD
#define PAD_PUPD_CTRL                     0x02CE
#define PRE_HEADER_ADDER_CH0_0            0x02D0
#define PRE_HEADER_ADDER_CH0_1            0x02D1
#define PRE_HEADER_ADDER_CH0_2            0x02D2
#define PRE_HEADER_ADDER_CH0_3            0x02D3
#define PRE_HEADER_ADDER_CH0_4            0x02D4
#define PRE_HEADER_ADDER_CH0_5            0x02D5
#define PRE_HEADER_ADDER_CH0_6            0x02D6
#define PRE_HEADER_ADDER_CH0_7            0x02D7
#define PRE_HEADER_ADDER_CH0_8            0x02D8
#define PRE_HEADER_ADDER_CH0_9            0x02D9
#define PRE_HEADER_ADDER_CH0_10           0x02DA
#define PRE_HEADER_ADDER_CH0_11           0x02DB
#define PRE_HEADER_ADDER_CH1_0            0x02E0
#define PRE_HEADER_ADDER_CH1_1            0x02E1
#define PRE_HEADER_ADDER_CH1_2            0x02E2
#define PRE_HEADER_ADDER_CH1_3            0x02E3
#define PRE_HEADER_ADDER_CH1_4            0x02E4
#define PRE_HEADER_ADDER_CH1_5            0x02E5
#define PRE_HEADER_ADDER_CH1_6            0x02E6
#define PRE_HEADER_ADDER_CH1_7            0x02E7
#define PRE_HEADER_ADDER_CH1_8            0x02E8
#define PRE_HEADER_ADDER_CH1_9            0x02E9
#define PRE_HEADER_ADDER_CH1_10           0x02EA
#define PRE_HEADER_ADDER_CH1_11           0x02EB
#define PRE_HEADER_ADDER_CTRL             0x02EC
#define PRE_HEADER_ADDER_LEN              0x02ED
#define PRE_HEADER_REMOVER_CTRL           0x02EE
#define FSM_DVB                           0x02F0
#define TS2USB_FSM_DEBUG                  0x02F2
#define TSOUT_PAR_FSM_DEBUG               0x02F3
#define GAP_REMOVER_FSM_DEBUG             0x02F4
#define PID_AND_SYNC_REMAPPER_FSM_DEBUG   0x02F5
#define PRE_HEADER_ADDER_FSM_DEBUG        0x02F6
#define SYNC_RTV_FSM_DEBUG                0x02F7
#define CHECK_PHY_CLK                     0x0E00
#define USB_CTRL1                         0x0E01
#define USB_ISO2_out                      0x0800
#define USB_ISO1_out                      0x1000
#define USB_Interrupt_out                 0x1E00
#define USB_Bulk_in                       0x1F00
#define CC2_Buffer_out                    0x2000
#define USB_EP0                           0x30C0
#define CC2_Buffer_in                     0x4000                        
#define USB_ISO2_in                       0x5800
#define USB_ISO1_in                       0x6000
#define nmb_vector_address_lsb            0xFFFA
#define nmb_vector_address_msb            0xFFFB
#define reset_vector_address_lsb          0xFFFC
#define reset_vector_address_msb          0xFFFD
#define irb_vector_address_lsb            0xFFFE
#define irb_vector_address_msb            0xFFFF


/* Gpio settings*/

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

extern char *optarg;
extern int optopt;


#define PRINT_ENABLE 0

#define BISTROM_SignatureLSB              0x0041   
#define BOOT_Status                       0x0009   


/*
 * 
 */
#define FW_START_ADDRESS                  0x8000

/** CIMaX+ register init request. */
#define CIMAX_REGISTER_INIT_REQ           0x00
/** CIMaX+ register write request. */
#define CIMAX_REGISTER_WRITE_REQ          0x7F
/** CIMaX+ register read request. */
#define CIMAX_REGISTER_READ_REQ           0xFF

/** CIMaX+ register maximum payload size. */
#define CIMAX_REGISTER_MAX_PAYLOAD_SIZE   255



unsigned int FWSign = 0;

int spi_fd;

/* GPIO */
static FILE *spFile;
int sFd;



typedef struct galois_gpio_data {
        int port;       /* port number: for SoC, 0~31; for SM, 0~11 */
        int mode;       /* port mode: data(in/out) or irq(level/edge) */
        unsigned int data;      /* port data: 1 or 0, only valid when mode is data(in/out) */
} galois_gpio_data_t;


/*
 * SPI Image Header, 64 Bytes
 */
struct MV_SPI_Image_Header {
    char image_md5[16]; // 128-bit MD5 Image Checksum
    char header_md5[16]; // 128-bit MD5 Header Checksum
    int image_len; // Image Body Length
    char reserve[28]; // Reserve 28 Bytes
};

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

typedef struct {
    /** CIMaX+ register address. */
    int reg;
    /** CIMaX+ register value. */
    int val;
    /** CIMaX+ register operation. */
    regOperation_t op;
} regSettings_t;


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

    {CkMan_Select, 0x15, REG_OP_WRITE}, //CkMan_Select Select 108MHz Serial 0 and Serial 1 clock.
    {CkMan_Config, 0x6F, REG_OP_WRITE}, //CkMan_Config Config Serial 0 and Serial 1 clock enable.

    {TS2USB_CTRL, 0x00, REG_OP_WRITE},

    {S2P_CH0_CTRL, 0x00, REG_OP_WRITE},
    {S2P_CH1_CTRL, 0x22, REG_OP_WRITE},

    {P2S_CH1_CTRL, 0x18, REG_OP_WRITE},

    {0x0000, 200, REG_OP_WAIT}, /** Wait 200 miliseconds. */

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

    /* Set in/out */
    {OUT_SEL, 0x04, REG_OP_WRITE}, //OUT_SEL CH1 => Serial 1 Out
    {IN_SEL, 0x01, REG_OP_WRITE}, //IN_SEL Parallel In => CH0
};


int gpio_open(galois_gpio_data_t gpio_data)
{
        spFile = fopen(GPIO_DEVICE, "r+");
        if (!spFile) {
        perror("Failed to open "GPIO_DEVICE".\n");
        return -1;
        }

        sFd = fileno(spFile);
        //printf("\n[gpio]: Init\n");

        if (ioctl(sFd, GPIO_IOCTL_SET, &gpio_data)) {
                perror("ioctl GPIO_IOCTL_SET error.\n");
                fclose(spFile);
                return -1;
        }
        /* close */
    //  fclose(file);
        return 0;
}
int gpio_write(galois_gpio_data_t gpio_data)
{

        if (ioctl(sFd, GPIO_IOCTL_WRITE, &gpio_data)) {
                perror("ioctl GPIO_IOCTL_WRITE error.\n");
        fclose(spFile);
        return -1;
        } else {
                printf("\tport %d : %d\n", gpio_data.port, gpio_data.data);
        }

        /* close */
        fclose(spFile);
        return 0;
}

int set_gpio(int port_no,int val)
{
        galois_gpio_data_t gpio_data;
        galois_gpio_data_t * pgpio_data=&gpio_data;
        gpio_data.port = port_no ;
        gpio_data.mode = 2;
        gpio_data.data = val;
        gpio_open(gpio_data);
        gpio_write(gpio_data);
        return 0;
}


/* PINMUX */

#define PINMUX_IOCTL_READ           0x5100
#define PINMUX_IOCTL_WRITE          0x5101
#define PINMUX_IOCTL_SETMOD         0x5102
#define PINMUX_IOCTL_GETMOD         0x5103
#define PINMUX_IOCTL_PRINTSTATUS    0x5104

#define SOC_PINMUX                  0
#define SM_PINMUX                   1

#define PINMUX_DEVICE "/dev/pinmux"

typedef struct galois_pinmux_data{
    int owner;              //  soc or sm pinmux
    int group;              //  group id
    int value;              //  group value
    char * requster;        //  who request the change
}galois_pinmux_data_t;

int pinmux_devopen(void)
{
        int pinmuxfd;

        pinmuxfd = open("/dev/pinmux", O_RDWR);
        if(pinmuxfd <= -1)
        {
                printf("open /dev/pinmux error\n");
        }
        return pinmuxfd;
}

int pinmux_ioctl(int pinmuxfd, int ioctlcode, galois_pinmux_data_t *pin)
{
        int ret = 0;

        ret = ioctl(pinmuxfd, ioctlcode, pin);
        if(ret > 0)
        {
                printf("ioctl error, ret = 0x%x\r\n", ret);
                return -1;
        }
        return ret;
}

void close_pinmux(int pinmuxfd)
{
        close(pinmuxfd);
}

int set_pinmux(int group, int value)
{
        int fd;
        struct galois_pinmux_data pin;
        char my[] = "test";
        int ret = 0;

        fd = pinmux_devopen();
        if(fd < 0)
                return fd;;
        pin.owner = SOC_PINMUX ;
        pin.group = group;//SM_GROUP7;
        pin.value = value;
        pin.requster = my;

        ret = pinmux_ioctl(fd, PINMUX_IOCTL_SETMOD, &pin);
        close_pinmux(fd);
        return ret;
}

int get_pinmux(int group, int *Value)
{
        int fd;
        struct galois_pinmux_data pin;
        char my[] = "test";
        int ret = 0;

        fd = pinmux_devopen();
        if(fd < 0)
                return fd;;
        pin.owner = SOC_PINMUX ;
        pin.group = group;//SM_GROUP7;
        pin.value = -1;
        pin.requster = my;

        ret = pinmux_ioctl(fd, PINMUX_IOCTL_GETMOD, &pin);
        *Value = pin.value;

        close_pinmux(fd);
        return ret;
}



int MV_SPI_Flash_WaitForWrite() {
    usleep(20000);
    return S_OK;
}

int MV_SPI_Flash_Init(int spi_fd) {
    galois_spi_info_t spi_info;
    int error;

    if (spi_fd < 0) {
        MV_SPI_PERR("SPI Device File Description is Invalid!");
        return S_FALSE;
    }

    spi_info.mst_id = MV_SPI_MASTERID;
    spi_info.slave_id = MV_SPI_SLAVEID;
    spi_info.xd_mode = SPIDEV_XDMODE2_WOA8D8n;

    spi_info.clock_mode = SPI_CLOCKMODE0_POL0PH0;

    spi_info.speed = 1000;
    spi_info.frame_width = SPI_FRAME_WIDTH8;

    error = ioctl(spi_fd, SPI_IOCTL_SETINFO, &spi_info);
    if (error != 0)
        return S_FALSE;

    return S_OK;
}

int MV_SPI_Get_Info(int spi_fd, galois_spi_info_t *spi_info) {
    int error;

    if (spi_fd < 0) {
        MV_SPI_PERR("SPI Device File Description is Invalid!");
        return S_FALSE;
    }

    error = ioctl(spi_fd, SPI_IOCTL_GETINFO, &spi_info);
    if (error != 0)
        return S_FALSE;

    return S_OK;
}

int MV_SPI_Print_Info(const galois_spi_info_t *spi_info) {
    printf("[SPI Information]:\n");
    printf("Master ID: %d, Slave Address: 0x%x\n",
            spi_info->mst_id, spi_info->slave_id);
    printf("Clock Mode: %d, XD Mode: %d\n",
            spi_info->clock_mode, spi_info->xd_mode);
    printf("Speed: %d, Frame Width: %d, IRQ: %d, Usable: %d\n",
            spi_info->speed, spi_info->frame_width, spi_info->irq, spi_info->usable);
    return S_OK;
}


int initFW(int spi_fd) {
    galois_spi_rw_t spi_rw;
    int count, idx, error;
    unsigned int *buffer = NULL;
    unsigned int read_buf[4];

    unsigned int bufferOut[4];

    memset(bufferOut, 0, sizeof (bufferOut));


    spi_rw.mst_id = MV_SPI_MASTERID;
    spi_rw.slave_id = MV_SPI_SLAVEID;

    spi_rw.wr_buf = bufferOut;
    spi_rw.wr_cnt = 4;
    spi_rw.rd_buf = NULL;
    spi_rw.rd_cnt = 0;

    error = ioctl(spi_fd, SPI_IOCTL_READWRITE, &spi_rw);
    if (0 != error) {
        free(buffer);
        return S_FALSE;
    }

    usleep(2000);

    spi_rw.mst_id = MV_SPI_MASTERID;
    spi_rw.slave_id = MV_SPI_SLAVEID;

    spi_rw.wr_buf = NULL;
    spi_rw.wr_cnt = 0;
    spi_rw.rd_buf = read_buf;
    spi_rw.rd_cnt = 4;


    error = ioctl(spi_fd, SPI_IOCTL_READWRITE, &spi_rw);
    if (0 != error) {
        free(buffer);
        return S_FALSE;
    }

#if PRINT_ENABLE
    printf("read buf ");
    int i = 0;
    for (i = 0; i < 4; i++) {
        printf("%0x:", read_buf[i]);
    }
    printf("\n");
#endif

    return S_OK;
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
unsigned int computeBistRom(const unsigned char *tab, int size, unsigned int FWSign) {
    int k;
    int i;
    unsigned int mySign;

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




#if 0

static int MV_SPI_Compare_MD5(const char *p1, const char *p2) {
    int md5_compare;

    if (!p1 || !p2) {
        MV_SPI_PERR("MD5 Key is NULL!");
        return S_FALSE;
    }

    md5_compare = strncmp(p1, p2, 16);
    if (md5_compare != 0) {
        MV_SPI_PERR("MD5 Key is NOT Match!");
        return S_FALSE;
    }

    return S_OK;
}

int MV_SPI_Image_Check(const void *upimg_ptr, int length) {
    MD5_CTX md5_ctx;
    char md5[16];
    struct MV_SPI_Image_Header *image_header;
    int md5_equal;
    int header_length = sizeof (struct MV_SPI_Image_Header);

    if (!upimg_ptr || length == 0) {
        MV_SPI_PERR("Update Image's Pointer is NULL or Length is Too Short!");
        return S_FALSE;
    }

    MD5_Init(&md5_ctx);
    MD5_Update(&md5_ctx, upimg_ptr, header_length);
    MD5_Final((unsigned char *) md5, &md5_ctx);

    image_header = (struct MV_SPI_Image_Header *) upimg_ptr;
    md5_equal = MV_SPI_Compare_MD5(md5, image_header->header_md5);
    if (md5_equal != S_OK) {
        MV_SPI_PERR("Update Image's Header MD5 Check Failed!");
        return S_FALSE;
    }

    upimg_ptr = (const char *) upimg_ptr + header_length;

    MD5_Init(&md5_ctx);
    MD5_Update(&md5_ctx, upimg_ptr, (length - header_length));
    MD5_Final((unsigned char *) md5, &md5_ctx);

    md5_equal = MV_SPI_Compare_MD5(md5, image_header->image_md5);
    if (md5_equal != S_OK) {
        MV_SPI_PERR("Update Image's Body MD5 Check Failed!");
        return S_FALSE;
    }

    return S_OK;
}
#endif

int MV_SPI_Flash_ReadStatus(int spi_fd, unsigned int *spi_status) {
    galois_spi_rw_t spi_rw;
    int error;
    unsigned int buffer;

    buffer = MV_SPI_FLASH_RDSR;

    spi_rw.mst_id = MV_SPI_MASTERID;
    spi_rw.slave_id = MV_SPI_SLAVEID;

    spi_rw.wr_buf = &buffer;
    spi_rw.wr_cnt = 1;
    spi_rw.rd_buf = spi_status;
    spi_rw.rd_cnt = 1;
    error = ioctl(spi_fd, SPI_IOCTL_READWRITE, &spi_rw);

    if (error != 0)
        return S_FALSE;

    return S_OK;
}

static int MV_SPI_Flash_ReadPage(int spi_fd, unsigned int read_addr, unsigned char *read_buf, int read_len) {
    galois_spi_rw_t spi_rw;
    int count, idx, error;
    unsigned int command[4];
    unsigned int *buffer;

    count = (read_addr & (MV_SPI_FLASH_PGSIZE - 1)) + read_len;
    if (count > MV_SPI_FLASH_PGSIZE) {
        MV_SPI_PERR("Read Area Exceed One Page Size: 0x%X!", MV_SPI_FLASH_PGSIZE);
        return S_FALSE;
    }

    //command[0] = MV_SPI_FLASH_READ;
    command[0] = CIMAX_REGISTER_READ_REQ;
    //command[1] = read_addr >> 16 & 0xFF;
    command[1] = read_addr >> 8 & 0xFF;
    command[2] = read_addr & 0xFF;
    command[3] = read_len;

    buffer = (unsigned int *) malloc((read_len + 4) * sizeof (int));
    if (!buffer) {
        MV_SPI_PERR("Malloc %d Space for Read Buffer Failed!", read_len * sizeof (int));
        return S_FALSE;
    }
    spi_rw.mst_id = MV_SPI_MASTERID;
    spi_rw.slave_id = MV_SPI_SLAVEID;

    spi_rw.wr_buf = command;
    spi_rw.wr_cnt = 4;
    spi_rw.rd_buf = 0;
    spi_rw.rd_cnt = 0;

    error = ioctl(spi_fd, SPI_IOCTL_READWRITE, &spi_rw);
    if (0 != error) {
        free(buffer);
        return S_FALSE;
    }

    //usleep(10000);
    usleep(5000);

    spi_rw.wr_buf = 0;
    spi_rw.wr_cnt = 0;
    spi_rw.rd_buf = buffer;
    spi_rw.rd_cnt = 4;

    error = ioctl(spi_fd, SPI_IOCTL_READWRITE, &spi_rw);
    if (0 != error) {
        free(buffer);
        return S_FALSE;
    }


#if PRINT_ENABLE
    printf("response ");
    int i = 0;
    for (i = 0; i < 4; i++)
        printf("%0x:", buffer[i]);
    printf("\n");
#endif

    //usleep(10000);
    usleep(5000);

    spi_rw.wr_buf = 0;
    spi_rw.wr_cnt = 0;
    spi_rw.rd_buf = buffer;
    spi_rw.rd_cnt = read_len;

    error = ioctl(spi_fd, SPI_IOCTL_READWRITE, &spi_rw);
    if (0 != error) {
        free(buffer);
        return S_FALSE;
    }




    for (idx = 0; idx < read_len; idx++)
        read_buf[idx] = buffer[idx] & 0xFF;

#if 0
    printf("read from cimax on addr %0X: command[1][2]: %0X %0X\n", read_addr, command[1], command[2]);
    for (int i = 0; i < read_len; i++) {
        printf("%X", read_buf[i]);
    }
    printf("\n");
#endif

    free(buffer);
    return S_OK;
}

int MV_SPI_Flash_WaitEnable(int spi_fd) {
    unsigned int spi_status;
    int error;

    while (1) {
        error = MV_SPI_Flash_ReadStatus(spi_fd, &spi_status);
        if (S_OK != error)
            return S_FALSE;

        if (!(spi_status & MV_SPI_FLASH_WIP))
            break;
    }
    return S_OK;
}

int MV_SPI_Flash_Read(int spi_fd, unsigned int read_addr, unsigned char *read_buf, int read_len) {
    int error, count, tmp_len;

    if (spi_fd < 0) {
        MV_SPI_PERR("SPI Device File Descriptor is Invalid!");
        return S_FALSE;
    }

    if (read_addr > MV_SPI_MAX_ADDR) {
        MV_SPI_PERR("Read Address exceeds SPI Flash's Capacity 0x%x!", MV_SPI_MAX_ADDR);
        return S_FALSE;
    }

    if (!read_buf) {
        MV_SPI_PERR("Read Buffer Pointer is Null!");
        return S_FALSE;
    }

    if (read_len <= 0)
        return S_OK;

    if (read_addr & (MV_SPI_FLASH_PGSIZE - 1)) {
        /*
                error = MV_SPI_Flash_WaitEnable(spi_fd);
                if (S_OK != error) {
                    MV_SPI_PERR("Wait SPI Flash to Enable Failed!");
                    return S_FALSE;
                }
         */



        tmp_len = MV_SPI_FLASH_PGSIZE - (read_addr & (MV_SPI_FLASH_PGSIZE - 1));
        count = (read_len > tmp_len) ? tmp_len : read_len;

        error = MV_SPI_Flash_ReadPage(spi_fd, read_addr, read_buf, count);
        if (S_OK != error) {
            MV_SPI_PERR("Read SPI Flash Failed at Address: 0x%X!", read_addr);
            return S_FALSE;
        }
        read_buf += count;
        read_len -= count;
        read_addr += count;
    }

    if (read_len <= 0)
        return S_OK;


    while (read_len > 0) {


        /*
                error = MV_SPI_Flash_WaitEnable(spi_fd);
                if (S_OK != error) {
                    MV_SPI_PERR("Wait SPI Flash to Enable Failed!");
                    return S_FALSE;
                }
         */



        //MV_SPI_Flash_WaitForWrite();

        count = (read_len > MV_SPI_FLASH_PGSIZE) ? MV_SPI_FLASH_PGSIZE : read_len;

        error = MV_SPI_Flash_ReadPage(spi_fd, read_addr, read_buf, count);
        if (S_OK != error) {
            MV_SPI_PERR("Read SPI Flash Failed at Address: 0x%X!", read_addr);
            return S_FALSE;
        }
        read_buf += count;
        read_len -= count;
        read_addr += count;
    }

    return S_OK;
}

static int MV_SPI_Flash_WriteEnable(int spi_fd) {
    int error;
    unsigned int command;
    galois_spi_rw_t spi_rw;

    command = MV_SPI_FLASH_WREN;

    spi_rw.mst_id = MV_SPI_MASTERID;
    spi_rw.slave_id = MV_SPI_SLAVEID;

    spi_rw.rd_buf = NULL;
    spi_rw.rd_cnt = 0;
    spi_rw.wr_buf = &command;
    spi_rw.wr_cnt = 1;

    error = ioctl(spi_fd, SPI_IOCTL_READWRITE, &spi_rw);
    if (error != 0) {
        return S_FALSE;
    }

    return S_OK;
}

static int MV_SPI_Flash_WritePage(int spi_fd, unsigned int write_addr, const unsigned char *write_buf, int write_len) {
    galois_spi_rw_t spi_rw;
    int count, idx, error;
    unsigned int *buffer = NULL;
    unsigned int read_buf[128];

    count = (write_addr & (MV_SPI_FLASH_PGSIZE - 1)) + write_len;
    if (count > MV_SPI_FLASH_PGSIZE) {
        MV_SPI_PERR("Write Area Exceed One Page Size: 0x%X!", MV_SPI_FLASH_PGSIZE);
        return S_FALSE;
    }

    buffer = (unsigned int *) malloc((4 + write_len) * sizeof (int));
    if (!buffer) {
        MV_SPI_PERR("Malloc %d Space for Write Buffer Failed!", write_len * sizeof (int));
        return S_FALSE;
    }

    buffer[0] = CIMAX_REGISTER_WRITE_REQ;
    buffer[1] = write_addr >> 8 & 0xFF;
    buffer[2] = write_addr & 0xFF;
    buffer[3] = write_len;


    for (idx = 0; idx < write_len; idx++) {
        buffer[idx + 4] = write_buf[idx];
        //printf("%0X", write_buf[idx]);
    }
    //printf("\n");

    spi_rw.mst_id = MV_SPI_MASTERID;
    spi_rw.slave_id = MV_SPI_SLAVEID;

    spi_rw.wr_buf = buffer;
    spi_rw.wr_cnt = 4 + write_len;
    spi_rw.rd_buf = NULL;
    spi_rw.rd_cnt = 0;

    error = ioctl(spi_fd, SPI_IOCTL_READWRITE, &spi_rw);
    if (0 != error) {
        free(buffer);
        return S_FALSE;
    }

#if 0
    printf("written:\n");
    for (int i = 0; i < write_len; i++)
        printf("%0x:", buffer[i]);
    printf("\n");
#endif



    usleep(2000);
    spi_rw.wr_buf = NULL;
    spi_rw.wr_cnt = 0;
    spi_rw.rd_buf = read_buf;
    spi_rw.rd_cnt = 4;


    error = ioctl(spi_fd, SPI_IOCTL_READWRITE, &spi_rw);
    if (0 != error) {
        free(buffer);
        return S_FALSE;
    }

#if PRINT_ENABLE
    printf("read buf ");
    int i = 0;
    for (i = 0; i < 4; i++)
        printf("%0x:", read_buf[i]);
    printf("\n");
#endif
    
    /*
     * error while writting to cimax
     * must restart cimax and write again
     */
    if (read_buf[0] == 0) {
        printf("Error getting response\n");
        free(buffer);
        return S_FALSE;
    }

    free(buffer);

    usleep(1000);

    return S_OK;
}

static int MV_SPI_Flash_WritePageFirmware(int spi_fd, unsigned int write_addr, const unsigned char *write_buf, int write_len) {
    galois_spi_rw_t spi_rw;
    int count, idx, error;
    unsigned int *buffer = NULL;
    unsigned int read_buf[128];

    count = (write_addr & (MV_SPI_FLASH_PGSIZE - 1)) + write_len;
    if (count > MV_SPI_FLASH_PGSIZE) {
        MV_SPI_PERR("Write Area Exceed One Page Size: 0x%X!", MV_SPI_FLASH_PGSIZE);
        return S_FALSE;
    }

    buffer = (unsigned int *) malloc((4 + write_len) * sizeof (int));
    if (!buffer) {
        MV_SPI_PERR("Malloc %d Space for Write Buffer Failed!", write_len * sizeof (int));
        return S_FALSE;
    }

    buffer[0] = CIMAX_REGISTER_WRITE_REQ;
    buffer[1] = write_addr >> 8 & 0xFF;
    buffer[2] = write_addr & 0xFF;
    buffer[3] = write_len;


    for (idx = 0; idx < write_len; idx++) {
        buffer[idx + 4] = write_buf[idx];
        //printf("%0X", write_buf[idx]);
    }
    //printf("\n");

    spi_rw.mst_id = MV_SPI_MASTERID;
    spi_rw.slave_id = MV_SPI_SLAVEID;

    spi_rw.wr_buf = buffer;
    spi_rw.wr_cnt = 4 + write_len;
    spi_rw.rd_buf = NULL;
    spi_rw.rd_cnt = 0;

    error = ioctl(spi_fd, SPI_IOCTL_READWRITE, &spi_rw);
    if (0 != error) {
        free(buffer);
        return S_FALSE;
    }

#if 0
    printf("written:\n");
    for (int i = 0; i < write_len; i++)
        printf("%0x:", buffer[i]);
    printf("\n");
#endif



    usleep(2000);
    spi_rw.wr_buf = NULL;
    spi_rw.wr_cnt = 0;
    spi_rw.rd_buf = read_buf;
    spi_rw.rd_cnt = 4;


    error = ioctl(spi_fd, SPI_IOCTL_READWRITE, &spi_rw);
    if (0 != error) {
        free(buffer);
        return S_FALSE;
    }

#if PRINT_ENABLE
    printf("read buf ");
    int i = 0;
    for (i = 0; i < 4; i++)
        printf("%0x:", read_buf[i]);
    printf("\n");
#endif
    
    /*
     * error while writting to cimax
     * must restart cimax and write again
     */
    if (read_buf[0] == 0) {
        printf("Error getting response\n");
        free(buffer);
        return S_FALSE;
    }

    free(buffer);

    usleep(1000);

    return S_OK;
}


int MV_SPI_Flash_Write(int spi_fd, unsigned int write_addr, const unsigned char *write_buf, int write_len) {
    int error;
    int tmp_len;
    int count;



    if (spi_fd < 0) {
        MV_SPI_PERR("SPI Device File Descriptor is Invalid!");
        return S_FALSE;
    }

    if (write_addr > MV_SPI_MAX_ADDR) {
        MV_SPI_PERR("Write Address exceeds SPI Flash's Capacity 0x%x!", MV_SPI_MAX_ADDR);
        return S_FALSE;
    }

    if (!write_buf) {
        MV_SPI_PERR("Write Buffer Pointer is NULL!");
        return S_FALSE;
    }

    if (write_len <= 0)
        return S_OK;

    write_buf += 0x8000;

    if (write_addr & (MV_SPI_FLASH_PGSIZE - 1)) {
        tmp_len = MV_SPI_FLASH_PGSIZE - (write_addr & (MV_SPI_FLASH_PGSIZE - 1));
        count = (write_len > tmp_len) ? tmp_len : write_len;

        error = MV_SPI_Flash_WritePage(spi_fd, write_addr, write_buf, count);
        if (S_OK != error) {
            MV_SPI_PERR("Write SPI Flash Failed at Address: 0x%X!", write_addr);
            return S_FALSE;
        }

        write_buf += count;
        write_len -= count;
        write_addr += count;
    }

    if (write_len <= 0)
        return S_OK;


    while (write_len > 0) {

#if PRINT_ENABLE
        printf("Wait for write...\n");
#endif            
        MV_SPI_Flash_WaitForWrite();

        tmp_len = MV_SPI_FLASH_PGSIZE - (write_addr & (MV_SPI_FLASH_PGSIZE - 1));
        count = (write_len > tmp_len) ? tmp_len : write_len;


        printf("FWSign = %0X len = %0X ", FWSign, count);
        FWSign = computeBistRom(write_buf, count, FWSign);
        printf("FWSign = %0X\n", FWSign);


        error = MV_SPI_Flash_WritePageFirmware(spi_fd, write_addr, write_buf, count);
        if (S_OK != error) {
            MV_SPI_PERR("Write SPI Flash Failed at Address: 0x%X!", write_addr);
            return S_FALSE;
        }


        write_buf += count;
        write_len -= count;
        write_addr += count;
    }

    return S_OK;
}
#if 0

int MV_SPI_Backup_Image(int spi_fd) {
    int error;
    struct MV_SPI_Image_Header image_header;
    int header_length = sizeof (struct MV_SPI_Image_Header);
    int backup_fd, backup_len;
    struct stat backup_stat;
    void *backup_ptr;

    backup_fd = open(MV_SPI_BACKUP_FILE, O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (backup_fd < 0) {
        MV_SPI_PERR("Open SPI Backup File Failed!");
        return S_FALSE;
    }

    error = fstat(backup_fd, &backup_stat);
    if (error < 0) {
        MV_SPI_PERR("Get SPI Backup File Status Failed!");
        error = S_FALSE;
        goto close_backup;
    }

    error = MV_SPI_Flash_Read(spi_fd, MV_SPI_HEADER_ADDR, (unsigned char *) &image_header, header_length);
    if (error != S_OK) {
        MV_SPI_PERR("Read SPI Image Header Failed!");
        error = S_FALSE;
        goto close_backup;
    }

    backup_len = image_header.image_len + header_length;
    backup_ptr = mmap(NULL, backup_len, PROT_WRITE, MAP_PRIVATE, backup_fd, 0);
    if (MAP_FAILED == backup_ptr) {
        MV_SPI_PERR("Memory Map SPI Backup File Failed!");
        error = S_FALSE;
        goto close_backup;
    }

    memcpy(backup_ptr, &image_header, header_length);
    error = MV_SPI_Flash_Read(spi_fd, MV_SPI_START_ADDR, (unsigned char *) (backup_ptr + header_length), image_header.image_len);
    if (error != S_OK) {
        MV_SPI_PERR("Read SPI Image Body Failed!");
        error = S_FALSE;
        goto unmap_backup;
    }

    error = msync(backup_ptr, backup_len, MS_SYNC);
    if (error < 0) {
        MV_SPI_PERR("Memory Sync SPI Backup File Failed!");
        error = S_FALSE;
        goto unmap_backup;
    }

    error = MV_SPI_Image_Check(backup_ptr, backup_len);
    if (error != S_OK) {
        MV_SPI_PERR("Check SPI Backup File Failed!");
        error = S_FALSE;
        goto unmap_backup;
    }

    error = S_OK;

unmap_backup:
    munmap(backup_ptr, backup_len);
close_backup:
    close(backup_fd);
    return error;
}
#endif

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
unsigned char checkBistRom(int startAddr, int endAddr, int signature) {

    unsigned char Val[2];

    /* Write "Flash" Size. */
    Val[0] = (unsigned char) ((0xD000 - startAddr)&0x00ff);
    Val[1] = (unsigned char) ((0xD000 - startAddr) >> 8);

    printf("Val [0] = %0X\n", Val[0]);
    printf("Val [1] = %0X\n", Val[1]);

    if (MV_SPI_Flash_WritePage(spi_fd, 0x008D, Val, 2) != 0)
        return S_FALSE;

    /* Write Signature. */
    Val[0] = (unsigned char) (signature & 0x00ff);
    Val[1] = (unsigned char) (signature >> 8);

    printf("Val [0] = %0X\n", Val[0]);
    printf("Val [1] = %0X\n", Val[1]);


    if (MV_SPI_Flash_WritePage(spi_fd, 0x0080, Val, 2) != 0)
        return S_FALSE;

    /* Launch BistRom [(D000-flashSize)..CFF9]+[FFFA..FFFF] computation. */
    Val[0] = 0x0F;
    if (MV_SPI_Flash_WritePage(spi_fd, 0x0082, Val, 1) != 0)
        return S_FALSE;

    /* Read Signature. */
    if (MV_SPI_Flash_ReadPage(spi_fd, BISTROM_SignatureLSB, Val, 2) != 0)
        return S_FALSE;

    printf("Val [0] = %0X\n", Val[0]);
    printf("Val [1] = %0X\n", Val[1]);


    /* Read Boot status. */
    if (MV_SPI_Flash_ReadPage(spi_fd, BOOT_Status, Val, 1) != 0)
        return S_FALSE;

    printf("Val [0] = %0X\n", Val[0]);
    printf("Val [1] = %0X\n", Val[1]);

    return Val[0];
}

int MV_SPI_Flash_WriteStatus(int spi_fd, unsigned int spi_status) {
    int error;
    galois_spi_rw_t spi_rw;
    unsigned int buffer[2];

    error = MV_SPI_Flash_WaitEnable(spi_fd);
    if (S_OK != error) {
        MV_SPI_PERR("Wait SPI Flash to Enable Failed!");
        return S_FALSE;
    }

    error = MV_SPI_Flash_WriteEnable(spi_fd);
    if (S_OK != error) {
        MV_SPI_PERR("Write Enable SPI Flash Failed!");
        return S_FALSE;
    }

    buffer[0] = MV_SPI_FLASH_WRSR;
    buffer[1] = spi_status;

    spi_rw.mst_id = MV_SPI_MASTERID;
    spi_rw.slave_id = MV_SPI_SLAVEID;

    spi_rw.wr_buf = buffer;
    spi_rw.wr_cnt = 2;
    spi_rw.rd_buf = NULL;
    spi_rw.rd_cnt = 0;

    error = ioctl(spi_fd, SPI_IOCTL_READWRITE, &spi_rw);
    if (error != 0)
        return S_FALSE;

    return S_OK;
}

int MV_SPI_Flash_EraseBlock(int spi_fd, unsigned int erase_addr, int blk_num) {
    int error;
    unsigned int idx;
    galois_spi_rw_t spi_rw;
    unsigned int buffer[4];

    if (erase_addr & (MV_SPI_FLASH_BLKSIZE - 1)) {
        MV_SPI_PERR("Erase Address Must be Block Size (0x%X) Align!", MV_SPI_FLASH_BLKSIZE);
        return S_FALSE;
    }

    for (idx = 0; idx < blk_num; idx++) {
        error = MV_SPI_Flash_WaitEnable(spi_fd);
        if (S_OK != error) {
            MV_SPI_PERR("Wait SPI Flash to Enable Failed!");
            return S_FALSE;
        }

        error = MV_SPI_Flash_WriteEnable(spi_fd);
        if (S_OK != error) {
            MV_SPI_PERR("Write Enable SPI Flash Failed!");
            return S_FALSE;
        }

        buffer[0] = MV_SPI_FLASH_SE;
        buffer[1] = erase_addr >> 16 & 0xFF;
        buffer[2] = erase_addr >> 8 & 0xFF;
        buffer[3] = erase_addr & 0xFF;

        spi_rw.mst_id = MV_SPI_MASTERID;
        spi_rw.slave_id = MV_SPI_SLAVEID;

        spi_rw.wr_buf = buffer;
        spi_rw.wr_cnt = 4;
        spi_rw.rd_buf = NULL;
        spi_rw.rd_cnt = 0;

        error = ioctl(spi_fd, SPI_IOCTL_READWRITE, &spi_rw);
        if (error != 0)
            return S_FALSE;
    }
    return S_OK;
}
#if 0

int MV_SPI_Flash_Update(const char* upimg_path) {
    int upimg_fd, spi_fd;
    FILE *spi_file;
    struct stat upimg_stat;
    int error, count;
    void *upimg_ptr;
    int header_length = sizeof (struct MV_SPI_Image_Header);
    int image_length;
    galois_spi_info_t spi_info;

    upimg_fd = open(upimg_path, O_RDONLY);
    if (upimg_fd < 0) {
        MV_SPI_PERR("Open SPI Update Image Failed!");
        return S_FALSE;
    }

    error = fstat(upimg_fd, &upimg_stat);
    if (error < 0) {
        MV_SPI_PERR("Get SPI Image Status Failed!");
        error = S_FALSE;
        goto exit_close;
    }

    upimg_ptr = mmap(NULL, upimg_stat.st_size, PROT_READ, MAP_PRIVATE, upimg_fd, 0);
    if (MAP_FAILED == upimg_ptr) {
        MV_SPI_PERR("Memory Map SPI Image Failed!");
        error = S_FALSE;
        goto exit_close;
    }

    // Check Update Image
    error = MV_SPI_Image_Check(upimg_ptr, upimg_stat.st_size);
    if (error != S_OK) {
        MV_SPI_PERR("Check SPI Image Failed!");
        error = S_FALSE;
        goto exit_unmap;
    }
    MV_SPI_PINF("SPI Update Image Check Finished!");

    // Initialize SPI Flash Device
    spi_file = fopen(MV_SPI_DEVICE_NAME, "r+");
    if (!spi_file) {
        MV_SPI_PERR("Open SPI Flash Device Failed!");
        error = S_FALSE;
        goto exit_unmap;
    }
    spi_fd = fileno(spi_file);

    error = MV_SPI_Flash_Init(spi_fd);
    if (error != S_OK) {
        MV_SPI_PERR("SPI Flash Initialize Failed!");
        error = S_FALSE;
        goto exit_unmap;
    }
    MV_SPI_PINF("SPI Flash is Initialized!");

    error = MV_SPI_Get_Info(spi_fd, &spi_info);
    if (error != S_OK) {
        MV_SPI_PERR("Get SPI Flash Infomation Failed!");
        error = S_FALSE;
        goto exit_unmap;
    }

    MV_SPI_Print_Info(&spi_info);

    // Backup SPI Image
    error = MV_SPI_Backup_Image(spi_fd);
    if (error != S_OK) {
        MV_SPI_PERR("SPI Flash Backup Failed!");
        error = S_FALSE;
        // need not to quit
    }
    if (error != S_FALSE)
        MV_SPI_PINF("SPI Image is Backup to %s!", MV_SPI_BACKUP_FILE);

    // Erase SPI Flash
    image_length = upimg_stat.st_size - header_length;
    count = (image_length + MV_SPI_FLASH_BLKSIZE - 1) / MV_SPI_FLASH_BLKSIZE;
    error = MV_SPI_Flash_EraseBlock(spi_fd, MV_SPI_START_ADDR, count);
    if (error != S_OK) {
        MV_SPI_PERR("Erase SPI Flash Body Failed!");
        error = S_FALSE;
        goto exit_unmap;
    }

    count = (header_length + MV_SPI_FLASH_BLKSIZE - 1) / MV_SPI_FLASH_BLKSIZE;
    error = MV_SPI_Flash_EraseBlock(spi_fd, MV_SPI_HEADER_ADDR, count);
    if (error != S_OK) {
        MV_SPI_PERR("Erase SPI Flash Partial Header Failed!");
        error = S_FALSE;
        goto exit_unmap;
    }



    // Write SPI Image
    error = MV_SPI_Flash_Write(spi_fd, MV_SPI_START_ADDR, (const unsigned char *) (upimg_ptr + header_length), image_length);
    if (error != S_OK) {
        MV_SPI_PERR("Write SPI Flash Body Failed!");
        error = S_FALSE;
        goto exit_unmap;
    }
    MV_SPI_PINF("Update SPI Image Body Complete!");


    error = MV_SPI_Flash_Write(spi_fd, MV_SPI_HEADER_ADDR, (const unsigned char *) upimg_ptr, header_length);
    if (error != S_OK) {
        MV_SPI_PERR("Write SPI Flash Body Failed!");
        error = S_FALSE;
        goto exit_unmap;
    }
    MV_SPI_PINF("Update SPI Image Header Complete!");


exit_unmap:
    munmap(upimg_ptr, upimg_stat.st_size);
exit_close:
    close(upimg_fd);
    return error;
}
#endif

/**
 * @ingroup     CIMaX_Statics
 *
 * @brief       Downloads configuration (register settings) defined in spiRegSettings array. 
 *
 * @param       none
 *
 * @return      CIMAX_NO_ERROR if successfull, CIMAX_ERROR if failure.
 */
int downloadCfg(void) {
    unsigned int cnt;
    unsigned char buf[CIMAX_REGISTER_MAX_PAYLOAD_SIZE];


    printf("Download CIMaX+ configuration(register settings): ");

    for (cnt = 0; cnt < sizeof (spiRegSettings) / sizeof (regSettings_t); cnt++) {

        printf("spiRegSettings[%d].op = %0X\n", cnt, spiRegSettings[cnt].op);

        switch (spiRegSettings[cnt].op) {
            case REG_OP_READ:
                /* Read register. */

                if (MV_SPI_Flash_ReadPage(spi_fd, spiRegSettings[cnt].reg, buf, 1)) {
                    /* CIMaX+ read error. */
                    printf("FAILED at REG_OP_READ operation.\n");
                    return S_FALSE;
                }
                //printf("Read operation: register 0x%04x = 0x%02x.\n", spiRegSettings[cnt].reg, buf[0]);
                break;
            case REG_OP_WRITE:
                /* Write register. */
                if (MV_SPI_Flash_WritePage(spi_fd, spiRegSettings[cnt].reg, (unsigned char *) &spiRegSettings[cnt].val, 1) < 0)
                    //if (CIMAX_WriteRegister(spiRegSettings[cnt].reg, (uint8 *)&spiRegSettings[cnt].val, 1) < 0)
                {
                    /* CIMaX+ write error. */
                    printf("FAILED at REG_OP_WRITE operation.\n");
                    return S_FALSE;
                }
                //printf("Write operation: register 0x%04x = 0x%02x.\n", spiRegSettings[cnt].reg, spiRegSettings[cnt].val);
                break;
            case REG_OP_WAIT_TO_BE_SET:
                do {
                    if (MV_SPI_Flash_ReadPage(spi_fd, spiRegSettings[cnt].reg, buf, 1) < 0) {
                        /* CIMaX+ read error. */
                        printf("FAILED at REG_OP_WAIT_TO_BE_SET operation.\n");
                        return S_FALSE;
                    }
                } while ((buf[0] & spiRegSettings[cnt].val) != spiRegSettings[cnt].val);
                //printf("Wait register to be set operation: register 0x%04x = 0x%02x.\n", spiRegSettings[cnt].reg, buf[0]);
                break;
            case REG_OP_WAIT_TO_BE_CLEARED:
                do {
                    if (MV_SPI_Flash_ReadPage(spi_fd, spiRegSettings[cnt].reg, buf, 1) < 0) {
                        /* CIMaX+ read error. */
                        printf("FAILED at REG_OP_WAIT_TO_BE_CLEARED operation.\n");
                        return S_FALSE;
                    }
                } while ((buf[0] & spiRegSettings[cnt].val) != 0);
                //printf("Wait register to be cleared operation: register 0x%04x = 0x%02x.\n", spiRegSettings[cnt].reg, buf[0]);
                break;
            case REG_OP_WAIT_EQUAL:
                do {
                    if (MV_SPI_Flash_ReadPage(spi_fd, spiRegSettings[cnt].reg, buf, 1) < 0) {
                        /* CIMaX+ read error. */
                        printf("FAILED at REG_OP_WAIT_EQUAL operation.\n");
                        return S_FALSE;
                    }
                } while (buf[0] != spiRegSettings[cnt].val);
                //printf("Wait register to be equal operation: register 0x%04x = 0x%02x.\n", spiRegSettings[cnt].reg, buf[0]);
                break;
            case REG_OP_LOGICAL_AND:
                if (MV_SPI_Flash_ReadPage(spi_fd, spiRegSettings[cnt].reg, buf, 1) < 0) {
                    /* CIMaX+ read error. */
                    printf("FAILED at REG_OP_LOGICAL_AND operation (reading).\n");
                    return S_FALSE;
                }
                buf[0] &= spiRegSettings[cnt].val;
                if (MV_SPI_Flash_WritePage(spi_fd, spiRegSettings[cnt].reg, buf, 1) < 0) {
                    /* CIMaX+ write error. */
                    printf("FAILED at REG_OP_LOGICAL_AND operation (writing).\n");
                    return S_FALSE;
                }
                //printf("Logical AND operation: register 0x%04x = 0x%02x.\n", spiRegSettings[cnt].reg, buf[0]);
                break;
            case REG_OP_LOGICAL_OR:
                if (MV_SPI_Flash_ReadPage(spi_fd, spiRegSettings[cnt].reg, buf, 1) < 0) {
                    /* CIMaX+ read error. */
                    printf("FAILED at REG_OP_LOGICAL_OR operation (reading).\n");
                    return S_FALSE;
                }
                buf[0] |= spiRegSettings[cnt].val;
                if (MV_SPI_Flash_WritePage(spi_fd, spiRegSettings[cnt].reg, buf, 1) < 0) {
                    /* CIMaX+ write error. */
                    printf("FAILED at REG_OP_LOGICAL_AND operation (writing).\n");
                    return S_FALSE;
                }
                //printf("Logical OR operation: register 0x%04x = 0x%02x.\n", spiRegSettings[cnt].reg, buf[0]);
                break;
            case REG_OP_WAIT:
                //printf("Wait for %d ms.\n", spiRegSettings[cnt].val);
                usleep(spiRegSettings[cnt].val);
                break;
            default:
                printf("\nInvalid operation 0x%02x!\n", spiRegSettings[cnt].op);
                return S_FALSE;
                break;
        }
    }
    printf("OK.\n");
    return S_OK;
}


int main(int argc, char **argv) {
    FILE *spi_file = NULL;
    char *image_path = NULL;
    unsigned char *image_ptr = NULL;
    int image_fd;

    int error, idx, count;
    int ch;
    struct stat image_stat;
    int spi_addr;
    unsigned char *check_ptr = NULL;

    unsigned char bootStatusReg[10];
    int bistromCheckStatus;

    unsigned char *firstBufferAddress = NULL;
    //unsigned int FWSign = 0;
    
    printf("***************** SPI CIMAX BOOT *******************\n");
    
    //sleep(5);
    
    /* TSI pinmux */

    set_pinmux(6, 1);
    set_pinmux(7, 1);
    
    
    /* GPIO 28 */
    set_pinmux(16, 0);
    
    // cimax is in not reset mode
    set_gpio(28, 1);
    
    printf("Pinmux is set\n");

/*
    if (argc != 5) {
        printf("Usage: %s -f IMAGE_FILE -a WRITE_OFFSET\n", argv[0]);
        error = S_FALSE;
        goto exit_ret;
    }

    while ((ch = getopt(argc, argv, "f:a:")) != -1) {
        switch (ch) {
            case 'f':
                image_path = optarg;
                error = S_OK;
                break;
            case 'a':
                spi_addr = strtol(optarg, NULL, 16);
                error = S_OK;
                break;
            case '?':
                printf("Usage: %s -f IMAGE_FILE -a WRITE_OFFSET\n", argv[0]);
                error = S_FALSE;
                break;
        }
    }
*/
    
    image_path = "cimax_spidvb_211.bin";

    /* force spi address 0x8000 */
    spi_addr = FW_START_ADDRESS;

    if (error == S_FALSE)
        goto exit_ret;

    if (spi_addr < 0 || spi_addr > MV_SPI_MAX_ADDR) {
        MV_SPI_PERR("Address %x is not supported!", spi_addr);
        error = S_FALSE;
        goto exit_ret;
    }

    if (!image_path) {
        MV_SPI_PERR("SPI Flash Image Path is NULL!");
        error = S_FALSE;
        goto exit_ret;
    }

    image_fd = open(image_path, O_RDONLY);
    if (image_fd < 0) {
        MV_SPI_PERR("Open SPI Flash Image '%s' Failed!", image_path);
        error = S_FALSE;
        goto exit_ret;
    }

    error = fstat(image_fd, &image_stat);
    if (error < 0) {
        MV_SPI_PERR("Get SPI Image Status Failed!");
        error = S_FALSE;
        goto exit_close;
    }

    image_ptr = (unsigned char *) mmap(NULL, image_stat.st_size, PROT_READ, MAP_PRIVATE, image_fd, 0);
    if (MAP_FAILED == image_ptr) {
        MV_SPI_PERR("Memory Map SPI Image Failed!");
        error = S_FALSE;
        goto exit_close;
    }

    // save first address of buffer
    firstBufferAddress = image_ptr;

    // Initialize SPI Flash Device
    spi_file = fopen(MV_SPI_DEVICE_NAME, "r+");
    if (!spi_file) {
        MV_SPI_PERR("Open SPI Flash Device Failed!");
        error = S_FALSE;
        goto exit_unmap;
    }
    //fseek(spi_file, 0x8000, SEEK_SET);
    spi_fd = fileno(spi_file);




    error = MV_SPI_Flash_Init(spi_fd);
    if (error != S_OK) {
        MV_SPI_PERR("SPI Flash Initialize Failed!");
        error = S_FALSE;
        goto exit_unmap;
    }
    //	MV_SPI_PINF("SPI Flash is Initialized!");


  
    printf("image_stat.st_size = %0X\n", image_stat.st_size);
    image_stat.st_size = 0x4FFA;


    //FWSign = computeBistRom()



    MV_SPI_PINF("Writing SPI Image at 0x%x, Length: %d...", spi_addr, image_stat.st_size);
    
    error = S_OK;
    
    int tries = 5;
    
    do {
    		tries--;
        printf("Start write\n");
        error = MV_SPI_Flash_Write(spi_fd, spi_addr, image_ptr, image_stat.st_size);
        if (error != S_OK) {
            
            //MV_SPI_PERR("Failed!");
            error = S_FALSE;
            
            printf("Writing FW failed, please reset cimax");

            
            //sleep(5);
            

            set_gpio(28, 0);
            usleep(10);
            set_gpio(28, 1);
            
            printf("Try again");
        }
        
    }while ((error != S_OK) && tries);
    

    MV_SPI_PINF("Checking SPI Flash at 0x%x, Length: %d...", spi_addr, image_stat.st_size);
    check_ptr = (unsigned char *) malloc(image_stat.st_size);
    if (!check_ptr) {
        MV_SPI_PERR("Malloc for Check Buffer Failed!");
        error = S_FALSE;
        goto exit_free;
    }


    printf("Write vector:\n");
    firstBufferAddress += 0xFFFA;
    error = MV_SPI_Flash_WritePage(spi_fd, 0xFFFA, firstBufferAddress, 6);
    if (error != S_OK) {
        MV_SPI_PERR("Writting vector failed!");
        error = S_FALSE;
        goto exit_free;
    }


    FWSign = computeBistRom(firstBufferAddress, 6, FWSign);
    printf("FWSign = %0X\n", FWSign);

    printf("Check bistrom\n");
    bistromCheckStatus = checkBistRom(0x8000, 0xCFF9, FWSign);
    printf("bistromCheckStatus = %0X\n", bistromCheckStatus);


    printf("Init FW");
    error = initFW(spi_fd);
    if (error != S_OK) {
        MV_SPI_PERR("Init FW failed!");
        error = S_FALSE;
        goto exit_free;
    }

    usleep(5000);


    printf("Download config");
    error = downloadCfg();
    if (error != S_OK) {
        MV_SPI_PERR("Download config failed!");
        error = S_FALSE;
        goto exit_free;
    }


    usleep(5000);

    printf("Read boot status register\n");

    error = MV_SPI_Flash_ReadPage(spi_fd, 0x0009, bootStatusReg, 1);
    if (S_OK != error) {
        MV_SPI_PERR("Read boot status register failed!\n");
        return S_FALSE;
    } else {
        printf("boot status = %0X\n", bootStatusReg);
    }

    //free(bootStatusReg);


    error = S_OK;
    
    printf("***************** SPI CIMAX BOOT OK *******************\n");

exit_free:
    if (check_ptr) {
        free(check_ptr);
        check_ptr = NULL;
    }
exit_unmap:
    if (image_ptr) {
        munmap(image_ptr, image_stat.st_size);
        image_ptr = NULL;
    }
exit_close:
    if (spi_file) {
        fclose(spi_file);
        spi_file = NULL;
    }
    if (image_fd) {
        close(image_fd);
        image_fd = 0;
    }
exit_ret:
    return error;
}
