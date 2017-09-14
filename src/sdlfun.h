

#pragma once

#include "SDL.h"

//sdlfun.c

static int KeyFilter(const void* data, const SDL_Event *event);
int InitSDL(void);

int ExitSDL(void);

Uint32 ConvertColor(Uint32 color);

int InitGame(void);

int ExitGame(void);

int JY_LoadPicture(const char* str, int x, int y);

int JY_ShowSurface(int flag);

int JY_ShowSlow(int delaytime, int Flag);

int JY_Delay(int x);

double JY_GetTime();

int JY_PlayMIDI(const char *filename);

int StopMIDI();

int JY_PlayWAV(const char *filename);

int JY_GetKey();

int JY_SetClip(int x1, int y1, int x2, int y2);


int JY_DrawRect(int x1, int y1, int x2, int y2, int color);

void HLine32(int x1, int x2, int y, int color, unsigned char *vbuffer, int lpitch);
void VLine32(int x1, int x2, int y, int color, unsigned char *vbuffer, int lpitch);

int JY_FillColor(int x1, int y1, int x2, int y2, int color);

int BlitSurface(SDL_Surface* lpdds, int x, int y, int flag, int value, int color);

int JY_Background(int x1, int y1, int x2, int y2, int Bright, int color);

int JY_PlayMPEG(const char* filename, int esckey);

int JY_FullScreen();
SDL_Surface *RotateSurface(SDL_Surface *src);

SDL_Rect RotateRect(const SDL_Rect *rect);

SDL_Rect RotateReverseRect(const SDL_Rect *rect);

int JY_SaveSur(int x, int y, int w, int h);		//保存屏幕到临时表面
int JY_LoadSur(int id, int x, int y);			//加载临时表面到屏幕
int JY_FreeSur(int id);				//释放