// Android 平台 PotDll 存根 (tinypot 视频播放不可用)
#pragma once

#ifdef __ANDROID__

#define MYTHAPI
#define HBAPI

inline void* MYTHAPI PotCreateFromHandle(void*) { return nullptr; }
inline void* MYTHAPI PotCreateFromWindow(void*) { return nullptr; }
inline int MYTHAPI PotInputVideo(void*, char*) { return -1; }
inline int MYTHAPI PotSeek(void*, int) { return -1; }
inline int MYTHAPI PotDestory(void*) { return 0; }

#else
#include "../../smallpot/include/PotDll.h"
#endif
