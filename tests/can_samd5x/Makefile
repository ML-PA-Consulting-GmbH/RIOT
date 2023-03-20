include ../Makefile.tests_common

FEATURES_REQUIRED += periph_can

BOARD = same54-xpro

USEMODULE += can
CFLAGS += -DISR_STACKSIZE="(1024)"
CFLAGS += -DCANDEV_SAMD5X_DEFAULT_TX_BUFFER_NUM=5

DEVELHELP ?= 1

include $(RIOTBASE)/Makefile.include