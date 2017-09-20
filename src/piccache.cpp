
// 读取idx/grp的贴图文件。
// 为提高速度，采用缓存方式读取。把idx/grp读入内存，然后定义若干个缓存表面
// 经常访问的pic放在缓存表面中

#include <stdlib.h>
#include "piccache.h"
#include "jymain.h"
#include "sdlfun.h"

PicFileCache pic_file[PIC_FILE_NUM];
//std::forward_list<CacheNode*> pic_cache;     //pic_cache链表
Uint32 m_color32[256];               // 256调色板
//int CacheFailNum = 0;

void CacheNode::toTexture()
{
    if (s)
    {
        if (t == NULL)
        {
            t = SDL_CreateTextureFromSurface(g_Renderer, s);
            SDL_FreeSurface(s);
            s = NULL;
        }
    }
}

// 初始化Cache数据。游戏开始时调用
int Init_Cache()
{
    int i;
    for (i = 0; i < PIC_FILE_NUM; i++)
    {
        pic_file[i].num = 0;
        pic_file[i].idx = NULL;
        pic_file[i].grp = NULL;
        pic_file[i].fp = NULL;
        //pic_file[i].pcache = NULL;
    }
    return 0;
}

// 初始化贴图cache信息
// PalletteFilename 为256调色板文件。第一次调用时载入
//                  为空字符串则表示重新清空贴图cache信息。在主地图/场景/战斗切换时调用

int JY_PicInit(char* PalletteFilename)
{
    struct list_head* pos, *p;
    int i;

    LoadPalette(PalletteFilename);   //载入调色板

    //如果链表不为空，删除全部链表
    //for (auto& c : pic_cache)
    //{
    //    delete c;
    //}
    //pic_cache.clear();

    for (i = 0; i < PIC_FILE_NUM; i++)
    {
        pic_file[i].num = 0;
        SafeFree(pic_file[i].idx);
        SafeFree(pic_file[i].grp);

        for (auto& p : pic_file[i].pcache)
        {
            delete p;
        }
        pic_file[i].pcache.clear();
        //SafeFree(pic_file[i].pcache);
        if (pic_file[i].fp)
        {
            fclose(pic_file[i].fp);
            pic_file[i].fp = NULL;
        }
    }
    //CacheFailNum = 0;
    return 0;
}

// 加载文件信息
// filename 文件名
// id  0 - PIC_FILE_NUM-1
int JY_PicLoadFile(const char* idxfilename, const char* grpfilename, int id, int width, int height)
{
    int i;
    struct CacheNode* tmpcache;
    FILE* fp;

    if (id < 0 || id >= PIC_FILE_NUM)    // id超出范围
    {
        return 1;
    }

    if (pic_file[id].pcache.size())          //释放当前文件占用的空间，并清理cache
    {
        int i;
        for (i = 0; i < pic_file[id].num; i++)     //循环全部贴图，
        {
            tmpcache = pic_file[id].pcache[i];
            if (tmpcache)         // 该贴图有缓存则删除
            {
                delete tmpcache;
            }
        }
        //SafeFree(pic_file[id].pcache);
    }
    SafeFree(pic_file[id].idx);
    SafeFree(pic_file[id].grp);
    if (pic_file[id].fp)
    {
        fclose(pic_file[id].fp);
        pic_file[id].fp = NULL;
    }

    // 读取idx文件

    pic_file[id].num = FileLength(idxfilename) / 4;    //idx 贴图个数
    pic_file[id].idx = (int*)malloc((pic_file[id].num + 1) * 4);
    if (pic_file[id].idx == NULL)
    {
        JY_Error("JY_PicLoadFile: cannot malloc idx memory!\n");
        return 1;
    }
    //读取贴图idx文件
    if ((fp = fopen(idxfilename, "rb")) == NULL)
    {
        JY_Error("JY_PicLoadFile: idx file not open ---%s", idxfilename);
        return 1;
    }

    fread(&pic_file[id].idx[1], 4, pic_file[id].num, fp);
    fclose(fp);

    pic_file[id].idx[0] = 0;

    //读取grp文件
    pic_file[id].filelength = FileLength(grpfilename);

    //读取贴图grp文件
    if ((fp = fopen(grpfilename, "rb")) == NULL)
    {
        JY_Error("JY_PicLoadFile: grp file not open ---%s", grpfilename);
        return 1;
    }
    if (g_PreLoadPicGrp == 1)     //grp文件读入内存
    {
        pic_file[id].grp = (unsigned char*)malloc(pic_file[id].filelength);
        if (pic_file[id].grp == NULL)
        {
            JY_Error("JY_PicLoadFile: cannot malloc grp memory!\n");
            return 1;
        }
        fread(pic_file[id].grp, 1, pic_file[id].filelength, fp);
        fclose(fp);
    }
    else
    {
        pic_file[id].fp = fp;
    }

    pic_file[id].pcache.resize(pic_file[id].num);
    if (pic_file[id].pcache.size() == 0)
    {
        JY_Error("JY_PicLoadFile: cannot malloc pcache memory!\n");
        return 1;
    }

    for (i = 0; i < pic_file[id].num; i++)
    { pic_file[id].pcache[i] = NULL; }

    if (height == 0)
    { height = width; }

    if (width > 0)
    {
        pic_file[id].width = width;
        pic_file[id].height = height;
    }

    return 0;
}

