#!/bin/bash

export EXTRA_PARAMS="-DEFI_LUA=FALSE"
export DEBUG_LEVEL_OPT="-O0 -ggdb -g -Wl,--defsym=FLASH_SIZE=768k"
export INCLUDE_ELF=yes
bash ../common_make.sh proteus ARCH_STM32F7
