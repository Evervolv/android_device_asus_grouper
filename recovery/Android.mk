LOCAL_PATH := $(call my-dir)

ifeq $($(TARGET_BOOTLOADER_BOARD_NAME),grouper)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := eng
LOCAL_C_INCLUDES += bootable/recovery
LOCAL_SRC_FILES := recovery_ui.cpp

# should match TARGET_RECOVERY_UI_LIB set in BoardConfig.mk
LOCAL_MODULE := librecovery_ui_grouper

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)

endif
