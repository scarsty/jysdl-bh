

// SDL ��غ���

#include "jymain.h"
#include "SDL2_rotozoom.h"

static HSTREAM currentMusic = 0;		//�����������ݣ�����ͬʱֻ����һ������һ������

#define WAVNUM 5

static HSAMPLE WavChunk[WAVNUM];        //������Ч���ݣ�����ͬʱ���ż��������������

static BASS_MIDI_FONT midfonts;

static int currentWav = 0;                //��ǰ���ŵ���Ч

#define RECTNUM  20
static SDL_Rect ClipRect[RECTNUM];        // ��ǰ���õļ��þ���
static int currentRect = 0;

extern SDL_Surface* g_Surface;        // ��Ϸʹ�õ���Ƶ����
extern Uint32 g_MaskColor32;      // ͸��ɫ

extern int g_Rotate;

extern int g_ScreenW ;
extern int g_ScreenH ;
extern int g_ScreenBpp ;

extern int g_FullScreen;
extern int g_EnableSound;
extern int g_MusicVolume;
extern int g_SoundVolume;
extern float g_Zoom;
extern int g_MP3;
extern char g_MidSF2[255];

#define SURFACE_NUM  20
static SDL_Surface* tmp_Surface[SURFACE_NUM];	//JY_SaveSurʹ��
static SDL_Surface* tmp_Surface2[SURFACE_NUM];


static SDL_Window* win;
static SDL_Renderer* render;
static SDL_Texture* texture;


//����ESC��RETURN��SPACE����ʹ���ǰ��º����ظ���
static int KeyFilter(const SDL_Event* event)
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
// ��ʼ��SDL
int InitSDL(void)
{
    int r;
    int i;
    char tmpstr[255];
    int so = 22050;
    r = SDL_Init(SDL_INIT_VIDEO);
    if (r < 0)
    {
        JY_Error(
            "Couldn't initialize SDL: %s\n", SDL_GetError());
        exit(1);
    }
    //atexit(SDL_Quit);    ���������⣬���ε�
    //SDL_VideoDriverName(tmpstr, 255);
    JY_Debug("InitSDL: Video Driver: %s\n", tmpstr);
    InitFont();  //��ʼ��
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
    //SDL_SetEventFilter(KeyFilter);
    if (g_MP3 != 1)
    {
        midfonts.font = BASS_MIDI_FontInit(g_MidSF2, 0);
        if (!midfonts.font)
        {
            JY_Error("BASS_MIDI_FontInit error ! %d", BASS_ErrorGetCode());
        }
        midfonts.preset = -1; // use all presets
        midfonts.bank = 0; // use default bank(s)
        BASS_MIDI_StreamSetFonts(0, &midfonts, 1); // set default soundfont
    }
    return 0;
}

// �˳�SDL
int ExitSDL(void)
{
    int i;
    ExitFont();
    StopMIDI();
    if (midfonts.font)
    {
        BASS_MIDI_FontFree(midfonts.font);
    }
    for (i = 0; i < WAVNUM; i++)
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
    JY_LoadPicture("", 0, 0);  // �ͷſ��ܼ��ص�ͼƬ����
    SDL_Quit();
    return 0;
}

// ת��0RGB����ǰ��Ļ��ɫ
Uint32 ConvertColor(Uint32 color)
{
    Uint8* p = (Uint8*)&color;
    return SDL_MapRGB(g_Surface->format, *(p + 2), *(p + 1), *p);
}


