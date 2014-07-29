
// ��ȡidx/grp����ͼ�ļ���
// Ϊ����ٶȣ����û��淽ʽ��ȡ����idx/grp�����ڴ棬Ȼ�������ɸ��������
// �������ʵ�pic���ڻ��������

#include <stdlib.h>
#include "jymain.h"

static struct PicFileCache pic_file[PIC_FILE_NUM];

LIST_HEAD(cache_head);             //����cache����ͷ

static int currentCacheNum = 0;           // ��ǰʹ�õ�cache��

static Uint32 m_color32[256];    // 256��ɫ��

extern int g_MAXCacheNum;                   // ���Cache����
extern Uint32 g_MaskColor32;      // ͸��ɫ

extern SDL_Surface* g_Surface;        // ��Ϸʹ�õ���Ƶ����
extern int g_Rotate;
extern int g_ScreenW ;
extern int g_ScreenH ;

extern int g_PreLoadPicGrp;
extern float g_Zoom;

int g_EnableRLE = 0;

int CacheFailNum = 0;

// ��ʼ��Cache���ݡ���Ϸ��ʼʱ����
int Init_Cache()
{
    int i;
    for (i = 0; i < PIC_FILE_NUM; i++)
    {
        pic_file[i].num = 0;
        pic_file[i].idx = NULL;
        pic_file[i].grp = NULL;
        pic_file[i].fp = NULL;
        pic_file[i].pcache = NULL;
    }
    return 0;
}

// ��ʼ����ͼcache��Ϣ
// PalletteFilename Ϊ256��ɫ���ļ�����һ�ε���ʱ����
//                  Ϊ���ַ������ʾ���������ͼcache��Ϣ��������ͼ/����/ս���л�ʱ����
int JY_PicInit(char* PalletteFilename)
{
    struct list_head* pos, *p;
    int i;
    LoadPallette(PalletteFilename);   //�����ɫ��
    //�������Ϊ�գ�ɾ��ȫ������
    list_for_each_safe(pos, p, &cache_head)
    {
        struct CacheNode* tmp = list_entry(pos, struct CacheNode , list);
        if (tmp->s != NULL)
        {
            SDL_FreeSurface(tmp->s);    //ɾ������
        }
        list_del(pos);
        SafeFree(tmp);
    }
    for (i = 0; i < PIC_FILE_NUM; i++)
    {
        pic_file[i].num = 0;
        SafeFree(pic_file[i].idx);
        SafeFree(pic_file[i].grp);
        SafeFree(pic_file[i].pcache);
        if (pic_file[i].fp)
        {
            fclose(pic_file[i].fp);
            pic_file[i].fp = NULL;
        }
    }
    currentCacheNum = 0;
    CacheFailNum = 0;
    return 0;
}

