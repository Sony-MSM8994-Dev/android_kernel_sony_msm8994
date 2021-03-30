# MHL SII8620/SII8061 chip support

obj-m += mhl_sii8620_8061_drv.o

mhl_sii8620_8061_drv-objs := mhl_sii8620_8061_device.o \
		mhl_sii8620_device_edid.o mhl_driver.o mhl_tx.o mhl_tx_rcp.o \
		mhl_lib_edid.o mhl_lib_infoframe.o mhl_lib_timer.o \
		mhl_cbus_control.o mhl_cbus_write_burst.o

ifdef CONFIG_MHL_SUPPORT_UNPOWERED_DONGLE
mhl_sii8620_8061_drv-objs := mhl_support_unpowered_dongle.o
endif
ifdef CONFIG_MHL_BIST
mhl_sii8620_8061_drv-objs := mhl_sii8620_bist.o
endif
