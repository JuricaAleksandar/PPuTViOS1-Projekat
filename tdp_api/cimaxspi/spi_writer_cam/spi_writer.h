#include "spi_bus.h"

#define PRINT_ENABLE 1

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

/******************** CIMaX_Register_Map CIMaX+ Register Map *********************/

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



#define FW_START_ADDRESS                  0x8000

/** CIMaX+ register init request. */
#define CIMAX_REGISTER_INIT_REQ           0x00
/** CIMaX+ register write request. */
#define CIMAX_REGISTER_WRITE_REQ          0x7F
/** CIMaX+ register read request. */
#define CIMAX_REGISTER_READ_REQ           0xFF

/** CIMaX+ register maximum payload size. */
#define CIMAX_REGISTER_MAX_PAYLOAD_SIZE   255



/******************** GPIO Settings ************************/

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




#define BISTROM_SignatureLSB              0x0041   
#define BOOT_Status                       0x0009   





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



/************************************ PINMUX ****************************************/

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



/*************** Configure for parallel input and serial output *********************/

static regSettings_t spiRegSettings[] = {

    {IN_SEL, 0x00, REG_OP_WRITE},       // Close TS input.
    {OUT_SEL, 0x00, REG_OP_WRITE},      // Close TS output.
    {FIFO_CTRL, 0x0f, REG_OP_WRITE},    // Reset TS FIFO.
    {SYNC_RTV_CTRL, 0x0f, REG_OP_WRITE},

    {0x0000, 200, REG_OP_WAIT},         // Wait 200 miliseconds. 


    {0x0049, 0x15, REG_OP_WRITE},       //CkMan_Select Select 108MHz Serial 0 and Serial 1 clock.
    {0x0048, 0x6F, REG_OP_WRITE},       //CkMan_Config Config Serial 0 and Serial 1 clock enable.
    
    {0x0224, 0x00, REG_OP_WRITE},       //S2P_CH1_CTRL S2P CH1 (enable & rising edge & reset.)
    
    {0x0228, 0x22, REG_OP_WRITE},       //S2P_CH1_CTRL S2P CH1 (enable & rising edge & reset.)
    {0x0227, 0x00, REG_OP_WRITE},       //S2P_CH1_CTRL S2P CH1 (enable & rising edge & reset.)
    {0x022A, 0x18, REG_OP_WRITE},       //P2S_CH1_CTRL P2S CH1 (MOSTRT aligned on one byte & internal clock & enable & reset).

    {0x0000, 200, REG_OP_WAIT},         // Wait 200 miliseconds 
    {0x022F, 0x18, REG_OP_WRITE},       //ROUTER_CAM_CH 
    {0x0230, 0x00, REG_OP_WRITE},       //ROUTER_CAM_MOD
    
    {0x022E, 0x04, REG_OP_WRITE},       //OUT_SEL CH0 => Serial 1 Out (P2S0), CH1 => Serial 2 Out (P2S1)
    
    {0x022D, 0x01, REG_OP_WRITE},       //IN_SEL Parallel In => CH0, Serial 2 In => CH1.
};


