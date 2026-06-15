// 主头文件 - 全局声明与公共接口
#pragma once

#include "config.h"

#include "SDL3/SDL.h"
#include "SDL3_image/SDL_image.h"
#include "SDL3_ttf/SDL_ttf.h"

extern "C"
{
#include "lua.h"
#include "lualib.h"
#include "luafun.h"

#include "lauxlib.h"
}

#include "ParticleExample.h"

#include <algorithm>  // for std::swap


// 公共宏定义

#ifndef BOOL
#define BOOL unsigned char
#endif
#ifndef TRUE
#define TRUE (BOOL)1
#endif
#ifndef FALSE
#define FALSE (BOOL)0
#endif

#define swap16(x) (((x & 0x00ffU) << 8) | ((x & 0xff00U) >> 8))

// 安全释放指针
#define SafeFree(p) \
    do { \
        if (p) \
        { \
            free(p); \
            p = NULL; \
        } \
    } while (0)

// 拼接当前工作路径与文件名
#define _(f) va("%s%s", JY_CurrentPath, f)

// 32位色RGBA掩码
#define RMASK (0xff0000)
#define GMASK (0xff00)
#define BMASK (0xff)
#define AMASK (0xff000000)


// ============ 函数声明 ============

// Lua 主流程
int Lua_Main(lua_State* pL_main);
int Lua_Config(lua_State* pL, const char* filename);
int getfield(lua_State* pL, const char* key);
int getfieldstr(lua_State* pL, const char* key, char* str);

// 日志输出 (底层实现，由宏包装自动注入 __FILE__/__LINE__/__FUNCTION__)
int JY_Debug_Impl(const char* file, int line, const char* func, const char* fmt, ...);
int JY_Error_Impl(const char* file, int line, const char* func, const char* fmt, ...);

#define JY_Debug(...) JY_Debug_Impl(__FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define JY_Error(...) JY_Error_Impl(__FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)

// 通用工具
int limitX(int x, int xmin, int xmax);
int FileExists(const char* name);
int FileLength(const char* filename);
char* va(const char* format, ...);


// ============ 全局变量 ============

// SDL 渲染相关
extern SDL_Window* g_Window;
extern SDL_Renderer* g_Renderer;
extern SDL_Texture* g_Texture;         // 主绘制纹理
extern SDL_Texture* g_TextureShow;     // 显示用纹理
extern SDL_Texture* g_TextureTmp;      // 临时纹理
extern SDL_Surface* g_Surface;         // 游戏使用的表面
extern Uint32 g_MaskColor32;           // 透明色

// 屏幕设置
extern int g_Rotate;       // 屏幕是否旋转
extern int g_ScreenW;      // 屏幕宽度
extern int g_ScreenH;      // 屏幕高度
extern int g_ScreenBpp;    // 屏幕色深
extern int g_FullScreen;   // 是否全屏

// 声音设置
extern int g_EnableSound;   // 声音开关 0关闭 1打开
extern int g_MusicVolume;   // 音乐音量
extern int g_SoundVolume;   // 音效音量
extern int g_SwitchABXY;    // 是否交换AB键

// 贴图参数
extern int g_XScale;    // 贴图X方向半宽
extern int g_YScale;    // 贴图Y方向半高

// 各地图绘制余量（保证完整显示）
extern int g_MMapAddX;
extern int g_MMapAddY;
extern int g_SMapAddX;
extern int g_SMapAddY;
extern int g_WMapAddX;
extern int g_WMapAddY;

// 缓存与加载
extern int g_MAXCacheNum;      // 最大贴图缓存数量
extern int g_LoadFullS;        // 是否全部加载S文件
extern int g_LoadMMapType;     // 大地图加载模式
extern int g_LoadMMapScope;    // 大地图加载范围

// 运行参数
extern int g_IsDebug;              // 是否开启调试日志
extern char g_MainLuaFile[255];    // Lua主脚本文件路径
extern int g_MP3;                  // 是否使用MP3格式音乐
extern char g_MidSF2[255];        // MIDI音色库文件路径
extern float g_Zoom;               // 图片缩放比例
extern char g_Softener[255];      // 柔化滤镜名称
extern int g_DelayTimes;          // 延迟帧数

extern lua_State* pL_main;
extern void* g_Tinypot;
extern const char* JY_CurrentPath;

extern ParticleExample g_Particle;
