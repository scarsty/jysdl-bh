// GameWar.cpp - 战斗系统实现
// 从 jywar.lua 转换而来

#include "GameWar.h"
#include "GameMain.h"
#include "GameEvent.h"
#include "mainmap.h"
#include "piccache.h"
#include "sdlfun.h"
#include "charset.h"
#include <cmath>
#include <cstring>
#include <algorithm>
#include <climits>

WarState WAR;

// ============= 地图操作包装 =============
int GetWarMap(int x, int y, int level) { return JY_GetWarMap(x, y, level); }
void SetWarMap(int x, int y, int level, int v) { JY_SetWarMap(x, y, level, v); }
void CleanWarMap(int level, int v) { JY_CleanWarMap(level, v); }

// ============= WarSetGlobal =============
void WarSetGlobal()
{
    memset(&WAR.SelectPerson, 0, sizeof(WAR.SelectPerson));
    for (int i = 0; i < 26; i++)
    {
        WAR.Person[i] = WarPersonInfo();
        WAR.Person[i].personId = -1;
        WAR.Person[i].isAlly = false;
        WAR.Person[i].x = -1;
        WAR.Person[i].y = -1;
        WAR.Person[i].dead = true;
        WAR.Person[i].direction = -1;
        WAR.Person[i].pic = -1;
        WAR.Person[i].picType = 0;
        WAR.Person[i].speed = 0;
        WAR.Person[i].moveSteps = 0;
        WAR.Person[i].points = 0;
        WAR.Person[i].exp = 0;
        WAR.Person[i].autoTarget = -1;
    }
    WAR.PersonNum = 0;
    WAR.AutoFight = 0;
    WAR.CurID = -1;
    WAR.ShowHead = 0;
    WAR.Effect = 0;

    WAR.EffectColor[0] = C_WHITE;
    WAR.EffectColor[1] = C_WHITE;
    WAR.EffectColor[2] = C_RED;
    WAR.EffectColor[3] = RGB_JY(208, 152, 208);
    WAR.EffectColor[4] = RGB_JY(236, 200, 40);
    WAR.EffectColor[5] = RGB_JY(120, 208, 88);
    WAR.EffectColor[6] = RGB_JY(56, 200, 120);
    WAR.EffectColor[7] = C_WHITE;

    memset(WAR.EffectXY, 0, sizeof(WAR.EffectXY));
    WAR.SSFwav = 0;
    WAR.LMSJwav = 0;
}

// ============= WarLoad =============
void WarLoad(int warid)
{
    WAR.Data.buf.loadfile(g_CC.WarFile.c_str(), g_CC.WarDataSize * warid, g_CC.WarDataSize);
}

// ============= WarMain =============
int WarMain(int warid, int isexp)
{
    WarSetGlobal();
    WarLoad(warid);
    WarSelectTeam();
    WarSelectEnemy();

    CleanMemory();
    WarLoadMap(WAR.Data.mapId());

    g_JY.Status = GAME_WMAP;

    // 加载战斗图片
    JY_ShowSlow(50, 1);
    JY_PicLoadFile(g_CC.WMAPPicFile[0].c_str(), g_CC.WMAPPicFile[1].c_str(), 0, 0, 0);
    int zoom = limitX(g_CC.ScreenW / 800 * g_Config.Zoom, 0, g_Config.Zoom);
    JY_LoadPNGPath(g_CC.HeadPath.c_str(), 1, g_CC.HeadNum, zoom, "png");
    JY_LoadPNGPath(g_CC.ThingPath.c_str(), 2, g_CC.ThingNum, zoom, "png");
    JY_PicLoadFile(g_CC.EffectFile[0].c_str(), g_CC.EffectFile[1].c_str(), 3, 0, 0);

    PlayMIDI(WAR.Data.music());

    WarPersonSort();

    // 战斗排序后加载fight图片
    CleanWarMap(2, -1);
    CleanWarMap(6, -2);
    for (int i = 0; i < WAR.PersonNum; i++)
    {
        int pid = WAR.Person[i].personId;
        auto p = g_JY.getPerson(pid);
        char idxbuf[256], grpbuf[256];
        snprintf(idxbuf, sizeof(idxbuf), g_CC.FightPicFile[0].c_str(), p.headId());
        snprintf(grpbuf, sizeof(grpbuf), g_CC.FightPicFile[1].c_str(), p.headId());
        JY_PicLoadFile(idxbuf, grpbuf, 4 + i, 0, 0);
    }

    int first = 0;
    int warStatus = 0;

    // 主战斗循环
    while (true)
    {
        // 每回合开始：重新计算所有人的贴图
        for (int i = 0; i < WAR.PersonNum; i++)
            WAR.Person[i].pic = WarCalPersonPic(i);

        // 每回合开始：重新计算移动步数（匹配Lua逻辑）
        for (int i = 0; i < WAR.PersonNum; i++)
        {
            int pid = WAR.Person[i].personId;
            auto person = g_JY.getPerson(pid);
            int move = (int)std::floor(WAR.Person[i].speed / 15.0)
                     - (int)std::floor(person.injury() / 40.0);
            if (move < 0) move = 0;
            WAR.Person[i].moveSteps = move;
        }

        WarSetPerson();

        int p = 0;
        while (p < WAR.PersonNum)
        {
            WAR.Effect = 0;
            if (WAR.AutoFight == 1)
            {
                int key = 0, type = 0, mx = 0, my = 0;
                JY_GetKey(&key, &type, &mx, &my);
                if (key == GK_SPACE || key == GK_RETURN)
                    WAR.AutoFight = 0;
            }

            if (!WAR.Person[p].dead)
            {
                WAR.CurID = p;

                if (first == 0)
                {
                    WarDrawMap(0);
                    ShowScreen();
                    first = 1;
                }

                int r;
                if (WAR.Person[p].isAlly)
                {
                    if (WAR.AutoFight == 0)
                        r = War_Manual();
                    else
                        r = War_Auto();
                }
                else
                {
                    r = War_Auto();
                }

                warStatus = War_isEnd();

                if (std::abs(r) == 7)  // 等待 -> 重试本人
                    p--;

                if (warStatus != 0) break;
            }
            p++;
        }

        if (warStatus != 0) break;
        War_PersonLostLife();
    }

    WAR.ShowHead = 0;

    // 显示胜负
    if (warStatus == 1)
        DrawStrBoxWaitKey("战斗胜利", C_WHITE, g_CC.DefaultFont);
    else
        DrawStrBoxWaitKey("战斗失败", C_WHITE, g_CC.DefaultFont);

    War_EndPersonData(isexp, warStatus);

    // 恢复场景
    if (g_JY.getScene(g_JY.SubScene).enterMusic() >= 0)
        PlayMIDI(g_JY.getScene(g_JY.SubScene).enterMusic());
    else
        PlayMIDI(0);

    CleanMemory();

    // 重新加载场景图片
    JY_PicInit(g_CC.SMAPPicFile[0].c_str());
    JY_PicInit(g_CC.EffectFile[0].c_str());
    g_JY.Status = GAME_SMAP;

    return warStatus;
}

// ============= War_PersonLostLife =============
void War_PersonLostLife()
{
    for (int i = 0; i < WAR.PersonNum; i++)
    {
        if (WAR.Person[i].dead) continue;
        int pid = WAR.Person[i].personId;
        auto p = g_JY.getPerson(pid);

        // 受伤扣血（匹配Lua: >0即扣）
        if (p.injury() > 0)
        {
            int v = (int)std::floor(p.injury() / 20.0);
            AddPersonAttrib(pid, "生命", -v);
        }
        // 中毒扣血
        if (p.poison() > 0)
        {
            int v = (int)std::floor(p.poison() / 10.0);
            AddPersonAttrib(pid, "生命", -v);
        }
        // 保底生命1
        if (p.hp() <= 0) p.setHp(1);
    }
}

// ============= War_isEnd =============
int War_isEnd()
{
    // 先标记死亡（匹配Lua）
    for (int i = 0; i < WAR.PersonNum; i++)
    {
        auto p = g_JY.getPerson(WAR.Person[i].personId);
        if (p.hp() <= 0)
            WAR.Person[i].dead = true;
    }
    WarSetPerson();

    Cls();
    ShowScreen();

    int myNum = 0, enemyNum = 0;
    for (int i = 0; i < WAR.PersonNum; i++)
    {
        if (!WAR.Person[i].dead)
        {
            if (WAR.Person[i].isAlly)
                myNum = 1;
            else
                enemyNum = 1;
        }
    }
    if (enemyNum == 0) return 1;  // 胜利
    if (myNum == 0) return 2;   // 失败
    return 0;
}

// ============= War_EndPersonData =============
void War_EndPersonData(int isexp, int warStatus)
{
    // 恢复敌方人员参数
    for (int i = 0; i < WAR.PersonNum; i++)
    {
        if (!WAR.Person[i].isAlly)
        {
            int pid = WAR.Person[i].personId;
            auto p = g_JY.getPerson(pid);
            p.setHp(p.maxHp());
            p.setMp(p.maxMp());
            p.setByName("体力", g_CC.PersonAttribMax["体力"]);
            p.setByName("受伤程度", 0);
            p.setByName("中毒程度", 0);
        }
    }

    // 我方人员参数恢复（输赢都有）
    for (int i = 0; i < WAR.PersonNum; i++)
    {
        if (WAR.Person[i].isAlly)
        {
            int pid = WAR.Person[i].personId;
            auto p = g_JY.getPerson(pid);
            if (p.hp() < p.maxHp() / 5)
                p.setHp((int16_t)(p.maxHp() / 5));
            if (p.stamina() < 10) p.setByName("体力", 10);
        }
    }

    if (warStatus == 2 && isexp == 0) return;

    // 计算我方活着人数
    int liveNum = 0;
    for (int i = 0; i < WAR.PersonNum; i++)
    {
        if (WAR.Person[i].isAlly && g_JY.getPerson(WAR.Person[i].personId).hp() > 0)
            liveNum++;
    }
    if (liveNum <= 0) liveNum = 1;

    // 胜利时分配基本经验
    if (warStatus == 1)
    {
        int baseExp = WAR.Data.expReward();
        for (int i = 0; i < WAR.PersonNum; i++)
        {
            if (WAR.Person[i].isAlly && g_JY.getPerson(WAR.Person[i].personId).hp() > 0)
                WAR.Person[i].exp += (int)std::floor((double)baseExp / liveNum);
        }
    }

    // 每个人经验增加、升级
    for (int i = 0; i < WAR.PersonNum; i++)
    {
        int pid = WAR.Person[i].personId;
        int exp = WAR.Person[i].exp;
        AddPersonAttrib(pid, "物品修炼点数", (int)std::floor(exp * 8 / 10.0));
        AddPersonAttrib(pid, "修炼点数", (int)std::floor(exp * 8 / 10.0));
        AddPersonAttrib(pid, "经验", exp);

        if (WAR.Person[i].isAlly)
        {
            auto p = g_JY.getPerson(pid);
            char buf[256];
            snprintf(buf, sizeof(buf), "%s 获得经验点数 %d", p.name().c_str(), exp);
            DrawStrBoxWaitKey(buf, C_WHITE, g_CC.DefaultFont);

            bool leveled = War_AddPersonLevel(pid);
            if (leveled)
            {
                snprintf(buf, sizeof(buf), "%s 升级了", p.name().c_str());
                DrawStrBoxWaitKey(buf, C_WHITE, g_CC.DefaultFont);
            }
        }

        War_PersonTrainBook(pid);
        War_PersonTrainDrug(pid);
    }
}

