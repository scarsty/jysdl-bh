// GameData.cpp - 游戏数据实现
// 从 jyconst.lua 转换而来

#include "GameData.h"
#include "jymain.h"
#include "sdlfun.h"
#include "mainmap.h"
#include "charset.h"
#include "INIReader.h"
#include <cstdio>
#include <cstdlib>
#include <cmath>

// ============== 全局变量 ==============
GameConfig g_Config;
GameConst g_CC;
GameState g_JY;

int C_STARTMENU;
int C_RED;
int C_WHITE;
int C_ORANGE;
int C_GOLD;
int C_BLACK;

// ============== 工具函数 ==============
int Rnd(int i)
{
    if (i <= 0) return 0;
    return rand() % i;
}

std::string charsetConvert(const std::string& str, int flag)
{
    // flag=0: GBK->UTF-8 (for display), flag=1: UTF-8->GBK (for file)
    char dest[2048] = {0};
    JY_CharSet(str.c_str(), dest, flag);
    return std::string(dest);
}

// ============== DataBuffer 实现 ==============
bool DataBuffer::loadfile(const char* filename, int start, int len)
{
    FILE* fp = fopen(filename, "rb");
    if (!fp) return false;
    fseek(fp, start, SEEK_SET);
    int readlen = (int)fread(data, 1, len, fp);
    fclose(fp);
    return readlen == len;
}

bool DataBuffer::savefile(const char* filename, int start, int len)
{
    // 以"r+b"打开已存在文件或"wb"创建新文件
    FILE* fp = fopen(filename, "r+b");
    if (!fp) fp = fopen(filename, "wb");
    if (!fp) return false;
    fseek(fp, start, SEEK_SET);
    int writelen = (int)fwrite(data, 1, len, fp);
    fclose(fp);
    return writelen == len;
}

// ============== Accessor name/string methods ==============
std::string PersonAccessor::name() const
{
    std::string raw = buf->getstr(baseOffset + 8, 10);
    if (g_CC.SrcCharSet == 0) return charsetConvert(raw, 0);
    return raw;
}

std::string PersonAccessor::nickname() const
{
    std::string raw = buf->getstr(baseOffset + 18, 10);
    if (g_CC.SrcCharSet == 0) return charsetConvert(raw, 0);
    return raw;
}

void PersonAccessor::setName(const std::string& s)
{
    std::string raw = (g_CC.SrcCharSet == 0) ? charsetConvert(s, 1) : s;
    buf->setstr(baseOffset + 8, 10, raw);
}

void PersonAccessor::setNickname(const std::string& s)
{
    std::string raw = (g_CC.SrcCharSet == 0) ? charsetConvert(s, 1) : s;
    buf->setstr(baseOffset + 18, 10, raw);
}

std::string ThingAccessor::name() const
{
    std::string raw = buf->getstr(baseOffset + 2, 20);
    if (g_CC.SrcCharSet == 0) return charsetConvert(raw, 0);
    return raw;
}

std::string ThingAccessor::name2() const
{
    std::string raw = buf->getstr(baseOffset + 22, 20);
    if (g_CC.SrcCharSet == 0) return charsetConvert(raw, 0);
    return raw;
}

std::string ThingAccessor::desc() const
{
    std::string raw = buf->getstr(baseOffset + 42, 30);
    if (g_CC.SrcCharSet == 0) return charsetConvert(raw, 0);
    return raw;
}

std::string SceneAccessor::name() const
{
    std::string raw = buf->getstr(baseOffset + 2, 10);
    if (g_CC.SrcCharSet == 0) return charsetConvert(raw, 0);
    return raw;
}

std::string WugongAccessor::name() const
{
    std::string raw = buf->getstr(baseOffset + 2, 10);
    if (g_CC.SrcCharSet == 0) return charsetConvert(raw, 0);
    return raw;
}

std::string WarDataDef::name() const
{
    std::string raw = buf.getstr(2, 10);
    if (g_CC.SrcCharSet == 0) return charsetConvert(raw, 0);
    return raw;
}

// ============== GameState 访问器工厂 ==============
PersonAccessor GameState::getPerson(int i)
{
    PersonAccessor a;
    a.buf = &Data_Person;
    a.baseOffset = i * g_CC.PersonSize;
    return a;
}

ThingAccessor GameState::getThing(int i)
{
    ThingAccessor a;
    a.buf = &Data_Thing;
    a.baseOffset = i * g_CC.ThingSize;
    return a;
}

SceneAccessor GameState::getScene(int i)
{
    SceneAccessor a;
    a.buf = &Data_Scene;
    a.baseOffset = i * g_CC.SceneSize;
    return a;
}

WugongAccessor GameState::getWugong(int i)
{
    WugongAccessor a;
    a.buf = &Data_Wugong;
    a.baseOffset = i * g_CC.WugongSize;
    return a;
}

ShopAccessor GameState::getShop(int i)
{
    ShopAccessor a;
    a.buf = &Data_Shop;
    a.baseOffset = i * g_CC.ShopSize;
    return a;
}