// �����ļ���Ϣ
// filename �ļ���
// id  0 - PIC_FILE_NUM-1
int JY_PicLoadFile(const char* idxfilename, const char* grpfilename, int id, int width, int height)
{
    int i;
    struct CacheNode* tmpcache;
    FILE* fp;
    if (id < 0 || id >= PIC_FILE_NUM) // id������Χ
    {
        return 1;
    }
    if (pic_file[id].pcache)        //�ͷŵ�ǰ�ļ�ռ�õĿռ䣬������cache
    {
        int i;
        for (i = 0; i < pic_file[id].num; i++) //ѭ��ȫ����ͼ��
        {
            tmpcache = pic_file[id].pcache[i];
            if (tmpcache)       // ����ͼ�л�����ɾ��
            {
                if (tmpcache->s != NULL)
                {
                    SDL_FreeSurface(tmpcache->s);    //ɾ������
                }
                list_del(&tmpcache->list);              //ɾ������
                SafeFree(tmpcache);                  //�ͷ�cache�ڴ�
                currentCacheNum--;
            }
        }
        SafeFree(pic_file[id].pcache);
    }
    SafeFree(pic_file[id].idx);
    SafeFree(pic_file[id].grp);
    if (pic_file[id].fp)
    {
        fclose(pic_file[id].fp);
        pic_file[id].fp = NULL;
    }
    // ��ȡidx�ļ�
    pic_file[id].num = FileLength(idxfilename) / 4; //idx ��ͼ����
    pic_file[id].idx = (int*)malloc((pic_file[id].num + 1) * 4);
    if (pic_file[id].idx == NULL)
    {
        JY_Error("JY_PicLoadFile: cannot malloc idx memory!\n");
        return 1;
    }
    //��ȡ��ͼidx�ļ�
    if ((fp = fopen(idxfilename, "rb")) == NULL)
    {
        JY_Error("JY_PicLoadFile: idx file not open ---%s", idxfilename);
        return 1;
    }
    fread(&pic_file[id].idx[1], 4, pic_file[id].num, fp);
    fclose(fp);
    pic_file[id].idx[0] = 0;
    //��ȡgrp�ļ�
    pic_file[id].filelength = FileLength(grpfilename);
    //��ȡ��ͼgrp�ļ�
    if ((fp = fopen(grpfilename, "rb")) == NULL)
    {
        JY_Error("JY_PicLoadFile: grp file not open ---%s", grpfilename);
        return 1;
    }
    if (g_PreLoadPicGrp == 1) //grp�ļ������ڴ�
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
    pic_file[id].pcache = (struct CacheNode**)malloc(pic_file[id].num * sizeof(struct CacheNode*));
    if (pic_file[id].pcache == NULL)
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
    return 0;
}

int JY_LoadPic(int fileid, int picid, int x, int y, int flag, int value)
{
    return JY_LoadPicColor(fileid, picid, x, y, flag, value, 0);
}

// ���ز���ʾ��ͼ
// fileid        ��ͼ�ļ�id
// picid     ��ͼ���
// x,y       ��ʾλ��
//  flag ��ͬbit����ͬ���壬ȱʡ��Ϊ0
//  B0    0 ����ƫ��xoff��yoff��=1 ������ƫ����
//  B1    0     , 1 �뱳��alpla �����ʾ, value Ϊalphaֵ(0-256), 0��ʾ͸��
//  B2            1 ȫ��
//  B3            1 ȫ��
//  value ����flag���壬Ϊalphaֵ��

