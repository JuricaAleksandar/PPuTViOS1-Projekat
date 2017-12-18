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

//#include <openssl/md5.h>
//#include "ErrorCode.h"
#include "spi_bus.h"

#define MV_SPI_PERR(fmt, ...) printf("[%s %d] E: "fmt"\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define MV_SPI_PINF(fmt, ...) printf("[%s %d] I: "fmt"\n", __FILE__, __LINE__, ##__VA_ARGS__)

#define MV_SPI_MASTERID			1		// SPI Master ID
#define MV_SPI_SLAVEID			0		// SPI Slave ID

#define MV_SPI_START_ADDR 		0x0		// SPI Flash Start Address
#define MV_SPI_HEADER_ADDR 		(MV_SPI_START_ADDR + 0x400000)	// 4M-bytes Offset for Image Header Adderss
#define MV_SPI_MAX_ADDR			0xFFFFFF	// 16MB

#define MV_SPI_FLASH_BLKSIZE	0x10000		// 64K-bytes Block Size
#define MV_SPI_FLASH_SECSIZE	0x1000		// 4K-bytes Sector Size
#define MV_SPI_FLASH_PGSIZE		0x100		// 256B	Page Size

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

extern char *optarg;
extern int optopt;

/*
 * SPI Image Header, 64 Bytes
 */
struct MV_SPI_Image_Header{
	char		image_md5[16]	;	// 128-bit MD5 Image Checksum
	char		header_md5[16]	;	// 128-bit MD5 Header Checksum
	int			image_len;			// Image Body Length
	char		reserve[28];		// Reserve 28 Bytes
};

int MV_SPI_Flash_Init(int spi_fd){
	galois_spi_info_t spi_info;
	int error;

	if(spi_fd < 0){
		MV_SPI_PERR("SPI Device File Description is Invalid!");
		return S_FALSE;
	}
	
	spi_info.mst_id = MV_SPI_MASTERID;
	spi_info.slave_id = MV_SPI_SLAVEID;
	spi_info.xd_mode = SPIDEV_XDMODE1_RWI8A24D8n;
	spi_info.clock_mode = SPI_CLOCKMODE3_POL1PH1;
	spi_info.speed = SPI_SPEED_200KHZ;
	spi_info.frame_width = SPI_FRAME_WIDTH8;
	
	error = ioctl(spi_fd, SPI_IOCTL_SETINFO, &spi_info);
	if(error != 0)
		return S_FALSE;
	
	return S_OK;
}

int MV_SPI_Get_Info(int spi_fd, galois_spi_info_t *spi_info){
	int error;

	if(spi_fd < 0){
		MV_SPI_PERR("SPI Device File Description is Invalid!");
		return S_FALSE;
	}
	
	error = ioctl(spi_fd, SPI_IOCTL_GETINFO, &spi_info);
	if(error != 0)
		return S_FALSE;

	return S_OK;
}

int MV_SPI_Print_Info(const galois_spi_info_t *spi_info){
    printf("[SPI Information]:\n");
	printf("Master ID: %d, Slave Address: 0x%x\n",
			spi_info->mst_id, spi_info->slave_id);
	printf("Clock Mode: %d, XD Mode: %d\n",
			spi_info->clock_mode, spi_info->xd_mode);
	printf("Speed: %d, Frame Width: %d, IRQ: %d, Usable: %d\n",
			spi_info->speed, spi_info->frame_width, spi_info->irq, spi_info->usable);
	return S_OK;
}

