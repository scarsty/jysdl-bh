
// 主程序
// 本程序为游泳的鱼编写。
// 版权所无，您可以以任何方式使用代码

#include "jymain.h"
#include "charset.h"
#include "mainmap.h"
#include "sdlfun.h"
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>

#ifdef _WIN32
extern "C" __declspec(dllimport) int __stdcall SetConsoleOutputCP(unsigned int);
#endif

// 全局变量
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
int g_SwitchABXY = 0;      // 调换AB键，0为不调换，1为调换

int g_XScale = 18;    //贴图x,y方向一半大小
int g_YScale = 9;

//各个地图绘制时xy方向需要多绘制的余量。保证可以全部显示
int g_MMapAddX;
int g_MMapAddY;
int g_SMapAddX;
int g_SMapAddY;
int g_WMapAddX;
int g_WMapAddY;
int g_MAXCacheNum = 1000;    //最大缓存数量
int g_LoadFullS = 1;         //是否全部加载S文件
int g_LoadMMapType = 0;      //是否全部加载M文件
int g_LoadMMapScope = 0;
int g_IsDebug = 0;           //是否打开调试日志
char g_MainLuaFile[255];     //lua主脚本文件
int g_MP3 = 0;               //是否使用MP3格式音乐
char g_MidSF2[255];          //MIDI音色库文件路径
float g_Zoom = 1;            //图片缩放比例
char g_Softener[255];        //柔化滤镜名称
int g_DelayTimes;            //延迟帧数

#ifdef _WIN32
const char* JY_CurrentPath = "./";
#else
const char* JY_CurrentPath = "/sdcard/JYLDCR/";
#endif

lua_State* pL_main = NULL;

void* g_Tinypot;
ParticleExample g_Particle;

static FILE* g_LogDebugFile = NULL;
static FILE* g_LogErrorFile = NULL;

static const char* JY_LogPriorityName(SDL_LogPriority priority)
{
    switch (priority)
    {
    case SDL_LOG_PRIORITY_VERBOSE: return "VERBOSE";
    case SDL_LOG_PRIORITY_DEBUG:   return "DEBUG";
    case SDL_LOG_PRIORITY_INFO:    return "INFO";
    case SDL_LOG_PRIORITY_WARN:    return "WARN";
    case SDL_LOG_PRIORITY_ERROR:   return "ERROR";
    case SDL_LOG_PRIORITY_CRITICAL:return "CRITICAL";
    default:                       return "UNKNOWN";
    }
}

static void JY_FormatNow(char* out, size_t size)
{
    if (!out || size == 0) return;
    time_t now = time(NULL);
    struct tm tm_now;
#ifdef _WIN32
    localtime_s(&tm_now, &now);
#else
    localtime_r(&now, &tm_now);
#endif
    Uint64 ticks = SDL_GetTicks();
    snprintf(out, size, "%04d-%02d-%02d %02d:%02d:%02d.%03llu",
        tm_now.tm_year + 1900, tm_now.tm_mon + 1, tm_now.tm_mday,
        tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec,
        (unsigned long long)(ticks % 1000));
}

static int JY_WriteLogLine(FILE* fp, const char* msg)
{
    if (!fp || !msg) return -1;
    if (fprintf(fp, "%s\n", msg) < 0) return -1;
    fflush(fp);
    return 0;
}

static int JY_LogMessage(SDL_LogPriority priority, const char* file, int line,
    const char* func, const char* fmt, va_list args)
{
    if (!fmt || !file || !func) return -1;

    char user_msg[2048];
    int n = vsnprintf(user_msg, sizeof(user_msg), fmt, args);
    if (n < 0) return -1;

    char time_text[64] = {0};
    JY_FormatNow(time_text, sizeof(time_text));

    char full_msg[3072];
    snprintf(full_msg, sizeof(full_msg), "[%s][%s][%s:%d][FUNC:%s] %s",
        time_text, JY_LogPriorityName(priority), file, line, func, user_msg);

    SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, priority, "%s", full_msg);

    if (priority >= SDL_LOG_PRIORITY_ERROR)
        return JY_WriteLogLine(g_LogErrorFile, full_msg);
    return JY_WriteLogLine(g_LogDebugFile, full_msg);
}

