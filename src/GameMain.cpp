// GameMain.cpp - 游戏主逻辑实现
// 从 jymain.lua 转换而来

#include "GameMain.h"
#include "GameData.h"
#include "jymain.h"
#include "sdlfun.h"
#include "mainmap.h"
#include "piccache.h"
#include "charset.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <string>
#include <vector>
#include <map>
#include <algorithm>

// 前向声明（战斗和事件模块�?
int WarMain(int warid, int isexp);
void WarLoad(int warid);
void ReadKDEF(int eventnum);
void ReadBin();
int War_UseAnqi(int id);

// ============ 全局运行时变量============
static int g_Step = 0;  // 走路动画步数

// ============ 工具函数 ============
void Cls(int x1, int y1, int x2, int y2)
{
    if (x1 == 0 && y1 == 0 && x2 == 0 && y2 == 0)
    {
        x1 = 0; y1 = 0; x2 = g_CC.ScreenW; y2 = g_CC.ScreenH;
    }
    JY_SetClip(x1, y1, x2, y2);
    if (g_JY.Status == GAME_START)
    {
        JY_FillColor(0, 0, 0, 0, 0);
        JY_LoadPicture(g_CC.FirstFile.c_str(), -1, -1);
    }
    else if (g_JY.Status == GAME_MMAP)
    {
        JY_DrawMMap(g_JY.getBase().personX(), g_JY.getBase().personY(), GetMyPic(g_JY.getBase().personDir(), g_Step));
    }
    else if (g_JY.Status == GAME_SMAP)
    {
        DrawSMap(g_JY.SubScene, g_JY.getBase().personX1(), g_JY.getBase().personY1(), g_JY.MyPic);
    }
    else if (g_JY.Status == GAME_WMAP)
    {
        // WarDrawMap(0) handled externally
    }
    else if (g_JY.Status == GAME_DEAD)
    {
        JY_FillColor(0, 0, 0, 0, 0);
        JY_LoadPicture(g_CC.DeadFile.c_str(), -1, -1);
    }
    JY_SetClip(0, 0, g_CC.ScreenW, g_CC.ScreenH);
}

void ShowScreen(int flag)
{
    JY_ShowSurface(flag);
}

void DrawString(int x, int y, const std::string& str, int color, int fontsize)
{
    JY_DrawStr(x, y, str.c_str(), color, fontsize, g_CC.FontName.c_str(), g_CC.SrcCharSet, g_CC.OSCharSet);
}

void MyDrawString(int x1, int x2, int y, const std::string& str, int color, int fontsize)
{
    int len = (int)(str.length() * fontsize / 4);
    int x = (x1 + x2) / 2 - len;
    DrawString(x, y, str, color, fontsize);
}

void DrawBox(int x1, int y1, int x2, int y2, int color)
{
    JY_Background(x1, y1, x2, y2, 192, 0);
    JY_DrawRect(x1, y1, x2, y2, color);
}

void DrawStrBox(int x, int y, const std::string& str, int color, int fontsize)
{
    int len = (int)str.length();
    int w = len * fontsize / 2 + 2 * g_CC.MenuBorderPixel;
    int h = fontsize + 2 * g_CC.MenuBorderPixel;
    if (x < 0) x = (g_CC.ScreenW - w) / 2;
    if (y < 0) y = (g_CC.ScreenH - h) / 2;
    DrawBox(x, y, x + w, y + h, C_WHITE);
    DrawString(x + g_CC.MenuBorderPixel, y + g_CC.MenuBorderPixel, str, color, fontsize);
}

void DrawStrBoxWaitKey(const std::string& str, int color, int fontsize)
{
    Cls();
    DrawStrBox(-1, -1, str, color, fontsize);
    ShowScreen();
    WaitKey();
    Cls();
}

int DrawStrBoxYesNo(const std::string& str, int color, int fontsize)
{
    Cls();
    DrawStrBox(-1, -1, str, color, fontsize);
    ShowScreen();
    while (true)
    {
        int key = WaitKey();
        if (key == GK_Y || key == GK_SPACE || key == GK_RETURN)
        {
            Cls();
            return 1;
        }
        if (key == GK_N || key == GK_ESCAPE)
        {
            Cls();
            return 0;
        }
    }
}

int WaitKey()
{
    while (true)
    {
        int key, type, mx, my;
        JY_GetKey(&key, &type, &mx, &my);
        if (key != 0) return key;
        JY_Delay(10);
    }
}

void PlayMIDI(int id)
{
    if (id >= 0)
    {
        char buf[512];
        snprintf(buf, sizeof(buf), g_CC.MIDIFile.c_str(), id);
        JY_PlayMIDI(buf);
    }
}

void PlayWAV(const std::string& file)
{
    JY_PlayWAV(file.c_str());
}

void PlayWavAtk(int id)
{
    if (id >= 0)
    {
        char buf[512];
        snprintf(buf, sizeof(buf), g_CC.ATKFile.c_str(), id);
        JY_PlayWAV(buf);
    }
}

void PlayWavE(int id)
{
    if (id >= 0)
    {
        char buf[512];
        snprintf(buf, sizeof(buf), g_CC.EFile.c_str(), id);
        JY_PlayWAV(buf);
    }
}

void CleanMemory()
{
    JY_PicInit(g_Config.PaletteFile.c_str());
}

// ============ 人物贴图 ============
int GetMyPic(int direction, int step)
{
    if (direction < 0) direction = g_JY.getBase().personDir();
    int n = g_CC.SceneFlagPic[g_Config.Type] + direction * 2;
    if (step % 2 == 1) n += 8;
    return n;
}

void AddMyCurrentPic(int direction, int step)
{
    g_Step++;
    g_JY.MyPic = GetMyPic(direction, g_Step);
}

// ============ 场景地图辅助 ============
int GetS(int scene, int x, int y, int level)
{
    return JY_GetS(scene, x, y, level);
}

void SetS(int scene, int x, int y, int level, int v)
{
    JY_SetS(scene, x, y, level, v);
}

int GetD(int scene, int id, int datalevel)
{
    return JY_GetD(scene, id, datalevel);
}

void SetD(int scene, int id, int datalevel, int v)
{
    JY_SetD(scene, id, datalevel, v);
}

bool SceneCanPass(int sceneid, int x, int y)
{
    if (x < 0 || x >= g_CC.SWidth || y < 0 || y >= g_CC.SHeight) return false;
    int v = GetS(sceneid, x, y, 1);
    if (v > 0) return false; // 建筑物挡住
    // 检查D*事件是否挡住
    int d = GetS(sceneid, x, y, 3);
    if (d >= 0)
    {
        int v0 = GetD(sceneid, d, 0);
        if (v0 > 0) return false; // 有事件
    }
    // 检查水面
    int ground = GetS(sceneid, x, y, 0);
    if (g_CC.SceneWater.count(ground)) return false;
    return true;
}

void Cal_D_Valid(int sceneid)
{
    g_JY.D_Valid.clear();
    g_JY.D_PicChange.clear();
    // 标记所有有效的D*事件
    for (int i = 0; i < 200; i++)
    {
        int v = GetD(sceneid, i, 0);
        if (v > 0)
        {
            g_JY.D_Valid[i] = 1;
        }
    }
}

void DtoSMap(int sceneid)
{
    if (sceneid < 0) sceneid = g_JY.SubScene;
    // 将D*数据写入S*的第3层和第4层
    for (int i = 0; i < 200; i++)
    {
        int x = GetD(sceneid, i, 9);
        int y = GetD(sceneid, i, 10);
        if (x >= 0 && y >= 0 && x < g_CC.SWidth && y < g_CC.SHeight)
        {
            int v = GetD(sceneid, i, 0);
            if (v > 0)
            {
                SetS(sceneid, x, y, 3, i);
                int pic = GetD(sceneid, i, 5);
                if (pic >= 0)
                {
                    SetS(sceneid, x, y, 4, pic);
                }
            }
        }
    }
}

void DrawSMap(int sceneid, int x, int y, int mypic)
{
    if (sceneid < 0) sceneid = g_JY.SubScene;
    if (x < 0) x = g_JY.getBase().personX1();
    if (y < 0) y = g_JY.getBase().personY1();
    if (mypic < 0) mypic = g_JY.MyPic;
    JY_DrawSMap(sceneid, x, y, g_JY.SubSceneX, g_JY.SubSceneY, mypic);
}

