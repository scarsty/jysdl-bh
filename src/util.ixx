

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
    //safe��ë���������������
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

export SDL_Surface* g_Surface;    // ��Ϸʹ�õ���Ƶ����
export Uint32 g_MaskColor32;      // ͸��ɫ

export int g_Rotate;     //��Ļ�Ƿ���ת
export int g_ScreenW;    // ��Ļ���
export int g_ScreenH;
export int g_ScreenBpp;    // ��Ļɫ��
export int g_FullScreen;
export int g_EnableSound;    // �������� 0 �ر� 1 ��
export int g_MusicVolume;    // ����������С
export int g_SoundVolume;    // ��Ч������С

export int g_XScale;    //��ͼx,y����һ���С
export int g_YScale;

//������ͼ����ʱxy������Ҫ����Ƶ���������֤����ȫ����ʾ
export int g_MMapAddX;
export int g_MMapAddY;
export int g_SMapAddX;
export int g_SMapAddY;
export int g_WMapAddX;
export int g_WMapAddY;

export int g_MAXCacheNum;     //��󻺴�����
export int g_LoadFullS;       //�Ƿ�ȫ������S�ļ�
export int g_LoadMMapType;    //�Ƿ�ȫ������M�ļ�
export int g_LoadMMapScope;
export int g_PreLoadPicGrp;     //�Ƿ�Ԥ�ȼ�����ͼ�ļ���grp
export int IsDebug;             //�Ƿ�򿪸����ļ�
export char JYMain_Lua[255];    //lua������
export int g_MP3;               //�Ƿ��MP3
export int g_BJ;                //�Ƿ��MP3
export char g_MidSF2[255];      //��ɫ���Ӧ���ļ�
export float g_Zoom;            //ͼƬ�Ŵ�
export char g_Softener[255];    //�Ƿ��ữ
export void* g_Tinypot;

export const char* JY_CurrentPath;

export int g_DelayTimes;

export ParticleExample g_Particle;    //����ϵͳ

export lua_State* pL_main;    //lua�����

export Uint32 m_color32[256];   

// ���Ժ���
// �����debug.txt��
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

// ���Ժ���
// �����error.txt��
export int JY_Error(const char* fmt, ...)
{
    //�޾Ʋ������������error��Ϣ
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

// ����x��С
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

// �����ļ����ȣ���Ϊ0�����ļ����ܲ�����
export int FileLength(const char* filename)
{
    FILE* f;
    int ll;
    if ((f = fopen(filename, "rb")) == NULL)
    {
        return 0;    // �ļ������ڣ�����
    }
    fseek(f, 0, SEEK_END);
    ll = ftell(f);    //����õ���len�����ļ��ĳ�����
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

    //��
    memset(buf, 0, 10);
    fgets(buf, 10, fp);
    *width = atoi(buf);

    //��
    memset(buf, 0, 10);
    fgets(buf, 10, fp);
    *height = atoi(buf);

    JY_Debug("GetModes: width=%d, height=%d", *width, *height);

    fclose(fp);
}