// ============== 按名字访问（用于事件系统等需要动态字段名的场景）==============
// Person
static const std::map<std::string, int>& getPersonFieldMap()
{
    static std::map<std::string, int> m;
    if (m.empty())
    {
        m["代号"] = 0; m["头像代号"] = 2; m["生命增长"] = 4;
        m["性别"] = 28; m["等级"] = 30; m["经验"] = 32;
        m["生命"] = 34; m["生命最大值"] = 36; m["受伤程度"] = 38;
        m["中毒程度"] = 40; m["体力"] = 42; m["物品修炼点数"] = 44;
        m["武器"] = 46; m["防具"] = 48;
        m["内力性质"] = 80; m["内力"] = 82; m["内力最大值"] = 84;
        m["攻击力"] = 86; m["轻功"] = 88; m["防御力"] = 90;
        m["医疗能力"] = 92; m["用毒能力"] = 94; m["解毒能力"] = 96;
        m["抗毒能力"] = 98; m["拳掌功夫"] = 100; m["御剑能力"] = 102;
        m["耍刀技巧"] = 104; m["特殊兵器"] = 106; m["暗器技巧"] = 108;
        m["武学常识"] = 110; m["品德"] = 112; m["攻击带毒"] = 114;
        m["左右互搏"] = 116; m["声望"] = 118; m["资质"] = 120;
        m["修炼物品"] = 122; m["修炼点数"] = 124;
        for (int i = 1; i <= 5; i++)
        {
            char buf[64];
            snprintf(buf, sizeof(buf), "出招动画帧数%d", i);
            m[buf] = 50 + 2 * (i - 1);
            snprintf(buf, sizeof(buf), "出招动画延迟%d", i);
            m[buf] = 60 + 2 * (i - 1);
            snprintf(buf, sizeof(buf), "武功音效延迟%d", i);
            m[buf] = 70 + 2 * (i - 1);
        }
        for (int i = 1; i <= 10; i++)
        {
            char buf[64];
            snprintf(buf, sizeof(buf), "武功%d", i);
            m[buf] = 126 + 2 * (i - 1);
            snprintf(buf, sizeof(buf), "武功等级%d", i);
            m[buf] = 146 + 2 * (i - 1);
        }
        for (int i = 1; i <= 4; i++)
        {
            char buf[64];
            snprintf(buf, sizeof(buf), "携带物品%d", i);
            m[buf] = 166 + 2 * (i - 1);
            snprintf(buf, sizeof(buf), "携带物品数量%d", i);
            m[buf] = 174 + 2 * (i - 1);
        }
    }
    return m;
}

int PersonAccessor::getByName(const std::string& fieldName) const
{
    auto& m = getPersonFieldMap();
    auto it = m.find(fieldName);
    if (it == m.end()) return 0;
    int off = it->second;
    if (off == 32) return (int)buf->getu16(baseOffset + off); // 经验 unsigned
    return (int)buf->get16(baseOffset + off);
}

void PersonAccessor::setByName(const std::string& fieldName, int value)
{
    auto& m = getPersonFieldMap();
    auto it = m.find(fieldName);
    if (it == m.end()) return;
    int off = it->second;
    buf->set16(baseOffset + off, (int16_t)value);
}

std::string PersonAccessor::getStrByName(const std::string& fieldName) const
{
    if (fieldName == "姓名") return name();
    if (fieldName == "外号") return nickname();
    return "";
}

void PersonAccessor::setStrByName(const std::string& fieldName, const std::string& value)
{
    if (fieldName == "姓名") setName(value);
    else if (fieldName == "外号") setNickname(value);
}

// Thing
int ThingAccessor::getByName(const std::string& fieldName) const
{
    static std::map<std::string, int> m;
    if (m.empty())
    {
        m["代号"] = 0; m["练出武功"] = 72; m["暗器动画编号"] = 74;
        m["使用人"] = 76; m["装备类型"] = 78; m["显示物品说明"] = 80;
        m["类型"] = 82; m["未知5"] = 84; m["未知6"] = 86; m["未知7"] = 88;
        m["加生命"] = 90; m["加生命最大值"] = 92; m["加中毒解毒"] = 94;
        m["加体力"] = 96; m["改变内力性质"] = 98; m["加内力"] = 100;
        m["加内力最大值"] = 102; m["加攻击力"] = 104; m["加轻功"] = 106;
        m["加防御力"] = 108; m["加医疗能力"] = 110; m["加用毒能力"] = 112;
        m["加解毒能力"] = 114; m["加抗毒能力"] = 116; m["加拳掌功夫"] = 118;
        m["加御剑能力"] = 120; m["加耍刀技巧"] = 122; m["加特殊兵器"] = 124;
        m["加暗器技巧"] = 126; m["加武学常识"] = 128; m["加品德"] = 130;
        m["加攻击次数"] = 132; m["加攻击带毒"] = 134;
        m["仅修炼人物"] = 136; m["需内力性质"] = 138; m["需内力"] = 140;
        m["需攻击力"] = 142; m["需轻功"] = 144; m["需用毒能力"] = 146;
        m["需医疗能力"] = 148; m["需解毒能力"] = 150; m["需拳掌功夫"] = 152;
        m["需御剑能力"] = 154; m["需耍刀技巧"] = 156; m["需特殊兵器"] = 158;
        m["需暗器技巧"] = 160; m["需资质"] = 162; m["需经验"] = 164;
        m["练出物品需经验"] = 166; m["需材料"] = 168;
        for (int i = 1; i <= 5; i++)
        {
            char buf[64];
            snprintf(buf, sizeof(buf), "练出物品%d", i);
            m[buf] = 170 + 2 * (i - 1);
            snprintf(buf, sizeof(buf), "需要物品数量%d", i);
            m[buf] = 180 + 2 * (i - 1);
        }
    }
    auto it = m.find(fieldName);
    if (it == m.end()) return 0;
    return (int)buf->get16(baseOffset + it->second);
}

