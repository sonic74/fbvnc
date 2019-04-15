LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := fbvnc
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT)/xbin
LOCAL_SRC_FILES := fbvnc.c draw.c d3des.c vncauth.c

#LOCAL_PRELINK_MODULE := true


LOCAL_STATIC_LIBRARIES := libvncserver
LOCAL_SHARED_LIBRARIES := libz libpng libjpeg libssl


include $(BUILD_EXECUTABLE)
