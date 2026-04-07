// GameEvent.cpp - 事件解释器实现
// 从readkdef.lua 转换而来

#include "GameEvent.h"
#include "GameData.h"
#include "GameMain.h"
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
#include <algorithm>
#include <format>

// 前向声明
int WarMain(int warid, int isexp);
void WarLoad(int warid);

X50Var x50[32769];

static std::string g_TalkString = "0"; // 是否使用了talk.grp内容

// ============ 辅助函数 ============
static int filelength(const char* path)
{
    FILE* fp = fopen(path, "rb");
    if (!fp) return 0;
    fseek(fp, 0, SEEK_END);
    int len = (int)ftell(fp);
    fclose(fp);
    return len;
}

void ReadBin()
{
    std::string leave = g_Config.CurrentPath + "list/leave.bin";
    std::string effect = g_Config.CurrentPath + "list/effect.bin";
    std::string match = g_Config.CurrentPath + "list/match.bin";

    // leave.bin - 离队人物列表
    int len = filelength(leave.c_str());
    if (len > 0)
    {
        DataBuffer bin;
        bin.alloc(len);
        bin.loadfile(leave.c_str(), 0, len);
        g_CC.PersonExit[g_Config.Version].clear();
        for (int i = 0; i < len / 2; i++)
        {
            int v = bin.get16(i * 2);
            g_CC.PersonExit[g_Config.Version].push_back({v, i * 2 + 1});
        }
    }

    // effect.bin - 武功特效帧数
    len = filelength(effect.c_str());
    if (len > 0)
    {
        DataBuffer bin;
        bin.alloc(len);
        bin.loadfile(effect.c_str(), 0, len);
        g_CC.Effect.clear();
        for (int i = 0; i < len / 2; i++)
        {
            int v = bin.get16(i * 2);
            g_CC.Effect.push_back(v);
        }
    }
    if (!g_CC.Effect.empty()) g_CC.Effect[0] = g_CC.Effect[0] - 1;

    // match.bin - 武功武器配合
    len = filelength(match.c_str());
    if (len > 0)
    {
        DataBuffer bin;
        bin.alloc(len);
        bin.loadfile(match.c_str(), 0, len);
        g_CC.ExtraOffense.clear();
        for (int i = 0; i < len / 6; i++)
        {
            g_CC.ExtraOffense.push_back({bin.get16(i * 6), bin.get16(i * 6 + 2), bin.get16(i * 6 + 4)});
        }
    }
}

int ReadCol(int id)
{
    DataBuffer col;
    col.alloc(4);
    col.loadfile(g_Config.PaletteFile.c_str(), id * 3, 3);
    int v = col.get32(0);
    int b = (v >> 16) & 0xFF;
    int g = (v >> 8) & 0xFF;
    int r = v & 0xFF;
    return RGB_JY(r * 4, g * 4, b * 4);
}

// ============ x50变量操作辅助 ============
static int getb(int b, int num)
{
    return (num >> b) & 1;
}

static int getvalue(int b, int t, int ee)
{
    if (getb(b, t) == 1) return x50[ee].asInt();
    return ee;
}

static std::string getchar_x50(int ch)
{
    if (ch < 0) ch = 65536 + ch;
    unsigned char l = ch % 256;
    unsigned char h = (unsigned char)(ch / 256);
    std::string r;
    r += (char)l;
    if (h != 0) r += (char)h;
    return r;
}

static std::string x50_readstr(int start)
{
    std::string str;
    for (int i = 0; i < 1000; i++)
    {
        if (x50[start + i].isStr)
        {
            str += x50[start + i].sval;
        }
        else
        {
            int v = x50[start + i].ival;
            if (v == 0) break;
            str += getchar_x50(v);
        }
    }
    return str;
}

static void x50_writestr(int dest, const std::string& str)
{
    int len = (int)((str.length() + 1) / 2);
    for (int i = 0; i < len; i++)
    {
        char buf[3] = {0, 0, 0};
        buf[0] = (i * 2 < (int)str.length()) ? str[i * 2] : 0;
        buf[1] = (i * 2 + 1 < (int)str.length()) ? str[i * 2 + 1] : 0;
        x50[dest + i].setStr(buf);
    }
    x50[dest + len].setInt(0);
}

// ============ ReadTalk ============
std::string ReadTalk(int id)
{
    if (id < 1) return "";
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
        unsigned char byte = (unsigned char)talk.data[i];
        byte = 255 - (byte % 256);
        str += (char)byte;
    }
    const char* enc = (g_CC.SrcCharSet == 1) ? "cp950" : "cp936";
    return PotConv::conv(str, enc, "utf-8");
}

// Forward declaration
std::string GenTalkString_ext(const std::string& str, int n);

static std::string ReadTALK(int id, int flag = 0)
{
    std::string str = ReadTalk(id);
    if (g_TalkString == "0")
        str = GenTalkString_ext(str, 24);
    else
        g_TalkString = "0";
    return str;
}