void ThingAccessor::setByName(const std::string& fieldName, int value)
{
    // 复用 getByName 的 map
    static std::map<std::string, int> m;
    if (m.empty())
    {
        m["使用人"] = 76; m["类型"] = 82;
        m["练出武功"] = 72; m["装备类型"] = 78;
        // 如果需要更多可以按需扩展
    }
    auto it = m.find(fieldName);
    if (it == m.end()) return;
    buf->set16(baseOffset + it->second, (int16_t)value);
}

std::string ThingAccessor::getStrByName(const std::string& fieldName) const
{
    if (fieldName == "名称") return name();
    if (fieldName == "名称2") return name2();
    if (fieldName == "物品说明") return desc();
    return "";
}

// Scene
int SceneAccessor::getByName(const std::string& fieldName) const
{
    static std::map<std::string, int> m;
    if (m.empty())
    {
        m["代号"] = 0; m["出门音乐"] = 12; m["进门音乐"] = 14;
        m["跳转场景"] = 16; m["进入条件"] = 18;
        m["外景入口X1"] = 20; m["外景入口Y1"] = 22;
        m["外景入口X2"] = 24; m["外景入口Y2"] = 26;
        m["入口X"] = 28; m["入口Y"] = 30;
        m["出口X1"] = 32; m["出口X2"] = 34; m["出口X3"] = 36;
        m["出口Y1"] = 38; m["出口Y2"] = 40; m["出口Y3"] = 42;
        m["跳转口X1"] = 44; m["跳转口Y1"] = 46;
        m["跳转口X2"] = 48; m["跳转口Y2"] = 50;
    }
    auto it = m.find(fieldName);
    if (it == m.end()) return 0;
    return (int)buf->get16(baseOffset + it->second);
}

void SceneAccessor::setByName(const std::string& fieldName, int value)
{
    static std::map<std::string, int> m;
    if (m.empty())
    {
        m["跳转口X2"] = 48; m["跳转口Y2"] = 50;
        m["跳转场景"] = 16; m["进入条件"] = 18;
        m["外景入口X1"] = 20; m["外景入口Y1"] = 22;
        m["外景入口X2"] = 24; m["外景入口Y2"] = 26;
        m["入口X"] = 28; m["入口Y"] = 30;
        m["出口X1"] = 32; m["出口X2"] = 34; m["出口X3"] = 36;
        m["出口Y1"] = 38; m["出口Y2"] = 40; m["出口Y3"] = 42;
        m["出门音乐"] = 12; m["进门音乐"] = 14;
    }
    auto it = m.find(fieldName);
    if (it == m.end()) return;
    buf->set16(baseOffset + it->second, (int16_t)value);
}

std::string SceneAccessor::getStrByName(const std::string& fieldName) const
{
    if (fieldName == "名称") return name();
    return "";
}

// Wugong
int WugongAccessor::getByName(const std::string& fieldName) const
{
    static std::map<std::string, int> m;
    if (m.empty())
    {
        m["代号"] = 0; m["出招音效"] = 22; m["武功类型"] = 24;
        m["武功动画&音效"] = 26; m["伤害类型"] = 28;
        m["攻击范围"] = 30; m["消耗内力点数"] = 32; m["敌人中毒点数"] = 34;
        for (int i = 1; i <= 10; i++)
        {
            char buf[64];
            snprintf(buf, sizeof(buf), "攻击力%d", i);
            m[buf] = 36 + 2 * (i - 1);
            snprintf(buf, sizeof(buf), "移动范围%d", i);
            m[buf] = 56 + 2 * (i - 1);
            snprintf(buf, sizeof(buf), "杀伤范围%d", i);
            m[buf] = 76 + 2 * (i - 1);
            snprintf(buf, sizeof(buf), "加内力%d", i);
            m[buf] = 96 + 2 * (i - 1);
            snprintf(buf, sizeof(buf), "杀内力%d", i);
            m[buf] = 116 + 2 * (i - 1);
        }
    }
    auto it = m.find(fieldName);
    if (it == m.end()) return 0;
    return (int)buf->get16(baseOffset + it->second);
}

std::string WugongAccessor::getStrByName(const std::string& fieldName) const
{
    if (fieldName == "名称") return name();
    return "";
}

