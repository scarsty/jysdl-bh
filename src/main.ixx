#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

export module main;
import util;
import std;
import sdlfun;
import piccache;
// ��ʼ����Ϸ����
export int InitGame(void)
{
    int w = g_ScreenW;
    int h = g_ScreenH;
    if (g_Rotate)
    {
        std::swap(w, h);
    }
    //putenv ("SDL_VIDEO_WINDOW_POS");
    //putenv ("SDL_VIDEO_CENTERED=1");
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, g_Softener);
    g_Window = SDL_CreateWindow((const char*)u8"����Ⱥ����", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, SDL_WINDOW_RESIZABLE);
    SDL_SetWindowIcon(g_Window, IMG_Load("ff.ico"));
    g_Renderer = SDL_CreateRenderer(g_Window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
    g_Texture = CreateRenderedTexture(g_ScreenW, g_ScreenH);
    g_TextureShow = CreateRenderedTexture(g_ScreenW, g_ScreenH);
    g_TextureTmp = CreateRenderedTexture(g_ScreenW, g_ScreenH);

    g_Surface = SDL_CreateRGBSurface(0, 1, 1, 32, RMASK, GMASK, BMASK, AMASK);
    //SDL_WM_SetCaption("The Fall of Star",_("ff.ico"));         //������ʾ���ڵ�
    //SDL_WM_SetIcon(IMG_Load(_("ff.ico")), NULL);
    if (g_FullScreen == 1)
    {
        SDL_SetWindowFullscreen(g_Window, SDL_WINDOW_FULLSCREEN_DESKTOP);
    }
    else
    {
        SDL_SetWindowFullscreen(g_Window, 0);
    }
    if (g_Window == NULL || g_Renderer == NULL || g_Texture == NULL || g_TextureShow == NULL)
    {
        JY_Error("Cannot set video mode");
    }
    Init_Cache();
    JY_PicInit("");    // ��ʼ����ͼcache
    g_Particle.setRenderer(g_Renderer);
    g_Particle.setPosition(w / 2, 0);
    g_Particle.getDefaultTexture();
    return 0;
}

// �ͷ���Ϸ��Դ
export int ExitGame(void)
{
    SDL_DestroyTexture(g_Texture);
    SDL_DestroyTexture(g_TextureShow);
    SDL_DestroyRenderer(g_Renderer);
    SDL_DestroyWindow(g_Window);
    JY_PicInit("");
    JY_LoadPicture("", 0, 0);
    JY_UnloadMMap();      //�ͷ�����ͼ�ڴ�
    JY_UnloadSMap();      //�ͷų�����ͼ�ڴ�
    JY_UnloadWarMap();    //�ͷ�ս����ͼ�ڴ�
    return 0;
}