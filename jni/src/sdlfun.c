// SDL 相关函数
#include "jymain.h"
#include <math.h>
#include <android/log.h>
//static Mix_Music *currentMusic=NULL;         //播放音乐数据，由于同时只播放一个，用一个变量
static HSTREAM currentMusic = 0;

#define WAVNUM 5

//static Mix_Chunk *WavChunk[WAVNUM];        //播放音效数据，可以同时播放几个，因此用数组
static HSAMPLE WavChunk[WAVNUM];        //播放音效数据，可以同时播放几个，因此用数组

static BASS_MIDI_FONT midfonts;


static int currentWav=0;                  //当前播放的音效

extern SDL_Window* g_window;
extern SDL_Renderer* g_renderer;
extern SDL_Texture* g_screenTex;

#define RECTNUM  20
static SDL_Rect ClipRect[RECTNUM];        // 当前设置的剪裁矩形
static int currentRect=0;

static SDL_Texture* op_Surface[12];		//八个操作图
static SDL_Rect op_Rect[12];		//操作图所在的位置
static int op_alpha1 = 192;	//有操作时的透明度
static int op_alpha2 = 128;	//无操作时的透明度

extern int g_ScreenW ;
extern int g_ScreenH ;
extern char JY_CurrentPath[512];
extern int g_ScreenBpp ;
extern int g_KeyRepeatDelay;		//第一次键盘重复等待ms数
extern int g_KeyRePeatInterval;	//一秒钟重复次数

extern float g_Zoom;

extern int g_FullScreen;
extern int g_EnableSound;
extern int g_MusicVolume;
extern int g_SoundVolume;

extern int g_D1X;
extern int g_D1Y;
extern int g_D2X;
extern int g_D2Y;
extern int g_D3X;
extern int g_D3Y;
extern int g_D4X;
extern int g_D4Y;
extern int g_C1X;
extern int g_C1Y;
extern int g_C2X;
extern int g_C2Y;
extern int g_AX;
extern int g_AY; 
extern int g_BX; 
extern int g_BY; 

extern int g_F1X ;
extern int g_F1Y ;
extern int g_F2X ;
extern int g_F2Y ;
extern int g_F3X;
extern int g_F3Y ;
extern int g_F4X ;
extern int g_F4Y;
extern int g_XScale;
extern int g_YScale;


extern int g_MP3;		//是否播放MP3
extern char g_MidSF2[255];

extern int g_KeyScale;

#define SURFACE_NUM  20
static SDL_Texture* tmp_Surface[SURFACE_NUM];	//JY_SaveSur使用

#ifndef BOOL
#define BOOL unsigned char
#endif
#ifndef TRUE
#define TRUE (BOOL) 1
#endif
#ifndef FALSE
#define FALSE (BOOL) 0
#endif

//抓屏
/*++
  Purpose:

    Save the screenshot of current screen to a BMP file.

  Parameters:

    None.

  Return value:

    None.

--*/
void JY_SaveScreenshot()

{
	/*
   int      iNumBMP = 0;
   FILE    *fp;

   //
   // Find a usable BMP filename.
   //
   for (iNumBMP = 0; iNumBMP <= 9999; iNumBMP++)
   {
      fp = fopen(va("%sscrn%.4d.bmp", JY_PREFIX, iNumBMP), "rb");
      if (fp == NULL)
      {
         break;
      }
      fclose(fp);
   }

   if (iNumBMP > 9999)
   {
      return;
   }

   //
   // Save the screenshot.
   //
   SDL_SaveBMP(g_Surface, va("%sscrn%.4d.bmp", JY_PREFIX, iNumBMP));
   */
}
/*++
  Purpose:

    SDL sound volume adjust function.

  Parameters:

    [IN]  iDirectory - value, Increase (>0) or decrease (<=0) 3% volume.

  Return value:

    None.

--*/
void JY_AdjustVolume(int iDirectory)

{
   if (iDirectory > 0)
   {
      if (g_MusicVolume <= 100)
      {
		  g_MusicVolume += 100 * 0.1;
      }
      else
      {
		  g_MusicVolume = 100;
      }
   }
   else
   {
      if (g_MusicVolume > 0)
      {
		  g_MusicVolume -= 100 * 0.1;
      }
      else
      {
		  g_MusicVolume = 0;
      }
   }
   g_SoundVolume = g_MusicVolume;
   //Mix_VolumeMusic(g_MusicVolume);
   BASS_SetVolume((float)(g_MusicVolume / 100.0));
}

int getKeyPressByMouseEvent(const SDL_Event *lpEvent)

