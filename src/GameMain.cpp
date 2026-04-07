// GameMain.cpp - 游戏主逻辑实现
// 从 jymain.lua 转换而来

#include "GameMain.h"
#include "GameData.h"
#include "PotConv.h"
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
#include <format>

// 前向声明（战斗和事件模块�?
int WarMain(int warid, int isexp);
void WarLoad(int warid);
void ReadKDEF(int eventnum);
void ReadBin();
int War_UseAnqi(int id);

// ============ 全局运行时变量============

// UTF-8显示宽度：ASCII算1，CJK算2（与GBK字节计数等价）
static int utf8DisplayLen(const std::string& s)
{
    int len = 0;
    for (size_t i = 0; i < s.size(); )
    {
        unsigned char c = (unsigned char)s[i];
        if (c < 0x80) { len += 1; i += 1; }
        else if (c < 0xE0) { len += 2; i += 2; }
        else if (c < 0xF0) { len += 2; i += 3; }
        else { len += 2; i += 4; }
    }
    return len;
}

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
        JY_DrawMMap(g_JY.getBase().personX(), g_JY.getBase().personY(), GetMyPic());
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
    JY_DrawStr(x, y, str.c_str(), color, fontsize, g_CC.FontName.c_str(), 3, g_CC.OSCharSet);
}

void MyDrawString(int x1, int x2, int y, const std::string& str, int color, int fontsize)
{
    int len = utf8DisplayLen(str) * fontsize / 4;
    int x = (x1 + x2) / 2 - len;
    DrawString(x, y, str, color, fontsize);
}

// 绘制四角凹进的方框（12条线段组成，角部内凹4像素）
static void DrawBox_1(int x1, int y1, int x2, int y2, int color)
{
    const int s = 4;
    JY_DrawRect(x1 + s, y1, x2 - s, y1, color);       // 上边
    JY_DrawRect(x2 - s, y1, x2 - s, y1 + s, color);   // 右上内
    JY_DrawRect(x2 - s, y1 + s, x2, y1 + s, color);   // 右上外
    JY_DrawRect(x2, y1 + s, x2, y2 - s, color);       // 右边
    JY_DrawRect(x2, y2 - s, x2 - s, y2 - s, color);   // 右下外
    JY_DrawRect(x2 - s, y2 - s, x2 - s, y2, color);   // 右下内
    JY_DrawRect(x2 - s, y2, x1 + s, y2, color);       // 下边
    JY_DrawRect(x1 + s, y2, x1 + s, y2 - s, color);   // 左下内
    JY_DrawRect(x1 + s, y2 - s, x1, y2 - s, color);   // 左下外
    JY_DrawRect(x1, y2 - s, x1, y1 + s, color);       // 左边
    JY_DrawRect(x1, y1 + s, x1 + s, y1 + s, color);   // 左上外
    JY_DrawRect(x1 + s, y1 + s, x1 + s, y1, color);   // 左上内
}

void DrawBox(int x1, int y1, int x2, int y2, int color)
{
    const int s = 4;
    // 背景半透明暗化（四角空出，分3个矩形区域，与Lua一致）
    JY_Background(x1, y1 + s, x1 + s, y2 - s, 128, 0);  // 左侧竖条
    JY_Background(x1 + s, y1, x2 - s, y2, 128, 0);      // 中间大块
    JY_Background(x2 - s, y1 + s, x2, y2 - s, 128, 0);  // 右侧竖条
    // 暗色阴影边框（偏移+1, 颜色减半）
    int r, g, b;
    GetRGB_JY(color, r, g, b);
    DrawBox_1(x1 + 1, y1 + 1, x2, y2, RGB_JY(r / 2, g / 2, b / 2));
    // 亮色主边框
    DrawBox_1(x1, y1, x2 - 1, y2 - 1, color);
}

void DrawStrBox(int x, int y, const std::string& str, int color, int fontsize)
{
    int len = utf8DisplayLen(str);
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
        if (key > 0) return key;
        JY_Delay(10);
    }
}

// 非阻塞获取按键，无按键返回 -1
int GetKey()
{
    int key, type, mx, my;
    JY_GetKey(&key, &type, &mx, &my);
    if (key > 0) return key;
    return -1;
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
    int n;
    if (g_JY.Status == GAME_MMAP && g_JY.getBase().boatFlag() == 1)
    {
        if (g_JY.MyCurrentPic >= 4) g_JY.MyCurrentPic = 0;
        n = g_CC.BoatStartPic + direction * 4 + g_JY.MyCurrentPic;
    }
    else
    {
        if (g_JY.MyCurrentPic > 6) g_JY.MyCurrentPic = 1;
        n = g_CC.MyStartPic + direction * 7 + g_JY.MyCurrentPic;
    }
    return n;
}

void AddMyCurrentPic(int direction, int step)
{
    g_JY.MyCurrentPic++;
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
    g_JY.D_Valid.assign(200, 0);
    g_JY.D_PicChange.clear();
    g_JY.D_Valid_Num = 0;
    // 标记所有有效的D*事件
    for (int i = 0; i < 200; i++)
    {
        int x = GetD(sceneid, i, 9);
        int y = GetD(sceneid, i, 10);
        int v = GetS(sceneid, x, y, 3);
        if (v >= 0)
        {
            g_JY.D_Valid[g_JY.D_Valid_Num] = i;
            g_JY.D_Valid_Num++;
        }
    }
}