// Base
int BaseAccessor::getByName(const std::string& fieldName) const
{
    static std::map<std::string, int> m;
    if (m.empty())
    {
        m["乘船"] = 0; m["人X"] = 4; m["人Y"] = 6;
        m["人X1"] = 8; m["人Y1"] = 10; m["人方向"] = 12;
        m["船X"] = 14; m["船Y"] = 16; m["船X1"] = 18; m["船Y1"] = 20; m["船方向"] = 22;
        for (int i = 1; i <= 6; i++)
        {
            char b[32];
            snprintf(b, sizeof(b), "队伍%d", i);
            m[b] = 24 + 2 * (i - 1);
        }
        for (int i = 1; i <= 200; i++)
        {
            char b[32];
            snprintf(b, sizeof(b), "物品%d", i);
            m[b] = 36 + 4 * (i - 1);
            snprintf(b, sizeof(b), "物品数量%d", i);
            m[b] = 36 + 4 * (i - 1) + 2;
        }
    }
    auto it = m.find(fieldName);
    if (it == m.end()) return 0;
    return (int)buf->get16(it->second);
}

void BaseAccessor::setByName(const std::string& fieldName, int value)
{
    static std::map<std::string, int> m;
    if (m.empty())
    {
        m["乘船"] = 0; m["人X"] = 4; m["人Y"] = 6;
        m["人X1"] = 8; m["人Y1"] = 10; m["人方向"] = 12;
        for (int i = 1; i <= 6; i++)
        {
            char b[32];
            snprintf(b, sizeof(b), "队伍%d", i);
            m[b] = 24 + 2 * (i - 1);
        }
        for (int i = 1; i <= 200; i++)
        {
            char b[32];
            snprintf(b, sizeof(b), "物品%d", i);
            m[b] = 36 + 4 * (i - 1);
            snprintf(b, sizeof(b), "物品数量%d", i);
            m[b] = 36 + 4 * (i - 1) + 2;
        }
    }
    auto it = m.find(fieldName);
    if (it == m.end()) return;
    buf->set16(it->second, (int16_t)value);
}

// ============== GameConfig 加载 ==============
void GameConfig::loadFromINI(const std::string& filename)
{
    INIReaderNormal ini;
    if (ini.loadFile(filename) != 0)
    {
        JY_Error("Cannot load config file: %s", filename.c_str());
        return;
    }

    Debug = ini["Main"]["Debug"].toInt();
    Type = ini["Main"]["Type"].toInt();
    Width = ini["Main"]["Width"].toInt();
    Height = ini["Main"]["Height"].toInt();
    bpp = ini["Main"]["bpp"].toInt();
    FullScreen = ini["Main"]["FullScreen"].toInt();
    EnableSound = ini["Main"]["EnableSound"].toInt();
    KeyRepeat = ini["Main"]["KeyRepeat"].toInt();
    KeyRepeatDelay = ini["Main"]["KeyRepeatDelay"].toInt();
    KeyRepeatInterval = ini["Main"]["KeyRepeatInterval"].toInt();
    XScale = ini["Main"]["XScale"].toInt();
    YScale = ini["Main"]["YScale"].toInt();
    OSCharSet = ini["Main"]["OSCharSet"].toInt();
    LargeMemory = ini["Main"]["LargeMemory"].toInt();
    MP3 = ini["Main"]["MP3"].toInt();
    MusicVolume = ini["Main"]["MusicVolume"].toInt();
    SoundVolume = ini["Main"]["SoundVolume"].toInt();
    FastShowScreen = ini["Main"]["FastShowScreen"].toInt();
    Zoom = ini["Main"]["Zoom"].toInt();
    Version = ini["Main"]["Version"].toInt();
    CleanMemory = ini["Main"]["CleanMemory"].toInt();
    MAXCacheNum = ini["Main"]["MAXCacheNum"].toInt();
    LoadFullS = ini["Main"]["LoadFullS"].toInt();
    LoadMMapScope = ini["Main"]["LoadMMapScope"].toInt();
    Operation = ini["Main"]["Operation"].toInt();
    PlayName = ini["Main"]["PlayName"].toString();
    if (PlayName.empty()) PlayName = "徐小侠";

    CurrentPath = ini["Path"]["CurrentPath"].toString();
    if (CurrentPath.empty()) CurrentPath = "./";
    PicturePath = ini["Path"]["PicturePath"].toString();
    if (PicturePath.empty()) PicturePath = CurrentPath + "pic/";
    MusicPath = ini["Path"]["MusicPath"].toString();
    if (MusicPath.empty()) MusicPath = CurrentPath + "music/";
    SoundPath = ini["Path"]["SoundPath"].toString();
    if (SoundPath.empty()) SoundPath = CurrentPath + "sound/";
    ScriptPath = ini["Path"]["ScriptPath"].toString();
    if (ScriptPath.empty()) ScriptPath = CurrentPath + "script/";
    PaletteFile = ini["Path"]["PaletteFile"].toString();
    if (PaletteFile.empty()) PaletteFile = CurrentPath + "mmap.col";
    FontName = ini["Path"]["FontName"].toString();
    if (FontName.empty()) FontName = CurrentPath + "font/font.ttc";
    MidSF2 = ini["Path"]["MidSF2"].toString();
    if (MidSF2.empty() && MP3 == 0) MidSF2 = MusicPath + "mid.sf2";

    MMapAddX = ini["Map"]["MMapAddX"].toInt();
    MMapAddY = ini["Map"]["MMapAddY"].toInt();
    SMapAddX = ini["Map"]["SMapAddX"].toInt();
    SMapAddY = ini["Map"]["SMapAddY"].toInt();
    WMapAddX = ini["Map"]["WMapAddX"].toInt();
    WMapAddY = ini["Map"]["WMapAddY"].toInt();

    // 计算 XScale/YScale
    if (Zoom > 100)
    {
        XScale = (int)(18 * Zoom / 100);
        YScale = (int)(9 * Zoom / 100);
    }

    // DataPath
    char buf[256];
    snprintf(buf, sizeof(buf), "%sdata/%d/", CurrentPath.c_str(), Version);
    DataPath = buf;
}

