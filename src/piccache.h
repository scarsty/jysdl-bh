#pragma once

#include "SDL.h"
#include "list.h"
#include <stdio.h>
//PicCache.c

// 定义使用的链表
struct CacheNode                         //贴图cache链表节点
{
    SDL_Surface* s;                      //此贴图对应的表面
    int w;                               //贴图宽度
    int h;                               //贴图高度
    int xoff;                            //贴图偏移
    int yoff;
    int id;                              //贴图编号
    int fileid;                          //贴图文件编号
    struct list_head list;               //链表结构，linux.h中的list.h中定义
};


struct PicFileCache                      //贴图文件链表节点
{
    int num;                             //文件贴图个数
    int* idx;                            //idx的内容
    int filelength;                      //grp文件长度
    FILE* fp;                            //grp文件句柄
    unsigned char* grp;                  //grp的内容
    int width;                           //指定宽度
    int height;                          //指定高度
    int percent;                         //指定比例
    struct CacheNode** pcache;           //文件中所有的贴图对应的cache节点指针，为空则表示没有。
    char path[512];
    char suffix[12];                     //后缀名
};

#define PIC_FILE_NUM 100                 //缓存的贴图文件(idx/grp)个数



int Init_Cache();

int JY_PicInit(char* PalletteFilename);

int JY_PicLoadFile(const char* idxfilename, const char* grpfilename, int id, int width, int height);

int JY_LoadPic(int fileid, int picid, int x, int y, int flag, int value);
int JY_LoadPicColor(int fileid, int picid, int x, int y, int flag, int value, int color);

static int LoadPic(int fileid, int picid, struct CacheNode* cache);

int JY_LoadPNGPath(const char* path, int fileid, int num, int percent, const char* suffix);
int JY_LoadPNG(int fileid, int picid, int x, int y, int flag, int value);
int JY_GetPNGXY(int fileid, int picid, int* w, int* h, int* xoff, int* yoff);


int JY_GetPicXY(int fileid, int picid, int* w, int* h, int* xoff, int* yoff);

static SDL_Surface* CreatePicSurface32(unsigned char* data, int w, int h, int datalong);

static int LoadPallette(char* filename);
