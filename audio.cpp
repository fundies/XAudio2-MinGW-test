//Safely release and delete objects
#define SafeRelease(pInterface) if(pInterface != NULL) {pInterface->Release(); pInterface=NULL;}
#define SafeDelete(pObject) if(pObject != NULL) {delete pObject; pObject=NULL;}

#include "audio.h"

//OGG handling code is from a tutorial @ www.flipcode.com

//Built with:
//OGG version 1.1.3
//Vorbis version 1.2.0

CAudio::CAudio(void)
{
    pXAudio2 = NULL;
    pMasteringVoice = NULL;
    pSourceVoice = NULL;

    resetParams();

#ifdef _DEBUG
    flags |= XAUDIO2_DEBUG_ENGINE;
#endif

    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    //LogInfo("<li>Audio created OK");
}

CAudio::~CAudio(void)
{
    if (pSourceVoice != NULL)
    {
        pSourceVoice->Stop(0);
        pSourceVoice->DestroyVoice();
    }

    if (pMasteringVoice != NULL)
        pMasteringVoice->DestroyVoice();

    SafeRelease(pXAudio2);

    if (bFileOpened)
        ov_clear(&vf);

    CoUninitialize();

    //LogInfo("<li>Audio destroyed OK");
}

void CAudio::resetParams()
{
    bFileOpened = false;
    isRunning = false;
    boolIsPaused = false;
    bLoop = false;
    bDone = false;
    bAlmostDone = false;
    currentDiskReadBuffer = 0;
    flags = 0;
}

bool CAudio::InitialiseAudio()
{
    HRESULT hr;
    if (FAILED(hr = XAudio2Create( &pXAudio2, flags)))
    {
        //LogError("<li>Failed to init XAudio2 engine: %#X", hr );
        return false;
    }

    if (FAILED(hr = pXAudio2->CreateMasteringVoice(&pMasteringVoice)))
    {
        //LogError("<li>Failed creating mastering voice: %#X", hr);
        return false;
    }

    return true;
}

bool CAudio::LoadSound(const char* szSoundFileName)
{
#if 0
    //WCHAR wstrSoundPath[MAX_PATH];
    char strSoundPath[MAX_PATH];

    //Get the our applications "sounds" directory.
    GetCurrentDirectory(MAX_PATH, strSoundPath);
    strncat(strSoundPath, "\\Sounds\\", MAX_PATH);
    strncat(strSoundPath, szSoundFileName, MAX_PATH);
#else
    // Just use the given filename as is
    const char *strSoundPath = szSoundFileName;
#endif

    //Convert the path to unicode.
    //MultiByteToWideChar(CP_ACP, 0, strSoundPath, -1, wstrSoundPath, MAX_PATH);

    //If we already have a file open then kill the current voice setup
    if (bFileOpened)
    {
        pSourceVoice->Stop(0);
        pSourceVoice->DestroyVoice();

        ov_clear(&vf);

        resetParams();
    }

    FILE *f;
    //errno_t err;

    //if ((err  = fopen_s( &f, strSoundPath, "rb" )) != 0)
    if (!(f = fopen( strSoundPath, "rb" )))
    {
        //LogError("<li>Failed to open audio: %s", strSoundPath);

        /*
        char szBuffer[MAX_PATH];
         _strerror_s(szBuffer, MAX_PATH, NULL);
         */
        //LogError("<li>Reason: %s", szBuffer);
        return false;
    }

    //ov_open(f, &vf, NULL, 0); //Windows does not like this function so we use ov_open_callbacks() instead
    if (ov_open_callbacks(f, &vf, NULL, 0, OV_CALLBACKS_DEFAULT) < 0)
    {
        fclose(f);
        return false;
    }
    else
        bFileOpened = true;

    //The vorbis_info struct keeps the most of the interesting format info
    vorbis_info *vi = ov_info(&vf, -1);

    //Set the wave format
    WAVEFORMATEX wfm;
    memset(&wfm, 0, sizeof(wfm));

    wfm.cbSize          = sizeof(wfm);
       wfm.nChannels       = vi->channels;
    wfm.wBitsPerSample  = 16;                    //Ogg vorbis is always 16 bit
    wfm.nSamplesPerSec  = vi->rate;
    wfm.nAvgBytesPerSec = wfm.nSamplesPerSec * wfm.nChannels * 2;
    wfm.nBlockAlign     = 2 * wfm.nChannels;
    wfm.wFormatTag      = 1;

    DWORD pos = 0;
    int sec = 0;
    int ret = 1;

    memset(&buffers[currentDiskReadBuffer], 0, sizeof(buffers[currentDiskReadBuffer]));

    //Read in the bits
    while(ret && pos<STREAMING_BUFFER_SIZE)
    {
        ret = ov_read(&vf, buffers[currentDiskReadBuffer]+pos, STREAMING_BUFFER_SIZE-pos, 0, 2, 1, &sec);
        pos += ret;
    }

    HRESULT hr;

    //Create the source voice
    if (FAILED(hr = pXAudio2->CreateSourceVoice(&pSourceVoice, &wfm)))
    {
        //LogError("<li>Error %#X creating source voice", hr);
        return false;
    }

    //Submit the wave sample data using an XAUDIO2_BUFFER structure
    XAUDIO2_BUFFER buffer = {0};
    buffer.pAudioData = (BYTE*)&buffers[currentDiskReadBuffer];
    buffer.AudioBytes = STREAMING_BUFFER_SIZE;
    
    if(FAILED(hr = pSourceVoice->SubmitSourceBuffer(&buffer)))
    {
        //LogError("<li>Error %#X submitting source buffer", hr);
        return false;
    }

    currentDiskReadBuffer++;

    return true;
}

