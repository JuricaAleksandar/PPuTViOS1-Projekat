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
 * @file     cimax_spi_pio.c
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

/******************************************************************************
 * Includes
 *****************************************************************************/
#include "cimax_spi_pio.h"


/******************************************************************************
 * Defines
 *****************************************************************************/

/******************************************************************************
 * Globals
 *****************************************************************************/
volatile int init_done = 0;

static FILE *spi_file = NULL;
static int spi_fd = 0;

#ifdef LOG_SPI_INTO_FILE
FILE* fp_log;
#endif

/******************************************************************************
 * Functions
 *****************************************************************************/
int32 CIMAX_SPI_Init(spiInitParams_t *pSpiInitParams) {

    galois_spi_info_t spi_info;
    int error;

    if (init_done == 0) {
        init_done = 1;
    }

#ifdef LOG_SPI_INTO_FILE    
    fp_log = fopen("spi_log", "w+");
#endif

    spi_file = fopen(MV_SPI_DEVICE_NAME, "r+");
    if (!spi_file) {
        printf("Open SPI Flash Device Failed!");
        return NULL;
    }
    spi_fd = fileno(spi_file);

    if (spi_fd < 0) {
        printf("SPI Device File Description is Invalid!");
        return NULL;
    }

    spi_info.mst_id = MV_SPI_MASTERID;
    spi_info.slave_id = MV_SPI_SLAVEID;
    spi_info.xd_mode = SPIDEV_XDMODE2_WOA8D8n;

    spi_info.clock_mode = SPI_CLOCKMODE0_POL0PH0;

    spi_info.speed = 1000; // for SPI on RT-RK board
    spi_info.frame_width = SPI_FRAME_WIDTH8;

    error = ioctl(spi_fd, SPI_IOCTL_SETINFO, &spi_info);
    if (error != 0) {
        printf("SPI Device configuration FAILED!!\n");
        return NULL;
    }

    printf("SPI Device configured: %d MHz\n", spi_info.speed / 1000);

    return 0;

}

int32 CIMAX_SPI_Term() {
    int Status = 0;

#ifdef LOG_SPI_INTO_FILE        
    fclose(fp_log);
#endif    

    if (spi_file) {
        fclose(spi_file);
        spi_file = NULL;
        if (init_done == 1) {
            init_done = 0;
        }
    }

    return Status;
}

int32 CIMAX_SPI_Write(uint8 *buff, uint32 numOfBytes) {

    galois_spi_rw_t spi_rw;
    int idx, error;
    unsigned int *buffer = NULL;

    if (init_done == 0) {
        printf("Not able to write before init\n");
        return 1; // fail
    }

    buffer = (unsigned int *) malloc((numOfBytes) * sizeof (int));
    memset(buffer, 0, (numOfBytes) * sizeof (int));

    if (!buffer) {
        printf("Malloc Space for Write Buffer Failed!");
        return 1; // fail
    }


#ifdef LOG_SPI_INTO_FILE    

    char c[5];

    fputs("TX(", fp_log);
    sprintf(c, "%04d", numOfBytes);
    fputs(c, fp_log);
    fputs("): ", fp_log);
#endif    

    for (idx = 0; idx < numOfBytes; idx++) {
        buffer[idx] = (int) buff[idx];

#ifdef LOG_SPI_INTO_FILE    
        sprintf(c, "%02x", buffer[idx]);
        fputs(c, fp_log);
        fputs(" ", fp_log);
#endif        

    }


#ifdef LOG_SPI_INTO_FILE    
    fputs("\n", fp_log);
#endif    



    spi_rw.mst_id = MV_SPI_MASTERID;
    spi_rw.slave_id = MV_SPI_SLAVEID;

    spi_rw.wr_buf = buffer;
    spi_rw.wr_cnt = numOfBytes;
    spi_rw.rd_buf = NULL;
    spi_rw.rd_cnt = 0;

    error = ioctl(spi_fd, SPI_IOCTL_READWRITE, &spi_rw);
    if (0 != error) {
        free(buffer);
        printf("Error SPI_Write\n");
        return 1; // fail
    }

    free(buffer);

    return 0; // ok

}

int32 CIMAX_SPI_Read(uint8 *buff, uint32 numOfBytes) {

    galois_spi_rw_t spi_rw;
    int error;
    int i = 0;
    unsigned int *read_buf = NULL;
    char c[5];
    int readBytes = 0;
    int bytesToRead = 0;
    int counter = 0;

    read_buf = (unsigned int *) malloc(numOfBytes * sizeof (unsigned int));
    memset(read_buf, 0, (numOfBytes) * sizeof (unsigned int));



    spi_rw.mst_id = MV_SPI_MASTERID;
    spi_rw.slave_id = MV_SPI_SLAVEID;

    spi_rw.wr_buf = NULL;
    spi_rw.wr_cnt = 0;

    spi_rw.rd_buf = read_buf;
    spi_rw.rd_cnt = numOfBytes;

    if (numOfBytes > 256) {

        bytesToRead = numOfBytes;

        for (counter = 0; counter < (numOfBytes / 256); counter++) {

            spi_rw.rd_buf = read_buf + counter * 256;
            spi_rw.rd_cnt = 256;

            error = ioctl(spi_fd, SPI_IOCTL_READWRITE, &spi_rw);
            if (0 != error) {
                printf("Error SPI_Read\n");
                return 0;
            }

            readBytes += 256;
        }

        // read rest
        if (readBytes < numOfBytes) {

            spi_rw.rd_buf = read_buf + readBytes;
            spi_rw.rd_cnt = numOfBytes - readBytes;

            error = ioctl(spi_fd, SPI_IOCTL_READWRITE, &spi_rw);
            if (0 != error) {
                printf("Error SPI_Read\n");
                return 0;
            }

        }

#ifdef LOG_SPI_INTO_FILE    

        fputs("RX(", fp_log);
        sprintf(c, "%04d", numOfBytes);
        fputs(c, fp_log);
        fputs("): ", fp_log);

#endif

        for (i = 0; i < numOfBytes; i++) {
            buff[i] = (int) read_buf[i];

#ifdef LOG_SPI_INTO_FILE            
            sprintf(c, "%02x", buff[i]);
            fputs(c, fp_log);
            fputs(" ", fp_log);
#endif

        }

#ifdef LOG_SPI_INTO_FILE                
        fputs("\n", fp_log);
#endif


        free(read_buf);
        return numOfBytes;

    } else {


        error = ioctl(spi_fd, SPI_IOCTL_READWRITE, &spi_rw);
        if (0 != error) {
            printf("Error SPI_Read\n");
            return 0;
        }

#ifdef LOG_SPI_INTO_FILE    

        fputs("RX(", fp_log);
        sprintf(c, "%04d", numOfBytes);
        fputs(c, fp_log);
        fputs("): ", fp_log);

#endif

        for (i = 0; i < numOfBytes; i++) {
            buff[i] = (int) read_buf[i];

#ifdef LOG_SPI_INTO_FILE            
            sprintf(c, "%02x", buff[i]);
            fputs(c, fp_log);
            fputs(" ", fp_log);
#endif

        }

#ifdef LOG_SPI_INTO_FILE                
        fputs("\n", fp_log);
#endif

        free(read_buf);
        return numOfBytes;

    }
}

int32 CIMAX_PIO_Init() {
    int Status = 0;

    return Status;
}

int32 CIMAX_PIO_GetIntState(uint8 *pState) {

    return 0;
}

int32 CIMAX_PIO_Reset() {
    return 0;
}

int32 CIMAX_PIO_Term() {
    return 0;
}

