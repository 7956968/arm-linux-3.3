ifeq ($(KERNELRELEASE),)
PWD := $(shell pwd)
sinclude $(PWD)/../../module_conf

ifndef SDK_MODULE_SRC
SDK_MODULE_SRC = $(SDK_SRC)/module/
endif
endif

ifeq ($(obj),)
obj=$(PWD)
endif

ifeq ($(srctree),)
include $(SDK_SRC)linux-3.3-fa/.config
else
include $(srctree)/.config
endif

common-objs := spi_common.o

fisp_iris_da-objs := $(common-objs) iris_da.o
modules-y+= fisp_iris_da.o
fisp_iris_pwm-objs := $(common-objs) iris_pwm.o
modules-y+= fisp_iris_pwm.o
fisp_ar0331-objs := $(common-objs) sen_ar0331.o
modules-y+= fisp_ar0331.o
fisp_mn34031-objs := $(common-objs) sen_mn34031.o
modules-y+= fisp_mn34031.o
fisp_imx076-objs := $(common-objs) sen_imx076.o
modules-y+= fisp_imx076.o
fisp_imx036-objs := $(common-objs) sen_imx036.o
modules-y+= fisp_imx036.o
fisp_imx122-objs := $(common-objs) sen_imx122.o
modules-y+= fisp_imx122.o
fisp_imx124-objs := $(common-objs) sen_imx124.o
modules-y+= fisp_imx124.o
fisp_imx138-objs := $(common-objs) sen_imx138.o
modules-y+= fisp_imx138.o
fisp_soih22-objs := $(common-objs) sen_soih22.o
modules-y+= fisp_soih22.o
fisp_soih42-objs := $(common-objs) sen_soih42.o
modules-y+= fisp_soih42.o
fisp_soih61-objs := $(common-objs) sen_soih61.o
modules-y+= fisp_soih61.o
fisp_mt9p031-objs := $(common-objs) sen_mt9p031.o
modules-y+= fisp_mt9p031.o
fisp_ov5653-objs := $(common-objs) sen_ov5653.o
modules-y+= fisp_ov5653.o
fisp_ov4689-objs := $(common-objs) sen_ov4689.o
modules-y+= fisp_ov4689.o
fisp_gc1004-objs := $(common-objs) sen_gc1004.o
modules-y+= fisp_gc1004.o
fisp_gc1024-objs := $(common-objs) sen_gc1024.o
modules-y+= fisp_gc1024.o
fisp_ov2718-objs := $(common-objs) sen_ov2718.o
modules-y+= fisp_ov2718.o
fisp_ov2715-objs := $(common-objs) sen_ov2715.o
modules-y+= fisp_ov2715.o
fisp_ov9715-objs := $(common-objs) sen_ov9715.o
modules-y+= fisp_ov9715.o
fisp_mt9m034-objs := $(common-objs) sen_mt9m034.o
modules-y+= fisp_mt9m034.o
fisp_mt9p031-objs := $(common-objs) sen_mt9p031.o
modules-y+= fisp_mt9p031.o
fisp_ar0140-objs := $(common-objs) sen_ar0140.o
modules-y+= fisp_ar0140.o
fisp_imx136-objs := $(common-objs) sen_imx136.o
modules-y+= fisp_imx136.o
fisp_imx104-objs := $(common-objs) sen_imx104.o
modules-y+= fisp_imx104.o
fisp_mn34041-objs := $(common-objs) sen_mn34041.o
modules-y+= fisp_mn34041.o
fisp_imx169-objs := $(common-objs) sen_imx169.o
modules-y+= fisp_imx169.o
fisp_imx179-objs := $(common-objs) sen_imx179.o
modules-y+= fisp_imx179.o
fisp_imx225-objs := $(common-objs) sen_imx225.o
modules-y+= fisp_imx225.o
fisp_imx290-objs := $(common-objs) sen_imx290.o
modules-y+= fisp_imx290.o
fisp_ov9714-objs := $(common-objs) sen_ov9714.o
modules-y+= fisp_ov9714.o
fisp_ar0330-objs := $(common-objs) sen_ar0330.o
modules-y+= fisp_ar0330.o
fisp_mn34220-objs := $(common-objs) sen_mn34220.o
modules-y+= fisp_mn34220.o
fisp_ar0230-objs := $(common-objs) sen_ar0230.o
modules-y+= fisp_ar0230.o
fisp_ar0237-objs := $(common-objs) sen_ar0237.o
modules-y+= fisp_ar0237.o
fisp_ov9750-objs := $(common-objs) sen_ov9750.o
modules-y+= fisp_ov9750.o
fisp_bg0703-objs := $(common-objs) sen_bg0703.o
modules-y+= fisp_bg0703.o
fisp_bg0803-objs := $(common-objs) sen_bg0803.o
modules-y+= fisp_bg0803.o
fisp_sc1035-objs := $(common-objs) sen_sc1035.o
modules-y+= fisp_sc1035.o
fisp_sc2035-objs := $(common-objs) sen_sc2035.o
modules-y+= fisp_sc2035.o
fisp_sc1045-objs := $(common-objs) sen_sc1045.o
modules-y+= fisp_sc1045.o
fisp_lens_9510a2-objs := $(common-objs) lens_9510a2.o
modules-y+= fisp_lens_9510a2.o
fisp_lens_lv8044-objs := $(common-objs) lens_lv8044.o
modules-y+= fisp_lens_lv8044.o
fisp_lens_an41908-objs := $(common-objs) lens_an41908.o
modules-y+= fisp_lens_an41908.o
fisp_lens_g2005-objs := $(common-objs) lens_g2005.o
modules-y+= fisp_lens_g2005.o
fisp_ov9732-objs := $(common-objs) sen_ov9732.o
modules-y+= fisp_ov9732.o
fisp_soif02-objs := $(common-objs) sen_soif02.o
modules-y+= fisp_soif02.o
fisp_hm2160-objs := $(common-objs) sen_hm2160.o
modules-y+= fisp_hm2160.o

ifneq ($(KERNELRELEASE),)
EXTRA_CFLAGS += -I$(obj)/../
EXTRA_CFLAGS += -I$(SDK_MODULE_SRC)/include/isp328
EXTRA_CFLAGS += -I$(SDK_MODULE_SRC)/include/pwm
obj-m += $(modules-y)
obj-m := $(sort $(obj-m))
else
include $(PWD)/../config.mak
modules:
	$(MAKE) -C $(LINUX_SRC) SDK_MODULE_SRC=$(SDK_MODULE_SRC) M=$(PWD) modules
	@$(STRIP) $(STRIPFLAGS) *.ko
	@cp *.ko ../ko/

clean:
	$(MAKE) -C $(LINUX_SRC) M=$(PWD) clean
endif
