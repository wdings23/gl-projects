# Copyright (C) 2009 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
LOCAL_PATH := $(call my-dir)

$(info LOCAL_PATH = $(LOCAL_PATH))

include $(CLEAR_VARS)

LOCAL_MODULE    := libworldviewer

PCH_FILE := $(LOCAL_PATH)/Prefix.h

LOCAL_CFLAGS := -Werror
LOCAL_CFLAGS += -include $(PCH_FILE)
LOCAL_CFLAGS += -g
LOCAL_CFLAGS += -mfpu=neon
LOCAL_CFLAGS += -march=armv7-a
LOCAL_CFLAGS += -mfloat-abi=softfp
LOCAL_CFLAGS += -std=c++11
LOCAL_CFLAGS += -fstack-protector-all

LOCAL_ARM_MODE := arm

LOCAL_LDLIBS    := -llog -lGLESv2 -landroid
LOCAL_LDLIBS    += -lOpenSLES

LOCAL_SHARED_LIBRARIES := libOpenSLES

# files to compile
LOCAL_SRC_FILES := $(wildcard $(LOCAL_PATH)/../../src/3d/*.cpp)
LOCAL_SRC_FILES += $(wildcard $(LOCAL_PATH)/../../src/assetload/*.cpp)
LOCAL_SRC_FILES += $(wildcard $(LOCAL_PATH)/../../src/tinyxml/*.cpp)
LOCAL_SRC_FILES += $(wildcard $(LOCAL_PATH)/../../src/job/*.cpp)
LOCAL_SRC_FILES += $(wildcard $(LOCAL_PATH)/../../src/math/*.cpp)
LOCAL_SRC_FILES += $(wildcard $(LOCAL_PATH)/../../src/platform/android/*.cpp)
LOCAL_SRC_FILES += $(wildcard $(LOCAL_PATH)/../../src/render/*.cpp)
LOCAL_SRC_FILES += $(wildcard $(LOCAL_PATH)/../../src/screens/*.cpp)
LOCAL_SRC_FILES += $(wildcard $(LOCAL_PATH)/../../src/ui/*.cpp)
LOCAL_SRC_FILES += $(wildcard $(LOCAL_PATH)/../../src/util/*.cpp)
#LOCAL_SRC_FILES += $(wildcard $(LOCAL_PATH)/../../src/controller/*.cpp)
LOCAL_SRC_FILES += $(wildcard $(LOCAL_PATH)/../../src/game/*.cpp)
LOCAL_SRC_FILES += $(wildcard $(LOCAL_PATH)/../../game/*.cpp)
LOCAL_SRC_FILES += $(wildcard $(LOCAL_PATH)/../../menus/*.cpp)
#LOCAL_SRC_FILES += $(wildcard $(LOCAL_PATH)/../../menus/*.cpp)
LOCAL_SRC_FILES += $(wildcard $(LOCAL_PATH)/*.cpp)

# include directories
LOCAL_C_INCLUDES := $(LOCAL_PATH)/
LOCAL_C_INCLUDES += ${shell find $(LOCAL_PATH)/../../src/ -type d}
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../game/
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../menus/

# $(info LOCAL_C_INCLUDES = $(LOCAL_C_INCLUDES))
# $(info LOCAL_SRC_FILES = $(LOCAL_SRC_FILES))

# shitty android command likes to append the local path shit in front when the file names already have the path
#LOCAL_PATH := /

include $(BUILD_SHARED_LIBRARY)
