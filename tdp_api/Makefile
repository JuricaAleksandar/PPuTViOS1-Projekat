CROSS_COMPILE=/home/korisnik/vjezbe_pputvios/marvell_linux/armv5-marvell-linux-gnueabi-softfp/bin/arm-marvell-linux-gnueabi

CC_PREFIX=$(CROSS_COMPILE)-
CC=$(CC_PREFIX)gcc
CXX=$(CC_PREFIX)g++
LD=$(CC_PREFIX)ld

GALOIS_INCLUDE=../marvell-sdk-1046/rootfs/home/galois/include
SYSROOT=/home/korisnik/vjezbe_pputvios/marvell_linux/marvell-sdk-1046/rootfs
INCS =	-I./include/ 							\
		-I$(SYSROOT)/usr/include/         \
		-I$(GALOIS_INCLUDE)/Common/include/     \
		-I$(GALOIS_INCLUDE)/OSAL/include/		\
		-I$(GALOIS_INCLUDE)/OSAL/include/CPU1/	\
		-I$(GALOIS_INCLUDE)/PE/Common/include/
		
INCS += -I./cimaxspi/

#INCS += -I./M88TS2022_adapt/ \

INCS += -I./Sony_Demod/include/ \
        -I./Sony_Demod/SRC/NuTuner/ \
        -I./Sony_Demod/SRC/i2c/ \
        -I./Sony_Demod/SRC/i2c/i2c_feusb/ \
        -I./
        
CFLAGS += -DNuTune_Tuner -D__LINUX__ -O0 -Wno-psabi --sysroot=$(SYSROOT)

LIBS_PATH = -L$(SYSROOT)/home/galois/lib/	\

LIBS := $(LIBS_PATH) -lOSAL	-lshm -lPEAgent

CXXFLAGS = $(CFLAGS)

all: tune_test

OBJS = i2c_adapt.o mt_fe_dmd_ds3000.o mt_fe_i2c.o mt_fe_tn_montage_ts2022.o tune_test.o

#SRCS = ./M88TS2022_adapt/i2c_adapt.c ./M88TS2022_adapt/mt_fe_dmd_ds3000.c ./M88TS2022_adapt/mt_fe_i2c.c ./M88TS2022_adapt/mt_fe_tn_montage_ts2022.c tune_test.c gpio_common.c

SRCS =  ./Sony_Demod/SRC/sony_dvb.c \
		./Sony_Demod/SRC/sony_dvb_cxd2820.c\
		./Sony_Demod/SRC/sony_dvb_demod.c \
		./Sony_Demod/SRC/sony_dvb_demod_monitorT2.c \
		./Sony_Demod/SRC/sony_dvb_demod_monitorT.c \
		./Sony_Demod/SRC/sony_dvb_demod_monitorC.c \
		./Sony_Demod/SRC/sony_dvb_demod_monitor.c \
		./Sony_Demod/SRC/sony_dvb_math.c \
		./Sony_Demod/SRC/sony_dvb_i2c.c \
		./Sony_Demod/SRC/i2c/i2c_feusb/drvi2c_feusb.c \
		./Sony_Demod/SRC/NuTuner/nutune_FT3114.c \
		./Sony_Demod/SRC/NuTuner/nutune_FT3114_I2C.c \
		./gpio_common.c \
		./i2c.c \
		./tdp_api.c

#SRCS += ./cimaxspi/cimax.c ./cimaxspi/cimax_spi_pio.c ./cimaxspi/hal_os.c

tune_test:
	$(CC) -o libtdp.so $(INCS) $(SRCS) $(CFLAGS) $(LIBS) -fPIC -shared
    
clean:
	rm -f libtdp.so