CalcClipRectResult Cal_PicClip(int dx1, int dy1, int pic1, int type1, int dx2, int dy2, int pic2, int type2)
{
    CalcClipRectResult r;
    int xoff1 = g_CC.XScale * (dx1 - dy1) + g_CC.ScreenW / 2;
    int yoff1 = g_CC.YScale * (dx1 + dy1) + g_CC.ScreenH / 2;
    int xoff2 = g_CC.XScale * (dx2 - dy2) + g_CC.ScreenW / 2;
    int yoff2 = g_CC.YScale * (dx2 + dy2) + g_CC.ScreenH / 2;
    int w1, h1, xo1, yo1, w2, h2, xo2, yo2;
    JY_GetPicXY(type1, pic1, &w1, &h1, &xo1, &yo1);
    JY_GetPicXY(type2, pic2, &w2, &h2, &xo2, &yo2);
    r.x1 = std::min(xoff1 + xo1, xoff2 + xo2);
    r.y1 = std::min(yoff1 + yo1, yoff2 + yo2);
    r.x2 = std::max(xoff1 + xo1 + w1, xoff2 + xo2 + w2);
    r.y2 = std::max(yoff1 + yo1 + h1, yoff2 + yo2 + h2);
    return r;
}

CalcClipRectResult* CalcClipRect(CalcClipRectResult r)
{
    static CalcClipRectResult result;
    r.x1 = std::max(0, r.x1);
    r.y1 = std::max(0, r.y1);
    r.x2 = std::min(g_CC.ScreenW, r.x2);
    r.y2 = std::min(g_CC.ScreenH, r.y2);
    if (r.x1 >= r.x2 || r.y1 >= r.y2) return nullptr;
    result = r;
    return &result;
}

CalcClipRectResult MergeRect(CalcClipRectResult a, CalcClipRectResult b)
{
    CalcClipRectResult r;
    r.x1 = std::min(a.x1, b.x1);
    r.y1 = std::min(a.y1, b.y1);
    r.x2 = std::max(a.x2, b.x2);
    r.y2 = std::max(a.y2, b.y2);
    return r;
}

// ============ 人物属性修改============
int AddPersonAttrib(int pid, const std::string& attr, int value)
{
    auto p = g_JY.getPerson(pid);
    int oldv = p.getByName(attr);
    int maxv = 9999;
    auto it = g_CC.PersonAttribMax.find(attr);
    if (it != g_CC.PersonAttribMax.end()) maxv = it->second;
    int newv = limitX(oldv + value, 0, maxv);
    p.setByName(attr, newv);
    return newv - oldv;
}

void AddPersonAttrib_str(int pid, const std::string& attr, int value)
{
    int add = AddPersonAttrib(pid, attr, value);
    (void)add;
}

int TrainNeedExp(int pid)
{
    auto p = g_JY.getPerson(pid);
    int thingid = p.trainItem();
    if (thingid < 0) return 999999;
    auto thing = g_JY.getThing(thingid);
    int wugongid = thing.trainWugong();
    int level = 0;
    if (wugongid >= 0)
    {
        for (int i = 1; i <= 10; i++)
        {
            if (p.wugong(i) == wugongid)
            {
                level = p.wugongLevel(i) / 100 + 1;
                break;
            }
        }
    }
    int needpoint = (7 - p.aptitude() / 15) * (level * 100 + 100);
    return needpoint;
}

// ============ 对话系统 ============
static std::string GenTalkString(const std::string& str, int n)
{
    // 去掉所有
    std::string tmpstr;
    for (char c : str) { if (c != '*') tmpstr += c; }
    
    std::string newstr;
    int pos = 0;
    while (pos < (int)tmpstr.length())
    {
        int w = 0;
        int start = pos;
        while (pos < (int)tmpstr.length())
        {
            unsigned char v = (unsigned char)tmpstr[pos];
            if (v >= 128) { pos += 2; w += 2; } else { pos++; w++; }
            if (w >= 2 * n - 1) break;
        }
        if (pos < (int)tmpstr.length())
            newstr += tmpstr.substr(start, pos - start) + "*";
        else
            newstr += tmpstr.substr(start);
    }
    return newstr;
}

static std::string ReadTalk(int id)
{
    // 读取 talk.idx / talk.grp
    DataBuffer tidx;
    tidx.alloc(id * 4 + 4);
    tidx.loadfile(g_CC.TDX.c_str(), 0, id * 4 + 4);
    int idx1 = (id < 1) ? 0 : tidx.get32((id - 1) * 4);
    int idx2 = tidx.get32(id * 4);
    int len = idx2 - idx1;
    if (len <= 0) return "";
    DataBuffer talk;
    talk.alloc(len);
    talk.loadfile(g_CC.TRP.c_str(), idx1, len);
    std::string str;
    for (int i = 0; i < len - 1; i++)
    {
        int byte = (unsigned char)talk.data[i];
        byte = 255 - (byte % 256);
        str += (char)byte;
    }
    // GBK->UTF8
    char dest[8192];
    JY_CharSet(str.c_str(), dest, 0);
    std::string result = dest;
    result = GenTalkString(result, 12);
    return result;
}

void TalkEx(const std::string& s, int headid, int flag)
{
    int picw = g_Config.Zoom;
    int pich = picw;
    int talkxnum = 12;
    int talkynum = 3;
    int dx = 2, dy = 2;
    int boxpicw = picw + 10, boxpich = pich + 10;
    int boxtalkw = 12 * g_CC.DefaultFont + 10;
    int talkBorder = (pich - talkynum * g_CC.DefaultFont) / (talkynum + 1);

    struct TALKXY { int headx, heady, talkx, talky, showhead; };
    TALKXY xy[6] = {
        {dx, dy, dx + boxpicw + 2, dy, 1},
        {g_CC.ScreenW - 1 - dx - boxpicw, g_CC.ScreenH - dy - boxpich,
         g_CC.ScreenW - 1 - dx - boxpicw - boxtalkw - 2, g_CC.ScreenH - dy - boxpich, 1},
        {dx, dy, dx + boxpicw + 2, dy, 0},
        {g_CC.ScreenW - 1 - dx - boxpicw, g_CC.ScreenH - dy - boxpich,
         g_CC.ScreenW - 1 - dx - boxpicw - boxtalkw - 2, g_CC.ScreenH - dy - boxpich, 1},
        {g_CC.ScreenW - 1 - dx - boxpicw, dy,
         g_CC.ScreenW - 1 - dx - boxpicw - boxtalkw - 2, dy, 1},
        {dx, g_CC.ScreenH - dy - boxpich, dx + boxpicw + 2, g_CC.ScreenH - dy - boxpich, 1},
    };
    if (flag < 0 || flag > 5) flag = 0;
    if (xy[flag].showhead == 0) headid = -1;

    std::string str = GenTalkString(s, talkxnum);

    int key, ktype, mx, my;
    JY_GetKey(&key, &ktype, &mx, &my);

    int startp = 0;
    int dyy = 0;
    while (true)
    {
        if (dyy == 0)
        {
            Cls();
            if (headid >= 0)
            {
                DrawBox(xy[flag].headx, xy[flag].heady, xy[flag].headx + boxpicw, xy[flag].heady + boxpich, C_WHITE);
                int w, h, xoff, yoff;
                JY_GetPNGXY(1, headid * 2, &w, &h, &xoff, &yoff);
                int px = (picw - w) / 2;
                int py = (pich - h) / 2;
                JY_LoadPNG(1, headid * 2, xy[flag].headx + 5 + px, xy[flag].heady + 5 + py, 1, 0, 100);
            }
            DrawBox(xy[flag].talkx, xy[flag].talky, xy[flag].talkx + boxtalkw, xy[flag].talky + boxpich, C_WHITE);
        }
        auto endp = str.find('*', startp);
        if (endp == std::string::npos)
        {
            DrawString(xy[flag].talkx + 5, xy[flag].talky + 5 + talkBorder + dyy * (g_CC.DefaultFont + talkBorder),
                str.substr(startp), C_WHITE, g_CC.DefaultFont);
            ShowScreen();
            WaitKey();
            break;
        }
        else
        {
            DrawString(xy[flag].talkx + 5, xy[flag].talky + 5 + talkBorder + dyy * (g_CC.DefaultFont + talkBorder),
                str.substr(startp, endp - startp), C_WHITE, g_CC.DefaultFont);
        }
        dyy++;
        startp = (int)endp + 1;
        if (dyy >= talkynum)
        {
            ShowScreen();
            WaitKey();
            dyy = 0;
        }
    }
    Cls();
}