// 外部使用的GenTalkString（从GameMain.cpp调用的那个是static的，这里重写一个简单版本
static std::string GenTalkString_local(const std::string& str, int n)
{
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

std::string GenTalkString_ext(const std::string& str, int n)
{
    return GenTalkString_local(str, n);
}

// ============ instruct_50 子指令============
static int* g_E = nullptr;
static int g_idx = 0;

static void sub50_0(int e1, int e2, int e3, int e4, int e5, int e6)
{
    x50[e1].setInt(e2);
}

static void sub50_1(int e1, int e2, int e3, int e4, int e5, int e6)
{
    e4 = getvalue(0, e1, e4);
    e5 = getvalue(1, e1, e5);
    if (e2 == 0) x50[e3 + e4].setInt(e5);
    else if (e2 == 1) x50[e3 + e4].setInt(e5 % 256);
}

static void sub50_2(int e1, int e2, int e3, int e4, int e5, int e6)
{
    e4 = getvalue(0, e1, e4);
    int num = getvalue(0, 1, e3 + e4);
    if (e2 == 1) num = num % 256;
    x50[e5].setInt(num);
}

static void sub50_3(int e1, int e2, int e3, int e4, int e5, int e6)
{
    if (e2 == 5) { e4 = getvalue(0, 1, e4); if (e4 < 0) e4 = 65536 + e4; e2 = 3; }
    else { e4 = getvalue(0, 1, e4); }
    e5 = getvalue(0, e1, e5);
    int result = 0;
    if (e2 == 0) result = e4 + e5;
    else if (e2 == 1) result = e4 - e5;
    else if (e2 == 2) result = e4 * e5;
    else if (e2 == 3 && e5 != 0) result = (int)(e4 / e5);
    else if (e2 == 4 && e5 != 0) result = e4 % e5;
    x50[e3].setInt(result);
}

static void sub50_4(int e1, int e2, int e3, int e4, int e5, int e6)
{
    x50[28672].setInt(1);
    e3 = getvalue(0, 1, e3);
    e4 = getvalue(0, e1, e4);
    bool cond = false;
    if (e2 == 0) cond = (e3 < e4);
    else if (e2 == 1) cond = (e3 <= e4);
    else if (e2 == 2) cond = (e3 == e4);
    else if (e2 == 3) cond = (e3 != e4);
    else if (e2 == 4) cond = (e3 >= e4);
    else if (e2 == 5) cond = (e3 > e4);
    else if (e2 == 6) cond = true;
    if (cond) x50[28672].setInt(0);
}

static void sub50_5(int e1, int e2, int e3, int e4, int e5, int e6)
{
    for (int i = 0; i < 32768; i++) x50[i].setInt(0);
}

static void sub50_6(int e1, int e2, int e3, int e4, int e5, int e6) { /* 变量名定义- no-op */ }

static void sub50_8(int e1, int e2, int e3, int e4, int e5, int e6)
{
    e2 = getvalue(0, e1, e2);
    std::string str = ReadTALK(e2, 1);
    int len = (int)((str.length() + 1) / 2);
    for (int i = 0; i < len; i++)
    {
        char buf[3] = {0, 0, 0};
        buf[0] = (i * 2 < (int)str.length()) ? str[i * 2] : 0;
        buf[1] = (i * 2 + 1 < (int)str.length()) ? str[i * 2 + 1] : 0;
        x50[e3 + i].setStr(buf);
    }
    x50[e3 + len].setInt(0);
    g_TalkString = "1";
}

static void sub50_9(int e1, int e2, int e3, int e4, int e5, int e6)
{
    e4 = getvalue(0, e1, e4);
    std::string fmt = x50_readstr(e3);
    char buf[4096];
    snprintf(buf, sizeof(buf), fmt.c_str(), e4);
    std::string result(buf);
    x50_writestr(e2, result);
}

static void sub50_10(int e1, int e2, int e3, int e4, int e5, int e6)
{
    for (int i = 0; i < 1000; i++)
    {
        if (x50[e1 + i].isStr) continue;
        if (x50[e1 + i].ival != 0) continue;
        x50[e2].setInt(i * 2);
        break;
    }
}

static void sub50_11(int e1, int e2, int e3, int e4, int e5, int e6)
{
    std::string stra = x50_readstr(e2);
    std::string strb = x50_readstr(e3);
    std::string strx = stra + strb;
    x50_writestr(e1, strx);
    g_TalkString = "1";
}

static void sub50_12(int e1, int e2, int e3, int e4, int e5, int e6)
{
    e3 = getvalue(0, e1, e3);
    int len = (int)((e3 + 1) / 2);
    for (int i = 0; i < len; i++) x50[e2 + i].setStr("  ");
    x50[e2 + len].setInt(0);
}

static void sub50_16(int e1, int e2, int e3, int e4, int e5, int e6)
{
    e3 = getvalue(0, e1, e3);
    e4 = getvalue(1, e1, e4);
    e5 = getvalue(2, e1, e5);
    if (e2 == 0) g_JY.Data_Person.set16(g_CC.PersonSize * e3 + e4, (int16_t)e5);
    else if (e2 == 1) g_JY.Data_Thing.set16(g_CC.ThingSize * e3 + e4, (int16_t)e5);
    else if (e2 == 2) g_JY.Data_Scene.set16(g_CC.SceneSize * e3 + e4, (int16_t)e5);
    else if (e2 == 3) g_JY.Data_Wugong.set16(g_CC.WugongSize * e3 + e4, (int16_t)e5);
    else if (e2 == 4) g_JY.Data_Shop.set16(g_CC.ShopSize * e3 + e4, (int16_t)e5);
}

static void sub50_17(int e1, int e2, int e3, int e4, int e5, int e6)
{
    e3 = getvalue(0, e1, e3);
    e4 = getvalue(1, e1, e4);
    int v = 0;
    if (e2 == 0) v = g_JY.Data_Person.get16(g_CC.PersonSize * e3 + e4);
    else if (e2 == 1) v = g_JY.Data_Thing.get16(g_CC.ThingSize * e3 + e4);
    else if (e2 == 2) v = g_JY.Data_Scene.get16(g_CC.SceneSize * e3 + e4);
    else if (e2 == 3) v = g_JY.Data_Wugong.get16(g_CC.WugongSize * e3 + e4);
    else if (e2 == 4) v = g_JY.Data_Shop.get16(g_CC.ShopSize * e3 + e4);
    x50[e5].setInt(v);
}

static void sub50_18(int e1, int e2, int e3, int e4, int e5, int e6)
{
    e2 = getvalue(0, e1, e2) + 1;
    e3 = getvalue(1, e1, e3);
    g_JY.getBase().setTeam(e2, e3);
}

static void sub50_19(int e1, int e2, int e3, int e4, int e5, int e6)
{
    e2 = getvalue(0, e1, e2) + 1;
    x50[e3].setInt(g_JY.getBase().team(e2));
}

static void sub50_20(int e1, int e2, int e3, int e4, int e5, int e6)
{
    e2 = getvalue(0, e1, e2);
    auto base = g_JY.getBase();
    for (int i = 1; i <= g_CC.MyThingNum; i++)
    {
        if (base.item(i) == e2) { x50[e3].setInt(base.itemNum(i)); break; }
    }
}

static void sub50_21(int e1, int e2, int e3, int e4, int e5, int e6)
{
    e2 = getvalue(0, e1, e2);
    e3 = getvalue(1, e1, e3);
    e4 = getvalue(2, e1, e4);
    e5 = getvalue(3, e1, e5);
    JY_SetD(e2, e3, e4, e5);
}

static void sub50_22(int e1, int e2, int e3, int e4, int e5, int e6)
{
    e2 = getvalue(0, e1, e2);
    e3 = getvalue(1, e1, e3);
    e4 = getvalue(2, e1, e4);
    x50[e5].setInt(JY_GetD(e2, e3, e4));
}

static void sub50_23(int e1, int e2, int e3, int e4, int e5, int e6)
{
    e2 = getvalue(0, e1, e2);
    e3 = getvalue(1, e1, e3);
    e4 = getvalue(2, e1, e4);
    e5 = getvalue(3, e1, e5);
    e6 = getvalue(4, e1, e6);
    JY_SetS(e2, e4, e5, e3, e6);
}

static void sub50_24(int e1, int e2, int e3, int e4, int e5, int e6)
{
    e2 = getvalue(0, e1, e2);
    e3 = getvalue(1, e1, e3);
    e4 = getvalue(2, e1, e4);
    e5 = getvalue(3, e1, e5);
    x50[e6].setInt(JY_GetS(e2, e4, e5, e3));
}

static void sub50_25(int e1, int e2, int e3, int e4, int e5, int e6)
{
    e5 = getvalue(0, e1, e5);
    e6 = getvalue(1, e1, e6);
    if (e3 < 0) e3 = 65536 + e3;
    if (e4 < 0) e4 = 65536 + e4;
    int address = e4 * 65536 + e3 + e6;
    auto base = g_JY.getBase();
    if (address == 1838072) g_JY.MyPic = e5;
    else if (address == 345330) base.setPersonDir(e5);
    else if (address == 1911134) g_JY.SubScene = e5;
    else if (address == 1911132) base.setPersonX1(e5);
    else if (address == 1911130) base.setPersonY1(e5);
    else if (address == 1837964) base.setPersonX(e5);
    else if (address == 1837960) base.setPersonY(e5);
    else if (address >= 1637932 && address < 1638732)
    {
        int off = address - 1637932;
        int id = 1 + off / 4;
        int kind = off % 4;
        if (kind == 0) base.setItem(id, e5);
        else if (kind == 2) base.setItemNum(id, e5);
    }
}

static void sub50_26(int e1, int e2, int e3, int e4, int e5, int e6)
{
    e6 = getvalue(0, e1, e6);
    if (e3 < 0) e3 = 65536 + e3;
    if (e4 < 0) e4 = 65536 + e4;
    int address = e4 * 65536 + e3 + e6;
    int v = 0;
    auto base = g_JY.getBase();
    if (address == 1838072) v = g_JY.MyPic;
    else if (address == 345330) v = base.personDir();
    else if (address == 1911134) v = g_JY.SubScene;
    else if (address == 1911132) v = base.personX1();
    else if (address == 1911130) v = base.personY1();
    else if (address == 1911128 || address == 1837964) v = base.personX();
    else if (address == 1911126 || address == 1837960) v = base.personY();
    else if (address == 374074) v = (g_Config.Type == 1) ? 1 : 0;
    else if (address >= 1637932 && address <= 1638734)
    {
        int off = address - 1637932;
        int id = 1 + off / 4;
        int kind = off % 4;
        if (kind == 0) v = base.item(id);
        else if (kind == 2) v = base.itemNum(id);
    }
    if (e2 == 0) x50[e5].setInt(v);
    else if (e2 == 1) x50[e5].setInt(v % 256);
}

static void sub50_27(int e1, int e2, int e3, int e4, int e5, int e6)
{
    e3 = getvalue(0, e1, e3);
    std::string str;
    if (e2 == 0) str = g_JY.getPerson(e3).name();
    else if (e2 == 1) str = g_JY.getThing(e3).name();
    else if (e2 == 2) { if (e3 >= 0) str = g_JY.getScene(e3).name(); }
    else if (e2 == 3) str = g_JY.getWugong(e3).name();
    x50_writestr(e4, str);
    g_TalkString = "1";
}

static void sub50_32(int e1, int e2, int e3, int e4, int e5, int e6)
{
    e3 = getvalue(0, e1, e3);
    int v = x50[e2].asInt();
    if (g_E) g_E[g_idx + 8 + e3] = v;
}

static void sub50_33(int e1, int e2, int e3, int e4, int e5, int e6)
{
    e3 = getvalue(0, e1, e3);
    e4 = getvalue(1, e1, e4);
    e5 = getvalue(2, e1, e5);
    if (e5 < 0) e5 = 65536 + e5;
    int colid = e5 % 256;
    int col = ReadCol(colid);
    std::string str = x50_readstr(e2);
    int fontsize = (g_Config.Type == 1) ? 18 : g_CC.DefaultFont;
    g_TalkString = "1";
    DrawString(e3, e4, str, col, fontsize);
    ShowScreen();
}

static void sub50_34(int e1, int e2, int e3, int e4, int e5, int e6)
{
    // 检查下一条指令是否为 50/39 或 50/40，如果是就跳过
    if (g_E && g_E[g_idx + 8] == 50 && (g_E[g_idx + 9] == 39 || g_E[g_idx + 9] == 40))
        return;
    e2 = getvalue(0, e1, e2);
    e3 = getvalue(1, e1, e3);
    e4 = getvalue(2, e1, e4);
    e5 = getvalue(3, e1, e5);
    DrawBox(e2, e3, e2 + e4, e3 + e5, C_WHITE);
}

static void sub50_35(int e1, int e2, int e3, int e4, int e5, int e6)
{
    int key = WaitKey();
    if (key == GK_UP) key = 158;
    else if (key == GK_DOWN) key = 152;
    else if (key == GK_LEFT) key = 154;
    else if (key == GK_RIGHT) key = 156;
    x50[e1].setInt(key);
}

static void sub50_36(int e1, int e2, int e3, int e4, int e5, int e6)
{
    e3 = getvalue(0, e1, e3);
    e4 = getvalue(1, e1, e4);
    e5 = getvalue(2, e1, e5);
    std::string str = x50_readstr(e2);
    DrawStrBox(e3, e4, str, C_ORANGE, g_CC.DefaultFont);
    ShowScreen();
    x50[28672].setInt(1);
    int key = WaitKey();
    if (key == 121 || key == GK_SPACE || key == GK_RETURN)
        x50[28672].setInt(0);
}

static void sub50_37(int e1, int e2, int e3, int e4, int e5, int e6)
{
    e2 = getvalue(0, e1, e2);
    JY_Delay(e2);
}

static void sub50_38(int e1, int e2, int e3, int e4, int e5, int e6)
{
    e2 = getvalue(0, e1, e2);
    x50[e3].setInt(Rnd(e2));
    int k, t, mx, my; JY_GetKey(&k, &t, &mx, &my);
}

static void sub50_39(int e1, int e2, int e3, int e4, int e5, int e6)
{
    e2 = getvalue(0, e1, e2);
    e5 = getvalue(1, e1, e5);
    e6 = getvalue(2, e1, e6);
    std::vector<MenuItem> mymenu;
    for (int i = 0; i < e2; i++)
    {
        int start = x50[e3 + i].asInt();
        std::string str = x50_readstr(start);
        if (g_TalkString == "1") g_TalkString = "0";
        mymenu.push_back({str, nullptr, 1});
    }
    int sel = ShowMenu(mymenu, e2, e2, e5 - 5, e6 - 5, 0, 0, 1, 1, g_CC.DefaultFont, C_ORANGE, C_WHITE);
    x50[e4].setInt(sel);
}

static void sub50_40(int e1, int e2, int e3, int e4, int e5, int e6)
{
    e2 = getvalue(0, e1, e2);
    e5 = getvalue(1, e1, e5);
    e6 = getvalue(2, e1, e6);
    int shownum = 0;
    int raw_e1 = e1;
    if (raw_e1 < 0) raw_e1 = 65536 + raw_e1;
    shownum = raw_e1 / 256;
    std::vector<MenuItem> mymenu;
    for (int i = 0; i < e2; i++)
    {
        int start = x50[e3 + i].asInt();
        std::string str = x50_readstr(start);
        if (g_TalkString == "1") g_TalkString = "0";
        mymenu.push_back({str, nullptr, 1});
    }
    int sel = ShowMenu(mymenu, e2, shownum, e5 - 5, e6 - 5, 0, 0, 1, 1, g_CC.DefaultFont, C_ORANGE, C_WHITE);
    x50[e4].setInt(sel);
}

static void sub50_41(int e1, int e2, int e3, int e4, int e5, int e6)
{
    e3 = getvalue(0, e1, e3);
    e4 = getvalue(1, e1, e4);
    e5 = getvalue(2, e1, e5);
    int id = 0;
    if (e2 == 0) id = 0;
    else if (e2 == 1) { id = 1; e5 = e5 * 2; }
    JY_LoadPNG(id, e5, e3, e4, 1, 0, 100);
    ShowScreen();
}

static void sub50_42(int e1, int e2, int e3, int e4, int e5, int e6)
{
    e2 = getvalue(0, e1, e2);
    e3 = getvalue(1, e1, e3);
    g_JY.getBase().setPersonX(e2);
    g_JY.getBase().setPersonY(e3);
}

static void sub50_43(int e1, int e2, int e3, int e4, int e5, int e6)
{
    e2 = getvalue(0, e1, e2);
    e3 = getvalue(1, e1, e3);
    e4 = getvalue(2, e1, e4);
    e5 = getvalue(3, e1, e5);
    e6 = getvalue(4, e1, e6);
    x50[28928].setInt(e3);
    x50[28929].setInt(e4);
    x50[28930].setInt(e5);
    x50[28931].setInt(e6);
    if (e2 == 202)
    {
        if (e5 == 0) instruct_2(e3, e4);
    }
    else if (e2 == 542)
    {
        JY_PicInit(g_Config.PaletteFile.c_str());
        JY_PicLoadFile(g_CC.SMAPPicFile[0].c_str(), g_CC.SMAPPicFile[1].c_str(), 0, 0, 0);
        int zoom = limitX(g_CC.ScreenW / 800 * g_Config.Zoom, 0, g_CC.ScreenH / 800 * g_Config.Zoom * 100 / 150);
        JY_LoadPNGPath(g_CC.HeadPath.c_str(), 1, g_CC.HeadNum, zoom, "png");
    }
    else if (e2 == 543)
    {
        std::string dreamcol = g_Config.DataPath + "dream.col";
        JY_PicInit(dreamcol.c_str());
        JY_PicLoadFile(g_CC.SMAPPicFile[0].c_str(), g_CC.SMAPPicFile[1].c_str(), 0, 0, 0);
        int zoom = limitX(g_CC.ScreenW / 800 * g_Config.Zoom, 0, g_CC.ScreenH / 800 * g_Config.Zoom * 100 / 150);
        JY_LoadPNGPath(g_CC.HeadPath.c_str(), 1, g_CC.HeadNum, zoom, "png");
    }
    else if (e3 == 544) // 注意原始代码用 e3 比较，不是 e2
    {
        std::string nightcol = g_Config.DataPath + "night.col";
        JY_PicInit(nightcol.c_str());
        JY_PicLoadFile(g_CC.SMAPPicFile[0].c_str(), g_CC.SMAPPicFile[1].c_str(), 0, 0, 0);
        int zoom = limitX(g_CC.ScreenW / 800 * g_Config.Zoom, 0, g_CC.ScreenH / 800 * g_Config.Zoom * 100 / 150);
        JY_LoadPNGPath(g_CC.HeadPath.c_str(), 1, g_CC.HeadNum, zoom, "png");
    }
    else
    {
        ReadKDEF(e2);
    }
}

typedef void (*Sub50Func)(int, int, int, int, int, int);
static Sub50Func sub50_table[] = {
    sub50_0,  sub50_1,  sub50_2,  sub50_3,  sub50_4,  sub50_5,
    sub50_6,  nullptr,  sub50_8,  sub50_9,  sub50_10, sub50_11,
    sub50_12, nullptr,  nullptr,  nullptr,  sub50_16, sub50_17,
    sub50_18, sub50_19, sub50_20, sub50_21, sub50_22, sub50_23,
    sub50_24, sub50_25, sub50_26, sub50_27, nullptr,  nullptr,
    nullptr,  nullptr,  sub50_32, sub50_33, sub50_34, sub50_35,
    sub50_36, sub50_37, sub50_38, sub50_39, sub50_40, sub50_41,
    sub50_42, sub50_43,
};

// ============ jymain.lua instruct函数（在事件循环中使用） ============
static void instruct_24()
{
    // 空操作- 原代码仅 return
}

static void instruct_34_jy(int id, int value)
{
    // 学习武功 - 同instruct_33但含swap逻辑
    // 原lua: 如果武功已满则与第一个互换
    auto p = g_JY.getPerson(id);
    int wugongid = value;
    for (int i = 1; i <= 10; i++)
    {
        if (p.wugong(i) == wugongid) return; // 已经会了
    }
    for (int i = 1; i <= 10; i++)
    {
        if (p.wugong(i) == 0 || p.wugong(i) == -1)
        {
            p.setWugong(i, wugongid);
            p.setWugongLevel(i, 0);
            DrawStrBoxWaitKey(std::format("{} 学会武功 {}", p.name(), g_JY.getWugong(wugongid).name()), C_ORANGE, g_CC.DefaultFont);
            return;
        }
    }
}

static void instruct_45_jy(int id, int value)
{
    int add = AddPersonAttrib(id, "轻功", value);
    DrawStrBoxWaitKey(std::format("{} 轻功 {:+d}", g_JY.getPerson(id).name(), add), C_ORANGE, g_CC.DefaultFont);
}

static void instruct_46_jy(int id, int value)
{
    int add = AddPersonAttrib(id, "内力最大值", value);
    AddPersonAttrib(id, "内力", 0);
    DrawStrBoxWaitKey(std::format("{} 内力最大值{:+d}", g_JY.getPerson(id).name(), add), C_ORANGE, g_CC.DefaultFont);
}

static void instruct_47_jy(int id, int value)
{
    int add = AddPersonAttrib(id, "攻击力", value);
    DrawStrBoxWaitKey(std::format("{} 攻击力{:+d}", g_JY.getPerson(id).name(), add), C_ORANGE, g_CC.DefaultFont);
}

static void instruct_48_jy(int id, int value)
{
    int add = AddPersonAttrib(id, "生命最大值", value);
    AddPersonAttrib(id, "生命", 0);
    if (instruct_16(id))
    {
        DrawStrBoxWaitKey(std::format("{} 生命最大值{:+d}", g_JY.getPerson(id).name(), add), C_ORANGE, g_CC.DefaultFont);
    }
}

static void instruct_49_jy(int personid, int value)
{
    g_JY.getPerson(personid).setByName("内力性质", value);
}

static void instruct_51_jy()
{
    // 问软体娃娃- 随机对话
    std::string str = ReadTALK(2547 + Rnd(18));
    TalkEx(str, 114, 0);
}

static void instruct_52_jy()
{
    DrawStrBoxWaitKey(std::format("你现在的品德指数为 {}", g_JY.getPerson(0).getByName("品德")), C_ORANGE, g_CC.DefaultFont);
}

static void instruct_53_jy()
{
    DrawStrBoxWaitKey(std::format("你现在的声望指数为 {}", g_JY.getPerson(0).getByName("声望")), C_ORANGE, g_CC.DefaultFont);
}

static void instruct_54_jy()
{
    for (int i = 0; i < g_JY.SceneNum; i++)
        g_JY.getScene(i).setByName("进入条件", 0);
    g_JY.getScene(2).setByName("进入条件", 2);
    g_JY.getScene(38).setByName("进入条件", 2);
    g_JY.getScene(75).setByName("进入条件", 1);
    g_JY.getScene(80).setByName("进入条件", 1);
}

static void instruct_57_jy()
{
    // 高昌迷宫劈门
    instruct_27(-1, 7664, 7674);
    for (int i = 0; i <= 56; i += 2)
    {
        double t1 = JY_GetTime();
        if (g_JY.MyPic < 7688 / 2) g_JY.MyPic = (7676 + i) / 2;
        SetD(g_JY.SubScene, 2, 5, i + 7690);
        SetD(g_JY.SubScene, 2, 6, i + 7690);
        SetD(g_JY.SubScene, 2, 7, i + 7690);
        SetD(g_JY.SubScene, 3, 5, i + 7748);
        SetD(g_JY.SubScene, 3, 6, i + 7748);
        SetD(g_JY.SubScene, 3, 7, i + 7748);
        SetD(g_JY.SubScene, 4, 5, i + 7806);
        SetD(g_JY.SubScene, 4, 6, i + 7806);
        SetD(g_JY.SubScene, 4, 7, i + 7806);
        DtoSMap();
        DrawSMap();
        ShowScreen();
        double t2 = JY_GetTime();
        if (t2 - t1 < g_CC.AnimationFrame) JY_Delay((int)(g_CC.AnimationFrame - (t2 - t1)));
    }
}

static void instruct_58_jy()
{
    // 武道大会比武
    int group = 5, num1 = 6, num2 = 3, startwar = 102;
    int flag[6] = {0};

    for (int i = 0; i < group; i++)
    {
        for (int j = 0; j < num1; j++) flag[j] = 0;
        for (int j = 0; j < num2; j++)
        {
            int r;
            while (true) { r = Rnd(num1); if (flag[r] == 0) { flag[r] = 1; break; } }
            int warnum = r + i * num1;
            WarLoad(warnum + startwar);
            std::string tstr = ReadTALK(2854 + warnum);
            // 简化：使用 WAR.Data["敌人1"] 的头像 - 需要从战斗数据获取
            TalkEx(tstr, 0, 0);
            Cls();
            if (WarMain(warnum + startwar, 0))
            {
                Cls();
                Cls(); JY_ShowSlow(50, 0); int k, t, mx, my; JY_GetKey(&k, &t, &mx, &my);
                TalkEx("还有那位前辈肯赐教？", 0, 1);
                Cls();
            }
            else
            {
                instruct_15();
                return;
            }
        }
        if (i < group - 1)
        {
            TalkEx("少侠已连战三场，*可先休息再战。", 70, 0);
            Cls();
            JY_ShowSlow(50, 1); g_JY.Darkness = 1;
            JY_Delay(300);
            auto p = g_JY.getPerson(0);
            if (p.getByName("受伤程度") < 50 && p.getByName("中毒程度") <= 0)
            {
                p.setByName("受伤程度", 0);
                AddPersonAttrib(0, "体力", 999999);
                AddPersonAttrib(0, "内力", 999999);
                AddPersonAttrib(0, "生命", 999999);
            }
            Cls(); JY_ShowSlow(50, 0); int k, t, mx, my; JY_GetKey(&k, &t, &mx, &my);
            TalkEx("我已经休息够了，*有谁要再上？", 0, 1);
            Cls();
        }
    }

    TalkEx("接下来换谁？**．．．．*．．．．***没有人了吗？", 0, 1); Cls();
    TalkEx("如果还没有人要出来向这位*少侠挑战，那麽这武功天下*第一之名，武林盟主之位，*就由这位少侠夺得。**．．．．．．*．．．．．．*．．．．．．*好，恭喜少侠，这武林盟主*之位就由少侠获得，而这柄\"武林神杖\"也由你保管．", 70, 0); Cls();
    TalkEx("恭喜少侠。", 12, 0); Cls();
    TalkEx("小兄弟，恭喜你！", 64, 4); Cls();
    TalkEx("好，今年的武林大会到此已*圆满结束，希望明年各位武*林同道能再到我华山一游．", 19, 0); Cls();

    JY_ShowSlow(50, 1); g_JY.Darkness = 1;
    for (int i = 24; i <= 72; i++)
        instruct_3(-2, i, 0, 0, -1, -1, -1, -1, -1, -1, -2, -2, -2);
    Cls();
    Cls(); JY_ShowSlow(50, 0); int k, t, mx, my; JY_GetKey(&k, &t, &mx, &my);
    TalkEx("历经千辛万苦，我终於打败*群雄，得到这武林盟主之位*及神杖．*但是\"圣堂\"在那呢？*为什麽没人告诉我，难道大*家都不知道．*这会儿又有的找了。", 0, 1); Cls();
    instruct_2(143, 1);
}

static void instruct_59_jy()
{
    // 全体队员离队
    for (int i = g_CC.TeamNum; i >= 2; i--)
    {
        int pid = g_JY.getBase().team(i);
        if (pid >= 0) instruct_21(pid);
    }
    // 使用AllPersonExit设定（如果有的话）
    auto& pexit = g_CC.AllPersonExit;
    int ver = g_Config.Version;
    if (pexit.count(ver))
    {
        for (auto& v : pexit[ver])
        {
            if (v.size() >= 2)
                instruct_3(v[0], v[1], 0, 0, -1, -1, -1, -1, -1, -1, 0, -2, -2);
        }
    }
}

static void instruct_61_jy()
{
    // 判断是否放完14天书
    // 注意：这是bool返回，在dispatch中处理
}
static bool instruct_61_check()
{
    for (int i = 11; i <= 24; i++)
        if (GetD(g_JY.SubScene, i, 5) != 4664) return false;
    return true;
}

static void instruct_62_jy(int id1, int sp1, int ep1, int id2, int sp2, int ep2)
{
    g_JY.MyPic = -1;
    instruct_44(id1, sp1, ep1, id2, sp2, ep2);
    std::string endpng = g_Config.PicturePath + "end.png";
    JY_LoadPicture(endpng.c_str(), -1, -1);
    ShowScreen();
    PlayMIDI(24);
    JY_Delay(5000);
    int k, t, mx, my; JY_GetKey(&k, &t, &mx, &my);
    WaitKey();
    g_JY.Status = GAME_END;
}

static void instruct_64_jy()
{
    // 小宝卖东西
    int headid = 111;
    int id = -1;
    for (int i = 0; i < g_JY.ShopNum; i++)
    {
        if (g_CC.ShopScene.count(i) && g_CC.ShopScene[i].sceneid == g_JY.SubScene)
        { id = i; break; }
    }
    if (id < 0) return;

    TalkEx("这位小哥，看看有什麽需要的，小宝我卖的东西价钱绝*对公道．", headid, 0);

    auto shop = g_JY.getShop(id);
    std::vector<MenuItem> menu;
    for (int i = 1; i <= 5; i++)
    {
        int thingid = shop.getByName("物品" + std::to_string(i));
        if (thingid < 0) thingid = 0;
        int price = shop.getByName("物品价格" + std::to_string(i));
        int num = shop.getByName("物品数量" + std::to_string(i));
        menu.push_back({std::format("{:<12s} {:5d}", g_JY.getThing(thingid).name(), price), nullptr, num > 0 ? 1 : 0});
    }

    int x1 = (g_CC.ScreenW - 9 * g_CC.DefaultFont - 2 * g_CC.MenuBorderPixel) / 2;
    int y1 = (g_CC.ScreenH - 5 * g_CC.DefaultFont - 4 * g_CC.RowPixel - 2 * g_CC.MenuBorderPixel) / 2;
    int r = ShowMenu(menu, 5, 0, x1, y1, 0, 0, 1, 1, g_CC.DefaultFont, C_ORANGE, C_WHITE);

    if (r > 0)
    {
        int price = shop.getByName("物品价格" + std::to_string(r));
        if (!instruct_31(price))
        {
            TalkEx("非常抱歉，你身上的钱似乎不够．", headid, 0);
        }
        else
        {
            int num = shop.getByName("物品数量" + std::to_string(r));
            shop.setByName("物品数量" + std::to_string(r), num - 1);
            instruct_32(g_CC.MoneyID, -price);
            int thingid = shop.getByName("物品" + std::to_string(r));
            instruct_32(thingid, 1);
            TalkEx("大爷买了我小宝的东西，保证绝不後悔。", headid, 0);
        }
    }

    // 设置离开触发事件
    if (g_CC.ShopScene.count(id))
    {
        for (int v : g_CC.ShopScene[id].d_leave)
            instruct_3(-2, v, 0, -2, -1, -1, 939, -1, -1, -1, -2, -2, -2);
    }
}

static void instruct_65_jy()
{
    int id = -1;
    for (int i = 0; i < g_JY.ShopNum; i++)
    {
        if (g_CC.ShopScene.count(i) && g_CC.ShopScene[i].sceneid == g_JY.SubScene)
        { id = i; break; }
    }
    if (id < 0) return;

    instruct_3(-2, g_CC.ShopScene[id].d_shop, 0, -2, -1, -1, -1, -1, -1, -1, -2, -2, -2);
    for (int v : g_CC.ShopScene[id].d_leave)
        instruct_3(-2, v, 0, -2, -1, -1, -1, -1, -1, -1, -2, -2, -2);

    int newid = id + 1;
    if (newid >= 5) newid = 0;
    if (g_CC.ShopScene.count(newid))
        instruct_3(g_CC.ShopScene[newid].sceneid, g_CC.ShopScene[newid].d_shop, 1, -2, 938, -1, -1, 8256, 8256, 8256, -2, -2, -2);
}

// ============ ReadKDEF 主事件解释器 ============
void ReadKDEF(int id)
{
    if (id < 1) return;

    DataBuffer kidx;
    kidx.alloc(8);
    kidx.loadfile(g_CC.KDX.c_str(), id * 4 - 4, 8);
    int idx1 = kidx.get32(0);
    int idx2 = kidx.get32(4);

    int len = idx2 - idx1;
    if (len <= 0) return;

    DataBuffer kdef;
    kdef.alloc(len);
    kdef.loadfile(g_CC.KRP.c_str(), idx1, len);

    len = len / 2;
    std::vector<int> E(len);
    for (int i = 0; i < len; i++)
        E[i] = kdef.get16(2 * i);

    // Crack(E, len); -- 如果需要解密则启用

    int idx = 0;
    g_E = E.data();

    while (idx < len)
    {
        g_idx = idx;
        int cmd = E[idx];

        if (cmd == 0) { Cls(); idx += 1; }
        else if (cmd == 1) { TalkEx(ReadTALK(E[idx+1]), E[idx+2], E[idx+3]); idx += 4; }
        else if (cmd == 2) { instruct_2(E[idx+1], E[idx+2]); idx += 3; }
        else if (cmd == 3)
        {
            instruct_3(E[idx+1], E[idx+2], E[idx+3], E[idx+4], E[idx+5], E[idx+6],
                E[idx+7], E[idx+8], E[idx+9], E[idx+10], E[idx+11], E[idx+12], E[idx+13]);
            idx += 14;
        }
        else if (cmd == 4)
        {
            if (instruct_4(E[idx+1])) idx += E[idx+2]; else idx += E[idx+3];
            idx += 4;
        }
        else if (cmd == 5)
        {
            if (instruct_5()) idx += E[idx+1]; else idx += E[idx+2];
            idx += 3;
        }
        else if (cmd == 6)
        {
            if (WarMain(E[idx+1], E[idx+4])) idx += E[idx+2]; else idx += E[idx+3];
            idx += 5;
        }
        else if (cmd == 7) { idx += 1; break; }
        else if (cmd == 8) { instruct_8(E[idx+1]); idx += 2; }
        else if (cmd == 9)
        {
            if (instruct_9()) idx += E[idx+1]; else idx += E[idx+2];
            idx += 3;
        }
        else if (cmd == 10) { instruct_10(E[idx+1]); idx += 2; }
        else if (cmd == 11)
        {
            if (instruct_11()) idx += E[idx+1]; else idx += E[idx+2];
            idx += 3;
        }
        else if (cmd == 12) { instruct_12(); idx += 1; }
        else if (cmd == 13) { instruct_13(); idx += 1; }
        else if (cmd == 14) { instruct_14(); idx += 1; }
        else if (cmd == 15) { instruct_15(); idx += 2; }
        else if (cmd == 16)
        {
            if (instruct_16(E[idx+1])) idx += E[idx+2]; else idx += E[idx+3];
            idx += 4;
        }
        else if (cmd == 17) { instruct_17(E[idx+1], E[idx+2], E[idx+3], E[idx+4], E[idx+5]); idx += 6; }
        else if (cmd == 18)
        {
            if (instruct_18(E[idx+1])) idx += E[idx+2]; else idx += E[idx+3];
            idx += 4;
        }
        else if (cmd == 19) { instruct_19(E[idx+1], E[idx+2]); idx += 3; }
        else if (cmd == 20)
        {
            if (instruct_20()) idx += E[idx+1]; else idx += E[idx+2];
            idx += 3;
        }
        else if (cmd == 21) { instruct_21(E[idx+1]); idx += 2; }
        else if (cmd == 22) { instruct_22(); idx += 1; }
        else if (cmd == 23) { instruct_23(E[idx+1], E[idx+2]); idx += 3; }
        else if (cmd == 24) { instruct_24(); idx += 1; }
        else if (cmd == 25) { instruct_25(E[idx+1], E[idx+2], E[idx+3], E[idx+4]); idx += 5; }
        else if (cmd == 26) { instruct_26(E[idx+1], E[idx+2], E[idx+3], E[idx+4], E[idx+5]); idx += 6; }
        else if (cmd == 27) { instruct_27(E[idx+1], E[idx+2], E[idx+3]); idx += 4; }
        else if (cmd == 28)
        {
            if (instruct_28(E[idx+1], E[idx+2], E[idx+3])) idx += E[idx+4]; else idx += E[idx+5];
            idx += 6;
        }
        else if (cmd == 29)
        {
            if (instruct_29(E[idx+1], E[idx+2], E[idx+3])) idx += E[idx+4]; else idx += E[idx+5];
            idx += 6;
        }
        else if (cmd == 30) { instruct_30(E[idx+1], E[idx+2], E[idx+3], E[idx+4]); idx += 5; }
        else if (cmd == 31)
        {
            if (instruct_31(E[idx+1])) idx += E[idx+2]; else idx += E[idx+3];
            idx += 4;
        }
        else if (cmd == 32) { instruct_32(E[idx+1], E[idx+2]); idx += 3; }
        else if (cmd == 33) { instruct_33(E[idx+1], E[idx+2], E[idx+3]); idx += 4; }
        else if (cmd == 34) { instruct_34_jy(E[idx+1], E[idx+2]); idx += 3; }
        else if (cmd == 35) { instruct_35(E[idx+1], E[idx+2], E[idx+3], E[idx+4]); idx += 5; }
        else if (cmd == 36)
        {
            if (E[idx+1] < 256)
            {
                if (instruct_36(E[idx+1])) idx += E[idx+2]; else idx += E[idx+3];
            }
            else
            {
                int tzflag = x50[28672].asInt();
                if (tzflag == 0) idx += E[idx+2]; else idx += E[idx+3];
            }
            idx += 4;
        }
        else if (cmd == 37) { instruct_37(E[idx+1]); idx += 2; }
        else if (cmd == 38) { instruct_38(E[idx+1], E[idx+2], E[idx+3], E[idx+4]); idx += 5; }
        else if (cmd == 39) { instruct_39(E[idx+1]); idx += 2; }
        else if (cmd == 40) { instruct_40(E[idx+1]); idx += 2; }
        else if (cmd == 41) { instruct_41(E[idx+1], E[idx+2], E[idx+3]); idx += 4; }
        else if (cmd == 42)
        {
            if (instruct_42()) idx += E[idx+1]; else idx += E[idx+2];
            idx += 3;
        }
        else if (cmd == 43)
        {
            if (instruct_43(E[idx+1])) idx += E[idx+2]; else idx += E[idx+3];
            idx += 4;
        }
        else if (cmd == 44) { instruct_44(E[idx+1], E[idx+2], E[idx+3], E[idx+4], E[idx+5], E[idx+6]); idx += 7; }
        else if (cmd == 45) { instruct_45_jy(E[idx+1], E[idx+2]); idx += 3; }
        else if (cmd == 46) { instruct_46_jy(E[idx+1], E[idx+2]); idx += 3; }
        else if (cmd == 47) { instruct_47_jy(E[idx+1], E[idx+2]); idx += 3; }
        else if (cmd == 48) { instruct_48_jy(E[idx+1], E[idx+2]); idx += 3; }
        else if (cmd == 49) { instruct_49_jy(E[idx+1], E[idx+2]); idx += 3; }
        else if (cmd == 50)
        {
            if (E[idx+1] > 128)
            {
                // 老版本判断物品
                if (instruct_50(E[idx+1], E[idx+2], E[idx+3], E[idx+4], E[idx+5]))
                    idx += E[idx+6];
                else
                    idx += E[idx+7];
            }
            else
            {
                int code = E[idx+1];
                if (code >= 0 && code < (int)(sizeof(sub50_table)/sizeof(sub50_table[0])) && sub50_table[code])
                    sub50_table[code](E[idx+2], E[idx+3], E[idx+4], E[idx+5], E[idx+6], E[idx+7]);
                else
                    JY_Debug("Unknown sub50 code: %d", code);
            }
            idx += 8;
        }
        else if (cmd == 51) { instruct_51_jy(); idx += 1; }
        else if (cmd == 52) { instruct_52_jy(); idx += 1; }
        else if (cmd == 53) { instruct_53_jy(); idx += 1; }
        else if (cmd == 54) { instruct_54_jy(); idx += 1; }
        else if (cmd == 55)
        {
            if (instruct_55(E[idx+1], E[idx+2])) idx += E[idx+3]; else idx += E[idx+4];
            idx += 5;
        }
        else if (cmd == 56) { instruct_56(E[idx+1]); idx += 2; }
        else if (cmd == 57) { instruct_57_jy(); idx += 1; }
        else if (cmd == 58) { instruct_58_jy(); idx += 1; }
        else if (cmd == 59) { instruct_59_jy(); idx += 1; }
        else if (cmd == 60)
        {
            if (instruct_60(E[idx+1], E[idx+2], E[idx+3])) idx += E[idx+4]; else idx += E[idx+5];
            idx += 6;
        }
        else if (cmd == 61)
        {
            if (instruct_61_check()) idx += E[idx+1]; else idx += E[idx+2];
            idx += 3;
        }
        else if (cmd == 62) { instruct_62_jy(E[idx+1], E[idx+2], E[idx+3], E[idx+4], E[idx+5], E[idx+6]); idx += 7; }
        else if (cmd == 63) { instruct_63(E[idx+1], E[idx+2]); idx += 3; }
        else if (cmd == 64) { instruct_64_jy(); idx += 1; }
        else if (cmd == 65) { instruct_65_jy(); idx += 1; }
        else if (cmd == 66) { instruct_66(E[idx+1]); idx += 2; }
        else if (cmd == 67) { instruct_67(E[idx+1]); idx += 2; }
        else { JY_Debug("Unknown instruction: %d at idx %d", cmd, idx); break; }
    }

    g_E = nullptr;
}
