

// SDL 相关函数

#include "PotDll.h"
#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"
#include "SDL2/SDL_ttf.h"

#include "bass.h"
#include "bassmidi.h"
#include "lua.hpp"

export module sdlfun;
import std;
import charset;
import util;

int KeyFilter(void* data, SDL_Event* event);
int InitSDL(void);
int ExitSDL(void);
Uint32 ConvertColor(Uint32 color);
int InitGame(void);
int ExitGame(void);
int RenderToTexture(SDL_Texture* src, SDL_Rect* src_rect, SDL_Texture* dst, SDL_Rect* dst_rect, double angle, SDL_Point* center, SDL_RendererFlip filp);
SDL_Texture* CreateRenderedTexture(SDL_Texture* ref);
SDL_Texture* CreateRenderedTexture(int w, int h);
int JY_LoadPicture(const char* str, int x, int y);
int JY_ShowSurface(int flag);
int JY_ShowSlow(int delaytime, int Flag);
int JY_Delay(int x);
double JY_GetTime();
int JY_PlayMIDI(const char* filename);
int StopMIDI();
int JY_PlayWAV(const char* filename);
int JY_GetKey(int* key, int* type, int* mx, int* my);
int JY_GetKeyState(int key);
int JY_SetClip(int x1, int y1, int x2, int y2);
int JY_DrawRect(int x1, int y1, int x2, int y2, int color);
void HLine32(int x1, int x2, int y, int color);
void VLine32(int x1, int x2, int y, int color);
int JY_FillColor(int x1, int y1, int x2, int y2, int color);
int JY_Background(int x1, int y1, int x2, int y2, int Bright, int color);
int JY_PlayMPEG(char* filename, int esckey);
int JY_FullScreen();
int JY_SaveSur(int x, int y, int w, int h);
int JY_LoadSur(int id, int x, int y);
int JY_FreeSur(int id);
SDL_Texture* CreateTextureFromRLE(unsigned char* data, int w, int h, int datalong);
int RenderTexture(SDL_Texture* lps, int x, int y, int flag, int value, int color, int width = -1, int height = -1, double angle = NULL, SDL_RendererFlip reversal = SDL_FLIP_NONE, int percent = 100);

SDL_Rect RotateRect(const SDL_Rect* rect);
SDL_Rect RotateReverseRect(const SDL_Rect* rect);

HSTREAM currentMusic = 0;    //播放音乐数据，由于同时只播放一个，用一个变量
#define WAVNUM 5
HSAMPLE WavChunk[WAVNUM];    //播放音效数据，可以同时播放几个，因此用数组
BASS_MIDI_FONT midfonts;
int currentWav = 0;    //当前播放的音效

#define RECTNUM 20
SDL_Rect ClipRect[RECTNUM];    // 当前设置的剪裁矩形
int currentRect = 0;

//#define SURFACE_NUM  20
//SDL_Texture* tmp_Surface[SURFACE_NUM];   //JY_SaveSur使用

