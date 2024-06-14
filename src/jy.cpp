#include <ctime>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <lua.hpp>

import jy;



// 主程序
int main(int argc, char* argv[])
{
    //lua_State* pL_main;
    srand(_time64(0));
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
    Lua_Config(pL_main, CONFIG_FILE);    //读取lua配置文件，设置参数

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