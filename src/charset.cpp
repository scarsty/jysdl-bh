
// 输出汉字和字符集转换

//为保证平台兼容性，自己生成了一个gbk简体/繁体/big5/unicode的码表文件
//通过此文件，即可进行各种格式的转换
//移除，改用iconv

#include "charset.h"
#include "OpenCCConverter.h"
#include "PotConv.h"
#include "jymain.h"

// 显示TTF 字符串
// 为快速显示，程序将保存已经打开的相应字号的字体结构。这样做可以加快程序速度
// 为简化代码，没有用链表，而是采用数组来保存打开的字体。
// 用先进先出的方法，循环关闭已经打开的字体。
// 考虑到一般打开的字体不多，比如640*480模式实际上只用了16*24*32三种字体。
// 设置数组为10已经足够。

UseFont Font[FONTNUM];    //保存已打开的字体

int currentFont = 0;

//字符集转换数组
Uint16 gbk_unicode[128][256];
Uint16 gbk_big5[128][256];
Uint16 big5_gbk[128][256];
Uint16 big5_unicode[128][256];

#define MAX_CACHE_CHAR (10000)
int char_count = 0;
std::map<std::string, std::map<int, SDL_Texture*>> chars_cache;
SDL_Texture** chars_record[MAX_CACHE_CHAR] = { 0 };

//初始化
int InitFont()
{
    int i;

    TTF_Init();    // 初始化sdl_ttf

    for (i = 0; i < FONTNUM; i++)    //字体数据初值
    {
        Font[i].size = 0;
        Font[i].name = NULL;
        Font[i].font = NULL;
    }

    return 0;
}

//释放字体结构
int ExitFont()
{
    JY_Debug("%d chars cached.", char_count);
    /*for (int i = 0; i < 100; i++)
    {
        for (int j = 0; j < 60000; j++)
        {
            if (chars_cache[i][j])
            { SDL_DestroyTexture(chars_cache[i][j]); }
        }
    }*/
    //for (auto& c : chars_cache)
    //{
    //SDL_DestroyTexture(c.second);
    //}

    for (int i = 0; i < FONTNUM; i++)    //释放字体数据
    {
        if (Font[i].font)
        {
            TTF_CloseFont(Font[i].font);
        }
        SafeFree(Font[i].name);
    }

    TTF_Quit();

    return 0;
}

// 根据字体文件名和字号打开字体
// size 为按像素大小的字号
TTF_Font* GetFont(const char* filename, int size)
{
    int i;
    TTF_Font* myfont = NULL;

    for (i = 0; i < FONTNUM; i++)    //  判断字体是否已打开
    {
        if ((Font[i].size == size) && (Font[i].name) && (strcmp(filename, Font[i].name) == 0))
        {
            myfont = Font[i].font;
            break;
        }
    }

    if (myfont == NULL)    //没有打开
    {
        myfont = TTF_OpenFont(filename, size);    //打开新字体
        if (myfont == NULL)
        {
            JY_Error("GetFont error: can not open font file %s\n", filename);
            return NULL;
        }
        Font[currentFont].size = size;
        if (Font[currentFont].font)    //直接关闭当前字体。
        {
            TTF_CloseFont(Font[currentFont].font);
        }

        Font[currentFont].font = myfont;

        SafeFree(Font[currentFont].name);
        Font[currentFont].name = (char*)malloc(strlen(filename) + 1);
        strcpy(Font[currentFont].name, filename);

        currentFont++;    // 增加队列入口计数
        if (currentFont == FONTNUM)
        {
            currentFont = 0;
        }
    }

    return myfont;
}

