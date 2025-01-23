include ../Makefile.drivers_common

USEMODULE += shell
USEMODULE += can
USEMODULE += isrpipe

FEATURES_REQUIRED += periph_can
USEMODULE += ztimer_msec
FEATURES_BLACKLIST=no_idle_thread

CFLAGS += -DISR_STACKSIZE="(2048)"
CFLAGS += -DDEBUG_ASSERT_VERBOSE

include $(RIOTBASE)/Makefile.include
