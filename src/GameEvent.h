// GameEvent.h - 事件解释器
// 从 readkdef.lua 转换而来
#pragma once

#include <string>
#include <vector>
#include <variant>

// x50变量数组 - 可存储int或2字节字符串
struct X50Var {
    int32_t ival = 0;
    char sval[3] = {0, 0, 0};
    bool isStr = false;

    void setInt(int v) { ival = v; isStr = false; }
    void setStr(const char* s) { sval[0] = s[0]; sval[1] = s[1]; sval[2] = 0; isStr = true; }
    int asInt() const {
        if (isStr) {
            unsigned char l = (unsigned char)sval[0];
            unsigned char h = (unsigned char)sval[1];
            return l + h * 256;
        }
        return ival;
    }
};

extern X50Var x50[32769]; // 0..32768

// 事件系统
void ReadKDEF(int id);
void ReadBin();
std::string ReadTalk(int id);
int ReadCol(int id);
