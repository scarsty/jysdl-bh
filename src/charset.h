#pragma once

#include "SDL_ttf.h"

//CharSet.c

typedef struct UseFont_Type {      // 定义当前使用的字体结构
    int size;                      //字号，单位像素
    char *name;                    //字体文件名
    TTF_Font *font;                //打开的字体
}UseFont;

#define FONTNUM 10                 //定义同时打开的字体个数

//初始化字体
int InitFont();

//释放字体结构
int ExitFont();

// 根据字体文件名和字号打开字体
// size 为按像素大小的字号
TTF_Font *GetFont(const char *filename, int size);

// 写字符串
// x,y 坐标
// str 字符串
// color 颜色
// size 字体大小，字形为宋体。 
// fontname 字体名
// charset 字符集 0 GBK 1 big5
// OScharset 无用
int JY_DrawStr(int x, int y, const char *str, int color, int size, const char *fontname,
    int charset, int OScharset);

//加载码表转换文件
int LoadMB(const char* mbfile);


// 汉字字符集转换
// flag = 0   Big5 --> GBK     
//      = 1   GBK  --> Big5    
//      = 2   Big5 --> Unicode
//      = 3   GBK  --> Unicode
// 注意要保证dest有足够的空间，一般建议取src长度的两倍+1，保证全英文字符也能转化为unicode
int  JY_CharSet(const char *src, char *dest, int flag);