// ��ʼ����Ϸ����
int InitGame(void)
{
    int w, h;
    if (g_Rotate == 0)
    {
        w = g_ScreenW;
        h = g_ScreenH;
    }
    else
    {
        w = g_ScreenH;
        h = g_ScreenW;
    }
    //putenv ("SDL_VIDEO_WINDOW_POS");
    //putenv ("SDL_VIDEO_CENTERED=1");
    //SDL_WM_SetCaption("\xE5\x85\x88\xE9\x94\x8B\xE7\xBE\xA4\xE4\xBE\xA0\xE4\xBC\xA0","game.ico");         //������ʾ���ڵ�
    //SDL_WM_SetIcon(SDL_LoadBMP("game.ico"), NULL);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
    win = SDL_CreateWindow("Lest We Forget", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, SDL_WINDOW_RESIZABLE);
	SDL_SetWindowIcon(win, IMG_Load("game.png"));
    render = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    texture = SDL_CreateTexture(render, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, w, h);
    //g_Surface=SDL_SetVideoMode(w, h, g_ScreenBpp, SDL_HWSURFACE|SDL_DOUBLEBUF|SDL_FULLSCREEN);
    g_Surface = SDL_CreateRGBSurface(0, w, h, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    Init_Cache();
    JY_PicInit("");        // ��ʼ����ͼcache
    return 0;
}




// �ͷ���Ϸ��Դ
int ExitGame(void)
{
    JY_PicInit("");
    JY_LoadPicture("", 0, 0);
    JY_UnloadMMap();     //�ͷ�����ͼ�ڴ�
    JY_UnloadSMap();     //�ͷų�����ͼ�ڴ�
    JY_UnloadWarMap();   //�ͷ�ս����ͼ�ڴ�
    return 0;
}

SDL_Surface* SDL_DisplayFormat(SDL_Surface* tmppic)
{
    return SDL_ConvertSurface(tmppic, g_Surface->format, g_Surface->flags);
}

SDL_Surface* SDL_DisplayFormatAlpha(SDL_Surface* tmppic)
{
    return SDL_ConvertSurface(tmppic, g_Surface->format, g_Surface->flags);
}



//����ͼ���ļ���������ʽҲ���Լ���
//x,y =-1 ����ص���Ļ����
//    ���δ���أ�����أ�Ȼ��blit��������أ�ֱ��blit
//  str �ļ��������Ϊ�գ����ͷű���
int JY_LoadPicture(const char* str, int x, int y)
{
    static char filename[255] = "\0";
    static SDL_Surface* pic = NULL;
    SDL_Surface* tmppic;
    SDL_Rect r;
    if (strlen(str) == 0)      // Ϊ�����ͷű���
    {
        if (pic)
        {
            SDL_FreeSurface(pic);
            pic = NULL;
        }
        return 0;
    }
    if (strcmp(str, filename) != 0) // ����ǰ�ļ�����ͬ�����ͷ�ԭ�����棬�����±���
    {
        if (pic)
        {
            SDL_FreeSurface(pic);
            pic = NULL;
        }
        tmppic = IMG_Load(str);
        if (tmppic)
        {
            pic = SDL_DisplayFormat(tmppic); // ��Ϊ��ǰ��������ظ�ʽ
            SDL_FreeSurface(tmppic);
            strcpy(filename, str);
        }
    }
    if (pic)
    {
        if ((x == -1) && (y == -1))
        {
            x = (g_ScreenW - pic->w) / 2;
            y = (g_ScreenH - pic->h) / 2;
        }
        r.x = x;
        r.y = y;
        SDL_BlitSurface(pic, NULL, g_Surface, &r);
    }
    else
    {
        JY_Error("JY_LoadPicture: Load picture file %s failed! %s", str, SDL_GetError());
    }
    return 0;
}



//��ʾ����
//flag = 0 ��ʾȫ������  =1 ����JY_SetClip���õľ�����ʾ�����û�о��Σ�����ʾ
int JY_ShowSurface(int flag)
{
    SDL_UpdateTexture(texture, NULL, g_Surface->pixels, g_Surface->pitch);
    SDL_RenderClear(render);
    SDL_RenderCopy(render, texture, NULL, NULL);
    SDL_RenderPresent(render);
    /*if(flag==1){
    	if(currentRect>0){
    		//SDL_UpdateRects(g_Surface,currentRect,ClipRect);
    	}
    }
    else{
    	//SDL_UpdateRect(g_Surface,0,0,0,0);
    }*/
    return 0;
}

//��ʱx����
int JY_Delay(int x)
{
    SDL_Delay(x);
    return 0;
}


// ������ʾͼ��
// delaytime ÿ�ν�����ʱ������
// Flag=0 �Ӱ�������1����������
int JY_ShowSlow(int delaytime, int Flag)
{
    int i;
    int step;
    int t1, t2;
    int alpha;
    SDL_Surface* lps1;  // ������ʱ����
    lps1 = SDL_CreateRGBSurface(SDL_SWSURFACE, g_Surface->w, g_Surface->h, g_Surface->format->BitsPerPixel,
                                g_Surface->format->Rmask, g_Surface->format->Gmask, g_Surface->format->Bmask, g_Surface->format->Amask);
    if (lps1 == NULL)
    {
        JY_Error("JY_ShowSlow: Create surface failed!");
        return 1;
    }
    SDL_BlitSurface(g_Surface , NULL, lps1, NULL); //��ǰ���渴�Ƶ���ʱ����
    for (i = 0; i <= 32; i++)
    {
        if (Flag == 0)
        {
            step = i;
        }
        else
        {
            step = 32 - i;
        }
        t1 = (int)JY_GetTime();
        SDL_FillRect(g_Surface, NULL, 0xff000000);        //��ǰ������
        alpha = step << 3;
        if (alpha > 255)
        {
            alpha = 255;
        }
        SDL_SetSurfaceAlphaMod(lps1, (Uint8)alpha);  //����alpha
        SDL_BlitSurface(lps1 , NULL, g_Surface, NULL);
        JY_ShowSurface(0);
        t2 = (int)JY_GetTime();
        if (delaytime / 10 > t2 - t1)
        {
            JY_Delay(delaytime - (t2 - t1));
        }
        JY_GetKey();
    }
    SDL_FreeSurface(lps1);       //�ͷű���
    return 0;
}


#ifdef HIGH_PRECISION_CLOCK

__int64 GetCycleCount()
{
    __asm _emit 0x0F
    __asm _emit 0x31
}

#endif

//�õ���ǰʱ�䣬��λ����
double JY_GetTime()
{
#ifdef HIGH_PRECISION_CLOCK
    return (double)(GetCycleCount()) / (1000 * CPU_FREQUENCY);
#else
    return (double) SDL_GetTicks();
#endif
}




//��������
int JY_PlayMIDI(const char* filename)
{
    static char currentfile[255] = "\0";
    if (g_EnableSound == 0)
    {
        JY_Error("disable sound!");
        return 1;
    }
    if (strlen(filename) == 0) //�ļ���Ϊ�գ�ֹͣ����
    {
        StopMIDI();
        strcpy(currentfile, filename);
        return 0;
    }
    if (strcmp(currentfile, filename) == 0) //�뵱ǰ�����ļ���ͬ��ֱ�ӷ���
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
        BASS_MIDI_StreamSetFonts(currentMusic, &midfonts, 1);    // set for current stream too
    }
    BASS_ChannelSetAttribute(currentMusic, BASS_ATTRIB_VOL, (float)(g_MusicVolume / 100.0));
    BASS_ChannelFlags(currentMusic, BASS_SAMPLE_LOOP, BASS_SAMPLE_LOOP);
    BASS_ChannelPlay(currentMusic, FALSE);
    strcpy(currentfile, filename);
    return 0;
}

