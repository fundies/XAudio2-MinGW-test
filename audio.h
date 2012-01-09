#pragma once
//#include "base.h"

#include <XAudio2.h>
#include <codec.h>
#include <vorbisfile.h>

#define STREAMING_BUFFER_SIZE 65536*10
#define MAX_BUFFER_COUNT 3

class CAudio //:
//public CBase
{
public:
    CAudio(void);
    virtual ~CAudio(void);

    bool IsPlaying();
    void Stop();
    bool Play(bool loop = true);
    bool LoadSound(const char* szSoundFilePath);
    bool InitialiseAudio();
    void AlterVolume(float fltVolume);
    void GetVolume(float &fltVolume);
    void Pause();
    void Update();

private:
    IXAudio2* pXAudio2;
    IXAudio2MasteringVoice* pMasteringVoice;
    IXAudio2SourceVoice* pSourceVoice;

    UINT32 flags;
    char buffers[MAX_BUFFER_COUNT][STREAMING_BUFFER_SIZE];
    bool bFileOpened;
    OggVorbis_File vf;
    bool isRunning;
    bool boolIsPaused;
    bool bAlmostDone;
    bool bDone;
    bool bLoop;
    DWORD currentDiskReadBuffer;

    void resetParams();
};
