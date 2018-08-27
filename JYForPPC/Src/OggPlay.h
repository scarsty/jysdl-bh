
#ifndef OGG_PLAY_H
#define OGG_PLAY_H

#include "common.h"

#ifdef __cplusplus

extern "C"
{
#endif

VOID OGG_FillBuffer(LPBYTE stream,INT len);

INT OGG_Init(VOID);

VOID OGG_Shutdown(VOID);

VOID OGG_Play(INT iNumOGG,INT iFloopTimes);

#ifdef __cplusplus
}
#endif

#endif