//ֹͣ��Ч
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


//������Ч
int JY_PlayWAV(const char* filename)
{
    HCHANNEL ch;
    if (g_EnableSound == 0)
    {
        return 1;
    }
    if (WavChunk[currentWav])          //�ͷŵ�ǰ��Ч
    {
        //Mix_FreeChunk(WavChunk[currentWav]);
        BASS_SampleStop(WavChunk[currentWav]);
        BASS_SampleFree(WavChunk[currentWav]);          //�������Free������
        WavChunk[currentWav] = 0;
    }
    //WavChunk[currentWav]= Mix_LoadWAV(filename);  //���ص���ǰ��Ч
    WavChunk[currentWav] = BASS_SampleLoad(0, filename, 0, 0, 1, 0);
    if (WavChunk[currentWav])
    {
        //Mix_VolumeChunk(WavChunk[currentWav],g_SoundVolume);
        //Mix_PlayChannel(-1, WavChunk[currentWav], 0);  //������Ч
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


// �õ�ǰ�水�µ��ַ�
uint32_t JY_GetKey()
{
    char s[245];
    SDL_Event event;
    uint32_t keyPress = -1;
    int w_win, w_sur, h_win, h_sur;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            case SDL_KEYDOWN:
                keyPress = event.key.keysym.sym;
                break;
            case SDL_MOUSEBUTTONUP:
                SDL_GetWindowSize(win, &w_win, &h_win);
                keyPress = 2100000000 + event.button.x * g_Surface->w / w_win * 1000 + event.button.y * g_Surface->h / h_win;
                if (event.button.button == SDL_BUTTON_RIGHT)
                {
                    keyPress = SDLK_ESCAPE;
                }
                break;
            case SDL_MOUSEMOTION:
                SDL_GetWindowSize(win, &w_win, &h_win);
                keyPress = 2000000000 + event.button.x * g_Surface->w / w_win * 1000 + event.button.y * g_Surface->h / h_win;
                break;
            case SDL_QUIT:
                if (MessageBox(NULL, "��ȷ��Ҫ�ر���Ϸ��?", "ϵͳ��ʾ", MB_ICONQUESTION | MB_OKCANCEL) == IDOK)
                {
                    ExitGame();       //�ͷ���Ϸ����
                    ExitSDL();        //�˳�SDL
                    exit(1);
                }
            default:
                break;
        }
    }
    return keyPress;
}