//过滤ESC、RETURN、SPACE键，使他们按下后不能重复。
export int KeyFilter(void* data, SDL_Event* event)
{
    static int Esc_KeyPress = 0;
    static int Space_KeyPress = 0;
    static int Return_KeyPress = 0;
    int r = 1;
    switch (event->type)
    {
    case SDL_KEYDOWN:
        switch (event->key.keysym.sym)
        {
        case SDLK_ESCAPE:
            if (1 == Esc_KeyPress)
            {
                r = 0;
            }
            else
            {
                Esc_KeyPress = 1;
            }
            break;
        case SDLK_RETURN:
            if (1 == Return_KeyPress)
            {
                r = 0;
            }
            else
            {
                Return_KeyPress = 1;
            }
            break;
        case SDLK_SPACE:
            if (1 == Space_KeyPress)
            {
                r = 0;
            }
            else
            {
                Space_KeyPress = 1;
            }
            break;
        default:
            break;
        }
        break;
    case SDL_KEYUP:
        switch (event->key.keysym.sym)
        {
        case SDLK_ESCAPE:
            Esc_KeyPress = 0;
            break;
        case SDLK_SPACE:
            Space_KeyPress = 0;
            break;
        case SDLK_RETURN:
            Return_KeyPress = 0;
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
    return r;
}

// 初始化SDL
export int InitSDL(void)
{
    int r;
    int i;
    //char tmpstr[255];
    int so = 22050;
    r = SDL_Init(SDL_INIT_VIDEO);
    if (r < 0)
    {
        JY_Error(
            "Couldn't initialize SDL: %s\n", SDL_GetError());
        exit(1);
    }
    //atexit(SDL_Quit);    可能有问题，屏蔽掉
    //SDL_VideoDriverName(tmpstr, 255);
    //JY_Debug("InitSDL: Video Driver: %s\n", tmpstr);
    InitFont();    //初始化
    r = SDL_InitSubSystem(SDL_INIT_AUDIO);
    if (r < 0)
    {
        g_EnableSound = 0;
        JY_Error("Init audio error!");
    }
    if (g_MP3 == 1)
    {
        so = 44100;
    }
    if (!BASS_Init(-1, so, 0, 0, NULL))
    {
        JY_Error("Can't initialize device");
        g_EnableSound = 0;
    }
    currentWav = 0;
    for (i = 0; i < WAVNUM; i++)
    {
        WavChunk[i] = 0;
    }
    SDL_SetEventFilter(KeyFilter, NULL);
    if (g_MP3 != 1)
    {
        midfonts.font = BASS_MIDI_FontInit(g_MidSF2, 0);
        if (!midfonts.font)
        {
            JY_Error("BASS_MIDI_FontInit error ! %d", BASS_ErrorGetCode());
        }
        midfonts.preset = -1;                         // use all presets
        midfonts.bank = 0;                            // use default bank(s)
        BASS_MIDI_StreamSetFonts(0, &midfonts, 1);    // set default soundfont
    }
    return 0;
}

// 退出SDL
export int ExitSDL(void)
{
    ExitFont();
    StopMIDI();
    if (midfonts.font)
    {
        BASS_MIDI_FontFree(midfonts.font);
    }
    for (int i = 0; i < WAVNUM; i++)
    {
        if (WavChunk[i])
        {
            //Mix_FreeChunk(WavChunk[i]);
            BASS_SampleFree(WavChunk[i]);
            WavChunk[i] = 0;
        }
    }
    //Mix_CloseAudio();
    BASS_Free();
#ifdef WIN32
    if (g_Tinypot) { PotDestory(g_Tinypot); }
#endif
    JY_LoadPicture("", 0, 0);    // 释放可能加载的图片表面
    SDL_Quit();
    return 0;
}

// 转换ARGB到当前屏幕颜色
export Uint32 ConvertColor(Uint32 color)
{
    Uint8* p = (Uint8*)&color;
    return SDL_MapRGBA(g_Surface->format, *(p + 2), *(p + 1), *p, 255);
}

export int RenderToTexture(SDL_Texture* src, SDL_Rect* src_rect, SDL_Texture* dst, SDL_Rect* dst_rect, double angle, SDL_Point* center, SDL_RendererFlip filp)
{
    SDL_SetRenderTarget(g_Renderer, dst);
    return SDL_RenderCopyEx(g_Renderer, src, src_rect, dst_rect, angle, center, filp);
}

export SDL_Texture* CreateRenderedTexture(SDL_Texture* ref)
{
    int w, h;
    SDL_QueryTexture(ref, NULL, NULL, &w, &h);
    return CreateRenderedTexture(w, h);
}

export SDL_Texture* CreateRenderedTexture(int w, int h)
{
    return SDL_CreateTexture(g_Renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, w, h);
}

//加载图形文件，其他格式也可以加载
//x,y =-1 则加载到屏幕中心
//    如果未加载，则加载，然后blit，如果加载，直接blit
//  str 文件名，如果为空，则释放表面
export int JY_LoadPicture(const char* str, int x, int y)
{
    static char filename[255] = "\0";
    static SDL_Texture* tex = NULL;
    static SDL_Rect r;
    SDL_Surface* tmppic;
    SDL_Surface* pic = NULL;
    if (strlen(str) == 0)    // 为空则释放表面
    {
        if (pic)
        {
            SDL_FreeSurface(pic);
            pic = NULL;
        }
        return 0;
    }
    if (strcmp(str, filename) != 0)    // 与以前文件名不同，则释放原来表面，加载新表面
    {
        if (tex)
        {
            SDL_DestroyTexture(tex);
            tex = NULL;
        }
        tmppic = IMG_Load(str);
        if (tmppic)
        {
            pic = SDL_ConvertSurfaceFormat(tmppic, g_Surface->format->format, 0);    // 改为当前表面的像素格式
            if ((x == -1) && (y == -1))
            {
                x = (g_ScreenW - pic->w) / 2;
                y = (g_ScreenH - pic->h) / 2;
            }
            r.x = x;
            r.y = y;
            r.w = pic->w;
            r.h = pic->h;
            tex = SDL_CreateTextureFromSurface(g_Renderer, pic);
            SDL_FreeSurface(pic);
            SDL_FreeSurface(tmppic);
            strcpy(filename, str);
        }
    }
    if (tex)
    {
        RenderToTexture(tex, NULL, g_Texture, &r, NULL, NULL, SDL_FLIP_NONE);
    }
    else
    {
        JY_Error("JY_LoadPicture: Load picture file %s failed! %s", str, SDL_GetError());
    }
    return 0;
}

//显示表面
//flag = 0 显示全部表面  =1 按照JY_SetClip设置的矩形显示，如果没有矩形，则不显示
export int JY_ShowSurface(int flag)
{
    SDL_SetRenderTarget(g_Renderer, g_TextureShow);
    if (flag == 1)
    {
        if (currentRect > 0)
        {
            for (int i = 0; i < currentRect; i++)
            {
                SDL_Rect* r = ClipRect + i;
                SDL_RenderCopy(g_Renderer, g_Texture, r, r);
            }
        }
    }
    else
    {
        SDL_RenderCopy(g_Renderer, g_Texture, NULL, NULL);
    }
    SDL_SetRenderTarget(g_Renderer, NULL);
    //SDL_Rect r;
    //SDL_RenderGetClipRect(g_Renderer, &r);
    SDL_RenderSetClipRect(g_Renderer, NULL);
    if (g_Rotate == 0)
    {
        SDL_RenderCopy(g_Renderer, g_TextureShow, NULL, NULL);
    }
    else
    {
        SDL_RenderCopyEx(g_Renderer, g_TextureShow, NULL, NULL, 90, NULL, SDL_FLIP_NONE);
    }
    SDL_RenderPresent(g_Renderer);
    //SDL_RenderSetClipRect(g_Renderer, &r);
    return 0;
}

//延时x毫秒
export int JY_Delay(int x)
{
    SDL_Delay(x);
    g_DelayTimes++;
    return 0;
}

// 缓慢显示图形
// delaytime 每次渐变延时毫秒数
// Flag=0 从暗到亮，1，从亮到暗
export int JY_ShowSlow(int delaytime, int Flag)
{
    int i;
    int step;
    int t1, t2;
    int alpha;
    SDL_Texture* lps1;    // 建立临时表面
    lps1 = SDL_CreateTexture(g_Renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, g_ScreenW, g_ScreenH);
    if (lps1 == NULL)
    {
        JY_Error("JY_ShowSlow: Create surface failed!");
        return 1;
    }
    SDL_SetRenderTarget(g_Renderer, lps1);
    SDL_RenderCopy(g_Renderer, g_Texture, NULL, NULL);
    //SDL_BlitSurface(g_Surface, NULL, lps1, NULL);    //当前表面复制到临时表面
    for (i = 0; i <= 32; i++)
    {
        if (Flag == 0)
        {
            step = 32 - i;
        }
        else
        {
            step = i;
        }
        t1 = (int)JY_GetTime();
        //SDL_SetRenderDrawColor(g_Renderer, 0, 0, 0, 0);
        //SDL_RenderFillRect(g_Renderer, NULL);          //当前表面变黑
        alpha = step << 3;
        if (alpha > 255)
        {
            alpha = 255;
        }
        //SDL_SetTextureAlphaMod(lps1, (Uint8)alpha);  //设置alpha
        //SDL_RenderCopy(g_Renderer, lps1, NULL, NULL);
        //SDL_BlitSurface(lps1, NULL, g_Surface, NULL);
        SDL_SetRenderTarget(g_Renderer, g_Texture);
        SDL_RenderCopy(g_Renderer, lps1, NULL, NULL);
        SDL_SetRenderDrawColor(g_Renderer, 0, 0, 0, alpha);
        SDL_SetRenderDrawBlendMode(g_Renderer, SDL_BLENDMODE_BLEND);
        SDL_RenderFillRect(g_Renderer, NULL);
        JY_ShowSurface(0);
        t2 = (int)JY_GetTime();
        if (delaytime > t2 - t1)
        {
            JY_Delay(delaytime - (t2 - t1));
        }
        //JY_GetKey();
    }
    SDL_DestroyTexture(lps1);    //释放表面
    return 0;
}

#ifdef HIGH_PRECISION_CLOCK

__int64 GetCycleCount()
{
    __asm _emit 0x0F __asm _emit 0x31
}

#endif

//得到当前时间，单位毫秒
export double JY_GetTime()
{
#ifdef HIGH_PRECISION_CLOCK
    return (double)(GetCycleCount()) / (1000 * CPU_FREQUENCY);
#else
    return (double)SDL_GetTicks();
#endif
}

//播放音乐
export int JY_PlayMIDI(const char* filename)
{
    static char currentfile[255] = "\0";
    if (g_EnableSound == 0)
    {
        JY_Error("disable sound!");
        return 1;
    }
    if (strlen(filename) == 0)    //文件名为空，停止播放
    {
        StopMIDI();
        strcpy(currentfile, filename);
        return 0;
    }
    if (strcmp(currentfile, filename) == 0)    //与当前播放文件相同，直接返回
    {
        return 0;
    }
    StopMIDI();
    //currentMusic = BASS_MIDI_StreamCreateFile(0, filename, 0, 0, 0, 0);
    currentMusic = BASS_StreamCreateFile(0, filename, 0, 0, 0);
    if (!currentMusic)
    {
        JY_Error("Open music file %s failed! %d", filename, BASS_ErrorGetCode());
        return 1;
    }
    if (g_MP3 == 1)
    {
        BASS_MIDI_StreamSetFonts(currentMusic, &midfonts, 1);
    }    // set for current stream too
    BASS_ChannelSetAttribute(currentMusic, BASS_ATTRIB_VOL, (float)(g_MusicVolume / 100.0));
    BASS_ChannelFlags(currentMusic, BASS_SAMPLE_LOOP, BASS_SAMPLE_LOOP);
    BASS_ChannelPlay(currentMusic, FALSE);
    strcpy(currentfile, filename);
    return 0;
}

//停止音效
export int StopMIDI()
{
    if (currentMusic)
    {
        BASS_ChannelStop(currentMusic);
        BASS_StreamFree(currentMusic);
        currentMusic = 0;
    }
    return 0;
}

//播放音效
export int JY_PlayWAV(const char* filename)
{
    HCHANNEL ch;
    if (g_EnableSound == 0)
    {
        return 1;
    }
    if (WavChunk[currentWav])    //释放当前音效
    {
        //Mix_FreeChunk(WavChunk[currentWav]);
        BASS_SampleStop(WavChunk[currentWav]);
        BASS_SampleFree(WavChunk[currentWav]);
        WavChunk[currentWav] = 0;
    }
    //WavChunk[currentWav]= Mix_LoadWAV(filename);  //加载到当前音效
    WavChunk[currentWav] = BASS_SampleLoad(0, filename, 0, 0, 1, 0);
    if (WavChunk[currentWav])
    {
        //Mix_VolumeChunk(WavChunk[currentWav],g_SoundVolume);
        //Mix_PlayChannel(-1, WavChunk[currentWav], 0);  //播放音效
        ch = BASS_SampleGetChannel(WavChunk[currentWav], 0);
        BASS_ChannelSetAttribute(ch, BASS_ATTRIB_VOL, (float)(g_SoundVolume / 100.0));
        BASS_ChannelFlags(ch, 0, BASS_SAMPLE_LOOP);
        BASS_ChannelPlay(ch, 0);
        currentWav++;
        if (currentWav >= WAVNUM)
        {
            currentWav = 0;
        }
    }
    else
    {
        JY_Error("Open wav file %s failed!", filename);
    }
    return 0;
}

// 得到前面按下的字符
export int JY_GetKey(int* key, int* type, int* mx, int* my)
{
    SDL_Event event;
    int win_w, win_h, r;
    *key = -1;
    *type = -1;
    *mx = -1;
    *my = -1;
    while (SDL_PollEvent(&event))
    //if (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_KEYDOWN:
            *key = event.key.keysym.sym;
            if (*key == SDLK_SPACE)
            {
                *key = SDLK_RETURN;
            }
            *type = 1;
            break;
        case SDL_KEYUP:
            //*key = event.key.keysym.sym;
            //if (*key == SDLK_SPACE)
            //{
            //    *key = SDLK_RETURN;
            //}
            break;
        case SDL_MOUSEMOTION:    //鼠标移动
            SDL_GetWindowSize(g_Window, &win_w, &win_h);
            *mx = event.motion.x * g_ScreenW / win_w;
            *my = event.motion.y * g_ScreenH / win_h;
            if (g_Rotate) { std::swap(*mx, *my); }
            *type = 2;
            break;
        case SDL_MOUSEBUTTONDOWN:    //鼠标点击
            SDL_GetWindowSize(g_Window, &win_w, &win_h);
            *mx = event.motion.x * g_ScreenW / win_w;
            *my = event.motion.y * g_ScreenH / win_h;
            if (g_Rotate) { std::swap(*mx, *my); }
            if (event.button.button == SDL_BUTTON_LEFT)    //左键
            {
                *type = 3;
            }
            else if (event.button.button == SDL_BUTTON_RIGHT)    //右键
            {
                *type = 4;
            }
            else if (event.button.button == SDL_BUTTON_MIDDLE)    //中键
            {
                *type = 5;
            }
            break;
        case SDL_MOUSEWHEEL:
            //无酒不欢：添加鼠标滚轮
            if (event.wheel.y == 1)
            {
                *type = 6;
            }
            else if (event.wheel.y == -1)
            {
                *type = 7;
            }
            break;
        case SDL_QUIT:
        {
            static int quit = 0;
            if (quit == 0)
            {
                quit = 1;
                lua_getglobal(pL_main, "Menu_Exit");
                lua_call(pL_main, 0, 1);
                r = (int)lua_tointeger(pL_main, -1);
                lua_pop(pL_main, 1);
                //if (MessageBox(NULL, "你确定要关闭游戏吗?", "系统提示", MB_ICONQUESTION | MB_OKCANCEL) == IDOK)
                if (r == 1)
                {
                    //ExitGame();    //释放游戏数据
                    ExitSDL();    //退出SDL
                    exit(1);
                }
                quit = 0;
            }
        }
        break;
        default:
            break;
        }
    }

    return *key;
}

