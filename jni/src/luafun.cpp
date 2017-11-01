 
// 与lua库的交互函数,使用lua5.1.2版


  
#include <stdio.h>
#include <string.h>
#ifdef __SYMBIAN32__
#include <stdlib.h>
#else
#include <malloc.h>
#endif

#include "jymain.h"
#ifdef ANDROID
#include <android/log.h>
#endif
extern int g_ScreenW;
extern int g_ScreenH;
extern char JY_CurrentPath[512];

//以下为所有包装的lua接口函数，对应于每个实际的函数

int HAPI_DrawStr(lua_State *pL)
{
    int x=(int)lua_tonumber(pL,1);
	int y=(int)lua_tonumber(pL,2);
	const char *str=lua_tostring(pL,3);
	int color=(int)lua_tonumber(pL,4);
	int size=(int)lua_tonumber(pL,5);
	const char *fontname=lua_tostring(pL,6);
	int charset=(int)lua_tonumber(pL,7);
	int OScharset=(int)lua_tonumber(pL,8);

    JY_DrawStr(x, y, str,color,size,fontname,charset,OScharset);

	return 0;
}


int HAPI_FillColor(lua_State *pL)
{
	int x1=(int)lua_tonumber(pL,1);
	int y1=(int)lua_tonumber(pL,2);
	int x2=(int)lua_tonumber(pL,3);
	int y2=(int)lua_tonumber(pL,4);
	int color=(int)lua_tonumber(pL,5);

    JY_FillColor(x1,y1,x2,y2,color);
	return 0;
}


int HAPI_Background(lua_State *pL)
{
	int x1=(int)lua_tonumber(pL,1);
	int y1=(int)lua_tonumber(pL,2);
	int x2=(int)lua_tonumber(pL,3);
	int y2=(int)lua_tonumber(pL,4);
	int Bright=(int)lua_tonumber(pL,5);
    JY_Background(x1,y1,x2,y2,Bright);
	return 0;
}

int HAPI_DrawRect(lua_State *pL)
{
	int x1=(int)lua_tonumber(pL,1);
	int y1=(int)lua_tonumber(pL,2);
	int x2=(int)lua_tonumber(pL,3);
	int y2=(int)lua_tonumber(pL,4);
	int color=(int)lua_tonumber(pL,5);

    JY_DrawRect(x1,y1,x2,y2,color);
	return 0;
}


int HAPI_ShowSlow(lua_State *pL)
{
	int delay=(int)lua_tonumber(pL,1);
	int flag=(int)lua_tonumber(pL,2);
    JY_ShowSlow(delay,flag);
	return 0;
}





int HAPI_LoadPicture(lua_State *pL)
{
	 int n = lua_gettop(pL);			//得到参数的长度
	const char *str=lua_tostring(pL,1);
 

	
	int x=(int)lua_tonumber(pL,2);
	int y=(int)lua_tonumber(pL,3);
 
	//蓝烟清：传入比例
	int percent = 0;
	if (n > 3)
	{
		percent = (int)lua_tonumber(pL,4);
	}

  
    JY_LoadPicture(str,x,y,percent);
 
	return 0;
}

 

int HAPI_GetKey(lua_State *pL)
{
	int keyPress;
    keyPress=JY_GetKey();    
	lua_pushnumber(pL,keyPress);
	return 1;
}

int HAPI_EnableKeyRepeat(lua_State *pL)
{
	int delay=(int)lua_tonumber(pL,1);
	int interval=(int)lua_tonumber(pL,2);
    //SDL_EnableKeyRepeat(delay,interval);
	return 0;
}



int HAPI_ShowSurface(lua_State *pL)
{
  int flag=(int)lua_tonumber(pL,1);
    JY_ShowSurface(flag);
	return 0;
}

 

int HAPI_GetTime(lua_State *pL)
{
    int t;
	t=JY_GetTime();
	lua_pushnumber(pL,t);
	return 1;
}


int HAPI_Delay(lua_State *pL)
{
    int x=(int)lua_tonumber(pL,1);
    JY_Delay(x);
	return 0;
}

int HAPI_Debug(lua_State *pL)
{
 
	const char *str=lua_tostring(pL,1);
   
	 JY_Debug(str);

	return 0;
}


int HAPI_CharSet(lua_State *pL)
{
    int length;
	const char *src=lua_tostring(pL,1);
	int flag=(int)lua_tonumber(pL,2);
    char *dest;

	length =strlen(src);

    dest=(char*)malloc(length+1);

    JY_CharSet(src,dest,flag);

    lua_pushstring(pL,dest);

	SafeFree(dest);

	return 1;
}