// ============== GameConst 初始化 ==============
void GameConst::init(int version, int zoom)
{
    // 颜色常量
    C_STARTMENU = RGB_JY(132, 0, 4);
    C_RED = RGB_JY(216, 20, 24);
    C_WHITE = RGB_JY(236, 236, 236);
    C_ORANGE = RGB_JY(252, 148, 16);
    C_GOLD = RGB_JY(236, 200, 40);
    C_BLACK = RGB_JY(0, 0, 0);

    SrcCharSet = 0;
    OSCharSet = g_Config.OSCharSet;
    FontName = g_Config.FontName;

    ScreenW = g_ScreenW;
    ScreenH = g_ScreenH;

    DataPath = g_Config.DataPath;

    XScale = g_Config.XScale;
    YScale = g_Config.YScale;

    // 文件路径
    for (int i = 0; i < 4; i++)
    {
        char buf[512];
        if (i == 0)
        {
            snprintf(buf, sizeof(buf), "%sranger.idx", DataPath.c_str());
            R_IDXFilename[i] = buf;
            snprintf(buf, sizeof(buf), "%sranger.grp", DataPath.c_str());
            R_GRPFilename[i] = buf;
            snprintf(buf, sizeof(buf), "%sallsin.grp", DataPath.c_str());
            S_Filename[i] = buf;
            snprintf(buf, sizeof(buf), "%salldef.grp", DataPath.c_str());
            D_Filename[i] = buf;
        }
        else
        {
            snprintf(buf, sizeof(buf), "%ssave/r%d.idx", DataPath.c_str(), i);
            R_IDXFilename[i] = buf;
            snprintf(buf, sizeof(buf), "%ssave/r%d.grp", DataPath.c_str(), i);
            R_GRPFilename[i] = buf;
            snprintf(buf, sizeof(buf), "%ssave/s%d.grp", DataPath.c_str(), i);
            S_Filename[i] = buf;
            snprintf(buf, sizeof(buf), "%ssave/d%d.grp", DataPath.c_str(), i);
            D_Filename[i] = buf;
        }
    }
    {
        char buf[512];
        snprintf(buf, sizeof(buf), "%sallsinbk.grp", DataPath.c_str());
        TempS_Filename = buf;
    }

    PaletteFile = g_Config.PaletteFile;
    FirstFile = g_Config.PicturePath + "title.png";
    DeadFile = g_Config.PicturePath + "dead.png";

    // MMap files
    {
        const char* names[] = { "earth.002", "surface.002", "building.002", "buildx.002", "buildy.002" };
        for (int i = 0; i < 5; i++)
        {
            MMapFile[i] = DataPath + names[i];
        }
    }

    MMAPPicFile[0] = DataPath + "mmap/mmap.idx";
    MMAPPicFile[1] = DataPath + "mmap/mmap.grp";
    SMAPPicFile[0] = DataPath + "smap/smap.idx";
    SMAPPicFile[1] = DataPath + "smap/smap.grp";
    WMAPPicFile[0] = DataPath + "wmap/wmap.idx";
    WMAPPicFile[1] = DataPath + "wmap/wmap.grp";
    EffectFile[0] = DataPath + "Eft.idx";
    EffectFile[1] = DataPath + "Eft.grp";
    FightPicFile[0] = DataPath + "fight/fight%03d.idx";
    FightPicFile[1] = DataPath + "fight/fight%03d.grp";

    HeadPath = DataPath + "head/";
    ThingPath = DataPath + "thing/";

    MIDIFile = g_Config.MusicPath + "%02d.mid";
    ATKFile = g_Config.SoundPath + "atk%02d.wav";
    EFile = g_Config.SoundPath + "e%02d.wav";

    WarFile = DataPath + "war.sta";
    WarMapFile[0] = DataPath + "warfld.idx";
    WarMapFile[1] = DataPath + "warfld.grp";

    KRP = DataPath + "kdef.grp";
    KDX = DataPath + "kdef.idx";
    TRP = DataPath + "talk.grp";
    TDX = DataPath + "talk.idx";

    // 船贴图
    int tmpBoat[][2] = { {0x166, 0x16a}, {0x176, 0x17c}, {0x1ca, 0x1d0}, {0x1fa, 0x262}, {0x3f8, 0x3fe} };
    for (auto& b : tmpBoat)
    {
        for (int j = b[0]; j <= b[1]; j += 2)
            MMapBoat[j] = 1;
    }
    // 深海
    for (int j = 0x264; j <= 0x29E; j += 2)
        MMapBoat[j] = 1;

    // 场景水面
    int tmpWater[][2] = { {0x166, 0x16a}, {0x176, 0x17c}, {0x1ca, 0x1d0}, {0x1fa, 0x262},
                          {0x332, 0x338}, {0x346, 0x346}, {0x3a6, 0x3a8}, {0x3f8, 0x3fe}, {0x52c, 0x544} };
    for (auto& w : tmpWater)
    {
        for (int j = w[0]; j <= w[1]; j += 2)
        {
            SceneWater[j] = 1;
            WarWater[j] = 1;
        }
    }

    // 特效帧数 - 所有12个版本都一样（简化处理，版本4特殊数据也保留）
    const int eftFrames[] = { 9, 14, 17, 9, 13, 17, 17, 17, 18, 19, 19, 15, 13, 10, 10, 15, 21, 16, 9, 11, 8, 9, 8, 8, 7, 8, 8, 9, 12, 19, 11, 14, 12, 17, 8, 11, 10, 13, 10, 19, 14, 17, 19, 14, 21, 16, 13, 18, 14, 17, 17, 16, 7, 12, 40, 16, 9, 15, 15, 31, 38, 24, 26, 24, 20, 12, 17, 14, 14, 10, 10, 18, 31, 12, 7, 6, 7, 28, 16, 7, 16, 20, 15, 13, 15, 11, 11, 11, 20, 20, 20, 17, 17, 17, 9, 8, 8, 17, 10, 11, 8, 29, 6, 13, 31, 14, 4, 13, 13, 15, 10 };
    for (int i = 0; i < (int)(sizeof(eftFrames) / sizeof(eftFrames[0])); i++)
        Effect.push_back(eftFrames[i]);

    // 离队事件 PersonExit
    // 版本1
    PersonExit[1] = { {1,950},{2,952},{9,954},{16,956},{17,958},{25,960},{28,962},{29,964},{35,966},{36,968},{37,970},{38,972},{44,974},{45,976},{47,978},{48,980},{49,982},{51,984},{53,986},{54,988},{58,990},{59,992},{61,994},{63,996},{76,998} };
    // 版本2-3
    PersonExit[2] = PersonExit[3] = { {1,100},{2,102},{4,104},{9,106},{16,108},{17,110},{25,112},{28,114},{29,116},{30,118},{35,120},{36,122},{37,124},{38,126},{44,128},{45,130},{47,132},{48,134},{49,136},{51,138},{52,140},{53,142},{54,144},{55,146},{56,148},{58,150},{59,152},{63,154},{66,156},{72,158},{73,160},{74,162},{75,164},{76,166},{77,168},{78,170},{79,172},{80,174},{81,176},{82,178},{83,180},{84,182},{85,184},{86,186},{87,188},{88,190},{89,192},{90,194},{91,196},{92,198} };
    PersonExit[4] = { {1,100},{2,102},{4,104},{9,106},{16,108},{17,110},{25,112},{28,114},{29,116},{30,118},{35,120},{36,122},{37,124},{38,126},{44,128},{45,130},{47,132},{48,134},{49,136},{51,138},{52,140},{53,142},{54,144},{55,146},{56,148},{58,150},{59,152},{63,154},{66,156},{72,158},{73,160},{74,162},{75,164},{76,166},{77,168},{78,170},{79,172},{80,174},{81,176},{82,178},{83,180},{84,182},{85,184},{86,186},{87,188},{88,190},{89,192},{90,194},{91,196},{92,198},{589,200},{590,202},{591,204},{592,206},{593,208},{594,210},{595,212},{596,214},{97,216},{597,218},{50,220},{598,222},{599,224},{600,226},{601,228},{103,230},{602,232},{603,234},{604,236},{605,238},{606,240},{607,242},{608,244},{609,246},{610,248},{93,250},{611,252} };
    PersonExit[5] = { {1,1050},{2,1052},{9,1054},{95,1056},{114,1058},{25,1060},{50,1062},{29,1064},{35,1066},{36,1068},{37,1070},{38,1072},{44,1074},{45,1076},{47,1078},{48,1080},{49,1082},{51,1084},{53,1086},{54,1088},{58,1090},{59,1092},{61,1094},{63,1096},{76,1098},{115,1100},{56,1102},{55,1104},{17,1106},{28,1108},{30,1110},{16,1112},{73,1114},{52,1116},{74,1118},{66,1120},{39,1122},{40,1124},{41,1126},{42,1128},{72,1130} };
    PersonExit[6] = { {1,950},{2,952},{9,954},{16,956},{17,958},{25,960},{28,962},{29,964},{35,966},{36,968},{37,970},{38,972},{44,974},{45,976},{47,978},{48,980},{49,982},{51,984},{53,986},{54,988},{58,990},{59,992},{61,994},{63,996},{76,998},{79,1000},{80,1002},{101,1004},{84,1006},{90,1008},{91,1010},{96,1012},{100,1014},{104,1016},{55,1018},{56,1020},{121,1022},{92,1024},{81,1026},{93,1028} };
    PersonExit[7] = { {2,100},{17,102},{25,104},{30,106},{47,108},{52,110},{56,112},{59,114},{63,116},{66,118},{73,120},{74,122},{76,124},{77,126},{78,128},{79,130},{81,132},{83,134},{86,136},{87,138},{90,140},{91,142},{92,144},{101,146},{102,148},{104,150},{105,152},{111,154},{115,156},{124,158},{125,160},{126,162},{127,164},{136,166},{144,168},{145,170},{146,172},{147,174},{148,176},{154,178},{161,180},{164,182},{175,184},{177,186},{178,188},{179,190},{180,192},{181,194},{182,196},{183,198},{220,200},{221,202},{222,204},{224,206},{225,208},{226,210},{227,212},{228,214},{229,216},{230,218},{231,220},{232,222},{233,224},{234,226},{235,228},{236,230} };
    PersonExit[8] = { {36,3},{119,5},{106,7},{105,9},{2,11},{48,13},{98,15},{39,17},{112,19},{111,21},{87,23},{40,25},{42,27},{41,29},{91,31},{107,33},{16,35},{17,37},{1,39},{63,41},{99,43},{81,45},{28,47},{51,49},{110,55},{45,57},{49,59},{77,61},{35,63},{230,65},{268,67},{117,69},{322,71},{323,73},{324,75},{73,77} };
    PersonExit[9] = PersonExit[10] = PersonExit[11] = PersonExit[2];
    PersonExit[12] = { {1,1},{74,3},{2,5},{73,7},{37,9},{115,11},{116,13},{79,15},{54,17},{126,19},{125,21},{77,23},{76,25},{28,27},{381,29},{58,31},{61,33} };

    // 全体离队事件 AllPersonExit
    AllPersonExit[1] = { {0,0},{49,2},{4,1},{44,0},{44,1},{37,5},{30,0},{59,0},{40,3},{56,1},{1,7},{1,8},{1,10},{40,7},{40,8},{77,0},{54,0},{62,3},{62,4},{60,2},{60,15},{52,1},{61,0},{61,8},{78,0},{18,0},{18,1},{69,0},{69,1},{45,0},{52,2},{42,6},{42,7},{8,8},{7,6},{80,1} };
    AllPersonExit[2] = AllPersonExit[3] = AllPersonExit[5] = AllPersonExit[9] = AllPersonExit[10] = AllPersonExit[11] = AllPersonExit[1];
    AllPersonExit[6] = { {70,13},{70,14},{70,6},{70,26},{70,25},{70,10},{70,5},{70,37},{70,32},{70,33},{70,11},{70,12},{70,34},{70,10},{70,30},{70,31},{70,35},{70,43},{70,21},{70,22},{70,27},{70,28},{70,36},{70,20},{70,39},{70,16},{70,18},{69,0},{69,1},{70,7},{70,19},{70,23},{70,24},{70,40},{70,29},{70,9},{70,50},{70,17},{70,41},{70,42},{70,15},{70,38},{70,47},{70,49},{70,48} };

    // 武功武器配合
    ExtraOffense = { {106, 57, 100}, {107, 49, 50}, {108, 49, 50}, {110, 54, 80}, {115, 63, 50}, {116, 67, 70}, {119, 68, 100} };

    // 新游戏数据
    int sceneids[12] = { 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70 };
    int scenexs[12] = { 19, 16, 26, 16, 19, 19, 16, 54, 16, 16, 16, 19 };
    int sceneys[12] = { 20, 31, 31, 31, 20, 20, 31, 13, 31, 31, 31, 20 };
    int events[12] = { 691, 691, 691, 691, 691, 691, 691, 691, 691, 691, 691, 691 };
    int pics[12] = { 3445, 2515, 2515, 2515, 3445, 3445, 2515, 3445, 2515, 2515, 2515, 3445 };
    memcpy(NewGameSceneID, sceneids, sizeof(sceneids));
    memcpy(NewGameSceneX, scenexs, sizeof(scenexs));
    memcpy(NewGameSceneY, sceneys, sizeof(sceneys));
    memcpy(NewGameEvent, events, sizeof(events));
    memcpy(NewPersonPic, pics, sizeof(pics));

    // 自宫
    Shemale[78] = 1;
    Shemale[91] = 1;

    NewPersonName = "徐小侠";
    if (!g_Config.PlayName.empty()) NewPersonName = g_Config.PlayName;

    // 商店场景
    ShopScene[0] = { 1, 16, {17, 18} };
    ShopScene[1] = { 3, 14, {15, 16} };
    ShopScene[2] = { 40, 20, {21, 22} };
    ShopScene[3] = { 60, 16, {17, 18} };
    ShopScene[4] = { 61, 9, {10, 11, 12} };

    // 经验表
    int expTable[] = { 0, 50, 150, 300, 500, 750, 1050, 1400, 1800, 2250, 2750, 3850, 5050, 6350, 7750, 9250, 10850, 12550, 14350, 16750, 18250, 21400, 24700, 28150, 31750, 35500, 39400, 43450, 47650, 52000, 60000 };
    for (int i = 0; i < 31; i++)
        Exp[i] = expTable[i];
    for (int i = 31; i < 62; i++)
        Exp[i] = 60000;

    // 人物属性最大值
    // AttribMax[row][version-1]
    int AttribMax[13][12] = {
        {999, 999, 9999, 9999, 9999, 9999, 9999, 9999, 9999, 9999, 9999, 999}, // 命
        {999, 9999, 9999, 9999, 9999, 9999, 9999, 9999, 9999, 9999, 9999, 999}, // 内
        {100, 800, 999, 999, 999, 999, 999, 999, 999, 999, 999, 100}, // 攻
        {100, 800, 999, 999, 999, 999, 999, 999, 999, 999, 999, 100}, // 防
        {100, 300, 999, 999, 300, 999, 999, 999, 500, 500, 500, 100}, // 医
        {100, 300, 999, 999, 300, 999, 999, 999, 500, 500, 500, 100}, // 毒
        {100, 300, 999, 999, 300, 999, 999, 999, 500, 500, 500, 100}, // 解毒
        {100, 600, 999, 999, 300, 999, 999, 999, 500, 500, 500, 100}, // 拳
        {100, 600, 999, 999, 300, 999, 999, 999, 500, 500, 500, 100}, // 剑
        {100, 600, 999, 999, 300, 999, 999, 999, 500, 500, 500, 100}, // 刀
        {100, 600, 999, 999, 300, 999, 999, 999, 500, 500, 500, 100}, // 特
        {100, 300, 999, 999, 300, 999, 999, 999, 500, 500, 500, 100}, // 暗器
        {30, 30, 60, 60, 60, 30, 60, 30, 60, 60, 60, 30}, // 等级
    };
    int vi = version - 1; // 0-indexed
    if (vi < 0) vi = 0;
    if (vi > 11) vi = 11;

    PersonAttribMax["经验"] = 60000;
    PersonAttribMax["物品修炼点数"] = 30000;
    PersonAttribMax["修炼点数"] = 30000;
    PersonAttribMax["生命最大值"] = AttribMax[0][vi];
    PersonAttribMax["受伤程度"] = 100;
    PersonAttribMax["中毒程度"] = 100;
    PersonAttribMax["内力最大值"] = AttribMax[1][vi];
    PersonAttribMax["体力"] = 100;
    PersonAttribMax["攻击力"] = AttribMax[2][vi];
    PersonAttribMax["防御力"] = AttribMax[3][vi];
    PersonAttribMax["轻功"] = 500;
    PersonAttribMax["医疗能力"] = AttribMax[4][vi];
    PersonAttribMax["用毒能力"] = AttribMax[5][vi];
    PersonAttribMax["解毒能力"] = AttribMax[6][vi];
    PersonAttribMax["抗毒能力"] = 240;
    PersonAttribMax["拳掌功夫"] = AttribMax[7][vi];
    PersonAttribMax["御剑能力"] = AttribMax[8][vi];
    PersonAttribMax["耍刀技巧"] = AttribMax[9][vi];
    PersonAttribMax["特殊兵器"] = AttribMax[10][vi];
    PersonAttribMax["暗器技巧"] = AttribMax[11][vi];
    PersonAttribMax["武学常识"] = 100;
    PersonAttribMax["品德"] = 100;
    PersonAttribMax["资质"] = 100;
    PersonAttribMax["攻击带毒"] = 240;
    PersonAttribMax["人物等级"] = AttribMax[12][vi];

    // 显示设置
    DefaultFont = (int)(std::min(ScreenW, ScreenH) / 320.0 * 16);
    SmallFont = DefaultFont * 3 / 4;
    FontBIG = (int)(DefaultFont * 1.45);
    FontBig = (int)(DefaultFont * 1.3);
    Fontbig = (int)(DefaultFont * 1.15);
    Fontsmall = (int)(DefaultFont * 0.85);
    FontSmall = (int)(DefaultFont * 0.7);
    FontSMALL = (int)(DefaultFont * 0.55);
    RowPixel = (int)(std::min(ScreenW, ScreenH) / 100.0);

    StartMenuFontSize = DefaultFont;
    NewGameFontSize = DefaultFont;

    StartMenuY = ScreenH - 3 * (StartMenuFontSize + RowPixel) - 20;
    NewGameY = ScreenH - 4 * (NewGameFontSize + RowPixel) - 10;

    MainSubMenuX = MainMenuX + 2 * MenuBorderPixel + 2 * DefaultFont + 5;
    MainSubMenuY = MainMenuY;
    MainSubMenuX2 = MainSubMenuX + 2 * MenuBorderPixel + 4 * DefaultFont + 5;
    SingleLineHeight = DefaultFont + 2 * MenuBorderPixel + 5;

    ThingFontSize = Fontsmall;
    ThingPicWidth = (int)(40.0 * zoom / 100);
    ThingPicHeight = ThingPicWidth;

    int n = (int)(ScreenW / (double)ThingPicWidth - 2);
    MenuThingXnum = (n < 10) ? 5 : 10;
    int m = (int)(ScreenH / (double)ThingPicHeight - 2);
    MenuThingYnum = (m < 5) ? 3 : 5;

    SceneFlagPic[0] = 2749;
    SceneFlagPic[1] = 2846;

    if (g_Config.FastShowScreen == 0)
    {
        ShowFlag = 1;
        AutoWarShowHead = (g_Config.Type == 1) ? 1 : 0;
    }
    else
    {
        ShowFlag = 0;
        AutoWarShowHead = 0;
    }
    FastShowScreen = g_Config.FastShowScreen;
}
