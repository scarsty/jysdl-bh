#pragma once

#include "SDL.h"
#include "config.h"
#include <stdio.h>

//mainmap.c

typedef struct
{
    int x;
    int y;
    int num;
    int key;
} BuildingType;


int JY_DrawMMap(int x, int y, int Mypic);
int JY_LoadMMap(const char* earthname, const char* surfacename, const char* buildingname,
    const char* buildxname, const char* buildyname, int x_max, int y_max, int x, int y);
int JY_UnloadMMap(void);
int JY_GetMMap(int x, int y, int flag);
int JY_SetMMap(short x, short y, int flag, short v);

int BuildingSort(short x, short y, short Mypic);
// »æÖÆÖ÷µØÍ¼
int JY_DrawMMap(int x, int y, int Mypic);
int JY_LoadSMap(const char* Sfilename, const char* tmpfilename, int num, int x_max, int y_max,
    const char* Dfilename, int d_num1, int d_num2);
int JY_SaveSMap(const char* Sfilename, const char* Dfilename);
int JY_UnloadSMap();
int ReadS(int id);
int WriteS(int id);
int JY_GetS(int id, int x, int y, int level);
int JY_SetS(int id, int x, int y, int level, int v);
int JY_GetD(int Sceneid, int id, int i);
int JY_SetD(int Sceneid, int id, int i, int v);
int JY_DrawSMap(int sceneid, int x, int y, int xoff, int yoff, int Mypic);
int JY_LoadWarMap(const char* WarIDXfilename, const char* WarGRPfilename, int mapid, int num, int x_max, int y_max);
int JY_UnloadWarMap();
int JY_GetWarMap(int x, int y, int level);
int JY_SetWarMap(int x, int y, int level, int v);
int JY_CleanWarMap(int level, int v);
int JY_DrawWarMap(int flag, int x, int y, int v1, int v2, int v3, int v4, int v5, int ex, int ey);
int LoadMMap_Sub(const char* filename, Sint16** p);
int LoadMMap_Part(int read, int x, int y);
int LoadMMap_Part_Sub(FILE* fp, Sint16** p);
