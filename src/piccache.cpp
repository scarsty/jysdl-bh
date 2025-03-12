
// 读取idx/grp的贴图文件。
// 为提高速度，采用缓存方式读取。把idx/grp读入内存，然后定义若干个缓存表面
// 经常访问的pic放在缓存表面中

#include "piccache.h"
#include "jymain.h"
#include "sdlfun.h"

PicFileCache pic_file[PIC_FILE_NUM];
//std::forward_list<CacheNode*> pic_cache;     //pic_cache链表
Uint32 m_color32[256];    // 256调色板

//int CacheFailNum = 0;

void CacheNode::toTexture()
{
    if (s)
    {
        if (t == NULL)
        {
            t = SDL_CreateTextureFromSurface(g_Renderer, s);
            tt[0] = t;
            SDL_DestroySurface(s);
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

int JY_PicInit(const char* PalletteFilename)
{
    struct list_head *pos, *p;
    int i;

    LoadPalette(PalletteFilename);    //载入调色板

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

    JY_PicLoadFile("./data/wmap.idx", "./data/wmap.grp", 0, NULL, NULL);    //--特效贴图
    JY_LoadPNGPath("./data/head", 1, 20000, g_ScreenW / 936 * 100, "png");
    //JY_PicLoadFile("./data/thing.idx", "./data/thing.grp", 2, NULL, NULL);
    JY_LoadPNGPath("./data/thing", 2, -1, 100, "png");
    JY_PicLoadFile("./data/Eft.idx", "./data/Eft.grp", 3, NULL, NULL);    //--特效贴图
    JY_LoadPNGPath("./data/body", 90, 20000, g_ScreenW / 936 * 100, "png");
    JY_LoadPNGPath("./data/xt", 91, 20000, g_ScreenW / 936 * 100, "png");
    JY_PicLoadFile("./data/bj.idx", "./data/bj.grp", 92, NULL, NULL);
    JY_LoadPNGPath("./data/mmap", 93, -1, 100, "png");
    JY_LoadPNGPath("./data/smap", 94, -1, 100, "png");
    //JY_PicLoadFile("./data/smap.idx", "./data/smap.grp", 94, 0, 0);
    JY_LoadPNGPath("./data/portrait", 95, 20000, g_ScreenW / 936 * 100, "png");
    JY_LoadPNGPath("./data/ui", 96, 20000, g_ScreenW / 936 * 100, "png");
    JY_LoadPNGPath("./data/cloud", 97, -1, 100, "png");
    JY_LoadPNGPath("./data/icons", 98, 20000, g_ScreenW / 936 * 100, "png");
    JY_LoadPNGPath("./data/head", 99, 20000, 26.923076923, "png");
    for (i = 101; i < 1000; i++)
    {
        char figidx[512];
        char figgrp[512];
        sprintf(figidx, "./data/fight/fight%03d.idx", i - 101);
        sprintf(figgrp, "./data/fight/fight%03d.grp", i - 101);
        JY_PicLoadFile(figidx, figgrp, i, NULL, NULL);
    }

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

    if (pic_file[id].pcache.size())    //释放当前文件占用的空间，并清理cache
    {
        int i;
        for (i = 0; i < pic_file[id].num; i++)    //循环全部贴图，
        {
            tmpcache = pic_file[id].pcache[i];
            if (tmpcache)    // 该贴图有缓存则删除
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
    if (true)    //grp文件读入内存
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
    {
        pic_file[id].pcache[i] = NULL;
    }

    if (height == 0)
    {
        height = width;
    }

    if (width > 0)
    {
        pic_file[id].width = width;
        pic_file[id].height = height;
    }
    pic_file[id].type = 0;
    return 0;
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

int JY_LoadPic(int fileid, int picid, int x, int y, int flag, int value, int color, int width, int height, double rotate, SDL_FlipMode reversal, int percent)
{
    struct CacheNode *newcache, *tmpcache;
    int xnew, ynew;
    SDL_Surface* tmpsur;

    if (pic_file[fileid].type == 1)
    {
        JY_LoadPNG(fileid, picid, x, y, flag, value, percent);
        return 0;
    }

    picid = picid / 2;

    if (fileid < 0 || fileid >= PIC_FILE_NUM || picid < 0 || picid >= pic_file[fileid].num)    // 参数错误
    {
        return 1;
    }

    if (pic_file[fileid].pcache[picid] == NULL)    //当前贴图没有加载
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
        //    //SDL_SetSurfaceColorKey(newcache->s, true, ConvertColor(g_MaskColor32));  //透明色
        //    SDL_DestroySurface(tmpsur);
        //}
        pic_file[fileid].pcache[picid] = newcache;
    }
    else
    {
        newcache = pic_file[fileid].pcache[picid];
    }

    if (newcache->t == NULL)    //贴图为空，直接退出
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
        if (width > 0 && height > 0)    //太极猫调整：宽高自定义，贴图的位置调整
        {
            xnew = x - newcache->xoff * width / newcache->w;
            ynew = y - newcache->yoff * height / newcache->h;
        }
        else if (width > 0 && height <= 0)    //太极猫调整：宽自定义，贴图的位置调整
        {
            xnew = x - newcache->xoff * width / newcache->w;
            ynew = y - newcache->yoff * width / newcache->w;
        }
        else if (width <= 0 && height > 0)    //太极猫调整：比利自定义，贴图的位置调整
        {
            float bl = height / 100.0;
            width = newcache->w * bl;
            height = newcache->h * bl;
            xnew = x - newcache->xoff * bl;
            ynew = y - newcache->yoff * bl;
        }
        else if (width == 0 && height == 0)    //太极猫调整：宽高都为0则根据zoom值改变贴图大小
        {
            width = newcache->w * g_Zoom;
            height = newcache->h * g_Zoom;
            xnew = x - newcache->xoff * g_Zoom;
            ynew = y - newcache->yoff * g_Zoom;
        }
        else
        {
            xnew = x - newcache->xoff;
            ynew = y - newcache->yoff;
        }
    }
    RenderTexture(newcache->t, xnew, ynew, flag, value, color, width, height, rotate, reversal, percent);
    return 0;
}

// 加载贴图到表面
int LoadPic(int fileid, int picid, struct CacheNode* cache)
{
    SDL_IOStream* fp_SDL;
    int id1, id2;
    int datalong;
    unsigned char *p, *data;

    SDL_Surface *tmpsurf = NULL, *tmpsur;

    if (pic_file[fileid].idx == NULL)
    {
        JY_Error("LoadPic: fileid %d can not load?\n", fileid);
        return 1;
    }
    id1 = pic_file[fileid].idx[picid];
    id2 = pic_file[fileid].idx[picid + 1];

    // 处理一些特殊情况，按照修改器中的代码
    if (id1 < 0)
    {
        datalong = 0;
    }

    if (id2 > pic_file[fileid].filelength)
    {
        id2 = pic_file[fileid].filelength;
    }

    datalong = id2 - id1;

    if (datalong > 0)
    {
        //读取贴图grp文件，得到原始数据
        if (true)    //有预读，从内存中读数据
        {
            data = pic_file[fileid].grp + id1;
            p = NULL;
        }
        else    //没有预读，从文件中读取
        {
            fseek(pic_file[fileid].fp, id1, SEEK_SET);
            data = (unsigned char*)malloc(datalong);
            p = data;
            fread(data, 1, datalong, pic_file[fileid].fp);
        }

        fp_SDL = SDL_IOFromMem(data, datalong);
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
            cache->tt[0] = cache->t;
            //cache->t = SDL_CreateTextureFromSurface(g_Renderer, cache->s);
            //SDL_DestroySurface(cache->s);
            cache->s = NULL;
        }
        else    //读取png格式
        {
            tmpsurf = IMG_LoadTyped_IO(fp_SDL, true, "png");
            if (tmpsurf == NULL)
            {
                JY_Error("LoadPic: cannot create SDL_Surface tmpsurf!\n");
            }
            cache->xoff = tmpsurf->w / 2;
            cache->yoff = tmpsurf->h / 2;
            cache->w = tmpsurf->w;
            cache->h = tmpsurf->h;
            cache->s = tmpsurf;
            cache->toTexture();
            //cache->t = SDL_CreateTextureFromSurface(g_Renderer, cache->s);
            //SDL_DestroySurface(cache->s);
            //cache->s = NULL;
        }
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
    int r = JY_LoadPic(fileid, picid, g_ScreenW + 1, g_ScreenH + 1, 1, 0);    //加载贴图到看不见的位置

    *w = 0;
    *h = 0;
    *xoff = 0;
    *yoff = 0;

    if (r != 0)
    {
        return 1;
    }

    newcache = pic_file[fileid].pcache[picid / 2];

    if (newcache->t)    // 已有，则直接显示
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
    SDL_Surface *ps1, *ps2;
    Uint32* data32 = NULL;

    data32 = (Uint32*)malloc(w * h * 4);
    if (data32 == NULL)
    {
        JY_Error("CreatePicSurface32: cannot malloc data32 memory!\n");
        return NULL;
    }

    for (i = 0; i < w * h; i++)
    {
        data32[i] = 0;
    }

    for (i = 0; i < h; i++)
    {
        yoffset = i * w;
        row = data[p];    // i行数据个数
        start = p;
        p++;
        if (row > 0)
        {
            x = 0;    // i行目前列
            for (;;)
            {
                x = x + data[p];    // i行空白点个数，跳个透明点
                if (x >= w)         // i行宽度到头，结束
                {
                    break;
                }

                p++;
                solidnum = data[p];    // 不透明点个数
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
                {
                    break;
                }    // i行宽度到头，结束
                if (p - start >= row)
                {
                    break;
                }    // i行没有数据，结束
            }
            if (p + 1 >= datalong)
            {
                break;
            }
        }
    }
    ps1 = SDL_CreateSurfaceFrom(w, h, SDL_GetPixelFormatForMasks(32, RMASK, GMASK, BMASK, AMASK), data32, w * 4);    //创建32位表面
    if (ps1 == NULL)
    {
        JY_Error("CreatePicSurface32: cannot create SDL_Surface ps1!\n");
    }
    ps2 = SDL_ConvertSurface(ps1, g_Surface->format);
    if (ps2 == NULL)
    {
        JY_Error("CreatePicSurface32: cannot create SDL_Surface ps2!\n");
    }
    auto tex = SDL_CreateTextureFromSurface(g_Renderer, ps2);
    SDL_DestroySurface(ps1);
    SDL_DestroySurface(ps2);
    SafeFree(data32);
    return tex;
}

// 读取调色板
// 文件名为空则直接返回
int LoadPalette(const char* filename)
{
    FILE* fp;
    char color[3];
    int i;
    if (strlen(filename) == 0)
    {
        return 1;
    }
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

    if (pic_file[fileid].pcache.size())    //释放当前文件占用的空间，并清理cache
    {
        int i;
        for (i = 0; i < pic_file[fileid].num; i++)    //循环全部贴图，
        {
            tmpcache = pic_file[fileid].pcache[i];
            if (tmpcache)    // 该贴图有缓存则删除
            {
                delete tmpcache;
            }
        }
        //SafeFree(pic_file[fileid].pcache);
    }

    //sb500添加
    pic_file[fileid].type = 1;
    int ll = 0;
    char zip_name[1024];
    sprintf(zip_name, "%s.zip", path);
    pic_file[fileid].zip_file.openFile(zip_name);

    char index_name[1024];
    std::vector<short> offset;

    if (pic_file[fileid].zip_file.opened())
    {
        std::string content = pic_file[fileid].zip_file.readEntryName("index.ka");
        ll = content.size();
        offset.resize(ll / 2);
        memcpy(offset.data(), content.data(), offset.size() * 2);
    }
    else
    {
        sprintf(index_name, "%s/index.ka", path);
        if (FILE* f = fopen(index_name, "rb"))
        {
            JY_Debug("JY_LoadPNGPath: found index file!\n");
            fseek(f, 0, SEEK_END);
            ll = ftell(f);
            fseek(f, 0, 0);
            offset.resize(ll / 2);
            fread(offset.data(), 2, ll / 2, f);
            fclose(f);
        }
    }

    if (num < 0)
    {
        num = ll / 4;    //图片个数
    }

    //sb500修改
    if (num > 0)
    {
        pic_file[fileid].num = num;
        sprintf(pic_file[fileid].path, "%s", path);

        pic_file[fileid].pcache.resize(pic_file[fileid].num);
        if (pic_file[fileid].pcache.size() == 0)
        {
            JY_Error("JY_LoadPNGPath: cannot malloc pcache memory!\n");
            return 1;
        }
        for (i = 0; i < pic_file[fileid].num; i++)
        {
            pic_file[fileid].pcache[i] = NULL;
        }
    }

    if (ll == 0)
    {
        //没有index文件
        pic_file[fileid].offset.resize(pic_file[fileid].num * 2);
        for (i = 0; i < pic_file[fileid].num; i++)
        {
            //没找到index文件则设置为一个不可能的数字
            pic_file[fileid].offset[i * 2] = 9999;
            pic_file[fileid].offset[i * 2 + 1] = 9999;
        }
    }
    else
    {
        //有index文件，则读取到的部分按照index设置偏移
        pic_file[fileid].offset = offset;
        pic_file[fileid].offset.resize(pic_file[fileid].num * 2);
        for (i = ll / 4; i < pic_file[fileid].num; i++)
        {
            pic_file[fileid].offset[i * 2] = 9999;
            pic_file[fileid].offset[i * 2 + 1] = 9999;
        }
    }

    pic_file[fileid].percent = percent;
    sprintf(pic_file[fileid].suffix, "%s", suffix);

    return 0;
}

int JY_LoadPNG(int fileid, int picid, int x, int y, int flag, int value, int percent)
{
    struct CacheNode *newcache, *tmpcache;
    SDL_Surface* tmpsur;
    SDL_FRect r;

    picid = picid / 2;

    if (fileid < 0 || fileid >= PIC_FILE_NUM || picid < 0 || picid >= pic_file[fileid].num)    // 参数错误
    {
        return 1;
    }
    if (pic_file[fileid].pcache[picid] == NULL)    //当前贴图没有加载
    {
        char str[512];
        SDL_IOStream* fp_SDL;
        double zoom = (double)pic_file[fileid].percent / 100.0;
        std::string content;
        //生成cache数据
        newcache = new CacheNode();
        if (newcache == NULL)
        {
            JY_Error("JY_LoadPNG: cannot malloc newcache memory!\n");
            return 1;
        }

        newcache->id = picid;
        newcache->fileid = fileid;
        newcache->t_count = 1;

        if (pic_file[fileid].zip_file.opened())
        {
            sprintf(str, "%d.png", picid);
            content = pic_file[fileid].zip_file.readEntryName(str);
            fp_SDL = SDL_IOFromMem((void*)content.data(), content.size());
            if (fp_SDL == NULL)
            {
                sprintf(str, "%d_0.png", picid);
                content = pic_file[fileid].zip_file.readEntryName(str);
                fp_SDL = SDL_IOFromMem((void*)content.data(), content.size());
                for (int i = 1; i < TEXTURE_NUM; i++)
                {
                    sprintf(str, "%d_%d.png", picid, i);
                    std::string content1 = pic_file[fileid].zip_file.readEntryName(str);
                    SDL_IOStream* fp_SDL1 = SDL_IOFromMem((void*)content1.data(), content1.size());
                    if (IMG_isPNG(fp_SDL1))
                    {
                        tmpsur = IMG_LoadTyped_IO(fp_SDL1, true, "png");
                        newcache->tt[i] = SDL_CreateTextureFromSurface(g_Renderer, tmpsur);
                        newcache->t_count = i + 1;
                        SDL_DestroySurface(tmpsur);
                    }
                }
            }
        }
        else
        {
            sprintf(str, "%s/%d.png", pic_file[fileid].path, picid);
            fp_SDL = SDL_IOFromFile(str, "rb");
            if (fp_SDL == NULL)
            {
                sprintf(str, "%s/%d_0.png", pic_file[fileid].path, picid);
                fp_SDL = SDL_IOFromFile(str, "rb");
                for (int i = 1; i < TEXTURE_NUM; i++)
                {
                    sprintf(str, "%s/%d_%d.png", pic_file[fileid].path, picid, i);
                    /*newcache->tt[i] = IMG_LoadTexture(g_Renderer, str);
                    if (newcache->tt[i]) { newcache->t_count = i; }*/
                    SDL_IOStream* fp_SDL1 = SDL_IOFromFile(str, "rb");
                    if (IMG_isPNG(fp_SDL1))
                    {
                        tmpsur = IMG_LoadTyped_IO(fp_SDL1, true, "png");
                        newcache->tt[i] = SDL_CreateTextureFromSurface(g_Renderer, tmpsur);
                        newcache->t_count = i + 1;
                        SDL_DestroySurface(tmpsur);
                    }
                }
            }
        }
        if (IMG_isPNG(fp_SDL))
        {
            tmpsur = IMG_LoadTyped_IO(fp_SDL, true, "png");
            if (tmpsur == NULL)
            {
                JY_Error("JY_LoadPNG: cannot create SDL_Surface tmpsurf!\n");
                return 1;
            }

            //sb500
            newcache->xoff = pic_file[fileid].offset[picid * 2];
            newcache->yoff = pic_file[fileid].offset[picid * 2 + 1];

            if (newcache->xoff == 9999)
            {
                newcache->xoff = tmpsur->w / 2;
            }
            if (newcache->yoff == 9999)
            {
                newcache->yoff = tmpsur->h / 2;
            }

            newcache->w = tmpsur->w;
            newcache->h = tmpsur->h;
            newcache->s = tmpsur;
            newcache->toTexture();
            //newcache->t = SDL_CreateTextureFromSurface(g_Renderer, newcache->s);
            //SDL_DestroySurface(newcache->s);
            //newcache->s = NULL;
        }
        else
        {
            newcache->s = NULL;
            newcache->t = NULL;
            newcache->xoff = 0;
            newcache->yoff = 0;
        }

        //指定比例
        if (pic_file[fileid].percent > 0 && pic_file[fileid].percent != 100 && zoom != 0 && zoom != 1)
        {
            newcache->w = (int)(zoom * newcache->w);
            newcache->h = (int)(zoom * newcache->h);
            //tmpsur = newcache->t;
            //newcache->s = zoomSurface(tmpsur, zoom, zoom, SMOOTHING_ON);
            newcache->xoff = (int)(zoom * newcache->xoff);
            newcache->yoff = (int)(zoom * newcache->yoff);
            //SDL_SetSurfaceColorKey(newcache->s,SDL_SRCCOLORKEY|SDL_RLEACCEL ,ConvertColor(g_MaskColor32));  //透明色
            //SDL_DestroySurface(tmpsur);
        }
        pic_file[fileid].pcache[picid] = newcache;
    }
    else    //已加载贴图
    {
        newcache = pic_file[fileid].pcache[picid];
    }

    if (newcache->t == NULL)    //贴图为空，直接退出
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
    r.w = newcache->w * percent / 100.0;
    r.h = newcache->h * percent / 100.0;

    SDL_SetRenderTarget(g_Renderer, g_Texture);
    if (g_DelayTimes % 2 == 0)
    {
        newcache->t = newcache->tt[g_DelayTimes / 2 % newcache->t_count];
    }
    SDL_RenderTexture(g_Renderer, newcache->t, NULL, &r);

    return 0;
}

int JY_GetPNGXY(int fileid, int picid, int* w, int* h, int* xoff, int* yoff)
{
    int r = JY_LoadPNG(fileid, picid, g_ScreenW + 1, g_ScreenH + 1, 1, 0, 100);    //加载贴图到看不见的位置

    *w = 0;
    *h = 0;
    *xoff = 0;
    *yoff = 0;

    if (r != 0)
    {
        return 1;
    }

    auto newcache = pic_file[fileid].pcache[picid / 2];

    if (newcache->t)    // 已有，则直接显示
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
int RenderTexture(SDL_Texture* lps, int x, int y, int flag, int value, int color, int width, int height, double rotate, SDL_FlipMode reversal, int percent)
{
    SDL_Surface* tmps;
    SDL_FRect rect, rect0;
    int i, j;
    //color = ConvertColor(g_MaskColor32);
    if (value > 255)
    {
        value = 255;
    }
    rect.x = x;
    rect.y = y;
    SDL_GetTextureSize(lps, &rect.w, &rect.h);

    rect.w *= percent / 100.0;
    rect.h *= percent / 100.0;

    if (width > 0 && height > 0)
    {
        rect.w = width;
        rect.h = height;
    }
    else if (width > 0 && height <= 0)
    {
        rect.h = width * rect.h / rect.w;
        rect.w = width;
    }
    rect0 = rect;
    rect0.x = 0;
    rect0.y = 0;
    if (!lps)
    {
        JY_Error("BlitSurface: lps is null!");
        return 1;
    }

    if ((flag & 0x2) == 0)    // 没有alpha
    {
        SDL_SetTextureColorMod(lps, 255, 255, 255);
        SDL_SetTextureBlendMode(lps, SDL_BLENDMODE_BLEND);
        SDL_SetTextureAlphaMod(lps, 255);
        RenderToTexture(lps, NULL, g_Texture, &rect, rotate, NULL, reversal);
        //SDL_BlitSurface(lps, NULL, g_Surface, &rect);
    }
    else    // 有alpha
    {
        if ((flag & 0x4) || (flag & 0x8) || (flag & 0x10))    // 4-黑, 8-白, 16-颜色
        {
            // 4-黑, 8-白, 16-颜色
            if (flag & 0x4)
            {
                SDL_SetTextureColorMod(lps, 32, 32, 32);
                SDL_SetTextureBlendMode(lps, SDL_BLENDMODE_BLEND);
                SDL_SetTextureAlphaMod(lps, (Uint8)value);
                RenderToTexture(lps, NULL, g_Texture, &rect, rotate, NULL, reversal);
            }
            else if (flag & 0x8)
            {
                SDL_SetTextureColorMod(lps, 255, 255, 255);
                SDL_SetTextureBlendMode(lps, SDL_BLENDMODE_NONE);
                SDL_SetTextureAlphaMod(lps, 255);
                RenderToTexture(lps, NULL, g_TextureTmp, &rect, rotate, NULL, reversal);
                SDL_SetTextureBlendMode(lps, SDL_BLENDMODE_ADD);
                SDL_SetRenderDrawColor(g_Renderer, 255, 255, 255, 255);
                SDL_SetRenderDrawBlendMode(g_Renderer, SDL_BLENDMODE_ADD);
                SDL_RenderFillRect(g_Renderer, &rect);
                SDL_SetTextureColorMod(g_TextureTmp, 255, 255, 255);
                SDL_SetTextureBlendMode(g_TextureTmp, SDL_BLENDMODE_BLEND);
                SDL_SetTextureAlphaMod(g_TextureTmp, (Uint8)value);
                RenderToTexture(g_TextureTmp, &rect, g_Texture, &rect, rotate, NULL, reversal);
                SDL_SetTextureAlphaMod(g_TextureTmp, 255);
            }
            else
            {
                Uint8 r = (Uint8)((color & RMASK) >> 16);
                Uint8 g = (Uint8)((color & GMASK) >> 8);
                Uint8 b = (Uint8)((color & BMASK));
                Uint8 a = 255;
                SDL_SetTextureColorMod(lps, 255, 255, 255);
                SDL_SetTextureBlendMode(lps, SDL_BLENDMODE_NONE);
                SDL_SetTextureAlphaMod(lps, 255);
                RenderToTexture(lps, NULL, g_TextureTmp, &rect, rotate, NULL, reversal);
                SDL_SetTextureBlendMode(lps, SDL_BLENDMODE_ADD);
                SDL_SetRenderDrawColor(g_Renderer, r, g, b, a);
                SDL_SetRenderDrawBlendMode(g_Renderer, SDL_BLENDMODE_ADD);
                SDL_RenderFillRect(g_Renderer, &rect);
                SDL_SetTextureColorMod(g_TextureTmp, 255, 255, 255);
                SDL_SetTextureBlendMode(g_TextureTmp, SDL_BLENDMODE_BLEND);
                SDL_SetTextureAlphaMod(g_TextureTmp, (Uint8)value);
                RenderToTexture(g_TextureTmp, &rect, g_Texture, &rect, rotate, NULL, reversal);
                SDL_SetTextureAlphaMod(g_TextureTmp, 255);
                //SDL_SetTextureColorMod(lps, r, g, b);
                //SDL_SetTextureBlendMode(lps, SDL_BLENDMODE_BLEND);
                //SDL_SetTextureAlphaMod(lps, (Uint8)value);
                //RenderToTexture(lps, NULL, g_Texture, &rect);
            }
        }
        else
        {
            SDL_SetTextureColorMod(lps, 255, 255, 255);
            SDL_SetTextureBlendMode(lps, SDL_BLENDMODE_BLEND);
            SDL_SetTextureAlphaMod(lps, (Uint8)value);
            RenderToTexture(lps, NULL, g_Texture, &rect, rotate, NULL, reversal);
            //SDL_BlitSurface(lps, NULL, g_Surface, &rect);
        }
    }
    return 0;
}