int HAPI_SetClip(lua_State *pL)
{
 
	if(lua_isnoneornil(pL,1)==0 ){ 
		int x1=(int)lua_tonumber(pL,1);
		int y1=(int)lua_tonumber(pL,2);
		int x2=(int)lua_tonumber(pL,3);
		int y2=(int)lua_tonumber(pL,4);
		JY_SetClip(x1,y1,x2,y2);
	}
	else{
        JY_SetClip(0,0,0,0);
    }

	return 0;
}

int HAPI_PlayMIDI(lua_State *pL)
{
 
	const char *filename=lua_tostring(pL,1);

    JY_PlayMIDI(filename);

	return 0;
}

int HAPI_PlayWAV(lua_State *pL)
{
 
	const char *filename=lua_tostring(pL,1);

    JY_PlayWAV(filename);

	return 0;
}


int HAPI_PlayMPEG(lua_State *pL)
{
 
	const char *filename=lua_tostring(pL,1);
	int key=(int)lua_tonumber(pL,2);
    JY_PlayMPEG(filename,key);

	return 0;
}




int HAPI_PicInit(lua_State *pL)
{
    char *filename;
    if(lua_isnoneornil(pL,1)==0 )
        filename=(char*)lua_tostring(pL,1);	    
    else
        filename="\0";
  	
    JY_PicInit(filename); 
 
	return 0;
}

int HAPI_PicLoadFile(lua_State *pL)
{
 int n = lua_gettop(pL);			//得到参数的长度
	const char *idx=lua_tostring(pL,1);
	const char *grp=lua_tostring(pL,2);
	int id=(int)lua_tonumber(pL,3); 
    
 //蓝烟清：传入比例
	int percent = 0;
	if (n > 3)
	{
		percent = (int)lua_tonumber(pL,4); 
	}
	

	JY_PicLoadFile(idx,grp,id, percent);
 
	return 0;
}
 
int HAPI_LoadPic(lua_State *pL)
{
 
	int fileid=(int)lua_tonumber(pL,1);
	int picid=(int)lua_tonumber(pL,2);
	int x=(int)lua_tonumber(pL,3);
	int y=(int)lua_tonumber(pL,4);
	int nooffset=0;
	int bright=0;

    if(lua_isnoneornil(pL,5)==0 )
        nooffset=(int)lua_tonumber(pL,5);

    if(lua_isnoneornil(pL,6)==0 )
        bright=(int)lua_tonumber(pL,6);
    
	JY_LoadPic(fileid,picid,x,y,nooffset,bright);

	return 0;
} 
 


int HAPI_GetPicXY(lua_State *pL)
{
	int fileid=(int)lua_tonumber(pL,1);
	int picid=(int)lua_tonumber(pL,2);

	int w,h, xoff, yoff;
	
	JY_GetPicXY(fileid,picid,&w,&h,&xoff,&yoff);
	lua_pushnumber(pL,w);
	lua_pushnumber(pL,h);
	lua_pushnumber(pL,xoff);
	lua_pushnumber(pL,yoff);
	return 4;
}






int HAPI_LoadMMap(lua_State *pL)
{
 
	const char *earth=lua_tostring(pL,1);
	const char *surface=lua_tostring(pL,2);
	const char *building=lua_tostring(pL,3);
	const char *buildx=lua_tostring(pL,4);
	const char *buildy=lua_tostring(pL,5);
	int xmax=(int)lua_tonumber(pL,6);
	int ymax=(int)lua_tonumber(pL,7);
	int x=(int)lua_tonumber(pL,8);
	int y=(int)lua_tonumber(pL,9);
  JY_LoadMMap(earth,surface,building,buildx,buildy,xmax,ymax,x,y);

	return 0;
}


int HAPI_DrawMMap(lua_State *pL)
{
	int x=(int)lua_tonumber(pL,1);
	int y=(int)lua_tonumber(pL,2);
	int mypic=(int)lua_tonumber(pL,3);

    JY_DrawMMap(x,y,mypic);
	return 0;
}

int HAPI_GetMMap(lua_State *pL)
{
	int x=(int)lua_tonumber(pL,1);
	int y=(int)lua_tonumber(pL,2);
	int flag=(int)lua_tonumber(pL,3);
    int v;
    v=JY_GetMMap(x,y,flag);
	lua_pushnumber(pL,v);
	return 1;
}

int HAPI_UnloadMMap(lua_State *pL)
{
    JY_UnloadMMap();
	return 0;
}