export int JY_GetKeyState(int key)
{
    return SDL_GetKeyboardState(NULL)[SDL_GetScancodeFromKey(key)];
}

//设置裁剪
export int JY_SetClip(int x1, int y1, int x2, int y2)
{
    SDL_Rect rect;
    SDL_SetRenderTarget(g_Renderer, g_Texture);
    if (x1 == 0 && y1 == 0 && x2 == 0 && y2 == 0)
    {
        rect.x = 0;
        rect.y = 0;
        rect.w = g_ScreenW;
        rect.h = g_ScreenH;
        SDL_RenderSetClipRect(g_Renderer, &rect);
        //SDL_SetClipRect(g_Surface, NULL);
        currentRect = 0;
    }
    else
    {
        SDL_Rect rect;
        rect.x = (Sint16)x1;
        rect.y = (Sint16)y1;
        rect.w = (Uint16)(x2 - x1);
        rect.h = (Uint16)(y2 - y1);
        ClipRect[currentRect] = rect;
        SDL_RenderSetClipRect(g_Renderer, &rect);
        currentRect = currentRect + 1;
        if (currentRect >= RECTNUM)
        {
            currentRect = 0;
        }
    }
    return 0;
}

// 绘制矩形框
// (x1,y1)--(x2,y2) 框的左上角和右下角坐标
// color 颜色
export int JY_DrawRect(int x1, int y1, int x2, int y2, int color)
{
    Uint32 c;
    c = ConvertColor(color);
    HLine32(x1, x2, y1, c);
    HLine32(x1, x2, y2, c);
    VLine32(y1, y2, x1, c);
    VLine32(y1, y2, x2, c);
    return 0;
}

