// GameData.h - 游戏数据结构和常量定义
// 从 jyconst.lua 转换而来
#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include <cmath>

// ============== 游戏状态枚举 ==============
enum GameStatus
{
    GAME_INIT = -1,
    GAME_START = 0,
    GAME_FIRSTMMAP = 1,
    GAME_MMAP = 2,
    GAME_FIRSTSMAP = 3,
    GAME_SMAP = 4,
    GAME_WMAP = 5,
    GAME_DEAD = 6,
    GAME_END = 7,
};

// ============== 键码定义 ==============
enum VirtualKeys
{
    VK_JY_ESCAPE = 27,
    VK_JY_N = 110,
    VK_JY_H = 104,
    VK_JY_S = 115,
    VK_JY_SPACE = 32,
    VK_JY_RETURN = 13,
    VK_JY_UP = 1073741906,
    VK_JY_DOWN = 1073741905,
    VK_JY_LEFT = 1073741904,
    VK_JY_RIGHT = 1073741903,
};

// ============== 颜色常量 ==============
inline int RGB_JY(int r, int g, int b) { return r * 65536 + g * 256 + b; }
inline void GetRGB_JY(int color, int& r, int& g, int& b)
{
    color = color % (65536 * 256);
    r = color / 65536;
    color = color % 65536;
    g = color / 256;
    b = color % 256;
}

// 颜色常量，在Init中初始化
extern int C_STARTMENU;
extern int C_RED;
extern int C_WHITE;
extern int C_ORANGE;
extern int C_GOLD;
extern int C_BLACK;

// ============== 二进制数据字段描述 ==============
// 类型: 0=有符号16位, 1=无符号16位, 2=字符串
struct FieldDef
{
    int offset;
    int type;    // 0=signed16, 1=unsigned16, 2=string
    int length;  // 字符串长度（仅type==2时有效）
};

// ============== 二进制缓冲区包装 ==============
struct DataBuffer
{
    uint8_t* data = nullptr;
    int size = 0;

    DataBuffer() = default;
    ~DataBuffer() { free(); }

    void alloc(int s)
    {
        free();
        data = new uint8_t[s]();
        size = s;
    }

    void free()
    {
        delete[] data;
        data = nullptr;
        size = 0;
    }

    int16_t get16(int offset) const
    {
        if (!data || offset < 0 || offset + 1 >= size) return 0;
        return (int16_t)(data[offset] | (data[offset + 1] << 8));
    }

    uint16_t getu16(int offset) const
    {
        if (!data || offset < 0 || offset + 1 >= size) return 0;
        return (uint16_t)(data[offset] | (data[offset + 1] << 8));
    }

    int32_t get32(int offset) const
    {
        if (!data || offset < 0 || offset + 3 >= size) return 0;
        return (int32_t)(data[offset] | (data[offset + 1] << 8) | (data[offset + 2] << 16) | (data[offset + 3] << 24));
    }

    void set16(int offset, int16_t v)
    {
        if (!data || offset < 0 || offset + 1 >= size) return;
        data[offset] = v & 0xFF;
        data[offset + 1] = (v >> 8) & 0xFF;
    }

    void setu16(int offset, uint16_t v)
    {
        if (!data || offset < 0 || offset + 1 >= size) return;
        data[offset] = v & 0xFF;
        data[offset + 1] = (v >> 8) & 0xFF;
    }

    void set32(int offset, int32_t v)
    {
        if (!data || offset < 0 || offset + 3 >= size) return;
        data[offset] = v & 0xFF;
        data[offset + 1] = (v >> 8) & 0xFF;
        data[offset + 2] = (v >> 16) & 0xFF;
        data[offset + 3] = (v >> 24) & 0xFF;
    }

    std::string getstr(int offset, int len) const
    {
        if (!data || offset < 0 || offset + len > size) return "";
        // 找到实际的字符串结尾（可能有\0填充）
        int actual = 0;
        for (int i = 0; i < len; i++)
        {
            if (data[offset + i] == 0) break;
            actual++;
        }
        return std::string((const char*)(data + offset), actual);
    }

    void setstr(int offset, int len, const std::string& s)
    {
        if (!data || offset < 0 || offset + len > size) return;
        memset(data + offset, 0, len);
        int copylen = std::min((int)s.size(), len);
        memcpy(data + offset, s.c_str(), copylen);
    }

    bool loadfile(const char* filename, int start, int len);
    bool savefile(const char* filename, int start, int len);