int HAPI_FullScreen(lua_State *pL)
{
    JY_FullScreen();
	return 0;
}


 
int HAPI_LoadSMap(lua_State *pL)
{
 

	const char *Sfilename=lua_tostring(pL,1);
	const char *tempfilename=lua_tostring(pL,2);
	int num=(int)lua_tonumber(pL,3);

	int x_max=(int)lua_tonumber(pL,4);
	int y_max=(int)lua_tonumber(pL,5);
	const char *Dfilename=lua_tostring(pL,6);
    int d_num1=(int)lua_tonumber(pL,7);
	int d_num2=(int)lua_tonumber(pL,8);

    JY_LoadSMap(Sfilename,tempfilename,num,x_max,y_max,Dfilename,d_num1,d_num2);

	return 0;
}

 

int HAPI_SaveSMap(lua_State *pL)
{
 
 
	const char *Sfilename=lua_tostring(pL,1);
	const char *Dfilename=lua_tostring(pL,2); 
    
	JY_SaveSMap(Sfilename,Dfilename);
	return 0;
}

 


int HAPI_GetS(lua_State *pL)
{

    int id=(Sint16)lua_tonumber(pL,1);
    int x=(int)lua_tonumber(pL,2);
    int y=(int)lua_tonumber(pL,3);
    int level=(int)lua_tonumber(pL,4);

	int v;
	v=JY_GetS(id,x,y,level);

	lua_pushnumber(pL,v);
	return 1;

}

int HAPI_SetS(lua_State *pL)
{

    int id=(int)lua_tonumber(pL,1);
    int x=(int)lua_tonumber(pL,2);
    int y=(int)lua_tonumber(pL,3);
    int level=(int)lua_tonumber(pL,4);
    int v=(int)lua_tonumber(pL,5);

	JY_SetS(id,x,y,level,v);

	return 0;

}
 
int HAPI_GetD(lua_State *pL)
{
	Sint16 v = -1;
    int Sceneid=(Sint16)lua_tonumber(pL,1);
    int id=(Sint16)lua_tonumber(pL,2);
    int i=(int)lua_tonumber(pL,3);
 

    v=JY_GetD(Sceneid,id,i);
    

	lua_pushnumber(pL,v);
	return 1;

}

int HAPI_SetD(lua_State *pL)
{

    int Sceneid=(Sint16)lua_tonumber(pL,1);
    int id=(Sint16)lua_tonumber(pL,2);
    int i=(int)lua_tonumber(pL,3);
    int v=(int)lua_tonumber(pL,4); 

 
    JY_SetD(Sceneid,id,i,v);

	return 0;

}

int HAPI_DrawSMap(lua_State *pL)
{
	int sceneid=(Sint16)lua_tonumber(pL,1);
	int x=(int)lua_tonumber(pL,2);
	int y=(int)lua_tonumber(pL,3);
	int xoff=(int)lua_tonumber(pL,4);
	int yoff=(int)lua_tonumber(pL,5);
	int mypic=(int)lua_tonumber(pL,6);
 
    JY_DrawSMap(sceneid,x,y,xoff,yoff,mypic);
 
	return 0;
}




int HAPI_LoadWarMap(lua_State *pL)
{
 
	const char *WarIDXfilename=lua_tostring(pL,1);
	const char *WarGRPfilename=lua_tostring(pL,2);
    int mapid=(int)lua_tonumber(pL,3);
	int num=(int)lua_tonumber(pL,4);
	int x_max=(int)lua_tonumber(pL,5);
    int y_max=(int)lua_tonumber(pL,6);

    JY_LoadWarMap(WarIDXfilename,WarGRPfilename,mapid,num,x_max,y_max);

	return 0;
}


 
int HAPI_GetWarMap(lua_State *pL)
{

    int x=(int)lua_tonumber(pL,1);
    int y=(int)lua_tonumber(pL,2);
    int level=(int)lua_tonumber(pL,3);
 

	Sint16 v;
	v=JY_GetWarMap(x,y,level);

	lua_pushnumber(pL,v);
	return 1;

}

int HAPI_SetWarMap(lua_State *pL)
{

    int x=(int)lua_tonumber(pL,1);
    int y=(int)lua_tonumber(pL,2);
    int level=(int)lua_tonumber(pL,3);
	int v=(int)lua_tonumber(pL,4);
	JY_SetWarMap(x,y,level,v);

	return 0;

}

int HAPI_CleanWarMap(lua_State *pL)
{

    int level=(int)lua_tonumber(pL,1);
	int v=(int)lua_tonumber(pL,2);
	JY_CleanWarMap(level,v);

	return 0;

}