//绘水平线
export void HLine32(int x1, int x2, int y, int color)
{
    Uint8 r = (Uint8)((color & RMASK) >> 16);
    Uint8 g = (Uint8)((color & GMASK) >> 8);
    Uint8 b = (Uint8)((color & BMASK));
    Uint8 a = 255;
    SDL_SetRenderTarget(g_Renderer, g_Texture);
    SDL_SetRenderDrawColor(g_Renderer, r, g, b, a);
    SDL_SetRenderDrawBlendMode(g_Renderer, SDL_BLENDMODE_BLEND);
    SDL_RenderDrawLine(g_Renderer, x1, y, x2, y);
}

//绘垂直线
export void VLine32(int x1, int x2, int y, int color)
{
    Uint8 r = (Uint8)((color & RMASK) >> 16);
    Uint8 g = (Uint8)((color & GMASK) >> 8);
    Uint8 b = (Uint8)((color & BMASK));
    Uint8 a = 255;
    SDL_SetRenderTarget(g_Renderer, g_Texture);
    SDL_SetRenderDrawColor(g_Renderer, r, g, b, a);
    SDL_SetRenderDrawBlendMode(g_Renderer, SDL_BLENDMODE_BLEND);
    SDL_RenderDrawLine(g_Renderer, y, x1, y, x2);
}

