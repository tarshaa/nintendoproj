BLOCKSDS ?= /opt/blocksds/core
NAME := photo-worlds
GAME_TITLE := Photo Worlds
GAME_SUBTITLE := Camera and playable prototype
GAME_AUTHOR := Tarshaa
ifeq ($(PHOTO_NETWORK),1)
BLOCKSDSEXT ?= /opt/blocksds/external
NAME := photo-worlds-online
GAME_SUBTITLE := Automatic photo game test
ARM7ELF := $(BLOCKSDS)/sys/arm7/main_core/arm7_dswifi_maxmod.elf
CFLAGS += -DPHOTO_NETWORK
LIBS := -lcurl -lmbedtls -lmbedx509 -lmbedcrypto -lz -ldswifi9 -lnds9
LIBDIRS := $(BLOCKSDS)/libs/dswifi $(BLOCKSDS)/libs/libnds $(BLOCKSDSEXT)/mbedtls $(BLOCKSDSEXT)/libcurl
endif
include $(BLOCKSDS)/sys/default_makefiles/rom_arm9/Makefile