int JY_LoadPic(int fileid, int picid, int x, int y, int flag, int value)
{
    return JY_LoadPicColor(fileid, picid, x, y, flag, value, 0);
}

// 加载并显示贴图
// fileid        贴图文件id
// picid     贴图编号
// x,y       显示位置
//  flag 不同bit代表不同含义，缺省均为0
//  B0    0 考虑偏移xoff，yoff。=1 不考虑偏移量
//  B1    0     , 1 与背景alpla 混合显示, value 为alpha值(0-256), 0表示透明
//  B2            1 全黑
//  B3            1 全白
//  value 按照flag定义，为alpha值，

int JY_LoadPicColor(int fileid, int picid, int x, int y, int flag, int value, int color)
{

    struct CacheNode* newcache, *tmpcache;
    int xnew, ynew;
    SDL_Surface* tmpsur;

    picid = picid / 2;

    if (fileid < 0 || fileid >= PIC_FILE_NUM || picid < 0 || picid >= pic_file[fileid].num)    // 参数错误
    { return 1; }

    if (pic_file[fileid].pcache[picid] == NULL)     //当前贴图没有加载
    {
        //生成cache数据
        newcache = new CacheNode();
        if (newcache == NULL)
        {
            JY_Error("JY_LoadPic: cannot malloc newcache memory!\n");
            return 1;
        }

        newcache->id = picid;
        newcache->fileid = fileid;
        LoadPic(fileid, picid, newcache);
        ////指定宽度和高度
        //if (newcache->s != NULL && pic_file[fileid].width > 0 && pic_file[fileid].height > 0
        //    && pic_file[fileid].width != newcache->s->w && pic_file[fileid].height != newcache->s->h)
        //{
        //    double zoomx = (double)pic_file[fileid].width / newcache->s->w;
        //    double zoomy = (double)pic_file[fileid].height / newcache->s->h;

        //    if (zoomx < zoomy)
        //    {
        //        zoomy = zoomx;
        //    }
        //    else
        //    {
        //        zoomx = zoomy;
        //    }

        //    tmpsur = newcache->s;

        //    newcache->s = zoomSurface(tmpsur, zoomx, zoomy, SMOOTHING_OFF);

        //    newcache->xoff = (int)(zoomx * newcache->xoff);
        //    newcache->yoff = (int)(zoomy * newcache->yoff);
        //    //SDL_SetColorKey(newcache->s, SDL_TRUE, ConvertColor(g_MaskColor32));  //透明色
        //    SDL_FreeSurface(tmpsur);
        //}
        pic_file[fileid].pcache[picid] = newcache;
    }
    else
    {
        newcache = pic_file[fileid].pcache[picid];
    }

    if (newcache->t == NULL)     //贴图为空，直接退出
    {
        return 1;
    }

    if (flag & 0x00000001)
    {
        xnew = x;
        ynew = y;
    }
    else
    {
        xnew = x - newcache->xoff;
        ynew = y - newcache->yoff;
    }
    RenderTexture(newcache->t, xnew, ynew, flag, value, color);
    return 0;
}