int JY_LoadPicColor(int fileid, int picid, int x, int y, int flag, int value, int color)
{
    struct CacheNode* newcache, *tmpcache;
    int xnew, ynew;
    SDL_Surface* tmpsur;
    picid = picid / 2;
    if (fileid < 0 || fileid >= PIC_FILE_NUM || picid < 0 || picid >= pic_file[fileid].num) // ��������
    {
        return 1;
    }
    if (pic_file[fileid].pcache[picid] == NULL) //��ǰ��ͼû�м���
    {
        //����cache����
        newcache = (struct CacheNode*)malloc(sizeof(struct CacheNode));
        if (newcache == NULL)
        {
            JY_Error("JY_LoadPic: cannot malloc newcache memory!\n");
            return 1;
        }
        newcache->id = picid;
        newcache->fileid = fileid;
        LoadPic(fileid, picid, newcache);
        //ָ����Ⱥ͸߶�
        if (newcache->s != NULL && pic_file[fileid].width > 0 && pic_file[fileid].height > 0
            && pic_file[fileid].width != 100 && pic_file[fileid].height != 100)
        {
            double zoomx = (double)pic_file[fileid].width /100;
            double zoomy = (double)pic_file[fileid].height /100;

            if (zoomx < zoomy)
            {
                zoomy = zoomx;
            }
            else
            {
                zoomx = zoomy;
            }
            tmpsur = newcache->s;
            newcache->s = zoomSurface(tmpsur, zoomx, zoomy, 1);
            //newcache->s = SDL_DisplayFormat(tmpsur);
            newcache->xoff = (int)(zoomx * newcache->xoff);
            newcache->yoff = (int)(zoomy * newcache->yoff);
            //SDL_SetColorKey(newcache->s, 1 , ConvertColor(g_MaskColor32)); //͸��ɫ
            SDL_FreeSurface(tmpsur);

        }
        pic_file[fileid].pcache[picid] = newcache;
        if (currentCacheNum < g_MAXCacheNum) //cacheû��
        {
            list_add(&newcache->list , &cache_head);   //���ص���ͷ
            currentCacheNum = currentCacheNum + 1;
        }
        else    //cache ����
        {
            tmpcache = list_entry(cache_head.prev, struct CacheNode , list); //���һ��cache
            pic_file[tmpcache->fileid].pcache[tmpcache->id] = NULL;
            if (tmpcache->s != NULL)
            {
                SDL_FreeSurface(tmpcache->s);    //ɾ������
            }
            list_del(&tmpcache->list);
            SafeFree(tmpcache);
            list_add(&newcache->list , &cache_head);   //���ص���ͷ
            CacheFailNum++;
            if (CacheFailNum % 100 == 1)
            {
                JY_Debug("Pic Cache is full!");
            }
        }
    }
    else    //�Ѽ�����ͼ
    {
        newcache = pic_file[fileid].pcache[picid];
        list_del(&newcache->list);    //�����cache������ժ��
        list_add(&newcache->list , &cache_head);   //�ٲ��뵽��ͷ
    }
    if (newcache->s == NULL) //��ͼΪ�գ�ֱ���˳�
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
    if (g_Rotate == 0)
    {
        BlitSurface(newcache->s , xnew, ynew, flag, value, color);
    }
    else
    {
        BlitSurface(newcache->s , g_ScreenH - ynew - newcache->s->w , xnew, flag, value, color);
    }
    return 0;
}

// ������ͼ������

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
    // ����һЩ��������������޸����еĴ���
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
        //��ȡ��ͼgrp�ļ����õ�ԭʼ����
        if (g_PreLoadPicGrp == 1)       //��Ԥ�������ڴ��ж�����
        {
            data = pic_file[fileid].grp + id1;
            p = NULL;
        }
        else        //û��Ԥ�������ļ��ж�ȡ
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
            cache->s = CreatePicSurface32(data + 8, w, h, datalong - 8);
            //�������������
            /*if (cache->s != NULL && g_Zoom > 1)
            {
                tmpsur = cache->s;

                //cache->s = zoomSurface(tmpsur, g_Zoom, g_Zoom, SMOOTHING_OFF);
                SDL_FreeSurface(tmpsur);

                tmpsur = SDL_DisplayFormat(cache->s);

                SDL_FreeSurface(cache->s);

                cache->s = tmpsur;
                cache->xoff = (int)(g_Zoom * cache->xoff);
                cache->yoff = (int)(g_Zoom * cache->yoff);

                SDL_SetColorKey(cache->s, 1 , ConvertColor(g_MaskColor32)); //͸��ɫ

            }*/
        }
        else       //��ȡpng��ʽ
        {
            tmpsurf = IMG_LoadPNG_RW(fp_SDL);
            if (tmpsurf == NULL)
            {
                JY_Error("LoadPic: cannot create SDL_Surface tmpsurf!\n");
            }
            cache->xoff = tmpsurf->w / 2;
            cache->yoff = tmpsurf->h / 2;
            if (g_Rotate == 0)
            {
                cache->s = tmpsurf;
            }
            else
            {
                cache->s = RotateSurface(tmpsurf);
                SDL_FreeSurface(tmpsurf);
            }
            //�������������
            /*if (cache->s != NULL && g_Zoom > 1)
            {
                tmpsur = cache->s;

                //cache->s = zoomSurface(tmpsur, g_Zoom, g_Zoom, SMOOTHING_OFF);
                SDL_FreeSurface(tmpsur);

                tmpsur = SDL_DisplayFormatAlpha(cache->s);

                SDL_FreeSurface(cache->s);

                cache->s = tmpsur;
                cache->xoff = (int)(g_Zoom * cache->xoff);
                cache->yoff = (int)(g_Zoom * cache->yoff);

                SDL_SetColorKey(cache->s, 1 , ConvertColor(g_MaskColor32)); //͸��ɫ

            }*/
        }
        SDL_FreeRW(fp_SDL);
        SafeFree(p);
    }
    else
    {
        cache->s = NULL;
        cache->xoff = 0;
        cache->yoff = 0;
    }
    return 0;
}