{
	
  	int keyPress=-1;
  	int i,index = -1;

   	
   	switch (lpEvent->type)
	   {
	   case SDL_MOUSEBUTTONDOWN:

	      for(i=0; i<4; i++)
		    {
		    	if(lpEvent->button.x >= op_Rect[i].x - op_Rect[i].w/2  && lpEvent->button.x <= op_Rect[i].x + op_Rect[i].w*3/2
		    		&& lpEvent->button.y  >= op_Rect[i].y - op_Rect[i].h/2 && lpEvent->button.y <= op_Rect[i].y + op_Rect[i].h*3/2)
		    		{
		    			index = i;
		    			break;
		    		}
		    }    
	      switch (index)
	      {
				case 0:
					keyPress =  SDLK_UP;
					break;
		  		case 1:
	        	keyPress = SDLK_RIGHT;
	        	break;
				case 2:
						keyPress =  SDLK_DOWN;
						break;
				case 3:
	    	 		keyPress = SDLK_LEFT;
	        	break;
	      }

	      break;
	    case SDL_MOUSEBUTTONUP:
	      for(i=4; i<12; i++)
		    {
		    	if(op_Surface[i] && lpEvent->button.x >= op_Rect[i].x - op_Rect[i].w/2  && lpEvent->button.x <= op_Rect[i].x + op_Rect[i].w*3/2
		    		&& lpEvent->button.y  >= op_Rect[i].y - op_Rect[i].h/2 && lpEvent->button.y <= op_Rect[i].y + op_Rect[i].h*3/2)
		    		{
		    			index = i;
		    			break;
		    		}
		    }  
	    	switch (index)
	      {
	    		case 4:  
	    	 		keyPress = SDLK_s;
	        	break;
	        case 5:  
	    	 		keyPress = SDLK_h;
	        	break;
	        case 6:  
	    	 		keyPress = SDLK_ESCAPE;
	        	break;
	        case 7:  
	    	 		keyPress = SDLK_RETURN;
	    	 		break;
	        case 8:
					keyPress = SDLK_q;
					break;
	        case 9:
					keyPress = SDLK_m;
					break;
	        case 10:
					keyPress = SDLK_x;
					break;
	        case 11:
					keyPress = SDLK_t;
	        	break;
	      }
	      
	      break;
	    	
   	}
   	
		return keyPress;
}


static int KeyFilter(const SDL_Event *event)

{
	static int Esc_KeyPress=0;
	static int Space_KeyPress=0;
	static int Return_KeyPress=0;

	int r=1;
	switch(event->type){   
    case SDL_KEYDOWN:  
		switch(event->key.keysym.sym){
	    case SDLK_ESCAPE:
			if(1==Esc_KeyPress){
				r=0;
			}
			else{
			    Esc_KeyPress=1;
			}
			break;
	    case SDLK_RETURN:
			if(1==Return_KeyPress){
				r=0;
			}
			else{
			    Return_KeyPress=1;
			}
			break;
	    case SDLK_SPACE:
			if(1==Space_KeyPress){
				r=0;
			}
			else{
			    Space_KeyPress=1;
			}
			break;
		default:
            break;
		}
		break;
    case SDL_KEYUP:
		switch(event->key.keysym.sym){
	    case SDLK_ESCAPE:
			Esc_KeyPress=0;
			break;
		case SDLK_SPACE:
			Space_KeyPress=0;
			break;
	    case SDLK_RETURN:
			Return_KeyPress=0;
			break;
		default:
            break;
		}
        break;
    default: 
        break;
    }

    return r;

}

// 初始化SDL
int InitSDL(void)
{
	int r;
		int i;
		//char tmpstr[255];
		int so = 22050;

		r=SDL_Init(SDL_INIT_VIDEO);
	    if( r < 0 ) {
	        JY_Error(
	                "Couldn't initialize SDL: %s\n", SDL_GetError());
	        exit(1);
	    }

	    //atexit(SDL_Quit);    可能有问题，屏蔽掉


	    InitFont();  //初始化

		r=SDL_InitSubSystem(SDL_INIT_AUDIO);
	    if(r<0)
		{
	        g_EnableSound=0;
			JY_Error("Init audio error!");
		}


		if(g_MP3 == 1)
		{
			so = 44100;
		}
		if (!BASS_Init(-1, so, 0, 0, NULL)) {
			JY_Error("Can't initialize device");
			g_EnableSound=0;
		}

		currentWav=0;

	    for(i=0;i<WAVNUM;i++)
	         WavChunk[i]=0;

	    SDL_SetEventFilter(NULL,KeyFilter);

		if(g_MP3 != 1)
		{
			midfonts.font=BASS_MIDI_FontInit(g_MidSF2,0);
			if(!midfonts.font)
			{
				JY_Error("BASS_MIDI_FontInit error ! %d", BASS_ErrorGetCode());
			}

			midfonts.preset=-1; // use all presets
			midfonts.bank=0; // use default bank(s)

			BASS_MIDI_StreamSetFonts(0,&midfonts,1); // set default soundfont
		}

	    return 0;
}

// 退出SDL
int ExitSDL(void)
{
	int i;

	ExitFont();

	StopMIDI();

	if(midfonts.font)
		BASS_MIDI_FontFree(midfonts.font);

	for(i=0;i<WAVNUM;i++){
		if(WavChunk[i]){
			BASS_SampleFree(WavChunk[i]);
			 WavChunk[i]=0;
		}
	}

	BASS_Free();

	/*
	for(i=0;i<WAVNUM;i++){
			if(WavChunk[i]){
	             Mix_FreeChunk(WavChunk[i]);
			     WavChunk[i]=NULL;
			}
		}

		Mix_CloseAudio();
		*/
	JY_LoadPicture("",0,0,0);    // 释放可能加载的图片表面

	SDL_DestroyTexture(g_screenTex);
	SDL_DestroyRenderer(g_renderer);
	SDL_DestroyWindow(g_window);

	SDL_Quit();
	return 0;
}