// 加载贴图到表面
static int LoadPic(int fileid, int picid, struct CacheNode* cache)
{

    SDL_RWops* fp_SDL;
    int id1, id2;
    int datalong;
    unsigned char* p, *data;

    SDL_Surface* tmpsurf = NULL, *tmpsur;

    if (pic_file[fileid].idx == NULL)
    {
        JY_Error("LoadPic: fileid %d can not load?\n", fileid);
        return 1;
    }
    id1 = pic_file[fileid].idx[picid];
    id2 = pic_file[fileid].idx[picid + 1];

    // 处理一些特殊情况，按照修改器中的代码
    if (id1 < 0)
    { datalong = 0; }

    if (id2 > pic_file[fileid].filelength)
    { id2 = pic_file[fileid].filelength; }

    datalong = id2 - id1;

    if (datalong > 0)
    {
        //读取贴图grp文件，得到原始数据
        if (g_PreLoadPicGrp == 1)           //有预读，从内存中读数据
        {
            data = pic_file[fileid].grp + id1;
            p = NULL;
        }
        else         //没有预读，从文件中读取
        {
            fseek(pic_file[fileid].fp, id1, SEEK_SET);
            data = (unsigned char*)malloc(datalong);
            p = data;
            fread(data, 1, datalong, pic_file[fileid].fp);
        }

        fp_SDL = SDL_RWFromMem(data, datalong);
        if (IMG_isPNG(fp_SDL) == 0)
        {
            int w, h;
            w = *(short*)data;
            h = *(short*)(data + 2);
            cache->xoff = *(short*)(data + 4);
            cache->yoff = *(short*)(data + 6);
            cache->w = w;
            cache->h = h;
            cache->t = CreateTextureFromRLE(data + 8, w, h, datalong - 8);
            //cache->t = SDL_CreateTextureFromSurface(g_Renderer, cache->s);
            //SDL_FreeSurface(cache->s);
            cache->s = NULL;
        }
        else        //读取png格式
        {
            tmpsurf = IMG_LoadPNG_RW(fp_SDL);
            if (tmpsurf == NULL)
            {
                JY_Error("LoadPic: cannot create SDL_Surface tmpsurf!\n");
            }
            cache->xoff = tmpsurf->w / 2;
            cache->yoff = tmpsurf->h;
            cache->w = tmpsurf->w;
            cache->h = tmpsurf->h;
            cache->s = tmpsurf;
            cache->toTexture();
            //cache->t = SDL_CreateTextureFromSurface(g_Renderer, cache->s);
            //SDL_FreeSurface(cache->s);
            //cache->s = NULL;
        }
        SDL_FreeRW(fp_SDL);
        SafeFree(p);
    }
    else
    {
        cache->s = NULL;
        cache->t = NULL;
        cache->xoff = 0;
        cache->yoff = 0;
    }

    return 0;
}


//得到贴图大小
int JY_GetPicXY(int fileid, int picid, int* w, int* h, int* xoff, int* yoff)
{
    struct CacheNode* newcache;
    int r = JY_LoadPic(fileid, picid, g_ScreenW + 1, g_ScreenH + 1, 1, 0);   //加载贴图到看不见的位置

    *w = 0;
    *h = 0;
    *xoff = 0;
    *yoff = 0;

    if (r != 0)
    { return 1; }

    newcache = pic_file[fileid].pcache[picid / 2];

    if (newcache->t)        // 已有，则直接显示
    {
        *w = newcache->w;
        *h = newcache->h;
        *xoff = newcache->xoff;
        *yoff = newcache->yoff;
    }

    return 0;
}

