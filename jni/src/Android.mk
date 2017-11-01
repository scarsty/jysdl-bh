LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := main

SDL_PATH := ../SDL
SDL_IMAGE_PATH := ../SDL_image
SDL_TTF_PATH := ../SDL_ttf
SDL_MIXER_PATH := ../SDL_mixer
LUA_PATH := ../LUA
BASS_PATH := ../Bass
MINIZIP_PATH := ../minizip

LOCAL_C_INCLUDES := $(LOCAL_PATH) \
										$(LOCAL_PATH)/$(SDL_PATH)/include \
										$(LOCAL_PATH)/$(SDL_IMAGE_PATH) \
										$(LOCAL_PATH)/$(SDL_TTF_PATH) \
										$(LOCAL_PATH)/$(SDL_MIXER_PATH) \
										$(LOCAL_PATH)/$(LUA_PATH) \
										$(LOCAL_PATH)/$(BASS_PATH)/include\
										$(LOCAL_PATH)/$(MINIZIP_PATH)\
										$(LOCAL_PATH)/../ffmpeg

# Add your application source files here...

LOCAL_SRC_FILES := \
	$(subst $(LOCAL_PATH)/,, \
	$(wildcard $(LOCAL_PATH)/*.cpp) \
	$(SDL_PATH)/src/main/android/SDL_android_main.c )



LOCAL_SHARED_LIBRARIES := SDL2 SDL2_image SDL2_ttf bass bassmidi lua minizip

LOCAL_LDLIBS := -lGLESv1_CM -lGLESv2 -llog

include $(BUILD_SHARED_LIBRARY)

# Enable c++11 extentions in source code
APP_CPPFLAGS += -std=c++11