void SetRendererColor(int color)
{
	Uint8 r, g, b;
	r=(Uint8) ((color & 0xff0000) >>16);
	g=(Uint8) ((color & 0xff00)>>8);
	b=(Uint8) ((color & 0xff));



	SDL_SetRenderDrawColor(g_renderer, r, g, b, 255);
}


// 初始化游戏数据
int InitGame(void)
{
	char str[255];
	int i,x;
	float zoom = (float)g_ScreenW/(float)g_KeyScale;

	//虚拟按键图
	char buttons[][5] = { "D1","D2","D3","D4","C1","C2","A","B","F1","F2","F3","F4" };


	if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0) {
		printf("Couldn't initialize SDL video subsystem: %s\n", SDL_GetError());
		__android_log_print(ANDROID_LOG_ERROR, "jy", "Couldn't initialize SDL video subsystem: %s\n", SDL_GetError());
		exit(1);
	  }

	JY_Debug("InitGame start(%d,%d)",g_ScreenW,g_ScreenH);

	g_window = SDL_CreateWindow("JY_LDCR",
						SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
						g_ScreenW, g_ScreenH,
						SDL_WINDOW_FULLSCREEN);
	if(g_window==NULL)
		JY_Error("Cannot set video mode");
	g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_ACCELERATED);

	g_screenTex = SDL_CreateTexture(g_renderer,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_TARGET,g_ScreenW,g_ScreenH);
	SDL_SetTextureBlendMode(g_screenTex, SDL_BLENDMODE_BLEND);
	SDL_SetRenderTarget(g_renderer,g_screenTex);
	SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, 255);
	SDL_RenderClear(g_renderer);



	Init_Cache();

	JY_PicInit("");        // 初始化贴图cache

	//虚拟按键
	for(i=0; i<12; i++){
		int ow, oh;

		sprintf(str,"%sPIC/%s.png",JY_CurrentPath, buttons[i]);
		op_Surface[i] = IMG_LoadTexture(g_renderer,str);

		if(!op_Surface[i]){
			JY_Error("Init Error not found or load %s failed!", str);
			continue;
		}
		SDL_SetTextureBlendMode(op_Surface[i], SDL_BLENDMODE_BLEND);
		SDL_SetTextureAlphaMod(op_Surface[i],(Uint8)op_alpha2);

		SDL_QueryTexture(op_Surface[i], 0, 0, &ow, &oh);
		if(zoom != 1)
		{
			ow *= zoom;
			oh *= zoom;
		}
		op_Rect[i].w = ow;
		op_Rect[i].h = oh;
	}


	if(g_D1X == -1)
		g_D1X = g_ScreenW/10 + op_Rect[0].w*3/2;
	if(g_D1Y == -1)
		g_D1Y = g_ScreenH - op_Rect[0].h*5;

	op_Rect[0].x = g_D1X;
	op_Rect[0].y = g_D1Y;


	if(g_D2X == -1)
		g_D2X = g_ScreenW/10 + op_Rect[1].w*3;
	if(g_D2Y == -1)
		g_D2Y = g_ScreenH - op_Rect[1].h*7/2;

	op_Rect[1].x = g_D2X;
	op_Rect[1].y = g_D2Y;


	if(g_D3X == -1)
		g_D3X = g_ScreenW/10 + op_Rect[2].w*3/2;
	if(g_D3Y == -1)
		g_D3Y = g_ScreenH - op_Rect[2].h*2;
	op_Rect[2].x = g_D3X;
	op_Rect[2].y = g_D3Y;


	if(g_D4X == -1)
		g_D4X = g_ScreenW/10;
	if(g_D4Y == -1)
		g_D4Y = g_ScreenH - op_Rect[3].h*7/2;
	op_Rect[3].x = g_D4X;
	op_Rect[3].y = g_D4Y;



	x = g_ScreenW - op_Rect[4].w*2;
	if(g_C1X == -1)
		g_C1X = x;
	if(g_C1Y == -1)
		g_C1Y = g_ScreenH/2 - op_Rect[4].h*3;
	op_Rect[4].x = g_C1X;
	op_Rect[4].y = g_C1Y;


	if(g_C2X == -1)
		g_C2X = x;
	if(g_C2Y == -1)
		g_C2Y = g_ScreenH/2 - op_Rect[5].h;

	op_Rect[5].x = g_C2X;
	op_Rect[5].y = g_C2Y;


	x = g_ScreenW/2 + op_Rect[6].w*2;
	if(g_AX == -1)
		g_AX = x;
	if(g_AY == -1)
		g_AY =  g_ScreenH - op_Rect[6].h*2;
	op_Rect[6].x = g_AX;
	op_Rect[6].y = g_AY;



	if(g_BX == -1)
		g_BX = x + op_Rect[7].w*3;
	if(g_BY == -1)
		g_BY =  g_ScreenH - op_Rect[7].h*2;
	op_Rect[7].x = g_BX;
	op_Rect[7].y = g_BY;

	x = 0;
	if(op_Surface[8])
	{
		if(g_F1X == -1)
			g_F1X = x;
		if(g_F1Y == -1)
			g_F1Y =  20;
		op_Rect[8].x = g_F1X;
		op_Rect[8].y = g_F1Y;
		x +=op_Rect[8].w;
	}
	if(op_Surface[9])
	{
		if(g_F2X == -1)
			g_F2X = x + op_Rect[9].w;
		if(g_F2Y == -1)
			g_F2Y =  20;
		op_Rect[9].x = g_F2X;
		op_Rect[9].y = g_F2Y;

		x +=op_Rect[9].w*2;
	}
	if(op_Surface[10])
	{
		if(g_F3X == -1)
			g_F3X = x + op_Rect[10].w;
		if(g_F3Y == -1)
			g_F3Y =  20;
		op_Rect[10].x = g_F3X;
		op_Rect[10].y = g_F3Y;

		x +=op_Rect[9].w*2;
	}
	if(op_Surface[11])
	{
		if(g_F4X == -1)
			g_F4X = x + op_Rect[11].w;
		if(g_F4Y == -1)
			g_F4Y =  20;
		op_Rect[11].x = g_F4X;
		op_Rect[11].y = g_F4Y;

	}

	return 0;
}