//�õ���ͼ��С
int JY_GetPicXY(int fileid, int picid, int* w, int* h, int* xoff, int* yoff)
{
    struct CacheNode* newcache;
    int r = JY_LoadPic(fileid, picid, g_ScreenW + 1, g_ScreenH + 1, 1, 0); //������ͼ����������λ��
    *w = 0;
    *h = 0;
    *xoff = 0;
    *yoff = 0;
    if (r != 0)
    {
        return 1;
    }
    newcache = pic_file[fileid].pcache[picid / 2];
    if (newcache->s)      // ���У���ֱ����ʾ
    {
        if (g_Rotate == 0)
        {
            *w = newcache->s->w;
            *h = newcache->s->h;
        }
        else
        {
            *w = newcache->s->h;
            *h = newcache->s->w;
        }
        *xoff = newcache->xoff;
        *yoff = newcache->yoff;
    }
    return 0;
}




//����ԭ����Ϸ��RLE��ʽ��������
static SDL_Surface* CreatePicSurface32(unsigned char* data, int w, int h, int datalong)
{
    int p = 0;
    int i, j;
    int yoffset;
    int row;
    int start;
    int x;
    int solidnum;
    SDL_Surface* ps1, *ps2 ;
    Uint32* data32 = NULL;
    data32 = (Uint32*)malloc(w * h * 4);
    if (data32 == NULL)
    {
        JY_Error("CreatePicSurface32: cannot malloc data32 memory!\n");
        return NULL;
    }
    for (i = 0; i < w * h; i++)
    {
        data32[i] = g_MaskColor32;
    }
    for (i = 0; i < h; i++)
    {
        yoffset = i * w;
        row = data[p];          // i�����ݸ���
        start = p;
        p++;
        if (row > 0)
        {
            x = 0;              // i��Ŀǰ��
            for (;;)
            {
                x = x + data[p]; // i�пհ׵����������͸����
                if (x >= w)     // i�п�ȵ�ͷ������
                {
                    break;
                }
                p++;
                solidnum = data[p]; // ��͸�������
                p++;
                for (j = 0; j < solidnum; j++)
                {
                    if (g_Rotate == 0)
                    {
                        data32[yoffset + x] = m_color32[data[p]] | 0xff000000;
                    }
                    else
                    {
                        data32[h - i - 1 + x * h] = m_color32[data[p]] | 0xff000000;
                    }
                    p++;
                    x++;
                }
                if (x >= w)
                {
                    break;    // i�п�ȵ�ͷ������
                }
                if (p - start >= row)
                {
                    break;    // i��û�����ݣ�����
                }
            }
            if (p + 1 >= datalong)
            {
                break;
            }
        }
    }
    if (g_Rotate == 0)
    {
        ps1 = SDL_CreateRGBSurfaceFrom(data32, w, h, 32, w * 4, 0xff0000, 0xff00, 0xff, 0xff000000); //����32λ����
    }
    else
    {
        ps1 = SDL_CreateRGBSurfaceFrom(data32, h, w, 32, h * 4, 0xff0000, 0xff00, 0xff, 0xff000000); //����32λ����
    }
    if (ps1 == NULL)
    {
        JY_Error("CreatePicSurface32: cannot create SDL_Surface ps1!\n");
    }
    ps2 = SDL_DisplayFormat(ps1); // ��32λ�����Ϊ��ǰ����
    if (ps2 == NULL)
    {
        JY_Error("CreatePicSurface32: cannot create SDL_Surface ps2!\n");
    }
    SDL_FreeSurface(ps1);
    SafeFree(data32);
    //SDL_SetColorKey(ps2, 1 , ConvertColor(g_MaskColor32)); //͸��ɫ
    return ps2;
}