#if 0
static int MV_SPI_Compare_MD5(const char *p1, const char *p2){
	int md5_compare;

	if(!p1 || !p2){
		MV_SPI_PERR("MD5 Key is NULL!");
		return S_FALSE;
	}
	
	md5_compare = strncmp(p1, p2, 16);
	if(md5_compare != 0){
		MV_SPI_PERR("MD5 Key is NOT Match!");
		return S_FALSE;
	}

	return S_OK;
}
int MV_SPI_Image_Check(const void *upimg_ptr, int length){
	MD5_CTX md5_ctx;
	char md5[16];
	struct MV_SPI_Image_Header *image_header;
	int md5_equal;
	int header_length = sizeof(struct MV_SPI_Image_Header);
	
	if(!upimg_ptr || length == 0){
		MV_SPI_PERR("Update Image's Pointer is NULL or Length is Too Short!");
		return S_FALSE;
	}

	MD5_Init(&md5_ctx);
	MD5_Update(&md5_ctx, upimg_ptr, header_length);
	MD5_Final((unsigned char *)md5, &md5_ctx);
	
	image_header = (struct MV_SPI_Image_Header *)upimg_ptr;
	md5_equal = MV_SPI_Compare_MD5(md5, image_header->header_md5);
	if(md5_equal != S_OK){
		MV_SPI_PERR("Update Image's Header MD5 Check Failed!");
		return S_FALSE;
	}
	
	upimg_ptr = (const char *)upimg_ptr + header_length;
	
	MD5_Init(&md5_ctx);
	MD5_Update(&md5_ctx, upimg_ptr, (length - header_length));
	MD5_Final((unsigned char *)md5, &md5_ctx);

	md5_equal = MV_SPI_Compare_MD5(md5, image_header->image_md5);
	if(md5_equal != S_OK){
		MV_SPI_PERR("Update Image's Body MD5 Check Failed!");
		return S_FALSE;
	}
	
	return S_OK;
}
#endif
int MV_SPI_Flash_ReadStatus(int spi_fd, unsigned char *spi_status){
	galois_spi_rw_char_t spi_rw;
	int error;
	unsigned char buffer;

	buffer = MV_SPI_FLASH_RDSR;

	spi_rw.mst_id = MV_SPI_MASTERID;
	spi_rw.slave_id = MV_SPI_SLAVEID;
	
	spi_rw.wr_buf = &buffer;
	spi_rw.wr_cnt = 1;
	spi_rw.rd_buf = spi_status;
	spi_rw.rd_cnt = 1;
	error = ioctl(spi_fd, SPI_IOCTL_READWRITE_CHAR, &spi_rw);
	
	if(error != 0)
		return S_FALSE;
	
	return S_OK;
}

static int MV_SPI_Flash_ReadPage(int spi_fd, unsigned int read_addr, unsigned char *read_buf, int read_len){
	galois_spi_rw_char_t spi_rw;
	int count, idx, error;
	unsigned char command[4];
	unsigned char *buffer;

	count = (read_addr & (MV_SPI_FLASH_PGSIZE - 1)) + read_len;
	if(count > MV_SPI_FLASH_PGSIZE){
		MV_SPI_PERR("Read Area Exceed One Page Size: 0x%X!", MV_SPI_FLASH_PGSIZE);
		return S_FALSE;
	}
	
	command[0] = MV_SPI_FLASH_READ;
	command[1] = read_addr >> 16 & 0xFF;
	command[2] = read_addr >> 8 & 0xFF;
	command[3] = read_addr& 0xFF;
	
	buffer = malloc(read_len);
	if(!buffer){
		MV_SPI_PERR("Malloc %d Space for Read Buffer Failed!", read_len * sizeof(int));
		return S_FALSE;
	}
	spi_rw.mst_id = MV_SPI_MASTERID;
	spi_rw.slave_id = MV_SPI_SLAVEID;
	
	spi_rw.wr_buf = command;
	spi_rw.wr_cnt = 4;
	spi_rw.rd_buf = buffer;
	spi_rw.rd_cnt = read_len;

	error = ioctl(spi_fd, SPI_IOCTL_READWRITE_CHAR, &spi_rw);
	if(0 != error){
		free(buffer);
		return S_FALSE;
	}
	for(idx = 0; idx < read_len; idx++)
		read_buf[idx] = buffer[idx] & 0xFF;
	
	free(buffer);
	return S_OK;
}

int MV_SPI_Flash_WaitEnable(int spi_fd){
	unsigned char spi_status;
	int error;

	while(1){
		error = MV_SPI_Flash_ReadStatus(spi_fd, &spi_status);	
		if(S_OK != error)
			return S_FALSE;
		
		if(!(spi_status & MV_SPI_FLASH_WIP))
			break;
	}
	return S_OK;
}