    // 禁止拷贝
    DataBuffer(const DataBuffer&) = delete;
    DataBuffer& operator=(const DataBuffer&) = delete;
    DataBuffer(DataBuffer&& o) noexcept : data(o.data), size(o.size) { o.data = nullptr; o.size = 0; }
    DataBuffer& operator=(DataBuffer&& o) noexcept
    {
        free();
        data = o.data; size = o.size;
        o.data = nullptr; o.size = 0;
        return *this;
    }
};

// ============== 人物数据结构（基于CC.Person_S, 每人182字节）==============
// 这些结构提供直接在二进制buffer上操作的访问器
// 避免在每帧做字符串哈希table查找

// 数据代理类——提供用偏移量直接读写全局buffer的能力
class PersonAccessor
{
public:
    DataBuffer* buf = nullptr;
    int baseOffset = 0;

    int16_t id() const { return buf->get16(baseOffset + 0); }
    int16_t headId() const { return buf->get16(baseOffset + 2); }
    int16_t hpGrowth() const { return buf->get16(baseOffset + 4); }
    std::string name() const;  // offset=8, 10 bytes, charset converted
    std::string nickname() const; // offset=18, 10 bytes
    int16_t gender() const { return buf->get16(baseOffset + 28); }
    int16_t level() const { return buf->get16(baseOffset + 30); }
    uint16_t exp() const { return buf->getu16(baseOffset + 32); }
    int16_t hp() const { return buf->get16(baseOffset + 34); }
    int16_t maxHp() const { return buf->get16(baseOffset + 36); }
    int16_t injury() const { return buf->get16(baseOffset + 38); }
    int16_t poison() const { return buf->get16(baseOffset + 40); }
    int16_t stamina() const { return buf->get16(baseOffset + 42); }
    int16_t trainPoints() const { return buf->get16(baseOffset + 44); }
    int16_t weapon() const { return buf->get16(baseOffset + 46); }
    int16_t armor() const { return buf->get16(baseOffset + 48); }

    // 出招动画帧数1-5
    int16_t attackFrame(int i) const { return buf->get16(baseOffset + 50 + 2 * (i - 1)); }
    int16_t attackDelay(int i) const { return buf->get16(baseOffset + 60 + 2 * (i - 1)); }
    int16_t attackSoundDelay(int i) const { return buf->get16(baseOffset + 70 + 2 * (i - 1)); }

    int16_t mpType() const { return buf->get16(baseOffset + 80); }
    int16_t mp() const { return buf->get16(baseOffset + 82); }
    int16_t maxMp() const { return buf->get16(baseOffset + 84); }
    int16_t attack() const { return buf->get16(baseOffset + 86); }
    int16_t agility() const { return buf->get16(baseOffset + 88); }
    int16_t defense() const { return buf->get16(baseOffset + 90); }
    int16_t medic() const { return buf->get16(baseOffset + 92); }
    int16_t usePoison() const { return buf->get16(baseOffset + 94); }
    int16_t detox() const { return buf->get16(baseOffset + 96); }
    int16_t antiPoison() const { return buf->get16(baseOffset + 98); }
    int16_t fist() const { return buf->get16(baseOffset + 100); }
    int16_t sword() const { return buf->get16(baseOffset + 102); }
    int16_t blade() const { return buf->get16(baseOffset + 104); }
    int16_t special() const { return buf->get16(baseOffset + 106); }
    int16_t hidden() const { return buf->get16(baseOffset + 108); }
    int16_t knowledge() const { return buf->get16(baseOffset + 110); }
    int16_t morality() const { return buf->get16(baseOffset + 112); }
    int16_t attackPoison() const { return buf->get16(baseOffset + 114); }
    int16_t dualWield() const { return buf->get16(baseOffset + 116); }
    int16_t fame() const { return buf->get16(baseOffset + 118); }
    int16_t aptitude() const { return buf->get16(baseOffset + 120); }
    int16_t trainItem() const { return buf->get16(baseOffset + 122); }
    int16_t trainExp() const { return buf->get16(baseOffset + 124); }

    // 武功1-10
    int16_t wugong(int i) const { return buf->get16(baseOffset + 126 + 2 * (i - 1)); }
    int16_t wugongLevel(int i) const { return buf->get16(baseOffset + 146 + 2 * (i - 1)); }
    // 携带物品1-4
    int16_t carryItem(int i) const { return buf->get16(baseOffset + 166 + 2 * (i - 1)); }
    int16_t carryItemNum(int i) const { return buf->get16(baseOffset + 174 + 2 * (i - 1)); }