// ��ȡ��ɫ��
// �ļ���Ϊ����ֱ�ӷ���
static int LoadPallette(char* filename)
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
        JY_Error("Pallette File not open ---%s", filename);
        return 1;
    }
    for (i = 0; i < 256; i++)
    {
        fread(color, 1, 3, fp);
        m_color32[i] = color[0] * 4 * 65536l + color[1] * 4 * 256 + color[2] * 4 ;
    }
    fclose(fp);
    return 0;
}


int JY_LoadPNGPath(const char* path, int fileid, int num, int width, int height)
{
    int i;
    struct CacheNode* tmpcache;
    if (fileid < 0 || fileid >= PIC_FILE_NUM) // id������Χ
    {
        return 1;
    }
    if (pic_file[fileid].pcache)        //�ͷŵ�ǰ�ļ�ռ�õĿռ䣬������cache
    {
        int i;
        for (i = 0; i < pic_file[fileid].num; i++) //ѭ��ȫ����ͼ��
        {
            tmpcache = pic_file[fileid].pcache[i];
            if (tmpcache)       // ����ͼ�л�����ɾ��
            {
                if (tmpcache->s != NULL)
                {
                    SDL_FreeSurface(tmpcache->s);    //ɾ������
                }
                list_del(&tmpcache->list);              //ɾ������
                SafeFree(tmpcache);                  //�ͷ�cache�ڴ�
                currentCacheNum--;
            }
        }
        SafeFree(pic_file[fileid].pcache);
    }
    pic_file[fileid].num = num;
    sprintf(pic_file[fileid].path, "%s", path);
    pic_file[fileid].pcache = (struct CacheNode**)malloc(pic_file[fileid].num * sizeof(struct CacheNode*));
    if (pic_file[fileid].pcache == NULL)
    {
        JY_Error("JY_LoadPNGPath: cannot malloc pcache memory!\n");
        return 1;
    }
    for (i = 0; i < pic_file[fileid].num; i++)
    {
        pic_file[fileid].pcache[i] = NULL;
    }
    if (height == 0)
    {
        height = width;
    }
    if (width > 0)
    {
        pic_file[fileid].width = width;
        pic_file[fileid].height = height;
    }
    return 0;
}

