PWD := $(shell pwd)
sinclude $(PWD)/../../../module_conf

STRIPFLAGS = -g --strip-unneeded
STRIP = $(CROSS_COMPILE)strip

SDK_MODULE_SRC := $(SDK_SRC)module
KERNELDIR := $(LINUX_SRC)
FE_HDCVI_INCDIR := $(SDK_MODULE_SRC)/include/front_end/hdcvi

ifneq ($(KERNELRELEASE),)

include $(srctree)/.config

EXTRA_CFLAGS += -Wall -g -I$(obj)/../../ -I$(obj)/dh9901_lib/ -I$(obj)/dh9910_lib/ -I$(obj)/dh9920_lib/ -I$(FE_HDCVI_INCDIR)/ -I$(SDK_MODULE_SRC)/include/

dh9901_dev-objs := dh9901_drv.o
obj-m += dh9901_dev.o
dh9910_dev-objs := dh9910_drv.o
obj-m += dh9910_dev.o
dh9920_dev-objs := dh9920_drv.o
obj-m += dh9920_dev.o

obj-m += $(modules-y)
obj-m := $(sort $(obj-m))

else

all modules:
	$(MAKE) -C $(KERNELDIR) SDK_MODULE_SRC=$(SDK_MODULE_SRC) M=$(PWD) modules
	@cp $(PWD)/dh9901_lib/dh9901_ko.lib $(PWD)/dh9901.ko
	@cp $(PWD)/dh9910_lib/dh9910_ko.lib $(PWD)/dh9910.ko
	@cp $(PWD)/dh9920_lib/dh9920_ko.lib $(PWD)/dh9920.ko
	@$(STRIP) $(STRIPFLAGS) *.ko

module_lib:
	@cp $(PWD)/dh9901_lib/dh9901_ko.lib $(PWD)/dh9901.ko
	@cp $(PWD)/dh9910_lib/dh9910_ko.lib $(PWD)/dh9910.ko
	@cp $(PWD)/dh9920_lib/dh9920_ko.lib $(PWD)/dh9920.ko

module_strip:
	@$(STRIP) $(STRIPFLAGS) *.ko

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean

endif