// ============= War_AddPersonLevel =============
// 匹配Lua War_AddPersonLevel：返回true表示升级了
bool War_AddPersonLevel(int personid)
{
    auto p = g_JY.getPerson(personid);
    int tmplevel = p.level();
    if (tmplevel >= g_CC.PersonAttribMax["人物等级"]) return false;
    if (p.exp() < g_CC.Exp[tmplevel]) return false;

    // 判断可以升几级
    while (tmplevel < g_CC.PersonAttribMax["人物等级"] && p.exp() >= g_CC.Exp[tmplevel])
        tmplevel++;

    int leveladd = tmplevel - p.level();
    p.setLevel((int16_t)tmplevel);

    AddPersonAttrib(personid, "生命最大值", (p.hpGrowth() + Rnd(3)) * leveladd * 3);
    p.setHp(p.maxHp());
    p.setByName("体力", g_CC.PersonAttribMax["体力"]);
    p.setByName("受伤程度", 0);
    p.setByName("中毒程度", 0);

    // 按资质计算增长点（越高技能增加越多，内力增加越少）
    int apt = p.aptitude();
    int cleveradd;
    if (apt < 30) cleveradd = 2;
    else if (apt < 50) cleveradd = 3;
    else if (apt < 70) cleveradd = 4;
    else if (apt < 90) cleveradd = 5;
    else cleveradd = 6;
    cleveradd = Rnd(cleveradd) + 1;

    AddPersonAttrib(personid, "内力最大值", (9 - cleveradd) * leveladd * 4);
    p.setMp(p.maxMp());

    AddPersonAttrib(personid, "攻击力", cleveradd * leveladd);
    AddPersonAttrib(personid, "防御力", cleveradd * leveladd);
    AddPersonAttrib(personid, "轻功", cleveradd * leveladd);

    if (p.getByName("医疗能力") >= 20) AddPersonAttrib(personid, "医疗能力", Rnd(3));
    if (p.getByName("用毒能力") >= 20) AddPersonAttrib(personid, "用毒能力", Rnd(3));
    if (p.getByName("解毒能力") >= 20) AddPersonAttrib(personid, "解毒能力", Rnd(3));
    if (p.getByName("拳掌功夫") >= 20) AddPersonAttrib(personid, "拳掌功夫", Rnd(3));
    if (p.getByName("御剑能力") >= 20) AddPersonAttrib(personid, "御剑能力", Rnd(3));
    if (p.getByName("耍刀技巧") >= 20) AddPersonAttrib(personid, "耍刀技巧", Rnd(3));
    if (p.getByName("暗器技巧") >= 20) AddPersonAttrib(personid, "暗器技巧", Rnd(3));

    return true;
}

// ============= War_PersonTrainBook =============
void War_PersonTrainBook(int personid)
{
    auto p = g_JY.getPerson(personid);
    int trainItem = p.trainItem();
    if (trainItem < 0) return;

    auto thing = g_JY.getThing(trainItem);
    int needPoint = thing.trainNeedExp();
    if (p.trainExp() < needPoint) return;

    p.setTrainExp((int16_t)(p.trainExp() - needPoint));

    // 修炼武功
    int trainWugong = thing.trainWugong();
    if (trainWugong >= 0)
    {
        // 查找是否已有该武功
        int slot = -1;
        for (int i = 1; i <= 10; i++)
        {
            if (p.wugong(i) == trainWugong)
            {
                slot = i;
                break;
            }
        }
        if (slot > 0)
        {
            // 已有武功，升级
            int lv = p.wugongLevel(slot);
            if (lv < 900)
            {
                p.setWugongLevel(slot, (int16_t)(lv + 100));
                auto wg = g_JY.getWugong(trainWugong);
                int newLv = (int)std::floor(p.wugongLevel(slot) / 100.0) + 1;
                DrawStrBox(-1, -1,
                    std::string(wg.name()) + " 升为 " + std::to_string(newLv) + " 级",
                    C_ORANGE, g_CC.DefaultFont);
                ShowScreen();
                JY_Delay(500);
                Cls();
            }
        }
        else
        {
            // 学新武功
            for (int i = 1; i <= 10; i++)
            {
                if (p.wugong(i) <= 0)
                {
                    p.setWugong(i, (int16_t)trainWugong);
                    p.setWugongLevel(i, 0);
                    auto wg = g_JY.getWugong(trainWugong);
                    DrawStrBox(-1, -1,
                        std::string(p.name()) + " 学会了" + wg.name(),
                        C_ORANGE, g_CC.DefaultFont);
                    ShowScreen();
                    JY_Delay(500);
                    Cls();
                    break;
                }
            }
        }
    }

    // 加属性
    if (thing.addHp() != 0) AddPersonAttrib(personid, "生命最大值", thing.addMaxHp());
    if (thing.addMp() != 0) AddPersonAttrib(personid, "内力最大值", thing.addMaxMp());
    if (thing.addAttack() != 0) AddPersonAttrib(personid, "攻击力", thing.addAttack());
    if (thing.addAgility() != 0) AddPersonAttrib(personid, "轻功", thing.addAgility());
    if (thing.addDefense() != 0) AddPersonAttrib(personid, "防御力", thing.addDefense());
    if (thing.addMedic() != 0) AddPersonAttrib(personid, "医疗能力", thing.addMedic());
    if (thing.addUsePoison() != 0) AddPersonAttrib(personid, "用毒能力", thing.addUsePoison());
    if (thing.addDetox() != 0) AddPersonAttrib(personid, "解毒能力", thing.addDetox());
    if (thing.addAntiPoison() != 0) AddPersonAttrib(personid, "抗毒能力", thing.addAntiPoison());
    if (thing.addFist() != 0) AddPersonAttrib(personid, "拳掌功夫", thing.addFist());
    if (thing.addSword() != 0) AddPersonAttrib(personid, "御剑能力", thing.addSword());
    if (thing.addBlade() != 0) AddPersonAttrib(personid, "耍刀技巧", thing.addBlade());
    if (thing.addSpecial() != 0) AddPersonAttrib(personid, "特殊兵器", thing.addSpecial());
    if (thing.addHidden() != 0) AddPersonAttrib(personid, "暗器技巧", thing.addHidden());
    if (thing.addAttackPoison() != 0) AddPersonAttrib(personid, "攻击带毒", thing.addAttackPoison());
    if (thing.addKnowledge() != 0) AddPersonAttrib(personid, "武学常识", thing.addKnowledge());
    if (thing.addMorality() != 0) AddPersonAttrib(personid, "道德", thing.addMorality());

    // 递归检查
    War_PersonTrainBook(personid);
}

// ============= War_PersonTrainDrug =============
void War_PersonTrainDrug(int personid)
{
    auto p = g_JY.getPerson(personid);
    int trainItem = p.trainItem();
    if (trainItem < 0) return;

    auto thing = g_JY.getThing(trainItem);
    int needMaterial = thing.needMaterial();
    if (needMaterial < 0) return;

    // 检查是否有足够材料
    int matCount = 0;
    for (int i = 1; i <= g_CC.MyThingNum; i++)
    {
        if (g_JY.Base.item(i) == needMaterial)
        {
            matCount = g_JY.Base.itemNum(i);
            break;
        }
    }
    if (matCount < 3) return;

    // 消耗材料
    instruct_32(needMaterial, -3);

    // 随机制造物品
    int candidates[5] = { -1, -1, -1, -1, -1 };
    int numCand = 0;
    for (int i = 1; i <= 5; i++)
    {
        int tid = thing.trainItem(i);
        if (tid >= 0)
        {
            candidates[numCand++] = tid;
        }
    }
    if (numCand > 0)
    {
        int sel = Rnd(numCand);
        int newThing = candidates[sel];
        instruct_2(newThing, 1);

        auto nt = g_JY.getThing(newThing);
        DrawStrBox(-1, -1,
            std::string(p.name()) + " 制造了 " + nt.name(),
            C_ORANGE, g_CC.DefaultFont);
        ShowScreen();
        JY_Delay(500);
        Cls();
    }

    // 递归检查
    War_PersonTrainDrug(personid);
}

// ============= WarSelectTeam =============
void WarSelectTeam()
{
    WAR.PersonNum = 0;

    // 先检查自动选择，如果有自动选择的人则直接返回（匹配Lua逻辑）
    for (int i = 1; i <= 6; i++)
    {
        int autoId = WAR.Data.autoSelect(i);
        if (autoId >= 0)
        {
            int n = WAR.PersonNum;
            WAR.Person[n].personId = autoId;
            WAR.Person[n].isAlly = true;
            WAR.Person[n].x = WAR.Data.allyX(i);
            WAR.Person[n].y = WAR.Data.allyY(i);
            WAR.Person[n].dead = false;
            WAR.Person[n].direction = 2;
            WAR.PersonNum++;
        }
    }
    if (WAR.PersonNum > 0)
        return;

    // 手动选择模式：先标记事先确定的参战人
    for (int i = 1; i <= g_CC.TeamNum; i++)
    {
        WAR.SelectPerson[i] = 0;
        int id = g_JY.Base.team(i);
        if (id >= 0)
        {
            for (int j = 1; j <= 6; j++)
            {
                if (WAR.Data.manualSelect(j) == id)
                    WAR.SelectPerson[i] = 1;
            }
        }
    }

    // 循环显示选人菜单，直到选中至少一个人
    while (true)
    {
        // 构建菜单（带回调的切换菜单）
        std::vector<MenuItem> menu;
        for (int i = 1; i <= g_CC.TeamNum; i++)
        {
            int id = g_JY.Base.team(i);
            if (id >= 0)
            {
                auto tp = g_JY.getPerson(id);
                std::string label = (WAR.SelectPerson[i] > 0) ?
                    std::string("*") + tp.name() :
                    std::string(" ") + tp.name();
                // 回调：切换选中状态
                int slotIdx = i;  // 捕获槽位索引
                auto toggleCb = [slotIdx](std::vector<MenuItem>& m, int midx) -> int {
                    if (WAR.SelectPerson[slotIdx] == 0)
                        WAR.SelectPerson[slotIdx] = 2;
                    else if (WAR.SelectPerson[slotIdx] == 2)
                        WAR.SelectPerson[slotIdx] = 0;
                    // 更新菜单文字中的*标记
                    std::string& lbl = m[midx].label;
                    if (WAR.SelectPerson[slotIdx] > 0)
                        lbl[0] = '*';
                    else
                        lbl[0] = ' ';
                    return 0;  // 不退出菜单
                };
                // 预选人不能取消（enabled=1但不用回调切换）
                if (WAR.SelectPerson[i] == 1)
                    menu.push_back({label, nullptr, 1, i});
                else
                    menu.push_back({label, toggleCb, 1, i});
            }
            else
            {
                menu.push_back({"", nullptr, 0, i});
            }
        }
        // 最后加"结束"项
        menu.push_back({" 结束", nullptr, 1, 0});

        Cls();
        int x = (g_CC.ScreenW - 7 * g_CC.DefaultFont - 2 * g_CC.MenuBorderPixel) / 2;
        DrawStrBox(x, 10, "请选择参战人物", C_WHITE, g_CC.DefaultFont);
        int r = ShowMenu(menu, (int)menu.size(), 0, x, 10 + g_CC.SingleLineHeight,
            0, 0, 1, 0, g_CC.DefaultFont, C_ORANGE, C_WHITE);
        Cls();

        // 根据SelectPerson组装参战人
        WAR.PersonNum = 0;
        for (int i = 1; i <= 6; i++)
        {
            if (WAR.SelectPerson[i] > 0)
            {
                int n = WAR.PersonNum;
                WAR.Person[n].personId = g_JY.Base.team(i);
                WAR.Person[n].isAlly = true;
                WAR.Person[n].x = WAR.Data.allyX(i);
                WAR.Person[n].y = WAR.Data.allyY(i);
                WAR.Person[n].dead = false;
                WAR.Person[n].direction = 2;
                WAR.PersonNum++;
            }
        }
        if (WAR.PersonNum > 0)
            break;
    }
}

// ============= WarSelectEnemy =============
void WarSelectEnemy()
{
    for (int i = 1; i <= 20; i++)
    {
        int eid = WAR.Data.enemy(i);
        if (eid >= 0)
        {
            int n = WAR.PersonNum;
            WAR.Person[n].personId = eid;
            WAR.Person[n].isAlly = false;
            WAR.Person[n].x = WAR.Data.enemyX(i);
            WAR.Person[n].y = WAR.Data.enemyY(i);
            WAR.Person[n].dead = false;
            WAR.Person[n].direction = 1;
            WAR.PersonNum++;
        }
    }
}

// ============= WarLoadMap =============
void WarLoadMap(int mapid)
{
    JY_LoadWarMap(g_CC.WarMapFile[0].c_str(), g_CC.WarMapFile[1].c_str(), mapid,
        7, g_CC.WarWidth, g_CC.WarHeight);
}

