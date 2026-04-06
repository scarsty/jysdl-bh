
// 主程序
// 本程序为游泳的鱼编写。
// 版权所无，您可以以任何方式使用代码

#include "jymain.h"
#include "charset.h"
#include "mainmap.h"
#include "sdlfun.h"
#include <stdio.h>
#include <time.h>

#include "spdlog/spdlog.h"
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "GameData.h"
#include "GameMain.h"

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
int g_BJ = 0;
int g_MAXCacheNum = 1000;    //最大缓存数量
int g_LoadFullS = 1;         //是否全部加载S文件
int g_LoadMMapType = 0;      //是否全部加载M文件
int g_LoadMMapScope = 0;
//int g_PreLoadPicGrp = 1;    //是否预先加载贴图文件的grp
int IsDebug = 0;         //是否打开跟踪文件
char JYMain_Lua[255];    //lua主函数
int g_MP3 = 0;           //是否打开MP3
char g_MidSF2[255];      //音色库对应的文件
float g_Zoom = 1;        //图片放大
char g_Softener[255];    //音色库对应的文件
int g_DelayTimes;

#ifdef _WIN32
const char* JY_CurrentPath = "./";
#else
const char* JY_CurrentPath = "/sdcard/JYLDCR/";
#endif

void* g_Tinypot;
ParticleExample g_Particle;

std::shared_ptr<spdlog::logger> g_logger_debug, g_logger_error;

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
    remove(DEBUG_FILE);
    remove(ERROR_FILE);    //设置stderr输出到文件

    spdlog::set_level(spdlog::level::debug);

    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(DEBUG_FILE, true);
    auto file_sink2 = std::make_shared<spdlog::sinks::basic_file_sink_mt>(ERROR_FILE, true);
    spdlog::sinks_init_list sink_list = { console_sink, file_sink };
    spdlog::sinks_init_list sink_list2 = { console_sink, file_sink2 };

    g_logger_debug = std::make_shared<spdlog::logger>("1", sink_list);
    g_logger_debug->set_level(spdlog::level::debug);    // 设置日志级别为debug
    g_logger_error = std::make_shared<spdlog::logger>("2", sink_list2);

    // 读取INI配置文件
    JY_Debug("LoadConfig();");
    g_Config.loadFromINI(_(CONFIG_FILE));

    // 将配置复制到全局变量（兼容原有代码）
    if (g_Config.Width != 0) g_ScreenW = g_Config.Width;
    if (g_Config.Height != 0) g_ScreenH = g_Config.Height;
    g_ScreenBpp = g_Config.bpp;
    g_FullScreen = g_Config.FullScreen;
    g_XScale = 18;
    g_YScale = 9;
    g_EnableSound = g_Config.EnableSound;
    IsDebug = g_Config.Debug;
    g_MMapAddX = g_Config.MMapAddX;
    g_MMapAddY = g_Config.MMapAddY;
    g_SMapAddX = g_Config.SMapAddX;
    g_SMapAddY = g_Config.SMapAddY;
    g_WMapAddX = g_Config.WMapAddX;
    g_WMapAddY = g_Config.WMapAddY;
    g_SoundVolume = g_Config.SoundVolume;
    g_MusicVolume = g_Config.MusicVolume;
    g_SwitchABXY = g_Config.SwitchABXY;
    g_MAXCacheNum = g_Config.MAXCacheNum;
    g_LoadFullS = g_Config.LoadFullS;
    g_MP3 = g_Config.MP3;
    g_Zoom = (float)(g_Config.Zoom / 100.0);
    strcpy(g_MidSF2, g_Config.MidSF2.c_str());
    strcpy(g_Softener, g_Config.Softener.c_str());

    // 初始化游戏常量
    g_CC.init(g_Config.Version, g_Config.Zoom);

    JY_Debug("InitSDL();");
    InitSDL();    //初始化SDL

    JY_Debug("InitGame();");
    InitGame();    //初始化游戏数据

    JY_Debug("JY_GameMain();");
    JY_GameMain();    //调用C++主函数，开始游戏

    JY_Debug("ExitGame();");
    ExitGame();    //释放游戏数据

    JY_Debug("ExitSDL();");
    ExitSDL();    //退出SDL

    JY_Debug("main() end;");

    return 0;
}

//以下为几个通用函数
// 调试函数
// 输出到debug.txt中

int JY_Debug(const char* fmt, ...)
{
    va_list argptr;
    char string[1024];
    va_start(argptr, fmt);
    vsnprintf(string, sizeof(string), fmt, argptr);
    va_end(argptr);
    g_logger_debug->debug("{}", string);
    return 0;
}

// 调试函数
// 输出到error.txt中
int JY_Error(const char* fmt, ...)
{
    va_list argptr;
    char string[1024];
    va_start(argptr, fmt);
    vsnprintf(string, sizeof(string), fmt, argptr);
    va_end(argptr);
    g_logger_error->error("{}", string);
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
