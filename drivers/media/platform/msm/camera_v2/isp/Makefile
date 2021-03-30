ccflags-y += -Idrivers/media/platform/msm/camera_v2
ccflags-y += -Idrivers/media/platform/msm/camera_v2/sensor/io
CFLAGS_msm_buf_mgr.o := $(call cc-disable-warning,int-in-bool-context,)
obj-$(CONFIG_MSMB_CAMERA) += msm_isp.o msm_buf_mgr.o msm_isp_util.o msm_isp_axi_util.o msm_isp_stats_util.o
obj-$(CONFIG_MSMB_CAMERA) += msm_isp47.o msm_isp46.o msm_isp44.o msm_isp40.o msm_isp32.o
