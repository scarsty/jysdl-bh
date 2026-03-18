#include "jymain.h"
#include "Audio.h"
#include "sdlfun.h"
#include <filesystem>

#ifdef TSFMODE
#include <algorithm> 
#endif

#if !defined(BASSMODE) && defined(TSFMODE)
#define TSF_IMPLEMENTATION
#define TSF_NO_STDIO
#include "tsf.h"

#define TML_IMPLEMENTATION
#define TML_NO_STDIO
#include "tml.h"
#endif

char Audio::currentfile[255] = "\0";

Audio::Audio() {}
Audio::~Audio() {}

void Audio::init()
{
    int so = 44100;
    hasMidSF2Font_ = std::filesystem::exists(g_MidSF2);

#if defined BASSMODE
    if (!BASS_Init(-1, so, 0, 0, NULL))
    {
        JY_Error("Can't initialize device");
        g_EnableSound = 0;
    }

    currentWav = 0;
    for (int i = 0; i < WAVNUM; i++)
    {
        WavChunk[i] = 0;
    }

    if (hasMidSF2Font_)
    {
        midfonts.font = BASS_MIDI_FontInit(g_MidSF2, 0);
        if (!midfonts.font)
        {
            JY_Error("BASS_MIDI_FontInit error ! %d", BASS_ErrorGetCode());
        }
        midfonts.preset = -1;
        midfonts.bank = 0;
        BASS_MIDI_StreamSetFonts(0, &midfonts, 1);
    }
#else
    if (MIX_Init() == 0) {
        JY_Error("Couldn't initialize SDL_Mixer: %s\n", SDL_GetError());
        g_EnableSound = 0;
    }
    SDL_AudioSpec spec;
    spec.freq = 44100;
    spec.format = SDL_AUDIO_S16;
    spec.channels = 2;
    mixer_ = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec);
    track_music_ = MIX_CreateTrack(mixer_);
    track_wav_.resize(WAVNUM);
    for (auto& t : track_wav_)
    {
        t = MIX_CreateTrack(mixer_);
    }
    
    if (hasMidSF2Font_)
    {
        SDL_setenv_unsafe("SDL_SOUNDFONTS", g_MidSF2, 1);
    }

    #ifdef TSFMODE
    g_MidiMutex = SDL_CreateMutex();
    g_MidiVolume = (float)g_MusicVolume / 128.0f;
    g_TotalFramesRendered = 0; 

    if (hasMidSF2Font_) {
        size_t size = 0;
        void* data = SDL_LoadFile(g_MidSF2, &size);
        if (data) {
            g_TinySoundFont = tsf_load_memory(data, (int)size);
            SDL_free(data); 
            
            if (g_TinySoundFont) {
                tsf_set_output(g_TinySoundFont, TSF_STEREO_INTERLEAVED, so, 0.0f);
                g_SampleRate = so;
            }
        } else {
            JY_Error("TSF Init: Failed to load SoundFont: %s", g_MidSF2);
        }
    }

    SDL_AudioSpec midiSpec;
    midiSpec.freq = so;
    midiSpec.format = SDL_AUDIO_F32; 
    midiSpec.channels = 2;
    g_MidiStream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &midiSpec, NULL, NULL);
    if (g_MidiStream) {
        SDL_SetAudioStreamGetCallback(g_MidiStream, MidiStreamCallback, this);
        SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(g_MidiStream));
    } else {
        JY_Error("TSF: Failed to open audio stream: %s", SDL_GetError());
    }
    #endif 

    currentWav = 0;
    for (int i = 0; i < WAVNUM; i++)
    {
        WavChunk[i] = 0;
    }
#endif
}