// 释放游戏资源
int ExitGame(void)
{

    JY_PicInit("");
    
    JY_LoadPicture("",0,0,0);

    JY_UnloadMMap();     //释放主地图内存

	JY_UnloadSMap();     //释放场景地图内存

    JY_UnloadWarMap();   //释放战斗地图内存

    return 0;
}


//加载图形文件，其他格式也可以加载
//x,y =-1 则加载到屏幕中心
//    如果未加载，则加载，然后blit，如果加载，直接blit
//  str 文件名，如果为空，则释放表面        
int JY_LoadPicture(const char* str,int x,int y, int percent)
{
	static char filename[255]="\0";
	static SDL_Texture *pic=NULL;

	SDL_Rect r;

	if(strlen(str)==0){        // 为空则释放表面
		if(pic){
			SDL_DestroyTexture(pic);
			pic=NULL;
		}
		return 0;
	}

	if(strcmp(str,filename)!=0){ // 与以前文件名不同，则释放原来表面，加载新表面
		if(pic){
			SDL_DestroyTexture(pic);
			pic=NULL;
		}
		pic = IMG_LoadTexture(g_renderer,str);
		if(pic){
			strcpy(filename,str);
		}
	}

	if(pic){
		int w, h;
		SDL_QueryTexture(pic, NULL, NULL, &w, &h);
		if( (x==-1) && (y==-1) ){
			x=(g_ScreenW-w)/2;
			y=(g_ScreenH-h)/2;
		}
		r.x=x;
		r.y=y;
		r.w = w;
		r.h = h;
		if(percent != 0 &&  percent != 100){
			float zoom = (float)(percent/100.0);
			r.w = (int)(r.w*zoom);
			r.h = (int)(r.h*zoom);
		}

		SDL_RenderCopy(g_renderer, pic, NULL, &r);


	}
	else{
		JY_Error("JY_LoadPicture: Load picture file %s failed! %s",str, SDL_GetError());
	}

	return 0;
}



//显示表面
int JY_ShowSurface(int flag)
{
	if(flag == 0)
	{
		int i;
		for(i=0; i<12; i++)
		{
			if(op_Surface[i])
			{
				SDL_RenderCopy(g_renderer, op_Surface[i], NULL,  &op_Rect[i]);
			}
		}
		//JY_ShowSlow(30,0);
		SDL_SetTextureColorMod(g_screenTex,255, 255, 255);
	}
	else
	{


	}

	SDL_SetRenderTarget(g_renderer,NULL);
	SDL_RenderCopy(g_renderer, g_screenTex, NULL, NULL);
	SDL_RenderPresent(g_renderer);
	SDL_SetRenderTarget(g_renderer,g_screenTex);
	return 0;
}

//延时x毫秒
int JY_Delay(int x)
{
    SDL_Delay(x);
    return 0;
}


// 缓慢显示图形 
// delaytime 每次渐变延时毫秒数
// Flag=0 从暗到亮，1，从亮到暗
int JY_ShowSlow(int delaytime,int Flag)
{
	int i;
	int step;
	int t1,t2;
	int alpha;

	for(i=0;i<=32;i++)
	{
		if(Flag==0)
			step=i;
		else
			step=32-i;

		t1=(int)JY_GetTime();
		alpha=step<<3;
		if(alpha>255)
			alpha=255;
		SDL_SetTextureColorMod(g_screenTex,alpha, alpha, alpha);
		JY_ShowSurface(1);

		t2=(int)JY_GetTime();

		if(delaytime > t2-t1)
			JY_Delay(delaytime-(t2-t1));

	}


	return 0;
}

//得到当前时间，单位毫秒
int JY_GetTime()
{
    return SDL_GetTicks();
}

