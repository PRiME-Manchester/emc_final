##------------------------------------------------------------------------------
##
## Makefile        Makefile for a simple SpiNNaker application
##
## Copyright (C)   The University of Manchester - 2013
##
## Author          Steve Temple, APT Group, School of Computer Science
##
## Email           temples@cs.man.ac.uk
##
##------------------------------------------------------------------------------

# Makefile for a simple SpiNNaker application. This will compile
# a single C source file into an APLX file which can be loaded onto
# SpiNNaker. It will link with either a 'bare' SARK library or a
# combined SARK/API library.

# The options below can be overridden from the command line or via
# environment variables. For example, to compile and link "my_example.c"
# with the ARM tools and generate ARM (as opposed to Thumb) code
#
# make APP=my_example GNU=0 THUMB=0

# Name of app (derived from C source - eg sark.c)
APP := emc

# Set to 1 to make Thumb code (0 for ARM)
THUMB := 1

# Configuration options
# Set to 1 if using SARK/API (0 for SARK)
API := 1

#-------------------------------------------------------------------------------
# Common includes for making SpiNNaker binaries

ifndef GNU
    GNU := 1
endif

ifndef LIB
    LIB := 0
endif

# If SPINN_DIRS is not defined, this is an error!
ifndef SPINN_DIRS
    $(error SPINN_DIRS is not set.  Please define SPINN_DIRS (possibly by running "source setup" in the spinnaker tools folder))
endif
SPINN_LIB_DIR = $(SPINN_DIRS)/lib
SPINN_INC_DIR = $(SPINN_DIRS)/include
SPINN_TOOLS_DIR = $(SPINN_DIRS)/tools


# ------------------------------------------------------------------------------
# Tools

ifeq ($(GNU),1)
  GP := arm-none-eabi
  AS := $(GP)-as --defsym GNU=1 -mthumb-interwork -march=armv5te
  
  ifeq ($(LIB), 1)
    CA := $(GP)-gcc -c -Os -mthumb-interwork -march=armv5te -std=gnu99 -I $(SPINN_INC_DIR)
    CFLAGS += -fdata-sections -ffunction-sections
    LD := $(GP)-ld -i
  else
    CA := $(GP)-gcc -mthumb-interwork -march=armv5te -std=gnu99 -I $(SPINN_INC_DIR) -c
    LD := $(GP)-gcc -T$(SPINN_LIB_DIR)/sark.lnk -Wl,-e,cpu_reset -Wl,-static -fdata-sections -ffunction-sections -Wl,--gc-sections -Wl,--use-blx -nostartfiles -static
  endif

	ifeq ($(API),1)
	#  LIBRARY := -L$(LIB_DIR) -lspin1_api
	  LIBRARY := $(SPINN_LIB_DIR)/libspin1_api.a
	else
	#  LIBRARY := -L$(LIB_DIR) -lsark
	  LIBRARY := $(SPINN_LIB_DIR)/libsark.a
	endif
  
  CC := $(CA) -mthumb -DTHUMB
  AR := $(GP)-ar -rcs
  OC := $(GP)-objcopy
  OD := $(GP)-objdump -dxt > $(APP).txt
  NM := $(GP)-nm

else
  AS := armasm --keep --cpu=5te --apcs /interwork
  CA := armcc -c --c99 --cpu=5te -Ospace --apcs /interwork --min_array_alignment=4 -I $(SPINN_INC_DIR)
  CC := $(CA) --thumb -DTHUMB
  
  ifeq ($(LIB), 1)
    CFLAGS += --split_sections
    LD := armlink --partial
  else
    LD = armlink --scatter=$(SPINN_TOOLS_DIR)/sark.sct --remove --entry cpu_reset
  endif

  AR := armar -rsc
  OC := fromelf
  OD := fromelf -cds --output
  NM := nm

endif

RM := rm -f
CAT = cat
LS  = ls -l

#-------------------------------------------------------------------------------

# Build the application

# List of objects making up the application. If there are other files
# in the application, add their object file names to this variable.

OBJECTS := $(APP).o


# Primary target is an APLX file - built from the ELF

#  1) Create a binary file which is the concatenation of RO and RW sections
#  2) Make an APLX header from the ELF file with "mkaplx" and concatenate
#     that with the binary to make the APLX file
#  3) Remove temporary files and "ls" the APLX file

$(APP).aplx: $(APP).elf
ifeq ($(GNU),1)
	$(OC) -O binary -j RO_DATA -j .ARM.exidx $(APP).elf RO_DATA.bin
	$(OC) -O binary -j RW_DATA $(APP).elf RW_DATA.bin
	mkbin RO_DATA.bin RW_DATA.bin > $(APP).bin
else
	$(OC) --bin --output $(APP).bin $(APP).elf
endif
	mkaplx $(APP).elf | $(CAT) - $(APP).bin > $(APP).aplx
	$(RM) $(APP).bin RO_DATA.bin RW_DATA.bin
	$(LS) $(APP).aplx


# Build the ELF file

#  1) Make a "sark_build.c" file containing app. name and build time
#     with "mkbuild" and compile it
#  2) Link application object(s), build file and library to make the ELF
#  3) Tidy up temporaries and create a list file

$(APP).elf: $(OBJECTS) $(SCRIPT) $(LIBRARY) 
	mkbuild $(APP) > sark_build.c
	$(CC) sark_build.c
	$(LD) $(LFLAGS) $(OBJECTS) sark_build.o $(LIBRARY) -o $(APP).elf
	$(RM) sark_build.c sark_build.o
	$(OD) $(APP).elf


# Build the main object file. If there are other files in the
# application, place their build dependencies below this one.

$(APP).o: $(APP).c crc.c $(SPINN_INC_DIR)/spinnaker.h $(SPINN_INC_DIR)/sark.h $(SPINN_INC_DIR)/spin1_api.h
	$(CC) $(CFLAGS) -Wall $(APP).c

# Tidy and cleaning dependencies

tidy:
	$(RM) $(OBJECTS) $(APP).elf $(APP).txt
clean:
	$(RM) $(OBJECTS) $(APP).elf $(APP).txt $(APP).aplx

#-------------------------------------------------------------------------------