//按照原来游戏的RLE格式创建表面
SDL_Texture* CreateTextureFromRLE(unsigned char* data, int w, int h, int datalong)
{
    int p = 0;
    int i, j;
    int yoffset;
    int row;
    int start;
    int x;
    int solidnum;
    SDL_Surface* ps1, *ps2;
    Uint32* data32 = NULL;

    data32 = (Uint32*)malloc(w * h * 4);
    if (data32 == NULL)
    {
        JY_Error("CreatePicSurface32: cannot malloc data32 memory!\n");
        return NULL;
    }

    for (i = 0; i < w * h; i++)
    { data32[i] = 0; }

    for (i = 0; i < h; i++)
    {
        yoffset = i * w;
        row = data[p];            // i行数据个数
        start = p;
        p++;
        if (row > 0)
        {
            x = 0;                // i行目前列
            for (;;)
            {
                x = x + data[p];    // i行空白点个数，跳个透明点
                if (x >= w)        // i行宽度到头，结束
                { break; }

                p++;
                solidnum = data[p];  // 不透明点个数
                p++;
                for (j = 0; j < solidnum; j++)
                {
                    if (g_Rotate == 0)
                    {
                        data32[yoffset + x] = m_color32[data[p]] | AMASK;
                    }
                    else
                    {
                        data32[h - i - 1 + x * h] = m_color32[data[p]] | AMASK;
                    }
                    p++;
                    x++;
                }
                if (x >= w)
                { break; }     // i行宽度到头，结束
                if (p - start >= row)
                { break; }    // i行没有数据，结束
            }
            if (p + 1 >= datalong)
            { break; }
        }
    }
    ps1 = SDL_CreateRGBSurfaceFrom(data32, w, h, 32, w * 4, RMASK, GMASK, BMASK, AMASK);  //创建32位表面
    if (ps1 == NULL)
    {
        JY_Error("CreatePicSurface32: cannot create SDL_Surface ps1!\n");
    }
    ps2 = SDL_ConvertSurfaceFormat(ps1, g_Surface->format->format, 0);
    if (ps2 == NULL)
    {
        JY_Error("CreatePicSurface32: cannot create SDL_Surface ps2!\n");
    }
    auto tex = SDL_CreateTextureFromSurface(g_Renderer, ps2);
    SDL_FreeSurface(ps1);
    SDL_FreeSurface(ps2);
    SafeFree(data32);
    return tex;
}

// 读取调色板
// 文件名为空则直接返回
static int LoadPalette(char* filename)
{
    FILE* fp;
    char color[3];
    int i;
    if (strlen(filename) == 0)
    { return 1; }
    if ((fp = fopen(filename, "rb")) == NULL)
    {
        JY_Error("palette File not open ---%s", filename);
        return 1;
    }
    for (i = 0; i < 256; i++)
    {
        fread(color, 1, 3, fp);
        m_color32[i] = color[0] * 4 * 65536l + color[1] * 4 * 256 + color[2] * 4 + 0x000000;

    }
    fclose(fp);

    return 0;
}


int JY_LoadPNGPath(const char* path, int fileid, int num, int percent, const char* suffix)
{
    int i;
    struct CacheNode* tmpcache;
    if (fileid < 0 || fileid >= PIC_FILE_NUM)    // id超出范围
    {
        return 1;
    }

    if (pic_file[fileid].pcache.size())          //释放当前文件占用的空间，并清理cache
    {
        int i;
        for (i = 0; i < pic_file[fileid].num; i++)     //循环全部贴图，
        {
            tmpcache = pic_file[fileid].pcache[i];
            if (tmpcache)         // 该贴图有缓存则删除
            {
                delete tmpcache;
            }
        }
        //SafeFree(pic_file[fileid].pcache);
    }

    pic_file[fileid].num = num;
    sprintf(pic_file[fileid].path, "%s", path);

    pic_file[fileid].pcache.resize(pic_file[fileid].num);
    if (pic_file[fileid].pcache.size() == 0)
    {
        JY_Error("JY_LoadPNGPath: cannot malloc pcache memory!\n");
        return 1;
    }
    for (i = 0; i < pic_file[fileid].num; i++)
    { pic_file[fileid].pcache[i] = NULL; }

    pic_file[fileid].percent = percent;
    sprintf(pic_file[fileid].suffix, "%s", suffix);

    return 0;
}