//���òü�
int JY_SetClip(int x1, int y1, int x2, int y2)
{
    if (x1 == 0 && y1 == 0 && x2 == 0 && y2 == 0)
    {
        SDL_SetClipRect(g_Surface, NULL);
        currentRect = 0;
    }
    else
    {
        SDL_Rect rect;
        rect.x = (Sint16)x1;
        rect.y = (Sint16)y1;
        rect.w = (Uint16)(x2 - x1);
        rect.h = (Uint16)(y2 - y1);
        if (g_Rotate == 0)
        {
            ClipRect[currentRect] = rect;
        }
        else if (g_Rotate == 1)
        {
            ClipRect[currentRect] = RotateRect(&rect);
        }
        SDL_SetClipRect(g_Surface, &ClipRect[currentRect]);
        currentRect = currentRect + 1;
        if (currentRect >= RECTNUM)
        {
            currentRect = 0;
        }
    }
    return 0;
}


// ���ƾ��ο�
// (x1,y1)--(x2,y2) ������ϽǺ����½�����
// color ��ɫ
int JY_DrawRect(int x1, int y1, int x2, int y2, int color)
{
    Uint8* p ;
    int lpitch = 0;
    Uint32 c;
    SDL_Rect rect1, rect2;
    int xmin, xmax, ymin, ymax;
    if (x1 < x2)
    {
        xmin = x1;
        xmax = x2;
    }
    else
    {
        xmin = x2;
        xmax = x1;
    }
    if (y1 < y2)
    {
        ymin = y1;
        ymax = y2;
    }
    else
    {
        ymin = y2;
        ymax = y1;
    }
    rect1.x = (Sint16)xmin;
    rect1.y = (Sint16)ymin;
    rect1.w = (Uint16)(xmax - xmin + 1);
    rect1.h = (Uint16)(ymax - ymin + 1);
    SDL_LockSurface(g_Surface);
    p = g_Surface->pixels;
    lpitch = g_Surface->pitch;
    c = ConvertColor(color);
    if (g_Rotate == 0)
    {
        rect2 = rect1;
    }
    else
    {
        rect2 = RotateRect(&rect1);
    }
    x1 = rect2.x;
    y1 = rect2.y;
    x2 = rect2.x + rect2.w - 1;
    y2 = rect2.y + rect2.h - 1;
    HLine32(x1, x2, y1, c, p, lpitch);
    HLine32(x1, x2, y2, c, p, lpitch);
    VLine32(y1, y2, x1, c, p, lpitch);
    VLine32(y1, y2, x2, c, p, lpitch);
    SDL_UnlockSurface(g_Surface);
    return 0;
}


