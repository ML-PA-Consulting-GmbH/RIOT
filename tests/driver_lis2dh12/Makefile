include ../Makefile.tests_common

# use lis2dh12_spi for SPI-Mode and lis2dh12 for I2C-Mode
DRIVER ?= lis2dh12

USEMODULE += fmt
USEMODULE += xtimer
USEMODULE += $(DRIVER)

include $(RIOTBASE)/Makefile.include
