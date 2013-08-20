LOCAL_PATH := $(call my-dir)
#
# devns demo secondary namespace and switch
#

##### devns service #####
include $(CLEAR_VARS)

LOCAL_CFLAGS := -Wall

LOCAL_SRC_FILES := \
	devns.c

LOCAL_SHARED_LIBRARIES := \
	libutils

LOCAL_MODULE := devns
LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)

##### devns fb demo #####
include $(CLEAR_VARS)

LOCAL_CFLAGS := -Wall

LOCAL_SRC_FILES := \
	devns_init.c

LOCAL_SHARED_LIBRARIES := \
	libutils

LOCAL_MODULE := devns_init
LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)

##### devns switch #####
include $(CLEAR_VARS)

LOCAL_CFLAGS := -Wall

LOCAL_SRC_FILES := \
	devns_switch.c

LOCAL_SHARED_LIBRARIES := \
	libutils

LOCAL_MODULE := devns_switch
LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)


