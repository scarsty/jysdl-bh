


//��Ӧ��ͬƽ̨�ͱ������ļ�����

//Ŀǰֻ��VC6/VC8/symbian C/gcc�²���ͨ��

#ifndef __CONFIG_H
#define __CONFIG_H


// �����������ʱ������Ŀ¼�·��ʵ��ļ�������������������Ŀ¼���ǵ�ǰĿ¼������¡�


#define CONFIG_FILE "config.lua"
#define DEBUG_FILE "debug.txt"
#define ERROR_FILE "error.txt"
#define HZMB_FILE "hzmb.dat"


//����vsnprintf. vc6����֧��
#if defined(_MSC_VER) && _MSC_VAR<1400
#define vsnprintf _vsnprintf
#endif


//�����Ƿ����smpeg��

#if !defined(__SYMBIAN32__)
#define HAS_SDL_MPEG
#endif

#if defined(_MSC_VER)
//����߾���ʱ�ӡ���x86��Ч��ֻ���ڵ��ԣ���������С��1ms��ʱ��������ʽ�����汾��Ӧ��ʹ��
// #define HIGH_PRECISION_CLOCK

//����CPUƵ�ʣ��߾���ʱ��ʹ�á���λΪMHz
#define CPU_FREQUENCY 2000
#endif

#endif

