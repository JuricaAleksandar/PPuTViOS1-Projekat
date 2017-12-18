#################################################################################
# Main Makefile
#################################################################################
TARGETDIR ?= bin

# IMPORTANT: check if given path will mtch your configuration
#################################################################################
GALOIS_LINUX_ROOT ?= /media/Android/LinuxB0/rootfs
GALOIS_PATH ?= $(GALOIS_LINUX_ROOT)/home/galois
#################################################################################

BUILD ?= marvell

ONAME := tunetest

OBJ_DIR:=obj/$(ONAME)
INS_DIR:= $(TARGETDIR)/

BUILD_TYPE := $(word 1,$(MAKECMDGOALS))
ifeq ($(BUILD_TYPE),)
   BUILD_TYPE := debug
endif

RM := rm -vrf
MKDIR := mkdir -p


#################################################################################
# Define list of files
#################################################################################
SRCS += mt_fe_main.c \
		mt_fe_dmd_ds3000.c \
        mt_fe_i2c.c \
        mt_fe_tn_montage_ts2022.c \
        i2c.c
           

ifeq ($(BUILD),marvell)

	include config
	CFLAGS += $(Galois_CFLAGS)
	EXTRA_CFLAGS += $(LINUX_LDFLAGS)
	#LIB_PATHS = -L$(GALOIS_PATH)/Builds/Linux/lib -lPEAgent -lOSAL
	export GALOIS_LINUX_ROOT

endif		


#################################################################################
# Create objects list based on source files
#################################################################################

OBJS   := $(foreach file,$(SRCS),$(OBJ_DIR)/$(strip $(basename $(file))).o)
DEPS   := $(foreach file,$(SRCS),$(OBJ_DIR)/$(strip $(basename $(file))).d)

#################################################################################
# General compiler flags
#################################################################################
CFLAGS +=  -g $(INC_PATHS) $(LIB_PATHS)
CFLAGS += -D_DEBUG -DDEBUG -DSKYPEKIT -DENABLE_DEBUG -DMINIMAL_MODE

EXTRA_CFLAGS += $(LIB_PATHS)

##################################################################################
# Build types
#################################################################################

ifeq ($(BUILD_TYPE),clean)
  TARGET := cleanup
endif
ifeq ($(BUILD_TYPE),debug)
  TARGET := build
endif

.PHONY: $(BUILD_TYPE)
$(BUILD_TYPE): $(TARGET)
	@echo -n



#################################################################################
# Build rules
#################################################################################

$(OBJ_DIR)/%.o:  %.cpp 
	@echo Compiling $<
	-$(MKDIR) "$(@D)"
	@$(CC) --sysroot=$(GALOIS_LINUX_ROOT) -Wall -c $(CFLAGS) $(DEFINES) -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"

$(OBJ_DIR)/%.o:  %.c 
	@echo Compiling $<
	-$(MKDIR) "$(@D)"
	@$(CC) --sysroot=$(GALOIS_LINUX_ROOT)  -Wall -c $(CFLAGS) $(DEFINES) -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	
build: $(OBJS)
	@echo 'Building target: $(ONAME)'
	-$(MKDIR) $(INS_DIR)
	$(CC) --sysroot=$(GALOIS_LINUX_ROOT) -Wall $(EXTRA_CFLAGS) -o $(INS_DIR)/$(ONAME) $(OBJS) $(LIBS)
	@echo 'Finished building target: $(ONAME)'
	@echo ' '
	
cleanup:
	-$(RM)  $(OBJ_DIR) $(INS_DIR)/$(ONAME)
	@echo 'Intermediate files removed'


