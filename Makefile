CROSS_COMPILE=/home/sedc/iic/rockchip/toolchain/aarch64-rockchip-linux-gnu/usr/bin/
SYSROOT=/home/sedc/iic/rockchip/toolchain/aarch64-rockchip-linux-gnu/usr/aarch64-buildroot-linux-gnu/sysroot/
CC := ${CROSS_COMPILE}/aarch64-rockchip-linux-gnu-gcc
LD := ${CROSS_COMPILE}/aarch64-rockchip-linux-gnu-ld
AR := ${CROSS_COMPILE}/aarch64-rockchip-linux-gnu-ar

#CC = gcc
#LD = ld
#AR = ar
#CFLAGS += `$(PKG-CONFIG) --cflags libdrm libudev gbm`
#LDFLAGS += `$(PKG-CONFIG) --libs libdrm libudev gbm`
#CFLAGS += -I$(SWIFT_ROOT)/utils -I$(SWIFT_ROOT)
#CFLAGS += -I$(SWIFT_ROOT)/video_display
#CFLAGS += -I$(SWIFT_ROOT)/video_display/gl_renderer
#ifeq ($(PLATFORM), RK3399)
#CFLAGS += -I$(SWIFT_ROOT)/video_display/hwaccel/decode
#CFLAGS += -I$(SWIFT_ROOT)/video_display/hwaccel/rga
#endif
#LDFLAGS += -L$(SWIFT_ROOT)/utils
#LDFLAGS += -L$(SWIFT_ROOT)/video_display/gl_renderer
#ifeq ($(PLATFORM), RK3399)
#LDFLAGS += -L$(SWIFT_ROOT)/video_display/hwaccel/decode -lswiftvd_hwdecode
#LDFLAGS += -L$(SWIFT_ROOT)/video_display/hwaccel/rga -lswiftvd_rga
#endif
LDFLAGS += -lm -lusb-1.0
#
#.PHONY: all
#.PHONY: clean
#
all: usb_peep  hotplugtest

usb_peep : lcd.o peep.o sysmon.o
	$(CC) -o usb_peep peep.o lcd.o sysmon.o -lusb-1.0

peep.o: peep.c lcd.h sysmon.h
	$(CC) -g -O3 -fPIC -c peep.c

lcd.o: lcd.c lcd.h
	$(CC) -g -O3 -fPIC -c lcd.c

sysmon.o: sysmon.c sysmon.h
	$(CC) -g -O3 -fPIC -c sysmon.c

hotplugtest: hotplugtest.c
	$(CC) -g -O3 -o hotplugtest hotplugtest.c -lusb-1.0

clean:
	-@rm -f usb_peep
	-@rm -f *.o