int HAPI_DrawWarMap(lua_State *pL)
{
	int n = lua_gettop(pL);			//得到参数的长度
	int flag=(int)lua_tonumber(pL,1);
	int x=(int)lua_tonumber(pL,2);
	int y=(int)lua_tonumber(pL,3);
	int v1=(int)lua_tonumber(pL,4);
	int v2=(int)lua_tonumber(pL,5);
	int v3=(int)lua_tonumber(pL,6);
	int v4=(int)lua_tonumber(pL,7);

	int v5 = -1;
	int ex = -1;
	int ey = -1;

	if(n >= 8)
		v5=(int)lua_tonumber(pL,8);
	if(n >= 9)
		ex=(int)lua_tonumber(pL,9);
	if(n >= 10)
		ey=(int)lua_tonumber(pL,10);

    JY_DrawWarMap(flag,x,y,v1,v2,v3,v4,v5,ex,ey);
	return 0;
}

int HAPI_SaveSur(lua_State *pL){		//保存屏幕到临时表面
	int x=(int)lua_tonumber(pL,1);
	int y=(int)lua_tonumber(pL,2);
	int w=(int)lua_tonumber(pL,3);
	int h=(int)lua_tonumber(pL,4);
	int id = JY_SaveSur(x, y, w, h);
	lua_pushnumber(pL, id);
	return 1;
}
int HAPI_LoadSur(lua_State *pL){			//加载临时表面到屏幕
	int id=(int)lua_tonumber(pL,1);
	int x=(int)lua_tonumber(pL,2);
	int y=(int)lua_tonumber(pL,3);
	JY_LoadSur(id, x, y);
	return 0;
}
int HAPI_FreeSur(lua_State *pL){				//释放
	int id=(int)lua_tonumber(pL,1);
	JY_FreeSur(id);
	return 0;
}

int HAPI_ScreenWidth(lua_State *pL)				//屏幕宽度
{
	lua_pushnumber(pL, g_ScreenW);
	return 1;
}
int HAPI_ScreenHeight(lua_State *pL)			//屏幕高度
{
	lua_pushnumber(pL, g_ScreenH);
	return 1;
}

int HAPI_LoadPNGPath(lua_State *pL)				//按图片读取PNG
{
	int n = lua_gettop(pL);			//得到参数的长度
	const char *path=lua_tostring(pL,1);
    int fileid=(int)lua_tonumber(pL,2);
	int num=(int)lua_tonumber(pL,3);
	int percent = 0;
	const char* suffix = "png";

	if(n>3)
		percent=(int)lua_tonumber(pL,4);

	if(n>4)
		suffix=lua_tostring(pL,5);

    JY_LoadPNGPath(path, fileid, num, percent, suffix);

	return 0;
}
int HAPI_LoadPNG(lua_State *pL)				//按图片读取PNG
{
	int n = lua_gettop(pL);			//得到参数的长度
    int fileid=(int)lua_tonumber(pL,1);
	int picid=(int)lua_tonumber(pL,2);
	int x=(int)lua_tonumber(pL,3);
	int y=(int)lua_tonumber(pL,4);
	int flag = 0;
	int value = 0;
	int px = 0;
	int py = 0;
	int pw = -1;
	int ph = -1;

	if(n>4)
		flag=(int)lua_tonumber(pL,5);
	if(n>5)
		value=(int)lua_tonumber(pL,6);
	if(n>6)
	{
		px=(int)lua_tonumber(pL,7);
		py=(int)lua_tonumber(pL,8);
		pw=(int)lua_tonumber(pL,9);
		ph=(int)lua_tonumber(pL,10);


	}

    JY_LoadPNG(fileid, picid, x,y,flag,value, px, py, pw, ph);

	return 0;
}

int HAPI_GetPNGXY(lua_State *pL)
{
	int fileid=(int)lua_tonumber(pL,1);
	int picid=(int)lua_tonumber(pL,2);

	int w,h,xoff,yoff;
	
	JY_GetPNGXY(fileid,picid,&w,&h,&xoff,&yoff);
	lua_pushnumber(pL,w);
	lua_pushnumber(pL,h);
	lua_pushnumber(pL,xoff);
	lua_pushnumber(pL,yoff);

	return 4;
}


// byte数组lua函数
/*  lua 调用形式：(注意，位置都是从0开始
     handle=Byte_create(size);
	 Byte_release(h);
	 handle=Byte_loadfile(h,filename,start,length);
     Byte_savefile(h,filename,start,length);
     v=Byte_get16(h,start);
	 Byte_set16(h,start,v);
     v=Byte_getu16(h,start);
	 Byte_setu16(h,start,v);
     v=Byte_get32(h,start);
	 Byte_set32(h,start,v);
	 str=Byte_getstr(h,start,length);
	 Byte_setstr(h,start,length,str);
  */

