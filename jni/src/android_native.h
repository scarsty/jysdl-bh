#ifndef _android_native_h
#define _android_native_h
#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT void JNICALL Java_cn_vip_ldcr_SDLActivity_nativeCustomPaused (JNIEnv * jniEnv, jobject thiz);
JNIEXPORT void JNICALL Java_cn_vip_ldcr_SDLActivity_nativeCustomResume (JNIEnv * jniEnv, jobject thiz);
JNIEXPORT void JNICALL Java_cn_vip_ldcr_SDLActivity_nativeSetGamePath (JNIEnv * jniEnv, jobject thiz, jstring path);
JNIEXPORT void JNICALL Java_cn_vip_ldcr_SDLActivity_nativeSetResolution (JNIEnv * jniEnv, jobject thiz, jint w, jint h);
JNIEXPORT void JNICALL Java_cn_vip_ldcr_SDLActivity_nativeSetVolume (JNIEnv * jniEnv, jobject thiz, jint value);

#ifdef __cplusplus
}
#endif
#endif