    // 通用设置器（用中文名做key的兼容方式，用于事件系统等需要动态访问的场景）
    int getByName(const std::string& fieldName) const;
    void setByName(const std::string& fieldName, int value);
    std::string getStrByName(const std::string& fieldName) const;
    void setStrByName(const std::string& fieldName, const std::string& value);

    // 直接偏移写入
    void setId(int16_t v) { buf->set16(baseOffset + 0, v); }
    void setHeadId(int16_t v) { buf->set16(baseOffset + 2, v); }
    void setHp(int16_t v) { buf->set16(baseOffset + 34, v); }
    void setMaxHp(int16_t v) { buf->set16(baseOffset + 36, v); }
    void setInjury(int16_t v) { buf->set16(baseOffset + 38, v); }
    void setPoison(int16_t v) { buf->set16(baseOffset + 40, v); }
    void setStamina(int16_t v) { buf->set16(baseOffset + 42, v); }
    void setLevel(int16_t v) { buf->set16(baseOffset + 30, v); }
    void setExp(uint16_t v) { buf->setu16(baseOffset + 32, v); }
    void setAttack(int16_t v) { buf->set16(baseOffset + 86, v); }
    void setDefense(int16_t v) { buf->set16(baseOffset + 90, v); }
    void setAgility(int16_t v) { buf->set16(baseOffset + 88, v); }
    void setMedic(int16_t v) { buf->set16(baseOffset + 92, v); }
    void setUsePoison(int16_t v) { buf->set16(baseOffset + 94, v); }
    void setDetox(int16_t v) { buf->set16(baseOffset + 96, v); }
    void setAntiPoison(int16_t v) { buf->set16(baseOffset + 98, v); }
    void setMpType(int16_t v) { buf->set16(baseOffset + 80, v); }
    void setMp(int16_t v) { buf->set16(baseOffset + 82, v); }
    void setMaxMp(int16_t v) { buf->set16(baseOffset + 84, v); }
    void setFist(int16_t v) { buf->set16(baseOffset + 100, v); }
    void setSword(int16_t v) { buf->set16(baseOffset + 102, v); }
    void setBlade(int16_t v) { buf->set16(baseOffset + 104, v); }
    void setSpecial(int16_t v) { buf->set16(baseOffset + 106, v); }
    void setHidden(int16_t v) { buf->set16(baseOffset + 108, v); }
    void setKnowledge(int16_t v) { buf->set16(baseOffset + 110, v); }
    void setMorality(int16_t v) { buf->set16(baseOffset + 112, v); }
    void setAttackPoison(int16_t v) { buf->set16(baseOffset + 114, v); }
    void setDualWield(int16_t v) { buf->set16(baseOffset + 116, v); }
    void setFame(int16_t v) { buf->set16(baseOffset + 118, v); }
    void setAptitude(int16_t v) { buf->set16(baseOffset + 120, v); }
    void setTrainItem(int16_t v) { buf->set16(baseOffset + 122, v); }
    void setTrainExp(int16_t v) { buf->set16(baseOffset + 124, v); }
    void setWeapon(int16_t v) { buf->set16(baseOffset + 46, v); }
    void setArmor(int16_t v) { buf->set16(baseOffset + 48, v); }
    void setGender(int16_t v) { buf->set16(baseOffset + 28, v); }
    void setHpGrowth(int16_t v) { buf->set16(baseOffset + 4, v); }
    void setWugong(int i, int16_t v) { buf->set16(baseOffset + 126 + 2 * (i - 1), v); }
    void setWugongLevel(int i, int16_t v) { buf->set16(baseOffset + 146 + 2 * (i - 1), v); }
    void setCarryItem(int i, int16_t v) { buf->set16(baseOffset + 166 + 2 * (i - 1), v); }
    void setCarryItemNum(int i, int16_t v) { buf->set16(baseOffset + 174 + 2 * (i - 1), v); }
    void setName(const std::string& s);
    void setNickname(const std::string& s);
};

// ============== 物品数据（CC.Thing_S, 每件190字节）==============
class ThingAccessor
{
public:
    DataBuffer* buf = nullptr;
    int baseOffset = 0;