// ============= WarDrawMap =============
void WarDrawMap(int flag, int extra1, int extra2, int extra3,
    int extra4, int extra5, int extra6, int extra7, int extra8)
{
    int x0 = WAR.Person[WAR.CurID].x;
    int y0 = WAR.Person[WAR.CurID].y;

    if (flag == 0)
    {
        JY_DrawWarMap(0, x0, y0, -1, -1, -1, -1, -1, -1, -1, g_Config.WMapAddX, g_Config.WMapAddY);
    }
    else if (flag == 1)  // 移动路径
    {
        JY_DrawWarMap(1, x0, y0, extra1, extra2, -1, -1, -1, -1, -1, g_Config.WMapAddX, g_Config.WMapAddY);
    }
    else if (flag == 2)  // 命中目标
    {
        JY_DrawWarMap(2, x0, y0, -1, -1, -1, -1, -1, -1, -1, g_Config.WMapAddX, g_Config.WMapAddY);
    }
    else if (flag == 4)  // 战斗动画
    {
        // extra1=pic*2, extra2=mytype, extra3=eft*2 or -1, extra4=nil, extra5=3, extra6=-1, extra7=ex, extra8=ey
        JY_DrawWarMap(4, x0, y0, extra1, extra2, extra3,
            extra4 >= 0 ? extra4 : -1,
            extra5 >= 0 ? extra5 : -1,
            extra6,
            extra7 >= 0 ? extra7 : -1,
            g_Config.WMapAddX, g_Config.WMapAddY);
    }

    if (WAR.ShowHead == 1)
        WarShowHead();
}

// ============= WarPersonSort =============
void WarPersonSort()
{
    // 计算速度
    for (int i = 0; i < WAR.PersonNum; i++)
    {
        auto p = g_JY.getPerson(WAR.Person[i].personId);
        int spd = p.agility();
        if (p.weapon() >= 0)
        {
            auto thing = g_JY.getThing(p.weapon());
            spd += thing.addAgility();
        }
        if (p.armor() >= 0)
        {
            auto thing = g_JY.getThing(p.armor());
            spd += thing.addAgility();
        }
        WAR.Person[i].speed = spd;
    }

    // 选择排序（按速度降序）
    for (int i = 0; i < WAR.PersonNum - 1; i++)
    {
        int maxIdx = i;
        for (int j = i + 1; j < WAR.PersonNum; j++)
        {
            if (WAR.Person[j].speed > WAR.Person[maxIdx].speed)
                maxIdx = j;
        }
        if (maxIdx != i)
            std::swap(WAR.Person[i], WAR.Person[maxIdx]);
    }
}

// ============= WarSetPerson =============
void WarSetPerson()
{
    // 清除人物位
    CleanWarMap(2, -1);
    CleanWarMap(5, -1);

    for (int i = 0; i < WAR.PersonNum; i++)
    {
        if (WAR.Person[i].dead) continue;
        SetWarMap(WAR.Person[i].x, WAR.Person[i].y, 2, i);
        SetWarMap(WAR.Person[i].x, WAR.Person[i].y, 5, WAR.Person[i].pic);
    }
}

// ============= WarCalPersonPic =============
int WarCalPersonPic(int id)
{
    int pid = WAR.Person[id].personId;
    auto p = g_JY.getPerson(pid);
    return (5106 + p.headId() * 8 + WAR.Person[id].direction * 2);
}

// ============= WarShowHead =============
void WarShowHead()
{
    int id = WAR.CurID;
    int pid = WAR.Person[id].personId;
    auto p = g_JY.getPerson(pid);
    int h = g_CC.DefaultFont;
    int width = g_CC.Fontsmall * 10 + 2 * g_CC.MenuBorderPixel;
    int height = (g_CC.Fontsmall + g_CC.RowPixel) * 7 + 2 * g_CC.MenuBorderPixel;
    int x1, y1;

    if (WAR.Person[id].isAlly)
    {
        x1 = g_CC.ScreenW - width - 10;
        y1 = g_CC.ScreenH - height - g_CC.ScreenH / 2;
        DrawBox(x1, y1, x1 + width, y1 + height + g_CC.ScreenH / 8, C_WHITE);
    }
    else
    {
        x1 = 10;
        y1 = 10;
        DrawBox(x1, y1, x1 + width, y1 + height + 30, C_WHITE);
    }

    int headw = 0, headh = 0;
    JY_GetPNGXY(1, p.headId() * 2, &headw, &headh, nullptr, nullptr);

    int headx = (width - headw) / 2;
    int heady = (g_CC.ScreenH / 5 - headh) / 2;
    JY_LoadPNG(1, p.headId() * 2, x1 + 5 + headw, y1 + 5 + heady, 1, 0, 100);

    x1 = x1 + g_CC.RowPixel;
    y1 = y1 + g_CC.RowPixel + headh;
    int color;

    if (p.injury() < p.poison())
    {
        if (p.poison() == 0)
            color = RGB_JY(252, 148, 16);
        else if (p.poison() < 50)
            color = RGB_JY(120, 208, 88);
        else
            color = RGB_JY(56, 136, 36);
    }
    else if (p.injury() < 33)
        color = RGB_JY(236, 200, 40);
    else if (p.injury() < 66)
        color = RGB_JY(244, 128, 32);
    else
        color = RGB_JY(232, 32, 44);

    MyDrawString(x1 + (width / 2), x1 + width, y1 + g_CC.RowPixel, p.name(), color, g_CC.DefaultFont);
    y1 = y1 + g_CC.DefaultFont + g_CC.RowPixel;

    DrawString(x1 + 3, y1 + g_CC.RowPixel, "生命:", C_ORANGE, g_CC.Fontsmall);
    DrawString(x1 + 3, y1 + g_CC.RowPixel + g_CC.RowPixel + g_CC.Fontsmall, "内力:", C_ORANGE, g_CC.Fontsmall);
    DrawString(x1 + 3, y1 + g_CC.RowPixel + 2 * (g_CC.RowPixel + g_CC.Fontsmall), "体力:", C_ORANGE, g_CC.Fontsmall);
    DrawString(x1 + 3, y1 + g_CC.RowPixel + 3 * (g_CC.RowPixel + g_CC.Fontsmall), "中毒:", C_ORANGE, g_CC.Fontsmall);

    char buf[64];
    snprintf(buf, sizeof(buf), "%d/%d", p.hp(), p.maxHp());

    if (p.injury() < 33)
        color = RGB_JY(236, 200, 40);
    else if (p.injury() < 66)
        color = RGB_JY(244, 128, 32);
    else
        color = RGB_JY(232, 32, 44);

    DrawString(x1 + g_CC.Fontsmall + 2 * g_CC.Fontsmall, y1 + g_CC.RowPixel, buf, color, g_CC.Fontsmall);

    if (p.mpType() == 0)
        color = RGB_JY(208, 152, 208);
    else if (p.mpType() == 1)
        color = RGB_JY(236, 200, 40);
    else
        color = RGB_JY(236, 236, 236);

    if (GetS(4, 5, 5, 5) == 5 && pid == 0)
        color = RGB_JY(216, 20, 24);

    snprintf(buf, sizeof(buf), "%d/%d", p.mp(), p.maxMp());
    DrawString(x1 + g_CC.Fontsmall + 2 * g_CC.Fontsmall, y1 + g_CC.RowPixel + g_CC.RowPixel + g_CC.Fontsmall,
        buf, color, g_CC.Fontsmall);

    snprintf(buf, sizeof(buf), "%d/100", p.stamina());
    DrawString(x1 + g_CC.Fontsmall + 2 * g_CC.Fontsmall, y1 + g_CC.RowPixel + 2 * (g_CC.RowPixel + g_CC.Fontsmall),
        buf, C_GOLD, g_CC.Fontsmall);

    snprintf(buf, sizeof(buf), "%d", p.poison());
    DrawString(x1 + 3 + 3 * g_CC.Fontsmall, y1 + g_CC.RowPixel + 3 * (g_CC.RowPixel + g_CC.Fontsmall),
        buf, RGB_JY(120, 208, 88), g_CC.Fontsmall);

    y1 = y1 + 2 * (g_CC.RowPixel + g_CC.Fontsmall);

    // 敌方显示携带物品
    if (!WAR.Person[id].isAlly)
    {
        y1 = y1 + 2 * (g_CC.RowPixel + g_CC.Fontsmall);
        DrawBox(x1 - 5, y1, x1 + width - 5, y1 + g_CC.DefaultFont * 6, C_WHITE);
        int hl = 1;
        for (int i = 1; i <= 4; i++)
        {
            int wp = p.carryItem(i);
            int wps = p.carryItemNum(i);
            if (wp >= 0)
            {
                auto t = g_JY.getThing(wp);
                snprintf(buf, sizeof(buf), "%s%d", t.name().c_str(), wps);
                DrawString(x1, y1 + hl * (g_CC.DefaultFont + g_CC.RowPixel), buf, C_ORANGE, g_CC.DefaultFont);
                hl++;
            }
        }
    }
}

// ============= War_GetMinNeiLi =============
int War_GetMinNeiLi(int pid)
{
    int minv = INT_MAX;
    auto p = g_JY.getPerson(pid);
    for (int i = 1; i <= 10; i++)
    {
        int wid = p.wugong(i);
        if (wid > 0)
        {
            auto wg = g_JY.getWugong(wid);
            if (wg.mpCost() < minv)
                minv = wg.mpCost();
        }
    }
    return minv;
}

// ============= War_Manual =============
int War_Manual()
{
    WAR.ShowHead = 1;
    while (true)
    {
        WarDrawMap(0);
        ShowScreen();

        int r = War_Manual_Sub();
        if (r == 1) break;  // 行动完毕
    }
    WAR.ShowHead = 0;
    return 0;
}

// ============= War_Manual_Sub =============
int War_Manual_Sub()
{
    int pid = WAR.Person[WAR.CurID].personId;
    auto p = g_JY.getPerson(pid);

    std::vector<MenuItem> menu = {
        {"移动", nullptr, 1},
        {"攻击", nullptr, 1},
        {"用毒", nullptr, 1},
        {"解毒", nullptr, 1},
        {"医疗", nullptr, 1},
        {"物品", nullptr, 1},
        {"等待", nullptr, 1},
        {"状态", nullptr, 1},
        {"休息", nullptr, 1},
        {"自动", nullptr, 1},
    };

    // 不能移动
    if (p.stamina() <= 5 || WAR.Person[WAR.CurID].moveSteps <= 0)
        menu[0].enabled = 0;

    int minv = War_GetMinNeiLi(pid);

    // 不能战斗
    if (p.mp() < minv || p.stamina() < 10)
        menu[1].enabled = 0;

    // 不能用毒
    if (p.stamina() < 10 || p.usePoison() < 20)
        menu[2].enabled = 0;

    // 不能解毒
    if (p.stamina() < 10 || p.detox() < 20)
        menu[3].enabled = 0;

    // 不能医疗
    if (p.stamina() < 50 || p.medic() < 20)
        menu[4].enabled = 0;

    JY_GetKey(nullptr, nullptr, nullptr, nullptr);
    Cls();
    int r = ShowMenu(menu, 10, 0, g_CC.MainMenuX, g_CC.MainMenuY, 0, 0, 1, 0,
        g_CC.DefaultFont, C_ORANGE, C_WHITE);

    if (r == 0) return 0;  // ESC

    int result = 0;
    switch (r)
    {
    case 1: result = War_MoveMenu(); break;
    case 2: result = War_FightMenu(); break;
    case 3: result = War_PoisonMenu(); break;
    case 4: result = War_DecPoisonMenu(); break;
    case 5: result = War_DoctorMenu(); break;
    case 6: result = War_ThingMenu(); break;
    case 7: result = War_WaitMenu(); break;
    case 8: War_StatusMenu(); return 0;
    case 9: result = War_RestMenu(); break;
    case 10: result = War_AutoMenu(); break;
    }

    return result;
}

// ============= War_MoveMenu =============
int War_MoveMenu()
{
    WAR.ShowHead = 0;
    if (WAR.Person[WAR.CurID].moveSteps <= 0)
        return 0;

    War_CalMoveStep(WAR.CurID, WAR.Person[WAR.CurID].moveSteps, 0);

    int x, y;
    bool selected = War_SelectMove(x, y);
    int r;
    if (selected)
    {
        War_MovePerson(x, y);
        r = 1;
    }
    else
    {
        r = 0;
        WAR.ShowHead = 1;
        Cls();
    }
    JY_GetKey(nullptr, nullptr, nullptr, nullptr);
    return r;
}

