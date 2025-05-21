

// SDL 相关函数

#include "sdlfun.h"
#include "PotDll.h"
#include "charset.h"
#include "jymain.h"
#include "mainmap.h"
#include "piccache.h"

HSTREAM currentMusic = 0;    //播放音乐数据，由于同时只播放一个，用一个变量
#define WAVNUM 5
HSAMPLE WavChunk[WAVNUM];    //播放音效数据，可以同时播放几个，因此用数组
BASS_MIDI_FONT midfonts;
int currentWav = 0;    //当前播放的音效

#define RECTNUM 20
SDL_FRect ClipRect[RECTNUM];    // 当前设置的剪裁矩形
int currentRect = 0;

//手柄支持相关设置
int Key1 = 0;    //模拟已按键位
int Key2 = 0;
SDL_Gamepad* ctrls[8] = { NULL };
const int Controller_DEAD_ZONE = 20000;    //手柄死区设置
float axes[16] = { 0.0f };

//#define SURFACE_NUM  20
//SDL_Texture* tmp_Surface[SURFACE_NUM];   //JY_SaveSur使用

//过滤ESC、RETURN、SPACE键，使他们按下后不能重复。
bool KeyFilter(void* data, SDL_Event* event)
{
    static int Esc_KeyPress = 0;
    static int Space_KeyPress = 0;
    static int Return_KeyPress = 0;
    int r = 1;
    switch (event->type)
    {
    case SDL_EVENT_KEY_DOWN:
        switch (event->key.key)
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
    case SDL_EVENT_KEY_UP:
        switch (event->key.key)
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
int InitSDL(void)
{
    int r;
    int i;
    //char tmpstr[255];
    int so = 22050;
    //初始化检测手柄

#ifdef _WIN32
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "direct3d12,direct3d,direct3d11,opengl,vulkan");
#endif

    r = (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD) != 0);
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
int ExitSDL(void)
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
Uint32 ConvertColor(Uint32 color)
{
    Uint8* p = (Uint8*)&color;
    return SDL_MapSurfaceRGBA(g_Surface, *(p + 2), *(p + 1), *p, 255);
}

// 初始化游戏数据
int InitGame(void)
{
    int w = g_ScreenW;
    int h = g_ScreenH;
    if (g_Rotate)
    {
        swap(w, h);
    }
    //putenv ("SDL_VIDEO_WINDOW_POS");
    //putenv ("SDL_VIDEO_CENTERED=1");
    //SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, g_Softener);
    {
        Prop props;
        props.set(SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, true);
        props.set(SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, w);
        props.set(SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, h);
        props.set(SDL_PROP_WINDOW_CREATE_TITLE_STRING, (const char*)u8"金书群侠传");
        g_Window = SDL_CreateWindowWithProperties(props.id());
    }
    SDL_SetWindowIcon(g_Window, IMG_Load("ff.ico"));
    {
        Prop props;
        props.set(SDL_PROP_RENDERER_CREATE_WINDOW_POINTER, g_Window);
        g_Renderer = SDL_CreateRendererWithProperties(props.id());
    }
    g_Texture = CreateRenderedTexture(g_ScreenW, g_ScreenH);
    SDL_SetTextureScaleMode(g_Texture, SDL_SCALEMODE_LINEAR);
    g_TextureShow = CreateRenderedTexture(g_ScreenW, g_ScreenH);
    g_TextureTmp = CreateRenderedTexture(g_ScreenW, g_ScreenH);

    g_Surface = SDL_CreateSurface(1, 1, SDL_GetPixelFormatForMasks(32, RMASK, GMASK, BMASK, AMASK));
    //SDL_WM_SetCaption("The Fall of Star",_("ff.ico"));         //这是显示窗口的
    //SDL_WM_SetIcon(IMG_Load(_("ff.ico")), NULL);
    if (g_FullScreen == 1)
    {
        SDL_SetWindowFullscreen(g_Window, true);
    }
    else
    {
        SDL_SetWindowFullscreen(g_Window, false);
    }
    if (g_Window == NULL || g_Renderer == NULL || g_Texture == NULL || g_TextureShow == NULL)
    {
        JY_Error("Cannot set video mode");
    }

    SDL_SetGamepadEventsEnabled(true);
	int i, num_joysticks;
	SDL_JoystickID* joysticks = SDL_GetJoysticks(&num_joysticks);
	if (joysticks) {
		for (i = 0; i < num_joysticks; ++i) {
			SDL_JoystickID instance_id = joysticks[i];
			if (SDL_IsGamepad(instance_id))
			{
				ctrls[i] = SDL_OpenGamepad(instance_id);
			}
		}
    }

    Init_Cache();
    JY_PicInit("");    // 初始化贴图cache
    g_Particle.setRenderer(g_Renderer);
    g_Particle.setPosition(w / 2, 0);
    g_Particle.getDefaultTexture();
    return 0;
}

// 释放游戏资源
int ExitGame(void)
{
    SDL_DestroyTexture(g_Texture);
    SDL_DestroyTexture(g_TextureShow);
    for (int i = 0; i < 8; ++i)
    {
        if (ctrls[i])
        {
            SDL_CloseGamepad(ctrls[i]);
            ctrls[i] = NULL;
        }
    }
    SDL_DestroyRenderer(g_Renderer);
    SDL_DestroyWindow(g_Window);
    //JY_PicInit("");
    JY_LoadPicture("", 0, 0);
    JY_UnloadMMap();      //释放主地图内存
    JY_UnloadSMap();      //释放场景地图内存
    JY_UnloadWarMap();    //释放战斗地图内存
    return 0;
}

int RenderToTexture(SDL_Texture* src, SDL_FRect* src_rect, SDL_Texture* dst, SDL_FRect* dst_rect, double angle, SDL_FPoint* center, SDL_FlipMode filp)
{
    SDL_SetRenderTarget(g_Renderer, dst);
    return SDL_RenderTextureRotated(g_Renderer, src, src_rect, dst_rect, angle, center, filp);
}

SDL_Texture* CreateRenderedTexture(SDL_Texture* ref)
{
    float w, h;
    SDL_GetTextureSize(ref, &w, &h);
    return CreateRenderedTexture(w, h);
}

SDL_Texture* CreateRenderedTexture(int w, int h)
{
    return SDL_CreateTexture(g_Renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, w, h);
}

//加载图形文件，其他格式也可以加载
//x,y =-1 则加载到屏幕中心
//    如果未加载，则加载，然后blit，如果加载，直接blit
//  str 文件名，如果为空，则释放表面
int JY_LoadPicture(const char* str, int x, int y)
{
    static char filename[255] = "\0";
    static SDL_Texture* tex = NULL;
    static SDL_FRect r;
    SDL_Surface* tmppic;
    SDL_Surface* pic = NULL;
    if (strlen(str) == 0)    // 为空则释放表面
    {
        if (pic)
        {
            SDL_DestroySurface(pic);
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
            pic = SDL_ConvertSurface(tmppic, g_Surface->format);    // 改为当前表面的像素格式
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
            SDL_DestroySurface(pic);
            SDL_DestroySurface(tmppic);
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
int JY_ShowSurface(int flag)
{
    SDL_SetRenderTarget(g_Renderer, g_TextureShow);
    if (flag == 1)
    {
        if (currentRect > 0)
        {
            for (int i = 0; i < currentRect; i++)
            {
                SDL_FRect* r = ClipRect + i;
                SDL_RenderTexture(g_Renderer, g_Texture, r, r);
            }
        }
    }
    else
    {
        SDL_RenderTexture(g_Renderer, g_Texture, NULL, NULL);
    }
    SDL_SetRenderTarget(g_Renderer, NULL);
    //SDL_Rect r;
    //SDL_GetRenderClipRect(g_Renderer, &r);
    SDL_SetRenderClipRect(g_Renderer, NULL);
    if (g_Rotate == 0)
    {
        SDL_RenderTexture(g_Renderer, g_TextureShow, NULL, NULL);
    }
    else
    {
        SDL_RenderTextureRotated(g_Renderer, g_TextureShow, NULL, NULL, 90, NULL, SDL_FLIP_NONE);
    }
    SDL_RenderPresent(g_Renderer);
    //SDL_SetRenderClipRect(g_Renderer, &r);
    return 0;
}

//延时x毫秒
int JY_Delay(int x)
{
    SDL_Delay(x);
    g_DelayTimes++;
    return 0;
}

// 缓慢显示图形
// delaytime 每次渐变延时毫秒数
// Flag=0 从暗到亮，1，从亮到暗
int JY_ShowSlow(int delaytime, int Flag)
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
    SDL_RenderTexture(g_Renderer, g_Texture, NULL, NULL);
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
        //SDL_RenderTexture(g_Renderer, lps1, NULL, NULL);
        //SDL_BlitSurface(lps1, NULL, g_Surface, NULL);
        SDL_SetRenderTarget(g_Renderer, g_Texture);
        SDL_RenderTexture(g_Renderer, lps1, NULL, NULL);
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

__int64 GetCycleCount(){
    __asm _emit 0x0F __asm _emit 0x31
}

#endif

//得到当前时间，单位毫秒
double JY_GetTime()
{
#ifdef HIGH_PRECISION_CLOCK
    return (double)(GetCycleCount()) / (1000 * CPU_FREQUENCY);
#else
    return (double)SDL_GetTicks();
#endif
}

//播放音乐
int JY_PlayMIDI(const char* filename)
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
int StopMIDI()
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
int JY_PlayWAV(const char* filename)
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
int JY_GetKey(int* key, int* type, int* mx, int* my)
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
        case SDL_EVENT_KEY_DOWN:
            *key = event.key.key;
            if (*key == SDLK_SPACE)
            {
                *key = SDLK_RETURN;
            }
            *type = 1;
            break;
        case SDL_EVENT_KEY_UP:
            //*key = event.key.key;
            //if (*key == SDLK_SPACE)
            //{
            //    *key = SDLK_RETURN;
            //}
            break;
        case SDL_EVENT_MOUSE_MOTION:    //鼠标移动
            SDL_GetWindowSize(g_Window, &win_w, &win_h);
            *mx = event.motion.x * g_ScreenW / win_w;
            *my = event.motion.y * g_ScreenH / win_h;
            if (g_Rotate) { swap(*mx, *my); }
            *type = 2;
            break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:    //鼠标点击
            SDL_GetWindowSize(g_Window, &win_w, &win_h);
            *mx = event.motion.x * g_ScreenW / win_w;
            *my = event.motion.y * g_ScreenH / win_h;
            if (g_Rotate) { swap(*mx, *my); }
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
        case SDL_EVENT_MOUSE_WHEEL:
            //无酒不欢:添加鼠标滚轮
            if (event.wheel.y == 1)
            {
                *type = 6;
            }
            else if (event.wheel.y == -1)
            {
                *type = 7;
            }
            break;
        case SDL_EVENT_GAMEPAD_AXIS_MOTION:
        {
            axes[event.gaxis.axis] = event.gaxis.value;
            if ((axes[1] > Controller_DEAD_ZONE) && (axes[0] > Controller_DEAD_ZONE))    //右下
            {
                Key1 = 1073741905;
                Key2 = 1073741903;
            }
            else if ((axes[1] < -Controller_DEAD_ZONE) && (axes[0] > Controller_DEAD_ZONE))    //右上
            {
                Key1 = 1073741906;
                Key2 = 1073741903;
            }
            else if ((axes[1] > Controller_DEAD_ZONE) && (axes[0] < -Controller_DEAD_ZONE))    //左下
            {
                Key1 = 1073741905;
                Key2 = 1073741904;
            }
            else if ((axes[1] < -Controller_DEAD_ZONE) && (axes[0] < -Controller_DEAD_ZONE))    //左上
            {
                Key1 = 1073741906;
                Key2 = 1073741904;
            }
            else if ((axes[1] > Controller_DEAD_ZONE) && ((Key1 != 1073741905) || (Key2 != 0)))    //下
            {
                Key1 = 1073741905;
                Key2 = 0;
                *key = SDLK_DOWN;
                *type = 1;
                break;
            }
            else if ((axes[1] < -Controller_DEAD_ZONE) && ((Key1 != 1073741906) || (Key2 != 0)))    //上
            {
                Key1 = 1073741906;
                Key2 = 0;
                *key = SDLK_UP;
                *type = 1;
                break;
            }
            else if ((axes[0] < -Controller_DEAD_ZONE) && ((Key1 != 0) || (Key2 != 1073741904)))    //左
            {
                Key1 = 0;
                Key2 = 1073741904;
                *key = SDLK_LEFT;
                *type = 1;
                break;
            }
            else if ((axes[0] > Controller_DEAD_ZONE) && ((Key1 != 0) || (Key2 != 1073741903)))    //右
            {
                Key1 = 0;
                Key2 = 1073741903;
                *key = SDLK_RIGHT;
                *type = 1;
                break;
            }
            else if ((axes[2] > Controller_DEAD_ZONE) && (Key1 != SDLK_P))    //右摇杆下
            {
                Key1 = SDLK_P;
                *key = SDLK_P;
                *type = 1;
                break;
            }
            else if ((axes[2] < -Controller_DEAD_ZONE) && (Key1 != SDLK_D))    //右摇杆上
            {
                Key1 = SDLK_D;
                *key = SDLK_D;
                *type = 1;
                break;
            }
            else if ((axes[3] < -Controller_DEAD_ZONE) && (Key1 != SDLK_W))    //右摇杆左
            {
                Key1 = SDLK_W;
                *key = SDLK_W;
                *type = 1;
                break;
            }
            else if ((axes[3] > Controller_DEAD_ZONE) && (Key1 != SDLK_J))    //右摇杆右
            {
                Key1 = SDLK_J;
                *key = SDLK_J;
                *type = 1;
                break;
            }
            else if ((axes[4] > Controller_DEAD_ZONE) && (Key1 != SDLK_E))    //L2
            {
                Key1 = SDLK_E;
                *key = SDLK_E;
                *type = 1;
                break;
            }
            else if ((axes[5] > Controller_DEAD_ZONE) && (Key1 != SDLK_Z))    //R2
            {
                Key1 = SDLK_Z;
                *key = SDLK_Z;
                *type = 1;
                break;
            }
            else if ((abs(axes[0]) < 4000) && (abs(axes[1]) < 4000) && (abs(axes[2]) < 4000) && (abs(axes[3]) < 4000))    //摇杆复位
            {
                Key1 = 0;
                Key2 = 0;
            }
        }
        break;
        case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
        {
            if (event.gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_UP)
            {
                if ((Key2 == 1073741903) || (Key2 == 1073741904))
                {
                    Key1 = 1073741906;
                }
                else
                {
                    Key1 = 1073741906;
                    Key2 = 0;
                    *key = SDLK_UP;
                }
            }
            else if (event.gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_DOWN)
            {
                if ((Key2 == 1073741903) || (Key2 == 1073741904))
                {
                    Key1 = 1073741905;
                }
                else
                {
                    Key1 = 1073741905;
                    Key2 = 0;
                    *key = SDLK_DOWN;
                }
            }
            else if (event.gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_LEFT)
            {
                if ((Key1 == 1073741905) || (Key1 == 1073741906))
                {
                    Key2 = 1073741904;
                }
                else
                {
                    Key2 = 1073741904;
                    Key1 = 0;
                    *key = SDLK_LEFT;
                }
            }
            else if (event.gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_RIGHT)
            {
                if ((Key1 == 1073741905) || (Key1 == 1073741906))
                {
                    Key2 = 1073741903;
                }
                else
                {
                    Key2 = 1073741903;
                    Key1 = 0;
                    *key = SDLK_RIGHT;
                }
            }
            else if (event.gbutton.button == SDL_GAMEPAD_BUTTON_SOUTH)
            {
                if (g_SwitchABXY == 1)
                {
                    *key = SDLK_ESCAPE;
                }
                else
                {
                    *key = SDLK_RETURN;
                }
            }
            else if (event.gbutton.button == SDL_GAMEPAD_BUTTON_EAST)
            {
                if (g_SwitchABXY == 1)
                {
                    *key = SDLK_RETURN;
                }
                else
                {
                    *key = SDLK_ESCAPE;
                }
            }
            else if (event.gbutton.button == SDL_GAMEPAD_BUTTON_WEST)
            {
                if (g_SwitchABXY == 1)
                {
                    *key = SDLK_F1;
                }
                else
                {
                    *key = SDLK_H;
                }
            }
            else if (event.gbutton.button == SDL_GAMEPAD_BUTTON_NORTH)
            {
                if (g_SwitchABXY == 1)
                {
                    *key = SDLK_H;
                }
                else
                {
                    *key = SDLK_F1;
                }
            }
            else if (event.gbutton.button == SDL_GAMEPAD_BUTTON_LEFT_SHOULDER)
            {
                *key = SDLK_S;
            }
            else if (event.gbutton.button == SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER)
            {
                *key = SDLK_L;
            }
            else if (event.gbutton.button == SDL_GAMEPAD_BUTTON_BACK)
            {
                *key = SDLK_C;
            }
            else if (event.gbutton.button == SDL_GAMEPAD_BUTTON_START)
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
                        ExitGame();    //释放游戏数据
                        ExitSDL();     //退出SDL
                        exit(1);
                    }
                    quit = 0;
                }
            }
            if ((event.gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_UP) && (event.gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_LEFT))
            {
                Key1 = 1073741904;
                Key2 = 1073741906;
            }
            else if ((event.gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_UP) && (event.gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_RIGHT))
            {
                Key1 = 1073741903;
                Key2 = 1073741906;
            }
            else if ((event.gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_DOWN) && (event.gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_LEFT))
            {
                Key1 = 1073741904;
                Key2 = 1073741905;
            }
            else if ((event.gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_DOWN) && (event.gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_RIGHT))
            {
                Key1 = 1073741903;
                Key2 = 1073741905;
            }
            *type = 1;
            break;
        }
        case SDL_EVENT_GAMEPAD_BUTTON_UP:
        {
            if ((event.gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_LEFT) || (event.gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_RIGHT))
            {
                Key2 = 0;
            }
            else if ((event.gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_UP) || (event.gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_DOWN))
            {
                Key1 = 0;
            }
            break;
        }
        //支持手柄热插拔
        case SDL_EVENT_GAMEPAD_ADDED:
        case SDL_EVENT_GAMEPAD_REMOVED:
        {
            JY_Error("Controller added or removed");
            for (int i = 0; i < 8; ++i)
			{
				if (ctrls[i])
				{
					SDL_CloseGamepad(ctrls[i]);
					ctrls[i] = NULL;
				}
			}
			SDL_SetGamepadEventsEnabled(true);
			int i, num_joysticks;
			SDL_JoystickID* joysticks = SDL_GetJoysticks(&num_joysticks);
			if (joysticks) {
				for (i = 0; i < num_joysticks; ++i) {
					SDL_JoystickID instance_id = joysticks[i];
					if (SDL_IsGamepad(instance_id))
					{
						ctrls[i] = SDL_OpenGamepad(instance_id);
					}
				}
			}
        }
        break;
        case SDL_EVENT_QUIT:
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
                    ExitGame();    //释放游戏数据
                    ExitSDL();     //退出SDL
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

int JY_GetKeyState(int key)
{
    if ((Key1 == key) || (Key2 == key))    //手柄模拟按键状态
    {
        return 1;
    }
    else
    {
        return SDL_GetKeyboardState(NULL)[SDL_GetScancodeFromKey(key, NULL)];
    }
}

//设置裁剪
int JY_SetClip(int x1, int y1, int x2, int y2)
{
    SDL_Rect rect;
    SDL_SetRenderTarget(g_Renderer, g_Texture);
    if (x1 == 0 && y1 == 0 && x2 == 0 && y2 == 0)
    {
        rect.x = 0;
        rect.y = 0;
        rect.w = g_ScreenW;
        rect.h = g_ScreenH;
        SDL_SetRenderClipRect(g_Renderer, &rect);
        //SDL_SetSurfaceClipRect(g_Surface, NULL);
        currentRect = 0;
    }
    else
    {
        SDL_FRect rect;
        SDL_Rect recti;
        rect.x = x1;
        rect.y = y1;
        rect.w = x2 - x1;
        rect.h = y2 - y1;
        recti.x = rect.x;
        recti.y = rect.y;
        recti.w = rect.w;
        recti.h = rect.h;
        ClipRect[currentRect] = rect;
        SDL_SetRenderClipRect(g_Renderer, &recti);
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
int JY_DrawRect(int x1, int y1, int x2, int y2, int color)
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
void HLine32(int x1, int x2, int y, int color)
{
    Uint8 r = (Uint8)((color & RMASK) >> 16);
    Uint8 g = (Uint8)((color & GMASK) >> 8);
    Uint8 b = (Uint8)((color & BMASK));
    Uint8 a = 255;
    SDL_SetRenderTarget(g_Renderer, g_Texture);
    SDL_SetRenderDrawColor(g_Renderer, r, g, b, a);
    SDL_SetRenderDrawBlendMode(g_Renderer, SDL_BLENDMODE_BLEND);
    SDL_RenderLine(g_Renderer, x1, y, x2, y);
}

//绘垂直线
void VLine32(int x1, int x2, int y, int color)
{
    Uint8 r = (Uint8)((color & RMASK) >> 16);
    Uint8 g = (Uint8)((color & GMASK) >> 8);
    Uint8 b = (Uint8)((color & BMASK));
    Uint8 a = 255;
    SDL_SetRenderTarget(g_Renderer, g_Texture);
    SDL_SetRenderDrawColor(g_Renderer, r, g, b, a);
    SDL_SetRenderDrawBlendMode(g_Renderer, SDL_BLENDMODE_BLEND);
    SDL_RenderLine(g_Renderer, y, x1, y, x2);
}

// 图形填充
// 如果x1,y1,x2,y2均为0，则填充整个表面
// color, 填充色，用RGB表示，从高到低字节为0RGB
int JY_FillColor(int x1, int y1, int x2, int y2, int color)
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
        //SDL_FillSurfaceRect(g_Surface, NULL, c);
    }
    else
    {
        SDL_FRect rect;
        rect.x = x1;
        rect.y = y1;
        rect.w = x2 - x1;
        rect.h = y2 - y1;
        SDL_RenderFillRect(g_Renderer, &rect);
        //SDL_FillSurfaceRect(g_Surface, &rect, c);
    }
    return 0;
}

// 背景变暗
// 把源表面(x1,y1,x2,y2)矩形内的所有点亮度降低
// bright 亮度等级 0-256
int JY_Background(int x1, int y1, int x2, int y2, int Bright, int color)
{
    SDL_FRect r1;
    if (x2 <= x1 || y2 <= y1)
    {
        return 0;
    }
    Bright = 256 - Bright;
    if (Bright > 255)
    {
        Bright = 255;
    }
    r1.x = x1;
    r1.y = y1;
    r1.w = x2 - x1;
    r1.h = y2 - y1;
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
int JY_PlayMPEG(char* filename, int esckey)
{
#ifdef WIN32
    if (g_Tinypot == NULL)
    {
        g_Tinypot = PotCreateFromWindow(g_Window);
    }
    StopMIDI();
    //int r = PotInputVideo(g_Tinypot, filename);
    //if (r == 1)
    {
        SDL_Event e;
        e.type = SDL_EVENT_QUIT;
        SDL_PushEvent(&e);
    }
#endif
    //g_Tinypot = NULL;
    return 0;
}

//取s的值
int JY_SetSound(int id, int flag)
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
int JY_FullScreen()
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
        SDL_SetWindowFullscreen(g_Window, true);
    }
    else
    {
        SDL_SetWindowFullscreen(g_Window, false);
    }
    //if (flag & SDL_FULLSCREEN)    //全屏，设置窗口
    //{ g_Surface = SDL_SetVideoMode(g_Surface->w, g_Surface->h, 0, SDL_SWSURFACE); }
    //else
    //{ g_Surface = SDL_SetVideoMode(g_Surface->w, g_Surface->h, g_ScreenBpp, SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_FULLSCREEN); }
    //SDL_BlitSurface(tmpsurface, NULL, g_Surface, NULL);
    JY_ShowSurface(0);
    //SDL_DestroySurface(tmpsurface);
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
int JY_SaveSur(int x, int y, int w, int h)
{
    int id = -1;
    int i;
    SDL_FRect r1;
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
    SDL_RenderTexture(g_Renderer, g_Texture, &r1, NULL);
    //tmp_Surface[id] = SDL_CreateRGBSurface(SDL_SWSURFACE, r1.w, r1.h, g_Surface->format->BitsPerPixel
    //    , g_Surface->format->Rmask, g_Surface->format->Gmask, g_Surface->format->Bmask, g_Surface->format->Amask);
    //SDL_BlitSurface(g_Surface, &r1, tmp_Surface[id], NULL);
    return id;
}

//加载临时表面到屏幕
int JY_LoadSur(int id, int x, int y)
{
    SDL_FRect r1;
    if (id < 0 || id > SURFACE_NUM - 1 || tmp_Surface[id] == NULL)
    {
        return 1;
    }
    r1.x = x;
    r1.y = y;
    if (tmp_Surface[id] == NULL)
    {
        return 1;
    }
    SDL_GetTextureSize(tmp_Surface[id], &r1.w, &r1.h);
    SDL_SetRenderTarget(g_Renderer, g_Texture);
    SDL_RenderTexture(g_Renderer, tmp_Surface[id], NULL, &r1);
    //SDL_BlitSurface(tmp_Surface[id], NULL, g_Surface, &r1);
    return 0;
}

//释放
int JY_FreeSur(int id)
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
SDL_Rect RotateRect(const SDL_Rect* rect)
{
    SDL_Rect r;
    r.x = (Sint16)(g_ScreenH - rect->y - rect->h);
    r.y = rect->x;
    r.w = rect->h;
    r.h = rect->w;
    return r;
}

//矩形左转90度
SDL_Rect RotateReverseRect(const SDL_Rect* rect)
{
    SDL_Rect r;
    r.x = rect->y;
    r.y = (Sint16)(g_ScreenH - rect->x - rect->w);
    r.w = rect->h;
    r.h = rect->w;
    return r;
}