// 图形填充
// 如果x1,y1,x2,y2均为0，则填充整个表面
// color, 填充色，用RGB表示，从高到低字节为0RGB
export int JY_FillColor(int x1, int y1, int x2, int y2, int color)
{
    Uint8 r = (Uint8)((color & RMASK) >> 16);
    Uint8 g = (Uint8)((color & GMASK) >> 8);
    Uint8 b = (Uint8)((color & BMASK));
    Uint8 a = (Uint8)((color & AMASK) >> 24);
    //int c = ConvertColor(color);
    SDL_SetRenderTarget(g_Renderer, g_Texture);
    SDL_SetRenderDrawColor(g_Renderer, r, g, b, 255);
    SDL_SetRenderDrawBlendMode(g_Renderer, SDL_BLENDMODE_BLEND);
    if (x1 == 0 && y1 == 0 && x2 == 0 && y2 == 0)
    {
        SDL_RenderFillRect(g_Renderer, NULL);
        //SDL_FillRect(g_Surface, NULL, c);
    }
    else
    {
        SDL_Rect rect;
        rect.x = (Sint16)x1;
        rect.y = (Sint16)y1;
        rect.w = (Uint16)(x2 - x1);
        rect.h = (Uint16)(y2 - y1);
        SDL_RenderFillRect(g_Renderer, &rect);
        //SDL_FillRect(g_Surface, &rect, c);
    }
    return 0;
}

// 背景变暗
// 把源表面(x1,y1,x2,y2)矩形内的所有点亮度降低
// bright 亮度等级 0-256
export int JY_Background(int x1, int y1, int x2, int y2, int Bright, int color)
{
    SDL_Rect r1;
    if (x2 <= x1 || y2 <= y1)
    {
        return 0;
    }
    Bright = 256 - Bright;
    if (Bright > 255)
    {
        Bright = 255;
    }
    r1.x = (Sint16)x1;
    r1.y = (Sint16)y1;
    r1.w = (Uint16)(x2 - x1);
    r1.h = (Uint16)(y2 - y1);
    Uint8 r = (Uint8)((color & RMASK) >> 16);
    Uint8 g = (Uint8)((color & GMASK) >> 8);
    Uint8 b = (Uint8)((color & BMASK));
    Uint8 a = 255;
    SDL_SetRenderTarget(g_Renderer, g_Texture);
    SDL_SetRenderDrawColor(g_Renderer, r, g, b, Bright);
    SDL_SetRenderDrawBlendMode(g_Renderer, SDL_BLENDMODE_BLEND);
    SDL_RenderFillRect(g_Renderer, &r1);
    return 1;
}