// ============ 菜单系统 ============
int ShowMenu(std::vector<MenuItem>& menu, int n, int shownum, int x, int y,
    int face, int d, int flag1, int flag2, int fontsize, int color1, int color2)
{
    if (n <= 0) return 0;
    if (shownum <= 0) shownum = n;

    int maxlen = 0;
    for (int i = 0; i < n; i++)
    {
        int len = (int)menu[i].label.length();
        if (len > maxlen) maxlen = len;
    }
    int itemw = maxlen * fontsize / 2 + 2 * g_CC.MenuBorderPixel;
    int itemh = fontsize + g_CC.RowPixel;
    int boxw = itemw + 5;
    int boxh = shownum * itemh + 2 * g_CC.MenuBorderPixel;

    if (x < 0) x = (g_CC.ScreenW - boxw) / 2;
    if (y < 0) y = (g_CC.ScreenH - boxh) / 2;

    int surid = JY_SaveSur(x, y, boxw, boxh);
    int cur = 0;
    // 找第一个enabled的项
    while (cur < n && menu[cur].enabled == 0) cur++;
    if (cur >= n) cur = 0;
    int top = 0;

    while (true)
    {
        // 绘制菜单
        DrawBox(x, y, x + boxw, y + boxh, C_WHITE);
        for (int i = 0; i < shownum && top + i < n; i++)
        {
            int idx = top + i;
            int cy = y + g_CC.MenuBorderPixel + i * itemh;
            int col = (idx == cur) ? color1 : color2;
            if (menu[idx].enabled == 0) col = RGB_JY(128, 128, 128);
            DrawString(x + g_CC.MenuBorderPixel, cy, menu[idx].label, col, fontsize);
        }
        ShowScreen();

        int key = WaitKey();
        if (key == GK_UP)
        {
            int prev = cur - 1;
            while (prev >= 0 && menu[prev].enabled == 0) prev--;
            if (prev >= 0) { cur = prev; if (cur < top) top = cur; }
        }
        else if (key == GK_DOWN)
        {
            int next = cur + 1;
            while (next < n && menu[next].enabled == 0) next++;
            if (next < n) { cur = next; if (cur >= top + shownum) top = cur - shownum + 1; }
        }
        else if (key == GK_SPACE || key == GK_RETURN)
        {
            if (menu[cur].callback)
            {
                int r = menu[cur].callback(menu, cur);
                if (r != 0)
                {
                    JY_LoadSur(surid, x, y);
                    JY_FreeSur(surid);
                    return cur + 1; // 1-indexed return
                }
            }
            else
            {
                JY_LoadSur(surid, x, y);
                JY_FreeSur(surid);
                return cur + 1;
            }
        }
        else if (key == GK_ESCAPE)
        {
            if (flag1 == 1)
            {
                JY_LoadSur(surid, x, y);
                JY_FreeSur(surid);
                return 0;
            }
        }
    }
}

int ShowMenu2(std::vector<MenuItem>& menu, int n, int y, int fontsize, int color1, int color2)
{
    int totalw = 0;
    for (int i = 0; i < n; i++)
        totalw += (int)menu[i].label.length() * fontsize / 2 + fontsize;
    int x = (g_CC.ScreenW - totalw) / 2;
    int h = fontsize + 2 * g_CC.MenuBorderPixel;

    int surid = JY_SaveSur(0, y, g_CC.ScreenW, h);
    int cur = 0;

    while (true)
    {
        DrawBox(x, y, x + totalw, y + h, C_WHITE);
        int cx = x + g_CC.MenuBorderPixel;
        for (int i = 0; i < n; i++)
        {
            int col = (i == cur) ? color1 : color2;
            if (menu[i].enabled == 0) col = RGB_JY(128, 128, 128);
            DrawString(cx, y + g_CC.MenuBorderPixel, menu[i].label, col, fontsize);
            cx += (int)menu[i].label.length() * fontsize / 2 + fontsize;
        }
        ShowScreen();

        int key = WaitKey();
        if (key == GK_LEFT)
        {
            if (cur > 0) cur--;
        }
        else if (key == GK_RIGHT)
        {
            if (cur < n - 1) cur++;
        }
        else if (key == GK_SPACE || key == GK_RETURN)
        {
            JY_LoadSur(surid, 0, y);
            JY_FreeSur(surid);
            return cur + 1;
        }
        else if (key == GK_ESCAPE)
        {
            JY_LoadSur(surid, 0, y);
            JY_FreeSur(surid);
            return 0;
        }
    }
}

// ============ 记录管理 ============
void LoadRecord(int id)
{
    // 读取 ranger.idx 得到6段偏移
    DataBuffer idx;
    idx.alloc(24);
    idx.loadfile(g_CC.R_IDXFilename[id].c_str(), 0, 24);
    int offset[6];
    for (int i = 0; i < 6; i++) offset[i] = idx.get32(i * 4);

    std::string grpfile = g_CC.R_GRPFilename[id];

    // 加载Base
    g_JY.Data_Base.alloc(g_CC.BaseSize);
    g_JY.Data_Base.loadfile(grpfile.c_str(), offset[0], g_CC.BaseSize);

    // 加载Person
    int personLen = offset[2] - offset[1];
    g_JY.PersonNum = personLen / g_CC.PersonSize;
    g_JY.Data_Person.alloc(personLen);
    g_JY.Data_Person.loadfile(grpfile.c_str(), offset[1], personLen);

    // 加载Thing
    int thingLen = offset[3] - offset[2];
    g_JY.ThingNum = thingLen / g_CC.ThingSize;
    g_JY.Data_Thing.alloc(thingLen);
    g_JY.Data_Thing.loadfile(grpfile.c_str(), offset[2], thingLen);

    // 加载Scene
    int sceneLen = offset[4] - offset[3];
    g_JY.SceneNum = sceneLen / g_CC.SceneSize;
    g_JY.Data_Scene.alloc(sceneLen);
    g_JY.Data_Scene.loadfile(grpfile.c_str(), offset[3], sceneLen);

    // 加载Wugong
    int wugongLen = offset[5] - offset[4];
    g_JY.WugongNum = wugongLen / g_CC.WugongSize;
    g_JY.Data_Wugong.alloc(wugongLen);
    g_JY.Data_Wugong.loadfile(grpfile.c_str(), offset[4], wugongLen);

    // 加载Shop（到文件末尾）
    int shopStart = offset[5];
    // 需要知道文件大小
    FILE* fp = fopen(grpfile.c_str(), "rb");
    if (fp)
    {
        fseek(fp, 0, SEEK_END);
        int fileSize = (int)ftell(fp);
        fclose(fp);
        int shopLen = fileSize - shopStart;
        g_JY.ShopNum = shopLen / g_CC.ShopSize;
        g_JY.Data_Shop.alloc(shopLen);
        g_JY.Data_Shop.loadfile(grpfile.c_str(), shopStart, shopLen);
    }

    // 加载地图
    std::string sfile = g_CC.S_Filename[id];
    std::string dfile = g_CC.D_Filename[id];
    std::string tempS = g_CC.TempS_Filename;
    JY_LoadSMap(sfile.c_str(), tempS.c_str(), g_JY.SceneNum, g_CC.SWidth, g_CC.SHeight,
        dfile.c_str(), g_CC.DNum, g_CC.DNum2);

    JY_Debug("LoadRecord %d ok", id);
}

void SaveRecord(int id)
{
    if (id == 0) return; // 不保存初始存档

    // 先保存地图
    JY_SaveSMap(g_CC.S_Filename[id].c_str(), g_CC.D_Filename[id].c_str());

    // 生成idx
    DataBuffer idx;
    idx.alloc(24);
    int pos = 0;
    int sizes[6];
    sizes[0] = g_CC.BaseSize;
    sizes[1] = g_JY.PersonNum * g_CC.PersonSize;
    sizes[2] = g_JY.ThingNum * g_CC.ThingSize;
    sizes[3] = g_JY.SceneNum * g_CC.SceneSize;
    sizes[4] = g_JY.WugongNum * g_CC.WugongSize;
    sizes[5] = g_JY.ShopNum * g_CC.ShopSize;
    for (int i = 0; i < 6; i++)
    {
        idx.set32(i * 4, pos);
        pos += sizes[i];
    }
    idx.savefile(g_CC.R_IDXFilename[id].c_str(), 0, 24);

    // 写入grp
    std::string grpfile = g_CC.R_GRPFilename[id];
    FILE* fp = fopen(grpfile.c_str(), "wb");
    if (fp)
    {
        fwrite(g_JY.Data_Base.data, 1, sizes[0], fp);
        fwrite(g_JY.Data_Person.data, 1, sizes[1], fp);
        fwrite(g_JY.Data_Thing.data, 1, sizes[2], fp);
        fwrite(g_JY.Data_Scene.data, 1, sizes[3], fp);
        fwrite(g_JY.Data_Wugong.data, 1, sizes[4], fp);
        fwrite(g_JY.Data_Shop.data, 1, sizes[5], fp);
        fclose(fp);
    }
    JY_Debug("SaveRecord %d ok", id);
}

// ============ 事件指令 ============
bool instruct_18(int thingid)
{
    auto base = g_JY.getBase();
    for (int i = 1; i <= g_CC.MyThingNum; i++)
    {
        if (base.item(i) == thingid) return true;
    }
    return false;
}

