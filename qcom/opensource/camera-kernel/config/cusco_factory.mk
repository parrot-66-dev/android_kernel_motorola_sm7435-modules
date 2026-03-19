# Settings for compiling waipio camera architecture

# Localized KCONFIG settings
# Camera: Remove for user build
ifneq (,$(filter userdebug eng, $(TARGET_BUILD_VARIANT)))
CONFIG_CCI_DEBUG_INTF := y
ccflags-y += -DCONFIG_CCI_DEBUG_INTF=1
endif
CONFIG_MOT_PROBE_SUB_DEVICE := y
CONFIG_CAM_SENSOR_PROBE_DEBUG := y
CONFIG_IMX896_MP_SENSOR_COMPATIBLE := y

# Flags to pass into C preprocessor
ccflags-y += -DCONFIG_MOT_CUSTOM_CTLE_PARAM=1
ccflags-y += -DCONFIG_MOT_SM6450_CUSCO=1
ccflags-y += -DCONFIG_MOT_PROBE_SUB_DEVICE=1
ccflags-y += -DCONFIG_CAM_SENSOR_PROBE_DEBUG=1
ccflags-y += -DCONFIG_IMX896_MP_SENSOR_COMPATIBLE=1
