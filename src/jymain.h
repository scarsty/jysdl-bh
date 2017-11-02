// 头文件
#pragma once

#include "config.h"

EXTERN_C_BEGIN
#include "SDL.h"
#include "SDL_ttf.h"
#include "SDL_image.h"
//#include "SDL_mixer.h"
//#include "smpeg.h"
#include "bass.h"
#include "bassmidi.h"
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "luafun.h"
EXTERN_C_END

// 公共部分
#ifndef BOOL
#define BOOL unsigned char
#endif
#ifndef TRUE
#define TRUE (BOOL) 1
#endif
#ifndef FALSE
#define FALSE (BOOL) 0
#endif

#define swap16( x )  ( ((x & 0x00ffU) << 8) |  ((x & 0xff00U) >> 8) )

//安全free指针的宏
#define SafeFree(p) do {if(p) {free(p);p=NULL;}} while(0)

//全程变量

#define _(f) va("%s%s", JY_CurrentPath, f)

#define RMASK (0xff0000)
#define GMASK (0xff00)
#define BMASK (0xff)
#define AMASK (0xff000000)

// jymain.c

int Lua_Main(lua_State* pL_main);

int Lua_Config(lua_State* pL, const char* filename);

int getfield(lua_State* pL, const char* key);

int getfieldstr(lua_State* pL, const char* key, char* str);

// 输出信息到文件debug.txt中
int JY_Debug(const char* fmt, ...);

// 输出信息到文件error.txt中
int JY_Error(const char* fmt, ...);

//限制 x在 xmin-xmax之间
int limitX(int x, int xmin, int xmax);

//取文件长度
int FileLength(const char* filename);

char* va(const char* format, ...);

template <typename T> void swap(T& a, T& b)
{
    auto t = a;
    a = b;
    b = t;
}

extern SDL_Window* g_Window;
extern SDL_Renderer* g_Renderer;
extern SDL_Texture* g_Texture;
extern SDL_Texture* g_TextureShow;
extern SDL_Texture* g_TextureTmp;

extern SDL_Surface* g_Surface;               // 游戏使用的视频表面
extern Uint32 g_MaskColor32;                 // 透明色

extern int g_Rotate;                         //屏幕是否旋转
extern int g_ScreenW;                        // 屏幕宽高
extern int g_ScreenH;
extern int g_ScreenBpp;                      // 屏幕色深
extern int g_FullScreen;
extern int g_EnableSound;                    // 声音开关 0 关闭 1 打开
extern int g_MusicVolume;                    // 音乐声音大小
extern int g_SoundVolume;                    // 音效声音大小

extern int g_XScale;                         //贴图x,y方向一半大小
extern int g_YScale;

                                             //各个地图绘制时xy方向需要多绘制的余量。保证可以全部显示
extern int g_MMapAddX;
extern int g_MMapAddY;
extern int g_SMapAddX;
extern int g_SMapAddY;
extern int g_WMapAddX;
extern int g_WMapAddY;

extern int g_MAXCacheNum;                    //最大缓存数量
extern int g_LoadFullS;                      //是否全部加载S文件
extern int g_LoadMMapType;                   //是否全部加载M文件
extern int g_LoadMMapScope;
extern int g_PreLoadPicGrp;                  //是否预先加载贴图文件的grp
extern int IsDebug;                          //是否打开跟踪文件
extern char JYMain_Lua[255];                 //lua主函数
extern int g_MP3;                            //是否打开MP3
extern char g_MidSF2[255];                   //音色库对应的文件
extern float g_Zoom;                         //图片放大

extern lua_State* pL_main;

extern void* g_Tinypot;

extern char* JY_CurrentPath;

