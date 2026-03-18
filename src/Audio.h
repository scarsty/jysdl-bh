#pragma once
#include <string>
#include <unordered_map>
#include <vector>

//#define BASSMODE 1 //使用BASS播放音乐和midi,注释后使用SDL3MIXER播放音乐，midi需要编译fluidsynth
//#define TSFMODE 1 //SDL3MIXER下使用tinysoundfont替代fluidsynth播放midi，无需额外编译fluidsynth

#define WAVNUM 5

#if defined BASSMODE 
#include "bass.h"
#include "bassmidi.h"
    #ifndef BOOL
	#define BOOL unsigned char
	#endif
	#ifndef TRUE
	#define TRUE (BOOL) 1
	#endif
	#ifndef FALSE
	#define FALSE (BOOL) 0
	#endif
    using MUSIC = HSTREAM;
    using WAV = HSAMPLE;
    using MIDI_FONT = BASS_MIDI_FONT;
#else
    #include "SDL3_mixer/SDL_mixer.h"
    #ifdef TSFMODE
    #include <SDL3/SDL.h> 
    #endif
    using MUSIC = MIX_Audio*;
    using WAV = MIX_Audio*;
    using MIDI_FONT = void*;
    
    #ifdef TSFMODE
    struct tsf;
    struct tml_message;
    #endif
#endif

class Audio
{
public:
    static Audio* getInstance()
    {
        static Audio a;
        return &a;
    }

    void init();
    void quit();
    int PlayMIDI(const char* filename);
    int StopMIDI();
    int PausedMIDI();
    int ResumeMIDI();
    int PlayWAV(const char* filename);
	void SetVolume(int volume);
private:
    Audio();
    ~Audio();

#if defined BASSMODE
    MUSIC currentMusic = 0;
    HSAMPLE WavChunk[WAVNUM];
    MIDI_FONT midfonts;
#else
    MUSIC currentMusic = 0;
    WAV WavChunk[WAVNUM];
    MIX_Track* track_music_{};
    std::vector<MIX_Track*> track_wav_;
    MIX_Mixer* mixer_{};

    #ifdef TSFMODE
    tsf* g_TinySoundFont = nullptr;           
    tml_message* g_MidiMessage = nullptr;
    tml_message* g_CurrentMidiMessage = nullptr; 
    
    unsigned long long g_TotalFramesRendered = 0; 
    
    int g_SampleRate = 44100;                 
    SDL_AudioStream* g_MidiStream = nullptr;  
    SDL_Mutex* g_MidiMutex = nullptr;         
    float g_MidiVolume = 1.0f;                
    bool isPlayingTSF_ = false;

    static void SDLCALL MidiStreamCallback(void* userdata, SDL_AudioStream* stream, int additional_amount, int total_amount);
    void RenderMidiBlock(float* stream, int num_samples);
    #endif

#endif
    int currentWav = 0;
    int current_music_index_ = -1;
    bool hasMidSF2Font_ = false; 

    MUSIC loadMusic(const char* filename);
    WAV loadWav(const char* filename);

    static char currentfile[255]; 
};