    int16_t id() const { return buf->get16(baseOffset + 0); }
    std::string name() const;    // offset=2, 20 bytes
    std::string name2() const;   // offset=22, 20 bytes
    std::string desc() const;    // offset=42, 30 bytes
    int16_t trainWugong() const { return buf->get16(baseOffset + 72); }
    int16_t hiddenAnim() const { return buf->get16(baseOffset + 74); }
    int16_t user() const { return buf->get16(baseOffset + 76); }
    int16_t equipType() const { return buf->get16(baseOffset + 78); }
    int16_t showDesc() const { return buf->get16(baseOffset + 80); }
    int16_t type() const { return buf->get16(baseOffset + 82); }
    int16_t addHp() const { return buf->get16(baseOffset + 90); }
    int16_t addMaxHp() const { return buf->get16(baseOffset + 92); }
    int16_t addDetoxPoison() const { return buf->get16(baseOffset + 94); }
    int16_t addStamina() const { return buf->get16(baseOffset + 96); }
    int16_t changeMpType() const { return buf->get16(baseOffset + 98); }
    int16_t addMp() const { return buf->get16(baseOffset + 100); }
    int16_t addMaxMp() const { return buf->get16(baseOffset + 102); }
    int16_t addAttack() const { return buf->get16(baseOffset + 104); }
    int16_t addAgility() const { return buf->get16(baseOffset + 106); }
    int16_t addDefense() const { return buf->get16(baseOffset + 108); }
    int16_t addMedic() const { return buf->get16(baseOffset + 110); }
    int16_t addUsePoison() const { return buf->get16(baseOffset + 112); }
    int16_t addDetox() const { return buf->get16(baseOffset + 114); }
    int16_t addAntiPoison() const { return buf->get16(baseOffset + 116); }
    int16_t addFist() const { return buf->get16(baseOffset + 118); }
    int16_t addSword() const { return buf->get16(baseOffset + 120); }
    int16_t addBlade() const { return buf->get16(baseOffset + 122); }
    int16_t addSpecial() const { return buf->get16(baseOffset + 124); }
    int16_t addHidden() const { return buf->get16(baseOffset + 126); }
    int16_t addKnowledge() const { return buf->get16(baseOffset + 128); }
    int16_t addMorality() const { return buf->get16(baseOffset + 130); }
    int16_t addAttackCount() const { return buf->get16(baseOffset + 132); }
    int16_t addAttackPoison() const { return buf->get16(baseOffset + 134); }
    int16_t onlyTrainPerson() const { return buf->get16(baseOffset + 136); }
    int16_t needMpType() const { return buf->get16(baseOffset + 138); }
    int16_t needMp() const { return buf->get16(baseOffset + 140); }
    int16_t needAttack() const { return buf->get16(baseOffset + 142); }
    int16_t needAgility() const { return buf->get16(baseOffset + 144); }
    int16_t needUsePoison() const { return buf->get16(baseOffset + 146); }
    int16_t needMedic() const { return buf->get16(baseOffset + 148); }
    int16_t needDetox() const { return buf->get16(baseOffset + 150); }
    int16_t needFist() const { return buf->get16(baseOffset + 152); }
    int16_t needSword() const { return buf->get16(baseOffset + 154); }
    int16_t needBlade() const { return buf->get16(baseOffset + 156); }
    int16_t needSpecial() const { return buf->get16(baseOffset + 158); }
    int16_t needHidden() const { return buf->get16(baseOffset + 160); }
    int16_t needAptitude() const { return buf->get16(baseOffset + 162); }
    int16_t needExp() const { return buf->get16(baseOffset + 164); }
    int16_t trainNeedExp() const { return buf->get16(baseOffset + 166); }
    int16_t needMaterial() const { return buf->get16(baseOffset + 168); }
    int16_t trainItem(int i) const { return buf->get16(baseOffset + 170 + 2 * (i - 1)); }
    int16_t trainItemNum(int i) const { return buf->get16(baseOffset + 180 + 2 * (i - 1)); }

    // 设置器
    void setUser(int16_t v) { buf->set16(baseOffset + 76, v); }
    void setType(int16_t v) { buf->set16(baseOffset + 82, v); }

    int getByName(const std::string& fieldName) const;
    void setByName(const std::string& fieldName, int value);
    std::string getStrByName(const std::string& fieldName) const;
};

// ============== 场景数据（CC.Scene_S, 每个52字节）==============
class SceneAccessor
{
public:
    DataBuffer* buf = nullptr;
    int baseOffset = 0;

