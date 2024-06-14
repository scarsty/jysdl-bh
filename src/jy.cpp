#include <ctime>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <lua.hpp>

import jy;



// ������
int main(int argc, char* argv[])
{
    //lua_State* pL_main;
    srand(_time64(0));
    remove(DEBUG_FILE);
    remove(ERROR_FILE);    //����stderr������ļ�

    pL_main = luaL_newstate();
    luaL_openlibs(pL_main);

    //ע��lua����
    lua_newtable(pL_main);
    luaL_setfuncs(pL_main, jylib, 0);
    lua_pushvalue(pL_main, -1);
    lua_setglobal(pL_main, "lib");

    lua_newtable(pL_main);
    luaL_setfuncs(pL_main, bytelib, 1);
    lua_pushvalue(pL_main, -1);
    lua_setglobal(pL_main, "Byte");

    JY_Debug("Lua_Config();");
    Lua_Config(pL_main, CONFIG_FILE);    //��ȡlua�����ļ������ò���

    JY_Debug("InitSDL();");
    InitSDL();    //��ʼ��SDL

    JY_Debug("InitGame();");
    InitGame();    //��ʼ����Ϸ����

    JY_Debug("Lua_Main();");
    Lua_Main(pL_main);    //����Lua����������ʼ��Ϸ

    JY_Debug("ExitGame();");
    ExitGame();    //�ͷ���Ϸ����

    JY_Debug("ExitSDL();");
    ExitSDL();    //�˳�SDL

    JY_Debug("main() end;");

    //�ر�lua
    lua_close(pL_main);

    return 0;
}