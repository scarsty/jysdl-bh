

#include <time.h>

#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"
#include "SDL2/SDL_ttf.h"

#include "bass.h"
#include "bassmidi.h"

#include "lua.hpp"

export module jy:util;

import std;
import ParticleExample;

export void SafeFree(void* p)
{
    if (p)
    {
        //free(p);
        p = NULL;
    }
    //safe个毛，这个函数有问题
}

//#define _(f) va("%s%s", JY_CurrentPath, f)

export uint32_t RMASK = 0xff0000;
export uint32_t GMASK = 0xff00;
export uint32_t BMASK = 0xff;
export uint32_t AMASK = 0xff000000;

export const char* CONFIG_FILE = "config.lua";
export const char* DEBUG_FILE = "debug.txt";
export const char* ERROR_FILE = "error.txt";

export SDL_Window* g_Window;
export SDL_Renderer* g_Renderer;
export SDL_Texture* g_Texture;
export SDL_Texture* g_TextureShow;
export SDL_Texture* g_TextureTmp;

export SDL_Surface* g_Surface;    // 游戏使用的视频表面
export Uint32 g_MaskColor32;      // 透明色

export int g_Rotate;     //屏幕是否旋转
export int g_ScreenW;    // 屏幕宽高
export int g_ScreenH;
export int g_ScreenBpp;    // 屏幕色深
export int g_FullScreen;
export int g_EnableSound;    // 声音开关 0 关闭 1 打开
export int g_MusicVolume;    // 音乐声音大小
export int g_SoundVolume;    // 音效声音大小

export int g_XScale;    //贴图x,y方向一半大小
export int g_YScale;

//各个地图绘制时xy方向需要多绘制的余量。保证可以全部显示
export int g_MMapAddX;
export int g_MMapAddY;
export int g_SMapAddX;
export int g_SMapAddY;
export int g_WMapAddX;
export int g_WMapAddY;

export int g_MAXCacheNum;     //最大缓存数量
export int g_LoadFullS;       //是否全部加载S文件
export int g_LoadMMapType;    //是否全部加载M文件
export int g_LoadMMapScope;
export int g_PreLoadPicGrp;     //是否预先加载贴图文件的grp
export int IsDebug;             //是否打开跟踪文件
export char JYMain_Lua[255];    //lua主函数
export int g_MP3;               //是否打开MP3
export int g_BJ;                //是否打开MP3
export char g_MidSF2[255];      //音色库对应的文件
export float g_Zoom;            //图片放大
export char g_Softener[255];    //是否柔化
export void* g_Tinypot;

export const char* JY_CurrentPath;

export int g_DelayTimes;

export ParticleExample g_Particle;    //粒子系统

export lua_State* pL_main;    //lua虚拟机

export Uint32 m_color32[256];   

// 调试函数
// 输出到debug.txt中
export int JY_Debug(const char* fmt, ...)
{
    time_t t;
    FILE* fp;
    struct tm* newtime;
    va_list argptr;
#ifdef _DEBUG
    if (IsDebug == 0)
    {
        return 0;
    }
#endif
    char string[1024];
    // concatenate all the arguments in one string
    va_start(argptr, fmt);
    vsnprintf(string, sizeof(string), fmt, argptr);
    va_end(argptr);
    _time64(&t);
    newtime = _localtime64(&t);
#ifdef _DEBUG
    fprintf(stderr, "%02d:%02d:%02d %s\n", newtime->tm_hour, newtime->tm_min, newtime->tm_sec, string);
#else
    fp = fopen(DEBUG_FILE, "a+t");
    if (fp)
    {
        fprintf(stdout, "%02d:%02d:%02d %s\n", newtime->tm_hour, newtime->tm_min, newtime->tm_sec, string);
        fclose(fp);
    }
#endif
    return 0;
}

// 调试函数
// 输出到error.txt中
export int JY_Error(const char* fmt, ...)
{
    //无酒不欢：不再输出error信息
#ifdef _DEBUG
    time_t t;
    FILE* fp;
    struct tm* newtime;

    va_list argptr;
    char string[1024];

    va_start(argptr, fmt);
    vsnprintf(string, sizeof(string), fmt, argptr);
    va_end(argptr);
    time(&t);
    newtime = localtime(&t);
    fprintf(stderr, "%02d:%02d:%02d %s\n", newtime->tm_hour, newtime->tm_min, newtime->tm_sec, string);
    //fp = fopen(ERROR_FILE, "a+t");
    //if (fp)
    //{
    //    fprintf(fp, "%02d:%02d:%02d %s\n", newtime->tm_hour, newtime->tm_min, newtime->tm_sec, string);
    //    fflush(fp);
    //}
#endif
    return 0;
}

// 限制x大小
export int limitX(int x, int xmin, int xmax)
{
    if (x > xmax)
    {
        x = xmax;
    }
    if (x < xmin)
    {
        x = xmin;
    }
    return x;
}

// 返回文件长度，若为0，则文件可能不存在
export int FileLength(const char* filename)
{
    FILE* f;
    int ll;
    if ((f = fopen(filename, "rb")) == NULL)
    {
        return 0;    // 文件不存在，返回
    }
    fseek(f, 0, SEEK_END);
    ll = ftell(f);    //这里得到的len就是文件的长度了
    fclose(f);
    return ll;
}

export char* va(const char* format, ...)
{
    static char string[256];
    va_list argptr;

    va_start(argptr, format);
    vsnprintf(string, 256, format, argptr);
    va_end(argptr);

    return string;
}

export void GetModes(int* width, int* height)
{
    char buf[10];
    FILE* fp = fopen("resolution.txt", "r");

    if (!fp)
    {
        JY_Error("GetModes: cannot open resolution.txt");
        return;
    }

    //宽
    memset(buf, 0, 10);
    fgets(buf, 10, fp);
    *width = atoi(buf);

    //高
    memset(buf, 0, 10);
    fgets(buf, 10, fp);
    *height = atoi(buf);

    JY_Debug("GetModes: width=%d, height=%d", *width, *height);

    fclose(fp);
}