//播放音乐
//播放音乐
int JY_PlayMIDI(const char *filename)
{
	static char currentfile[255]="\0";
	

    if(g_EnableSound==0)
	{
		JY_Error("disable sound!");
		return 1;
	}

	if(strlen(filename)==0){  //文件名为空，停止播放
        StopMIDI();
        strcpy(currentfile,filename);
		return 0;
	}
	
	if(strcmp(currentfile,filename)==0) //与当前播放文件相同，直接返回
		return 0;

    StopMIDI();
	

    //BASS
	if(g_MP3 == 0)
		currentMusic = BASS_MIDI_StreamCreateFile(0, filename, 0, 0, 0, 0);
	else
		currentMusic = BASS_StreamCreateFile(0, filename, 0, 0, 0);
	

	if(!currentMusic){
		JY_Error("Open music file %s failed! %d",filename,BASS_ErrorGetCode());
		return 1;
	}
	
	if(g_MP3 == 0)
		BASS_MIDI_StreamSetFonts(currentMusic,&midfonts,1); // set for current stream too
	BASS_ChannelSetAttribute(currentMusic, BASS_ATTRIB_VOL, (float)(g_MusicVolume / 100.0));
	BASS_ChannelFlags(currentMusic, BASS_SAMPLE_LOOP, BASS_SAMPLE_LOOP);


	BASS_ChannelPlay(currentMusic, FALSE);

    //SDL2_mixer
    /*
    currentMusic=Mix_LoadMUS(filename);
    if(currentMusic==NULL){
		JY_Error("Open music file %s failed!",filename);
		return 1;
	}

	Mix_VolumeMusic(g_MusicVolume);

	Mix_PlayMusic(currentMusic, -1);
*/
    strcpy(currentfile,filename);


	return 0;
}

//停止音效
int StopMIDI()
{

    if(currentMusic){
		BASS_ChannelStop(currentMusic);
		BASS_StreamFree(currentMusic);
		currentMusic=0;
		
	}

	/*
	if(currentMusic!=NULL){
		Mix_HaltMusic();
		Mix_FreeMusic(currentMusic);
		currentMusic=NULL;
	}*/
    return 0;
}

int PausedMIDI()
{
	if(currentMusic){
		BASS_ChannelStop(currentMusic);

	}
	/*
	if(currentMusic){
			Mix_HaltMusic();
			Mix_FreeMusic(currentMusic);
			//BASS_ChannelStop(currentMusic);

			currentMusic=NULL;
		}
	*/
    return 0;
}
int ResumeMIDI()
{

	if(currentMusic)
	{
		BASS_ChannelPlay(currentMusic, FALSE);
	}
}


//播放音效
int JY_PlayWAV(const char *filename)
{

	HCHANNEL ch;
  if(g_EnableSound==0)
		return 1;    

	if(WavChunk[currentWav]){          //释放当前音效
        //Mix_FreeChunk(WavChunk[currentWav]);
        BASS_SampleStop(WavChunk[currentWav]);
        BASS_SampleFree(WavChunk[currentWav]);
        WavChunk[currentWav]=0; 
	}

	
	WavChunk[currentWav] = BASS_SampleLoad(0, filename, 0, 0, 1, 0);

	if(WavChunk[currentWav]){
		ch = BASS_SampleGetChannel(WavChunk[currentWav], 0);
  		BASS_ChannelSetAttribute(ch, BASS_ATTRIB_VOL, (float)(g_SoundVolume / 100.0));
  		BASS_ChannelFlags(ch, 0, BASS_SAMPLE_LOOP);		//不循环
  		BASS_ChannelPlay(ch, 0);
		currentWav++;
		if(currentWav>=WAVNUM)
			currentWav=0;
	}
	else{
		JY_Error("Open wav file %s failed!",filename);
	}

/*
	if(g_EnableSound==0)
			return 1;

	if(WavChunk[currentWav]){          //释放当前音效
		Mix_FreeChunk(WavChunk[currentWav]);
		WavChunk[currentWav]=NULL;
	}

	WavChunk[currentWav]= Mix_LoadWAV(filename);  //加载到当前音效

	if(WavChunk[currentWav]){
		Mix_VolumeChunk(WavChunk[currentWav],g_SoundVolume);
		Mix_PlayChannel(-1, WavChunk[currentWav], 0);  //播放音效
		currentWav++;
		if(currentWav>=WAVNUM)
			currentWav=0;
	}
	else{
		JY_Error("Open wav file %s failed!",filename);
	}
*/
	return 0;
	
}

// 得到前面按下的字符
int JY_GetKey()
{
  SDL_Event event;
  static g_keyPress = -1;
	int keyPress=-1;
	static lastPressButtonTime1=-1, lastPressButtonTime2=-1;
	static pressFlag = -1;

  while(SDL_PollEvent(&event)){   
		switch(event.type){
	   case SDL_KEYDOWN:
	   		switch (event.key.keysym.sym)
	   		{
		   		case SDLK_HASH: //# for mobile device
		   		case SDLK_p:
		   			JY_SaveScreenshot();
		   			break;
		   		case 1073741902:
		   			JY_AdjustVolume(0);
		   		  break;
		   		case 1073741899:
		   			JY_AdjustVolume(1);
		   			break;
		   		case SDLK_ESCAPE:
		   		case SDLK_RETURN:
		   		case SDLK_SPACE:
		   			g_keyPress = -1;
		   			keyPress=event.key.keysym.sym;
		   			return keyPress;
		   		default:
		   			g_keyPress=event.key.keysym.sym;
		   			break;
	   		}
	   		break;
	   	case SDL_KEYUP:
	   		g_keyPress = -1;
	   		break;
	   	case SDL_MOUSEMOTION:
	   		break;
	    case SDL_MOUSEBUTTONDOWN:
	    	pressFlag = 0;
	    case SDL_MOUSEBUTTONUP:
	    	g_keyPress = getKeyPressByMouseEvent(&event);
	    	if(event.type == SDL_MOUSEBUTTONUP && g_keyPress >= 0)
			{
	    		keyPress = g_keyPress;
				g_keyPress = -1;

			}
	    	if(event.type == SDL_MOUSEBUTTONUP)
	    	{
	    	pressFlag = -1;
	    	return keyPress;
	    	}

	    	break;
    	case SDL_QUIT:
    		ExitGame(); 
    		ExitSDL();
    		exit(0);
    		break;
    	default:
          break;
     }
	}
  if (g_keyPress >= 0)
  {
  	keyPress = g_keyPress;
  }

  if(pressFlag == 0)
  {
	  pressFlag = 1;
	  lastPressButtonTime1 = SDL_GetTicks();
	  return keyPress;
  }
  else if(pressFlag == 1)
  {
	  if(SDL_GetTicks() - lastPressButtonTime1 > g_KeyRepeatDelay*2)
	  {
		  pressFlag = 2;
		  return keyPress;
	  }
	  else
	  {
		  return -1;
	  }
  }
  else if(pressFlag == 2)
  {
	  if(SDL_GetTicks() - lastPressButtonTime1 > g_KeyRePeatInterval)
	  {
		  pressFlag = 2;
		  lastPressButtonTime1 = SDL_GetTicks();
		  return keyPress;
	  }
	  else
	  {
		  return -1;
	  }
  }
  else
  {
	  return keyPress;
  }

}


