
#include <ctime>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <lua.hpp>

export module jy:main1;
import std;
import :util;
import :sdlfun;
import :piccache;
import :luafun;



// 初始化游戏数据
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
    g_Window = SDL_CreateWindow((const char*)u8"金书群侠传", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, SDL_WINDOW_RESIZABLE);
    SDL_SetWindowIcon(g_Window, IMG_Load("ff.ico"));
    g_Renderer = SDL_CreateRenderer(g_Window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
    g_Texture = CreateRenderedTexture(g_ScreenW, g_ScreenH);
    g_TextureShow = CreateRenderedTexture(g_ScreenW, g_ScreenH);
    g_TextureTmp = CreateRenderedTexture(g_ScreenW, g_ScreenH);

    g_Surface = SDL_CreateRGBSurface(0, 1, 1, 32, RMASK, GMASK, BMASK, AMASK);
    //SDL_WM_SetCaption("The Fall of Star",_("ff.ico"));         //这是显示窗口的
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
    JY_PicInit("");    // 初始化贴图cache
    g_Particle.setRenderer(g_Renderer);
    g_Particle.setPosition(w / 2, 0);
    g_Particle.getDefaultTexture();
    return 0;
}

// 释放游戏资源
export int ExitGame(void)
{
    SDL_DestroyTexture(g_Texture);
    SDL_DestroyTexture(g_TextureShow);
    SDL_DestroyRenderer(g_Renderer);
    SDL_DestroyWindow(g_Window);
    JY_PicInit("");
    JY_LoadPicture("", 0, 0);
    JY_UnloadMMap();      //释放主地图内存
    JY_UnloadSMap();      //释放场景地图内存
    JY_UnloadWarMap();    //释放战斗地图内存
    return 0;
}

//定义的lua接口函数名
export const struct luaL_Reg jylib[] = {
    { "Debug", HAPI_Debug },

    { "GetKey", HAPI_GetKey },
    { "GetKeyState", HAPI_GetKeyState },
    { "EnableKeyRepeat", HAPI_EnableKeyRepeat },

    { "Delay", HAPI_Delay },
    { "GetTime", HAPI_GetTime },

    { "CharSet", HAPI_CharSet },
    { "DrawStr", HAPI_DrawStr },

    { "SetClip", HAPI_SetClip },
    { "FillColor", HAPI_FillColor },
    { "Background", HAPI_Background },
    { "DrawRect", HAPI_DrawRect },

    { "ShowSurface", HAPI_ShowSurface },
    { "ShowSlow", HAPI_ShowSlow },

    { "PicInit", HAPI_PicInit },
    { "PicGetXY", HAPI_GetPicXY },
    { "PicLoadCache", HAPI_LoadPic },
    { "PicLoadFile", HAPI_PicLoadFile },

    { "FullScreen", HAPI_FullScreen },

    { "LoadPicture", HAPI_LoadPicture },

    { "PlayMIDI", HAPI_PlayMIDI },
    { "PlayWAV", HAPI_PlayWAV },
    { "PlayMPEG", HAPI_PlayMPEG },

    { "LoadMMap", HAPI_LoadMMap },
    { "DrawMMap", HAPI_DrawMMap },
    { "GetMMap", HAPI_GetMMap },
    { "UnloadMMap", HAPI_UnloadMMap },

    { "LoadSMap", HAPI_LoadSMap },
    { "SaveSMap", HAPI_SaveSMap },
    { "GetS", HAPI_GetS },
    { "SetS", HAPI_SetS },
    { "GetD", HAPI_GetD },
    { "SetD", HAPI_SetD },
    { "SetSound", HAPI_SetSound },
    { "DrawSMap", HAPI_DrawSMap },

    { "LoadWarMap", HAPI_LoadWarMap },
    { "GetWarMap", HAPI_GetWarMap },
    { "SetWarMap", HAPI_SetWarMap },
    { "CleanWarMap", HAPI_CleanWarMap },

    { "DrawWarMap", HAPI_DrawWarMap },
    { "SaveSur", HAPI_SaveSur },
    { "LoadSur", HAPI_LoadSur },
    { "FreeSur", HAPI_FreeSur },
    { "GetScreenW", HAPI_ScreenWidth },
    { "GetScreenH", HAPI_ScreenHeight },
    { "LoadPNGPath", HAPI_LoadPNGPath },
    { "LoadPNG", HAPI_LoadPNG },
    { "GetPNGXY", HAPI_GetPNGXY },
    { "SetWeather", HAPI_SetWeather },
    { NULL, NULL }
};