// ============= War_CalMoveStep =============
void War_CalMoveStep(int id, int stepmax, int flag, StepArray* out, int outSize)
{
    CleanWarMap(3, 255);

    int x = WAR.Person[id].x;
    int y = WAR.Person[id].y;

    // 使用内部数组
    const int MAX_STEPS = 64;
    StepArray steparray[MAX_STEPS + 1];

    SetWarMap(x, y, 3, 0);
    steparray[0].num = 1;
    steparray[0].x[0] = x;
    steparray[0].y[0] = y;

    int maxStep = std::min(stepmax, MAX_STEPS);
    for (int i = 0; i < maxStep; i++)
    {
        War_FindNextStep(steparray, i, flag);
        if (steparray[i + 1].num == 0) break;
    }

    if (out && outSize > 0)
    {
        int copyNum = std::min(maxStep + 1, outSize);
        for (int i = 0; i < copyNum; i++)
            out[i] = steparray[i];
    }
}

// ============= War_FindNextStep =============
void War_FindNextStep(StepArray* steparray, int step, int flag)
{
    int num = 0;
    int step1 = step + 1;
    steparray[step1].num = 0;

    for (int i = 0; i < steparray[step].num; i++)
    {
        int x = steparray[step].x[i];
        int y = steparray[step].y[i];

        // 略
        if (x + 1 < g_CC.WarWidth - 1)
        {
            int v = GetWarMap(x + 1, y, 3);
            if (v == 255 && War_CanMoveXY(x + 1, y, flag))
            {
                if (num < 256)
                {
                    steparray[step1].x[num] = x + 1;
                    steparray[step1].y[num] = y;
                    num++;
                }
                SetWarMap(x + 1, y, 3, step1);
            }
        }
        // 略
        if (x - 1 > 0)
        {
            int v = GetWarMap(x - 1, y, 3);
            if (v == 255 && War_CanMoveXY(x - 1, y, flag))
            {
                if (num < 256)
                {
                    steparray[step1].x[num] = x - 1;
                    steparray[step1].y[num] = y;
                    num++;
                }
                SetWarMap(x - 1, y, 3, step1);
            }
        }
        // 略
        if (y + 1 < g_CC.WarHeight - 1)
        {
            int v = GetWarMap(x, y + 1, 3);
            if (v == 255 && War_CanMoveXY(x, y + 1, flag))
            {
                if (num < 256)
                {
                    steparray[step1].x[num] = x;
                    steparray[step1].y[num] = y + 1;
                    num++;
                }
                SetWarMap(x, y + 1, 3, step1);
            }
        }
        // 略
        if (y - 1 > 0)
        {
            int v = GetWarMap(x, y - 1, 3);
            if (v == 255 && War_CanMoveXY(x, y - 1, flag))
            {
                if (num < 256)
                {
                    steparray[step1].x[num] = x;
                    steparray[step1].y[num] = y - 1;
                    num++;
                }
                SetWarMap(x, y - 1, 3, step1);
            }
        }
    }
    steparray[step1].num = num;
}

// ============= War_CanMoveXY =============
bool War_CanMoveXY(int x, int y, int flag)
{
    if (GetWarMap(x, y, 1) > 0) return false;
    if (flag == 0)
    {
        if (g_CC.WarWater.count(GetWarMap(x, y, 0))) return false;
        if (GetWarMap(x, y, 2) >= 0) return false;
    }
    return true;
}

// ============= War_SelectMove =============
bool War_SelectMove(int& outx, int& outy)
{
    int x0 = WAR.Person[WAR.CurID].x;
    int y0 = WAR.Person[WAR.CurID].y;
    int x = x0, y = y0;

    while (true)
    {
        int x2 = x, y2 = y;
        WarDrawMap(1, x, y);
        ShowScreen();

        int key = WaitKey();
        if (key == GK_UP) y2 = y - 1;
        else if (key == GK_DOWN) y2 = y + 1;
        else if (key == GK_LEFT) x2 = x - 1;
        else if (key == GK_RIGHT) x2 = x + 1;
        else if (key == GK_SPACE || key == GK_RETURN)
        {
            outx = x; outy = y;
            return true;
        }
        else if (key == GK_ESCAPE)
            return false;

        if (GetWarMap(x2, y2, 3) < 128)
        {
            x = x2; y = y2;
        }
    }
}

// ============= War_MovePerson =============
void War_MovePerson(int x, int y)
{
    int movenum = GetWarMap(x, y, 3);
    WAR.Person[WAR.CurID].moveSteps -= movenum;

    struct MoveEntry { int x, y, direct; };
    std::vector<MoveEntry> movetable(movenum + 1);

    // 从目的位置反找路径
    int tx = x, ty = y;
    for (int i = movenum; i >= 1; i--)
    {
        movetable[i].x = tx;
        movetable[i].y = ty;
        if (GetWarMap(tx - 1, ty, 3) == i - 1) { movetable[i].direct = 1; tx--; }
        else if (GetWarMap(tx + 1, ty, 3) == i - 1) { movetable[i].direct = 2; tx++; }
        else if (GetWarMap(tx, ty - 1, 3) == i - 1) { movetable[i].direct = 3; ty--; }
        else if (GetWarMap(tx, ty + 1, 3) == i - 1) { movetable[i].direct = 0; ty++; }
    }

    for (int i = 1; i <= movenum; i++)
    {
        int t1 = JY_GetTime();

        SetWarMap(WAR.Person[WAR.CurID].x, WAR.Person[WAR.CurID].y, 2, -1);
        SetWarMap(WAR.Person[WAR.CurID].x, WAR.Person[WAR.CurID].y, 5, -1);

        WAR.Person[WAR.CurID].x = movetable[i].x;
        WAR.Person[WAR.CurID].y = movetable[i].y;
        WAR.Person[WAR.CurID].direction = movetable[i].direct;
        WAR.Person[WAR.CurID].pic = WarCalPersonPic(WAR.CurID);

        SetWarMap(WAR.Person[WAR.CurID].x, WAR.Person[WAR.CurID].y, 2, WAR.CurID);
        SetWarMap(WAR.Person[WAR.CurID].x, WAR.Person[WAR.CurID].y, 5, WAR.Person[WAR.CurID].pic);

        WarDrawMap(0);
        ShowScreen();

        int t2 = JY_GetTime();
        if (i < movenum)
        {
            if ((t2 - t1) < 2 * g_CC.Frame)
                JY_Delay(2 * g_CC.Frame - (t2 - t1));
        }
    }
}

// ============= War_FightMenu =============
int War_FightMenu()
{
    int pid = WAR.Person[WAR.CurID].personId;
    auto p = g_JY.getPerson(pid);

    int numwugong = 0;
    std::vector<MenuItem> menu;
    for (int i = 1; i <= 10; i++)
    {
        int tmp = p.wugong(i);
        if (tmp > 0)
        {
            auto wg = g_JY.getWugong(tmp);
            MenuItem mi;
            mi.label = wg.name();
            mi.enabled = (wg.mpCost() > p.mp()) ? 0 : 1;
            menu.push_back(mi);
            numwugong++;
        }
    }

    if (numwugong == 0) return 0;

    int r;
    if (numwugong == 1)
        r = 1;
    else
        r = ShowMenu(menu, numwugong, 0, g_CC.MainSubMenuX, g_CC.MainSubMenuY, 0, 0, 1, 1,
            g_CC.DefaultFont, C_ORANGE, C_WHITE);

    if (r == 0) return 0;

    WAR.ShowHead = 0;
    int r2 = War_Fight_Sub(WAR.CurID, r);
    WAR.ShowHead = 1;
    Cls();
    return r2;
}

// ============= War_Fight_Sub =============
int War_Fight_Sub(int id, int wugongnum, int x, int y)
{
    int pid = WAR.Person[id].personId;
    auto p = g_JY.getPerson(pid);
    int wugong = p.wugong(wugongnum);
    int level = (int)std::floor(p.wugongLevel(wugongnum) / 100.0) + 1;

    CleanWarMap(4, 0);

    auto wg = g_JY.getWugong(wugong);
    int fightscope = wg.attackRange();

    bool hasTarget = (x != -9999 && y != -9999);
    if (fightscope == 0)
    {
        if (!War_FightSelectType0(wugong, level, x, y))
            return 0;
    }
    else if (fightscope == 1)
        War_FightSelectType1(wugong, level, x, y);
    else if (fightscope == 2)
        War_FightSelectType2(wugong, level);
    else if (fightscope == 3)
    {
        if (!War_FightSelectType3(wugong, level, x, y))
            return 0;
    }

    int fightnum = 1;
    if (p.dualWield() == 1) fightnum = 2;

    for (int k = 1; k <= fightnum; k++)
    {
        for (int i = 0; i < g_CC.WarWidth; i++)
        {
            for (int j = 0; j < g_CC.WarHeight; j++)
            {
                int effect = GetWarMap(i, j, 4);
                if (effect > 0)
                {
                    int emeny = GetWarMap(i, j, 2);
                    if (emeny >= 0)
                    {
                        if (WAR.Person[WAR.CurID].isAlly != WAR.Person[emeny].isAlly)
                        {
                            if (wg.damageType() == 1 && (fightscope == 0 || fightscope == 3))
                            {
                                WAR.Person[emeny].points = -War_WugongHurtNeili(emeny, wugong, level);
                                SetWarMap(i, j, 4, 3);
                                WAR.Effect = 3;
                            }
                            else
                            {
                                WAR.Person[emeny].points = -War_WugongHurtLife(emeny, wugong, level);
                                WAR.Effect = 2;
                                SetWarMap(i, j, 4, 2);
                            }
                        }
                    }
                }
            }
        }

        War_ShowFight(pid, wugong, wg.wugongType(), level, x, y, wg.animAndSound());

        for (int i = 0; i < WAR.PersonNum; i++)
            WAR.Person[i].points = 0;

        WAR.Person[WAR.CurID].exp += 2;

        if (p.wugongLevel(wugongnum) < 900)
            p.setWugongLevel(wugongnum, (int16_t)(p.wugongLevel(wugongnum) + Rnd(2) + 1));

        int newLevel = (int)std::floor(p.wugongLevel(wugongnum) / 100.0) + 1;
        if (newLevel != level)
        {
            level = newLevel;
            char buf[128];
            snprintf(buf, sizeof(buf), "%s 升为 %d 级", wg.name().c_str(), level);
            DrawStrBox(-1, -1, buf, C_ORANGE, g_CC.DefaultFont);
            ShowScreen();
            JY_Delay(500);
            Cls();
            ShowScreen();
        }

        AddPersonAttrib(pid, "内力", -(int)std::floor((level + 1) / 2.0) * wg.mpCost());
    }

    AddPersonAttrib(pid, "体力", -3);
    return 1;
}

// ============= War_FightSelectType0 =============
bool War_FightSelectType0(int wugong, int level, int x1, int y1)
{
    int x0 = WAR.Person[WAR.CurID].x;
    int y0 = WAR.Person[WAR.CurID].y;
    auto wg = g_JY.getWugong(wugong);
    War_CalMoveStep(WAR.CurID, wg.moveRange(level), 1);

    bool hasXY = (x1 != -9999 && y1 != -9999);
    if (!hasXY)
    {
        if (!War_SelectMove(x1, y1))
        {
            JY_GetKey(nullptr, nullptr, nullptr, nullptr);
            Cls();
            return false;
        }
    }

    WAR.Person[WAR.CurID].direction = War_Direct(x0, y0, x1, y1);
    SetWarMap(x1, y1, 4, 1);

    WAR.EffectXY[0][0] = x1; WAR.EffectXY[0][1] = y1;
    WAR.EffectXY[1][0] = x1; WAR.EffectXY[1][1] = y1;
    return true;
}