void Audio::quit()
{
    PlayMIDI("");
    for (int i = 0; i < WAVNUM; i++)
    {
        if (WavChunk[i])
        {
#if defined BASSMODE
            BASS_SampleFree(WavChunk[i]);
#else
            MIX_DestroyAudio(WavChunk[i]);
#endif
            WavChunk[i] = 0;
        }
    }
#if defined BASSMODE
    if (midfonts.font)
    {
        BASS_MIDI_FontFree(midfonts.font);
    }
    BASS_Free();
#else
    
    #ifdef TSFMODE
    if (g_MidiStream) {
        SDL_DestroyAudioStream(g_MidiStream);
        g_MidiStream = nullptr;
    }
    if (g_TinySoundFont) {
        tsf_close(g_TinySoundFont);
        g_TinySoundFont = nullptr;
    }
    if (g_MidiMessage) {
        tml_free(g_MidiMessage);
        g_MidiMessage = nullptr;
    }
    if (g_MidiMutex) {
        SDL_DestroyMutex(g_MidiMutex);
        g_MidiMutex = nullptr;
    }
    #endif

    MIX_DestroyMixer(mixer_);
#endif
}

int Audio::PlayMIDI(const char* filename)
{
#ifdef TSFMODE
    if (g_EnableSound == 0) return 1;
#else
    if (g_EnableSound == 0)
    {
        JY_Error("disable sound!");
        return 1;
    }
#endif

    if (strlen(filename) == 0)
    {
        StopMIDI();
        strcpy(currentfile, filename);
        return 0;
    }
    if (strcmp(currentfile, filename) == 0)
    {
        return 0;
    }
    StopMIDI();

#ifdef TSFMODE
    std::string fname = filename;
    std::string fname_lower = fname;
    std::transform(fname_lower.begin(), fname_lower.end(), fname_lower.begin(), ::tolower);
    bool isMidiFile = hasMidSF2Font_ && (fname_lower.find(".mid") != std::string::npos);
#endif

#if defined BASSMODE
    currentMusic = loadMusic(filename);

    if (currentMusic == 0) {
        JY_Debug("Open music file %s failed!", filename);
        strcpy(currentfile, filename);
        return 1;
    }

    if (g_MP3 == 1)
    {
        BASS_MIDI_StreamSetFonts(currentMusic, &midfonts, 1);
    }
    SetVolume(g_MusicVolume);
    BASS_ChannelFlags(currentMusic, BASS_SAMPLE_LOOP, BASS_SAMPLE_LOOP);
    BASS_ChannelPlay(currentMusic, FALSE);

#else 

    #ifdef TSFMODE
    
    if (isMidiFile && !g_TinySoundFont && g_MidiStream && hasMidSF2Font_) {
        SDL_LockMutex(g_MidiMutex);
        size_t size = 0;
        void* data = SDL_LoadFile(g_MidSF2, &size);
        if (data) {
            g_TinySoundFont = tsf_load_memory(data, (int)size);
            SDL_free(data);
            if (g_TinySoundFont) {
                tsf_set_output(g_TinySoundFont, TSF_STEREO_INTERLEAVED, g_SampleRate, 0.0f);
                tsf_set_volume(g_TinySoundFont, g_MidiVolume);
                JY_Debug("TSF: Lazy loaded SoundFont from %s", g_MidSF2);
            }
        }
        SDL_UnlockMutex(g_MidiMutex);
    }

    if (isMidiFile && g_TinySoundFont && g_MidiStream) {
        size_t size = 0;
        void* data = SDL_LoadFile(filename, &size);
        if (data) {
            tml_message* newMidi = tml_load_memory(data, (int)size);
            SDL_free(data);
            
            if (newMidi) {
                SDL_LockMutex(g_MidiMutex);
                
                tsf_reset(g_TinySoundFont); 

                g_MidiMessage = newMidi;
                g_CurrentMidiMessage = g_MidiMessage;
                
                g_TotalFramesRendered = 0; 
                
                isPlayingTSF_ = true;

                g_MidiVolume = (float)g_MusicVolume / 128.0f;
                if (g_TinySoundFont) tsf_set_volume(g_TinySoundFont, g_MidiVolume);
                SDL_UnlockMutex(g_MidiMutex);
                
                SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(g_MidiStream));
                
                strcpy(currentfile, filename);
                return 0; 
            }
        }
        JY_Error("TSF: Failed to load MIDI file: %s", filename);
    }
    #endif

    currentMusic = loadMusic(filename);

    if (currentMusic == 0) {
        JY_Debug("Open music file %s failed!", filename);
        strcpy(currentfile, filename);
        return 1;
    }

    MIX_SetTrackAudio(track_music_, currentMusic);
	SetVolume(g_MusicVolume);
    Prop pro;
    pro.set(MIX_PROP_PLAY_FADE_IN_MILLISECONDS_NUMBER, 500);
    pro.set(MIX_PROP_PLAY_LOOPS_NUMBER, -1);
    MIX_PlayTrack(track_music_, pro.id());