int JY_LoadPNG(int fileid, int picid, int x, int y, int flag, int value)
{
    struct CacheNode* newcache, *tmpcache;
    SDL_Surface* tmpsur;
    SDL_Rect r;

    picid = picid / 2;

    if (fileid < 0 || fileid >= PIC_FILE_NUM || picid < 0 || picid >= pic_file[fileid].num)    // 参数错误
    { return 1; }

    if (pic_file[fileid].pcache[picid] == NULL)     //当前贴图没有加载
    {
        char str[512];
        SDL_RWops* fp_SDL;
        double zoom = (double)pic_file[fileid].percent / 100.0;

        sprintf(str, "%s/%d.png", pic_file[fileid].path, picid);

        //生成cache数据
        newcache = new CacheNode();
        if (newcache == NULL)
        {
            JY_Error("JY_LoadPNG: cannot malloc newcache memory!\n");
            return 1;
        }

        newcache->id = picid;
        newcache->fileid = fileid;

        fp_SDL = SDL_RWFromFile(str, "rb");
        if (IMG_isPNG(fp_SDL))
        {
            tmpsur = IMG_LoadPNG_RW(fp_SDL);
            if (tmpsur == NULL)
            {
                JY_Error("JY_LoadPNG: cannot create SDL_Surface tmpsurf!\n");
                return 1;
            }

            newcache->xoff = tmpsur->w / 2;
            newcache->yoff = tmpsur->h / 2;
            newcache->w = tmpsur->w;
            newcache->h = tmpsur->h;
            newcache->s = tmpsur;
            newcache->toTexture();
            //newcache->t = SDL_CreateTextureFromSurface(g_Renderer, newcache->s);
            //SDL_FreeSurface(newcache->s);
            //newcache->s = NULL;
        }
        else
        {
            newcache->s = NULL;
            newcache->t = NULL;
            newcache->xoff = 0;
            newcache->yoff = 0;
        }

        SDL_FreeRW(fp_SDL);

        //指定比例
        if (pic_file[fileid].percent > 0 && pic_file[fileid].percent != 100 && zoom != 0 && zoom != 1)
        {
            newcache->w = (int)(zoom * newcache->w);
            newcache->h = (int)(zoom * newcache->h);
            //tmpsur = newcache->t;
            //newcache->s = zoomSurface(tmpsur, zoom, zoom, SMOOTHING_ON);
            newcache->xoff = (int)(zoom * newcache->xoff);
            newcache->yoff = (int)(zoom * newcache->yoff);
            //SDL_SetColorKey(newcache->s,SDL_SRCCOLORKEY|SDL_RLEACCEL ,ConvertColor(g_MaskColor32));  //透明色
            //SDL_FreeSurface(tmpsur);
        }
        pic_file[fileid].pcache[picid] = newcache;
    }
    else     //已加载贴图
    {
        newcache = pic_file[fileid].pcache[picid];
    }

    if (newcache->t == NULL)     //贴图为空，直接退出
    {
        return 1;
    }

    if (flag & 0x00000001)
    {
        r.x = x;
        r.y = y;
    }
    else
    {
        r.x = x - newcache->xoff;
        r.y = y - newcache->yoff;
    }

    //SDL_BlitSurface(newcache->s, NULL, g_Surface, &r);
    r.w = newcache->w;
    r.h = newcache->h;

    SDL_SetRenderTarget(g_Renderer, g_Texture);
    SDL_RenderCopy(g_Renderer, newcache->t, NULL, &r);

    return 0;
}

int JY_GetPNGXY(int fileid, int picid, int* w, int* h, int* xoff, int* yoff)
{
    int r = JY_LoadPNG(fileid, picid, g_ScreenW + 1, g_ScreenH + 1, 1, 0);   //加载贴图到看不见的位置

    *w = 0;
    *h = 0;
    *xoff = 0;
    *yoff = 0;

    if (r != 0)
    { return 1; }

    auto newcache = pic_file[fileid].pcache[picid / 2];

    if (newcache->t)        // 已有，则直接显示
    {
        *w = newcache->w;
        *h = newcache->h;
        *xoff = newcache->xoff;
        *yoff = newcache->yoff;
    }

    return 0;
}


