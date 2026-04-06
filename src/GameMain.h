// GameMain.h - 游戏主逻辑声明
// 从 jymain.lua 转换而来

#pragma once
#include "GameData.h"
#include <string>
#include <vector>
#include <functional>

// ====== 常量 ======
constexpr int GK_UP = 1073741906;
constexpr int GK_DOWN = 1073741905;
constexpr int GK_LEFT = 1073741904;
constexpr int GK_RIGHT = 1073741903;
constexpr int GK_SPACE = 32;
constexpr int GK_RETURN = 13;
constexpr int GK_ESCAPE = 27;
constexpr int GK_Y = 121;
constexpr int GK_N = 110;

// ====== 菜单项 ======
struct MenuItem
{
    std::string label;                              // 菜单文字
    std::function<int(std::vector<MenuItem>&, int)> callback;  // 回调函数
    int enabled = 1;                                // 是否可用
    int extra = 0;                                  // 额外数据
};

// ====== 游戏主入口 ======
int JY_GameMain();

// ====== 核心游戏循环 ======
void Game_Cycle();
void NewGame();
void Game_MMap();
void Game_SMap();
void Init_MMap();
void Init_SMap(int sceneid);

// ====== 记录加载/保存 ======
void LoadRecord(int id);
void SaveRecord(int id);

// ====== 菜单系统 ======
int ShowMenu(std::vector<MenuItem>& menu, int n, int shownum, int x, int y,
    int face, int d, int flag1, int flag2, int fontsize, int color1, int color2);
int ShowMenu2(std::vector<MenuItem>& menu, int n, int y, int fontsize, int color1, int color2);
void MMenu();
int Menu_System();
void Menu_Status();
void Menu_PersonExit();
void Menu_Doctor();
void Menu_DecPoison();
void Menu_Thing();
int SelectTeamMenu(int flag = 0);
void ShowPersonStatus(int pid);

// ====== 物品系统 ======
int SelectThing(int* thing, int* thingnum);
int UseThing(int thingid, int personid = -1);
int UseThingEffect(int thingid, int pid);
void AddPersonAttrib_str(int pid, const std::string& attr, int value);
int AddPersonAttrib(int pid, const std::string& attr, int value);
int ExecDoctor(int doctorid, int patientid);
int ExecDecPoison(int doctorid, int patientid);

// ====== 数据读写核心（替代Lua的GetDataFromStruct/SetDataFromStruct）======
void LoadData(DataBuffer& buf, int recordSize, int idx, const std::string& idxFile, const std::string& grpFile);
void SaveData(DataBuffer& buf, int recordSize, int idx, const std::string& idxFile, const std::string& grpFile);

// ====== 场景地图辅助 ======
int GetS(int scene, int x, int y, int level);
void SetS(int scene, int x, int y, int level, int v);
int GetD(int scene, int id, int datalevel);
void SetD(int scene, int id, int datalevel, int v);
bool SceneCanPass(int sceneid, int x, int y);
void Cal_D_Valid(int sceneid);
void DtoSMap(int sceneid = -1);

// ====== 绘图辅助 ======
void Cls(int x1 = 0, int y1 = 0, int x2 = 0, int y2 = 0);
void ShowScreen(int flag = 0);
void DrawString(int x, int y, const std::string& str, int color, int fontsize);
void MyDrawString(int x1, int x2, int y, const std::string& str, int color, int fontsize);
void DrawBox(int x1, int y1, int x2, int y2, int color);
void DrawStrBox(int x, int y, const std::string& str, int color, int fontsize);
void DrawStrBoxWaitKey(const std::string& str, int color, int fontsize);
int DrawStrBoxYesNo(const std::string& str, int color, int fontsize);

// ====== 人物贴图 ======
int GetMyPic(int direction = -1, int step = 0);
void AddMyCurrentPic(int direction = -1, int step = 0);

// ====== 绘制场景地图 ======
void DrawSMap(int sceneid = -1, int x = -1, int y = -1, int mypic = -1);
struct CalcClipRectResult { int x1, y1, x2, y2; };
CalcClipRectResult Cal_PicClip(int dx1, int dy1, int pic1, int type1, int dx2, int dy2, int pic2, int type2);
CalcClipRectResult* CalcClipRect(CalcClipRectResult r);
CalcClipRectResult MergeRect(CalcClipRectResult a, CalcClipRectResult b);

// ====== 音乐/音效 ======
void PlayMIDI(int id);
void PlayWAV(const std::string& file);
void PlayWavAtk(int id);
void PlayWavE(int id);

// ====== 等待键 ======
int WaitKey();

// ====== 修炼经验计算 ======
int TrainNeedExp(int pid);

// ====== 事件执行 ======
void EventExecute(int sceneid, int did);

// ====== 退出确认 ======
int Menu_Exit();

// ====== 清理内存 ======
void CleanMemory();

// ====== 事件系统指令（被readkdef调用，也被其他模块调用）======
void TalkEx(const std::string& s, int headid, int flag);
void instruct_15();
bool instruct_16(int personid);
void instruct_27(int id, int startpic, int endpic);
void instruct_2(int thingid, int num);
void instruct_32(int thingid, int addnum);
void instruct_41(int pid, int thingid, int addnum);
void instruct_3(int sceneid, int id, int v0, int v1, int v2, int v3, int v4,
    int v5, int v6, int v7, int v8, int v9, int v10);
bool instruct_4(int thingid);
bool instruct_5();
void instruct_8(int musicid);
bool instruct_9();
void instruct_10(int personid);
bool instruct_11();
void instruct_12();
void instruct_13();
void instruct_14();
bool instruct_18(int thingid);
void instruct_19(int x, int y);
bool instruct_20();
void instruct_21(int personid);
void instruct_22();
void instruct_23(int personid, int value);
void instruct_25(int x1, int y1, int x2, int y2);
void instruct_26(int sceneid, int id, int v1, int v2, int v3);
bool instruct_28(int personid, int vmin, int vmax);
bool instruct_29(int personid, int vmin, int vmax);
void instruct_30(int x1, int y1, int x2, int y2);
bool instruct_31(int num);
void instruct_33(int personid, int wugongid, int flag);
void instruct_35(int personid, int id, int wugongid, int wugonglevel);
bool instruct_36(int sex);
void instruct_37(int v);
void instruct_38(int sceneid, int level, int oldpic, int newpic);
void instruct_39(int sceneid);
void instruct_40(int v);
bool instruct_42();
bool instruct_43(int thingid);
void instruct_44(int id1, int sp1, int ep1, int id2, int sp2, int ep2);
bool instruct_50(int id1, int id2, int id3, int id4, int id5);
bool instruct_55(int id, int num);
void instruct_56(int v);
bool instruct_60(int sceneid, int id, int num);
void instruct_63(int personid, int sex);
void instruct_66(int id);
void instruct_67(int id);
void instruct_17(int sceneid, int level, int x, int y, int v);