    int16_t id() const { return buf->get16(baseOffset + 0); }
    std::string name() const;  // offset=2, 10 bytes
    int16_t exitMusic() const { return buf->get16(baseOffset + 12); }
    int16_t enterMusic() const { return buf->get16(baseOffset + 14); }
    int16_t jumpScene() const { return buf->get16(baseOffset + 16); }
    int16_t enterCondition() const { return buf->get16(baseOffset + 18); }
    int16_t outerEnterX1() const { return buf->get16(baseOffset + 20); }
    int16_t outerEnterY1() const { return buf->get16(baseOffset + 22); }
    int16_t outerEnterX2() const { return buf->get16(baseOffset + 24); }
    int16_t outerEnterY2() const { return buf->get16(baseOffset + 26); }
    int16_t enterX() const { return buf->get16(baseOffset + 28); }
    int16_t enterY() const { return buf->get16(baseOffset + 30); }
    int16_t exitX1() const { return buf->get16(baseOffset + 32); }
    int16_t exitX2() const { return buf->get16(baseOffset + 34); }
    int16_t exitX3() const { return buf->get16(baseOffset + 36); }
    int16_t exitY1() const { return buf->get16(baseOffset + 38); }
    int16_t exitY2() const { return buf->get16(baseOffset + 40); }
    int16_t exitY3() const { return buf->get16(baseOffset + 42); }
    int16_t jumpX1() const { return buf->get16(baseOffset + 44); }
    int16_t jumpY1() const { return buf->get16(baseOffset + 46); }
    int16_t jumpX2() const { return buf->get16(baseOffset + 48); }
    int16_t jumpY2() const { return buf->get16(baseOffset + 50); }

    void setJumpX2(int16_t v) { buf->set16(baseOffset + 48, v); }
    void setJumpY2(int16_t v) { buf->set16(baseOffset + 50, v); }

    int getByName(const std::string& fieldName) const;
    void setByName(const std::string& fieldName, int value);
    std::string getStrByName(const std::string& fieldName) const;
};

// ============== 武功数据（CC.Wugong_S, 每个136字节）==============
class WugongAccessor
{
public:
    DataBuffer* buf = nullptr;
    int baseOffset = 0;

    int16_t id() const { return buf->get16(baseOffset + 0); }
    std::string name() const;  // offset=2, 10 bytes
    int16_t castSound() const { return buf->get16(baseOffset + 22); }
    int16_t wugongType() const { return buf->get16(baseOffset + 24); }
    int16_t animAndSound() const { return buf->get16(baseOffset + 26); }
    int16_t damageType() const { return buf->get16(baseOffset + 28); }
    int16_t attackRange() const { return buf->get16(baseOffset + 30); }
    int16_t mpCost() const { return buf->get16(baseOffset + 32); }
    int16_t poisonDmg() const { return buf->get16(baseOffset + 34); }

    // 等级相关 (1-10)
    int16_t attackPower(int i) const { return buf->get16(baseOffset + 36 + 2 * (i - 1)); }
    int16_t moveRange(int i) const { return buf->get16(baseOffset + 56 + 2 * (i - 1)); }
    int16_t killRange(int i) const { return buf->get16(baseOffset + 76 + 2 * (i - 1)); }
    int16_t addMpVal(int i) const { return buf->get16(baseOffset + 96 + 2 * (i - 1)); }
    int16_t killMp(int i) const { return buf->get16(baseOffset + 116 + 2 * (i - 1)); }

    int getByName(const std::string& fieldName) const;
    std::string getStrByName(const std::string& fieldName) const;
};

// ============== 商店数据（CC.Shop_S, 每个30字节）==============
class ShopAccessor
{
public:
    DataBuffer* buf = nullptr;
    int baseOffset = 0;

    int16_t item(int i) const { return buf->get16(baseOffset + 2 * (i - 1)); }
    int16_t itemNum(int i) const { return buf->get16(baseOffset + 10 + 2 * (i - 1)); }
    int16_t itemPrice(int i) const { return buf->get16(baseOffset + 20 + 2 * (i - 1)); }

    void setItem(int i, int16_t v) { buf->set16(baseOffset + 2 * (i - 1), v); }
    void setItemNum(int i, int16_t v) { buf->set16(baseOffset + 10 + 2 * (i - 1), v); }
    void setItemPrice(int i, int16_t v) { buf->set16(baseOffset + 20 + 2 * (i - 1), v); }