// ============= War_FightSelectType1 =============
void War_FightSelectType1(int wugong, int level, int x, int y)
{
    int x0 = WAR.Person[WAR.CurID].x;
    int y0 = WAR.Person[WAR.CurID].y;
    int direct;

    bool hasXY = (x != -9999 && y != -9999);
    if (!hasXY)
    {
        direct = -1;
        DrawStrBox(g_CC.MainSubMenuX, g_CC.MainSubMenuY, "请选择攻击方向", C_ORANGE, g_CC.DefaultFont);
        ShowScreen();

        while (true)
        {
            int key = WaitKey();
            if (key == GK_UP) { direct = 0; break; }
            if (key == GK_DOWN) { direct = 3; break; }
            if (key == GK_LEFT) { direct = 2; break; }
            if (key == GK_RIGHT) { direct = 1; break; }
        }
        Cls(g_CC.MainSubMenuX, g_CC.MainSubMenuY, g_CC.ScreenW, g_CC.ScreenH);
        ShowScreen();
    }
    else
    {
        direct = War_Direct(x0, y0, x, y);
    }

    WAR.Person[WAR.CurID].direction = direct;
    auto wg = g_JY.getWugong(wugong);
    int move = wg.moveRange(level);

    for (int i = 1; i <= move; i++)
    {
        if (direct == 0) SetWarMap(x0, y0 - i, 4, 1);
        else if (direct == 3) SetWarMap(x0, y0 + i, 4, 1);
        else if (direct == 2) SetWarMap(x0 - i, y0, 4, 1);
        else if (direct == 1) SetWarMap(x0 + i, y0, 4, 1);
    }

    if (direct == 0) { WAR.EffectXY[0][0] = x0; WAR.EffectXY[0][1] = y0 - 1; WAR.EffectXY[1][0] = x0; WAR.EffectXY[1][1] = y0 - move; }
    else if (direct == 3) { WAR.EffectXY[0][0] = x0; WAR.EffectXY[0][1] = y0 + 1; WAR.EffectXY[1][0] = x0; WAR.EffectXY[1][1] = y0 + move; }
    else if (direct == 2) { WAR.EffectXY[0][0] = x0 - 1; WAR.EffectXY[0][1] = y0; WAR.EffectXY[1][0] = x0 - move; WAR.EffectXY[1][1] = y0; }
    else if (direct == 1) { WAR.EffectXY[0][0] = x0 + 1; WAR.EffectXY[0][1] = y0; WAR.EffectXY[1][0] = x0 + move; WAR.EffectXY[1][1] = y0; }
}

// ============= War_FightSelectType2 =============
void War_FightSelectType2(int wugong, int level)
{
    int x0 = WAR.Person[WAR.CurID].x;
    int y0 = WAR.Person[WAR.CurID].y;
    auto wg = g_JY.getWugong(wugong);
    int move = wg.moveRange(level);

    for (int i = 1; i <= move; i++)
    {
        SetWarMap(x0, y0 - i, 4, 1);
        SetWarMap(x0, y0 + i, 4, 1);
        SetWarMap(x0 - i, y0, 4, 1);
        SetWarMap(x0 + i, y0, 4, 1);
    }

    WAR.EffectXY[0][0] = x0 - move; WAR.EffectXY[0][1] = y0;
    WAR.EffectXY[1][0] = x0 + move; WAR.EffectXY[1][1] = y0;
}

// ============= War_FightSelectType3 =============
bool War_FightSelectType3(int wugong, int level, int x1, int y1)
{
    int x0 = WAR.Person[WAR.CurID].x;
    int y0 = WAR.Person[WAR.CurID].y;
    auto wg = g_JY.getWugong(wugong);
    War_CalMoveStep(WAR.CurID, wg.moveRange(level), 1);

    bool hasXY = (x1 != -9999 && y1 != -9999);
    if (!hasXY)
    {
        if (!War_SelectMove(x1, y1))
        {
            JY_GetKey(nullptr, nullptr, nullptr, nullptr);
            Cls();
            return false;
        }
    }

    WAR.Person[WAR.CurID].direction = War_Direct(x0, y0, x1, y1);
    int killmove = wg.killRange(level);

    for (int i = -killmove; i <= killmove; i++)
        for (int j = -killmove; j <= killmove; j++)
            SetWarMap(x1 + i, y1 + j, 4, 1);

    WAR.EffectXY[0][0] = x1 - 2 * killmove; WAR.EffectXY[0][1] = y1;
    WAR.EffectXY[1][0] = x1 + 2 * killmove; WAR.EffectXY[1][1] = y1;
    return true;
}

// ============= War_Direct =============
int War_Direct(int x1, int y1, int x2, int y2)
{
    int dx = x2 - x1;
    int dy = y2 - y1;
    if (std::abs(dy) > std::abs(dx))
        return (dy > 0) ? 3 : 0;
    else
        return (dx > 0) ? 1 : 2;
}

// ============= War_ShowFight =============
void War_ShowFight(int pid, int wugong, int wugongtype, int level, int x, int y, int eft)
{
    int x0 = WAR.Person[WAR.CurID].x;
    int y0 = WAR.Person[WAR.CurID].y;

    auto p = g_JY.getPerson(pid);

    int fightdelay, fightframe, sounddelay;
    if (wugongtype >= 0)
    {
        fightdelay = 7;
        fightframe = p.attackFrame(wugongtype + 1);
        sounddelay = 3;
    }
    else
    {
        fightdelay = 0;
        fightframe = -1;
        sounddelay = -1;
    }

    // 计算效果帧数
    auto& effectVec = g_CC.Effect;
    int effectFrames = 0;
    if (eft >= 0 && eft < (int)effectVec.size())
        effectFrames = effectVec[eft];

    int framenum = fightdelay + effectFrames;

    int startframe = 0;
    if (wugongtype >= 0)
    {
        for (int i = 0; i < wugongtype; i++)
            startframe += 4 * p.attackFrame(i + 1);
    }

    int starteft = 0;
    {
        auto& ev = g_CC.Effect;
        for (int i = 0; i < eft; i++)
        {
            if (i < (int)ev.size())
                starteft += ev[i];
        }
    }

    WAR.Person[WAR.CurID].picType = 0;
    WAR.Person[WAR.CurID].pic = WarCalPersonPic(WAR.CurID);

    int oldpic = WAR.Person[WAR.CurID].pic / 2;
    int oldpic_type = 0;
    int oldeft_v = -1;

    // 显示攻击动画
    for (int i = 0; i < framenum; i++)
    {
        int tstart = JY_GetTime();
        int mytype = 0;

        if (fightframe > 0)
        {
            WAR.Person[WAR.CurID].picType = 1;
            mytype = 4 + WAR.CurID;
            if (i < fightframe)
                WAR.Person[WAR.CurID].pic = (startframe + WAR.Person[WAR.CurID].direction * fightframe + i) * 2;
        }
        else
        {
            WAR.Person[WAR.CurID].picType = 0;
            WAR.Person[WAR.CurID].pic = WarCalPersonPic(WAR.CurID);
        }

        if (i == sounddelay)
        {
            auto wg = g_JY.getWugong(wugong);
            PlayWavAtk(wg.castSound());
        }

        if (i == fightdelay)
            PlayWavE(eft);

        if (i == 1 && WAR.SSFwav == 1)
            WAR.SSFwav = 0;
        if (i == 1 && WAR.LMSJwav == 1)
        {
            PlayWavAtk(31);
            WAR.LMSJwav = 0;
        }

        int pic = WAR.Person[WAR.CurID].pic / 2;

        JY_SetClip(0, 0, 0, 0);
        oldpic = pic;
        oldpic_type = mytype;

        if (i < fightdelay)
        {
            WarDrawMap(4, pic * 2, mytype, -1);
        }
        else
        {
            starteft++;
            JY_SetClip(0, 0, 0, 0);
            WarDrawMap(4, pic * 2, mytype, starteft * 2, -1, 3, -1, -1, -1);
            oldeft_v = starteft;

            int estart = JY_GetTime();
            if (g_CC.Frame - (estart - tstart) > 0)
                JY_Delay(g_CC.Frame - (estart - tstart));
        }

        ShowScreen();
        JY_SetClip(0, 0, 0, 0);

        int tend = JY_GetTime();
        if (g_CC.Frame - (tend - tstart) > 0)
            JY_Delay(g_CC.Frame - (tend - tstart));

        JY_GetKey(nullptr, nullptr, nullptr, nullptr);
    }

    JY_SetClip(0, 0, 0, 0);
    WAR.Person[WAR.CurID].picType = 0;
    WAR.Person[WAR.CurID].pic = WarCalPersonPic(WAR.CurID);
    WarSetPerson();
    WarDrawMap(0);
    ShowScreen();
    JY_Delay(200);

    WarDrawMap(2);
    ShowScreen();
    JY_Delay(200);

    WarDrawMap(0);
    ShowScreen();

    // 显示命中点数
    struct HitEntry { int x, y; std::string text; };
    std::vector<HitEntry> hitXY;

    for (int i = 0; i < WAR.PersonNum; i++)
    {
        int hx = WAR.Person[i].x;
        int hy = WAR.Person[i].y;
        if (!WAR.Person[i].dead && GetWarMap(hx, hy, 4) > 1)
        {
            int n = WAR.Person[i].points;
            char buf[32];
            snprintf(buf, sizeof(buf), "%+d", n);
            hitXY.push_back({ hx, hy, buf });
        }
    }

    if (!hitXY.empty())
    {
        struct ClipR { int x1, y1, x2, y2; };
        std::vector<ClipR> clips(hitXY.size());

        for (size_t i = 0; i < hitXY.size(); i++)
        {
            int dx = hitXY[i].x - x0;
            int dy = hitXY[i].y - y0;
            int ll = (int)hitXY[i].text.length();
            int w = ll * g_CC.DefaultFont / 2 + 1;
            clips[i].x1 = g_CC.XScale * (dx - dy) + g_CC.ScreenW / 2;
            clips[i].y1 = g_CC.YScale * (dx + dy) + g_CC.ScreenH / 2;
            clips[i].x2 = clips[i].x1 + w;
            clips[i].y2 = clips[i].y1 + g_CC.DefaultFont + 1;
        }

        for (int i = 1; i <= 15; i++)
        {
            int tstart = JY_GetTime();
            int y_off = i * 2 + 65;

            JY_SetClip(0, 0, g_CC.ScreenW, g_CC.ScreenH);
            WarDrawMap(0);
            for (size_t j = 0; j < hitXY.size(); j++)
            {
                DrawString(clips[j].x1, clips[j].y1 - y_off, hitXY[j].text,
                    WAR.EffectColor[WAR.Effect], g_CC.DefaultFont);
            }

            ShowScreen(1);
            JY_SetClip(0, 0, 0, 0);

            int tend = JY_GetTime();
            if ((tend - tstart) < g_CC.Frame)
                JY_Delay(g_CC.Frame - (tend - tstart));
        }
    }

    JY_SetClip(0, 0, 0, 0);
    WarDrawMap(0);
    ShowScreen();
}