//��ˮƽ��
void HLine32(int x1, int x2, int y, int color, unsigned char* vbuffer, int lpitch)
{
    int temp;
    int i;
    int max_x, max_y, min_x, min_y;
    Uint8* vbuffer2;
    int bpp;
    bpp = g_Surface->format->BytesPerPixel;
    //�ֹ�����
    min_x = g_Surface->clip_rect.x;
    min_y = g_Surface->clip_rect.y;
    max_x = g_Surface->clip_rect.x + g_Surface->clip_rect.w - 1;
    max_y = g_Surface->clip_rect.y + g_Surface->clip_rect.h - 1;
    if (y > max_y || y < min_y)
    {
        return;
    }
    if (x1 > x2)
    {
        temp = x1;
        x1   = x2;
        x2   = temp;
    }
    if (x1 > max_x || x2 < min_x)
    {
        return;
    }
    x1 = ((x1 < min_x) ? min_x : x1);
    x2 = ((x2 > max_x) ? max_x : x2);
    vbuffer2 = vbuffer + y * lpitch + x1 * bpp;
    switch (bpp)
    {
        case 2:        //16λɫ��
            for (i = 0; i <= x2 - x1; i++)
            {
                *(Uint16*)vbuffer2 = (Uint16)color;
                vbuffer2 += 2;
            }
            break;
        case 3:        //24λɫ��
            for (i = 0; i <= x2 - x1; i++)
            {
                Uint8* p = (Uint8*)(&color);
                *vbuffer2 = *p;
                *(vbuffer2 + 1) = *(p + 1);
                *(vbuffer2 + 2) = *(p + 2);
                vbuffer2 += 3;
            }
            break;
        case 4:        //32λɫ��
            for (i = 0; i <= x2 - x1; i++)
            {
                *(Uint32*)vbuffer2 = (Uint32)color;
                vbuffer2 += 4;
            }
            break;
    }
}

//�洹ֱ��
void VLine32(int y1, int y2, int x, int color, unsigned char* vbuffer, int lpitch)
{
    int temp;
    int i;
    int max_x, max_y, min_x, min_y;
    Uint8* vbuffer2;
    int bpp;
    bpp = g_Surface->format->BytesPerPixel;
    min_x = g_Surface->clip_rect.x;
    min_y = g_Surface->clip_rect.y;
    max_x = g_Surface->clip_rect.x + g_Surface->clip_rect.w - 1;
    max_y = g_Surface->clip_rect.y + g_Surface->clip_rect.h - 1;
    if (x > max_x || x < min_x)
    {
        return;
    }
    if (y1 > y2)
    {
        temp = y1;
        y1   = y2;
        y2   = temp;
    }
    if (y1 > max_y || y2 < min_y)
    {
        return;
    }
    y1 = ((y1 < min_y) ? min_y : y1);
    y2 = ((y2 > max_y) ? max_y : y2);
    vbuffer2 = vbuffer + y1 * lpitch + x * bpp;
    switch (bpp)
    {
        case 2:
            for (i = 0; i <= y2 - y1; i++)
            {
                *(Uint16*)vbuffer2 = (Uint16)color;
                vbuffer2 += lpitch;
            }
            break;
        case 3:
            for (i = 0; i <= y2 - y1; i++)
            {
                Uint8* p = (Uint8*)(&color);
                *vbuffer2 = *p;
                *(vbuffer2 + 1) = *(p + 1);
                *(vbuffer2 + 2) = *(p + 2);
                vbuffer2 += lpitch;
            }
            break;
        case 4:
            for (i = 0; i <= y2 - y1; i++)
            {
                *(Uint32*)vbuffer2 = (Uint32)color;
                vbuffer2 += lpitch;
            }
            break;
    }
}



// ͼ�����
// ���x1,y1,x2,y2��Ϊ0���������������
// color, ���ɫ����RGB��ʾ���Ӹߵ����ֽ�Ϊ0RGB
int JY_FillColor(int x1, int y1, int x2, int y2, int color)
{
    int c = ConvertColor(color);
    SDL_Rect rect;
    if (x1 == 0 && y1 == 0 && x2 == 0 && y2 == 0)
    {
        SDL_FillRect(g_Surface, NULL, c);
    }
    else
    {
        rect.x = (Sint16)x1;
        rect.y = (Sint16)y1;
        rect.w = (Uint16)(x2 - x1);
        rect.h = (Uint16)(y2 - y1);
        SDL_FillRect(g_Surface, &rect, c);
    }
    return 0;
}


// �ѱ���blit����������ǰ������
// x,y Ҫ���ص���������Ͻ�����

//  flag ��ͬbit����ͬ���壬ȱʡ��Ϊ0
//  B0    0 ����ƫ��xoff��yoff��=1 ������ƫ����
//  B1    0     , 1 �뱳��alpla �����ʾ, value Ϊalphaֵ(0-256), 0��ʾ͸��
//  B2            1 ȫ��
//  B3            1 ȫ��

