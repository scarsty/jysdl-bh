// ͷ�ļ�
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

// ��������
#ifndef BOOL
#define BOOL unsigned char
#endif
#ifndef TRUE
#define TRUE (BOOL) 1
#endif
#ifndef FALSE
#define FALSE (BOOL) 0
#endif

//static char* JY_CurrentPath = "./";

#ifdef ANDROID
static char* JY_CurrentPath = "/sdcard/JYLDCR/";
#endif

//��ȫfreeָ��ĺ�
#define swap16( x )  ( ((x & 0x00ffU) << 8) |  ((x & 0xff00U) >> 8) )

#define SafeFree(p) do {if(p) {free(p);p=NULL;}} while(0)

//ȫ�̱���

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

// �����Ϣ���ļ�debug.txt��
int JY_Debug(const char* fmt, ...);

// �����Ϣ���ļ�error.txt��
int JY_Error(const char* fmt, ...);

//���� x�� xmin-xmax֮��
int limitX(int x, int xmin, int xmax);

//ȡ�ļ�����
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

extern SDL_Surface* g_Surface;               // ��Ϸʹ�õ���Ƶ����
extern Uint32 g_MaskColor32;                 // ͸��ɫ

extern int g_Rotate;                         //��Ļ�Ƿ���ת
extern int g_ScreenW;                        // ��Ļ���
extern int g_ScreenH;
extern int g_ScreenBpp;                      // ��Ļɫ��
extern int g_FullScreen;
extern int g_EnableSound;                    // �������� 0 �ر� 1 ��
extern int g_MusicVolume;                    // ����������С
extern int g_SoundVolume;                    // ��Ч������С

extern int g_XScale;                         //��ͼx,y����һ���С
extern int g_YScale;

                                             //������ͼ����ʱxy������Ҫ����Ƶ���������֤����ȫ����ʾ
extern int g_MMapAddX;
extern int g_MMapAddY;
extern int g_SMapAddX;
extern int g_SMapAddY;
extern int g_WMapAddX;
extern int g_WMapAddY;

extern int g_MAXCacheNum;                    //��󻺴�����
extern int g_LoadFullS;                      //�Ƿ�ȫ������S�ļ�
extern int g_LoadMMapType;                   //�Ƿ�ȫ������M�ļ�
extern int g_LoadMMapScope;
extern int g_PreLoadPicGrp;                  //�Ƿ�Ԥ�ȼ�����ͼ�ļ���grp
extern int IsDebug;                          //�Ƿ�򿪸����ļ�
extern char JYMain_Lua[255];                 //lua������
extern int g_MP3;                            //�Ƿ��MP3
extern char g_MidSF2[255];                   //��ɫ���Ӧ���ļ�
extern float g_Zoom;                         //ͼƬ�Ŵ�

extern lua_State* pL_main;

extern void* g_Tinypot;