//播放mpeg
// esckey 停止播放的按键
export int JY_PlayMPEG(char* filename, int esckey)
{
#ifdef WIN32
    if (g_Tinypot == NULL)
    {
        g_Tinypot = PotCreateFromWindow(g_Window);
    }
    StopMIDI();
    int r = PotInputVideo(g_Tinypot, filename);
    if (r == 1)
    {
        SDL_Event e;
        e.type = SDL_QUIT;
        SDL_PushEvent(&e);
    }
#endif
    //g_Tinypot = NULL;
    return 0;
}

//取s的值
export int JY_SetSound(int id, int flag)
{
    if (flag == 1)
    {
        g_SoundVolume = id;    // 声音开关 0 关闭 1 打开
    }
    else if (flag == 2)
    {
        g_MusicVolume = id;
        BASS_ChannelSetAttribute(currentMusic, BASS_ATTRIB_VOL, (float)(g_MusicVolume / 100.0));
    }
    return 0;
}

// 全屏切换
export int JY_FullScreen()
{
    //SDL_Surface* tmpsurface;
    ////const SDL_VideoInfo *info;
    //Uint32 flag = g_Surface->flags;
    //tmpsurface = SDL_CreateRGBSurface(SDL_SWSURFACE, g_Surface->w, g_Surface->h, g_Surface->format->BitsPerPixel,
    //    g_Surface->format->Rmask, g_Surface->format->Gmask, g_Surface->format->Bmask, g_Surface->format->Amask);
    //SDL_BlitSurface(g_Surface, NULL, tmpsurface, NULL);
    g_FullScreen = 1 - g_FullScreen;
    if (g_FullScreen == 1)
    {
        SDL_SetWindowFullscreen(g_Window, SDL_WINDOW_FULLSCREEN_DESKTOP);
    }
    else
    {
        SDL_SetWindowFullscreen(g_Window, 0);
    }
    //if (flag & SDL_FULLSCREEN)    //全屏，设置窗口
    //{ g_Surface = SDL_SetVideoMode(g_Surface->w, g_Surface->h, 0, SDL_SWSURFACE); }
    //else
    //{ g_Surface = SDL_SetVideoMode(g_Surface->w, g_Surface->h, g_ScreenBpp, SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_FULLSCREEN); }
    //SDL_BlitSurface(tmpsurface, NULL, g_Surface, NULL);
    JY_ShowSurface(0);
    //SDL_FreeSurface(tmpsurface);
    //info = SDL_GetVideoInfo();
    //JY_Debug("hw_available=%d,wm_available=%d", info->hw_available, info->wm_available);
    //JY_Debug("blit_hw=%d,blit_hw_CC=%d,blit_hw_A=%d", info->blit_hw, info->blit_hw_CC, info->blit_hw_A);
    //JY_Debug("blit_sw=%d,blit_sw_CC=%d,blit_sw_A=%d", info->blit_hw, info->blit_hw_CC, info->blit_hw_A);
    //JY_Debug("blit_fill=%d,videomem=%d", info->blit_fill, info->video_mem);
    //JY_Debug("Color depth=%d", info->vfmt->BitsPerPixel);
    return 0;
}

#define SURFACE_NUM 20
SDL_Texture* tmp_Surface[SURFACE_NUM];    //JY_SaveSur使用
//保存屏幕到临时表面
//保存屏幕到临时表面
export int JY_SaveSur(int x, int y, int w, int h)
{
    int id = -1;
    int i;
    SDL_Rect r1;
    for (i = 0; i < SURFACE_NUM; i++)
    {
        if (tmp_Surface[i] == NULL)
        {
            id = i;
            break;
        }
    }
    if (id < 0) { return -1; }
    if (w + x > g_ScreenW) { w = g_ScreenW - x; }
    if (h + y > g_ScreenH) { h = g_ScreenH - h; }
    if (w <= 0 || h <= 0) { return -1; }
    r1.x = x;
    r1.y = y;
    r1.w = w;
    r1.h = h;
    if (tmp_Surface[id] != NULL)
    {
        SDL_DestroyTexture(tmp_Surface[id]);
    }
    tmp_Surface[id] = SDL_CreateTexture(g_Renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, w, h);
    SDL_SetRenderTarget(g_Renderer, tmp_Surface[id]);
    SDL_RenderCopy(g_Renderer, g_Texture, &r1, NULL);
    //tmp_Surface[id] = SDL_CreateRGBSurface(SDL_SWSURFACE, r1.w, r1.h, g_Surface->format->BitsPerPixel
    //    , g_Surface->format->Rmask, g_Surface->format->Gmask, g_Surface->format->Bmask, g_Surface->format->Amask);
    //SDL_BlitSurface(g_Surface, &r1, tmp_Surface[id], NULL);
    return id;
}