//设置裁剪
int JY_SetClip(int x1,int y1,int x2,int y2)
{

	//安卓里没有效果，就不用了吧
	/*
	if (x1==0 && y1==0 && x2==0 && y2==0){
		SDL_RenderSetClipRect(g_renderer,NULL);
		currentRect=0;
	}
	else{
		SDL_Rect rect;
		rect.x=(Sint16)x1;
		rect.y=(Sint16)y1;
		rect.w=(Uint16)(x2-x1);
		rect.h=(Uint16)(y2-y1);

		ClipRect[currentRect]=rect;


		SDL_RenderSetClipRect(g_renderer,&ClipRect[currentRect]);
		currentRect=currentRect+1;
		if(currentRect>=RECTNUM){
			currentRect=0;
		}
	}
*/
	return 0;
}


// 绘制矩形框
// (x1,y1)--(x2,y2) 框的左上角和右下角坐标
// color 颜色
int JY_DrawRect(int x1,int y1,int x2,int y2,int color)
{
	SDL_Rect rect1;
	int xmin,xmax,ymin,ymax;
	Uint8 r, g, b;

	if(x1<x2){
		xmin=x1;
		xmax=x2;
	}
	else{
		xmin=x2;
		xmax=x1;
	}

	if(y1<y2){
		ymin=y1;
		ymax=y2;
	}
	else{
		ymin=y2;
		ymax=y1;
	}
	r=(Uint8) ((color & 0xff0000) >>16);
	g=(Uint8) ((color & 0xff00)>>8);
	b=(Uint8) ((color & 0xff));

	rect1.x=(Sint16)xmin;
	rect1.y=(Sint16)ymin;
	rect1.w=(Uint16)(xmax-xmin+1);
	rect1.h=(Uint16)(ymax-ymin+1);
 
	SDL_SetRenderDrawColor(g_renderer, r, g, b, 255);
	SDL_RenderDrawRect(g_renderer,&rect1);
 
	return 0;
}


//绘水平线
void HLine32(int x1,int x2,int y,int color, unsigned char *vbuffer, int lpitch)
{
 
	int temp;
	int max_x = 0,max_y = 0,min_x = 0,min_y = 0;


	if (y > max_y || y < min_y)
		return;

	if (x1>x2){
	   temp = x1;
	   x1   = x2;
	   x2   = temp;
	}

	if (x1 > max_x || x2 < min_x)
		return;

	x1 = ((x1 < min_x) ? min_x : x1);
	x2 = ((x2 > max_x) ? max_x : x2);

	SetRendererColor(color);
	SDL_RenderDrawLine(g_renderer, x1, y, x2, y);
}  

//绘垂直线
void VLine32(int y1,int y2,int x,int color, unsigned char *vbuffer, int lpitch)
{
 
	int temp;
	int max_x = 0,max_y = 0,min_x = 0,min_y = 0;


	if (x > max_x || x < min_x)
		return;

	if (y1>y2){
	   temp = y1;
	   y1   = y2;
	   y2   = temp;
	}

	if (y1 > max_y || y2 < min_y)
		return;

	y1 = ((y1 < min_y) ? min_y : y1);
	y2 = ((y2 > max_y) ? max_y : y2);

	SetRendererColor(color);
	SDL_RenderDrawLine(g_renderer, x, y1, x, y2);
}  



// 图形填充
// 如果x1,y1,x2,y2均为0，则填充整个表面
// color, 填充色，用RGB表示，从高到低字节为0RGB
int JY_FillColor(int x1,int y1,int x2,int y2,int color)
{
	SDL_Rect rect;

	if(x1 == 0 && x2 == 0 && y1 == 0 && y2 == 0)
	{
		rect.x = 0;
		rect.y = 0;
		rect.w = g_ScreenW;
		rect.h = g_ScreenH;
	}
	else
	{
		rect.x = x1;
		rect.y = y1;
		rect.w = (x2-x1);
		rect.h = (y2-y1);
	}

	SetRendererColor(color);
	SDL_RenderFillRect(g_renderer, &rect);

	return 0;

}