#endif
    strcpy(currentfile, filename);
    return 0;
}

MUSIC Audio::loadMusic(const char* filename)
{
#if defined BASSMODE
    MUSIC m = 0;
    if (g_MP3 == 1)
    {
        m = BASS_StreamCreateFile(0, filename, 0, 0, 0);
    }
    else
    {
        m = BASS_MIDI_StreamCreateFile(0, filename, 0, 0, 0, 0);
    }
#else
    MUSIC m = nullptr;
    #ifdef TSFMODE
    Prop pro;
    auto io = SDL_IOFromFile(filename, "rb");
    if (io) {
        pro.set(MIX_PROP_AUDIO_LOAD_IOSTREAM_POINTER, io);
        m = MIX_LoadAudioWithProperties(pro.id());
        SDL_CloseIO(io);
    }
    #else
    if (hasMidSF2Font_ && (std::string(filename).find(".mid") != std::string::npos))
    {
        Prop pro;
        auto io = SDL_IOFromFile(filename, "rb");
        pro.set(MIX_PROP_AUDIO_LOAD_IOSTREAM_POINTER, io);
        pro.set(MIX_PROP_AUDIO_DECODER_STRING, "fluidsynth");
        
        pro.set("SDL_mixer.decoder.fluidsynth.soundfont_path", g_MidSF2);
        m = MIX_LoadAudioWithProperties(pro.id());
        SDL_CloseIO(io);
    }
    else
    {
        Prop pro;
        auto io = SDL_IOFromFile(filename, "rb");
        pro.set(MIX_PROP_AUDIO_LOAD_IOSTREAM_POINTER, io);
        m = MIX_LoadAudioWithProperties(pro.id());
        SDL_CloseIO(io);
    }
    #endif

#endif
    return m;
}

int Audio::StopMIDI()
{
#if defined BASSMODE
    if (currentMusic)
    {
        BASS_ChannelStop(currentMusic);
        BASS_StreamFree(currentMusic);
        currentMusic = 0;
    }
#else
    #ifdef TSFMODE
    if (g_MidiStream && g_MidiMessage) {
        SDL_LockMutex(g_MidiMutex);
        tml_free(g_MidiMessage);
        g_MidiMessage = nullptr;
        g_CurrentMidiMessage = nullptr;
        if (g_TinySoundFont) tsf_note_off_all(g_TinySoundFont);
        isPlayingTSF_ = false;
        SDL_UnlockMutex(g_MidiMutex);
    }
    #endif

    if (track_music_) MIX_StopTrack(track_music_, 1000);
    if (currentMusic) {
        MIX_DestroyAudio(currentMusic);
        currentMusic = 0;
    }
#endif
    return 0;
}

int Audio::PausedMIDI(void)
{
#if defined BASSMODE
    if (currentMusic) {
        BASS_ChannelStop(currentMusic);
    }
#else
    #ifdef TSFMODE
    if (isPlayingTSF_ && g_MidiStream) {
        SDL_PauseAudioDevice(SDL_GetAudioStreamDevice(g_MidiStream));
    }
    #endif

    if (track_music_ != NULL) {
        MIX_PauseTrack(track_music_);
    }
#endif
    return 0;
}

