
// 主程序
// 本程序为游泳的鱼编写。
// 版权所无，您可以以任何方式使用代码

#include <stdio.h>
#include <time.h>

#include "charset.h"
#include "jymain.h"
#include "mainmap.h"
#include "sdlfun.h"

// 全程变量
SDL_Window* g_Window = NULL;
SDL_Renderer* g_Renderer = NULL;
SDL_Texture* g_Texture = NULL;
SDL_Texture* g_TextureShow = NULL;
SDL_Texture* g_TextureTmp = NULL;

SDL_Surface* g_Surface = NULL;        // 游戏使用的视频表面
Uint32 g_MaskColor32 = 0xff706020;    // 透明色

int g_Rotate = 0;       //屏幕是否旋转
int g_ScreenW = 800;    // 屏幕宽高
int g_ScreenH = 600;
int g_ScreenBpp = 16;    // 屏幕色深
int g_FullScreen = 0;
int g_EnableSound = 1;     // 声音开关 0 关闭 1 打开
int g_MusicVolume = 32;    // 音乐声音大小
int g_SoundVolume = 32;    // 音效声音大小

int g_XScale = 18;    //贴图x,y方向一半大小
int g_YScale = 9;

//各个地图绘制时xy方向需要多绘制的余量。保证可以全部显示
int g_MMapAddX;
int g_MMapAddY;
int g_SMapAddX;
int g_SMapAddY;
int g_WMapAddX;
int g_WMapAddY;
int g_BJ = 0;
int g_MAXCacheNum = 1000;    //最大缓存数量
int g_LoadFullS = 1;         //是否全部加载S文件
int g_LoadMMapType = 0;      //是否全部加载M文件
int g_LoadMMapScope = 0;
int g_PreLoadPicGrp = 1;    //是否预先加载贴图文件的grp
int IsDebug = 0;            //是否打开跟踪文件
char JYMain_Lua[255];       //lua主函数
int g_MP3 = 0;              //是否打开MP3
char g_MidSF2[255];         //音色库对应的文件
float g_Zoom = 1;           //图片放大
char g_Softener[255];       //音色库对应的文件
int g_DelayTimes;

#ifdef _WIN32
const char* JY_CurrentPath = "./";
#else
const char* JY_CurrentPath = "/sdcard/JYLDCR/";
#endif

lua_State* pL_main = NULL;

void* g_Tinypot;
ParticleExample g_Particle;

//定义的lua接口函数名
const struct luaL_Reg jylib[] = {
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

const struct luaL_Reg bytelib[] = {
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

void GetModes(int* width, int* height)
{
    char buf[10];
    FILE* fp = fopen(_("resolution.txt"), "r");

    if (!fp)
    {
        JY_Error("GetModes: cannot open resolution.txt");
        return;
    }

    //宽
    memset(buf, 0, 10);
    fgets(buf, 10, fp);
    *width = atoi(buf);

    //高
    memset(buf, 0, 10);
    fgets(buf, 10, fp);
    *height = atoi(buf);

    JY_Debug("GetModes: width=%d, height=%d", *width, *height);

    fclose(fp);
}

// 主程序
int main(int argc, char* argv[])
{
    //lua_State* pL_main;
    srand(time(0));
    remove(DEBUG_FILE);
    remove(ERROR_FILE);    //设置stderr输出到文件

    pL_main = luaL_newstate();
    luaL_openlibs(pL_main);

    //注册lua函数
    lua_newtable(pL_main);
    luaL_setfuncs(pL_main, jylib, 0);
    lua_pushvalue(pL_main, -1);
    lua_setglobal(pL_main, "lib");

    lua_newtable(pL_main);
    luaL_setfuncs(pL_main, bytelib, 1);
    lua_pushvalue(pL_main, -1);
    lua_setglobal(pL_main, "Byte");

    JY_Debug("Lua_Config();");
    Lua_Config(pL_main, _(CONFIG_FILE));    //读取lua配置文件，设置参数

    JY_Debug("InitSDL();");
    InitSDL();    //初始化SDL

    JY_Debug("InitGame();");
    InitGame();    //初始化游戏数据

    JY_Debug("Lua_Main();");
    Lua_Main(pL_main);    //调用Lua主函数，开始游戏

    JY_Debug("ExitGame();");
    ExitGame();    //释放游戏数据

    JY_Debug("ExitSDL();");
    ExitSDL();    //退出SDL

    JY_Debug("main() end;");

    //关闭lua
    lua_close(pL_main);

    return 0;
}

//Lua主函数
int Lua_Main(lua_State* pL_main)
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
int Lua_Config(lua_State* pL, const char* filename)
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

//读取lua表中的整型
int getfield(lua_State* pL, const char* key)
{
    int result;
    lua_getfield(pL, -1, key);
    result = (int)lua_tonumber(pL, -1);
    lua_pop(pL, 1);
    return result;
}

//读取lua表中的字符串
int getfieldstr(lua_State* pL, const char* key, char* str)
{
    const char* tmp;
    lua_getfield(pL, -1, key);
    tmp = (const char*)lua_tostring(pL, -1);
    if (tmp) { strcpy(str, tmp); }
    lua_pop(pL, 1);
    return 0;
}

//以下为几个通用函数

// 调试函数
// 输出到debug.txt中
int JY_Debug(const char* fmt, ...)
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
    time(&t);
    newtime = localtime(&t);
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

// 调试函数
// 输出到error.txt中
int JY_Error(const char* fmt, ...)
{
    //无酒不欢：不再输出error信息
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
    fp = fopen(ERROR_FILE, "a+t");
    if (fp)
    {
        fprintf(fp, "%02d:%02d:%02d %s\n", newtime->tm_hour, newtime->tm_min, newtime->tm_sec, string);
        fflush(fp);
    }
#endif
    return 0;
}

// 限制x大小
int limitX(int x, int xmin, int xmax)
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

// 返回文件长度，若为0，则文件可能不存在
int FileLength(const char* filename)
{
    FILE* f;
    int ll;
    if ((f = fopen(filename, "rb")) == NULL)
    {
        return 0;    // 文件不存在，返回
    }
    fseek(f, 0, SEEK_END);
    ll = ftell(f);    //这里得到的len就是文件的长度了
    fclose(f);
    return ll;
}

char* va(const char* format, ...)
{
    static char string[256];
    va_list argptr;

    va_start(argptr, format);
    vsnprintf(string, 256, format, argptr);
    va_end(argptr);

    return string;
}