void instruct_2(int thingid, int num)
{
    if (thingid < 0 || thingid >= g_JY.ThingNum) return;
    instruct_32(thingid, num);
    auto thing = g_JY.getThing(thingid);
    char buf[256];
    snprintf(buf, sizeof(buf), "得到物品:%s %d", thing.name().c_str(), num);
    DrawStrBoxWaitKey(buf, C_ORANGE, g_CC.DefaultFont);
}

void instruct_32(int thingid, int addnum)
{
    auto base = g_JY.getBase();
    int p = 0;
    for (int i = 1; i <= g_CC.MyThingNum; i++)
    {
        if (base.item(i) == thingid)
        {
            base.setItemNum(i, base.itemNum(i) + addnum);
            p = i;
            break;
        }
        else if (base.item(i) == -1)
        {
            base.setItem(i, thingid);
            base.setItemNum(i, addnum);
            p = i;
            break;
        }
    }
    if (p > 0 && base.itemNum(p) <= 0)
    {
        for (int i = p + 1; i <= g_CC.MyThingNum; i++)
        {
            base.setItem(i - 1, base.item(i));
            base.setItemNum(i - 1, base.itemNum(i));
        }
        base.setItem(g_CC.MyThingNum, -1);
        base.setItemNum(g_CC.MyThingNum, 0);
    }
}

void instruct_41(int personid, int thingid, int addnum)
{
    auto p = g_JY.getPerson(personid);
    int k = 0;
    for (int i = 1; i <= 4; i++)
    {
        if (p.carryItem(i) == thingid)
        {
            p.setCarryItemNum(i, p.carryItemNum(i) + addnum);
            k = i;
            break;
        }
    }
    if (k > 0 && p.carryItemNum(k) <= 0)
    {
        for (int i = k + 1; i <= 4; i++)
        {
            p.setCarryItem(i - 1, p.carryItem(i));
            p.setCarryItemNum(i - 1, p.carryItemNum(i));
        }
        p.setCarryItem(4, -1);
        p.setCarryItemNum(4, 0);
    }
    if (k == 0)
    {
        for (int i = 1; i <= 4; i++)
        {
            if (p.carryItem(i) == -1)
            {
                p.setCarryItem(i, thingid);
                p.setCarryItemNum(i, addnum);
                break;
            }
        }
    }
}

void instruct_3(int sceneid, int id, int v0, int v1, int v2, int v3, int v4,
    int v5, int v6, int v7, int v8, int v9, int v10)
{
    if (sceneid == -2) sceneid = g_JY.SubScene;
    if (id == -2) id = g_JY.CurrentD;
    if (v0 != -2) SetD(sceneid, id, 0, v0);
    if (v1 != -2) SetD(sceneid, id, 1, v1);
    if (v2 != -2) SetD(sceneid, id, 2, v2);
    if (v3 != -2) SetD(sceneid, id, 3, v3);
    if (v4 != -2) SetD(sceneid, id, 4, v4);
    if (v5 != -2) SetD(sceneid, id, 5, v5);
    if (v6 != -2) SetD(sceneid, id, 6, v6);
    if (v7 != -2) SetD(sceneid, id, 7, v7);
    if (v8 != -2) SetD(sceneid, id, 8, v8);
    if (v9 != -2 && v10 != -2)
    {
        if (v9 > 0 && v10 > 0)
        {
            SetS(sceneid, GetD(sceneid, id, 9), GetD(sceneid, id, 10), 3, -1);
            SetD(sceneid, id, 9, v9);
            SetD(sceneid, id, 10, v10);
            SetS(sceneid, GetD(sceneid, id, 9), GetD(sceneid, id, 10), 3, id);
        }
    }
}

bool instruct_4(int thingid) { return g_JY.CurrentThing == thingid; }
bool instruct_5() { return DrawStrBoxYesNo("是否与之过招(Y/N)?", C_ORANGE, g_CC.DefaultFont) != 0; }
void instruct_8(int musicid) { g_JY.MmapMusic = musicid; }
bool instruct_9() { Cls(); return DrawStrBoxYesNo("是否要求加入(Y/N)?", C_ORANGE, g_CC.DefaultFont) != 0; }

void instruct_10(int personid)
{
    int add = 0;
    auto base = g_JY.getBase();
    for (int i = 2; i <= g_CC.TeamNum; i++)
    {
        if (base.team(i) < 0) { base.setTeam(i, personid); add = 1; break; }
    }
    if (add == 0) return;
    auto p = g_JY.getPerson(personid);
    for (int i = 1; i <= 4; i++)
    {
        int id = p.carryItem(i);
        int n = p.carryItemNum(i);
        if (id >= 0 && n > 0)
        {
            instruct_2(id, n);
            p.setCarryItem(i, -1);
            p.setCarryItemNum(i, 0);
        }
    }
}

bool instruct_11() { Cls(); return DrawStrBoxYesNo("是否住宿(Y/N)?", C_ORANGE, g_CC.DefaultFont) != 0; }

void instruct_12()
{
    auto base = g_JY.getBase();
    for (int i = 1; i <= g_CC.TeamNum; i++)
    {
        int id = base.team(i);
        if (id >= 0)
        {
            auto p = g_JY.getPerson(id);
            if (p.getByName("受伤程度") < 33 && p.getByName("中毒程度") <= 0)
            {
                p.setByName("受伤程度", 0);
                AddPersonAttrib(id, "体力", 999999);
                AddPersonAttrib(id, "生命", 999999);
                AddPersonAttrib(id, "内力", 999999);
            }
        }
    }
}

void instruct_13() { Cls(); g_JY.Darkness = 0; JY_ShowSlow(50, 0); int k, t, mx, my; JY_GetKey(&k, &t, &mx, &my); }
void instruct_14() { JY_ShowSlow(50, 1); g_JY.Darkness = 1; }

void instruct_15()
{
    g_JY.Status = GAME_DEAD;
    Cls();
    DrawString(g_CC.GameOverX, g_CC.GameOverY, g_JY.getPerson(0).name(), RGB_JY(0, 0, 0), g_CC.DefaultFont);
    std::vector<MenuItem> loadMenu = {
        {"载入进度一", nullptr, 1}, {"载入进度", nullptr, 1},
        {"载入进度", nullptr, 1}, {"回家睡觉", nullptr, 1}
    };
    int y = g_CC.ScreenH - 4 * (g_CC.DefaultFont + g_CC.RowPixel) - 10;
    int x = g_CC.ScreenW - 9 * g_CC.DefaultFont;
    int r = ShowMenu(loadMenu, 4, 0, x, y, 0, 0, 0, 0, g_CC.DefaultFont, C_ORANGE, C_WHITE);
    if (r > 0 && r < 4) { LoadRecord(r); g_JY.Status = GAME_FIRSTMMAP; }
    else g_JY.Status = GAME_END;
}

bool instruct_16(int personid)
{
    auto base = g_JY.getBase();
    for (int i = 1; i <= g_CC.TeamNum; i++)
        if (base.team(i) == personid) return true;
    return false;
}

void instruct_17(int sceneid, int level, int x, int y, int v)
{
    if (sceneid == -2) sceneid = g_JY.SubScene;
    SetS(sceneid, x, y, level, v);
}

void instruct_19(int x, int y)
{
    g_JY.getBase().setPersonX1(x);
    g_JY.getBase().setPersonY1(y);
    g_JY.SubSceneX = 0;
    g_JY.SubSceneY = 0;
}

bool instruct_20() { return g_JY.getBase().team(g_CC.TeamNum) >= 0; }

void instruct_21(int personid)
{
    auto base = g_JY.getBase();
    int j = 0;
    for (int i = 1; i <= g_CC.TeamNum; i++)
        if (base.team(i) == personid) { j = i; break; }
    if (j == 0) return;
    for (int i = j + 1; i <= g_CC.TeamNum; i++)
        base.setTeam(i - 1, base.team(i));
    base.setTeam(g_CC.TeamNum, -1);
    auto p = g_JY.getPerson(personid);
    if (p.weapon() >= 0) { g_JY.getThing(p.weapon()).setByName("使用人", -1); p.setWeapon(-1); }
    if (p.armor() >= 0) { g_JY.getThing(p.armor()).setByName("使用人", -1); p.setArmor(-1); }
    if (p.trainItem() >= 0) { g_JY.getThing(p.trainItem()).setByName("使用人", -1); p.setTrainItem(-1); }
    p.setByName("修炼点数", 0);
    p.setByName("物品修炼点数", 0);
}

void instruct_22()
{
    auto base = g_JY.getBase();
    for (int i = 1; i <= g_CC.TeamNum; i++)
        if (base.team(i) >= 0) g_JY.getPerson(base.team(i)).setByName("内力", 0);
}

void instruct_23(int personid, int value)
{
    g_JY.getPerson(personid).setByName("用毒能力", value);
    AddPersonAttrib(personid, "用毒能力", 0);
}