int Byte_create(lua_State *pL)
{
	int x=(int)lua_tonumber(pL,1);
    char *p=(char *)lua_newuserdata(pL,x);                //创建userdata，不需要释放了。
	int i;

	if(p==NULL){
		fprintf(stderr,"Byte_create:cannot malloc memory\n");
		return 1;
	}
	for(i=0;i<x;i++)
		p[i]=0;

	return 1;
}


int Byte_loadfile(lua_State *pL)
{
	char *p=(char *)lua_touserdata(pL,1);
	const char *filename=lua_tostring(pL,2);
	int start=(int)lua_tonumber(pL,3);
	int length=(int)lua_tonumber(pL,4);

 
    
	FILE *fp;
    if((fp=fopen(filename,"rb"))==NULL){
        fprintf(stderr,"Byte_loadfile:file not open ---%s",filename);
		
		return 1;
	}
	fseek(fp,start,SEEK_SET);
    fread(p,1,length,fp);
	fclose(fp);
	return 0;
}

int Byte_savefile(lua_State *pL)
{
	char *p=(char *)lua_touserdata(pL,1);
	const char *filename=lua_tostring(pL,2);
	int start=(int)lua_tonumber(pL,3);
	int length=(int)lua_tonumber(pL,4);

	FILE *fp;
    if((fp=fopen(filename,"a+b"))==NULL){
        fprintf(stderr,"file not open ---%s",filename);
		return 1;
	}
	fseek(fp,start,SEEK_SET);
    fwrite(p,1,length,fp);
	fclose(fp);
	return 0;
}

int Byte_get16(lua_State *pL)
{
	char *p=(char *)lua_touserdata(pL,1);
	int start=(int)lua_tonumber(pL,2);

	short v=*(short*)(p+start);
	lua_pushnumber(pL,v);
	return 1;
}

int Byte_set16(lua_State *pL)
{
	char *p=(char *)lua_touserdata(pL,1);
	int start=(int)lua_tonumber(pL,2);
	short v=(short)lua_tonumber(pL,3);
    *(short*)(p+start)=v;
	return 0;
}

int Byte_getu16(lua_State *pL)
{
	char *p=(char *)lua_touserdata(pL,1);
	int start=(int)lua_tonumber(pL,2);

	unsigned short v=*(unsigned short*)(p+start);
	lua_pushnumber(pL,v);
	return 1;
}

int Byte_setu16(lua_State *pL)
{
	char *p=(char *)lua_touserdata(pL,1);
	int start=(int)lua_tonumber(pL,2);
	unsigned short v=(unsigned short)lua_tonumber(pL,3);
    *(unsigned short*)(p+start)=v;
	return 0;
 
}

int Byte_get32(lua_State *pL)
{
	char *p=(char *)lua_touserdata(pL,1);
	int start=(int)lua_tonumber(pL,2);

	int v=*(int*)(p+start);
	lua_pushnumber(pL,v);
	return 1;
}

int Byte_set32(lua_State *pL)
{
	char *p=(char *)lua_touserdata(pL,1);
	int start=(int)lua_tonumber(pL,2);
	int v=(int)lua_tonumber(pL,3);
    *(int*)(p+start)=v;
	return 0;
}

int Byte_getstr(lua_State *pL)
{
	char *p=(char *)lua_touserdata(pL,1);
	int start=(int)lua_tonumber(pL,2);
	int length=(int)lua_tonumber(pL,3);
	char *s=(char*)malloc(length+1);
	int i;
	for(i=0;i<length;i++)
		s[i]=p[start+i];

	s[length]='\0';
    lua_pushstring(pL,s);
	SafeFree(s);
	return 1;
}

int Byte_setstr(lua_State *pL)
{
	char *p=(char *)lua_touserdata(pL,1);
	int start=(int)lua_tonumber(pL,2);
	int length=(int)lua_tonumber(pL,3);
	const char *s=lua_tostring(pL,4);
	int i;
	int l=strlen(s);
	for(i=0;i<length;i++)
		p[start+i]=0;
	
	if(l>length) l=length;

	for(i=0;i<l;i++)
		p[start+i]=s[i];

 
    lua_pushstring(pL,s);
 
	return 1;
}

int Config_GetPath(lua_State *pL)
{
	lua_pushstring(pL,JY_CurrentPath);
	return 1;
}