void DtoSMap(int sceneid)
{
    if (sceneid < 0) sceneid = g_JY.SubScene;
    g_JY.NumD_PicChange = 0;
    g_JY.D_PicChange.clear();

    if (g_JY.D_Valid_Dirty)
    {
        Cal_D_Valid(sceneid);
        g_JY.D_Valid_Dirty = false;
    }

    for (int k = 0; k < g_JY.D_Valid_Num; k++)
    {
        int i = g_JY.D_Valid[k];
        int p1 = GetD(sceneid, i, 5);
        if (p1 > 0)
        {
            int p2 = GetD(sceneid, i, 6);
            int p3 = GetD(sceneid, i, 7);
            if (p1 != p2)
            {
                int old_p3 = p3;
                int delay = GetD(sceneid, i, 8);
                bool isFlag = (p3 >= g_CC.SceneFlagPic[0] * 2 && p3 <= g_CC.SceneFlagPic[1] * 2 && g_CC.ShowFlag == 0);
                if (!isFlag)
                {
                    if (p3 <= p1)
                    {
                        if (g_JY.MyTick2 % 100 > delay) p3 += 2;
                    }
                    else
                    {
                        if (g_JY.MyTick2 % 4 == 0) p3 += 2;
                    }
                    if (p3 > p2) p3 = p1;
                }
                if (old_p3 != p3)
                {
                    int x = GetD(sceneid, i, 9);
                    int y = GetD(sceneid, i, 10);
                    int dy = GetS(sceneid, x, y, 4);
                    g_JY.D_PicChange.push_back({x, y, dy, old_p3 / 2, p3 / 2});
                    g_JY.NumD_PicChange++;
                    SetD(sceneid, i, 7, p3);
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
    int x0 = g_JY.SubSceneX + x - 1;
    int y0 = g_JY.SubSceneY + y - 1;
    int xoff = limitX(x0, g_CC.SceneXMin, g_CC.SceneXMax) - x;
    int yoff = limitX(y0, g_CC.SceneYMin, g_CC.SceneYMax) - y;
    JY_DrawSMap(sceneid, x, y, xoff, yoff, mypic);
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
    // 去掉所有*
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
            if (v >= 0xF0) { pos += 4; w += 2; }
            else if (v >= 0xE0) { pos += 3; w += 2; }
            else if (v >= 0xC0) { pos += 2; w += 2; }
            else { pos++; w++; }
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
    // data charset -> UTF8
    const char* enc = (g_CC.SrcCharSet == 1) ? "cp950" : "cp936";
    std::string result = PotConv::conv(str, enc, "utf-8");
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
    GetKey(); // 清空按键缓冲

    // 过滤出可见菜单项（enabled>0），保留原始索引映射
    struct VisibleItem { std::string label; std::function<int(std::vector<MenuItem>&, int)> callback; int enabled; int origIdx; };
    std::vector<VisibleItem> vis;
    for (int i = 0; i < n; i++)
    {
        if (menu[i].enabled > 0)
            vis.push_back({menu[i].label, menu[i].callback, menu[i].enabled, i});
    }
    int newNumItem = (int)vis.size();
    if (newNumItem <= 0) return 0;

    // 实际显示项数（自动压缩高度）
    int num = newNumItem;
    if (shownum > 0 && shownum < num) num = shownum;

    // 计算边框宽高
    int maxlen = 0;
    for (auto& v : vis)
    {
        int len = utf8DisplayLen(v.label);
        if (len > maxlen) maxlen = len;
    }
    int itemh = fontsize + g_CC.RowPixel;
    int boxw = maxlen * fontsize / 2 + 2 * g_CC.MenuBorderPixel;
    int boxh = num * itemh + g_CC.MenuBorderPixel;

    if (x < 0) x = (g_CC.ScreenW - boxw) / 2;
    if (y < 0) y = (g_CC.ScreenH - boxh) / 2;

    // 保存全屏背景（父菜单等内容都保留）
    int surid = JY_SaveSur(0, 0, g_CC.ScreenW, g_CC.ScreenH);

    int cur = 0;
    // 找默认选中项（enabled==2 表示默认选中）
    for (int i = 0; i < newNumItem; i++)
    {
        if (vis[i].enabled == 2) { cur = i; break; }
    }
    if (shownum > 0) cur = 0;
    int top = 0;

    while (true)
    {
        // 先恢复原背景，防止Background叠加变暗
        Cls();
        JY_LoadSur(surid, 0, 0);

        // 绘制菜单框
        DrawBox(x, y, x + boxw, y + boxh, C_WHITE);
        for (int i = 0; i < num && top + i < newNumItem; i++)
        {
            int idx = top + i;
            int cy = y + g_CC.MenuBorderPixel + i * itemh;
            int col = (idx == cur) ? color1 : color2;
            DrawString(x + g_CC.MenuBorderPixel, cy, vis[idx].label, col, fontsize);
        }
        ShowScreen();

        int key = WaitKey();
        if (key == GK_UP)
        {
            cur--;
            if (cur < 0) { cur = newNumItem - 1; top = (cur >= num) ? cur - num + 1 : 0; }
            else if (cur < top) top = cur;
        }
        else if (key == GK_DOWN)
        {
            cur++;
            if (cur >= newNumItem) { cur = 0; top = 0; }
            else if (cur >= top + num) top = cur - num + 1;
        }
        else if (key == GK_SPACE || key == GK_RETURN)
        {
            if (vis[cur].callback)
            {
                int r = vis[cur].callback(menu, vis[cur].origIdx);
                if (r != 0)
                {
                    JY_LoadSur(surid, 0, 0);
                    JY_FreeSur(surid);
                    return vis[cur].origIdx + 1;
                }
                else
                {
                    // 子功能返回后，恢复背景并重绘本菜单
                    JY_FillColor(0, 0, 0, 0, 0);
                    JY_LoadSur(surid, 0, 0);
                }
            }
            else
            {
                JY_LoadSur(surid, 0, 0);
                JY_FreeSur(surid);
                return vis[cur].origIdx + 1;
            }
        }
        else if (key == GK_ESCAPE)
        {
            if (flag1 == 1)
            {
                JY_LoadSur(surid, 0, 0);
                JY_FreeSur(surid);
                return 0;
            }
        }
    }
}

int ShowMenu2(std::vector<MenuItem>& menu, int n, int x, int y, int fontsize, int color1, int color2)
{
    GetKey(); // 清空按键缓冲

    // 过滤可见项
    struct VisibleItem { std::string label; int origIdx; };
    std::vector<VisibleItem> vis;
    for (int i = 0; i < n; i++)
    {
        if (menu[i].enabled > 0)
            vis.push_back({menu[i].label, i});
    }
    int num = (int)vis.size();
    if (num <= 0) return 0;

    int maxlen = 0;
    for (auto& v : vis)
    {
        int len = utf8DisplayLen(v.label);
        if (len > maxlen) maxlen = len;
    }
    int cellw = fontsize * maxlen / 2 + g_CC.RowPixel;
    int totalw = cellw * num + g_CC.MenuBorderPixel;
    if (x < 0) x = (g_CC.ScreenW - totalw) / 2;
    int h = fontsize + 2 * g_CC.MenuBorderPixel;

    // 先绘制框（ShowMenu2 在循环外画框，与Lua一致）
    DrawBox(x, y, x + totalw, y + h, C_WHITE);

    // 保存含框的画面（用于每帧恢复）
    int surid = JY_SaveSur(x, y, totalw + 1, h + 1);
    int cur = 0;
    // 找默认选中项
    for (int i = 0; i < num; i++)
    {
        if (menu[vis[i].origIdx].enabled == 2) { cur = i; break; }
    }

    while (true)
    {
        // 恢复框区域（清除上次文字），重绘文字
        JY_LoadSur(surid, x, y);
        int cx = x + g_CC.MenuBorderPixel;
        for (int i = 0; i < num; i++)
        {
            int col = (i == cur) ? color1 : color2;
            DrawString(cx, y + g_CC.MenuBorderPixel, vis[i].label, col, fontsize);
            cx += cellw;
        }
        ShowScreen();

        int key = WaitKey();
        if (key == GK_LEFT)
        {
            cur--;
            if (cur < 0) cur = num - 1;
        }
        else if (key == GK_RIGHT)
        {
            cur++;
            if (cur >= num) cur = 0;
        }
        else if (key == GK_SPACE || key == GK_RETURN)
        {
            // 恢复菜单区域
            Cls(x, y, x + totalw + 1, y + h + 1);
            JY_FreeSur(surid);
            return vis[cur].origIdx + 1;
        }
        else if (key == GK_ESCAPE)
        {
            Cls(x, y, x + totalw + 1, y + h + 1);
            JY_FreeSur(surid);
            return 0;
        }
    }
}

// ============ 记录管理 ============
void LoadRecord(int id)
{
    // 始终从原始 ranger.idx 读取6个边界偏移（与Lua一致）
    // idx文件格式: 6个int32边界值，Base隐式从0开始
    // offset[0]=Base结束/Person开始, offset[1]=Person结束/Thing开始, ...
    DataBuffer idx;
    idx.alloc(24);
    idx.loadfile(g_CC.R_IDXFilename[0].c_str(), 0, 24);
    int offset[6];
    for (int i = 0; i < 6; i++) offset[i] = idx.get32(i * 4);

    const std::string& grpfile = g_CC.R_GRPFilename[id];

    // 加载Base: 位置0, 大小=offset[0]
    g_JY.Data_Base.alloc(offset[0]);
    g_JY.Data_Base.loadfile(grpfile.c_str(), 0, offset[0]);
    g_JY.Base.buf = &g_JY.Data_Base;

    // 加载Person: 位置offset[0], 大小=offset[1]-offset[0]
    int personLen = offset[1] - offset[0];
    g_JY.PersonNum = personLen / g_CC.PersonSize;
    g_JY.Data_Person.alloc(personLen);
    g_JY.Data_Person.loadfile(grpfile.c_str(), offset[0], personLen);

    // 加载Thing: 位置offset[1], 大小=offset[2]-offset[1]
    int thingLen = offset[2] - offset[1];
    g_JY.ThingNum = thingLen / g_CC.ThingSize;
    g_JY.Data_Thing.alloc(thingLen);
    g_JY.Data_Thing.loadfile(grpfile.c_str(), offset[1], thingLen);

    // 加载Scene: 位置offset[2], 大小=offset[3]-offset[2]
    int sceneLen = offset[3] - offset[2];
    g_JY.SceneNum = sceneLen / g_CC.SceneSize;
    g_JY.Data_Scene.alloc(sceneLen);
    g_JY.Data_Scene.loadfile(grpfile.c_str(), offset[2], sceneLen);

    // 加载Wugong: 位置offset[3], 大小=offset[4]-offset[3]
    int wugongLen = offset[4] - offset[3];
    g_JY.WugongNum = wugongLen / g_CC.WugongSize;
    g_JY.Data_Wugong.alloc(wugongLen);
    g_JY.Data_Wugong.loadfile(grpfile.c_str(), offset[3], wugongLen);

    // 加载Shop: 位置offset[4], 大小=offset[5]-offset[4]
    int shopLen = offset[5] - offset[4];
    g_JY.ShopNum = shopLen / g_CC.ShopSize;
    g_JY.Data_Shop.alloc(shopLen);
    g_JY.Data_Shop.loadfile(grpfile.c_str(), offset[4], shopLen);

    // 加载地图
    JY_LoadSMap(g_CC.S_Filename[id].c_str(), g_CC.TempS_Filename.c_str(),
        g_JY.SceneNum, g_CC.SWidth, g_CC.SHeight,
        g_CC.D_Filename[id].c_str(), g_CC.DNum, g_CC.DNum2);

    JY_Debug("LoadRecord %d ok", id);
}

void SaveRecord(int id)
{
    if (id == 0) return; // 不保存初始存档

    // 始终从原始 ranger.idx 读取边界偏移（与Lua一致，不生成新idx）
    DataBuffer idx;
    idx.alloc(24);
    idx.loadfile(g_CC.R_IDXFilename[0].c_str(), 0, 24);
    int offset[6];
    for (int i = 0; i < 6; i++) offset[i] = idx.get32(i * 4);

    // 删除旧存档GRP
    const std::string& grpfile = g_CC.R_GRPFilename[id];
    remove(grpfile.c_str());

    // 按原始偏移位置写入各段数据
    g_JY.Data_Base.savefile(grpfile.c_str(), 0, offset[0]);
    g_JY.Data_Person.savefile(grpfile.c_str(), offset[0], g_CC.PersonSize * g_JY.PersonNum);
    g_JY.Data_Thing.savefile(grpfile.c_str(), offset[1], g_CC.ThingSize * g_JY.ThingNum);
    g_JY.Data_Scene.savefile(grpfile.c_str(), offset[2], g_CC.SceneSize * g_JY.SceneNum);
    g_JY.Data_Wugong.savefile(grpfile.c_str(), offset[3], g_CC.WugongSize * g_JY.WugongNum);
    g_JY.Data_Shop.savefile(grpfile.c_str(), offset[4], g_CC.ShopSize * g_JY.ShopNum);

    // 保存地图
    JY_SaveSMap(g_CC.S_Filename[id].c_str(), g_CC.D_Filename[id].c_str());

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
    auto msg = std::format("得到物品:{} {}", thing.name(), num);
    DrawStrBoxWaitKey(msg, C_ORANGE, g_CC.DefaultFont);
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
        auto msg = std::format("{} 学会武功 {}", p.name(), g_JY.getWugong(wugongid).name());
        DrawStrBoxWaitKey(msg, C_ORANGE, g_CC.DefaultFont);
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
    msgs.push_back(std::format("使用 {}", thing.name()));

    auto tryAdd = [&](const std::string& attr) {
        int addv = thing.getByName("加" + attr);
        if (addv != 0)
        {
            int actual = AddPersonAttrib(pid, attr, addv);
            if (actual != 0)
            {
                msgs.push_back(std::format(" {} {:+d}", attr, actual));
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
        if (actual != 0) { msgs.push_back(std::format(" 生命 {:+d}", actual)); changed = 1; }
    }

    tryAdd("生命最大值");

    int decPoison = thing.getByName("加中毒解药");
    if (decPoison < 0)
    {
        int actual = AddPersonAttrib(pid, "中毒程度", decPoison / 2);
        if (actual != 0) { msgs.push_back(std::format(" 中毒程度 {:+d}", actual)); changed = 1; }
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
            int r = SelectTeamMenu(g_CC.MainSubMenuX, g_CC.MainSubMenuY);
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
    // 图形化物品选择菜单 (匹配Lua版SelectThing)
    int xnum = g_CC.MenuThingXnum;
    int ynum = g_CC.MenuThingYnum;

    int w = g_CC.ThingPicWidth * xnum + (xnum - 1) * g_CC.ThingGapIn + 2 * g_CC.ThingGapOut;
    int h = g_CC.ThingPicHeight * ynum + (ynum - 1) * g_CC.ThingGapIn + 2 * g_CC.ThingGapOut;

    int dx = (g_CC.ScreenW - w) / 2;
    int dy = (g_CC.ScreenH - h - 2 * (g_CC.ThingFontSize + 2 * g_CC.MenuBorderPixel + 5)) / 2;

    int cur_line = 0;
    int cur_x = 0;
    int cur_y = 0;
    int cur_thing = -1;

    while (true)
    {
        Cls();
        // 名称栏
        int y1_1 = dy;
        int y1_2 = y1_1 + g_CC.ThingFontSize + 2 * g_CC.MenuBorderPixel;
        DrawBox(dx, y1_1, dx + w, y1_2, C_WHITE);
        // 说明栏
        int y2_1 = y1_2 + 5;
        int y2_2 = y2_1 + g_CC.ThingFontSize + 2 * g_CC.MenuBorderPixel;
        DrawBox(dx, y2_1, dx + w, y2_2, C_WHITE);
        // 物品图片区
        int y3_1 = y2_2 + 5;
        int y3_2 = y3_1 + h;
        DrawBox(dx, y3_1, dx + w, y3_2, C_WHITE);

        for (int yy = 0; yy < ynum; yy++)
        {
            for (int xx = 0; xx < xnum; xx++)
            {
                int idx = yy * xnum + xx + xnum * cur_line;
                int boxcolor;
                if (xx == cur_x && yy == cur_y)
                {
                    boxcolor = C_WHITE;
                    if (thing[idx] >= 0)
                    {
                        cur_thing = thing[idx];
                        auto t = g_JY.getThing(thing[idx]);
                        std::string str = t.name();
                        if (t.type() == 1 || t.type() == 2)
                        {
                            if (t.user() >= 0)
                            {
                                str += "(" + g_JY.getPerson(t.user()).name() + ")";
                            }
                        }
                        auto label = std::format("{} X {}", str, thingnum[idx]);
                        std::string str2 = t.desc();
                        DrawString(dx + g_CC.ThingGapOut, y1_1 + g_CC.MenuBorderPixel, label, C_GOLD, g_CC.ThingFontSize);
                        DrawString(dx + g_CC.ThingGapOut, y2_1 + g_CC.MenuBorderPixel, str2, C_ORANGE, g_CC.ThingFontSize);
                    }
                    else
                    {
                        cur_thing = -1;
                    }
                }
                else
                {
                    boxcolor = C_BLACK;
                }
                int boxx = dx + g_CC.ThingGapOut + xx * (g_CC.ThingPicWidth + g_CC.ThingGapIn);
                int boxy = y3_1 + g_CC.ThingGapOut + yy * (g_CC.ThingPicHeight + g_CC.ThingGapIn);
                JY_DrawRect(boxx, boxy, boxx + g_CC.ThingPicWidth + 1, boxy + g_CC.ThingPicHeight + 1, boxcolor);
                if (thing[idx] >= 0)
                {
                    JY_LoadPNG(2, thing[idx] * 2, boxx + 1, boxy + 1, 1, 0, 100);
                }
            }
        }

        ShowScreen();
        int keypress = WaitKey();
        JY_Delay(100);
        if (keypress == GK_ESCAPE)
        {
            cur_thing = -1;
            break;
        }
        else if (keypress == GK_RETURN || keypress == GK_SPACE)
        {
            break;
        }
        else if (keypress == GK_UP)
        {
            if (cur_y == 0)
            {
                if (cur_line > 0) cur_line--;
            }
            else
                cur_y--;
        }
        else if (keypress == GK_DOWN)
        {
            if (cur_y == ynum - 1)
            {
                if (cur_line < (200 / xnum - ynum)) cur_line++;
            }
            else
                cur_y++;
        }
        else if (keypress == GK_LEFT)
        {
            if (cur_x > 0) cur_x--;
            else cur_x = xnum - 1;
        }
        else if (keypress == GK_RIGHT)
        {
            if (cur_x == xnum - 1) cur_x = 0;
            else cur_x++;
        }
    }
    Cls();
    return cur_thing;
}

// ============ 医疗/解毒 ============
int ExecDoctor(int doctorid, int patientid)
{
    // 严格按照Lua版ExecDoctor移植
    auto doctor = g_JY.getPerson(doctorid);
    auto patient = g_JY.getPerson(patientid);

    if (doctor.getByName("体力") < 50) return 0;

    int add = doctor.getByName("医疗能力");
    int value = patient.getByName("受伤程度");

    if (value > add + 20) return 0;

    // 根据受伤程度计算实际医疗能力
    if (value < 25)
        add = add * 4 / 5;
    else if (value < 50)
        add = add * 3 / 4;
    else if (value < 75)
        add = add * 2 / 3;
    else
        add = add / 2;

    add = add + Rnd(5);

    AddPersonAttrib(patientid, "受伤程度", -add);
    return AddPersonAttrib(patientid, "生命", add);
}

int ExecDecPoison(int doctorid, int patientid)
{
    // 严格按照Lua版ExecDecPoison移植
    auto doctor = g_JY.getPerson(doctorid);
    auto patient = g_JY.getPerson(patientid);

    int add = doctor.getByName("解毒能力");
    int value = patient.getByName("中毒程度");

    if (value > add + 20) return 0;

    add = limitX(add / 3 + Rnd(10) - Rnd(10), 0, value);
    return -AddPersonAttrib(patientid, "中毒程度", -add);
}

// ============ 菜单处理 ============
int SelectTeamMenu(int x, int y)
{
    auto base = g_JY.getBase();
    std::vector<MenuItem> menu;
    for (int i = 1; i <= g_CC.TeamNum; i++)
    {
        int id = base.team(i);
        if (id >= 0 && g_JY.getPerson(id).hp() > 0)
            menu.push_back({g_JY.getPerson(id).name(), nullptr, 1});
        else
            menu.push_back({"", nullptr, 0});
    }
    int count = (int)menu.size();
    return ShowMenu(menu, count, 0, x, y, 0, 0, 1, 1, g_CC.DefaultFont, C_ORANGE, C_WHITE);
}

void Menu_Status()
{
    DrawStrBox(g_CC.MainSubMenuX, g_CC.MainSubMenuY, "要查阅谁的状态", C_WHITE, g_CC.DefaultFont);
    int nexty = g_CC.MainSubMenuY + g_CC.SingleLineHeight;
    int r = SelectTeamMenu(g_CC.MainSubMenuX, nexty);
    if (r > 0)
    {
        ShowPersonStatus(r);
    }
    else
    {
        Cls();
    }
}

static int GetTeamNum()
{
    int r = g_CC.TeamNum;
    for (int i = 1; i <= g_CC.TeamNum; i++)
    {
        if (g_JY.getBase().team(i) < 0) { r = i - 1; break; }
    }
    return r;
}

static void ShowPersonStatus_sub(int id, int page)
{
    int size = g_CC.DefaultFont;
    auto p = g_JY.getPerson(id);
    int width = 18 * size + 15;
    int h = size + g_CC.PersonStateRowPixel;
    int height = 13 * h + 10;
    int dx = (g_CC.ScreenW - width) / 2;
    int dy = (g_CC.ScreenH - height) / 2;
    int i = 1;
    int x1, y1;

    DrawBox(dx, dy, dx + width, dy + height, C_WHITE);

    x1 = dx + 5;
    y1 = dy + 5;
    int x2 = 4 * size;

    int headw, headh, hxoff, hyoff;
    JY_GetPNGXY(1, p.headId() * 2, &headw, &headh, &hxoff, &hyoff);
    int headx = (width / 2 - headw) / 2;
    int heady = (h * 6 - headh) / 2;
    JY_LoadPNG(1, p.headId() * 2, x1 + headx, y1 + heady, 1, 0, 100);

    i = 6;
    DrawString(x1, y1 + h * i, p.name(), C_WHITE, size);
    DrawString(x1 + 10 * size / 2, y1 + h * i, std::format("{:3d}", p.level()), C_GOLD, size);
    DrawString(x1 + 13 * size / 2, y1 + h * i, "级", C_ORANGE, size);

    auto drawAttrib = [&](const char* str, int color1, int color2, int val) {
        DrawString(x1, y1 + h * i, str, color1, size);
        DrawString(x1 + x2, y1 + h * i, std::format("{:5d}", val), color2, size);
        i++;
    };

    if (page == 1)
    {
        int color;
        if (p.injury() < 33) color = RGB_JY(236, 200, 40);
        else if (p.injury() < 66) color = RGB_JY(244, 128, 32);
        else color = RGB_JY(232, 32, 44);
        i = 7;
        DrawString(x1, y1 + h * i, "生命", C_ORANGE, size);
        DrawString(x1 + 2 * size, y1 + h * i, std::format("{:5d}", p.hp()), color, size);
        DrawString(x1 + 9 * size / 2, y1 + h * i, "/", C_GOLD, size);
        if (p.poison() == 0) color = RGB_JY(252, 148, 16);
        else if (p.poison() < 50) color = RGB_JY(120, 208, 88);
        else color = RGB_JY(56, 136, 36);
        DrawString(x1 + 5 * size, y1 + h * i, std::format("{:5d}", p.maxHp()), color, size);
        i++;
        if (p.mpType() == 0) color = RGB_JY(208, 152, 208);
        else if (p.mpType() == 1) color = RGB_JY(236, 200, 40);
        else color = RGB_JY(236, 236, 236);
        DrawString(x1, y1 + h * i, "内力", C_ORANGE, size);
        DrawString(x1 + 2 * size, y1 + h * i, std::format("{:5d}/{:5d}", p.mp(), p.maxMp()), color, size);
        i++;
        drawAttrib("体力", C_ORANGE, C_GOLD, p.stamina());
        drawAttrib("经验", C_ORANGE, C_GOLD, (int)p.exp());
        DrawString(x1, y1 + h * i, "升级", C_ORANGE, size);
        if (p.level() >= g_CC.PersonAttribMax["人物等级"])
            DrawString(x1 + x2, y1 + h * i, "    =", C_GOLD, size);
        else {
            DrawString(x1 + x2, y1 + h * i, std::format("{:5d}", g_CC.Exp[p.level()]), C_GOLD, size);
        }
        i++;

        int tmp1 = 0, tmp2 = 0, tmp3 = 0;
        if (p.weapon() > -1) {
            auto t = g_JY.getThing(p.weapon());
            tmp1 += t.addAttack(); tmp2 += t.addDefense(); tmp3 += t.addAgility();
        }
        if (p.armor() > -1) {
            auto t = g_JY.getThing(p.armor());
            tmp1 += t.addAttack(); tmp2 += t.addDefense(); tmp3 += t.addAgility();
        }

        DrawString(x1, y1 + h * i, "左右键翻页，上下键查看其它队友", C_RED, size);

        i = 0;
        x1 = dx + width / 2;
        drawAttrib("攻击力", C_WHITE, C_GOLD, p.attack() + tmp1);
        drawAttrib("防御力", C_WHITE, C_GOLD, p.defense() + tmp2);
        drawAttrib("轻功", C_WHITE, C_GOLD, p.agility() + tmp3);
        drawAttrib("医疗能力", C_WHITE, C_GOLD, p.medic());
        drawAttrib("用毒能力", C_WHITE, C_GOLD, p.usePoison());
        drawAttrib("解毒能力", C_WHITE, C_GOLD, p.detox());
        drawAttrib("拳掌功夫", C_WHITE, C_GOLD, p.fist());
        drawAttrib("御剑能力", C_WHITE, C_GOLD, p.sword());
        drawAttrib("耍刀技巧", C_WHITE, C_GOLD, p.blade());
        drawAttrib("特殊兵器", C_WHITE, C_GOLD, p.special());
        drawAttrib("暗器技巧", C_WHITE, C_GOLD, p.hidden());
        drawAttrib("资质", C_WHITE, C_GOLD, p.aptitude());
    }
    else if (page == 2)
    {
        i = 7;
        DrawString(x1, y1 + h * i, "武器:", C_ORANGE, size);
        if (p.weapon() > -1)
            DrawString(x1 + size * 3, y1 + h * i, g_JY.getThing(p.weapon()).name(), C_GOLD, size);
        i++;
        DrawString(x1, y1 + h * i, "防具:", C_ORANGE, size);
        if (p.armor() > -1)
            DrawString(x1 + size * 3, y1 + h * i, g_JY.getThing(p.armor()).name(), C_GOLD, size);
        i++;
        DrawString(x1, y1 + h * i, "修炼物品", C_ORANGE, size);
        int thingid = p.trainItem();
        if (thingid > 0)
        {
            i++;
            DrawString(x1 + size, y1 + h * i, g_JY.getThing(thingid).name(), C_GOLD, size);
            i++;
            int n = TrainNeedExp(id);
            std::string trainStr;
            if (n < 32767) {
                trainStr = std::format("{:5d}/{:5d}", p.trainPoints(), n);
            } else {
                trainStr = std::format("{:5d}/===", p.trainPoints());
            }
            DrawString(x1 + size, y1 + h * i, trainStr, C_GOLD, size);
        }
        else
        {
            i += 2;
        }
        i++;
        DrawString(x1, y1 + h * i, "左右键翻页，上下键查看其它队友", C_RED, size);

        i = 0;
        x1 = dx + width / 2;
        DrawString(x1, y1 + h * i, "所会功夫", C_ORANGE, size);
        for (int j = 1; j <= 10; j++)
        {
            i++;
            int wg = p.wugong(j);
            if (wg > 0)
            {
                int lv = p.wugongLevel(j) / 100 + 1;
                DrawString(x1 + size, y1 + h * i, g_JY.getWugong(wg).name(), C_GOLD, size);
                DrawString(x1 + size * 7, y1 + h * i, std::format("{:2d}", lv), C_WHITE, size);
            }
        }
    }
}

void ShowPersonStatus(int teamid)
{
    int page = 1;
    int pagenum = 2;
    int teamnum = GetTeamNum();

    while (true)
    {
        Cls();
        int id = g_JY.getBase().team(teamid);
        if (id >= 0) ShowPersonStatus_sub(id, page);
        ShowScreen();
        int key = WaitKey();
        JY_Delay(100);
        if (key == GK_ESCAPE) break;
        else if (key == GK_UP) teamid--;
        else if (key == GK_DOWN) teamid++;
        else if (key == GK_LEFT) page--;
        else if (key == GK_RIGHT) page++;
        teamid = limitX(teamid, 1, teamnum);
        page = limitX(page, 1, pagenum);
    }
}

void Menu_Doctor()
{
    DrawStrBox(g_CC.MainSubMenuX, g_CC.MainSubMenuY, "谁要使用医术", C_WHITE, g_CC.DefaultFont);
    int nexty = g_CC.MainSubMenuY + g_CC.SingleLineHeight;
    DrawStrBox(g_CC.MainSubMenuX, nexty, "医疗能力", C_ORANGE, g_CC.DefaultFont);

    auto base = g_JY.getBase();
    std::vector<MenuItem> menu1;
    for (int i = 1; i <= g_CC.TeamNum; i++)
    {
        int id = base.team(i);
        if (id >= 0 && g_JY.getPerson(id).medic() >= 20)
        {
            menu1.push_back({std::format("{:<10s}{:4d}", g_JY.getPerson(id).name(), g_JY.getPerson(id).medic()), nullptr, 1});
        }
        else
            menu1.push_back({"", nullptr, 0});
    }
    nexty += g_CC.SingleLineHeight;
    int r = ShowMenu(menu1, g_CC.TeamNum, 0, g_CC.MainSubMenuX, nexty, 0, 0, 1, 1, g_CC.DefaultFont, C_ORANGE, C_WHITE);

    if (r > 0)
    {
        int doctorid = base.team(r);
        Cls(g_CC.MainSubMenuX, g_CC.MainSubMenuY, g_CC.ScreenW, g_CC.ScreenH);
        DrawStrBox(g_CC.MainSubMenuX, g_CC.MainSubMenuY, "要医治谁", C_WHITE, g_CC.DefaultFont);
        nexty = g_CC.MainSubMenuY + g_CC.SingleLineHeight;

        std::vector<MenuItem> menu2;
        for (int i = 1; i <= g_CC.TeamNum; i++)
        {
            int id = base.team(i);
            if (id >= 0)
            {
                menu2.push_back({std::format("{:<10s}{:4d}/{:4d}", g_JY.getPerson(id).name(), g_JY.getPerson(id).hp(), g_JY.getPerson(id).maxHp()), nullptr, 1});
            }
            else
                menu2.push_back({"", nullptr, 0});
        }
        int r2 = ShowMenu(menu2, g_CC.TeamNum, 0, g_CC.MainSubMenuX, nexty, 0, 0, 1, 1, g_CC.DefaultFont, C_ORANGE, C_WHITE);
        if (r2 > 0)
        {
            int patientid = base.team(r2);
            int num = ExecDoctor(doctorid, patientid);
            if (num > 0) AddPersonAttrib(doctorid, "体力", -2);
            DrawStrBoxWaitKey(std::format("{} 生命增加 {}", g_JY.getPerson(patientid).name(), num), C_ORANGE, g_CC.DefaultFont);
        }
    }
    Cls();
}

void Menu_DecPoison()
{
    DrawStrBox(g_CC.MainSubMenuX, g_CC.MainSubMenuY, "谁要帮人解毒", C_WHITE, g_CC.DefaultFont);
    int nexty = g_CC.MainSubMenuY + g_CC.SingleLineHeight;
    DrawStrBox(g_CC.MainSubMenuX, nexty, "解毒能力", C_ORANGE, g_CC.DefaultFont);

    auto base = g_JY.getBase();
    std::vector<MenuItem> menu1;
    for (int i = 1; i <= g_CC.TeamNum; i++)
    {
        int id = base.team(i);
        if (id >= 0 && g_JY.getPerson(id).detox() >= 20)
        {
            menu1.push_back({std::format("{:<10s}{:4d}", g_JY.getPerson(id).name(), g_JY.getPerson(id).detox()), nullptr, 1});
        }
        else
            menu1.push_back({"", nullptr, 0});
    }
    nexty += g_CC.SingleLineHeight;
    int r = ShowMenu(menu1, g_CC.TeamNum, 0, g_CC.MainSubMenuX, nexty, 0, 0, 1, 1, g_CC.DefaultFont, C_ORANGE, C_WHITE);

    if (r > 0)
    {
        int doctorid = base.team(r);
        Cls(g_CC.MainSubMenuX, g_CC.MainSubMenuY, g_CC.ScreenW, g_CC.ScreenH);
        DrawStrBox(g_CC.MainSubMenuX, g_CC.MainSubMenuY, "替谁解毒", C_WHITE, g_CC.DefaultFont);
        nexty = g_CC.MainSubMenuY + g_CC.SingleLineHeight;
        DrawStrBox(g_CC.MainSubMenuX, nexty, "中毒程度", C_WHITE, g_CC.DefaultFont);
        nexty += g_CC.SingleLineHeight;

        std::vector<MenuItem> menu2;
        for (int i = 1; i <= g_CC.TeamNum; i++)
        {
            int id = base.team(i);
            if (id >= 0)
            {
                menu2.push_back({std::format("{:<10s}{:5d}", g_JY.getPerson(id).name(), g_JY.getPerson(id).poison()), nullptr, 1});
            }
            else
                menu2.push_back({"", nullptr, 0});
        }
        int r2 = ShowMenu(menu2, g_CC.TeamNum, 0, g_CC.MainSubMenuX, nexty, 0, 0, 1, 1, g_CC.DefaultFont, C_ORANGE, C_WHITE);
        if (r2 > 0)
        {
            int patientid = base.team(r2);
            int num = ExecDecPoison(doctorid, patientid);
            DrawStrBoxWaitKey(std::format("{} 中毒程度减少 {}", g_JY.getPerson(patientid).name(), num), C_ORANGE, g_CC.DefaultFont);
        }
    }
    Cls();
}

void Menu_Thing()
{
    // 物品类型子菜单: 全部物品, 剧情物品, 神兵宝甲, 武功秘笈, 灵丹妙药, 伤人暗器
    std::vector<MenuItem> menu = {
        {"全部物品", nullptr, 1},
        {"剧情物品", nullptr, 1},
        {"神兵宝甲", nullptr, 1},
        {"武功秘笈", nullptr, 1},
        {"灵丹妙药", nullptr, 1},
        {"伤人暗器", nullptr, 1},
    };
    int r = ShowMenu(menu, 6, 0, g_CC.MainSubMenuX, g_CC.MainSubMenuY, 0, 0, 1, 1, g_CC.DefaultFont, C_ORANGE, C_WHITE);
    if (r > 0)
    {
        int thing[200], thingnum[200];
        for (int i = 0; i < g_CC.MyThingNum; i++) { thing[i] = -1; thingnum[i] = 0; }

        auto base = g_JY.getBase();
        int num = 0;
        for (int i = 0; i < g_CC.MyThingNum; i++)
        {
            int id = base.item(i + 1);
            if (id >= 0)
            {
                if (r == 1) // 全部物品
                {
                    thing[i] = id;
                    thingnum[i] = base.itemNum(i + 1);
                }
                else
                {
                    if (g_JY.getThing(id).type() == r - 2) // 按类型过滤: 0=剧情,1=神兵,2=秘笈,3=灵丹,4=暗器
                    {
                        thing[num] = id;
                        thingnum[num] = base.itemNum(i + 1);
                        num++;
                    }
                }
            }
        }

        int sel = SelectThing(thing, thingnum);
        if (sel >= 0)
        {
            UseThing(sel);
        }
    }
}

void Menu_PersonExit()
{
    DrawStrBox(g_CC.MainSubMenuX, g_CC.MainSubMenuY, "要求谁离队", C_WHITE, g_CC.DefaultFont);
    int nexty = g_CC.MainSubMenuY + g_CC.SingleLineHeight;
    int r = SelectTeamMenu(g_CC.MainSubMenuX, nexty);
    if (r == 1)
    {
        DrawStrBoxWaitKey("抱歉！没有你游戏进行不下去", C_WHITE, g_CC.DefaultFont);
    }
    else if (r > 1)
    {
        int pid = g_JY.getBase().team(r);
        if (pid >= 0)
        {
            auto& pexit = g_CC.PersonExit[g_Config.Version];
            for (auto& pe : pexit)
            {
                if (pe.personId == pid)
                {
                    ReadKDEF(pe.eventId);
                    break;
                }
            }
        }
    }
    Cls();
}

int Menu_System()
{
    std::vector<MenuItem> menu = {
        {"读取进度", nullptr, 1},
        {"保存进度", nullptr, 1},
        {"关闭音乐", nullptr, 1},
        {"关闭音效", nullptr, 1},
        {"离开游戏", nullptr, 1}
    };
    int r = ShowMenu(menu, 5, 0, g_CC.MainSubMenuX, g_CC.MainSubMenuY, 0, 0, 1, 1, g_CC.DefaultFont, C_ORANGE, C_WHITE);
    if (r == 1) // 读取进度
    {
        std::vector<MenuItem> loadMenu = {
            {"进度一", nullptr, 1}, {"进度二", nullptr, 1}, {"进度三", nullptr, 1}
        };
        int lr = ShowMenu(loadMenu, 3, 0, g_CC.MainSubMenuX2, g_CC.MainSubMenuY, 0, 0, 1, 1, g_CC.DefaultFont, C_ORANGE, C_WHITE);
        if (lr > 0)
        {
            DrawStrBox(g_CC.MainSubMenuX2, g_CC.MainSubMenuY, "请稍候......", C_WHITE, g_CC.DefaultFont);
            ShowScreen();
            LoadRecord(lr);
            g_JY.Status = GAME_FIRSTMMAP;
            return 1;
        }
    }
    else if (r == 2) // 保存进度
    {
        std::vector<MenuItem> saveMenu = {
            {"进度一", nullptr, 1}, {"进度二", nullptr, 1}, {"进度三", nullptr, 1}
        };
        int sr = ShowMenu(saveMenu, 3, 0, g_CC.MainSubMenuX2, g_CC.MainSubMenuY, 0, 0, 1, 1, g_CC.DefaultFont, C_ORANGE, C_WHITE);
        if (sr > 0)
        {
            DrawStrBox(g_CC.MainSubMenuX2, g_CC.MainSubMenuY, "请稍候......", C_WHITE, g_CC.DefaultFont);
            ShowScreen();
            SaveRecord(sr);
        }
    }
    else if (r == 5) // 离开游戏
    {
        Cls();
        if (DrawStrBoxYesNo("是否真的要离开游戏？", C_WHITE, g_CC.DefaultFont))
            g_JY.Status = GAME_END;
        return 1;
    }
    return 0;
}

void MMenu()
{
    std::vector<MenuItem> menu = {
        {"医疗", nullptr, 1}, {"解毒", nullptr, 1}, {"物品", nullptr, 1},
        {"状态", nullptr, 1}, {"离队", nullptr, 1}, {"系统", nullptr, 1}
    };
    // 场景中禁用离队和系统
    if (g_JY.Status == GAME_SMAP)
    {
        menu[4].enabled = 0;
        menu[5].enabled = 0;
    }
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
// eventtype: 1=空格, 2=物品, 3=路过
void EventExecute(int did, int eventtype)
{
    g_JY.CurrentD = did;
    g_JY.CurrentEventType = eventtype;
    // 空格→D字段2, 物品→D字段3, 路过→D字段4
    int dfield = eventtype + 1;
    int eventnum = GetD(g_JY.SubScene, did, dfield);
    if (eventnum > 0)
        ReadKDEF(eventnum);
    g_JY.CurrentD = -1;
    g_JY.CurrentEventType = -1;
    g_JY.Darkness = 0;
}

// ============ 初始化 ============
void Init_MMap()
{
    JY_PicInit(g_Config.PaletteFile.c_str());
    JY_LoadMMap(g_CC.MMapFile[0].c_str(), g_CC.MMapFile[1].c_str(), g_CC.MMapFile[2].c_str(),
        g_CC.MMapFile[3].c_str(), g_CC.MMapFile[4].c_str(),
        g_CC.MWidth, g_CC.MHeight,
        g_JY.getBase().personX(), g_JY.getBase().personY());
    JY_PicLoadFile(g_CC.MMAPPicFile[0].c_str(), g_CC.MMAPPicFile[1].c_str(), 0, 0, 0);
    int zoom = limitX(g_CC.ScreenW / 800 * g_Config.Zoom, 0, g_Config.Zoom);
    JY_LoadPNGPath(g_CC.HeadPath.c_str(), 1, g_CC.HeadNum, zoom, "png");
    JY_LoadPNGPath(g_CC.ThingPath.c_str(), 2, g_CC.ThingNum, zoom, "png");
    g_JY.EnterSceneXY_Dirty = true;
    g_JY.oldMMapX = -1;
    g_JY.oldMMapY = -1;
    PlayMIDI(g_JY.MmapMusic);
}

void Init_SMap(int sceneid, int showname)
{
    g_JY.SubScene = sceneid;
    JY_PicInit(g_Config.PaletteFile.c_str());
    JY_PicLoadFile(g_CC.SMAPPicFile[0].c_str(), g_CC.SMAPPicFile[1].c_str(), 0, 0, 0);
    int zoom = limitX(g_CC.ScreenW / 800 * g_Config.Zoom, 0, g_Config.Zoom);
    JY_LoadPNGPath(g_CC.HeadPath.c_str(), 1, g_CC.HeadNum, zoom, "png");
    JY_LoadPNGPath(g_CC.ThingPath.c_str(), 2, g_CC.ThingNum, zoom, "png");
    int music = g_JY.getScene(sceneid).enterMusic();
    if (music >= 0) PlayMIDI(music);
    g_JY.oldSMapX = -1;
    g_JY.oldSMapY = -1;
    g_JY.SubSceneX = 0;
    g_JY.SubSceneY = 0;
    g_JY.OldDPass = -1;
    g_JY.D_Valid_Dirty = true;
    g_JY.MyPic = GetMyPic();
    Cal_D_Valid(sceneid);
    DtoSMap(sceneid);
    DrawSMap();
    JY_ShowSlow(50, 0);
    int k, t, mx, my;
    JY_GetKey(&k, &t, &mx, &my);

    if (showname == 1)
    {
        std::string sceneName = g_JY.getScene(sceneid).name();
        DrawStrBox(-1, 10, sceneName, C_WHITE, g_CC.DefaultFont);
        ShowScreen();
        WaitKey();
        Cls();
        ShowScreen();
    }
}

// ============ 场景入口缓存 ============
void Cal_EnterSceneXY()
{
    g_JY.EnterSceneXY.clear();
    for (int id = 0; id < g_JY.SceneNum; id++)
    {
        auto scene = g_JY.getScene(id);
        int x1 = scene.outerEnterX1(), y1 = scene.outerEnterY1();
        int x2 = scene.outerEnterX2(), y2 = scene.outerEnterY2();
        if (x1 > 0 && y1 > 0)
            g_JY.EnterSceneXY[y1 * g_CC.MWidth + x1] = id;
        if (x2 > 0 && y2 > 0)
            g_JY.EnterSceneXY[y2 * g_CC.MWidth + x2] = id;
    }
    g_JY.EnterSceneXY_Dirty = false;
}

int CanEnterScene(int x, int y)
{
    if (g_JY.EnterSceneXY_Dirty)
        Cal_EnterSceneXY();

    auto it = g_JY.EnterSceneXY.find(y * g_CC.MWidth + x);
    if (it != g_JY.EnterSceneXY.end())
    {
        int id = it->second;
        int e = g_JY.getScene(id).enterCondition();
        if (e == 0) return id;          // 可进
        if (e == 1) return -1;          // 不可进
        if (e == 2)                     // 有轻功高者进
        {
            for (int i = 1; i <= g_CC.TeamNum; i++)
            {
                int pid = g_JY.getBase().team(i);
                if (pid >= 0)
                {
                    if (g_JY.getPerson(pid).agility() >= 70)
                        return id;
                }
            }
        }
    }
    return -1;
}

// ============ 游戏循环 ============
void Game_MMap()
{
    int direct = -1;
    int key = GetKey();
    if (key != -1)
    {
        g_JY.MyTick = 0;
        if (key == GK_ESCAPE)
        {
            MMenu();
            if (g_JY.Status == GAME_FIRSTMMAP) return;
            g_JY.oldMMapX = -1;
            g_JY.oldMMapY = -1;
        }
        else if (key == GK_UP)    direct = 0;
        else if (key == GK_DOWN)  direct = 3;
        else if (key == GK_LEFT)  direct = 2;
        else if (key == GK_RIGHT) direct = 1;
    }

    int x, y;
    if (direct != -1)
    {
        AddMyCurrentPic();
        x = g_JY.getBase().personX() + g_CC.DirectX[direct];
        y = g_JY.getBase().personY() + g_CC.DirectY[direct];
        g_JY.getBase().setPersonDir(direct);
    }
    else
    {
        x = g_JY.getBase().personX();
        y = g_JY.getBase().personY();
    }

    g_JY.SubScene = CanEnterScene(x, y);

    // 没有建筑才可以到达
    if (JY_GetMMap(x, y, 3) == 0 && JY_GetMMap(x, y, 4) == 0)
    {
        g_JY.getBase().setPersonX(x);
        g_JY.getBase().setPersonY(y);
    }
    g_JY.getBase().setPersonX(limitX(g_JY.getBase().personX(), 10, g_CC.MWidth - 10));
    g_JY.getBase().setPersonY(limitX(g_JY.getBase().personY(), 10, g_CC.MHeight - 10));

    // 判断乘船
    if (g_CC.MMapBoat.count(JY_GetMMap(g_JY.getBase().personX(), g_JY.getBase().personY(), 0)))
        g_JY.getBase().setBoatFlag(1);
    else
        g_JY.getBase().setBoatFlag(0);

    int pic = GetMyPic();

    // 绘制主地图
    JY_SetClip(0, 0, g_CC.ScreenW, g_CC.ScreenH);
    JY_DrawMMap(g_JY.getBase().personX(), g_JY.getBase().personY(), pic);

    ShowScreen();
    JY_SetClip(0, 0, 0, 0);

    g_JY.oldMMapX = g_JY.getBase().personX();
    g_JY.oldMMapY = g_JY.getBase().personY();
    g_JY.oldMMapPic = pic;

    if (g_JY.SubScene >= 0)       // 进入子场景
    {
        CleanMemory();
        JY_UnloadMMap();
        JY_PicInit(g_Config.PaletteFile.c_str());
        JY_ShowSlow(50, 1);

        g_JY.Status = GAME_SMAP;
        g_JY.MMAPMusic = -1;

        g_JY.MyPic = GetMyPic();
        g_JY.getBase().setPersonX1(g_JY.getScene(g_JY.SubScene).enterX());
        g_JY.getBase().setPersonY1(g_JY.getScene(g_JY.SubScene).enterY());

        Init_SMap(g_JY.SubScene, 1);
    }
}

void Game_SMap()
{
    // 先绘制场景
    DrawSMap();
    ShowScreen();
    JY_SetClip(0, 0, 0, 0);

    // 检查路过触发事件
    int d_pass = GetS(g_JY.SubScene, g_JY.getBase().personX1(), g_JY.getBase().personY1(), 3);
    if (d_pass >= 0)
    {
        if (d_pass != g_JY.OldDPass)
        {
            EventExecute(d_pass, 3);
            g_JY.OldDPass = d_pass;
            g_JY.oldSMapX = -1;
            g_JY.oldSMapY = -1;
            g_JY.D_Valid_Dirty = true;
        }
    }
    else
    {
        g_JY.OldDPass = -1;
    }

    // 检查是否到出口（出口→返回主地图）
    auto scene = g_JY.getScene(g_JY.SubScene);
    int px1 = g_JY.getBase().personX1();
    int py1 = g_JY.getBase().personY1();
    bool isout = (px1 == scene.exitX1() && py1 == scene.exitY1()) ||
                 (px1 == scene.exitX2() && py1 == scene.exitY2()) ||
                 (px1 == scene.exitX3() && py1 == scene.exitY3());

    if (isout)
    {
        g_JY.Status = GAME_MMAP;
        JY_PicInit(g_Config.PaletteFile.c_str());
        CleanMemory();
        JY_ShowSlow(50, 1);
        if (g_JY.MMAPMusic < 0)
            g_JY.MMAPMusic = scene.exitMusic();
        Init_MMap();
        g_JY.SubScene = -1;
        g_JY.oldSMapX = -1;
        g_JY.oldSMapY = -1;
        JY_DrawMMap(g_JY.getBase().personX(), g_JY.getBase().personY(), GetMyPic());
        JY_ShowSlow(50, 0);
        int k, t, mx, my;
        JY_GetKey(&k, &t, &mx, &my);
        return;
    }

    // 检查是否跳转到其他场景（跳转口坐标不同于出口坐标）
    if (scene.jumpScene() >= 0)
    {
        if (px1 == scene.jumpX1() && py1 == scene.jumpY1())
        {
            int newScene = scene.jumpScene();
            JY_ShowSlow(50, 1);
            auto ns = g_JY.getScene(newScene);
            if (ns.outerEnterX1() == 0 && ns.outerEnterY1() == 0)
            {
                g_JY.getBase().setPersonX1(ns.enterX());
                g_JY.getBase().setPersonY1(ns.enterY());
            }
            else
            {
                g_JY.getBase().setPersonX1(ns.jumpX2());
                g_JY.getBase().setPersonY1(ns.jumpY2());
            }
            g_JY.SubScene = newScene;
            Init_SMap(newScene, 1);
            return;
        }
    }

    // 处理键盘输入
    int key = GetKey();
    int direct = -1;
    if (key != -1)
    {
        g_JY.MyTick = 0;
        if (key == GK_ESCAPE)
        {
            MMenu();
            g_JY.oldSMapX = -1;
            g_JY.oldSMapY = -1;
        }
        else if (key == GK_UP)    direct = 0;
        else if (key == GK_DOWN)  direct = 3;
        else if (key == GK_LEFT)  direct = 2;
        else if (key == GK_RIGHT) direct = 1;
        else if (key == GK_SPACE || key == GK_RETURN)
        {
            if (g_JY.getBase().personDir() >= 0)
            {
                int fx = g_JY.getBase().personX1() + g_CC.DirectX[g_JY.getBase().personDir()];
                int fy = g_JY.getBase().personY1() + g_CC.DirectY[g_JY.getBase().personDir()];
                int d_num = GetS(g_JY.SubScene, fx, fy, 3);
                if (d_num >= 0)
                {
                    EventExecute(d_num, 1);
                    g_JY.oldSMapX = -1;
                    g_JY.oldSMapY = -1;
                    g_JY.D_Valid_Dirty = true;
                }
            }
        }
    }

    if (g_JY.Status != GAME_SMAP) return;

    int x, y;
    if (direct != -1)
    {
        AddMyCurrentPic();
        x = g_JY.getBase().personX1() + g_CC.DirectX[direct];
        y = g_JY.getBase().personY1() + g_CC.DirectY[direct];
        g_JY.getBase().setPersonDir(direct);
    }
    else
    {
        x = g_JY.getBase().personX1();
        y = g_JY.getBase().personY1();
    }

    g_JY.MyPic = GetMyPic();
    DtoSMap();

    if (SceneCanPass(g_JY.SubScene, x, y))
    {
        g_JY.getBase().setPersonX1(x);
        g_JY.getBase().setPersonY1(y);
    }
    g_JY.getBase().setPersonX1(limitX(g_JY.getBase().personX1(), 1, g_CC.SWidth - 2));
    g_JY.getBase().setPersonY1(limitX(g_JY.getBase().personY1(), 1, g_CC.SHeight - 2));
}

void Game_Cycle()
{
    while (g_JY.Status != GAME_END)
    {
        double tstart = JY_GetTime();

        g_JY.MyTick++;
        g_JY.MyTick2++;

        if (g_JY.MyTick == 20)
        {
            g_JY.MyCurrentPic = 0;
            g_JY.MyTick = 0;
        }
        if (g_JY.MyTick2 == 1000)
        {
            g_JY.MyTick2 = 0;
        }

        if (g_JY.Status == GAME_FIRSTMMAP)
        {
            CleanMemory();
            JY_ShowSlow(50, 1);
            g_JY.MmapMusic = 16;
            g_JY.Status = GAME_MMAP;
            Init_MMap();
            JY_DrawMMap(g_JY.getBase().personX(), g_JY.getBase().personY(), GetMyPic());
            JY_ShowSlow(50, 0);
        }
        else if (g_JY.Status == GAME_MMAP)
        {
            Game_MMap();
        }
        else if (g_JY.Status == GAME_SMAP)
        {
            Game_SMap();
        }

        double tend = JY_GetTime();
        int elapsed = (int)(tend - tstart);
        if (elapsed < g_CC.Frame)
        {
            JY_Delay(g_CC.Frame - elapsed);
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

    // 设置主角姓名
    auto person0 = g_JY.getPerson(0);
    person0.setName(g_CC.NewPersonName);

    // 属性随机分配循环
    while (true)
    {
        person0.setMpType(Rnd(2));
        person0.setMaxMp(Rnd(20) + 21);
        person0.setAttack(Rnd(10) + 21);
        person0.setDefense(Rnd(10) + 21);
        person0.setAgility(Rnd(10) + 21);
        person0.setMedic(Rnd(10) + 21);
        person0.setUsePoison(Rnd(10) + 21);
        person0.setDetox(Rnd(10) + 21);
        person0.setAntiPoison(Rnd(10) + 21);
        person0.setFist(Rnd(10) + 21);
        person0.setSword(Rnd(10) + 21);
        person0.setBlade(Rnd(10) + 21);
        person0.setSpecial(Rnd(10) + 21);
        person0.setHidden(Rnd(10) + 21);
        int hpGrow = Rnd(5) + 3;
        person0.setHpGrowth(hpGrow);
        person0.setMaxHp(hpGrow * 3 + 29);

        int rate = Rnd(10);
        if (rate < 2)
            person0.setAptitude(Rnd(35) + 30);
        else if (rate <= 7)
            person0.setAptitude(Rnd(20) + 60);
        else
            person0.setAptitude(Rnd(20) + 75);

        person0.setHp(person0.maxHp());
        person0.setMp(person0.maxMp());

        Cls();

        int fontsize = g_CC.NewGameFontSize;
        int h = fontsize + g_CC.RowPixel;
        int w = fontsize * 4;
        int x1 = (g_CC.ScreenW - w * 4) / 2;
        int y1 = g_CC.NewGameY;

        // 显示提示
        DrawString(x1, y1, "这样的属性满意吗(Y/N)?", C_GOLD, fontsize);
        y1 += h;

        // 绘制属性（4列×3行）
        auto drawAttrib = [&](int col, const char* label, int value) {
            DrawString(x1 + col * w, y1, label, C_RED, fontsize);
            DrawString(x1 + col * w + fontsize * 2, y1, std::format("{:3d}", value), C_WHITE, fontsize);
        };

        drawAttrib(0, "内力", person0.mp());
        drawAttrib(1, "攻击", person0.attack());
        drawAttrib(2, "轻功", person0.agility());
        drawAttrib(3, "防御", person0.defense());
        y1 += h;

        drawAttrib(0, "生命", person0.hp());
        drawAttrib(1, "医疗", person0.medic());
        drawAttrib(2, "用毒", person0.usePoison());
        drawAttrib(3, "解毒", person0.detox());
        y1 += h;

        drawAttrib(0, "拳掌", person0.fist());
        drawAttrib(1, "御剑", person0.sword());
        drawAttrib(2, "耍刀", person0.blade());
        drawAttrib(3, "暗器", person0.hidden());

        ShowScreen();

        // 显示"是/否"菜单
        std::vector<MenuItem> ynMenu = {
            {"是 ", nullptr, 1},
            {"否 ", nullptr, 1}
        };
        int ok = ShowMenu2(ynMenu, 2, x1 + 11 * fontsize,
            g_CC.NewGameY - g_CC.MenuBorderPixel, fontsize, C_RED, C_WHITE);
        if (ok == 1) break;
    }

    // 版本11特殊处理
    if (g_Config.Version == 11)
    {
        g_JY.getScene(52).setJumpX2(51);
        g_JY.getScene(52).setJumpY2(7);
    }

    // 设置新游戏场景
    g_JY.SubScene = g_CC.NewGameSceneID[vi];
    g_JY.getBase().setPersonX1(g_CC.NewGameSceneX[vi]);
    g_JY.getBase().setPersonY1(g_CC.NewGameSceneY[vi]);

    // 设置主角贴图
    g_JY.MyPic = g_CC.NewPersonPic[vi];
}

// ============ 版本选择 ============
void Edition()
{
    std::vector<MenuItem> menu = {
        {"侠客西游", nullptr, 1}, {"苍龙逐日", nullptr, 1},
        {"苍龙极乐", nullptr, 1}, {"天书劫", nullptr, 1},
        {"乡民PTT", nullptr, 1},  {"小猪PTT", nullptr, 1},
        {"群芳726", nullptr, 1},  {"再战江湖", nullptr, 1},
        {"苍龙1028", nullptr, 1}, {"正邪谁判", nullptr, 1},
        {"真龙觉醒", nullptr, 1}, {"天书奇侠", nullptr, 1}
    };
    int n = (int)menu.size();
    int menux = (g_CC.ScreenW - 4 * g_CC.StartMenuFontSize - 2 * g_CC.MenuBorderPixel) / 2;
    int menuy = g_CC.StartMenuY - g_CC.StartMenuFontSize * 4 + g_CC.StartMenuFontSize;
    int r = ShowMenu(menu, n, 4, menux, menuy, 0, 0, 0, 0,
        g_CC.StartMenuFontSize, C_STARTMENU, C_RED);
    if (r < 1) r = 1;
    g_Config.Version = r;
    // 更新 DataPath, 让 g_CC.init 使用正确的数据目录
    g_Config.DataPath = std::format("{}data/{}/", g_Config.CurrentPath, g_Config.Version);
    g_CC.init(g_Config.Version, g_Config.Zoom);
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

    // 版本选择
    Edition();

    // 开始菜单
    g_JY.Status = GAME_START;
    Cls();
    ShowScreen();
    JY_ShowSlow(50, 0);

    std::vector<MenuItem> startMenu = {
        {"重新开始", nullptr, 1},
        {"载入进度", nullptr, 1},
        {"离开游戏", nullptr, 1}
    };
    int menux = (g_CC.ScreenW - 4 * g_CC.StartMenuFontSize - 2 * g_CC.MenuBorderPixel) / 2;
    int r = ShowMenu(startMenu, 3, 0, menux, g_CC.StartMenuY, 0, 0, 0, 0,
        g_CC.StartMenuFontSize, C_STARTMENU, C_RED);

    if (r == 1)
    {
        Cls();
        DrawString(menux, g_CC.StartMenuY, "请稍候...", C_RED, g_CC.StartMenuFontSize);
        ShowScreen();
        NewGame();
        JY_ShowSlow(50, 1);
        g_JY.Status = GAME_SMAP;
        g_JY.MMAPMusic = -1;
        CleanMemory();
        Init_SMap(g_JY.SubScene, 0);

        // 执行新游戏事件（必须在 Init_SMap 之后）
        int vi = g_Config.Version - 1;
        if (vi < 0) vi = 0;
        if (vi > 11) vi = 11;
        if (g_CC.NewGameEvent[vi] > 0)
            ReadKDEF(g_CC.NewGameEvent[vi]);
    }
    else if (r == 2)
    {
        Cls();
        std::vector<MenuItem> loadMenu = {
            {"进度一", nullptr, 1}, {"进度二", nullptr, 1}, {"进度三", nullptr, 1}
        };
        int lr = ShowMenu(loadMenu, 3, 0, menux, g_CC.StartMenuY, 0, 0, 0, 0,
            g_CC.StartMenuFontSize, C_STARTMENU, C_RED);
        if (lr > 0)
        {
            Cls();
            DrawString(menux, g_CC.StartMenuY, "请稍候...", C_RED, g_CC.StartMenuFontSize);
            ShowScreen();
            LoadRecord(lr);
            Cls();
            ShowScreen();
            g_JY.Status = GAME_FIRSTMMAP;
        }
    }
    else if (r == 3)
    {
        JY_LoadPicture("", 0, 0);
        JY_Debug("JY_GameMain end");
        return 0;
    }

    JY_LoadPicture("", 0, 0);
    int k, t, mx, my;
    JY_GetKey(&k, &t, &mx, &my);
    Game_Cycle();

    JY_Debug("JY_GameMain end");
    return 0;
}