void instruct_25(int x1, int y1, int x2, int y2)
{
    if (y1 != y2)
    {
        int sign = (y2 < y1) ? -1 : 1;
        for (int i = y1 + sign; ; i += sign)
        {
            double t1 = JY_GetTime();
            g_JY.SubSceneY += sign;
            DrawSMap();
            ShowScreen();
            double t2 = JY_GetTime();
            if (t2 - t1 < g_CC.SceneMoveFrame) JY_Delay((int)(g_CC.SceneMoveFrame - (t2 - t1)));
            if (i == y2) break;
        }
    }
    if (x1 != x2)
    {
        int sign = (x2 < x1) ? -1 : 1;
        for (int i = x1 + sign; ; i += sign)
        {
            double t1 = JY_GetTime();
            g_JY.SubSceneX += sign;
            DrawSMap();
            ShowScreen();
            double t2 = JY_GetTime();
            if (t2 - t1 < g_CC.SceneMoveFrame) JY_Delay((int)(g_CC.SceneMoveFrame - (t2 - t1)));
            if (i == x2) break;
        }
    }
}

void instruct_26(int sceneid, int id, int v1, int v2, int v3)
{
    if (sceneid == -2) sceneid = g_JY.SubScene;
    SetD(sceneid, id, 2, GetD(sceneid, id, 2) + v1);
    SetD(sceneid, id, 3, GetD(sceneid, id, 3) + v2);
    SetD(sceneid, id, 4, GetD(sceneid, id, 4) + v3);
}

void instruct_27(int id, int startpic, int endpic)
{
    int old1 = 0, old2 = 0, old3 = 0;
    if (id != -1) { old1 = GetD(g_JY.SubScene, id, 5); old2 = GetD(g_JY.SubScene, id, 6); old3 = GetD(g_JY.SubScene, id, 7); }
    for (int i = startpic; i <= endpic; i += 2)
    {
        double t1 = JY_GetTime();
        if (id == -1) g_JY.MyPic = i / 2;
        else { SetD(g_JY.SubScene, id, 5, i); SetD(g_JY.SubScene, id, 6, i); SetD(g_JY.SubScene, id, 7, i); }
        DtoSMap();
        DrawSMap();
        ShowScreen();
        double t2 = JY_GetTime();
        if (t2 - t1 < g_CC.AnimationFrame) JY_Delay((int)(g_CC.AnimationFrame - (t2 - t1)));
    }
    if (id != -1) { SetD(g_JY.SubScene, id, 5, old1); SetD(g_JY.SubScene, id, 6, old2); SetD(g_JY.SubScene, id, 7, old3); }
}

bool instruct_28(int personid, int vmin, int vmax)
{
    int v = g_JY.getPerson(personid).getByName("品德");
    return v >= vmin && v <= vmax;
}

bool instruct_29(int personid, int vmin, int vmax)
{
    int v = g_JY.getPerson(personid).getByName("攻击力");
    return v >= vmin && v <= vmax;
}

static void instruct_30_sub(int direct)
{
    AddMyCurrentPic();
    int x = g_JY.getBase().personX1() + g_CC.DirectX[direct];
    int y = g_JY.getBase().personY1() + g_CC.DirectY[direct];
    g_JY.getBase().setPersonDir(direct);
    g_JY.MyPic = GetMyPic();
    DtoSMap();
    if (SceneCanPass(g_JY.SubScene, x, y))
    {
        g_JY.getBase().setPersonX1(x);
        g_JY.getBase().setPersonY1(y);
    }
    g_JY.getBase().setPersonX1(limitX(g_JY.getBase().personX1(), 1, g_CC.SWidth - 2));
    g_JY.getBase().setPersonY1(limitX(g_JY.getBase().personY1(), 1, g_CC.SHeight - 2));
    DrawSMap();
    ShowScreen();
}

void instruct_30(int x1, int y1, int x2, int y2)
{
    auto move = [](int from, int to, int dir) {
        if (from < to) for (int i = from + 1; i <= to; i++) { double t1 = JY_GetTime(); instruct_30_sub(dir); double t2 = JY_GetTime(); if (t2 - t1 < g_CC.PersonMoveFrame) JY_Delay((int)(g_CC.PersonMoveFrame - (t2 - t1))); }
        else if (from > to) for (int i = to + 1; i <= from; i++) { double t1 = JY_GetTime(); instruct_30_sub(dir == 1 ? 2 : (dir == 3 ? 0 : dir)); double t2 = JY_GetTime(); if (t2 - t1 < g_CC.PersonMoveFrame) JY_Delay((int)(g_CC.PersonMoveFrame - (t2 - t1))); }
    };
    if (x1 < x2) for (int i = x1 + 1; i <= x2; i++) { double t1 = JY_GetTime(); instruct_30_sub(1); double t2 = JY_GetTime(); if (t2 - t1 < g_CC.PersonMoveFrame) JY_Delay((int)(g_CC.PersonMoveFrame - (t2 - t1))); }
    else if (x1 > x2) for (int i = x2 + 1; i <= x1; i++) { double t1 = JY_GetTime(); instruct_30_sub(2); double t2 = JY_GetTime(); if (t2 - t1 < g_CC.PersonMoveFrame) JY_Delay((int)(g_CC.PersonMoveFrame - (t2 - t1))); }
    if (y1 < y2) for (int i = y1 + 1; i <= y2; i++) { double t1 = JY_GetTime(); instruct_30_sub(3); double t2 = JY_GetTime(); if (t2 - t1 < g_CC.PersonMoveFrame) JY_Delay((int)(g_CC.PersonMoveFrame - (t2 - t1))); }
    else if (y1 > y2) for (int i = y2 + 1; i <= y1; i++) { double t1 = JY_GetTime(); instruct_30_sub(0); double t2 = JY_GetTime(); if (t2 - t1 < g_CC.PersonMoveFrame) JY_Delay((int)(g_CC.PersonMoveFrame - (t2 - t1))); }
}

bool instruct_31(int num)
{
    auto base = g_JY.getBase();
    for (int i = 1; i <= g_CC.MyThingNum; i++)
    {
        if (base.item(i) == g_CC.MoneyID)
            return base.itemNum(i) >= num;
    }
    return false;
}

void instruct_33(int personid, int wugongid, int flag)
{
    auto p = g_JY.getPerson(personid);
    int add = 0;
    for (int i = 1; i <= 10; i++)
    {
        if (p.wugong(i) == 0) { p.setWugong(i, wugongid); p.setWugongLevel(i, 0); add = 1; break; }
    }
    if (add == 0) { p.setWugong(10, wugongid); p.setWugongLevel(10, 0); }
    if (flag == 0)
    {
        char buf[256];
        snprintf(buf, sizeof(buf), "%s 学会武功 %s", p.name().c_str(), g_JY.getWugong(wugongid).name().c_str());
        DrawStrBoxWaitKey(buf, C_ORANGE, g_CC.DefaultFont);
    }
}

void instruct_35(int personid, int id, int wugongid, int wugonglevel)
{
    auto p = g_JY.getPerson(personid);
    if (id >= 0) { p.setWugong(id + 1, wugongid); p.setWugongLevel(id + 1, wugonglevel); }
    else
    {
        for (int i = 1; i <= 10; i++)
        {
            if (p.wugong(i) == 0) { p.setWugong(i, wugongid); p.setWugongLevel(i, wugonglevel); return; }
        }
        p.setWugong(1, wugongid); p.setWugongLevel(1, wugonglevel);
    }
}

bool instruct_36(int sex) { return g_JY.getPerson(0).getByName("性别") == sex; }
void instruct_37(int v) { AddPersonAttrib(0, "品德", v); }

void instruct_38(int sceneid, int level, int oldpic, int newpic)
{
    if (sceneid == -2) sceneid = g_JY.SubScene;
    for (int i = 0; i < g_CC.SWidth; i++)
        for (int j = 0; j < g_CC.SHeight; j++)
            if (GetS(sceneid, i, j, level) == oldpic)
                SetS(sceneid, i, j, level, newpic);
}

void instruct_39(int sceneid) { g_JY.getScene(sceneid).setByName("进入条件", 0); }
void instruct_40(int v) { g_JY.getBase().setPersonDir(v); g_JY.MyPic = GetMyPic(); }

bool instruct_42()
{
    auto base = g_JY.getBase();
    for (int i = 1; i <= g_CC.TeamNum; i++)
        if (base.team(i) >= 0 && g_JY.getPerson(base.team(i)).getByName("性别") == 1) return true;
    return false;
}

bool instruct_43(int thingid) { return instruct_18(thingid); }

