#include <stdio.h>
#include <sys/ioctl.h>
#ifndef uint
#ifndef ANDROID
typedef unsigned int uint;
#endif
#endif
#include "i2c_adapt.h"
#include <errno.h>


/* slv_addr */
#define TWSI_SLVADDR_THS8200a	    0x20
#define TWSI_SLVADDR_CS4382A		0x18

#define TWSI_BUS_ID 0
#define TOTAL_TWSI	4

#define MT_FE_DEBUG							0		/*	0 off, 1 on*/
#if (MT_FE_DEBUG == 1)
	#define mt_fe_print(str, ...)				printf (str, __VA_ARGS__);
#else
	#define mt_fe_print(...)
#endif
   
static char twsi_bus_dev_name[TOTAL_TWSI][16] = {"/dev/twsi0", "/dev/twsi1", "/dev/twsi2","/dev/twsi3"};
static FILE * twsi_fp[TOTAL_TWSI] = {0, 0, 0, 0};
static int twsi_fd[TOTAL_TWSI] = {-1, -1, -1, -1};


static int TWSI_init(int master_id) 
{
	FILE * fp;
	int fd;

	if(master_id < 0 || master_id >= TOTAL_TWSI || twsi_fp[master_id]) {
		return -1;
	}

	fp = fopen(twsi_bus_dev_name[master_id], "r+");
	if(fp == NULL) {
		mt_fe_print("Fail to open\n");
		return -1;
	} else {
		mt_fe_print("Success to open\n");
	}
	fd = fileno(fp);
	twsi_fp[master_id] = fp; 
	twsi_fd[master_id] = fd;
	return 0;
}

static int TWSI_setspeed(int master_id, int type, int speed)
{
	mt_fe_print("Set speed master_id = %d !\n", twsi_fd[master_id]);
	galois_twsi_speed_t	twsi_speed;

	twsi_speed.mst_id = master_id;  /* user input: master num you're operating */
	twsi_speed.speed_type = type; /* user input: TWSI_STANDARD_SPEED or TWSI_CUSTOM_SPEED */
	twsi_speed.speed = speed;     /* user input: if speed==0, user old setting, else update twsi speed */
	if(ioctl(twsi_fd[master_id], TWSI_IOCTL_SETSPEED, &twsi_speed)) {
		mt_fe_print("Fail to set speed!\n");
		return -1;
	}
	return 0;
}

int I2C_Init(void)
{
	mt_fe_print("I2C initialization... \n");
	
	// Initialization
	if (TWSI_init(TWSI_BUS_ID) != 0)
	    return -1;
	
	// Set TWSI Speed here
	if (TWSI_setspeed(TWSI_BUS_ID, TWSI_CUSTOM_SPEED, 100) != 0 ) // set 100KHz
	    return -1;
	    
	return 0;    
}

static int TWSI_writeread_bytes(int master_id, int slv_addr, int addr_type, unsigned char *pwbuf, int wnum, unsigned char *prbuf, int rnum, int dum) 
{

	galois_twsi_rw_t twsi_rw;

	if(master_id < 0 || master_id >= TOTAL_TWSI || twsi_fp[master_id] == NULL) {
		mt_fe_print("Wrong bus!\n");
		return -1;
	}

	twsi_rw.mst_id = master_id;
	twsi_rw.slv_addr = slv_addr;
	twsi_rw.addr_type = addr_type;
	twsi_rw.rd_cnt = rnum;
	twsi_rw.rd_buf = prbuf;
	twsi_rw.wr_cnt = wnum;
	twsi_rw.wr_buf = pwbuf;

	if(ioctl(twsi_fd[master_id], TWSI_IOCTL_READWRITE, &twsi_rw)) {
		perror("read");
		return -1;
	}
	return 0;
}


int mt_fe_set_one_register(uint8_t dev_addr, uint8_t reg, uint8_t value)
{
	unsigned char pwbuf[10];
	unsigned char prbuf[10];
	int rnum ;
	int wnum ;

	// set register address
	pwbuf[0] = reg;     //subaddr, register offset
	pwbuf[1] = value;   //value
	rnum = 0;
	wnum = 2;
	if (TWSI_writeread_bytes(TWSI_BUS_ID, dev_addr, 0, pwbuf, wnum, prbuf, rnum, 0) != 0) 
	{
		mt_fe_print("WriteReg	ERROR\n");
		return -1;
	}
	
    return 0;
}

int mt_fe_get_one_register(uint8_t dev_addr, uint8_t reg, uint8_t* ret_buf)
{
	unsigned char pwbuf[10];
	int rnum ;
	int wnum ;

//	// set register address
//	pwbuf[0] = reg;     //subaddr, register offset
//	rnum = 1;
//	wnum = 1;
//	if (TWSI_writeread_bytes(TWSI_BUS_ID, dev_addr, 0, pwbuf, wnum, ret_buf, rnum, 0) != 0) 
//	{
//		mt_fe_print("ReadReg	ERROR\n");
//		return -1;
//	}


	// set register address
	pwbuf[0] = reg;     //subaddr, register offset
	rnum = 0;
	wnum = 1;
	if (TWSI_writeread_bytes(TWSI_BUS_ID, dev_addr, 0, pwbuf, wnum, ret_buf, rnum, 0) != 0) 
	{
		mt_fe_print("Write subaddress failed. \n");
		return -1;
	}
		// set register address
	pwbuf[0] = 0x00;     //subaddr, register offset
	rnum = 1;
	wnum = 0;
	if (TWSI_writeread_bytes(TWSI_BUS_ID, dev_addr, 0, pwbuf, wnum, ret_buf, rnum, 0) != 0) 
	{
		mt_fe_print("ReadReg	ERROR\n");
		return -1;
	}
	
    return 0;
}


int mt_fe_write_fw(uint8_t dev_addr, uint8_t reg, uint8_t* buff, uint16_t nbytes)
{
	unsigned char pwbuf[70];
	unsigned char prbuf[10];	
	int rnum ;
	int wnum ;
    int i;
    
    pwbuf[0] = reg;
    for(i=0; i<nbytes; i++)                           
    {                           
    	pwbuf[1+i] = buff[i];                           
    }                      
	rnum = 0;              
	wnum = 1 + nbytes;     
                           
	if (TWSI_writeread_bytes(TWSI_BUS_ID, dev_addr, 0, pwbuf, wnum, prbuf, rnum, 0) != 0) 
	{                      
		mt_fe_print("WriteReg ERROR\n");
		return -1;         
	}                      
	
    return 0;
}


int I2C_Close(void)
{
   	fclose(twsi_fp[TWSI_BUS_ID]);
	
	return 0;
}
