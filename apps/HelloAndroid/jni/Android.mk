LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := native
LOCAL_ARM_MODE  := arm
LOCAL_SRC_FILES := native.cpp
LOCAL_LDFLAGS   := -Ljni
LOCAL_LDLIBS    := -lm -llog -landroid $(LOCAL_PATH)/halide_generated_$(TARGET_ARCH_ABI)/halide_generated.o # -lOpenCL -lllvm-a3xx
LOCAL_STATIC_LIBRARIES := android_native_app_glue
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../../include $(LOCAL_PATH)/../../../build/include $(LOCAL_PATH)/halide_generated_$(TARGET_ARCH_ABI)/

include $(BUILD_SHARED_LIBRARY)

$(call import-module,android/native_app_glue)
