# Settings for compiling waipio camera architecture

# Localized KCONFIG settings
# Camera: Remove for user build
ifneq (,$(filter userdebug eng, $(TARGET_BUILD_VARIANT)))
CONFIG_CCI_DEBUG_INTF := y
ccflags-y += -DCONFIG_CCI_DEBUG_INTF=1
endif
CONFIG_CAM_SENSOR_PROBE_DEBUG := y
CONFIG_CAM_SENSOR_POWER_RETRY := y

ccflags-y += -DCONFIG_MOT_CUSTOM_CTLE_PARAM=1
ccflags-y += -DCONFIG_MOT_SM6475_MONA=1
# Flags to pass into C preprocessor
ccflags-y += -DCONFIG_CAM_SENSOR_PROBE_DEBUG=1
ccflags-y += -DCONFIG_CAM_SENSOR_POWER_RETRY=1