static void JY_CloseLogFiles()
{
    if (g_LogDebugFile) { fclose(g_LogDebugFile); g_LogDebugFile = NULL; }
    if (g_LogErrorFile) { fclose(g_LogErrorFile); g_LogErrorFile = NULL; }
}

static int JY_OpenLogFiles()
{
    g_LogDebugFile = fopen(DEBUG_FILE, "w");
    g_LogErrorFile = fopen(ERROR_FILE, "w");
    if (!g_LogDebugFile || !g_LogErrorFile) { JY_CloseLogFiles(); return -1; }
    return 0;
}

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
    { "GetM", HAPI_GetM },
    { "SetM", HAPI_SetM },
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
#ifdef _WIN32
    SetConsoleOutputCP(65001);
#endif
    //lua_State* pL_main;
    srand(time(0));

    // 初始化 SDL 日志优先级和文件输出
    SDL_SetLogPriorities(SDL_LOG_PRIORITY_DEBUG);
    if (JY_OpenLogFiles() != 0)
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "cannot open %s or %s", DEBUG_FILE, ERROR_FILE);

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
    JY_CloseLogFiles();

    return 0;
}

//Lua主函数
int Lua_Main(lua_State* pL_main)
{
    int result = 0;
    do {
        result = luaL_loadfile(pL_main, g_MainLuaFile);
        if (result)
        {
            break;
        }
        result = lua_pcall(pL_main, 0, 0, 0);
        if (result)
        {
            break;
        }
        //调用lua的主函数JY_Main
        lua_getglobal(pL_main, "JY_Main");
        result = lua_pcall(pL_main, 0, 1, 0);
    } while (0);
    if (result)
    {
        JY_Error(lua_tostring(pL_main, -1));
        lua_pop(pL_main, 1);
        return 1;
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
    g_IsDebug = getfield(pL, "Debug");
    //g_Pic = getfield(pL, "Pic");
    g_MMapAddX = getfield(pL, "MMapAddX");
    g_MMapAddY = getfield(pL, "MMapAddY");
    g_SMapAddX = getfield(pL, "SMapAddX");
    g_SMapAddY = getfield(pL, "SMapAddY");
    g_WMapAddX = getfield(pL, "WMapAddX");
    g_WMapAddY = getfield(pL, "WMapAddY");
    g_SoundVolume = getfield(pL, "SoundVolume");
    g_MusicVolume = getfield(pL, "MusicVolume");
    g_SwitchABXY = getfield(pL, "SwitchABXY");

    g_MAXCacheNum = getfield(pL, "MAXCacheNum");
    g_LoadFullS = getfield(pL, "LoadFullS");
    g_MP3 = getfield(pL, "MP3");
    g_Zoom = (float)(getfield(pL, "Zoom") / 100.0);
    getfieldstr(pL, "MidSF2", g_MidSF2);
    getfieldstr(pL, "JYMain_Lua", g_MainLuaFile);
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

// ============ 通用工具函数 ============

// 输出调试信息到 debug.log
int JY_Debug_Impl(const char* file, int line, const char* func, const char* fmt, ...)
{
    va_list argptr;
    va_start(argptr, fmt);
    int ret = JY_LogMessage(SDL_LOG_PRIORITY_DEBUG, file, line, func, fmt, argptr);
    va_end(argptr);
    return ret;
}

// 输出错误信息到 error.log
int JY_Error_Impl(const char* file, int line, const char* func, const char* fmt, ...)
{
    va_list argptr;
    va_start(argptr, fmt);
    int ret = JY_LogMessage(SDL_LOG_PRIORITY_ERROR, file, line, func, fmt, argptr);
    va_end(argptr);
    return ret;
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

int FileExists(const char* name)
{
    // 检查文件是否存在
    if (strlen(name) == 0)
    {
        return 0;    // 空字符串不代表文件
    }

    // 使用stat函数检查文件状态
    struct stat buffer;
    return (stat(name, &buffer) == 0);
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
