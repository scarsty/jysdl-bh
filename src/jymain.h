// 头文件 
#pragma once

#include "config.h"

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
#include "list.h" 

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

static char *JY_CurrentPath = "./";

#ifdef ANDROID
*JY_CurrentPath = "/sdcard/JYLDCR/";
#endif

//安全free指针的宏
#define swap16( x )  ( ((x & 0x00ffU) << 8) |  ((x & 0xff00U) >> 8) )

#define SafeFree(p) do {if(p) {free(p);p=NULL;}} while(0)

//全程变量

#define _(f) va("%s%s", JY_CurrentPath, f)

#define RMASK (0xff0000)
#define GMASK (0xff00)
#define BMASK (0xff)
#define AMASK (0xff000000)

// jymain.c

int Lua_Main(lua_State *pL_main);

int Lua_Config(lua_State *pL,const char *filename);

int getfield(lua_State *pL,const char *key);

int getfieldstr(lua_State *pL,const char *key,char *str);

// 输出信息到文件debug.txt中
int JY_Debug(const char * fmt,...);

// 输出信息到文件error.txt中
int JY_Error(const char * fmt,...);

//限制 x在 xmin-xmax之间
int limitX(int x, int xmin, int xmax);

//取文件长度
int FileLength(const char *filename);

char *va(
   const char *format,
   ...
);

