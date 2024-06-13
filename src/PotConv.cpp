
#include "iconv.h"

//#include <algorithm>
//#include <cstring>
//#include <map>
//#include <string>

import std.compat;

class PotConv
{
public:
    PotConv();
    virtual ~PotConv();

    static std::string conv(const std::string& src, const char* from, const char* to);
    static std::string conv(const std::string& src, const std::string& from, const std::string& to);
    static std::string cp936toutf8(const std::string& src) { return conv(src, "cp936", "utf-8"); }
    static std::string cp950toutf8(const std::string& src) { return conv(src, "cp950", "utf-8"); }
    static std::string cp950tocp936(const std::string& src) { return conv(src, "cp950", "cp936"); }
    static std::string utf8tocp936(const std::string& src) { return conv(src, "utf-8", "cp936"); }
    static void fromCP950ToCP936(const char* s0, char* s1)
    {
        auto str = PotConv::cp950tocp936(s0);
        memcpy(s1, str.data(), str.length());
    }
    static void fromCP950ToUTF8(const char* s0, char* s1)
    {
        auto str = PotConv::cp950toutf8(s0);
        memcpy(s1, str.data(), str.length());
    }
    static void fromCP936ToUTF8(const char* s0, char* s1)
    {
        auto str = PotConv::cp936toutf8(s0);
        memcpy(s1, str.data(), str.length());
    }
    static std::string to_read(const std::string& src);

private:
    std::map<std::string, iconv_t> cds_;
    static PotConv potconv_;
    static iconv_t createcd(const char* from, const char* to);
};


PotConv::PotConv()
{
}

PotConv::~PotConv()
{
    for (auto& cd : cds_)
    {
        iconv_close(cd.second);
    }
}

std::string PotConv::conv(const std::string& src, const char* from, const char* to)
{
    //const char *from_charset, const char *to_charset, const char *inbuf, size_t inlen, char *outbuf;
    iconv_t cd = createcd(from, to);
    if (cd == nullptr)
    {
        return "";
    }
    size_t inlen = src.length();
    size_t outlen = src.length() * 2;
    auto in = new char[inlen + 1];
    auto out = new char[outlen + 1];
    memset(in, 0, inlen + 1);
    memcpy(in, src.c_str(), inlen);
    memset(out, 0, outlen + 1);
    char *pin = in, *pout = out;
    if (iconv(cd, &pin, &inlen, &pout, &outlen) == -1)
    {
        out[0] = '\0';
    }
    std::string result(out, src.length() * 2 - outlen);
    delete[] in;
    delete[] out;
    return result;
}

std::string PotConv::conv(const std::string& src, const std::string& from, const std::string& to)
{
    return conv(src, from.c_str(), to.c_str());
}

std::string PotConv::to_read(const std::string& src)
{
#ifdef _WIN32
    return conv(src, "utf-8", "cp936");
#else
    return src;
#endif
}

PotConv PotConv::potconv_;

iconv_t PotConv::createcd(const char* from, const char* to)
{
    std::string cds = std::string(from) + std::string(to);
    if (potconv_.cds_.count(cds) == 0)
    {
        iconv_t cd;
        cd = iconv_open(to, from);
        potconv_.cds_[cds] = cd;
        return cd;
    }
    else
    {
        return potconv_.cds_[cds];
    }
}