//B4Ϊ����zoom���������ҷ�ת������

//  value ����flag���壬Ϊalphaֵ��

int BlitSurface(SDL_Surface* lps, int x, int y , int flag, int value, int pcolor)
{
    SDL_Surface* tmps, *tmps2;
    SDL_Rect rect;
    int i, j;

    int tempsur = 0;

    if (value > 255)
    {
        value = 255;
    }
    rect.x = (Sint16)x;
    rect.y = (Sint16)y;
    if (!lps)
    {
        JY_Error("BlitSurface: lps is null!");
        return 1;
    }
    rect.w = lps->w;
    rect.h = lps->h;
    tmps = lps;
    if (flag & 0x2)      // alpla
    {
        SDL_SetSurfaceAlphaMod(tmps, (Uint8)value);
    }
    else
    {
        SDL_SetSurfaceAlphaMod(tmps, 255);
    }
    if (flag & 0x4) // ��
    {
        SDL_SetSurfaceColorMod(tmps, 0, 0, 0);
    }
    if (flag & 0x8) // ��
    {
        //������ʱ����
        tmps = SDL_DisplayFormat(lps);
		tmps2 = SDL_CreateRGBSurface(0, tmps->w, tmps->h, tmps->format->BitsPerPixel,
			tmps->format->Rmask, tmps->format->Gmask, tmps->format->Bmask, tmps->format->Amask);
		SDL_SetSurfaceBlendMode(tmps2, SDL_BLENDMODE_ADD);
		SDL_FillRect(tmps2, NULL, 0xffffffff);
		SDL_BlitSurface(tmps2, NULL, tmps, NULL);
        tempsur = 1;
    }
    if (flag & 0x10)      //�ƺ������ҷ�ת����Ū�ð�
    {
        tmps = zoomSurface(lps, -1, 1, 0);
        tempsur = 1;
    }

    SDL_BlitSurface(tmps, NULL, g_Surface, &rect);
    if (tempsur)
    {
        SDL_FreeSurface(tmps);
    }

    return 0;
}


// �����䰵
// ��Դ����(x1,y1,x2,y2)�����ڵ����е����Ƚ���
// bright ���ȵȼ� 0-256
int JY_Background(int x1, int y1, int x2, int y2, int Bright)
{
    SDL_Surface* lps1;
    SDL_Rect r1, r2;
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
    if (g_Rotate == 0)
    {
        r2 = r1;
    }
    else
    {
        r2 = RotateRect(&r1);
    }
    lps1 =
        SDL_CreateRGBSurface(0, r2.w, r2.h, g_Surface->format->BitsPerPixel,
                             g_Surface->format->Rmask, g_Surface->format->Gmask, g_Surface->format->Bmask, g_Surface->format->Amask);
    SDL_FillRect(lps1, NULL, 0xff000000);
    SDL_SetSurfaceAlphaMod(lps1, (Uint8)Bright);
    SDL_BlitSurface(lps1, NULL, g_Surface, &r2);
    SDL_FreeSurface(lps1);
    return 1;
}

//����mpeg
// esckey ֹͣ���ŵİ���
int JY_PlayMPEG(const char* filename, int esckey)
{
    /*
    SMPEG_Info mpg_info;
     SMPEG* mpg = NULL;
     char *err;

     mpg=SMPEG_new(filename,&mpg_info,0);

     err=SMPEG_error(mpg);
     if(err!=NULL){
    	 JY_Error("Open file %s error: %s\n",filename,err);
    	 return 1;
     }


     SMPEG_setdisplay(mpg,g_Surface,NULL,NULL);

     //Play the movie, using SDL_mixer for audio
     SMPEG_enableaudio(mpg, 1);

     SMPEG_play(mpg);

     while( (SMPEG_status(mpg) == SMPEG_PLAYING)){
         int key=JY_GetKey();
    	 if(key==esckey){
             break;
    	 }
         SDL_Delay(500);
     }

    SMPEG_stop(mpg);
    SMPEG_delete(mpg);
    //Mix_HookMusic(NULL, NULL);
    */
    return 0;
}