// 把表面blt到背景或者前景表面
// x,y 要加载到表面的左上角坐标
// 把表面blt到背景或者前景表面
// x,y 要加载到表面的左上角坐标
int BlitSurface(SDL_Texture* lps, int x, int y ,int w, int h, int flag,int value, int pcolor)
{

	SDL_Rect rect;

			if(value>255)
				value=255;

			rect.x=(Sint16)x;
			rect.y=(Sint16)y;

			rect.w = w;
			rect.h = h;

			if(!lps)
			{
				JY_Error("BlitSurface: lps is null!");
				return 1;
			}

			if((flag & 0x2)==0){        // 没有alpla
				SDL_RenderCopy(g_renderer, lps, NULL, &rect);
			}
		    else{  // 有alpha

				if( (flag &0x4) || (flag &0x8) || (flag &0x10)){   // 黑白
					Uint8 r, g, b, a;
					if(flag &0x4)
					{
						r = 0;
						g = 0;
						b = 0;
						a = 255;
					}
					else if(flag &0x8)
					{
						r = 0;
						g = 215;
						b = 0;
						a = 255;
					}
					else
					{
						r=(Uint8) ((pcolor & 0xff0000) >>16);
						g=(Uint8) ((pcolor & 0xff00)>>8);
						b=(Uint8) ((pcolor & 0xff));
						a = 255;

					}

					SDL_SetTextureBlendMode(lps, SDL_BLENDMODE_BLEND);
					SDL_SetTextureColorMod(lps,r,g,b);
					SDL_SetTextureAlphaMod(lps,(Uint8)a);
					SDL_RenderCopy(g_renderer, lps, NULL, &rect);

					SDL_SetTextureColorMod(lps,255,255,255);

				 }
		        else{

					SDL_SetTextureAlphaMod(lps,(Uint8)value);
					SDL_RenderCopy(g_renderer, lps,NULL,  &rect);
		        }
			}

			return 0;
}


// 背景变暗
// 把源表面(x1,y1,x2,y2)矩形内的所有点亮度降低
// bright 亮度等级 0-256 
int JY_Background(int x1,int y1,int x2,int y2,int Bright)
{
	SDL_Texture* tex = NULL;
	SDL_Rect r1,r2;

	if(x2<=x1 || y2<=y1) 
		return 0;

	Bright=256-Bright;
	if(Bright>255)
		Bright=255;

	r1.x=(Sint16)x1;
	r1.y=(Sint16)y1;
	r1.w=(Uint16)(x2-x1);
	r1.h=(Uint16)(y2-y1);

		r2=r1;

	SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(g_renderer,0,0,0,Bright);
	SDL_RenderFillRect(g_renderer, &r2);

	SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_NONE);


	return 1;
}


//播放mpeg
// esckey 停止播放的按键
int JY_PlayMPEG(const char* filename,int esckey)
{

    return 0;
} 


// 全屏切换
int JY_FullScreen()
{    

  
	return 0;
}



//保存屏幕到临时表面
int JY_SaveSur(int x,int y,int w,int h)
{ 
	int id=-1;
	int i;
	SDL_Rect r1;
	for(i=0;i<SURFACE_NUM;i++)
	{
		if(tmp_Surface[i]==NULL)
		{
			id=i;
			break;
		}
	}

	if(id==-1)
		return -1;

	x = 0;
	y = 0;
	w = g_ScreenW;
	h = g_ScreenH;
	
	r1.x=(Sint16)x;
	r1.y=(Sint16)y;
	r1.w=(Uint16)w;
	r1.h=(Uint16)h;

	tmp_Surface[id] = SDL_CreateTexture(g_renderer,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_TARGET,r1.w,r1.h);

	SDL_SetRenderTarget(g_renderer, tmp_Surface[id]);
	SDL_RenderClear(g_renderer);
	SDL_RenderCopy(g_renderer, g_screenTex, &r1, NULL);

	SDL_SetRenderTarget(g_renderer, g_screenTex);

	return id;
}

//加载临时表面到屏幕
int JY_LoadSur(int id, int x, int y)
{
	SDL_Rect r1;
	int w, h;
	if (id<0||id>SURFACE_NUM-1||tmp_Surface[id]==NULL){
		return 1;
	}
	SDL_QueryTexture(tmp_Surface[id], 0, 0, &w, &h);
	r1.x=(Sint16)0;
	r1.y=(Sint16)0;
	r1.w = (Uint16)w;
	r1.h = (Uint16)h;

	SDL_RenderCopy(g_renderer, tmp_Surface[id], NULL, &r1);
	return 0;
}

//释放
int JY_FreeSur(int id)
{
	if (id<0||id>SURFACE_NUM-1||tmp_Surface[id]==NULL){
		return 1;
	}

	if(tmp_Surface[id]!=NULL){
		SDL_DestroyTexture(tmp_Surface[id]);
		tmp_Surface[id]=NULL;
	}
	return 0;
}

static int gfxPrimitivesCompareInt(const void *a, const void *b)
{
	return (*(const int *) a) - (*(const int *) b);
}