// 把表面blit到背景或者前景表面
// x,y 要加载到表面的左上角坐标
int RenderTexture(SDL_Texture* lps, int x, int y, int flag, int value, int color)
{
    SDL_Surface* tmps;
    SDL_Rect rect, rect0;
    int i, j;
    //color = ConvertColor(g_MaskColor32);
    if (value > 255)
    { value = 255; }
    rect.x = x;
    rect.y = y;
    SDL_QueryTexture(lps, NULL, NULL, &rect.w, &rect.h);
    rect0 = rect;
    rect0.x = 0;
    rect0.y = 0;
    if (!lps)
    {
        JY_Error("BlitSurface: lps is null!");
        return 1;
    }

    if ((flag & 0x2) == 0)          // 没有alpha
    {
        SDL_SetTextureColorMod(lps, 255, 255, 255);
        SDL_SetTextureBlendMode(lps, SDL_BLENDMODE_BLEND);
        SDL_SetTextureAlphaMod(lps, 255);
        RenderToTexture(lps, NULL, g_Texture, &rect);
        //SDL_BlitSurface(lps, NULL, g_Surface, &rect);
    }
    else    // 有alpha
    {
        if ((flag & 0x4) || (flag & 0x8) || (flag & 0x10))     // 4-黑, 8-白, 16-颜色
        {
            // 4-黑, 8-白, 16-颜色
            if (flag & 0x4)
            {
                SDL_SetTextureColorMod(lps, 32, 32, 32);
                SDL_SetTextureBlendMode(lps, SDL_BLENDMODE_BLEND);
                SDL_SetTextureAlphaMod(lps, (Uint8)value);
                RenderToTexture(lps, NULL, g_Texture, &rect);
            }
            else if (flag & 0x8)
            {
                SDL_SetTextureBlendMode(lps, SDL_BLENDMODE_NONE);
                RenderToTexture(lps, NULL, g_TextureTmp, &rect);
                SDL_SetTextureBlendMode(lps, SDL_BLENDMODE_ADD);
                SDL_SetRenderDrawColor(g_Renderer, 255, 255, 255, 255);
                SDL_SetRenderDrawBlendMode(g_Renderer, SDL_BLENDMODE_ADD);
                SDL_RenderFillRect(g_Renderer, &rect);
                SDL_SetTextureColorMod(g_TextureTmp, 255, 255, 255);
                SDL_SetTextureBlendMode(g_TextureTmp, SDL_BLENDMODE_BLEND);
                SDL_SetTextureAlphaMod(g_TextureTmp, (Uint8)value);
                RenderToTexture(g_TextureTmp, &rect, g_Texture, &rect);
                SDL_SetTextureAlphaMod(g_TextureTmp, 255);
            }
            else
            {
                Uint8 r = (Uint8)((color & RMASK) >> 16);
                Uint8 g = (Uint8)((color & GMASK) >> 8);
                Uint8 b = (Uint8)((color & BMASK));
                Uint8 a = 255;
                SDL_SetTextureColorMod(lps, r, g, b);
                SDL_SetTextureBlendMode(lps, SDL_BLENDMODE_BLEND);
                SDL_SetTextureAlphaMod(lps, (Uint8)value);
                RenderToTexture(lps, NULL, g_Texture, &rect);
            }
        }
        else
        {
            SDL_SetTextureColorMod(lps, 255, 255, 255);
            SDL_SetTextureBlendMode(lps, SDL_BLENDMODE_BLEND);
            SDL_SetTextureAlphaMod(lps, (Uint8)value);
            RenderToTexture(lps, NULL, g_Texture, &rect);
            //SDL_BlitSurface(lps, NULL, g_Surface, &rect);
        }
    }
    return 0;
}


