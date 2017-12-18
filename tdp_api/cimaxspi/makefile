# ****************************************
# CIMAX makefile
# ****************************************

include $(TDAL_MAKE)/environment.mak
include $(CHAL_BUILD_MAKE)/drivers.mak

# ****************************************
# Define module name
# ****************************************
MODULE  = cimaxspi

# ****************************************
# This define the default command for gmake
# ****************************************
all: $(LIB_PREFFIX)$(MODULE).$(LIB_SUFFIX) cp_firmware

# ****************************************
# Include paths needed to compile those files
# ****************************************
DIR_INCLUDE = \
	$(INCLUDE_PREFIX)$(TDAL_ROOT)/tdal_priv_inc/ \
	$(INCLUDE_PREFIX)$(TDAL_CI_INCLUDE) \
	$(INCLUDE_PREFIX)$(TDAL_COMMON_INCLUDE) \
	$(INCLUDE_PREFIX)$(DRIVERS_CFG_INCLUDE) \
	$(INCLUDE_PREFIX)$(DRIVERS_INCLUDE) \
	$(INCLUDE_PREFIX)$(TBOX_INCLUDE) \
	$(INCLUDE_PREFIX)./ \

# ***************************************************
# List of object files needed to create library
# ***************************************************
LIBOBJS = hal_os.$(OBJ_SUFFIX) cimax_spi_pio.$(OBJ_SUFFIX) cimax.$(OBJ_SUFFIX)

# ****************************************
# List of dependencies
# ****************************************
hal_os.$(OBJ_SUFFIX): os.h hal_os.c
cimax_spi_pio.$(OBJ_SUFFIX): cimax_spi_pio.h hal_os.$(OBJ_SUFFIX) cimax_spi_pio.c makefile
cimax.$(OBJ_SUFFIX): cimax_spi_pio.h cimax.h spi_bus.h hal_os.$(OBJ_SUFFIX) cimax_spi_pio.$(OBJ_SUFFIX) cimax.c makefile

# ****************************************
# spi firmware coping
# ****************************************
cp_firmware:
	$(CP) *.bin $(BUILD_EXPORTS)

# ****************************************
# Clean
# ****************************************
clean_all:
	rm -f *.tco *.o *.lib *.ab *.abg *.bdx *.hex *.map *.elf *.lku *.a

# ****************************************
# Always include crules.mak
# ****************************************
include $(BUILD_MAKE)/crules.mak
include $(COMEDIA_BUILD_MAKE)/comedia_exports.mak