// 写字符串
// x,y 坐标
// str 字符串
// color 颜色
// size 字体大小，字形为宋体。
// fontname 字体名
// charset 字符集 0 GBK 1 big5 3 utf-8
// OScharset 0 简体显示 1 繁体显示
int JY_DrawStr(int x, int y, const char* str, int color, int size, const char* fontname,
    int charset, int OScharset)
{
    SDL_Color c, c2, white;
    SDL_Surface *fontSurface = NULL, *fontSurface1 = NULL;
    int w, h;
    SDL_FRect rect1, rect2, rect_dest;
    SDL_Rect rect;
    //char tmp1[256], tmp2[256];
    TTF_Font* myfont;
    SDL_Surface* tempSurface;

    //没有内容不显示
    if (strlen(str) == 0)
    {
        return 0;
    }

    if (strlen(str) > 127)
    {
        JY_Error("JY_DrawStr: string length more than 127: %s", str);
        return 0;
    }

    myfont = GetFont(fontname, size);
    if (myfont == NULL)
    {
        return 1;
    }

    c.r = (Uint8)((color & RMASK) >> 16);
    c.g = (Uint8)((color & GMASK) >> 8);
    c.b = (Uint8)((color & BMASK));
    c.a = AMASK;
    c2.r = c.r >> 1;
    c2.b = c.b >> 1;
    c2.g = c.g >> 1;
    c2.a = AMASK;
    white.r = 255;
    white.g = 255;
    white.b = 255;
    white.a = 255;

    std::string tmp;
    //charset=3;
    if (charset == 0 && OScharset == 0)    //GBK -->unicode简体
    {
        tmp = PotConv::conv(str, "cp936", "utf-16le");
        //JY_CharSet(str, tmp2, 3);
    }
    else if (charset == 0 && OScharset == 1)    //GBK -->unicode繁体
    {
        tmp = PotConv::conv(str, "cp936", "utf-8");
        tmp = OpenCCConverter::getInstance()->UTF8s2t(tmp);
        tmp = PotConv::conv(tmp, "utf-8", "utf-16le");
        //JY_CharSet(str, tmp1, 1);
        //JY_CharSet(tmp1, tmp2, 2);
    }
    else if (charset == 1 && OScharset == 0)    //big5-->unicode简体
    {
        tmp = PotConv::conv(str, "cp950", "utf-16le");
        //JY_CharSet(tmp1, tmp2, 3);
    }
    else if (charset == 1 && OScharset == 1)    //big5-->unicode繁体
    {
        tmp = PotConv::conv(str, "cp936", "utf-8");
        tmp = OpenCCConverter::getInstance()->UTF8s2t(tmp);
        tmp = PotConv::conv(tmp, "utf-8", "utf-16le");
        //JY_CharSet(str, tmp2, 2);
    }
    else if (charset == 3 && OScharset == 0)    //big5-->unicode简体
    {
        tmp = PotConv::conv(str, "utf-8", "utf-16le");
        //JY_CharSet(tmp1, tmp2, 3);
    }
    else if (charset == 3 && OScharset == 1)    //big5-->unicode繁体
    {
        tmp = OpenCCConverter::getInstance()->UTF8s2t(str);
        tmp = PotConv::conv(tmp, "utf-8", "utf-16le");
        //JY_CharSet(str, tmp2, 2);
    }
    else
    {
        //strcpy(tmp2, str);
        tmp = str;
    }
    tmp.resize(tmp.size() + 2);
    SDL_GetRenderClipRect(g_Renderer, &rect);
    if (rect.w == 0) { rect.w = g_ScreenW - rect.x; }
    if (rect.h == 0) { rect.h = g_ScreenH - rect.y; }

    //TTF_Size(myfont, (Uint16*)tmp.c_str(), &w, &h);
    w=size;
    h = size;

    if ((x >= rect.x + rect.w) || (x + w + 1) <= rect.x || (y >= rect.y + rect.h) || (y + h + 1) <= rect.y)    // 超出裁剪范围则不显示
    {
        return 1;
    }

    //fontSurface=TTF_RenderUNICODE_Solid(myfont, (Uint16*)tmp2, c);  //生成表面

    Uint16* p = (Uint16*)tmp.c_str();
    rect1.x = x;
    rect1.y = y;
    int pi = 0;
    //printf("%d,%d\n", strlen(str), strlen(tmp2));
    for (int i = 0; i < 128; i++)
    {
        int s = size;
        if (*p == 0)
        {
            break;
        }
        if (*p <= 128) { s = size / 2; }
        SDL_Texture* tex = NULL;
        if (chars_cache[fontname].count(*p + size * 65536))
        {
            tex = chars_cache[fontname][*p + size * 65536];
        }
        if (tex == NULL)
        {
            Uint32 tmp = 0;
            tmp = *p;
            SDL_Surface* sur = TTF_RenderGlyph_Blended(myfont, tmp, white);
            tex = SDL_CreateTextureFromSurface(g_Renderer, sur);
            chars_cache[fontname][*p + size * 65536] = tex;
            SDL_DestroySurface(sur);
            char_count++;
#ifdef _DEBUG
            unsigned char out[3] = { 0, 0, 0 };
            out[0] = *(str + pi);
            if (out[0] > 128)
            {
                out[1] = *(str + pi + 1);
            }
            //JY_Debug("cache [%d][%s(%d)], total %d", size, out, *p, char_count);
#endif
        }
#ifdef _DEBUG
        if (*p <= 128) { pi++; }
        else { pi += 2; }
#endif
        if (*p != 32)
        {
            SDL_GetTextureSize(tex, &rect1.w, &rect1.h);
            rect2 = rect1;
            SDL_SetRenderTarget(g_Renderer, g_Texture);
            SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
            SDL_SetTextureColorMod(tex, c2.r, c2.g, c2.b);
            SDL_SetTextureAlphaMod(tex, 128);

            if ((c.r == 0x00000000) && (c.g == 0x00000000) && (c.b == 0x00000000))
            {
            }
            else
            {
                rect2.x = rect1.x + 1;
                rect2.y = rect1.y + 1;
                SDL_RenderTexture(g_Renderer, tex, NULL, &rect2);
                //SDL_BlitSurface(*tex, NULL, g_Surface, &rect2);
                rect2.x = rect1.x + 1;
                rect2.y = rect1.y;
                SDL_RenderTexture(g_Renderer, tex, NULL, &rect2);
                //SDL_BlitSurface(*tex, NULL, g_Surface, &rect2);
                rect2.x = rect1.x;
                rect2.y = rect1.y + 1;
                SDL_RenderTexture(g_Renderer, tex, NULL, &rect2);
            }
            //SDL_BlitSurface(*tex, NULL, g_Surface, &rect2);
            SDL_SetTextureColorMod(tex, c.r, c.g, c.b);
            SDL_SetTextureAlphaMod(tex, 255);

            SDL_RenderTexture(g_Renderer, tex, NULL, &rect1);
            //SDL_BlitSurface(*tex, NULL, g_Surface, &rect1);
            s = rect1.w;
        }
        rect1.x = rect1.x + s;
        p++;
    }

    //JY_Debug("%d chars cached.", char_count);
    //fontSurface = TTF_RenderUNICODE_Blended(myfont, (Uint16*)tmp2, c);
    //fontSurface1 = TTF_RenderUNICODE_Blended(myfont, (Uint16*)tmp2, c2);
    //SDL_SetSurfaceAlphaMod(fontSurface, 255);
    //SDL_SetSurfaceAlphaMod(fontSurface1, 128);

    if (fontSurface == NULL)
    {
        return 1;
    }

    rect1.x = (Sint16)x;
    rect1.y = (Sint16)y;
    rect1.w = (Uint16)fontSurface->w;
    rect1.h = (Uint16)fontSurface->h;

    /*
    if (g_Rotate == 0)
    {
        rect2 = rect1;

        rect_dest.x = rect2.x + 1;
        rect_dest.y = rect2.y;
        //SDL_SetPaletteColors(fontSurface1->format->palette, &c2, 1, 1);
        SDL_BlitSurface(fontSurface1, NULL, g_Surface, &rect_dest);    //表面写到游戏表面--阴影色

        rect_dest.x = rect2.x;
        rect_dest.y = rect2.y + 1;
        //SDL_SetPaletteColors(fontSurface1->format->palette, &c2, 1, 1);
        SDL_BlitSurface(fontSurface1, NULL, g_Surface, &rect_dest);    //表面写到游戏表面--阴影色

        rect_dest.x = rect2.x + 1;
        rect_dest.y = rect2.y + 1;
        //SDL_SetPaletteColors(fontSurface1->format->palette, &c2, 1, 1);
        SDL_BlitSurface(fontSurface1, NULL, g_Surface, &rect_dest);    //表面写到游戏表面--阴影色

        rect_dest.x = rect2.x;
        rect_dest.y = rect2.y;
        //SDL_SetPaletteColors(fontSurface->format->palette, &c, 1, 1);
        SDL_BlitSurface(fontSurface, NULL, g_Surface, &rect_dest);    //表面写到游戏表面
        SDL_FreeSurface(fontSurface1);
    }
    else if (g_Rotate == 1)
    {
        tempSurface = RotateSurface(fontSurface);
        SDL_FreeSurface(fontSurface);
        fontSurface = tempSurface;
        rect2 = RotateRect(&rect1);

        rect_dest.x = rect2.x + 1;
        rect_dest.y = rect2.y;
        SDL_SetPaletteColors(fontSurface1->format->palette, &c2, 1, 1);
        SDL_BlitSurface(fontSurface1, NULL, g_Surface, &rect_dest);    //表面写到游戏表面--阴影色

        rect_dest.x = rect2.x - 1;
        rect_dest.y = rect2.y;
        SDL_SetPaletteColors(fontSurface1->format->palette, &c2, 1, 1);
        SDL_BlitSurface(fontSurface1, NULL, g_Surface, &rect_dest);    //表面写到游戏表面--阴影色

        rect_dest.x = rect2.x;
        rect_dest.y = rect2.y + 1;
        SDL_SetPaletteColors(fontSurface1->format->palette, &c2, 1, 1);
        SDL_BlitSurface(fontSurface1, NULL, g_Surface, &rect_dest);    //表面写到游戏表面--阴影色

        rect_dest.x = rect2.x;
        rect_dest.y = rect2.y - 1;
        SDL_SetPaletteColors(fontSurface1->format->palette, &c2, 1, 1);
        SDL_BlitSurface(fontSurface1, NULL, g_Surface, &rect_dest);    //表面写到游戏表面--阴影色


        rect_dest.x = rect2.x;
        rect_dest.y = rect2.y;
        SDL_SetPaletteColors(fontSurface->format->palette, &c, 1, 1);
        SDL_BlitSurface(fontSurface, NULL, g_Surface, &rect_dest);    //表面写到游戏表面

    }*/

    //SDL_FreeSurface(fontSurface);   //释放表面
    return 0;
}

