cmd_drivers/video/omap2/built-in.o :=  /opt/gm8136/toolchain_gnueabi-4.4.0_ARMv5TE/usr/bin/arm-unknown-linux-uclibcgnueabi-ld -EL    -r -o drivers/video/omap2/built-in.o drivers/video/omap2/displays/built-in.o ; scripts/mod/modpost drivers/video/omap2/built-in.o