int MV_SPI_Flash_Read(int spi_fd, unsigned int read_addr, unsigned char *read_buf, int read_len){
	int error, count, tmp_len;

	if(spi_fd < 0){
		MV_SPI_PERR("SPI Device File Descriptor is Invalid!");
		return S_FALSE;
	}
	
	if(read_addr > MV_SPI_MAX_ADDR){
		MV_SPI_PERR("Read Address exceeds SPI Flash's Capacity 0x%x!", MV_SPI_MAX_ADDR);
		return S_FALSE;
	}
	
	if(!read_buf){
		MV_SPI_PERR("Read Buffer Pointer is Null!");
		return S_FALSE;
	}

	if(read_len <= 0)
		return S_OK;
	
	if(read_addr & (MV_SPI_FLASH_PGSIZE - 1)){
		error = MV_SPI_Flash_WaitEnable(spi_fd);
		if(S_OK != error){
			MV_SPI_PERR("Wait SPI Flash to Enable Failed!");
			return S_FALSE;
		}

		tmp_len = MV_SPI_FLASH_PGSIZE - (read_addr & (MV_SPI_FLASH_PGSIZE - 1));
		count = (read_len > tmp_len) ? tmp_len : read_len;
	
		error = MV_SPI_Flash_ReadPage(spi_fd, read_addr, read_buf, count);
		if(S_OK != error){
			MV_SPI_PERR("Read SPI Flash Failed at Address: 0x%X!", read_addr);
			return S_FALSE;
		}
		read_buf += count;
		read_len -= count;
		read_addr += count;
	}
	
	if(read_len <= 0)
		return S_OK;
	
	while(read_len > 0){
		error = MV_SPI_Flash_WaitEnable(spi_fd);
		if(S_OK != error){
			MV_SPI_PERR("Wait SPI Flash to Enable Failed!");
			return S_FALSE;
		}
		
		count = (read_len > MV_SPI_FLASH_PGSIZE) ? MV_SPI_FLASH_PGSIZE : read_len;
		
		error = MV_SPI_Flash_ReadPage(spi_fd, read_addr, read_buf, count);
		if(S_OK != error){
			MV_SPI_PERR("Read SPI Flash Failed at Address: 0x%X!", read_addr);
			return S_FALSE;
		}
		read_buf += count;
		read_len -= count;
		read_addr += count;
	}
	
	return S_OK;
}

static int MV_SPI_Flash_WriteEnable(int spi_fd){
	int error;
	unsigned char command;
	galois_spi_rw_char_t spi_rw;
	
	command = MV_SPI_FLASH_WREN;
	
	spi_rw.mst_id = MV_SPI_MASTERID;
	spi_rw.slave_id = MV_SPI_SLAVEID;
	
	spi_rw.rd_buf = NULL;
	spi_rw.rd_cnt = 0;
	spi_rw.wr_buf = &command;
	spi_rw.wr_cnt = 1;
	
	error = ioctl(spi_fd, SPI_IOCTL_READWRITE_CHAR, &spi_rw);
	if(error != 0){
		return S_FALSE;
	}

	return S_OK;
}

static int MV_SPI_Flash_WritePage(int spi_fd, unsigned int write_addr, const unsigned char *write_buf, int write_len){
	galois_spi_rw_char_t spi_rw;
	int count, idx, error;
	unsigned char *buffer = NULL;

	count = (write_addr & (MV_SPI_FLASH_PGSIZE - 1)) + write_len;
	if(count > MV_SPI_FLASH_PGSIZE){
		MV_SPI_PERR("Write Area Exceed One Page Size: 0x%X!", MV_SPI_FLASH_PGSIZE);
		return S_FALSE;
	}
	
	buffer = malloc((4 + write_len));
	if(!buffer){
		MV_SPI_PERR("Malloc %d Space for Write Buffer Failed!", write_len);
		return S_FALSE;
	}
	
	buffer[0] = MV_SPI_FLASH_PP;
	buffer[1] = write_addr >> 16 & 0xFF;
	buffer[2] = write_addr >> 8 & 0xFF;
	buffer[3] = write_addr & 0xFF;
	
	for(idx = 0; idx < write_len; idx++)
		buffer[idx + 4] = write_buf[idx];
	
	spi_rw.mst_id = MV_SPI_MASTERID;
	spi_rw.slave_id = MV_SPI_SLAVEID;
	
	spi_rw.wr_buf = buffer;
	spi_rw.wr_cnt = 4 + write_len;
	spi_rw.rd_buf = NULL;
	spi_rw.rd_cnt = 0;
	
	error = ioctl(spi_fd, SPI_IOCTL_READWRITE_CHAR, &spi_rw);
	if(0 != error){
		free(buffer);
		return S_FALSE;
	}

	free(buffer);
	return S_OK;
}