// 汉字字符集转换
// flag = 0   Big5 --> GBK
//      = 1   GBK  --> Big5
//      = 2   Big5 --> Unicode
//      = 3   GBK  --> Unicode
// 注意要保证dest有足够的空间，一般建议取src长度的两倍+2，保证全英文字符也能转化为unicode
int JY_CharSet(const char* src, char* dest, int flag)
{

    Uint8 *psrc, *pdest;
    Uint8 b0, b1;
    int d0;
    Uint16 tmpchar;

    psrc = (Uint8*)src;
    pdest = (Uint8*)dest;

    for (;;)
    {
        b0 = *psrc;
        if (b0 == 0)    //字符串结束
        {
            if ((flag == 0) || (flag == 1))
            {
                *pdest = 0;
                break;
            }
            else    //unicode结束标志 0x0000?
            {
                *pdest = 0;
                *(pdest + 1) = 0;
                break;
            }
        }
        if (b0 < 128)    //英文字符
        {
            if ((flag == 0) || (flag == 1))    //不转换
            {
                *pdest = b0;
                pdest++;
                psrc++;
            }
            else    //unicode 后面加个0
            {
                *pdest = b0;
                pdest++;
                *pdest = 0;
                pdest++;
                psrc++;
            }
        }
        else    //中文字符
        {
            b1 = *(psrc + 1);
            if (b1 == 0)    // 非正常结束
            {
                *pdest = '?';
                *(pdest + 1) = 0;
                break;
            }
            else
            {
                d0 = b0 + b1 * 256;
                switch (flag)
                {
                case 0:    //Big5 --> GBK
                    tmpchar = big5_gbk[b0 - 128][b1];
                    break;
                case 1:    //GBK  --> Big5
                    tmpchar = gbk_big5[b0 - 128][b1];
                    break;
                case 2:    //Big5 --> Unicode
                    tmpchar = big5_unicode[b0 - 128][b1];
                    break;
                case 3:    //GBK  --> Unicode
                    tmpchar = gbk_unicode[b0 - 128][b1];
                    break;
                default:
                    tmpchar = 0;
                }

                if (tmpchar != 0)
                {
                    *(Uint16*)pdest = tmpchar;
                }
                else
                {
                    *pdest = '?';
                    *(pdest + 1) = '?';
                }

                pdest = pdest + 2;
                psrc = psrc + 2;
            }
        }
    }

    return 0;
}

