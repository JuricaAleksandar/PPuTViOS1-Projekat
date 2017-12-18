/*
   Copyright notice
   � 2005 RT-RK LLC. All rights reserved
   This software and related documentation (the "Software") are intellectual
   property owned by RT-RK and are copyright of RT-RK, unless specifically
   noted otherwise.
   Any use of the Software is permitted only pursuant to the terms of the license
   agreement, if any, which accompanies, is included with or applicable to the
   Software ("License Agreement") or upon express written consent of RT-RK. Any
   copying, reproduction or redistribution of the Software in whole or in part by
   any means not in accordance with the License Agreement or as agreed in writing
   by RT-RK is expressly prohibited.
   THE SOFTWARE IS WARRANTED, IF AT ALL, ONLY ACCORDING TO THE TERMS OF THE LICENSE
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
* @file     cimax_spi_pio.h
*
* @brief    CIMaX+ SPI and PIO driver API.
*
* @note     Copyright (C) 2010 RT-RK LLC 
*           All Rights Reserved
*
* @author   RT-RK CIMaX+ team
*
* @date     21/11/2010          
*/


#ifndef __CIMAX_SPI_PIO_H__
#define __CIMAX_SPI_PIO_H__

/******************************************************************************
 * Includes
 *****************************************************************************/
#include "os.h"
#include <pthread.h>
#include <stdio.h>


/******************************************************************************
 * Defines
 *****************************************************************************/
#define CIMAX_SPI_MAX_BUFF_LEN      512

/* Supported SPI modes	*/
#define CIMAX_SPI_MODE_0            0
#define CIMAX_SPI_MODE_1            1
#define CIMAX_SPI_MODE_2            2
#define CIMAX_SPI_MODE_3            3

/* Supported SPI clocks	*/
#define CIMAX_SPI_CLOCK_1MHZ        1*1000000
#define CIMAX_SPI_CLOCK_2MHZ        2*CIMAX_SPI_CLOCK_1MHZ
#define CIMAX_SPI_CLOCK_3MHZ        3*CIMAX_SPI_CLOCK_1MHZ
#define CIMAX_SPI_CLOCK_4MHZ        4*CIMAX_SPI_CLOCK_1MHZ
#define CIMAX_SPI_CLOCK_5MHZ        5*CIMAX_SPI_CLOCK_1MHZ
#define CIMAX_SPI_CLOCK_6MHZ        6*CIMAX_SPI_CLOCK_1MHZ
#define CIMAX_SPI_CLOCK_7MHZ        7*CIMAX_SPI_CLOCK_1MHZ
#define CIMAX_SPI_CLOCK_8MHZ        8*CIMAX_SPI_CLOCK_1MHZ
#define CIMAX_SPI_CLOCK_9MHZ        9*CIMAX_SPI_CLOCK_1MHZ
#define CIMAX_SPI_CLOCK_10MHZ       10*CIMAX_SPI_CLOCK_1MHZ
#define CIMAX_SPI_CLOCK_11MHZ       11*CIMAX_SPI_CLOCK_1MHZ
#define CIMAX_SPI_CLOCK_12MHZ       12*CIMAX_SPI_CLOCK_1MHZ



#define MV_SPI_DEVICE_NAME		"/dev/spi1"
#define MV_SPI_MASTERID			1		// SPI Master ID
#define MV_SPI_SLAVEID			2		// SPI Slave ID


/* slv_addr */
#define SPI_SLAVE_ID_M25P40a	0
#define SPI_SLAVE_ID_M25P40b	1
#define SPI_SLAVE_ID_M25P40c	2
#define SPI_SLAVE_ID_WM8768a	0
#define SPI_SLAVE_ID_WM8751La	1
#define SPI_SLAVE_ID_BOEa		2
/* xd_mode */
#define SPIDEV_XDMODE0_WOA7D9		0
#define SPIDEV_XDMODE1_RWI8A24D8n	1
#define SPIDEV_XDMODE2_WOA8D8n		2
/* clock mode */
#define SPI_CLOCKMODE0_POL0PH0		0
#define SPI_CLOCKMODE1_POL0PH1		1
#define SPI_CLOCKMODE2_POL1PH0		2
#define SPI_CLOCKMODE3_POL1PH1		3
/* speed */
#define SPI_SPEED_200KHZ	200
#define SPI_SPEED_1000KHZ	1000
/* frame width */
#define SPI_FRAME_WIDTH8	8

/*
 * ioctl commands
 */
#define SPI_IOCTL_SETINFO	0x1111
#define SPI_IOCTL_GETINFO	0x1004
#define SPI_IOCTL_READ		0x1980	/* obsoleted */
#define SPI_IOCTL_WRITE		0x2008	/* obsoleted */
#define SPI_IOCTL_READWRITE	0x0426
#define SPI_IOCTL_READWRITE_CHAR	0x6666


/******************************************************************************
 * Typedefs
 *****************************************************************************/

typedef struct {
   uint8    mode;
   uint32   clock;
} spiInitParams_t;

typedef struct galois_pinmux_data {
    int owner; //  soc or sm pinmux
    int group; //  group id
    int value; //  group value
    char * requster; //  who request the change
} galois_pinmux_data_t;

typedef struct galois_gpio_data {
        int port;       /* port number: for SoC, 0~31; for SM, 0~11 */
        int mode;       /* port mode: data(in/out) or irq(level/edge) */
        unsigned int data;      /* port data: 1 or 0, only valid when mode is data(in/out) */
} galois_gpio_data_t;

typedef struct galois_spi_info {
	uint mst_id;		/* user input: master num you access */
	uint slave_id;		/* user input: which CS_n line */
	uint xd_mode; 		/* user input: obsoleted, ignore it */
	uint clock_mode;	/* user input: control polarity and phrase of SPI proto */
	uint speed;			/* user input: control baudrate, 200KHz by default */
	uint frame_width;	/* user input: frame width, 8bits by default */
	int irq;	/* kernel info, don't touch */
	int usable;	/* kernel info, don't touch */
} galois_spi_info_t;

typedef struct galois_spi_rw {
	uint mst_id;	/* user input: master num you're operating */
	uint slave_id;	/* user input: the slave's address you're operating */

	uint rd_cnt;	/* read number in frame_width, <=256 */
	uint *rd_buf;	/* data buffer for read */
	uint wr_cnt;	/* write number in frame_width, <=256 */
	uint *wr_buf;	/* data buffer for write */
} galois_spi_rw_t;

/******************************************************************************
 * Functions
 *****************************************************************************/
int32 CIMAX_SPI_Init (spiInitParams_t *pSpiInitParams);
int32 CIMAX_SPI_Write (uint8 *buff, uint32 numOfBytes);
int32 CIMAX_SPI_Read (uint8 *buff, uint32 numOfBytes);
int32 CIMAX_SPI_Term ();

int32 CIMAX_PIO_Init ();
int32 CIMAX_PIO_Reset ();
int32 CIMAX_PIO_GetIntState (uint8 *pState);
int32 CIMAX_PIO_Term ();


#endif   /* __CIMAX_SPI_PIO_H__  */