// ============= War_WugongHurtLife =============
int War_WugongHurtLife(int emenyid, int wugong, int level)
{
    int pid = WAR.Person[WAR.CurID].personId;
    int eid = WAR.Person[emenyid].personId;
    auto myp = g_JY.getPerson(pid);
    auto ep = g_JY.getPerson(eid);

    // 计算武学常识
    int mywuxue = 0, emenywuxue = 0;
    for (int i = 0; i < WAR.PersonNum; i++)
    {
        int id = WAR.Person[i].personId;
        auto pp = g_JY.getPerson(id);
        if (!WAR.Person[i].dead && pp.knowledge() > 80)
        {
            if (WAR.Person[WAR.CurID].isAlly == WAR.Person[i].isAlly)
                mywuxue += pp.knowledge();
            else
                emenywuxue += pp.knowledge();
        }
    }

    // 计算实际武功等级
    auto wg = g_JY.getWugong(wugong);
    while (level > 1)
    {
        if ((int)std::floor((level + 1) / 2.0) * wg.mpCost() > myp.mp())
            level--;
        else
            break;
    }
    if (level <= 0) level = 1;

    // 武功武器配合
    int fightnum = 0;
    for (auto& v : g_CC.ExtraOffense)
    {
        if (v.weaponId == myp.weapon() && v.wugongId == wugong)
        {
            fightnum = v.addAttack;
            break;
        }
    }

    // 攻击力
    fightnum = fightnum + (myp.attack() * 3 + wg.attackPower(level)) / 2;

    if (myp.weapon() >= 0)
        fightnum += g_JY.getThing(myp.weapon()).addAttack();
    if (myp.armor() >= 0)
        fightnum += g_JY.getThing(myp.armor()).addAttack();
    fightnum += mywuxue;

    // 防御力
    int defencenum = ep.defense();
    if (ep.weapon() >= 0)
        defencenum += g_JY.getThing(ep.weapon()).addDefense();
    if (ep.armor() >= 0)
        defencenum += g_JY.getThing(ep.armor()).addDefense();
    defencenum += emenywuxue;

    // 实际伤害
    int hurt = (fightnum - 3 * defencenum) * 2 / 3 + Rnd(20) - Rnd(20);
    if (hurt < 0) hurt = Rnd(10) + 1;
    hurt = hurt + myp.stamina() / 15 + ep.injury() / 20;

    // 距离因素
    int offset = std::abs(WAR.Person[WAR.CurID].x - WAR.Person[emenyid].x)
        + std::abs(WAR.Person[WAR.CurID].y - WAR.Person[emenyid].y);
    if (offset < 10)
        hurt = (int)std::floor(hurt * (100.0 - (offset - 1) * 3) / 100.0);
    else
        hurt = hurt * 2 / 3;

    hurt = (int)std::floor((double)hurt);
    if (hurt <= 0) hurt = Rnd(8) + 1;

    ep.setHp((int16_t)(ep.hp() - hurt));
    WAR.Person[WAR.CurID].exp += (int)std::floor(hurt / 5.0);

    if (ep.hp() < 0)
    {
        ep.setHp(0);
        WAR.Person[WAR.CurID].exp += ep.level() * 10;
    }

    AddPersonAttrib(eid, "受伤程度", (int)std::floor(hurt / 10.0));

    // 中毒
    int poisonnum = level * wg.poisonDmg() + myp.attackPoison();
    if (ep.antiPoison() < poisonnum && ep.antiPoison() < 90)
        AddPersonAttrib(eid, "中毒程度", (int)std::floor(poisonnum / 15.0));

    return hurt;
}

// ============= War_WugongHurtNeili =============
int War_WugongHurtNeili(int enemyid, int wugong, int level)
{
    int pid = WAR.Person[WAR.CurID].personId;
    int eid = WAR.Person[enemyid].personId;
    auto wg = g_JY.getWugong(wugong);

    int addvalue = wg.addMpVal(level);
    int decvalue = wg.killMp(level);

    if (addvalue > 0)
    {
        if ((int)std::floor(addvalue / 2.0) > 0)
            AddPersonAttrib(pid, "内力最大值", Rnd((int)std::floor(addvalue / 2.0)));
        AddPersonAttrib(pid, "内力", std::abs(addvalue + Rnd(3) - Rnd(3)));
    }
    return -AddPersonAttrib(eid, "内力", -std::abs(decvalue + Rnd(3) - Rnd(3)));
}

// ============= War_PoisonMenu =============
int War_PoisonMenu()
{
    WAR.ShowHead = 0;
    int r = War_ExecuteMenu(1);
    WAR.ShowHead = 1;
    Cls();
    return r;
}

// ============= War_PoisonHurt =============
int War_PoisonHurt(int pid, int emenyid)
{
    auto p = g_JY.getPerson(pid);
    auto ep = g_JY.getPerson(emenyid);
    int v = (int)std::floor((p.usePoison() - ep.antiPoison()) / 4.0);
    if (v < 0) v = 0;
    return AddPersonAttrib(emenyid, "中毒程度", v);
}

// ============= War_DecPoisonMenu =============
int War_DecPoisonMenu()
{
    WAR.ShowHead = 0;
    int r = War_ExecuteMenu(2);
    WAR.ShowHead = 1;
    Cls();
    return r;
}

// ============= War_DoctorMenu =============
int War_DoctorMenu()
{
    WAR.ShowHead = 0;
    int r = War_ExecuteMenu(3);
    WAR.ShowHead = 1;
    Cls();
    return r;
}

// ============= War_ExecuteMenu =============
int War_ExecuteMenu(int flag, int thingid)
{
    int pid = WAR.Person[WAR.CurID].personId;
    auto p = g_JY.getPerson(pid);
    int step;

    if (flag == 1) step = (int)std::floor(p.usePoison() / 15.0) + 1;
    else if (flag == 2) step = (int)std::floor(p.detox() / 15.0) + 1;
    else if (flag == 3) step = (int)std::floor(p.medic() / 15.0) + 1;
    else if (flag == 4) step = (int)std::floor(p.hidden() / 15.0) + 1;
    else return 0;

    War_CalMoveStep(WAR.CurID, step, 1);

    int x1, y1;
    if (!War_SelectMove(x1, y1))
    {
        JY_GetKey(nullptr, nullptr, nullptr, nullptr);
        Cls();
        return 0;
    }
    return War_ExecuteMenu_Sub(x1, y1, flag, thingid);
}

// ============= War_ExecuteMenu_Sub =============
int War_ExecuteMenu_Sub(int x1, int y1, int flag, int thingid)
{
    int pid = WAR.Person[WAR.CurID].personId;
    int x0 = WAR.Person[WAR.CurID].x;
    int y0 = WAR.Person[WAR.CurID].y;

    CleanWarMap(4, 0);
    WAR.Person[WAR.CurID].direction = War_Direct(x0, y0, x1, y1);
    SetWarMap(x1, y1, 4, 1);

    int emeny = GetWarMap(x1, y1, 2);
    if (emeny >= 0)
    {
        if (flag == 1)
        {
            if (WAR.Person[WAR.CurID].isAlly != WAR.Person[emeny].isAlly)
            {
                WAR.Person[emeny].points = War_PoisonHurt(pid, WAR.Person[emeny].personId);
                SetWarMap(x1, y1, 4, 5);
                WAR.Effect = 5;
            }
        }
        else if (flag == 2)
        {
            if (WAR.Person[WAR.CurID].isAlly == WAR.Person[emeny].isAlly)
            {
                WAR.Person[emeny].points = ExecDecPoison(pid, WAR.Person[emeny].personId);
                SetWarMap(x1, y1, 4, 6);
                WAR.Effect = 6;
            }
        }
        else if (flag == 3)
        {
            if (WAR.Person[WAR.CurID].isAlly == WAR.Person[emeny].isAlly)
            {
                WAR.Person[emeny].points = ExecDoctor(pid, WAR.Person[emeny].personId);
                SetWarMap(x1, y1, 4, 4);
                WAR.Effect = 4;
            }
        }
        else if (flag == 4)
        {
            if (WAR.Person[WAR.CurID].isAlly != WAR.Person[emeny].isAlly)
            {
                WAR.Person[emeny].points = War_AnqiHurt(pid, WAR.Person[emeny].personId, thingid);
                SetWarMap(x1, y1, 4, 2);
                WAR.Effect = 2;
            }
        }
    }

    WAR.EffectXY[0][0] = x1; WAR.EffectXY[0][1] = y1;
    WAR.EffectXY[1][0] = x1; WAR.EffectXY[1][1] = y1;

    if (flag == 1) War_ShowFight(pid, 0, 0, 0, x1, y1, 30);
    else if (flag == 2) War_ShowFight(pid, 0, 0, 0, x1, y1, 36);
    else if (flag == 3) War_ShowFight(pid, 0, 0, 0, x1, y1, 0);
    else if (flag == 4)
    {
        if (emeny >= 0)
        {
            auto t = g_JY.getThing(thingid);
            War_ShowFight(pid, 0, -1, 0, x1, y1, t.hiddenAnim());
        }
    }

    for (int i = 0; i < WAR.PersonNum; i++)
        WAR.Person[i].points = 0;

    if (flag == 4)
    {
        if (emeny >= 0)
        {
            instruct_32(thingid, -1);
            return 1;
        }
        return 0;
    }

    WAR.Person[WAR.CurID].exp += 1;
    AddPersonAttrib(pid, "体力", -2);
    return 1;
}

// ============= War_ThingMenu =============
int War_ThingMenu()
{
    WAR.ShowHead = 0;
    int thing[200], thingnum[200];
    for (int i = 0; i < g_CC.MyThingNum; i++) { thing[i] = -1; thingnum[i] = 0; }

    int num = 0;
    for (int i = 0; i < g_CC.MyThingNum; i++)
    {
        int id = g_JY.Base.item(i + 1);
        if (id >= 0)
        {
            auto t = g_JY.getThing(id);
            if (t.type() == 3 || t.type() == 4)
            {
                thing[num] = id;
                thingnum[num] = g_JY.Base.itemNum(i + 1);
                num++;
            }
        }
    }

    int r = SelectThing(thing, thingnum);
    Cls();
    int rr = 0;
    if (r >= 0)
    {
        if (UseThing(r) == 1)
            rr = 1;
    }
    WAR.ShowHead = 1;
    Cls();
    return rr;
}

// ============= War_UseAnqi =============
int War_UseAnqi(int thingid)
{
    return War_ExecuteMenu(4, thingid);
}

// ============= War_AnqiHurt =============
int War_AnqiHurt(int pid, int emenyid, int thingid)
{
    auto ep = g_JY.getPerson(emenyid);
    auto pp = g_JY.getPerson(pid);
    auto t = g_JY.getThing(thingid);

    int num;
    if (ep.injury() == 0)
        num = t.addHp() / 4 - Rnd(5);
    else if (ep.injury() <= 33)
        num = t.addHp() / 3 - Rnd(5);
    else if (ep.injury() <= 66)
        num = t.addHp() / 2 - Rnd(5);
    else
        num = t.addHp() / 2 - Rnd(5);

    num = (int)std::floor((num - pp.hidden() * 2) / 3.0);
    AddPersonAttrib(emenyid, "受伤程度", (int)std::floor(-num / 4.0));

    int r = AddPersonAttrib(emenyid, "生命", (int)std::floor((double)num));

    if (t.addDetoxPoison() > 0)
    {
        int pn = (int)std::floor((t.addDetoxPoison() + pp.hidden()) / 2.0);
        pn = pn - ep.antiPoison();
        pn = limitX(pn, 0, g_CC.PersonAttribMax["用毒能力"]);
        AddPersonAttrib(emenyid, "中毒程度", pn);
    }
    return r;
}

// ============= War_RestMenu =============
int War_RestMenu()
{
    int pid = WAR.Person[WAR.CurID].personId;
    auto p = g_JY.getPerson(pid);
    int v = 3 + Rnd(3);
    AddPersonAttrib(pid, "体力", v);
    if (p.stamina() > 30)
    {
        v = 3 + Rnd((int)std::floor(p.stamina() / 10.0) - 2);
        AddPersonAttrib(pid, "生命", v);
        v = 3 + Rnd((int)std::floor(p.stamina() / 10.0) - 2);
        AddPersonAttrib(pid, "内力", v);
    }
    return 1;
}

// ============= War_WaitMenu =============
int War_WaitMenu()
{
    for (int i = WAR.CurID; i < WAR.PersonNum - 1; i++)
        std::swap(WAR.Person[i + 1], WAR.Person[i]);

    WarSetPerson();
    for (int i = 0; i < WAR.PersonNum; i++)
        WAR.Person[i].pic = WarCalPersonPic(i);

    return 1;
}

// ============= War_StatusMenu =============
void War_StatusMenu()
{
    WAR.ShowHead = 0;
    Menu_Status();
    WAR.ShowHead = 1;
    Cls();
}

// ============= War_AutoMenu =============
int War_AutoMenu()
{
    WAR.AutoFight = 1;
    WAR.ShowHead = 0;
    Cls();
    War_Auto();
    return 1;
}

// ============================================================================
// ============================== 自动战斗系统 ================================
// ============================================================================