int MV_SPI_Flash_Write(int spi_fd, unsigned int write_addr, const unsigned char *write_buf, int write_len){
	int error;
	int tmp_len;
	int count;

	if(spi_fd < 0){
		MV_SPI_PERR("SPI Device File Descriptor is Invalid!");
		return S_FALSE;
	}

	if(write_addr > MV_SPI_MAX_ADDR){
		MV_SPI_PERR("Write Address exceeds SPI Flash's Capacity 0x%x!", MV_SPI_MAX_ADDR);
		return S_FALSE;
	}

	if(!write_buf){
		MV_SPI_PERR("Write Buffer Pointer is NULL!");
		return S_FALSE;
	}
	
	if(write_len <= 0)
		return S_OK;
	
	if(write_addr & (MV_SPI_FLASH_PGSIZE - 1)){
		error = MV_SPI_Flash_WaitEnable(spi_fd);
		if(S_OK != error){
			MV_SPI_PERR("Wait SPI Flash to Enable Failed!");
			return S_FALSE;
		}
		
		error = MV_SPI_Flash_WriteEnable(spi_fd);
		if(S_OK != error){
			MV_SPI_PERR("Write Enable SPI Flash Failed!");
			return S_FALSE;
		}
		tmp_len = MV_SPI_FLASH_PGSIZE - (write_addr & (MV_SPI_FLASH_PGSIZE - 1));
		count = (write_len > tmp_len) ? tmp_len : write_len;
	
		error = MV_SPI_Flash_WritePage(spi_fd, write_addr, write_buf, count);
		if(S_OK != error){
			MV_SPI_PERR("Write SPI Flash Failed at Address: 0x%X!", write_addr);
			return S_FALSE;
		}
		write_buf += count;
		write_len -= count;
		write_addr += count;
	}

	if(write_len <= 0)
		return S_OK;
	
	while(write_len > 0){
		error = MV_SPI_Flash_WaitEnable(spi_fd);
		if(S_OK != error){
			MV_SPI_PERR("Wait SPI Flash to Enable Failed!");
			return S_FALSE;
		}
		
		error = MV_SPI_Flash_WriteEnable(spi_fd);
		if(S_OK != error){
			MV_SPI_PERR("Write Enable SPI Flash Failed!");
			return S_FALSE;
		}
		
		tmp_len = MV_SPI_FLASH_PGSIZE - (write_addr & (MV_SPI_FLASH_PGSIZE - 1));
		count = (write_len > tmp_len) ? tmp_len : write_len;
	
		error = MV_SPI_Flash_WritePage(spi_fd, write_addr, write_buf, count);
		if(S_OK != error){
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
int MV_SPI_Backup_Image(int spi_fd){
	int error;
	struct MV_SPI_Image_Header image_header;
	int header_length = sizeof(struct MV_SPI_Image_Header);
	int backup_fd, backup_len;
	struct stat backup_stat;
	void *backup_ptr;

	backup_fd = open(MV_SPI_BACKUP_FILE, O_RDWR | O_CREAT| O_TRUNC, 0666);
	if(backup_fd < 0){
		MV_SPI_PERR("Open SPI Backup File Failed!");
		return S_FALSE;
	}

	error = fstat(backup_fd, &backup_stat);
	if(error < 0){
		MV_SPI_PERR("Get SPI Backup File Status Failed!");
		error = S_FALSE;
		goto close_backup;
	}
	
	error = MV_SPI_Flash_Read(spi_fd, MV_SPI_HEADER_ADDR, (unsigned char *)&image_header, header_length);
	if(error != S_OK){
		MV_SPI_PERR("Read SPI Image Header Failed!");
		error = S_FALSE;
		goto close_backup;
	}
	
	backup_len = image_header.image_len + header_length;
	backup_ptr = mmap(NULL, backup_len, PROT_WRITE, MAP_PRIVATE, backup_fd, 0);
	if(MAP_FAILED == backup_ptr){
		MV_SPI_PERR("Memory Map SPI Backup File Failed!");
		error = S_FALSE;
		goto close_backup;
	}

	memcpy(backup_ptr, &image_header, header_length);
	error = MV_SPI_Flash_Read(spi_fd, MV_SPI_START_ADDR, (unsigned char *)(backup_ptr + header_length), image_header.image_len);
	if(error != S_OK){
		MV_SPI_PERR("Read SPI Image Body Failed!");
		error = S_FALSE;
		goto unmap_backup;
	}
	
	error = msync(backup_ptr, backup_len, MS_SYNC);
	if(error < 0){
		MV_SPI_PERR("Memory Sync SPI Backup File Failed!");
		error = S_FALSE;
		goto unmap_backup;
	}
	
	error = MV_SPI_Image_Check(backup_ptr, backup_len);
	if(error != S_OK){
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
int MV_SPI_Flash_WriteStatus(int spi_fd, unsigned char spi_status){
	int error;
	galois_spi_rw_char_t spi_rw;
	unsigned char buffer[2];

	error = MV_SPI_Flash_WaitEnable(spi_fd);
	if(S_OK != error){
		MV_SPI_PERR("Wait SPI Flash to Enable Failed!");
		return S_FALSE;
	}
	
	error = MV_SPI_Flash_WriteEnable(spi_fd);
	if(S_OK != error){
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

	error = ioctl(spi_fd, SPI_IOCTL_READWRITE_CHAR, &spi_rw);
	if(error != 0)
		return S_FALSE;
	
	return S_OK;
}

int MV_SPI_Flash_EraseBlock(int spi_fd, unsigned int erase_addr, int blk_num){
	int error;
	unsigned int idx;
	galois_spi_rw_char_t spi_rw;
	unsigned char buffer[4];

	if(erase_addr & (MV_SPI_FLASH_BLKSIZE - 1)){
		MV_SPI_PERR("Erase Address Must be Block Size (0x%X) Align!", MV_SPI_FLASH_BLKSIZE);
		return S_FALSE;
	}
	
	for(idx = 0; idx < blk_num; idx++){
		error = MV_SPI_Flash_WaitEnable(spi_fd);
		if(S_OK != error){
			MV_SPI_PERR("Wait SPI Flash to Enable Failed!");
			return S_FALSE;
		}
		
		error = MV_SPI_Flash_WriteEnable(spi_fd);
		if(S_OK != error){
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
		
		error = ioctl(spi_fd, SPI_IOCTL_READWRITE_CHAR, &spi_rw);
		if(error != 0)
			return S_FALSE;
	}
	return S_OK;
}
#if 0
int MV_SPI_Flash_Update(const char* upimg_path){
	int upimg_fd, spi_fd;
	FILE *spi_file;
	struct stat upimg_stat;
	int error, count;
	void *upimg_ptr;
	int header_length = sizeof(struct MV_SPI_Image_Header);
	int image_length;
	galois_spi_info_t spi_info;

	upimg_fd = open(upimg_path, O_RDONLY);
	if(upimg_fd < 0){
		MV_SPI_PERR("Open SPI Update Image Failed!");
		return S_FALSE;
	}
	
	error = fstat(upimg_fd, &upimg_stat);
	if(error < 0){
		MV_SPI_PERR("Get SPI Image Status Failed!");
		error = S_FALSE;
		goto exit_close;
	}
	
	upimg_ptr = mmap(NULL, upimg_stat.st_size, PROT_READ, MAP_PRIVATE, upimg_fd, 0);
	if(MAP_FAILED == upimg_ptr){
		MV_SPI_PERR("Memory Map SPI Image Failed!");
		error = S_FALSE;
		goto exit_close;
	}
	
	// Check Update Image
	error = MV_SPI_Image_Check(upimg_ptr, upimg_stat.st_size);
	if(error != S_OK){
		MV_SPI_PERR("Check SPI Image Failed!");
		error = S_FALSE;
		goto exit_unmap;
	}
	MV_SPI_PINF("SPI Update Image Check Finished!");
	
	// Initialize SPI Flash Device
	spi_file = fopen(MV_SPI_DEVICE_NAME, "r+");
	if(!spi_file){
		MV_SPI_PERR("Open SPI Flash Device Failed!");
		error = S_FALSE;
		goto exit_unmap;
	}
	spi_fd = fileno(spi_file);

	error = MV_SPI_Flash_Init(spi_fd);
	if(error != S_OK){
		MV_SPI_PERR("SPI Flash Initialize Failed!");
		error = S_FALSE;
		goto exit_unmap;
	}
	MV_SPI_PINF("SPI Flash is Initialized!");
	
	error = MV_SPI_Get_Info(spi_fd, &spi_info);
	if(error != S_OK){
		MV_SPI_PERR("Get SPI Flash Infomation Failed!");
		error = S_FALSE;
		goto exit_unmap;
	}
	
	MV_SPI_Print_Info(&spi_info);
	
	// Backup SPI Image
	error = MV_SPI_Backup_Image(spi_fd);
	if(error != S_OK){
		MV_SPI_PERR("SPI Flash Backup Failed!");
		error = S_FALSE;
		// need not to quit
	}
	if(error != S_FALSE)
		MV_SPI_PINF("SPI Image is Backup to %s!", MV_SPI_BACKUP_FILE);

	// Erase SPI Flash
	image_length = upimg_stat.st_size - header_length;
	count = (image_length + MV_SPI_FLASH_BLKSIZE - 1) / MV_SPI_FLASH_BLKSIZE;
	error = MV_SPI_Flash_EraseBlock(spi_fd, MV_SPI_START_ADDR, count);
	if(error != S_OK){
		MV_SPI_PERR("Erase SPI Flash Body Failed!");
		error = S_FALSE;
		goto exit_unmap;
	}
	
	count = (header_length + MV_SPI_FLASH_BLKSIZE - 1) / MV_SPI_FLASH_BLKSIZE;
	error = MV_SPI_Flash_EraseBlock(spi_fd, MV_SPI_HEADER_ADDR, count);
	if(error != S_OK){
		MV_SPI_PERR("Erase SPI Flash Partial Header Failed!");
		error = S_FALSE;
		goto exit_unmap;
	}
	
	// Write SPI Image
	error = MV_SPI_Flash_Write(spi_fd, MV_SPI_START_ADDR, (const unsigned char *)(upimg_ptr + header_length), image_length);
	if(error != S_OK){
		MV_SPI_PERR("Write SPI Flash Body Failed!");
		error = S_FALSE;
		goto exit_unmap;
	}
	MV_SPI_PINF("Update SPI Image Body Complete!");

	error = MV_SPI_Flash_Write(spi_fd, MV_SPI_HEADER_ADDR, (const unsigned char *)upimg_ptr, header_length);
	if(error != S_OK){
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

int main(int argc, char **argv){
	FILE *spi_file = NULL;
	char *image_path = NULL;
	char *image_ptr = NULL;
	int image_fd, spi_fd;
	int error, idx, count;
	int ch;
	struct stat image_stat;
	int spi_addr;
	unsigned char *check_ptr = NULL;

	if(argc != 5){
		printf("Usage: %s -f IMAGE_FILE -a WRITE_OFFSET\n", argv[0]);
		error = S_FALSE;
		goto exit_ret;
	}
	
	while((ch = getopt(argc, argv, "f:a:")) != -1){
		switch(ch){
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
	
	if(error == S_FALSE)
		goto exit_ret;
	
	if(spi_addr < 0 || spi_addr > MV_SPI_MAX_ADDR){
		MV_SPI_PERR("Address %x is not supported!", spi_addr);
		error = S_FALSE;
		goto exit_ret;
	}

	if(!image_path){
		MV_SPI_PERR("SPI Flash Image Path is NULL!");
		error = S_FALSE;
		goto exit_ret;
	}
	
	image_fd = open(image_path, O_RDONLY);
	if(image_fd < 0){
		MV_SPI_PERR("Open SPI Flash Image '%s' Failed!", image_path);
		error = S_FALSE;
		goto exit_ret;
	}
	
	error = fstat(image_fd, &image_stat);
	if(error < 0){
		MV_SPI_PERR("Get SPI Image Status Failed!");
		error = S_FALSE;
		goto exit_close;
	}

	image_ptr = mmap(NULL, image_stat.st_size, PROT_READ, MAP_PRIVATE, image_fd, 0);
	if(MAP_FAILED == image_ptr){
		MV_SPI_PERR("Memory Map SPI Image Failed!");
		error = S_FALSE;
		goto exit_close;
	}
	
	// Initialize SPI Flash Device
	spi_file = fopen(MV_SPI_DEVICE_NAME, "r+");
	if(!spi_file){
		MV_SPI_PERR("Open SPI Flash Device Failed!");
		error = S_FALSE;
		goto exit_unmap;
	}
	
	spi_fd = fileno(spi_file);

	error = MV_SPI_Flash_Init(spi_fd);
	if(error != S_OK){
		MV_SPI_PERR("SPI Flash Initialize Failed!");
		error = S_FALSE;
		goto exit_unmap;
	}
//	MV_SPI_PINF("SPI Flash is Initialized!");

	count = (image_stat.st_size + MV_SPI_FLASH_BLKSIZE - 1) / MV_SPI_FLASH_BLKSIZE;
	MV_SPI_PINF("Erasing %d Blocks, form 0x%x to 0x%x...", count, spi_addr, spi_addr + MV_SPI_FLASH_BLKSIZE * count - 1);
	error = MV_SPI_Flash_EraseBlock(spi_fd, spi_addr, count);
	if(error != S_OK){
		MV_SPI_PERR("Erase SPI Flash Failed!");
		error = S_FALSE;
		goto exit_unmap;
	}
	
	MV_SPI_PINF("Writing SPI Image at 0x%x, Length: %d...", spi_addr, image_stat.st_size);
	error = MV_SPI_Flash_Write(spi_fd, spi_addr, image_ptr, image_stat.st_size);
	if(error != S_OK){
		MV_SPI_PERR("Failed!");
		error = S_FALSE;
		goto exit_unmap;
	}

	MV_SPI_PINF("Checking SPI Flash at 0x%x, Length: %d...", spi_addr, image_stat.st_size);
	check_ptr = malloc(image_stat.st_size);
	if(!check_ptr){
		MV_SPI_PERR("Malloc for Check Buffer Failed!");
		error = S_FALSE;
		goto exit_free;
	}
	error = MV_SPI_Flash_Read(spi_fd, spi_addr, check_ptr, image_stat.st_size);
	if(error != S_OK){
		MV_SPI_PERR("Read SPI Flash Failed!");
		error = S_FALSE;
		goto exit_free;
	}

	error = strncmp(image_ptr, check_ptr, image_stat.st_size);
	if(error != 0){
		MV_SPI_PERR("Falied, Please Try to ReWrite!");
		error = S_FALSE;
		goto exit_free;
	}else{
		MV_SPI_PINF("Done, Write Successfully!");
	}

	error = S_OK;

exit_free:
	if(check_ptr){
		free(check_ptr);
		check_ptr = NULL;
	}
exit_unmap:
	if(image_ptr){
		munmap(image_ptr, image_stat.st_size);
		image_ptr = NULL;
	}
exit_close:
	if(spi_file){
		fclose(spi_file);
		spi_file = NULL;
	}
	if(image_fd){
		close(image_fd);
		image_fd = 0;
	}
exit_ret:
	return error;
}
