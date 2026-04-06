// GameWar.h - 战斗系统
// 从 jywar.lua 转换而来
#pragma once

#include "GameData.h"
#include <string>
#include <vector>
#include <map>

// 战斗人物信息
struct WarPersonInfo {
    int personId = -1;
    bool isAlly = true;
    int x = -1;
    int y = -1;
    bool dead = true;
    int direction = -1;
    int pic = -1;
    int picType = 0; // 0=wmap, 1=fight
    int speed = 0;
    int moveSteps = 0;
    int points = 0;
    int exp = 0;
    int autoTarget = -1;
};

// 战斗全局数据
struct WarState {
    WarDataDef Data;
    int SelectPerson[26] = {0};
    WarPersonInfo Person[26];
    int PersonNum = 0;
    int AutoFight = 0;
    int CurID = -1;
    int ShowHead = 0;
    int Effect = 0;
    int EffectColor[8] = {0};
    int EffectXY[2][2] = {{0}}; // 武功效果坐标 [0]起点 [1]终点, 每个{x,y}
    int SSFwav = 0;
    int LMSJwav = 0;
};

extern WarState WAR;

// 战斗系统入口
int WarMain(int warid, int isexp);
void WarLoad(int warid);
void WarSetGlobal();

// 战斗操作
void WarSelectTeam();
void WarSelectEnemy();
void WarLoadMap(int mapid);
void WarDrawMap(int flag, int extra1 = -1, int extra2 = -1, int extra3 = -1,
    int extra4 = -1, int extra5 = -1, int extra6 = -1, int extra7 = -1, int extra8 = -1);
void WarPersonSort();
void WarSetPerson();
int WarCalPersonPic(int id);
void WarShowHead();

// 地图操作包装
int GetWarMap(int x, int y, int level);
void SetWarMap(int x, int y, int level, int v);
void CleanWarMap(int level, int v);

// 手动/自动战斗
int War_Manual();
int War_Manual_Sub();
int War_Auto();
int War_isEnd();

// 战斗辅助
void War_PersonLostLife();
void War_EndPersonData(int isexp, int warStatus);
void War_AddPersonLevel(int personid, int addexp);
void War_PersonTrainBook(int personid);
void War_PersonTrainDrug(int personid);
int War_UseAnqi(int thingid);

// 移动系统
struct StepArray {
    int num = 0;
    int x[256] = {0};
    int y[256] = {0};
};
void War_CalMoveStep(int id, int stepmax, int flag, StepArray* out = nullptr, int outSize = 0);
void War_FindNextStep(StepArray* steparray, int step, int flag);
bool War_CanMoveXY(int x, int y, int flag);
bool War_SelectMove(int& outx, int& outy);
void War_MovePerson(int x, int y);

// 战斗菜单
int War_FightMenu();
int War_Fight_Sub(int id, int wugongnum, int x = -9999, int y = -9999);
bool War_FightSelectType0(int wugong, int level, int x = -9999, int y = -9999);
void War_FightSelectType1(int wugong, int level, int x = -9999, int y = -9999);
void War_FightSelectType2(int wugong, int level);
bool War_FightSelectType3(int wugong, int level, int x = -9999, int y = -9999);
int War_Direct(int x1, int y1, int x2, int y2);
void War_ShowFight(int pid, int wugong, int wugongtype, int level, int x, int y, int eft);
int War_WugongHurtLife(int emenyid, int wugong, int level);
int War_WugongHurtNeili(int enemyid, int wugong, int level);

// 用毒/解毒/医疗/暗器
int War_MoveMenu();
int War_PoisonMenu();
int War_DecPoisonMenu();
int War_DoctorMenu();
int War_ExecuteMenu(int flag, int thingid = -1);
int War_ExecuteMenu_Sub(int x1, int y1, int flag, int thingid);
int War_PoisonHurt(int pid, int emenyid);
int War_AnqiHurt(int pid, int emenyid, int thingid);

// 其他战斗菜单
int War_ThingMenu();
int War_RestMenu();
int War_WaitMenu();
void War_StatusMenu();
int War_AutoMenu();

// AI系统
int War_Think();
int War_ThinkDrug(int flag);
int War_ThinkDoctor();
void War_AutoFight();
int War_AutoSelectWugong();
int War_AutoSelectEnemy();
int War_AutoSelectEnemy_near();
int War_AutoMove(int wugongnum);
void War_GetCanFightEnemyXY(int scope, int& outx, int& outy, bool& found);
void War_AutoCalMaxEnemyMap(int wugongid, int level);
int War_AutoCalMaxEnemy(int x, int y, int wugongid, int level, int& outx, int& outy);
void War_AutoExecuteFight(int wugongnum);
void War_AutoEscape();
void War_AutoEatDrug(int flag);
void War_AutoDoctor();
int War_GetMinNeiLi(int pid);