bool CAudio::Play(bool loop)
{
    if (pSourceVoice == NULL)
    {
        //LogError("<li>Error: pSourceVoice NOT created");
        return false;
    }

    HRESULT hr;

    if(FAILED(hr = pSourceVoice->Start(0)))
    {
        //LogError("<li>Error %#X submitting source buffer", hr);
    }

    XAUDIO2_VOICE_STATE state;
    pSourceVoice->GetState(&state);
    isRunning = (state.BuffersQueued > 0) != 0;

    bLoop = loop;
    bDone = false;
    bAlmostDone = false;
    boolIsPaused = false;

    return isRunning;
}

void CAudio::Stop()
{
    if (pSourceVoice == NULL)
        return;

    //XAUDIO2_FLUSH_BUFFERS according to MSDN is meant to flush the buffers after the voice is stopped
    //unfortunately the March 2008 release of the SDK does not include this parameter in the xaudio files
    //and I have been unable to ascertain what its value is
    //pSourceVoice->Stop(XAUDIO2_FLUSH_BUFFERS);
    pSourceVoice->Stop(0);

    boolIsPaused = false;
    isRunning = false;
}

bool CAudio::IsPlaying()
{
    /*XAUDIO2_VOICE_STATE state;
    pSourceVoice->GetState(&state);
    return (state.BuffersQueued > 0) != 0;*/

    return isRunning;
}


//Alter the volume up and down
void CAudio::AlterVolume(float fltVolume)
{
    if (pSourceVoice == NULL)
        return;

    pSourceVoice->SetVolume(fltVolume);         //Current voice volume
    //pMasteringVoice->SetVolume(fltVolume);    //Playback device volume
}

//Return the current volume
void CAudio::GetVolume(float &fltVolume)
{
    if (pSourceVoice == NULL)
        return;

    pSourceVoice->GetVolume(&fltVolume);
    //pMasteringVoice->GetVolume(&fltVolume);
}

void CAudio::Pause()
{
    if (pSourceVoice == NULL)
        return;

    if (boolIsPaused)
    {
        pSourceVoice->Start(0); //Unless we tell it otherwise the voice resumes playback from its last position
        boolIsPaused = false;
    }
    else
    {
        pSourceVoice->Stop(0);
        boolIsPaused = true;
    }
}

void CAudio::Update()
{
    if (pSourceVoice == NULL)
        return;

    if (!isRunning)
        return;

    //Do we have any free buffers?
    XAUDIO2_VOICE_STATE state;
    pSourceVoice->GetState( &state );
    if ( state.BuffersQueued < MAX_BUFFER_COUNT - 1 )
    {
        if (bDone && !bLoop)
        {
            pSourceVoice->Stop(0);
        }

        //Got to use this trick because otherwise all the bits wont play
        if (bAlmostDone && !bLoop)
            bDone = true;

        memset(&buffers[currentDiskReadBuffer], 0, sizeof(buffers[currentDiskReadBuffer]));

        DWORD pos = 0;
            int sec = 0;
            int ret = 1;
                
            while(ret && pos<STREAMING_BUFFER_SIZE)
            {
                    ret = ov_read(&vf, buffers[currentDiskReadBuffer]+pos, STREAMING_BUFFER_SIZE-pos, 0, 2, 1, &sec);
                    pos += ret;
            }

            //Reached the end?
            if (!ret && bLoop)
            {
            //We are looping so restart from the beginning
            //NOTE: sound with sizes smaller than BUFSIZE may be cut off

            ret = 1;
            ov_pcm_seek(&vf, 0);
            while(ret && pos<STREAMING_BUFFER_SIZE)
            {
                ret = ov_read(&vf, buffers[currentDiskReadBuffer]+pos, STREAMING_BUFFER_SIZE-pos, 0, 2, 1, &sec);
                pos += ret;
            }
        }
        else if (!ret && !(bLoop))
            {
                    //Not looping so fill the rest with 0
                    //while(pos<size)
                    //    *(buffers[currentDiskReadBuffer]+pos)=0; pos ++;

                    //And say that after the current section no other section follows
                    bAlmostDone = true;
            }

        XAUDIO2_BUFFER buffer = {0};
        buffer.pAudioData = (BYTE*)&buffers[currentDiskReadBuffer];
        if (bAlmostDone)
            buffer.Flags = NULL;   //Tell the source voice not to expect any data after this buffer
        buffer.AudioBytes = STREAMING_BUFFER_SIZE;

        HRESULT hr;
        if( FAILED(hr = pSourceVoice->SubmitSourceBuffer( &buffer ) ) )
        {
            //LogError("<li>Error %#X submitting source buffer\n", hr );
            return;
        }
        
        currentDiskReadBuffer++;
        currentDiskReadBuffer %= MAX_BUFFER_COUNT;
    }
}
