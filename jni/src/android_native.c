#include "android_native.h"

#include "jymain.h"
#include <android/log.h>

extern int g_ScreenW ;
extern int g_ScreenH ;
extern int g_MusicVolume;
extern int g_SoundVolume;
extern char JY_CurrentPath[512];

JNIEXPORT void JNICALL Java_cn_vip_ldcr_SDLActivity_nativeCustomPaused (JNIEnv * jniEnv, jobject thiz)
{
	System_Paused();
}

JNIEXPORT void JNICALL Java_cn_vip_ldcr_SDLActivity_nativeCustomResume (JNIEnv * jniEnv, jobject thiz)
{
	System_Resume();
}

JNIEXPORT void JNICALL Java_cn_vip_ldcr_SDLActivity_nativeSetGamePath (JNIEnv * jniEnv, jobject thiz, jstring path)
{
	const char const *p_char = (*jniEnv)->GetStringUTFChars(jniEnv, path, NULL);

	memset(JY_CurrentPath,0, 512);
	memcpy(JY_CurrentPath,p_char, strlen(p_char));
	__android_log_print(ANDROID_LOG_INFO, "jy", "set path = %s", JY_CurrentPath);
}

JNIEXPORT void JNICALL Java_cn_vip_ldcr_SDLActivity_nativeSetResolution (JNIEnv * jniEnv, jobject thiz, jint w, jint h)
{
	g_ScreenW = w;
	g_ScreenH = h;

	__android_log_print(ANDROID_LOG_INFO, "jy", "w = %d, h=%d", g_ScreenW, g_ScreenH);
}

JNIEXPORT void JNICALL Java_cn_vip_ldcr_SDLActivity_nativeSetVolume (JNIEnv * jniEnv, jobject thiz, jint value)
{
	g_MusicVolume = value;
	g_SoundVolume = value;
}
