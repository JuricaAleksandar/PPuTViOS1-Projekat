#include <stdio.h> 
#include <math.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "gpio_common.h"

int sFd;
FILE *spFile;


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

typedef struct galois_pinmux_data{
    int owner;              //  soc or sm pinmux
    int group;              //  group id
    int value;              //  group value
    char * requster;        //  who request the change
}galois_pinmux_data_t;


/*
 * GPIO functions
 */

int gpio_read(galois_gpio_data_t *gpio_data) {

    int fd;

    fd = open(GPIO_DEVICE, O_RDWR);
    if (!fd) {
        perror("Failed to open "GPIO_DEVICE".\n");
        return -1;
    }

    /* gpio_setmode: set GPIO port to mode */
    galois_gpio_data_t gpio_data_set;
    gpio_data_set.port = 0;
    gpio_data_set.mode = GPIO_MODE_DATA_IN;
    if (ioctl(fd, GPIO_IOCTL_SET, &gpio_data_set)) {
        perror("ioctl GPIO_IOCTL_SET error.\n");
        return -1;
    } else {
        //printf("Mode is set\n");
    }


    if (ioctl(fd, GPIO_IOCTL_READ, gpio_data)) {
        printf("ioctl READ error.\n");
        close(fd);
        return -1;
    }

    /* close */
    close(fd);
    return 0;
}



/*
 * Pinmux functions
 */

void close_pinmux(int pinmuxfd) {
    close(pinmuxfd);
}

int pinmux_ioctl(int pinmuxfd, int ioctlcode, galois_pinmux_data_t *pin) {
    int ret = 0;

    ret = ioctl(pinmuxfd, ioctlcode, pin);
    if (ret > 0) {
        printf("ioctl error, ret = 0x%x\r\n", ret);
        return -1;
    }
    return ret;
}

int pinmux_devopen(void) {
    int pinmuxfd;

    pinmuxfd = open("/dev/pinmux", O_RDWR);
    if (pinmuxfd <= -1) {
        printf("open /dev/pinmux error\n");
    }
    return pinmuxfd;
}

int set_pinmux(int owner, int group, int value) {
    int fd;
    struct galois_pinmux_data pin;
    char my[] = "test";
    int ret = 0;

    fd = pinmux_devopen();
    if (fd < 0)
        return fd;
    ;
    pin.owner = owner;
    pin.group = group; //SM_GROUP7;
    pin.value = value;
    pin.requster = my;

    ret = pinmux_ioctl(fd, PINMUX_IOCTL_SETMOD, &pin);
    close_pinmux(fd);

    printf("Set pinmux ret = %d\n", ret);
    return ret;
}


int gpio_open(galois_gpio_data_t gpio_data)
{
        spFile = fopen(GPIO_DEVICE, "rw+");
        if (!spFile) {
          printf("Failed to open GPIO_DEVICE.\n");
        return -1;
        }
        sFd = fileno(spFile);

        if (ioctl(sFd, GPIO_IOCTL_SET, &gpio_data)) {
                printf("ioctl GPIO_IOCTL_SET error.\n");
                fclose(spFile);
                return -1;
        }
        return 0;
}
int gpio_close()
{
           fclose(spFile);
           return 0;
}
int gpio_write(galois_gpio_data_t gpio_data)
{

        if (ioctl(sFd, GPIO_IOCTL_WRITE, &gpio_data))
        {
           printf("ioctl GPIO_IOCTL_WRITE error.\n");
           return -1;
        } 
        return 0;
}
int set_gpio(int port_no,int val)
{
        int error = 0;

        galois_gpio_data_t gpio_data;
        galois_gpio_data_t * pgpio_data=&gpio_data;
        gpio_data.port = port_no ;
        gpio_data.mode = 2;
        gpio_data.data = val;
        error |= gpio_open(gpio_data);
        error |= gpio_write(gpio_data);
        error |= gpio_close();        
        return error;
}


int sm_gpio_open(galois_gpio_data_t gpio_data)
{
        spFile = fopen(GPIO_DEVICE, "rw+");
        if (!spFile) {
          printf("Failed to open GPIO_DEVICE.\n");
        return -1;
        }
        sFd = fileno(spFile);

        if (ioctl(sFd, SM_GPIO_IOCTL_SET, &gpio_data)) {
                printf("ioctl GPIO_IOCTL_SET error.\n");
                fclose(spFile);
                return -1;
        }
        return 0;
}
int sm_gpio_close()
{
           fclose(spFile);
           return 0;
}
int sm_gpio_write(galois_gpio_data_t gpio_data)
{

        if (ioctl(sFd, SM_GPIO_IOCTL_WRITE, &gpio_data))
        {
           printf("ioctl GPIO_IOCTL_WRITE error.\n");
           return -1;
        } 
        return 0;
}
int sm_set_gpio(int port_no,int val)
{
        int error = 0;

        galois_gpio_data_t gpio_data;
        galois_gpio_data_t * pgpio_data=&gpio_data;
        gpio_data.port = port_no ;
        gpio_data.mode = 2;
        gpio_data.data = val;
        error |= sm_gpio_open(gpio_data);
        error |= sm_gpio_write(gpio_data);
        error |= sm_gpio_close();        
        return error;
}