    int getByName(const std::string& name) const {
        // 解析 "物品N", "物品数量N", "物品价格N"
        if (name.find("物品价格") == 0) { int i = std::stoi(name.substr(strlen("物品价格"))); return itemPrice(i); }
        if (name.find("物品数量") == 0) { int i = std::stoi(name.substr(strlen("物品数量"))); return itemNum(i); }
        if (name.find("物品") == 0) { int i = std::stoi(name.substr(strlen("物品"))); return item(i); }
        return 0;
    }
    void setByName(const std::string& name, int v) {
        if (name.find("物品价格") == 0) { int i = std::stoi(name.substr(strlen("物品价格"))); setItemPrice(i, (int16_t)v); }
        else if (name.find("物品数量") == 0) { int i = std::stoi(name.substr(strlen("物品数量"))); setItemNum(i, (int16_t)v); }
        else if (name.find("物品") == 0) { int i = std::stoi(name.substr(strlen("物品"))); setItem(i, (int16_t)v); }
    }
};

// ============== 战斗数据（CC.WarData_S, 每个186字节）==============
struct WarDataDef
{
    DataBuffer buf;

    int16_t id() const { return buf.get16(0); }
    std::string name() const;  // offset=2, 10 bytes
    int16_t mapId() const { return buf.get16(12); }
    int16_t expReward() const { return buf.get16(14); }
    int16_t music() const { return buf.get16(16); }
    int16_t manualSelect(int i) const { return buf.get16(18 + (i - 1) * 2); }
    int16_t autoSelect(int i) const { return buf.get16(30 + (i - 1) * 2); }
    int16_t allyX(int i) const { return buf.get16(42 + (i - 1) * 2); }
    int16_t allyY(int i) const { return buf.get16(54 + (i - 1) * 2); }
    int16_t enemy(int i) const { return buf.get16(66 + (i - 1) * 2); }
    int16_t enemyX(int i) const { return buf.get16(106 + (i - 1) * 2); }
    int16_t enemyY(int i) const { return buf.get16(146 + (i - 1) * 2); }
};

// ============== 基本数据（Base_S）==============
class BaseAccessor
{
public:
    DataBuffer* buf = nullptr;

    int16_t boatFlag() const { return buf->get16(0); }
    int16_t personX() const { return buf->get16(4); }
    int16_t personY() const { return buf->get16(6); }
    int16_t personX1() const { return buf->get16(8); }
    int16_t personY1() const { return buf->get16(10); }
    int16_t personDir() const { return buf->get16(12); }
    int16_t boatX() const { return buf->get16(14); }
    int16_t boatY() const { return buf->get16(16); }
    int16_t boatX1() const { return buf->get16(18); }
    int16_t boatY1() const { return buf->get16(20); }
    int16_t boatDir() const { return buf->get16(22); }
    int16_t team(int i) const { return buf->get16(24 + 2 * (i - 1)); }
    int16_t item(int i) const { return buf->get16(36 + 4 * (i - 1)); }
    int16_t itemNum(int i) const { return buf->get16(36 + 4 * (i - 1) + 2); }

    void setBoatFlag(int16_t v) { buf->set16(0, v); }
    void setPersonX(int16_t v) { buf->set16(4, v); }
    void setPersonY(int16_t v) { buf->set16(6, v); }
    void setPersonX1(int16_t v) { buf->set16(8, v); }
    void setPersonY1(int16_t v) { buf->set16(10, v); }
    void setPersonDir(int16_t v) { buf->set16(12, v); }
    void setTeam(int i, int16_t v) { buf->set16(24 + 2 * (i - 1), v); }
    void setItem(int i, int16_t v) { buf->set16(36 + 4 * (i - 1), v); }
    void setItemNum(int i, int16_t v) { buf->set16(36 + 4 * (i - 1) + 2, v); }

    int getByName(const std::string& fieldName) const;
    void setByName(const std::string& fieldName, int value);
};

// ============== 战斗人物信息 ==============
struct WarPerson
{
    int personId = -1;
    bool isAlly = true;
    int posX = -1;
    int posY = -1;
    bool dead = true;
    int direction = -1;
    int pic = -1;
    int picType = 0;    // 0=wmap贴图, 1=fight***贴图
    int agility = 0;
    int moveSteps = 0;
    int actionPoints = 0;
    int exp = 0;
    int autoTarget = -1;
};

// ============== 游戏常量 CC ==============
struct GameConst
{
    int SrcCharSet = 0;
    int OSCharSet = 0;
    std::string FontName;