// ============= War_Auto =============
int War_Auto()
{
    WAR.ShowHead = 1;
    WarDrawMap(0);
    ShowScreen();
    JY_Delay(g_CC.WarAutoDelay);
    WAR.ShowHead = 0;

    if (g_CC.AutoWarShowHead == 1)
        WAR.ShowHead = 1;

    int autotype = War_Think();

    if (autotype == 0)
    {
        War_AutoEscape();
        War_RestMenu();
    }
    else if (autotype == 1)
    {
        War_AutoFight();
    }
    else if (autotype == 2)
    {
        War_AutoEscape();
        War_AutoEatDrug(2);
    }
    else if (autotype == 3)
    {
        War_AutoEscape();
        War_AutoEatDrug(3);
    }
    else if (autotype == 4)
    {
        War_AutoEscape();
        War_AutoEatDrug(4);
    }
    else if (autotype == 5)
    {
        War_AutoEscape();
        War_AutoDoctor();
    }
    else if (autotype == 6)
    {
        War_AutoEscape();
        War_AutoEatDrug(6);
    }

    return 0;
}

// ============= War_Think =============
int War_Think()
{
    int pid = WAR.Person[WAR.CurID].personId;
    auto p = g_JY.getPerson(pid);
    int r = -1;

    if (p.stamina() < 10)
    {
        r = War_ThinkDrug(4);
        if (r >= 0) return r;
        return 0;
    }

    if (p.hp() < 20 || p.injury() > 50)
    {
        r = War_ThinkDrug(2);
        if (r >= 0) return r;
    }

    int rate = -1;
    if (p.hp() < p.maxHp() / 5) rate = 90;
    else if (p.hp() < p.maxHp() / 4) rate = 70;
    else if (p.hp() < p.maxHp() / 3) rate = 50;
    else if (p.hp() < p.maxHp() / 2) rate = 25;

    if (Rnd(100) < rate)
    {
        r = War_ThinkDrug(2);
        if (r >= 0) return r;
        r = War_ThinkDoctor();
        if (r >= 0) return r;
    }

    rate = -1;
    if (p.mp() < p.maxMp() / 5) rate = 75;
    else if (p.mp() < p.maxMp() / 4) rate = 50;

    if (Rnd(100) < rate)
    {
        r = War_ThinkDrug(3);
        if (r >= 0) return r;
    }

    rate = -1;
    if (p.poison() > g_CC.PersonAttribMax["中毒程度"] * 3 / 4) rate = 60;
    else if (p.poison() > g_CC.PersonAttribMax["中毒程度"] / 2) rate = 30;

    if (Rnd(100) < rate)
    {
        r = War_ThinkDrug(6);
        if (r >= 0) return r;
    }

    int minNeili = War_GetMinNeiLi(pid);
    if (p.mp() >= minNeili)
        r = 1;
    else
        r = 0;

    return r;
}

// ============= War_ThinkDrug =============
int War_ThinkDrug(int flag)
{
    int pid = WAR.Person[WAR.CurID].personId;
    int r = -1;
    std::string str;

    if (flag == 2) str = "加生命";
    else if (flag == 3) str = "加内力";
    else if (flag == 4) str = "加体力";
    else if (flag == 6) str = "加中毒解药";
    else return r;

    auto getAdd = [&](int thingid) -> int {
        auto t = g_JY.getThing(thingid);
        int v = t.getByName(str);
        if (flag == 6) return -v;
        return v;
    };

    if (WAR.Person[WAR.CurID].isAlly)
    {
        for (int i = 1; i <= g_CC.MyThingNum; i++)
        {
            int thingid = g_JY.Base.item(i);
            if (thingid >= 0)
            {
                auto t = g_JY.getThing(thingid);
                if (t.type() == 3 && getAdd(thingid) > 0)
                {
                    r = flag;
                    break;
                }
            }
        }
    }
    else
    {
        auto p = g_JY.getPerson(pid);
        for (int i = 1; i <= 4; i++)
        {
            int thingid = p.carryItem(i);
            if (thingid >= 0)
            {
                auto t = g_JY.getThing(thingid);
                if (t.type() == 3 && getAdd(thingid) > 0)
                {
                    r = flag;
                    break;
                }
            }
        }
    }
    return r;
}

// ============= War_ThinkDoctor =============
int War_ThinkDoctor()
{
    int pid = WAR.Person[WAR.CurID].personId;
    auto p = g_JY.getPerson(pid);

    if (p.stamina() < 50 || p.medic() < 20) return -1;
    if (p.injury() > p.medic() + 20) return -1;

    int rate = -1;
    int v = p.maxHp() - p.hp();
    if (p.medic() < v / 4) rate = 30;
    else if (p.medic() < v / 3) rate = 50;
    else if (p.medic() < v / 2) rate = 70;
    else rate = 90;

    if (Rnd(100) < rate) return 5;
    return -1;
}

// ============= War_AutoFight =============
void War_AutoFight()
{
    int wugongnum = War_AutoSelectWugong();
    if (wugongnum <= 0)
    {
        War_AutoEscape();
        War_RestMenu();
        return;
    }

    int r = War_AutoMove(wugongnum);
    if (r == 1)
        War_AutoExecuteFight(wugongnum);
    else
        War_RestMenu();
}

// ============= War_AutoSelectWugong =============
int War_AutoSelectWugong()
{
    int pid = WAR.Person[WAR.CurID].personId;
    auto p = g_JY.getPerson(pid);

    int probability[10] = {};
    int wugongnum = 10;

    for (int i = 1; i <= 10; i++)
    {
        int wugongid = p.wugong(i);
        if (wugongid > 0)
        {
            auto wg = g_JY.getWugong(wugongid);
            if (wg.damageType() == 0)
            {
                if (wg.mpCost() <= p.mp())
                {
                    int level = (int)std::floor(p.wugongLevel(i) / 100.0) + 1;
                    probability[i - 1] = (p.attack() * 3 + wg.attackPower(level)) / 2;
                }
            }
            else
            {
                probability[i - 1] = 10;
            }
        }
        else
        {
            wugongnum = i - 1;
            break;
        }
    }

    int maxoffense = 0;
    for (int i = 0; i < wugongnum; i++)
        if (probability[i] > maxoffense)
            maxoffense = probability[i];

    // 计算我方和敌方个数
    int mynum = 0, enemynum = 0;
    for (int i = 0; i < WAR.PersonNum; i++)
    {
        if (!WAR.Person[i].dead)
        {
            if (WAR.Person[i].isAlly == WAR.Person[WAR.CurID].isAlly)
                mynum++;
            else
                enemynum++;
        }
    }

    int factor = (enemynum > mynum) ? 2 : 1;

    for (int i = 0; i < wugongnum; i++)
    {
        int wugongid = p.wugong(i + 1);
        if (probability[i] > 0)
        {
            if (probability[i] < maxoffense / 2)
                probability[i] = 0;

            int extranum = 0;
            for (auto& v : g_CC.ExtraOffense)
            {
                if (v.weaponId == p.weapon() && v.wugongId == wugongid)
                {
                    extranum = v.addAttack;
                    break;
                }
            }

            auto wg = g_JY.getWugong(wugongid);
            int level = (int)std::floor(p.wugongLevel(i + 1) / 100.0) + 1;
            probability[i] += wg.attackRange() * factor * wg.killRange(level) * 20;
        }
    }

    // 累加概率
    int s[11] = {};
    int maxnum = 0;
    for (int i = 0; i < wugongnum; i++)
    {
        s[i] = maxnum;
        maxnum += probability[i];
    }
    s[wugongnum] = maxnum;

    if (maxnum == 0) return -1;

    int v = Rnd(maxnum);
    for (int i = 0; i < wugongnum; i++)
    {
        if (v >= s[i] && v < s[i + 1])
            return i + 1;
    }
    return -1;
}

// ============= War_AutoSelectEnemy =============
int War_AutoSelectEnemy()
{
    int enemyid = War_AutoSelectEnemy_near();
    WAR.Person[WAR.CurID].autoTarget = enemyid;
    return enemyid;
}

// ============= War_AutoSelectEnemy_near =============
int War_AutoSelectEnemy_near()
{
    War_CalMoveStep(WAR.CurID, 100, 1);

    int maxDest = INT_MAX;
    int nearid = -1;
    for (int i = 0; i < WAR.PersonNum; i++)
    {
        if (WAR.Person[WAR.CurID].isAlly != WAR.Person[i].isAlly)
        {
            if (!WAR.Person[i].dead)
            {
                int step = GetWarMap(WAR.Person[i].x, WAR.Person[i].y, 3);
                if (step < maxDest)
                {
                    nearid = i;
                    maxDest = step;
                }
            }
        }
    }
    return nearid;
}

// ============= War_AutoMove =============
int War_AutoMove(int wugongnum)
{
    int pid = WAR.Person[WAR.CurID].personId;
    auto p = g_JY.getPerson(pid);
    int wugongid = p.wugong(wugongnum);
    int level = (int)std::floor(p.wugongLevel(wugongnum) / 100.0) + 1;

    auto wg = g_JY.getWugong(wugongid);
    int wugongtype = wg.attackRange();
    int movescope = wg.moveRange(level);
    int fightscope = wg.killRange(level);
    int scope = movescope + fightscope;

    int bestx = -1, besty = -1;
    int bestmove = 128;
    int maxenemy = 0;

    // 计算移动步数
    const int MAX_STEPS = 64;
    StepArray movestep[MAX_STEPS + 1];
    War_CalMoveStep(WAR.CurID, WAR.Person[WAR.CurID].moveSteps, 0, movestep, MAX_STEPS + 1);

    War_AutoCalMaxEnemyMap(wugongid, level);

    for (int i = 0; i <= WAR.Person[WAR.CurID].moveSteps && i <= MAX_STEPS; i++)
    {
        if (movestep[i].num == 0) break;
        for (int j = 0; j < movestep[i].num; j++)
        {
            int xx = movestep[i].x[j];
            int yy = movestep[i].y[j];

            int num = 0;
            if (wugongtype == 0 || wugongtype == 2 || wugongtype == 3)
            {
                num = GetWarMap(xx, yy, 4);
            }
            else if (wugongtype == 1)
            {
                int v = GetWarMap(xx, yy, 4);
                if (v > 0)
                {
                    int tmpx, tmpy;
                    num = War_AutoCalMaxEnemy(xx, yy, wugongid, level, tmpx, tmpy);
                }
            }

            if (num > maxenemy)
            {
                maxenemy = num;
                bestx = xx;
                besty = yy;
                bestmove = i;
            }
            else if (num == maxenemy && num > 0)
            {
                if (Rnd(3) == 0)
                {
                    maxenemy = num;
                    bestx = xx;
                    besty = yy;
                    bestmove = i;
                }
            }
        }
    }

    if (maxenemy > 0)
    {
        War_CalMoveStep(WAR.CurID, WAR.Person[WAR.CurID].moveSteps, 0);
        War_MovePerson(bestx, besty);
        return 1;
    }

    // 无法直接攻击，尝试走向敌人
    bool found = false;
    int fx, fy;
    War_GetCanFightEnemyXY(scope, fx, fy, found);

    if (!found)
    {
        int enemyid = War_AutoSelectEnemy();
        War_CalMoveStep(WAR.CurID, 100, 0);

        int minDest = INT_MAX;
        bestx = -1; besty = -1;
        for (int i = 0; i < g_CC.WarWidth; i++)
        {
            for (int j = 0; j < g_CC.WarHeight; j++)
            {
                int dest = GetWarMap(i, j, 3);
                if (dest < 128 && enemyid >= 0)
                {
                    int dx = std::abs(i - WAR.Person[enemyid].x);
                    int dy = std::abs(j - WAR.Person[enemyid].y);
                    if (minDest > (dx + dy))
                    {
                        minDest = dx + dy;
                        bestx = i; besty = j;
                    }
                    else if (minDest == (dx + dy) && Rnd(2) == 0)
                    {
                        bestx = i; besty = j;
                    }
                }
            }
        }

        if (minDest < INT_MAX)
        {
            // 反向寻路
            int tx = bestx, ty = besty;
            while (true)
            {
                int si = GetWarMap(tx, ty, 3);
                if (si <= WAR.Person[WAR.CurID].moveSteps) break;

                if (GetWarMap(tx - 1, ty, 3) == si - 1) tx--;
                else if (GetWarMap(tx + 1, ty, 3) == si - 1) tx++;
                else if (GetWarMap(tx, ty - 1, 3) == si - 1) ty--;
                else if (GetWarMap(tx, ty + 1, 3) == si - 1) ty++;
                else break;
            }
            War_MovePerson(tx, ty);
        }
    }
    else
    {
        // 反向寻路到可攻击位置
        int tx = fx, ty = fy;
        while (true)
        {
            int si = GetWarMap(tx, ty, 3);
            if (si <= WAR.Person[WAR.CurID].moveSteps) break;

            if (GetWarMap(tx - 1, ty, 3) == si - 1) tx--;
            else if (GetWarMap(tx + 1, ty, 3) == si - 1) tx++;
            else if (GetWarMap(tx, ty - 1, 3) == si - 1) ty--;
            else if (GetWarMap(tx, ty + 1, 3) == si - 1) ty++;
            else break;
        }
        War_MovePerson(tx, ty);
    }
    return 0;
}