//加载临时表面到屏幕
export int JY_LoadSur(int id, int x, int y)
{
    SDL_Rect r1;
    if (id < 0 || id > SURFACE_NUM - 1 || tmp_Surface[id] == NULL)
    {
        return 1;
    }
    r1.x = (Sint16)x;
    r1.y = (Sint16)y;
    if (tmp_Surface[id] == NULL)
    {
        return 1;
    }
    SDL_QueryTexture(tmp_Surface[id], NULL, NULL, &r1.w, &r1.h);
    SDL_SetRenderTarget(g_Renderer, g_Texture);
    SDL_RenderCopy(g_Renderer, tmp_Surface[id], NULL, &r1);
    //SDL_BlitSurface(tmp_Surface[id], NULL, g_Surface, &r1);
    return 0;
}

//释放
export int JY_FreeSur(int id)
{
    if (id < 0 || id > SURFACE_NUM - 1 || tmp_Surface[id] == NULL)
    {
        return 1;
    }
    if (tmp_Surface[id] != NULL)
    {
        SDL_DestroyTexture(tmp_Surface[id]);
        tmp_Surface[id] = NULL;
    }
    return 0;
}

//矩形右转90度
export SDL_Rect RotateRect(const SDL_Rect* rect)
{
    SDL_Rect r;
    r.x = (Sint16)(g_ScreenH - rect->y - rect->h);
    r.y = rect->x;
    r.w = rect->h;
    r.h = rect->w;
    return r;
}