export const struct luaL_Reg bytelib[] = {
    { "create", Byte_create },
    { "loadfile", Byte_loadfile },
    { "loadfilefromzip", Byte_loadfilefromzip },
    { "savefile", Byte_savefile },
    { "unzip", Byte_unzip },
    { "zip", Byte_zip },
    { "get16", Byte_get16 },
    { "set16", Byte_set16 },
    { "getu16", Byte_getu16 },
    { "setu16", Byte_setu16 },
    { "get32", Byte_get32 },
    { "set32", Byte_set32 },
    { "getstr", Byte_getstr },
    { "setstr", Byte_setstr },
    { NULL, NULL }
};

static const struct luaL_Reg configLib[] = {

    { "GetPath", Config_GetPath },
    { NULL, NULL }
};


//Lua主函数
export int Lua_Main(lua_State* pL_main)
{
    int result = 0;

    //初始化lua

    //加载lua文件
    result = luaL_loadfile(pL_main, JYMain_Lua);
    switch (result)
    {
    case LUA_ERRSYNTAX:
        JY_Error("load lua file %s error: syntax error!\n", JYMain_Lua);
        break;
    case LUA_ERRMEM:
        JY_Error("load lua file %s error: memory allocation error!\n", JYMain_Lua);
        break;
    case LUA_ERRFILE:
        JY_Error("load lua file %s error: can not open file!\n", JYMain_Lua);
        break;
    }

    result = lua_pcall(pL_main, 0, LUA_MULTRET, 0);

    //调用lua的主函数JY_Main
    lua_getglobal(pL_main, "JY_Main");
    result = lua_pcall(pL_main, 0, 0, 0);

    if (result)
    {
        JY_Error(lua_tostring(pL_main, -1));
        lua_pop(pL_main, 1);
    }

    return 0;
}

//Lua读取配置信息
export int Lua_Config(lua_State* pL, const char* filename)
{
    int result = 0;

    //加载lua配置文件
    result = luaL_loadfile(pL, filename);
    switch (result)
    {
    case LUA_ERRSYNTAX:
        fprintf(stderr, "load lua file %s error: syntax error!\n", filename);
        break;
    case LUA_ERRMEM:
        fprintf(stderr, "load lua file %s error: memory allocation error!\n", filename);
        break;
    case LUA_ERRFILE:
        fprintf(stderr, "load lua file %s error: can not open file!\n", filename);
        break;
    }

    result = lua_pcall(pL, 0, LUA_MULTRET, 0);

    if (result)
    {
        JY_Error(lua_tostring(pL, -1));
        lua_pop(pL, 1);
    }

    lua_getglobal(pL, "CONFIG");    //读取config定义的值
    if (getfield(pL, "Width") != 0)
    {
        g_ScreenW = getfield(pL, "Width");
    }
    if (getfield(pL, "Height") != 0)
    {
        g_ScreenH = getfield(pL, "Height");
    }
    g_ScreenBpp = getfield(pL, "bpp");
    g_FullScreen = getfield(pL, "FullScreen");
    g_XScale = getfield(pL, "XScale");
    g_YScale = getfield(pL, "YScale");
    g_XScale = 18;
    g_YScale = 9;
    g_EnableSound = getfield(pL, "EnableSound");
    IsDebug = getfield(pL, "Debug");
    //g_Pic = getfield(pL, "Pic");
    g_MMapAddX = getfield(pL, "MMapAddX");
    g_MMapAddY = getfield(pL, "MMapAddY");
    g_SMapAddX = getfield(pL, "SMapAddX");
    g_SMapAddY = getfield(pL, "SMapAddY");
    g_WMapAddX = getfield(pL, "WMapAddX");
    g_WMapAddY = getfield(pL, "WMapAddY");
    g_SoundVolume = getfield(pL, "SoundVolume");
    g_MusicVolume = getfield(pL, "MusicVolume");

    g_MAXCacheNum = getfield(pL, "MAXCacheNum");
    g_LoadFullS = getfield(pL, "LoadFullS");
    g_MP3 = getfield(pL, "MP3");
    g_Zoom = (float)(getfield(pL, "Zoom") / 100.0);
    getfieldstr(pL, "MidSF2", g_MidSF2);
    getfieldstr(pL, "JYMain_Lua", JYMain_Lua);
    getfieldstr(pL, "Softener", g_Softener);
    return 0;
}