void instruct_44(int id1, int sp1, int ep1, int id2, int sp2, int ep2)
{
    int old[6];
    old[0] = GetD(g_JY.SubScene, id1, 5); old[1] = GetD(g_JY.SubScene, id1, 6); old[2] = GetD(g_JY.SubScene, id1, 7);
    old[3] = GetD(g_JY.SubScene, id2, 5); old[4] = GetD(g_JY.SubScene, id2, 6); old[5] = GetD(g_JY.SubScene, id2, 7);
    for (int i = sp1; i <= ep1; i += 2)
    {
        double t1 = JY_GetTime();
        if (id1 == -1) g_JY.MyPic = i / 2;
        else { SetD(g_JY.SubScene, id1, 5, i); SetD(g_JY.SubScene, id1, 6, i); SetD(g_JY.SubScene, id1, 7, i); }
        if (id2 == -1) g_JY.MyPic = i / 2;
        else { int j = i - sp1 + sp2; SetD(g_JY.SubScene, id2, 5, j); SetD(g_JY.SubScene, id2, 6, j); SetD(g_JY.SubScene, id2, 7, j); }
        DtoSMap();
        DrawSMap();
        ShowScreen();
        double t2 = JY_GetTime();
        if (t2 - t1 < g_CC.AnimationFrame) JY_Delay((int)(g_CC.AnimationFrame - (t2 - t1)));
    }
    SetD(g_JY.SubScene, id1, 5, old[0]); SetD(g_JY.SubScene, id1, 6, old[1]); SetD(g_JY.SubScene, id1, 7, old[2]);
    SetD(g_JY.SubScene, id2, 5, old[3]); SetD(g_JY.SubScene, id2, 6, old[4]); SetD(g_JY.SubScene, id2, 7, old[5]);
}

bool instruct_50(int id1, int id2, int id3, int id4, int id5)
{
    int n = 0;
    if (instruct_18(id1)) n++; if (instruct_18(id2)) n++;
    if (instruct_18(id3)) n++; if (instruct_18(id4)) n++;
    if (instruct_18(id5)) n++;
    return n == 5;
}

bool instruct_55(int id, int num) { return GetD(g_JY.SubScene, id, 2) == num; }
void instruct_56(int v) { g_JY.getPerson(0).setByName("声望", g_JY.getPerson(0).getByName("声望") + v); }
bool instruct_60(int sceneid, int id, int num)
{
    if (sceneid == -2) sceneid = g_JY.SubScene;
    if (id == -2) id = g_JY.CurrentD;
    return GetD(sceneid, id, 5) == num;
}

void instruct_63(int personid, int sex) { g_JY.getPerson(personid).setByName("性别", sex); }
void instruct_66(int id) { PlayMIDI(id); }
void instruct_67(int id) { PlayWavAtk(id); }

// ============ 物品使用 ============
int UseThingEffect(int thingid, int pid)
{
    auto thing = g_JY.getThing(thingid);
    int changed = 0;
    std::vector<std::string> msgs;
    char buf[256];
    snprintf(buf, sizeof(buf), "使用 %s", thing.name().c_str());
    msgs.push_back(buf);

    auto tryAdd = [&](const std::string& attr) {
        int addv = thing.getByName("加" + attr);
        if (addv != 0)
        {
            int actual = AddPersonAttrib(pid, attr, addv);
            if (actual != 0)
            {
                snprintf(buf, sizeof(buf), " %s %+d", attr.c_str(), actual);
                msgs.push_back(buf);
                changed = 1;
            }
        }
    };

    // 加生命（特殊处理受伤）
    int addLife = thing.getByName("加生命");
    if (addLife > 0)
    {
        int injury = g_JY.getPerson(pid).getByName("受伤程度");
        int add = addLife - injury / 2 + Rnd(10);
        if (add <= 0) add = 5 + Rnd(5);
        AddPersonAttrib(pid, "受伤程度", -addLife / 4);
        int actual = AddPersonAttrib(pid, "生命", add);
        if (actual != 0) { snprintf(buf, sizeof(buf), " 生命 %+d", actual); msgs.push_back(buf); changed = 1; }
    }

    tryAdd("生命最大值");

    int decPoison = thing.getByName("加中毒解药");
    if (decPoison < 0)
    {
        int actual = AddPersonAttrib(pid, "中毒程度", decPoison / 2);
        if (actual != 0) { snprintf(buf, sizeof(buf), " 中毒程度 %+d", actual); msgs.push_back(buf); changed = 1; }
    }

    tryAdd("体力");
    if (thing.getByName("改变内力性质") == 2) { g_JY.getPerson(pid).setByName("内力性质", 2); msgs.push_back("内力门路改为阴阳合一"); changed = 1; }
    tryAdd("内力"); tryAdd("内力最大值"); tryAdd("攻击力"); tryAdd("防御力"); tryAdd("轻功");
    tryAdd("医疗能力"); tryAdd("用毒能力"); tryAdd("解毒能力"); tryAdd("抗毒能力");
    tryAdd("拳掌功夫"); tryAdd("御剑能力"); tryAdd("耍刀技巧"); tryAdd("特殊兵器");
    tryAdd("暗器技巧"); tryAdd("武学常识"); tryAdd("攻击带毒");

    if (msgs.size() > 1)
    {
        Cls();
        int maxlen = 0;
        for (auto& s : msgs) if ((int)s.length() > maxlen) maxlen = (int)s.length();
        int ww = maxlen * g_CC.DefaultFont / 2 + g_CC.MenuBorderPixel * 2;
        int hh = (int)msgs.size() * g_CC.DefaultFont + ((int)msgs.size() - 1) * g_CC.RowPixel + 2 * g_CC.MenuBorderPixel;
        int x = (g_CC.ScreenW - ww) / 2;
        int y = (g_CC.ScreenH - hh) / 2;
        DrawBox(x, y, x + ww, y + hh, C_WHITE);
        DrawString(x + g_CC.MenuBorderPixel, y + g_CC.MenuBorderPixel, msgs[0], C_WHITE, g_CC.DefaultFont);
        for (int i = 1; i < (int)msgs.size(); i++)
            DrawString(x + g_CC.MenuBorderPixel, y + g_CC.MenuBorderPixel + (g_CC.DefaultFont + g_CC.RowPixel) * i, msgs[i], C_ORANGE, g_CC.DefaultFont);
        ShowScreen();
        return 1;
    }
    return 0;
}

int UseThing(int thingid, int personid)
{
    auto thing = g_JY.getThing(thingid);
    int type = thing.type();

    if (type == 3) // 药品
    {
        if (personid < 0)
        {
            // 选择使用对象
            int r = SelectTeamMenu(0);
            if (r <= 0) return 0;
            personid = g_JY.getBase().team(r);
        }
        if (personid < 0) return 0;
        if (UseThingEffect(thingid, personid) == 1)
        {
            instruct_32(thingid, -1);
            WaitKey();
            return 1;
        }
        return 0;
    }
    else if (type == 4) // 暗器
    {
        if (g_JY.Status == GAME_WMAP)
            return War_UseAnqi(thingid);
        return 0;
    }
    return 0;
}

int SelectThing(int* thing, int* thingnum)
{
    // 简单选择物品菜单
    std::vector<MenuItem> menu;
    int count = 0;
    for (int i = 0; i < g_CC.MyThingNum && thing[i] >= 0; i++)
    {
        auto t = g_JY.getThing(thing[i]);
        char buf[256];
        snprintf(buf, sizeof(buf), "%-12s x%d", t.name().c_str(), thingnum[i]);
        menu.push_back({buf, nullptr, 1});
        count++;
    }
    if (count == 0) return -1;
    int r = ShowMenu(menu, count, 0, -1, -1, 0, 0, 1, 1, g_CC.DefaultFont, C_ORANGE, C_WHITE);
    if (r > 0) return thing[r - 1];
    return -1;
}

// ============ 医疗/解毒 ============
int ExecDoctor(int doctorid, int patientid)
{
    auto doctor = g_JY.getPerson(doctorid);
    auto patient = g_JY.getPerson(patientid);
    int ability = doctor.getByName("医疗能力");
    int injury = patient.getByName("受伤程度");
    int heal = ability - injury + Rnd(10);
    if (heal <= 0) heal = 5 + Rnd(5);
    AddPersonAttrib(patientid, "受伤程度", -ability / 4);
    return AddPersonAttrib(patientid, "生命", heal);
}

int ExecDecPoison(int doctorid, int patientid)
{
    auto doctor = g_JY.getPerson(doctorid);
    int ability = doctor.getByName("解毒能力");
    int v = -(ability / 2 + Rnd(5));
    return AddPersonAttrib(patientid, "中毒程度", v);
}