//矩形左转90度
export SDL_Rect RotateReverseRect(const SDL_Rect* rect)
{
    SDL_Rect r;
    r.x = rect->y;
    r.y = (Sint16)(g_ScreenH - rect->x - rect->w);
    r.w = rect->h;
    r.h = rect->w;
    return r;
}
export    //按照原来游戏的RLE格式创建表面
    SDL_Texture*
    CreateTextureFromRLE(unsigned char* data, int w, int h, int datalong)
{
    int p = 0;
    int i, j;
    int yoffset;
    int row;
    int start;
    int x;
    int solidnum;
    SDL_Surface *ps1, *ps2;
    Uint32* data32 = NULL;

    data32 = (Uint32*)malloc(w * h * 4);
    if (data32 == NULL)
    {
        JY_Error("CreatePicSurface32: cannot malloc data32 memory!\n");
        return NULL;
    }

    for (i = 0; i < w * h; i++)
    {
        data32[i] = 0;
    }

    for (i = 0; i < h; i++)
    {
        yoffset = i * w;
        row = data[p];    // i行数据个数
        start = p;
        p++;
        if (row > 0)
        {
            x = 0;    // i行目前列
            for (;;)
            {
                x = x + data[p];    // i行空白点个数，跳个透明点
                if (x >= w)         // i行宽度到头，结束
                {
                    break;
                }

                p++;
                solidnum = data[p];    // 不透明点个数
                p++;
                for (j = 0; j < solidnum; j++)
                {
                    if (g_Rotate == 0)
                    {
                        data32[yoffset + x] = m_color32[data[p]] | AMASK;
                    }
                    else
                    {
                        data32[h - i - 1 + x * h] = m_color32[data[p]] | AMASK;
                    }
                    p++;
                    x++;
                }
                if (x >= w)
                {
                    break;
                }    // i行宽度到头，结束
                if (p - start >= row)
                {
                    break;
                }    // i行没有数据，结束
            }
            if (p + 1 >= datalong)
            {
                break;
            }
        }
    }
    ps1 = SDL_CreateRGBSurfaceFrom(data32, w, h, 32, w * 4, RMASK, GMASK, BMASK, AMASK);    //创建32位表面
    if (ps1 == NULL)
    {
        JY_Error("CreatePicSurface32: cannot create SDL_Surface ps1!\n");
    }
    ps2 = SDL_ConvertSurfaceFormat(ps1, g_Surface->format->format, 0);
    if (ps2 == NULL)
    {
        JY_Error("CreatePicSurface32: cannot create SDL_Surface ps2!\n");
    }
    auto tex = SDL_CreateTextureFromSurface(g_Renderer, ps2);
    SDL_FreeSurface(ps1);
    SDL_FreeSurface(ps2);
    SafeFree(data32);
    return tex;
}
// 把表面blit到背景或者前景表面
// x,y 要加载到表面的左上角坐标
export int RenderTexture(SDL_Texture* lps, int x, int y, int flag, int value, int color, int width, int height, double rotate, SDL_RendererFlip reversal, int percent)
{
    SDL_Surface* tmps;
    SDL_Rect rect, rect0;
    int i, j;
    //color = ConvertColor(g_MaskColor32);
    if (value > 255)
    {
        value = 255;
    }
    rect.x = x;
    rect.y = y;
    SDL_QueryTexture(lps, NULL, NULL, &rect.w, &rect.h);

    rect.w *= percent / 100.0;
    rect.h *= percent / 100.0;

    if (width > 0 && height > 0)
    {
        rect.w = width;
        rect.h = height;
    }
    else if (width > 0 && height <= 0)
    {
        rect.h = width * rect.h / rect.w;
        rect.w = width;
    }
    rect0 = rect;
    rect0.x = 0;
    rect0.y = 0;
    if (!lps)
    {
        JY_Error("BlitSurface: lps is null!");
        return 1;
    }

    if ((flag & 0x2) == 0)    // 没有alpha
    {
        SDL_SetTextureColorMod(lps, 255, 255, 255);
        SDL_SetTextureBlendMode(lps, SDL_BLENDMODE_BLEND);
        SDL_SetTextureAlphaMod(lps, 255);
        RenderToTexture(lps, NULL, g_Texture, &rect, rotate, NULL, reversal);
        //SDL_BlitSurface(lps, NULL, g_Surface, &rect);
    }
    else    // 有alpha
    {
        if ((flag & 0x4) || (flag & 0x8) || (flag & 0x10))    // 4-黑, 8-白, 16-颜色
        {
            // 4-黑, 8-白, 16-颜色
            if (flag & 0x4)
            {
                SDL_SetTextureColorMod(lps, 32, 32, 32);
                SDL_SetTextureBlendMode(lps, SDL_BLENDMODE_BLEND);
                SDL_SetTextureAlphaMod(lps, (Uint8)value);
                RenderToTexture(lps, NULL, g_Texture, &rect, rotate, NULL, reversal);
            }
            else if (flag & 0x8)
            {
                SDL_SetTextureColorMod(lps, 255, 255, 255);
                SDL_SetTextureBlendMode(lps, SDL_BLENDMODE_NONE);
                SDL_SetTextureAlphaMod(lps, 255);
                RenderToTexture(lps, NULL, g_TextureTmp, &rect, rotate, NULL, reversal);
                SDL_SetTextureBlendMode(lps, SDL_BLENDMODE_ADD);
                SDL_SetRenderDrawColor(g_Renderer, 255, 255, 255, 255);
                SDL_SetRenderDrawBlendMode(g_Renderer, SDL_BLENDMODE_ADD);
                SDL_RenderFillRect(g_Renderer, &rect);
                SDL_SetTextureColorMod(g_TextureTmp, 255, 255, 255);
                SDL_SetTextureBlendMode(g_TextureTmp, SDL_BLENDMODE_BLEND);
                SDL_SetTextureAlphaMod(g_TextureTmp, (Uint8)value);
                RenderToTexture(g_TextureTmp, &rect, g_Texture, &rect, rotate, NULL, reversal);
                SDL_SetTextureAlphaMod(g_TextureTmp, 255);
            }
            else
            {
                Uint8 r = (Uint8)((color & RMASK) >> 16);
                Uint8 g = (Uint8)((color & GMASK) >> 8);
                Uint8 b = (Uint8)((color & BMASK));
                Uint8 a = 255;
                SDL_SetTextureColorMod(lps, 255, 255, 255);
                SDL_SetTextureBlendMode(lps, SDL_BLENDMODE_NONE);
                SDL_SetTextureAlphaMod(lps, 255);
                RenderToTexture(lps, NULL, g_TextureTmp, &rect, rotate, NULL, reversal);
                SDL_SetTextureBlendMode(lps, SDL_BLENDMODE_ADD);
                SDL_SetRenderDrawColor(g_Renderer, r, g, b, a);
                SDL_SetRenderDrawBlendMode(g_Renderer, SDL_BLENDMODE_ADD);
                SDL_RenderFillRect(g_Renderer, &rect);
                SDL_SetTextureColorMod(g_TextureTmp, 255, 255, 255);
                SDL_SetTextureBlendMode(g_TextureTmp, SDL_BLENDMODE_BLEND);
                SDL_SetTextureAlphaMod(g_TextureTmp, (Uint8)value);
                RenderToTexture(g_TextureTmp, &rect, g_Texture, &rect, rotate, NULL, reversal);
                SDL_SetTextureAlphaMod(g_TextureTmp, 255);
                //SDL_SetTextureColorMod(lps, r, g, b);
                //SDL_SetTextureBlendMode(lps, SDL_BLENDMODE_BLEND);
                //SDL_SetTextureAlphaMod(lps, (Uint8)value);
                //RenderToTexture(lps, NULL, g_Texture, &rect);
            }
        }
        else
        {
            SDL_SetTextureColorMod(lps, 255, 255, 255);
            SDL_SetTextureBlendMode(lps, SDL_BLENDMODE_BLEND);
            SDL_SetTextureAlphaMod(lps, (Uint8)value);
            RenderToTexture(lps, NULL, g_Texture, &rect, rotate, NULL, reversal);
            //SDL_BlitSurface(lps, NULL, g_Surface, &rect);
        }
    }
    return 0;
}