// ȫ���л�
int JY_FullScreen()
{
    /*
    SDL_Surface *tmpsurface;
    //const SDL_VideoInfo *info;


    Uint32 flag=g_Surface->flags;

    tmpsurface=SDL_CreateRGBSurface(SDL_SWSURFACE,g_Surface->w,g_Surface->h,g_Surface->format->BitsPerPixel,
    	                      g_Surface->format->Rmask,g_Surface->format->Gmask,g_Surface->format->Bmask,0);

    SDL_BlitSurface(g_Surface,NULL,tmpsurface,NULL);

    if(flag)    //ȫ�������ô���
        g_Surface=SDL_SetVideoMode(g_Surface->w,g_Surface->h, 0, SDL_SWSURFACE);
    else
        g_Surface=SDL_SetVideoMode(g_Surface->w, g_Surface->h, g_ScreenBpp, SDL_HWSURFACE|SDL_DOUBLEBUF|SDL_FULLSCREEN);


    SDL_BlitSurface(tmpsurface,NULL,g_Surface,NULL);

    JY_ShowSurface(0);

    SDL_FreeSurface(tmpsurface);

    info=SDL_GetVideoInfo();
    JY_Debug("hw_available=%d,wm_available=%d",info->hw_available,info->wm_available);
    JY_Debug("blit_hw=%d,blit_hw_CC=%d,blit_hw_A=%d",info->blit_hw,info->blit_hw_CC ,info->blit_hw_A );
    JY_Debug("blit_sw=%d,blit_sw_CC=%d,blit_sw_A=%d",info->blit_hw,info->blit_hw_CC ,info->blit_hw_A );
    JY_Debug("blit_fill=%d,videomem=%d",info->blit_fill,info->video_mem  );
    JY_Debug("Color depth=%d",info->vfmt->BitsPerPixel);
    */
    return 0;
}

//������ת90��
SDL_Surface* RotateSurface(SDL_Surface* src)
{
    SDL_Surface* dest;
    /*
    int i,j;


    dest=SDL_CreateRGBSurface(SDL_SWSURFACE,src->h,src->w,src->format->BitsPerPixel,
    	                      src->format->Rmask,src->format->Gmask,src->format->Bmask,src->format->Amask);

    if(src->format->BitsPerPixel==8){
    	SDL_SetPalette(dest,SDL_LOGPAL,src->format->palette->colors,0,src->format->palette->ncolors);
    }

    SDL_LockSurface(src);
    SDL_LockSurface(dest);

    if(src->format->BitsPerPixel==32){
    	for(j=0;j<src->h;j++){
    		Uint32 *psrc=(Uint32*)((Uint8*)src->pixels+j*src->pitch);
    		Uint8 *pdest=(Uint8*)dest->pixels+(src->h-j-1)*4;
    		for(i=0;i<src->w;i++){
    			*(Uint32*)pdest=*psrc;
    			psrc++;
    			pdest+=dest->pitch;
    		}
    	}
    }
    else if(src->format->BitsPerPixel==16){
    	for(j=0;j<src->h;j++){
    		Uint16 *psrc=(Uint16*)((Uint8*)src->pixels+j*src->pitch);
    		Uint8 *pdest=(Uint8*)dest->pixels+(src->h-j-1)*2;
    		for(i=0;i<src->w;i++){
    			*(Uint16*)pdest=*psrc;
    			psrc++;
    			pdest+=dest->pitch;
    		}
    	}
    }

    else if(src->format->BitsPerPixel==24){
    	for(j=0;j<src->h;j++){
    		Uint8 *psrc=((Uint8*)src->pixels+j*src->pitch);
    		Uint8 *pdest=(Uint8*)dest->pixels+(src->h-j-1)*3;
    		for(i=0;i<src->w;i++){
    			*pdest=*psrc;
    			*(pdest+1)=*(psrc+1);
    			*(pdest+2)=*(psrc+2);
    			psrc+=3;
    			pdest+=dest->pitch;
    		}
    	}
    }
    else if(src->format->BitsPerPixel==8){
    	for(j=0;j<src->h;j++){
    		Uint8 *psrc=((Uint8*)src->pixels+j*src->pitch);
    		Uint8 *pdest=(Uint8*)dest->pixels+(src->h-j-1);
    		for(i=0;i<src->w;i++){
    			*pdest=*psrc;
    			psrc++;
    			pdest+=dest->pitch;
    		}
    	}
    }

    SDL_UnlockSurface(src);
    SDL_UnlockSurface(dest);

    //SDL_SetColorKey(dest,SDL_SRCCOLORKEY|SDL_RLEACCEL ,src->format->colorkey);

    */
    return dest;
}