int Audio::ResumeMIDI(void)
{
#if defined BASSMODE
    if (currentMusic)
    {
        BASS_ChannelPlay(currentMusic, FALSE);
    }
#else
    #ifdef TSFMODE
    if (isPlayingTSF_ && g_MidiStream) {
        SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(g_MidiStream));
    }
    #endif

    if (track_music_ != NULL) {
        MIX_ResumeTrack(track_music_);
    }
#endif
    return 0;
}

int Audio::PlayWAV(const char* filename)
{
#if defined BASSMODE
    HCHANNEL ch;
#endif
    if (g_EnableSound == 0)
    {
        return 1;
    }
    if (WavChunk[currentWav])
    {
#if defined BASSMODE
        BASS_SampleStop(WavChunk[currentWav]);
        BASS_SampleFree(WavChunk[currentWav]);
#else
        MIX_DestroyAudio(WavChunk[currentWav]);
#endif
        WavChunk[currentWav] = 0;
    }
    WavChunk[currentWav] = loadWav(filename);

    if (WavChunk[currentWav])
    {
#if defined BASSMODE
        ch = BASS_SampleGetChannel(WavChunk[currentWav], 0);
        BASS_ChannelSetAttribute(ch, BASS_ATTRIB_VOL, (float)(g_SoundVolume / 128.0));
        BASS_ChannelFlags(ch, 0, BASS_SAMPLE_LOOP);
        BASS_ChannelPlay(ch, 0);
#else
        MIX_SetTrackAudio(track_wav_[currentWav], WavChunk[currentWav]);
        float gain = (float)g_SoundVolume / 128.0f;
        MIX_SetTrackGain(track_wav_[currentWav], gain);
        int result = MIX_PlayTrack(track_wav_[currentWav], 0);
        if (result < 0) {
            SDL_Log("MIX_PlayTrack failed: %s", SDL_GetError());
            MIX_DestroyTrack(track_wav_[currentWav]);
            return -1;
        }
#endif
        currentWav++;
        if (currentWav >= WAVNUM)
        {
            currentWav = 0;
        }
    }
    else
    {
        JY_Error("Open wav file %s failed!", filename);
        SDL_Log("Open wav file %s failed!", filename);
    }
    return 0;
}

WAV Audio::loadWav(const char* filename)
{
#if defined BASSMODE
    WAV w = 0;
    w = BASS_SampleLoad(0, filename, 0, 0, 1, 0);
#else
    WAV w = 0;
    w = MIX_LoadAudio(nullptr, filename, false);
#endif
    return w;
}

void Audio::SetVolume(int volume)
{
    if (volume < 0) {
        volume = 0;
    }
    if (volume > 128) {
        volume = 128;
    }

    g_MusicVolume = volume;

#if defined BASSMODE
    if (currentMusic) {
        BASS_ChannelSetAttribute(currentMusic, BASS_ATTRIB_VOL, (float)(g_MusicVolume / 128.0));
    }
#else
    #ifdef TSFMODE
    if (g_MidiMutex) {
        SDL_LockMutex(g_MidiMutex);
        g_MidiVolume = (float)volume / 128.0f;
        if (g_TinySoundFont) tsf_set_volume(g_TinySoundFont, g_MidiVolume);
        SDL_UnlockMutex(g_MidiMutex);
    }
    #endif

    float gain = (float)g_MusicVolume / 128.0f;
    if (track_music_) {
        MIX_SetTrackGain(track_music_, gain);
    }
#endif
}

#if !defined(BASSMODE) && defined(TSFMODE)