// ============ 菜单处理 ============
int SelectTeamMenu(int flag)
{
    auto base = g_JY.getBase();
    std::vector<MenuItem> menu;
    for (int i = 1; i <= g_CC.TeamNum; i++)
    {
        int id = base.team(i);
        if (id >= 0)
            menu.push_back({g_JY.getPerson(id).name(), nullptr, 1});
        else
            menu.push_back({"", nullptr, 0});
    }
    int count = (int)menu.size();
    return ShowMenu(menu, count, 0, g_CC.MainSubMenuX, g_CC.MainSubMenuY, 0, 0, 1, 1, g_CC.DefaultFont, C_ORANGE, C_WHITE);
}

void Menu_Status()
{
    int r = SelectTeamMenu(0);
    if (r > 0)
    {
        int pid = g_JY.getBase().team(r);
        if (pid >= 0) ShowPersonStatus(pid);
    }
}

void ShowPersonStatus(int pid)
{
    auto p = g_JY.getPerson(pid);
    Cls();
    int x = 20, y = 20;
    int fs = g_CC.DefaultFont;
    int rp = g_CC.RowPixel;
    char buf[256];

    // 头像
    int w, h, xoff, yoff;
    JY_GetPNGXY(1, p.headId() * 2, &w, &h, &xoff, &yoff);
    JY_LoadPNG(1, p.headId() * 2, x, y, 1, 0, 100);

    int tx = x + w + 20;
    snprintf(buf, sizeof(buf), "%s  等级:%d  经验:%d", p.name().c_str(), p.level(), (int)p.exp());
    DrawString(tx, y, buf, C_WHITE, fs); y += fs + rp;

    snprintf(buf, sizeof(buf), "生命 %d/%d  内力 %d/%d  体力 %d", p.hp(), p.maxHp(), p.mp(), p.maxMp(), p.stamina());
    DrawString(tx, y + fs + rp, buf, C_ORANGE, fs);

    snprintf(buf, sizeof(buf), "攻击 %d  防御 %d  轻功 %d", p.attack(), p.defense(), p.agility());
    DrawString(tx, y + 2 * (fs + rp), buf, C_ORANGE, fs);

    ShowScreen();
    WaitKey();
    Cls();
}

void Menu_Doctor()
{
    int r = SelectTeamMenu(0);
    if (r <= 0) return;
    int doctorid = g_JY.getBase().team(r);
    if (doctorid < 0 || g_JY.getPerson(doctorid).getByName("医疗能力") < 20) return;
    int r2 = SelectTeamMenu(0);
    if (r2 <= 0) return;
    int patientid = g_JY.getBase().team(r2);
    if (patientid < 0) return;
    int heal = ExecDoctor(doctorid, patientid);
    AddPersonAttrib(doctorid, "体力", -5);
    char buf[256];
    snprintf(buf, sizeof(buf), "%s 医疗 %s, 恢复 %d 点生命", g_JY.getPerson(doctorid).name().c_str(), g_JY.getPerson(patientid).name().c_str(), heal);
    DrawStrBoxWaitKey(buf, C_ORANGE, g_CC.DefaultFont);
}

void Menu_DecPoison()
{
    int r = SelectTeamMenu(0);
    if (r <= 0) return;
    int doctorid = g_JY.getBase().team(r);
    if (doctorid < 0 || g_JY.getPerson(doctorid).getByName("解毒能力") < 20) return;
    int r2 = SelectTeamMenu(0);
    if (r2 <= 0) return;
    int patientid = g_JY.getBase().team(r2);
    if (patientid < 0) return;
    int v = ExecDecPoison(doctorid, patientid);
    AddPersonAttrib(doctorid, "体力", -5);
    char buf[256];
    snprintf(buf, sizeof(buf), "%s 解毒 %s, 减少 %d 点中毒", g_JY.getPerson(doctorid).name().c_str(), g_JY.getPerson(patientid).name().c_str(), -v);
    DrawStrBoxWaitKey(buf, C_ORANGE, g_CC.DefaultFont);
}

void Menu_Thing()
{
    auto base = g_JY.getBase();
    int thing[200], thingnum[200];
    int count = 0;
    for (int i = 1; i <= g_CC.MyThingNum; i++)
    {
        int id = base.item(i);
        if (id >= 0) { thing[count] = id; thingnum[count] = base.itemNum(i); count++; }
    }
    if (count == 0) return;
    int r = SelectThing(thing, thingnum);
    if (r >= 0) UseThing(r);
}

void Menu_PersonExit()
{
    int r = SelectTeamMenu(0);
    if (r <= 0 || r == 1) return; // 不能开除主角
    int pid = g_JY.getBase().team(r);
    if (pid < 0) return;
    char buf[256];
    snprintf(buf, sizeof(buf), "确定要让 %s 离队吗？", g_JY.getPerson(pid).name().c_str());
    if (DrawStrBoxYesNo(buf, C_ORANGE, g_CC.DefaultFont))
    {
        instruct_21(pid);
        // 设置离队事件
        auto& pexit = g_CC.PersonExit[g_Config.Version];
        for (auto& pe : pexit)
        {
            if (pe.personId == pid)
            {
                instruct_3(g_JY.SubScene, pe.eventId, 1, -2, -1, -1, -1, -1, -1, -1, -2, -2, -2);
                break;
            }
        }
    }
}

int Menu_System()
{
    std::vector<MenuItem> menu = {
        {"存档一", nullptr, 1}, {"存档位", nullptr, 1}, {"存档位", nullptr, 1},
        {"读档一", nullptr, 1}, {"读档二", nullptr, 1}, {"读档三", nullptr, 1},
        {"退出游戏", nullptr, 1}
    };
    int r = ShowMenu(menu, 7, 0, g_CC.MainSubMenuX, g_CC.MainSubMenuY, 0, 0, 1, 1, g_CC.DefaultFont, C_ORANGE, C_WHITE);
    if (r >= 1 && r <= 3) { SaveRecord(r); DrawStrBoxWaitKey("存档完成", C_ORANGE, g_CC.DefaultFont); }
    else if (r >= 4 && r <= 6) { LoadRecord(r - 3); return 1; }
    else if (r == 7) { g_JY.Status = GAME_END; return 1; }
    return 0;
}

void MMenu()
{
    std::vector<MenuItem> menu = {
        {"医疗", nullptr, 1}, {"解毒", nullptr, 1}, {"物品", nullptr, 1},
        {"状态", nullptr, 1}, {"离队", nullptr, 1}, {"系统", nullptr, 1}
    };
    int r = ShowMenu(menu, 6, 0, g_CC.MainMenuX, g_CC.MainMenuY, 0, 0, 1, 1, g_CC.DefaultFont, C_ORANGE, C_WHITE);
    switch (r)
    {
        case 1: Menu_Doctor(); break;
        case 2: Menu_DecPoison(); break;
        case 3: Menu_Thing(); break;
        case 4: Menu_Status(); break;
        case 5: Menu_PersonExit(); break;
        case 6: Menu_System(); break;
    }
    Cls();
}

// ============ 事件执行 ============
void EventExecute(int sceneid, int did)
{
    g_JY.CurrentD = did;
    int eventnum = GetD(sceneid, did, 2); // 空格触发事件
    if (eventnum > 0)
        ReadKDEF(eventnum);
    g_JY.CurrentD = -1;
    g_JY.Darkness = 0;
}

// ============ 初始�?============
void Init_MMap()
{
    CleanMemory();
    JY_PicInit(g_Config.PaletteFile.c_str());
    JY_LoadMMap(g_CC.MMapFile[0].c_str(), g_CC.MMapFile[1].c_str(), g_CC.MMapFile[2].c_str(),
        g_CC.MMapFile[3].c_str(), g_CC.MMapFile[4].c_str(),
        g_CC.MWidth, g_CC.MHeight,
        g_JY.getBase().personX(), g_JY.getBase().personY());
    JY_PicLoadFile(g_CC.MMAPPicFile[0].c_str(), g_CC.MMAPPicFile[1].c_str(), 0, 0, 0);
    PlayMIDI(g_JY.MmapMusic);
    g_JY.Status = GAME_MMAP;
    g_JY.MyPic = GetMyPic();
}

void Init_SMap(int sceneid)
{
    g_JY.SubScene = sceneid;
    CleanMemory();
    JY_PicInit(g_Config.PaletteFile.c_str());
    JY_PicLoadFile(g_CC.SMAPPicFile[0].c_str(), g_CC.SMAPPicFile[1].c_str(), 0, 0, 0);
    int zoom = limitX(g_CC.ScreenW / 800 * g_Config.Zoom, 0, g_Config.Zoom);
    JY_LoadPNGPath(g_CC.HeadPath.c_str(), 1, g_CC.HeadNum, zoom, "png");
    JY_LoadPNGPath(g_CC.ThingPath.c_str(), 2, g_CC.ThingNum, zoom, "png");
    int music = g_JY.getScene(sceneid).getByName("进门音乐");
    if (music >= 0) PlayMIDI(music);
    g_JY.Status = GAME_SMAP;
    g_JY.SubSceneX = 0;
    g_JY.SubSceneY = 0;
    g_JY.MyPic = GetMyPic();
    Cal_D_Valid(sceneid);
    DtoSMap(sceneid);
}

