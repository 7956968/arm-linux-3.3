sinclude $(PWD)/../module_conf
PWD := $(shell pwd)

EXTRA_CFLAGS += -fno-common -DMPFX=$(MODULE) -DEXPORT_SYMTAB -I$(SDK_MODULE_SRC)/include/pwm
MODULE = ftpwmtmr010
STRIP = $(CROSS_COMPILE)strip
STRIPFLAGS = -g --strip-unneeded

ifneq ($(KERNELRELEASE),)
ifeq ($(CONFIG_PLATFORM_GM8287),y)
$(MODULE)-objs := gm8287.o
endif
obj-m += $(MODULE).o
obj-m := $(sort $(obj-m))
else
ifndef SDK_MODULE_SRC
SDK_MODULE_SRC = $(SDK_SRC)/module/
endif

modules:
	$(MAKE) -C $(LINUX_SRC) SDK_MODULE_SRC=$(SDK_MODULE_SRC) M=$(PWD) modules
	find -name \*.ko | xargs $(STRIP) $(STRIPFLAGS)

.PHONEY: clean
clean:
	$(MAKE) -C $(LINUX_SRC) M=$(PWD) clean

.PHONEY: all

all: modules

copy:
	cp -f $(MODULE).ko $(SDK_SRC)/target/rootfs-cpio/lib/modules/
endif