    int ScreenW = 800;
    int ScreenH = 600;
    int TeamNum = 6;
    int MyThingNum = 200;
    int DNum = 200;
    int DNum2 = 11;
    int BaseSize = 836;

    int MWidth = 480;
    int MHeight = 480;
    int SWidth = 64;
    int SHeight = 64;

    int XScale = 18;
    int YScale = 9;

    int Frame = 50;
    int SceneMoveFrame = 100;
    int PersonMoveFrame = 100;
    int AnimationFrame = 150;
    int WarAutoDelay = 300;

    int DirectX[4] = { 0, 1, -1, 0 };
    int DirectY[4] = { -1, 0, 0, 1 };

    int MyStartPic = 2501;
    int BoatStartPic = 3715;

    // 文件路径
    std::string DataPath;
    std::string R_IDXFilename[4];
    std::string R_GRPFilename[4];
    std::string S_Filename[4];
    std::string TempS_Filename;
    std::string D_Filename[4];

    std::string PaletteFile;
    std::string FirstFile;
    std::string DeadFile;
    std::string MMapFile[5];
    std::string MMAPPicFile[2];
    std::string SMAPPicFile[2];
    std::string WMAPPicFile[2];
    std::string EffectFile[2];
    std::string FightPicFile[2];    // 格式化字符串
    std::string HeadPath;
    int HeadNum = 500;
    std::string ThingPath;
    int ThingNum = 500;
    std::string MIDIFile;    // 格式化字符串
    std::string ATKFile;     // 格式化字符串
    std::string EFile;       // 格式化字符串
    std::string WarFile;
    std::string WarMapFile[2];
    std::string KRP;
    std::string KDX;
    std::string TRP;
    std::string TDX;

    int PersonSize = 182;
    int ThingSize = 190;
    int SceneSize = 52;
    int WugongSize = 136;
    int ShopSize = 30;
    int WarDataSize = 186;

    int WarWidth = 64;
    int WarHeight = 64;

    int ShowXY = 1;
    int MenuBorderPixel = 5;
    int DefaultFont = 16;
    int SmallFont = 12;
    int FontBIG = 23;
    int FontBig = 21;
    int Fontbig = 18;
    int Fontsmall = 14;
    int FontSmall = 11;
    int FontSMALL = 9;
    int RowPixel = 3;
    int StartMenuFontSize = 16;
    int StartMenuY = 160;
    int NewGameFontSize = 16;
    int NewGameY = 160;
    int MainMenuX = 10;
    int MainMenuY = 10;
    int MainSubMenuX = 0;
    int MainSubMenuY = 0;
    int MainSubMenuX2 = 0;
    int SingleLineHeight = 0;
    int GameOverX = 90;
    int GameOverY = 65;
    int PersonStateRowPixel = 1;
    int ThingFontSize = 14;
    int ThingPicWidth = 40;
    int ThingPicHeight = 40;
    int MenuThingXnum = 5;
    int MenuThingYnum = 3;
    int ThingGapOut = 10;
    int ThingGapIn = 10;
    int StartThingPic = 0;
    int SceneXMin = 12;
    int SceneYMin = 12;
    int SceneXMax = 45;
    int SceneYMax = 45;
    int SceneFlagPic[2] = { 2749, 2846 };
    int ShowFlag = 1;
    int AutoWarShowHead = 0;
    int LoadThingPic = 1;
    int FastShowScreen = 0;

    int BookNum = 14;
    int BookStart = 144;
    int MoneyID = 174;

    std::string NewPersonName = "徐小侠";

    // 升级经验表
    int Exp[62] = {};

    // 属性上限（按版本）
    std::map<std::string, int> PersonAttribMax;

    // 船可以进入的贴图
    std::map<int, int> MMapBoat;
    std::map<int, int> SceneWater;
    std::map<int, int> WarWater;

    // 版本相关数组
    int NewGameSceneID[12] = {};
    int NewGameSceneX[12] = {};
    int NewGameSceneY[12] = {};
    int NewGameEvent[12] = {};
    int NewPersonPic[12] = {};

    // 离队事件等大数据表（在GameData.cpp中初始化）
    // PersonExit, AllPersonExit, ExtraOffense 等
    struct PersonExitEntry { int personId; int eventId; };
    std::vector<PersonExitEntry> PersonExit[13];  // 每个版本