// ============= War_GetCanFightEnemyXY =============
void War_GetCanFightEnemyXY(int scope, int& outx, int& outy, bool& found)
{
    int minStep = INT_MAX;
    found = false;

    War_CalMoveStep(WAR.CurID, 100, 0);
    for (int x = 0; x < g_CC.WarWidth; x++)
    {
        for (int y = 0; y < g_CC.WarHeight; y++)
        {
            if (GetWarMap(x, y, 4) > 0)
            {
                int step = GetWarMap(x, y, 3);
                if (step < 128)
                {
                    if (minStep > step)
                    {
                        minStep = step;
                        outx = x; outy = y;
                        found = true;
                    }
                    else if (minStep == step && Rnd(2) == 0)
                    {
                        outx = x; outy = y;
                    }
                }
            }
        }
    }
}

// ============= War_AutoCalMaxEnemyMap =============
void War_AutoCalMaxEnemyMap(int wugongid, int level)
{
    auto wg = g_JY.getWugong(wugongid);
    int wugongtype = wg.attackRange();
    int movescope = wg.moveRange(level);

    CleanWarMap(4, 0);

    if (wugongtype == 0 || wugongtype == 3)
    {
        for (int n = 0; n < WAR.PersonNum; n++)
        {
            if (n != WAR.CurID && !WAR.Person[n].dead
                && WAR.Person[n].isAlly != WAR.Person[WAR.CurID].isAlly)
            {
                StepArray mstep[65];
                War_CalMoveStep(n, movescope, 1, mstep, 65);
                for (int i = 1; i <= movescope; i++)
                {
                    if (mstep[i].num == 0) break;
                    for (int j = 0; j < mstep[i].num; j++)
                        SetWarMap(mstep[i].x[j], mstep[i].y[j], 4, 1);
                }
            }
        }
    }
    else if (wugongtype == 1 || wugongtype == 2)
    {
        for (int n = 0; n < WAR.PersonNum; n++)
        {
            if (n != WAR.CurID && !WAR.Person[n].dead
                && WAR.Person[n].isAlly != WAR.Person[WAR.CurID].isAlly)
            {
                int xx = WAR.Person[n].x;
                int yy = WAR.Person[n].y;
                for (int direct = 0; direct < 4; direct++)
                {
                    for (int i = 1; i <= movescope; i++)
                    {
                        int xnew = xx + g_CC.DirectX[direct] * i;
                        int ynew = yy + g_CC.DirectY[direct] * i;
                        if (xnew >= 0 && xnew < g_CC.WarWidth && ynew >= 0 && ynew < g_CC.WarHeight)
                        {
                            int v = GetWarMap(xnew, ynew, 4);
                            SetWarMap(xnew, ynew, 4, v + 1);
                        }
                    }
                }
            }
        }
    }
}

// ============= War_AutoCalMaxEnemy =============
int War_AutoCalMaxEnemy(int x, int y, int wugongid, int level, int& outx, int& outy)
{
    auto wg = g_JY.getWugong(wugongid);
    int wugongtype = wg.attackRange();
    int movescope = wg.moveRange(level);
    int fightscope = wg.killRange(level);

    int maxnum = 0;
    outx = x; outy = y;

    if (wugongtype == 0 || wugongtype == 3)
    {
        StepArray mstep[65];
        War_CalMoveStep(WAR.CurID, movescope, 1, mstep, 65);
        for (int i = 1; i <= movescope; i++)
        {
            if (mstep[i].num == 0) break;
            for (int j = 0; j < mstep[i].num; j++)
            {
                int xx = mstep[i].x[j];
                int yy = mstep[i].y[j];
                int enemynum = 0;
                for (int n = 0; n < WAR.PersonNum; n++)
                {
                    if (n != WAR.CurID && !WAR.Person[n].dead
                        && WAR.Person[n].isAlly != WAR.Person[WAR.CurID].isAlly)
                    {
                        int dx = std::abs(WAR.Person[n].x - xx);
                        int dy = std::abs(WAR.Person[n].y - yy);
                        if (dx <= fightscope && dy <= fightscope)
                            enemynum++;
                    }
                }
                if (enemynum > maxnum)
                {
                    maxnum = enemynum;
                    outx = xx; outy = yy;
                }
            }
        }
    }
    else if (wugongtype == 1)
    {
        for (int direct = 0; direct < 4; direct++)
        {
            int enemynum = 0;
            for (int i = 1; i <= movescope; i++)
            {
                int xnew = x + g_CC.DirectX[direct] * i;
                int ynew = y + g_CC.DirectY[direct] * i;
                if (xnew >= 0 && xnew < g_CC.WarWidth && ynew >= 0 && ynew < g_CC.WarHeight)
                {
                    int id = GetWarMap(xnew, ynew, 2);
                    if (id >= 0 && WAR.Person[WAR.CurID].isAlly != WAR.Person[id].isAlly)
                        enemynum++;
                }
            }
            if (enemynum > maxnum)
            {
                maxnum = enemynum;
                outx = x + g_CC.DirectX[direct];
                outy = y + g_CC.DirectY[direct];
            }
        }
    }
    else if (wugongtype == 2)
    {
        int enemynum = 0;
        for (int direct = 0; direct < 4; direct++)
        {
            for (int i = 1; i <= movescope; i++)
            {
                int xnew = x + g_CC.DirectX[direct] * i;
                int ynew = y + g_CC.DirectY[direct] * i;
                if (xnew >= 0 && xnew < g_CC.WarWidth && ynew >= 0 && ynew < g_CC.WarHeight)
                {
                    int id = GetWarMap(xnew, ynew, 2);
                    if (id >= 0 && WAR.Person[WAR.CurID].isAlly != WAR.Person[id].isAlly)
                        enemynum++;
                }
            }
        }
        if (enemynum > 0)
        {
            maxnum = enemynum;
            outx = x; outy = y;
        }
    }

    return maxnum;
}

// ============= War_AutoExecuteFight =============
void War_AutoExecuteFight(int wugongnum)
{
    int pid = WAR.Person[WAR.CurID].personId;
    int x0 = WAR.Person[WAR.CurID].x;
    int y0 = WAR.Person[WAR.CurID].y;
    auto p = g_JY.getPerson(pid);
    int wugongid = p.wugong(wugongnum);
    int level = (int)std::floor(p.wugongLevel(wugongnum) / 100.0) + 1;

    int outx, outy;
    int maxnum = War_AutoCalMaxEnemy(x0, y0, wugongid, level, outx, outy);

    if (maxnum > 0)
        War_Fight_Sub(WAR.CurID, wugongnum, outx, outy);
}

// ============= War_AutoEscape =============
void War_AutoEscape()
{
    int pid = WAR.Person[WAR.CurID].personId;
    auto p = g_JY.getPerson(pid);
    if (p.stamina() <= 5) return;

    int maxDest = 0;
    int bestx = -1, besty = -1;

    War_CalMoveStep(WAR.CurID, WAR.Person[WAR.CurID].moveSteps, 0);

    for (int i = 0; i < g_CC.WarWidth; i++)
    {
        for (int j = 0; j < g_CC.WarHeight; j++)
        {
            if (GetWarMap(i, j, 3) < 128)
            {
                int minDist = INT_MAX;
                for (int k = 0; k < WAR.PersonNum; k++)
                {
                    if (WAR.Person[WAR.CurID].isAlly != WAR.Person[k].isAlly && !WAR.Person[k].dead)
                    {
                        int dx = std::abs(i - WAR.Person[k].x);
                        int dy = std::abs(j - WAR.Person[k].y);
                        if (minDist > (dx + dy))
                            minDist = dx + dy;
                    }
                }
                if (minDist > maxDest)
                {
                    maxDest = minDist;
                    bestx = i; besty = j;
                }
            }
        }
    }

    if (maxDest > 0 && bestx >= 0)
        War_MovePerson(bestx, besty);
}

// ============= War_AutoEatDrug =============
void War_AutoEatDrug(int flag)
{
    int pid = WAR.Person[WAR.CurID].personId;
    auto p = g_JY.getPerson(pid);

    std::string str;
    int shouldadd, maxattrib;

    if (flag == 2)
    {
        maxattrib = p.maxHp();
        shouldadd = maxattrib - p.hp();
        str = "加生命";
    }
    else if (flag == 3)
    {
        maxattrib = p.maxMp();
        shouldadd = maxattrib - p.mp();
        str = "加内力";
    }
    else if (flag == 4)
    {
        maxattrib = g_CC.PersonAttribMax["体力"];
        shouldadd = maxattrib - p.stamina();
        str = "加体力";
    }
    else if (flag == 6)
    {
        maxattrib = g_CC.PersonAttribMax["中毒程度"];
        shouldadd = p.poison();
        str = "加中毒解药";
    }
    else return;

    auto getAdd = [&](int thingid) -> int {
        auto t = g_JY.getThing(thingid);
        int v = t.getByName(str);
        if (flag == 6) return -v / 2;
        return v;
    };

    int selectid = -1;

    if (WAR.Person[WAR.CurID].isAlly)
    {
        // 先找恰好够用的
        int minvalue = INT_MAX;
        bool extra = false;
        for (int i = 1; i <= g_CC.MyThingNum; i++)
        {
            int thingid = g_JY.Base.item(i);
            if (thingid >= 0)
            {
                auto t = g_JY.getThing(thingid);
                int add = getAdd(thingid);
                if (t.type() == 3 && add > 0)
                {
                    int v = shouldadd - add;
                    if (v < 0) { extra = true; break; }
                    if (v < minvalue) { minvalue = v; selectid = thingid; }
                }
            }
        }
        if (extra)
        {
            // 找浪费最小的
            minvalue = INT_MAX;
            for (int i = 1; i <= g_CC.MyThingNum; i++)
            {
                int thingid = g_JY.Base.item(i);
                if (thingid >= 0)
                {
                    auto t = g_JY.getThing(thingid);
                    int add = getAdd(thingid);
                    if (t.type() == 3 && add > 0)
                    {
                        int v = add - shouldadd;
                        if (v >= 0 && v < minvalue) { minvalue = v; selectid = thingid; }
                    }
                }
            }
        }

        if (selectid >= 0 && UseThingEffect(selectid, pid) == 1)
            instruct_32(selectid, -1);
    }
    else
    {
        int minvalue = INT_MAX;
        bool extra = false;
        for (int i = 1; i <= 4; i++)
        {
            int thingid = p.carryItem(i);
            if (thingid >= 0)
            {
                auto t = g_JY.getThing(thingid);
                int add = getAdd(thingid);
                if (t.type() == 3 && add > 0)
                {
                    int v = shouldadd - add;
                    if (v < 0) { extra = true; break; }
                    if (v < minvalue) { minvalue = v; selectid = thingid; }
                }
            }
        }
        if (extra)
        {
            minvalue = INT_MAX;
            for (int i = 1; i <= 4; i++)
            {
                int thingid = p.carryItem(i);
                if (thingid >= 0)
                {
                    auto t = g_JY.getThing(thingid);
                    int add = getAdd(thingid);
                    if (t.type() == 3 && add > 0)
                    {
                        int v = add - shouldadd;
                        if (v >= 0 && v < minvalue) { minvalue = v; selectid = thingid; }
                    }
                }
            }
        }

        if (selectid >= 0 && UseThingEffect(selectid, pid) == 1)
            instruct_41(pid, selectid, -1);
    }

    JY_Delay(500);
}

// ============= War_AutoDoctor =============
void War_AutoDoctor()
{
    int x1 = WAR.Person[WAR.CurID].x;
    int y1 = WAR.Person[WAR.CurID].y;
    War_ExecuteMenu_Sub(x1, y1, 3, -1);
}
