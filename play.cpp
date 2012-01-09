//--------------------------------------------------------------------------------------
// File: XAudio2BasicSound.cpp
//
// XNA Developer Connection
// (C) Copyright Microsoft Corp.  All rights reserved.
//--------------------------------------------------------------------------------------
#define _WIN32_DCOM
#define _CRT_SECURE_NO_DEPRECATE
//#include <windows.h>
//#include <xaudio2.h>
//#include <strsafe.h>
//#include <shellapi.h>
//#include <mmsystem.h>
//#include <conio.h>
#include "audio.h"

/*
//--------------------------------------------------------------------------------------
// Helper macros
//--------------------------------------------------------------------------------------
#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p);   (p)=NULL; } }
#endif
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }
#endif
*/


//--------------------------------------------------------------------------------------
// Entry point to the program
//--------------------------------------------------------------------------------------
int main()
{
    CAudio* m_pAudioBg = new CAudio();
    if (m_pAudioBg->InitialiseAudio())			//Setup OGG playback
        m_pAudioBg->LoadSound("base.ogg");	//Sound to play - directory location hard coded into sound class

    m_pAudioBg->Play(false);

    // Call  Update()  from your main loop...
    while(m_pAudioBg->IsPlaying())  //IsPlaying() doesn't seem to work...
        m_pAudioBg->Update();
}