void Audio::RenderMidiBlock(float* stream, int num_samples)
{
    int num_frames = num_samples / 2; 
    int frames_filled = 0;
    float* out_ptr = stream;

    double sample_rate_double = (double)g_SampleRate;
    double ms_per_frame = 1000.0 / sample_rate_double;

    int max_events_per_block = 5000; 
    int events_processed = 0;

    while (frames_filled < num_frames) {
        
        if (!g_CurrentMidiMessage) {
            
            g_CurrentMidiMessage = g_MidiMessage;
            
            g_TotalFramesRendered = 0; 

            if (!g_CurrentMidiMessage) {
                int remaining = (num_frames - frames_filled) * 2;
                memset(out_ptr, 0, remaining * sizeof(float));
                return;
            }
        }

        double current_time_ms = g_TotalFramesRendered * ms_per_frame;

        double time_to_next_event = g_CurrentMidiMessage->time - current_time_ms;

        if (time_to_next_event <= 0.01) { 
            
            if (++events_processed > max_events_per_block) {
                g_TotalFramesRendered += 1; 
                frames_filled += 1; 
                out_ptr += 2;
                continue; 
            }

            switch (g_CurrentMidiMessage->type) {
                case TML_NOTE_ON:
                    tsf_channel_note_on(g_TinySoundFont, g_CurrentMidiMessage->channel, g_CurrentMidiMessage->key, g_CurrentMidiMessage->velocity / 127.0f);
                    break;
                case TML_NOTE_OFF:
                    tsf_channel_note_off(g_TinySoundFont, g_CurrentMidiMessage->channel, g_CurrentMidiMessage->key);
                    break;
                case TML_PROGRAM_CHANGE:
                    tsf_channel_set_presetnumber(g_TinySoundFont, g_CurrentMidiMessage->channel, g_CurrentMidiMessage->program, (g_CurrentMidiMessage->channel == 9));
                    break;
                case TML_PITCH_BEND:
                    tsf_channel_set_pitchwheel(g_TinySoundFont, g_CurrentMidiMessage->channel, g_CurrentMidiMessage->pitch_bend);
                    break;
                case TML_CONTROL_CHANGE:
                    tsf_channel_midi_control(g_TinySoundFont, g_CurrentMidiMessage->channel, g_CurrentMidiMessage->control, g_CurrentMidiMessage->control_value);
                    break;
            }
            
            g_CurrentMidiMessage = g_CurrentMidiMessage->next;
            continue; 
        }

        int frames_remaining_in_buffer = num_frames - frames_filled;
        
        int frames_until_event = (int)(time_to_next_event / ms_per_frame);

        if (frames_until_event <= 0) frames_until_event = 1;

        int frames_to_render = frames_remaining_in_buffer;
        if (frames_to_render > frames_until_event) {
            frames_to_render = frames_until_event;
        }
        
        if (frames_to_render > 512) frames_to_render = 512;

        tsf_render_float(g_TinySoundFont, out_ptr, frames_to_render, 0);

        frames_filled += frames_to_render;
        out_ptr += frames_to_render * 2;
        
        g_TotalFramesRendered += frames_to_render;
    }
}

void SDLCALL Audio::MidiStreamCallback(void* userdata, SDL_AudioStream* stream, int additional_amount, int total_amount)
{
    Audio* audio = (Audio*)userdata;
    
    if (audio->g_MidiMutex) SDL_LockMutex(audio->g_MidiMutex);
    if (!audio->g_TinySoundFont || !audio->g_MidiMessage) {
        if (additional_amount > 0) {
            void* silence = SDL_calloc(1, additional_amount);
            SDL_PutAudioStreamData(stream, silence, additional_amount);
            SDL_free(silence);
        }
        if (audio->g_MidiMutex) SDL_UnlockMutex(audio->g_MidiMutex);
        return;
    }

    int bytes_per_sample = sizeof(float);
    int num_samples = additional_amount / bytes_per_sample;
    
    float* buffer = (float*)SDL_malloc(additional_amount);
    if (!buffer) {
        if (audio->g_MidiMutex) SDL_UnlockMutex(audio->g_MidiMutex);
        return;
    }

    audio->RenderMidiBlock(buffer, num_samples);
    if (audio->g_MidiMutex) SDL_UnlockMutex(audio->g_MidiMutex);

    SDL_PutAudioStreamData(stream, buffer, additional_amount);
    SDL_free(buffer);
}
#endif