int JY_LoadPNG(int fileid, int picid, int x, int y, int flag, int value)
{
    struct CacheNode* newcache, *tmpcache;
    SDL_Surface* tmpsur;
    SDL_Rect r;

    picid = picid / 2;
    if (fileid < 0 || fileid >= PIC_FILE_NUM || picid < 0 || picid >= pic_file[fileid].num) // ��������
    {
        return 1;
    }
    if (pic_file[fileid].pcache[picid] == NULL) //��ǰ��ͼû�м���
    {
        char str[512];
        SDL_RWops* fp_SDL;
        sprintf(str, "%s/%d.png", pic_file[fileid].path, picid);
        //����cache����
        newcache = (struct CacheNode*)malloc(sizeof(struct CacheNode));
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
            if (g_Rotate == 0)
            {
                newcache->s = tmpsur;
            }
            else
            {
                newcache->s = RotateSurface(tmpsur);
                SDL_FreeSurface(tmpsur);
            }
        }
        else
        {
            newcache->s = NULL;
            newcache->xoff = 0;
            newcache->yoff = 0;
        }
        SDL_FreeRW(fp_SDL);
        //ָ����Ⱥ͸߶�
        //�����ƺ���û��
        if (newcache->s != NULL && pic_file[fileid].width > 0 && pic_file[fileid].height > 0
            && pic_file[fileid].width != 100 && pic_file[fileid].height != 100)
        {
            double zoomx = (double)pic_file[fileid].width / 100;
            double zoomy = (double)pic_file[fileid].height / 100;
            if (zoomx < zoomy)
            {
                zoomy = zoomx;
            }
            else
            {
                zoomx = zoomy;
            }
            tmpsur = newcache->s;
            //newcache->s = SDL_DisplayFormat(tmpsur);
            newcache->s = zoomSurface(tmpsur, zoomx, zoomy, 1);
            newcache->xoff = (int)(zoomx * newcache->xoff);
            newcache->yoff = (int)(zoomy * newcache->yoff);
            //SDL_SetColorKey(newcache->s, 1 , ConvertColor(g_MaskColor32)); //͸��ɫ
            SDL_FreeSurface(tmpsur);
        }
        /*
        //�������������
        else if (newcache->s != NULL && g_Zoom > 1)
        {
        	tmpsur = newcache->s;

        	newcache->s = zoomSurface(tmpsur, g_Zoom, g_Zoom, SMOOTHING_ON);
        	SDL_FreeSurface(tmpsur);

        	tmpsur=SDL_DisplayFormat(newcache->s);

        	//SDL_FreeSurface(newcache->s);

        	//newcache->s=tmpsur;
        	newcache->xoff = (int)(g_Zoom * newcache->xoff);
        	newcache->yoff = (int)(g_Zoom * newcache->yoff);

        	//SDL_SetColorKey(newcache->s,SDL_SRCCOLORKEY|SDL_RLEACCEL ,ConvertColor(g_MaskColor32));  //͸��ɫ

        }
        */
        pic_file[fileid].pcache[picid] = newcache;
        if (currentCacheNum < g_MAXCacheNum) //cacheû��
        {
            list_add(&newcache->list , &cache_head);   //���ص���ͷ
            currentCacheNum = currentCacheNum + 1;
        }
        else    //cache ����
        {
            tmpcache = list_entry(cache_head.prev, struct CacheNode , list); //���һ��cache
            pic_file[tmpcache->fileid].pcache[tmpcache->id] = NULL;
            if (tmpcache->s != NULL)
            {
                SDL_FreeSurface(tmpcache->s);    //ɾ������
            }
            list_del(&tmpcache->list);
            SafeFree(tmpcache);
            list_add(&newcache->list , &cache_head);   //���ص���ͷ
            CacheFailNum++;
            if (CacheFailNum % 100 == 1)
            {
                JY_Debug("Pic Cache is full!");
            }
        }
    }
    else    //�Ѽ�����ͼ
    {
        newcache = pic_file[fileid].pcache[picid];
        list_del(&newcache->list);    //�����cache������ժ��
        list_add(&newcache->list , &cache_head);   //�ٲ��뵽��ͷ
    }
    if (newcache->s == NULL) //��ͼΪ�գ�ֱ���˳�
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
    if (g_Rotate == 0)
    {
        //BlitSurface(newcache->s , xnew,ynew,flag,value);
    }
    else
    {
        // BlitSurface(newcache->s , g_ScreenH-ynew-newcache->s->w ,xnew,flag,value);
        int t = r.x;
        r.x = r.y;
        r.y = t;
    }
    SDL_BlitSurface(newcache->s, NULL, g_Surface, &r);
    return 0;
}


int JY_GetPNGXY(int fileid, int picid, int* w, int* h, int* xoff, int* yoff)
{
    struct CacheNode* newcache;
    int r = JY_LoadPNG(fileid, picid, g_ScreenW + 1, g_ScreenH + 1, 1, 0); //������ͼ����������λ��
    *w = 0;
    *h = 0;
    *xoff = 0;
    *yoff = 0;
    if (r != 0)
    {
        return 1;
    }
    newcache = pic_file[fileid].pcache[picid / 2];
    if (newcache->s)      // ���У���ֱ����ʾ
    {
        if (g_Rotate == 0)
        {
            *w = newcache->s->w;
            *h = newcache->s->h;
        }
        else
        {
            *w = newcache->s->h;
            *h = newcache->s->w;
        }
        *xoff = newcache->xoff;
        *yoff = newcache->yoff;
    }
    return 0;
}