static int hline(Sint16 x1, Sint16 x2, Sint16 y)
{
	return SDL_RenderDrawLine(g_renderer, x1, y, x2, y);
}

SDL_Texture* createPolygonTexture(Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
	int result;
	int i;
	int y, xa, xb;
	int miny, maxy;
	int x1, y1;
	int x2, y2;
	int ind1, ind2;
	int ints;
	int *gfxPrimitivesPolyInts = NULL;
	int *gfxPrimitivesPolyIntsNew = NULL;
	int gfxPrimitivesPolyAllocated = 0;

	const Sint16 vx[] = {0, g_XScale, g_XScale*2, g_XScale};
	const Sint16 vy[] = {g_YScale, g_YScale*2, g_YScale, 0};
	int n = 4;

	SDL_Texture* tex = SDL_CreateTexture(g_renderer,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_TARGET ,g_XScale*2,g_YScale*2);

	/*
	* Vertex array NULL check
	*/
	if (vx == NULL) {
		return tex;
	}
	if (vy == NULL) {
		return tex;
	}

	/*
	* Sanity check number of edges
	*/
	if (n < 3) {
		return tex;
	}


	/*
	* Allocate temp array, only grow array
	*/
	if (!gfxPrimitivesPolyAllocated) {
		gfxPrimitivesPolyInts = (int *) malloc(sizeof(int) * n);
		gfxPrimitivesPolyAllocated = n;
	} else {
		if (gfxPrimitivesPolyAllocated < n) {
			gfxPrimitivesPolyIntsNew = (int *) realloc(gfxPrimitivesPolyInts, sizeof(int) * n);
			if (!gfxPrimitivesPolyIntsNew) {
				if (!gfxPrimitivesPolyInts) {
					free(gfxPrimitivesPolyInts);
					gfxPrimitivesPolyInts = NULL;
				}
				gfxPrimitivesPolyAllocated = 0;
			} else {
				gfxPrimitivesPolyInts = gfxPrimitivesPolyIntsNew;
				gfxPrimitivesPolyAllocated = n;
			}
		}
	}

	/*
	* Check temp array
	*/
	if (gfxPrimitivesPolyInts==NULL) {
		gfxPrimitivesPolyAllocated = 0;
	}


	/*
	* Check temp array again
	*/
	if (gfxPrimitivesPolyInts==NULL) {
		return tex;
	}

	/*
	* Determine Y maxima
	*/
	miny = vy[0];
	maxy = vy[0];
	for (i = 1; (i < n); i++) {
		if (vy[i] < miny) {
			miny = vy[i];
		} else if (vy[i] > maxy) {
			maxy = vy[i];
		}
	}

	/*
	* Draw, scanning y
	*/
	result = 0;
	SDL_SetTextureBlendMode(tex,SDL_BLENDMODE_BLEND);
	SDL_SetRenderTarget(g_renderer, tex);
	SDL_SetRenderDrawColor(g_renderer, 255, 255, 255, 0);
	SDL_RenderClear(g_renderer);
	for (y = miny; (y <= maxy); y++) {
		ints = 0;
		for (i = 0; (i < n); i++) {
			if (!i) {
				ind1 = n - 1;
				ind2 = 0;
			} else {
				ind1 = i - 1;
				ind2 = i;
			}
			y1 = vy[ind1];
			y2 = vy[ind2];
			if (y1 < y2) {
				x1 = vx[ind1];
				x2 = vx[ind2];
			} else if (y1 > y2) {
				y2 = vy[ind1];
				y1 = vy[ind2];
				x2 = vx[ind1];
				x1 = vx[ind2];
			} else {
				continue;
			}
			if ( ((y >= y1) && (y < y2)) || ((y == maxy) && (y > y1) && (y <= y2)) ) {
				gfxPrimitivesPolyInts[ints++] = ((65536 * (y - y1)) / (y2 - y1)) * (x2 - x1) + (65536 * x1);
			}
		}

		qsort(gfxPrimitivesPolyInts, ints, sizeof(int), gfxPrimitivesCompareInt);

		/*
		* Set color
		*/
		result = 0;
	    result |= SDL_SetRenderDrawBlendMode(g_renderer, (a == 255) ? SDL_BLENDMODE_NONE : SDL_BLENDMODE_BLEND);
		result |= SDL_SetRenderDrawColor(g_renderer, r, g, b, a);

		for (i = 0; (i < ints); i += 2) {
			xa = gfxPrimitivesPolyInts[i] + 1;
			xa = (xa >> 16) + ((xa & 32768) >> 15);
			xb = gfxPrimitivesPolyInts[i+1] - 1;
			xb = (xb >> 16) + ((xb & 32768) >> 15);
			result |= hline(xa, xb, y);
		}

		SDL_SetRenderDrawColor(g_renderer, 160, 215, 160, 215);
		SDL_RenderDrawLine(g_renderer, vx[0], vy[0], vx[1], vy[1]);
		SDL_RenderDrawLine(g_renderer, vx[1], vy[1], vx[2], vy[2]);
		SDL_RenderDrawLine(g_renderer, vx[2], vy[2], vx[3], vy[3]);
		SDL_RenderDrawLine(g_renderer, vx[3], vy[3], vx[0], vy[0]);
	}

	SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_NONE);
	SDL_SetRenderTarget(g_renderer, g_screenTex);

	return tex;
}