    // 武功武器配合
    struct ExtraOffenseEntry { int weaponId; int wugongId; int addAttack; };
    std::vector<ExtraOffenseEntry> ExtraOffense;

    // 需自宫的秘籍
    std::map<int, int> Shemale;

    // 全体离队事件 AllPersonExit[version] = {{sceneid, eventid}, ...}
    std::map<int, std::vector<std::vector<int>>> AllPersonExit;

    // 商店场景数据
    struct ShopSceneEntry { int sceneid; int d_shop; std::vector<int> d_leave; };
    std::map<int, ShopSceneEntry> ShopScene;

    // 特效帧数
    std::vector<int> Effect;

    void init(int version, int zoom);
};

// ============== 游戏配置 ==============
struct GameConfig
{
    int Debug = 1;
    int Type = 0;
    int Width = 0;
    int Height = 0;
    int bpp = 16;
    int FullScreen = 0;
    int EnableSound = 1;
    int KeyRepeat = 0;
    int KeyRepeatDelay = 90;
    int KeyRepeatInterval = 30;
    int XScale = 18;
    int YScale = 9;
    int OSCharSet = 0;
    int LargeMemory = 1;
    int MP3 = 0;
    int MusicVolume = 50;
    int SoundVolume = 50;
    int FastShowScreen = 0;
    int Zoom = 150;
    int Version = 1;
    int CleanMemory = 0;
    int MAXCacheNum = 1000;
    int LoadFullS = 1;
    int LoadMMapScope = 0;
    int Operation = 0;
    int SwitchABXY = 0;
    std::string Softener;

    std::string CurrentPath = "./";
    std::string PicturePath;
    std::string MusicPath;
    std::string SoundPath;
    std::string ScriptPath;
    std::string PaletteFile;
    std::string FontName;
    std::string DataPath;
    std::string MidSF2;
    std::string PlayName = "徐小侠";

    int MMapAddX = 2, MMapAddY = 2;
    int SMapAddX = 2, SMapAddY = 16;
    int WMapAddX = 2, WMapAddY = 16;

    void loadFromINI(const std::string& filename);
};

// ============== 全局游戏状态 JY ==============
struct GameState
{
    int Status = GAME_INIT;

    // 数据缓冲区
    DataBuffer Data_Base;
    DataBuffer Data_Person;
    DataBuffer Data_Thing;
    DataBuffer Data_Scene;
    DataBuffer Data_Wugong;
    DataBuffer Data_Shop;

    // 访问器
    BaseAccessor Base;
    BaseAccessor& getBase() { return Base; }
    int PersonNum = 0;
    int ThingNum = 0;
    int SceneNum = 0;
    int WugongNum = 0;
    int ShopNum = 0;

    PersonAccessor getPerson(int i);
    ThingAccessor getThing(int i);
    SceneAccessor getScene(int i);
    WugongAccessor getWugong(int i);
    ShopAccessor getShop(int i);

    int MyCurrentPic = 0;
    int MyPic = 0;
    int MyTick = 0;
    int MyTick2 = 0;

    int SubScene = -1;
    int SubSceneX = 0;
    int SubSceneY = 0;

    int Darkness = 0;

    int CurrentD = -1;
    int OldDPass = -1;
    int CurrentEventType = -1;

    int oldMMapX = -1;
    int oldMMapY = -1;
    int oldMMapPic = -1;

    int oldSMapX = -1;
    int oldSMapY = -1;
    int oldSMapXoff = -1;
    int oldSMapYoff = -1;
    int oldSMapPic = -1;

    int CurrentThing = -1;
    int MmapMusic = -1;
    int MMAPMusic = -1;

    int CurrentMIDI = -1;
    int EnableMusic = 1;
    int EnableSound = 1;

    // D有效性缓存
    std::vector<int> D_Valid;
    int D_Valid_Num = 0;
    bool D_Valid_Dirty = true;

    // D事件动画改变
    struct DPicChange { int x, y, dy, p1, p2; };
    std::vector<DPicChange> D_PicChange;
    int NumD_PicChange = 0;

    // 场景入口缓存
    std::map<int, int> EnterSceneXY;
    bool EnterSceneXY_Dirty = true;
};

// ============== 全局变量声明 ==============
extern GameConfig g_Config;
extern GameConst g_CC;
extern GameState g_JY;

// ============== 工具函数 ==============
int limitX(int x, int minv, int maxv);
int Rnd(int i);

// 字符集转换
std::string charsetConvert(const std::string& str, int flag);
