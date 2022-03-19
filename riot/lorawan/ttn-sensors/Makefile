# name of the application
APPLICATION = ttn-sensors

# The default board is ST B-L072Z-LRWAN1 LoRa discovery board
BOARD ?= b-l072z-lrwan1

BOARD_WITHOUT_LORAMAC_RX := \
    arduino-mega2560 \
    i-nucleo-lrwan1 \
    stm32f0discovery \
    waspmote-pro \

RIOTBASE ?= $(CURDIR)/../../RIOT
#RIOTBASE ?= /home/jovyan/work/old-training/riot/RIOT
#
# Here we add the modules that are needed
#
USEMODULE += ztimer_msec
USEMODULE += hts221
#
# The application needs LoRaWAN related modules and variables:
#

# The Semtech LoRa radio device (SX1276)
USEMODULE += sx1276

# The Semtech Loramac package
USEPKG += semtech-loramac
USEMODULE += $(LORA_DRIVER)

# load loramac RX if board supports it
ifeq (,$(filter $(BOARD),$(BOARD_WITHOUT_LORAMAC_RX)))
  USEMODULE += semtech_loramac_rx
endif

# Default region is Europe and default band is 868MHz
LORA_REGION ?= EU868

# This must be the last line of the Makefile
include $(RIOTBASE)/Makefile.include