//加载码表文件
//码表文件顺序： 按GBK排列，unicode，big5。然后按big5排列，unicode和gbk。
int LoadMB(const char* mbfile)
{
    FILE* fp;
    int i, j;

    Uint16 gbk, big5, unicode;

    fp = fopen(mbfile, "rb");
    if (fp == NULL)
    {
        JY_Error("cannot open mbfile");
        return 1;
    }

    for (i = 0; i < 128; i++)
    {
        for (j = 0; j < 256; j++)
        {
            gbk_unicode[i][j] = 0;
            gbk_big5[i][j] = 0;
            big5_gbk[i][j] = 0;
            big5_unicode[i][j] = 0;
        }
    }

    for (i = 0x81; i <= 0xfe; i++)
    {
        for (j = 0x40; j <= 0xfe; j++)
        {
            if (j != 0x7f)
            {
                fread(&unicode, 2, 1, fp);
                fread(&big5, 2, 1, fp);
                gbk_unicode[i - 128][j] = unicode;
                gbk_big5[i - 128][j] = big5;
            }
        }
    }

    for (i = 0xa0; i <= 0xfe; i++)
    {
        for (j = 0x40; j <= 0xfe; j++)
        {
            if (j <= 0x7e || j >= 0xa1)
            {
                fread(&unicode, 2, 1, fp);
                fread(&gbk, 2, 1, fp);
                big5_unicode[i - 128][j] = unicode;
                big5_gbk[i - 128][j] = gbk;
            }
        }
    }

    fclose(fp);

    return 0;
}
