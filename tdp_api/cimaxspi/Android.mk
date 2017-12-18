LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

include $(TDAL_MAKE)/environment.mak
include $(CHAL_BUILD_MAKE)/drivers.mak

LOCAL_SRC_FILES:= \
           hal_os.c \
           cimax_spi_pio.c \
           cimax.c

LOCAL_C_INCLUDES:= \
        $(TDAL_ROOT)/tdal_priv_inc/ \
        $(TDAL_CI_INCLUDE) \
        $(TDAL_COMMON_INCLUDE) \
        $(DRIVERS_CFG_INCLUDE) \
        $(DRIVERS_INCLUDE) \
        $(TBOX_INCLUDE) \
        ./ \
        $(KERNEL_DVB_INCLUDE) \
        $(COMEDIA_CONF_INCLUDE)

LOCAL_MODULE := libcimaxspi

include $(BUILD_MAKE)/crules.mak
include $(COMEDIA_BUILD_MAKE)/comedia_exports.mak

include $(BUILD_STATIC_LIBRARY)