// ============ 游戏循环 ============
void Game_MMap()
{
    Cls();
    ShowScreen();
    int key = WaitKey();
    int x = g_JY.getBase().personX();
    int y = g_JY.getBase().personY();

    if (key == GK_UP) { y--; g_JY.getBase().setPersonDir(0); }
    else if (key == GK_DOWN) { y++; g_JY.getBase().setPersonDir(3); }
    else if (key == GK_LEFT) { x--; g_JY.getBase().setPersonDir(2); }
    else if (key == GK_RIGHT) { x++; g_JY.getBase().setPersonDir(1); }
    else if (key == GK_ESCAPE) { MMenu(); return; }

    x = limitX(x, 0, g_CC.MWidth - 1);
    y = limitX(y, 0, g_CC.MHeight - 1);

    // 检查是否进入场景
    int building = JY_GetMMap(x, y, 2);
    if (building > 0)
    {
        // 查找场景
        for (int i = 0; i < g_JY.SceneNum; i++)
        {
            auto s = g_JY.getScene(i);
            if ((s.getByName("外景入口X1") == x && s.getByName("外景入口Y1") == y) ||
                (s.getByName("外景入口X2") == x && s.getByName("外景入口Y2") == y))
            {
                if (s.getByName("进入条件") == 0)
                {
                    g_JY.getBase().setPersonX(x);
                    g_JY.getBase().setPersonY(y);
                    Init_SMap(i);
                    g_JY.getBase().setPersonX1(s.enterX());
                    g_JY.getBase().setPersonY1(s.enterY());
                    g_JY.Status = GAME_SMAP;
                    return;
                }
            }
        }
    }

    // 检查是否水面/可通行
    int ground = JY_GetMMap(x, y, 0);
    if (g_CC.MMapBoat.count(ground))
    {
        // 船行�?
        g_JY.getBase().setBoatFlag(1);
    }

    g_JY.getBase().setPersonX(x);
    g_JY.getBase().setPersonY(y);
    AddMyCurrentPic();
    g_JY.MyPic = GetMyPic();
}

void Game_SMap()
{
    DtoSMap();
    DrawSMap();
    ShowScreen();
    int key = WaitKey();
    int x = g_JY.getBase().personX1();
    int y = g_JY.getBase().personY1();

    if (key == GK_UP) { y--; g_JY.getBase().setPersonDir(0); }
    else if (key == GK_DOWN) { y++; g_JY.getBase().setPersonDir(3); }
    else if (key == GK_LEFT) { x--; g_JY.getBase().setPersonDir(2); }
    else if (key == GK_RIGHT) { x++; g_JY.getBase().setPersonDir(1); }
    else if (key == GK_ESCAPE) { MMenu(); return; }
    else if (key == GK_SPACE || key == GK_RETURN)
    {
        // 空格触发事件
        int fx = x + g_CC.DirectX[g_JY.getBase().personDir()];
        int fy = y + g_CC.DirectY[g_JY.getBase().personDir()];
        int d = GetS(g_JY.SubScene, fx, fy, 3);
        if (d >= 0)
        {
            int ev = GetD(g_JY.SubScene, d, 2);
            if (ev > 0)
            {
                EventExecute(g_JY.SubScene, d);
                Cal_D_Valid(g_JY.SubScene);
                DtoSMap(g_JY.SubScene);
            }
        }
        return;
    }

    if (SceneCanPass(g_JY.SubScene, x, y))
    {
        g_JY.getBase().setPersonX1(x);
        g_JY.getBase().setPersonY1(y);
    }
    g_JY.getBase().setPersonX1(limitX(g_JY.getBase().personX1(), 1, g_CC.SWidth - 2));
    g_JY.getBase().setPersonY1(limitX(g_JY.getBase().personY1(), 1, g_CC.SHeight - 2));
    AddMyCurrentPic();
    g_JY.MyPic = GetMyPic();

    // 检查是否路过触发事件
    int d = GetS(g_JY.SubScene, g_JY.getBase().personX1(), g_JY.getBase().personY1(), 3);
    if (d >= 0)
    {
        int ev = GetD(g_JY.SubScene, d, 4);
        if (ev > 0)
        {
            g_JY.CurrentD = d;
            ReadKDEF(ev);
            g_JY.CurrentD = -1;
            Cal_D_Valid(g_JY.SubScene);
            DtoSMap(g_JY.SubScene);
        }
    }

    // 检查是否到出口
    auto scene = g_JY.getScene(g_JY.SubScene);
    if ((g_JY.getBase().personX1() == scene.exitX1() && g_JY.getBase().personY1() == scene.getByName("出口Y1")) ||
        (g_JY.getBase().personX1() == scene.getByName("出口X2") && g_JY.getBase().personY1() == scene.getByName("出口Y2")) ||
        (g_JY.getBase().personX1() == scene.getByName("出口X3") && g_JY.getBase().personY1() == scene.getByName("出口Y3")))
    {
        // 检查跳转场景
        int jumpScene = scene.jumpScene();
        if (jumpScene >= 0)
        {
            Init_SMap(jumpScene);
            auto js = g_JY.getScene(jumpScene);
            g_JY.getBase().setPersonX1(js.enterX());
            g_JY.getBase().setPersonY1(js.enterY());
        }
        else
        {
            // 回到大地图
            g_JY.Status = GAME_FIRSTMMAP;
        }
    }
}

void Game_Cycle()
{
    while (g_JY.Status != GAME_END)
    {
        if (g_JY.Status == GAME_FIRSTMMAP)
        {
            Init_MMap();
            Cls();
            JY_ShowSlow(50, 0);
            int k, t, mx, my;
            JY_GetKey(&k, &t, &mx, &my);
            g_JY.Status = GAME_MMAP;
        }
        else if (g_JY.Status == GAME_MMAP)
        {
            Game_MMap();
        }
        else if (g_JY.Status == GAME_SMAP)
        {
            Game_SMap();
        }
    }
}

void NewGame()
{
    LoadRecord(0);
    int ver = g_Config.Version;
    int vi = ver - 1;
    if (vi < 0) vi = 0;
    if (vi > 11) vi = 11;

    // 设置新游戏场景
    auto scene = g_JY.getScene(g_CC.NewGameSceneID[vi]);
    g_JY.SubScene = g_CC.NewGameSceneID[vi];
    g_JY.getBase().setPersonX1(g_CC.NewGameSceneX[vi]);
    g_JY.getBase().setPersonY1(g_CC.NewGameSceneY[vi]);

    // 设置主角贴图
    g_JY.MyPic = g_CC.NewPersonPic[vi] / 2;

    g_JY.Status = GAME_FIRSTMMAP;
    g_JY.MmapMusic = 3;

    // 执行新游戏事件
    ReadKDEF(g_CC.NewGameEvent[vi]);
}

// ============ 退出确认 ============
int Menu_Exit()
{
    Cls();
    if (DrawStrBoxYesNo("是否真的要离开游戏？", C_WHITE, g_CC.DefaultFont))
    {
        g_JY.Status = GAME_END;
    }
    return 1;
}

// ============ 游戏主入口============
int JY_GameMain()
{
    JY_Debug("JY_GameMain start");

    // 读取二进制辅助数据
    ReadBin();

    // 开始菜单
    g_JY.Status = GAME_START;
    Cls();
    ShowScreen();
    JY_ShowSlow(50, 0);

    std::vector<MenuItem> startMenu = {
        {"新的游戏", nullptr, 1},
        {"载入进度", nullptr, 1},
        {"离开游戏", nullptr, 1}
    };
    int menux = (g_CC.ScreenW - 3 * g_CC.StartMenuFontSize - 2 * g_CC.MenuBorderPixel) / 2;
    int r = ShowMenu(startMenu, 3, 0, menux, g_CC.StartMenuY, 0, 0, 0, 0,
        g_CC.StartMenuFontSize, C_STARTMENU, C_RED);

    if (r == 1)
    {
        NewGame();
        Game_Cycle();
    }
    else if (r == 2)
    {
        std::vector<MenuItem> loadMenu = {
            {"进度一", nullptr, 1}, {"进度二", nullptr, 1}, {"进度三", nullptr, 1}
        };
        int lr = ShowMenu(loadMenu, 3, 0, menux, g_CC.StartMenuY, 0, 0, 1, 1,
            g_CC.StartMenuFontSize, C_STARTMENU, C_RED);
        if (lr > 0)
        {
            LoadRecord(lr);
            g_JY.Status = GAME_FIRSTMMAP;
            Game_Cycle();
        }
    }

    JY_Debug("JY_GameMain end");
    return 0;
}