//������ת90��
SDL_Rect RotateRect(const SDL_Rect* rect)
{
    SDL_Rect r;
    r.x = (Sint16)(g_ScreenH - rect->y - rect->h);
    r.y = rect->x;
    r.w = rect->h;
    r.h = rect->w;
    return r;
}

//������ת90��
SDL_Rect RotateReverseRect(const SDL_Rect* rect)
{
    SDL_Rect r;
    r.x = rect->y;
    r.y = (Sint16)(g_ScreenH - rect->x - rect->w);
    r.w = rect->h;
    r.h = rect->w;
    return r;
}

//������Ļ����ʱ����
int JY_SaveSur(int x, int y, int w, int h)
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
    if (id == -1)
    {
        return -1;
    }
    r1.x = (Sint16)x;
    r1.y = (Sint16)y;
    r1.w = (Uint16)w;
    r1.h = (Uint16)h;
    tmp_Surface[id] = SDL_CreateRGBSurface(SDL_SWSURFACE, r1.w, r1.h, g_Surface->format->BitsPerPixel
                                           , g_Surface->format->Rmask, g_Surface->format->Gmask, g_Surface->format->Bmask, 0);
    SDL_BlitSurface(g_Surface, &r1, tmp_Surface[id], NULL);
    return id;
}
int JY_SaveSur2(int x, int y, int w, int h)
{
    int id = -1;
    int i;
    SDL_Rect r1;
    for (i = 0; i < SURFACE_NUM; i++)
    {
        if (tmp_Surface2[i] == NULL)
        {
            id = i;
            break;
        }
    }

    if (id == -1)
    {
        return -1;
    }


    r1.x = (Sint16)x;
    r1.y = (Sint16)y;
    r1.w = (Uint16)w;
    r1.h = (Uint16)h;


    tmp_Surface2[id] = SDL_CreateRGBSurface(SDL_SWSURFACE, r1.w, r1.h, g_Surface->format->BitsPerPixel
                                            , g_Surface->format->Rmask, g_Surface->format->Gmask, g_Surface->format->Bmask, 0);
    SDL_BlitSurface(g_Surface, &r1, tmp_Surface2[id], NULL);

    return id;
}

//������ʱ���浽��Ļ
int JY_LoadSur(int id, int x, int y)
{
    SDL_Rect r1;
    if (id < 0 || id > SURFACE_NUM - 1 || tmp_Surface[id] == NULL)
    {
        return 1;
    }
    r1.x = (Sint16)x;
    r1.y = (Sint16)y;
    SDL_BlitSurface(tmp_Surface[id], NULL, g_Surface, &r1);
    return 0;
}

//������ʱ���浽��Ļ
int JY_LoadSur2(int id, int x, int y)
{
    SDL_Rect r1;
    if (id < 0 || id > SURFACE_NUM - 1 || tmp_Surface2[id] == NULL)
    {
        return 1;
    }

    r1.x = (Sint16)x;
    r1.y = (Sint16)y;
    SDL_BlitSurface(tmp_Surface2[id], NULL, g_Surface, &r1);
    return 0;
}

//�ͷ�
int JY_FreeSur(int id)
{
    if (id < 0 || id > SURFACE_NUM - 1 || tmp_Surface[id] == NULL)
    {
        return 1;
    }
    if (tmp_Surface[id] != NULL)
    {
        SDL_FreeSurface(tmp_Surface[id]);
        tmp_Surface[id] = NULL;
    }
    return 0;
}

//�ͷ�
int JY_FreeSur2(int id)
{
    if (id < 0 || id > SURFACE_NUM - 1 || tmp_Surface2[id] == NULL)
    {
        return 1;
    }

    if (tmp_Surface2[id] != NULL)
    {
        SDL_FreeSurface(tmp_Surface2[id]);
        tmp_Surface2[id] = NULL;
    }
    return 0;
}




