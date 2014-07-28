 
// lua�е��õ�C API�ӿ�



#ifndef __LUAFUN_H
#define __LUAFUN_H

#ifdef __cplusplus
extern "C" {
#endif


// luafun.c


 int HAPI_Debug(lua_State *pL);

 int HAPI_DrawStr(lua_State *pL);

 int HAPI_FillColor(lua_State *pL);

 int HAPI_Background(lua_State *pL);

 int HAPI_DrawRect(lua_State *pL);

 int HAPI_LoadPicture(lua_State *pL);

int HAPI_EnableKeyRepeat(lua_State *pL);

 int HAPI_GetKey(lua_State *pL);

 int HAPI_ShowSurface(lua_State *pL);

 int HAPI_Delay(lua_State *pL);

 int HAPI_GetTime(lua_State *pL);

 int HAPI_ShowSlow(lua_State *pL);

  int HAPI_SetClip(lua_State *pL);

 int HAPI_CharSet(lua_State *pL);

 int HAPI_PicLoadCache(lua_State *pL);
 
 int HAPI_PicLoadFile(lua_State *pL);

 int HAPI_PicInit(lua_State *pL);

 int HAPI_GetPicXY(lua_State *pL);

 int HAPI_PlayMIDI(lua_State *pL);


  int HAPI_PlayWAV(lua_State *pL);
  
  int HAPI_PlayMPEG(lua_State *pL);

  int HAPI_LoadMMap(lua_State *pL);

int HAPI_UnloadMMap(lua_State *pL);

  int HAPI_DrawMMap(lua_State *pL);

  int HAPI_GetMMap(lua_State *pL);

 int HAPI_FullScreen(lua_State *pL);

int HAPI_LoadSMap(lua_State *pL);


int HAPI_SaveSMap(lua_State *pL);

int HAPI_GetS(lua_State *pL);

int HAPI_SetS(lua_State *pL);

int HAPI_GetD(lua_State *pL);

int HAPI_SetD(lua_State *pL);

int HAPI_DrawSMap(lua_State *pL);

int HAPI_LoadWarMap(lua_State *pL);

int HAPI_GetWarMap(lua_State *pL);

int HAPI_SetWarMap(lua_State *pL);

int HAPI_CleanWarMap(lua_State *pL);

int HAPI_DrawWarMap(lua_State *pL);

int HAPI_SaveSur(lua_State *pL);		//������Ļ����ʱ����
int HAPI_LoadSur(lua_State *pL);			//������ʱ���浽��Ļ
int HAPI_FreeSur(lua_State *pL);				//�ͷ�

int HAPI_SaveSur2(lua_State *pL);		//������Ļ����ʱ����
int HAPI_LoadSur2(lua_State *pL);			//������ʱ���浽��Ļ
int HAPI_FreeSur2(lua_State *pL);

int HAPI_ScreenWidth(lua_State *pL);				//��Ļ���
int HAPI_ScreenHeight(lua_State *pL);				//��Ļ�߶�

int HAPI_LoadPNGPath(lua_State *pL);				//��ȡPNGͼƬ·��
int HAPI_LoadPNG(lua_State *pL);				//��ͼƬ��ȡPNG
int HAPI_GetPNGXY(lua_State *pL);				//��ȡͼƬ�Ĵ�С��ƫ��

// ���������麯��

/*  lua ������ʽ��(ע�⣬λ�ö��Ǵ�0��ʼ
     handle=Byte_create(size);
	 Byte_release(h);
	 Byte_loadfile(h,filename,start,end);
     Byte_savefile(h,filename,start,end);
     v=Byte_get16(h,start);
	 Byte_set16(h,start,v);
     v=Byte_getu16(h,start);
	 Byte_setu16(h,start,v);
     v=Byte_get32(h,start);
	 Byte_set32(h,start,v);
	 str=Byte_getstr(h,start,length);
	 Byte_setstr(h,start,length,str);
  */

  int Byte_create(lua_State *pL);
  int Byte_release(lua_State *pL);
  int Byte_loadfile(lua_State *pL);
  int Byte_savefile(lua_State *pL);
  int Byte_get16(lua_State *pL);
  int Byte_set16(lua_State *pL);
  int Byte_getu16(lua_State *pL);
  int Byte_setu16(lua_State *pL);
  int Byte_get32(lua_State *pL);
  int Byte_set32(lua_State *pL);
  int Byte_getstr(lua_State *pL);
  int Byte_setstr(lua_State *pL);
  int Byte_hash(lua_State *pL);
 



int HAPI_InitCache(lua_State *pL);
int HAPI_LoadPic(lua_State *pL);


#ifdef __cplusplus
extern "C" {
#endif



#endif


