/*##########################################################################

	macsound.c

	Routines for performing simple sound streaming.

##########################################################################*/

#include "driver.h"

#include "mac.h"
#include "macsound.h"

void dtox80 (const double *x, extended80 *x80);

#ifdef __MWERKS__
#pragma require_prototypes on
#endif

/*##########################################################################
	COMPILE-TIME DEFINITIONS
##########################################################################*/

#define LOG_SOUND_STATUS		0
#define USE_DOUBLEBACK			(!TARGET_API_MAC_CARBON)


/*##########################################################################
	CONSTANTS
##########################################################################*/

enum
{
	kSoundStreamFramesAt44kHz		= 12288,
	kTicksToWaitBeforeStopping		= 15,
	kTotalSoundBuffers				= 3,
	kQueuedSoundBuffers				= 2
};


/*##########################################################################
	GLOBAL VARIABLES
##########################################################################*/

// our buffered MAME data
static SInt16			sSoundStream[kSoundStreamFramesAt44kHz * 2];
static UInt32			sSoundStreamFrames;
static UInt32			sSoundIn;
static UInt32			sSoundOut;
static UInt32			sSoundInTotalFrames;
static UInt32			sSoundOutTotalFrames;

// the sound channel
static SndChannelPtr	sSoundChannel;

// sound timing
ComponentInstance 		gSoundClock;
TimeRecord				gSoundTime;
wide					gSoundClockConversion;

// sound buffer parameters
static UInt32			sSoundBufferFrames;
static UInt32			sSoundBufferBytes;
static Boolean			sSoundIsStereo;
static Boolean			sSoundIsPaused;
static Boolean			sSoundIsPausedWithinMAME;
static SInt16			sLastSample[2];
static volatile Boolean	sStopSoundPlaying;

// sound buffers
#if USE_DOUBLEBACK
static SndDoubleBackUPP	sDoubleBackCallback;
static SndDoubleBufferHeader sSoundDoubleBuffer;
#else
static SndCallBackUPP	sSoundCallback;
static ExtSoundHeader *	sSoundBuffer[kTotalSoundBuffers];
static SndCommand		sSoundBufferCmd[kTotalSoundBuffers];
static SndCommand		sSoundCallbackCmd[kTotalSoundBuffers];
#endif

// timing data
static double 			sFramesPerVideoFrame;
static double 			sFramesLeftOver;
static UInt32			sFramesThisVideoFrame;

static volatile Boolean	sQueuedEmptyBuffer;

// flag to prevent us to crash when init fails (R. Nabet 001212)
static Boolean			sInitFailed;

/*##########################################################################
	FUNCTION PROTOTYPES
##########################################################################*/

#if USE_DOUBLEBACK
static pascal void 		DoubleBackCallback(SndChannelPtr inChannel, SndDoubleBufferPtr inDoubleBuffer);
#else
static pascal void 		SoundCallback(SndChannelPtr inChannel, SndCommand *inCommand);
#endif
static void 			FillSoundBuffer(SInt16 *inBuffer);

static Boolean			InitializeSoundChannel(void);
static Boolean			InitializeSoundBuffers(void);
static void 			FreeSoundBuffers(void);


/*##########################################################################
	IMPLEMENTATION
##########################################################################*/

#pragma mark ¥ Basic MAME Streaming

//===============================================================================
//	osd_start_audio_stream
//
//	Start the audio system going.
//===============================================================================

int osd_start_audio_stream(int stereo)
{
	UInt32 microsecondsPerFrame;
	
	// don't do anything at all if we're not playing sound
	if (!gPrefs.playSound)
		return 1; // must not return 0 with 0.93 sound rework

	// remember the stereo setting
	sSoundIsStereo = stereo ? 1 : 0;
	sSoundIsPaused = false;
	sSoundIsPausedWithinMAME = false;
	
	// reset the sound buffer
	sSoundIn = sSoundOut = 0;
	sSoundInTotalFrames = sSoundOutTotalFrames = 0;
	sSoundStreamFrames = kSoundStreamFramesAt44kHz * gPrefs.sampleRate / 44100;
	sLastSample[0] = sLastSample[1] = 0;
	sStopSoundPlaying = false;

	// pick the appropriate sound buffer size
	sSoundBufferFrames = 2048 * gPrefs.sampleRate / 44100;
	sSoundBufferBytes = sSoundBufferFrames << (1 + sSoundIsStereo);
	
	// allocate the sound channel and initialize the buffering
	if (InitializeSoundBuffers())
	{
		sInitFailed = TRUE;	// R. Nabet 001212 : we must not play anything
		logerror("InitializeSoundBuffers routine failed (too little memory, or sound manager internal error?)\n");
		return 1;
	}
	
	// get the original volume and set the requested volume
	osd_set_mastervolume(gPrefs.attenuation);
	
	// determine the number of samples per frame; use the rounded number of
	// microseconds per frame, since that's what the video system uses
	microsecondsPerFrame = (UInt32)(1000000.0 / Machine->drv->frames_per_second);
	sFramesPerVideoFrame = (double)gPrefs.sampleRate * (double)microsecondsPerFrame / 1000000.0;

	// compute how many samples to generate this frame
	sFramesLeftOver = sFramesPerVideoFrame;
	sFramesThisVideoFrame = (UInt32)sFramesLeftOver;
	sFramesLeftOver -= (double)sFramesThisVideoFrame;

	sInitFailed = FALSE;	// R. Nabet 001212 : everything is OK

	return sFramesThisVideoFrame;
}


//===============================================================================
//	osd_stop_audio_stream
//
//	Stops the audio system.
//===============================================================================

void osd_stop_audio_stream(void)
{
	UInt32 ticks;
	
	// give the sound system time to shut itself down
	sStopSoundPlaying = true;
	Delay(kTicksToWaitBeforeStopping, &ticks);
	
	// free data
	FreeSoundBuffers();
}


//===============================================================================
//	osd_update_audio_stream
//
//	Pushes data into the audio stream.
//===============================================================================

int osd_update_audio_stream(INT16 *buffer)
{
	UInt32		soundIn = sSoundIn;
	UInt32		framesToCopy;
	
	// don't do anything at all if we're not playing sound
	if (!gPrefs.playSound)
		return 0;

	if (sInitFailed)	// R. Nabet 001212
		return 1;	// a '0' crashes the system

	// copy up to the end of the stream buffer
	while (sFramesThisVideoFrame > 0)
	{
		// determine how many frames we can copy
		framesToCopy = sSoundStreamFrames - soundIn;
		if (framesToCopy > sFramesThisVideoFrame)
			framesToCopy = sFramesThisVideoFrame;
		
		// copy and count the samples
		memcpy(&sSoundStream[soundIn << sSoundIsStereo], buffer, framesToCopy << (1 + sSoundIsStereo));
		sFramesThisVideoFrame -= framesToCopy;
		buffer += framesToCopy << sSoundIsStereo;
		
		// adjust the output pointer
		soundIn += framesToCopy;
		sSoundInTotalFrames += framesToCopy;
		if (soundIn >= sSoundStreamFrames)
			soundIn -= sSoundStreamFrames;
	}
	sSoundIn = soundIn;

	// compute how many samples to generate this frame
	sFramesLeftOver += sFramesPerVideoFrame;
	sFramesThisVideoFrame = (UInt32)sFramesLeftOver;
	sFramesLeftOver -= (double)sFramesThisVideoFrame;

#if LOG_SOUND_STATUS
	{
		static FILE *log;
		static UInt32 startTime;
		static UInt32 biggestDelta;
		
		if (!log)
		{
			log = fopen("sound.log", "w");
			startTime = GetMicroseconds();
			biggestDelta = 0;
		}
		else
		{
			UInt32 delta = sSoundInTotalFrames - sSoundOutTotalFrames;
			UInt32 thisTime;
			Boolean wasBiggest = false;

			thisTime = GetMicroseconds();
			if (delta > biggestDelta)
				biggestDelta = delta, wasBiggest = true;
			fprintf(log, "dt=%10u In=%10u Out=%10u Diff=%10u %c\n", thisTime - startTime,
				sSoundInTotalFrames, sSoundOutTotalFrames, delta, wasBiggest ? '*' : ' ');
		}
	}
#endif
	
	return sFramesThisVideoFrame;
}


#pragma mark -
#pragma mark ¥ MAME Volume Controls

//===============================================================================
//	osd_set_mastervolume
//
//	Sets the main volume.
//===============================================================================

void osd_set_mastervolume(int inAttenuation)
{
	float multiplier = 1.0;
	UInt32 volume;

	// remember the new attenuation
	gPrefs.attenuation = inAttenuation;

	// compute a multiplier from the original volume
	while (inAttenuation++ < 0)
		multiplier *= 1.0 / 1.122018454;	// = (10 ^ (1/20)) = 1dB

	// apply it
	volume = (UInt32)(multiplier * 0x100);
	if (volume > 0x100)
		volume = 0x100;

	{
		SndCommand mySndCmd;
		//OSErr myErr;

		mySndCmd.cmd = volumeCmd;
		mySndCmd.param1 = 0;		// unused with volumeCmd
		mySndCmd.param2 = (volume << 16) | volume;
		/*myErr =*/ /*SndDoImmediate*/SndDoCommand(sSoundChannel, &mySndCmd, false);
	}
}


//===============================================================================
//	osd_get_mastervolume
//
//	Returns the main volume.
//===============================================================================

int osd_get_mastervolume(void)
{
	return gPrefs.attenuation;
}


//===============================================================================
//	osd_sound_enable
//
//	Enables/disables sound.
//===============================================================================

void osd_sound_enable(int enable)
{
	sSoundIsPausedWithinMAME = !enable;
}


#pragma mark -
#pragma mark ¥ Mac-specific Sound Code

//===============================================================================
//	InitializeSound
//
//	Allocates a sound channel and starts it running.
//===============================================================================

void InitializeSound(void)
{
	ComponentResult	result;
	SndCommand 		command;
	OSErr			err;
	
	// attempt to initialize the sound buffers
	InitializeSoundChannel();

	// nuke any existing clock references
	gSoundClock = NULL;

	// first turn on the clock
	command.cmd = clockComponentCmd;
	command.param1 = true;
	err = SndDoImmediate(sSoundChannel, &command);
	if (err != noErr)
		return;
		
	// then get a component instance
	command.cmd = getClockComponentCmd;
	command.param2 = (SInt32)&gSoundClock;
	err = SndDoImmediate(sSoundChannel, &command);
	if (err != noErr)
	{
		gSoundClock = NULL;
		return;
	}
	
	// determine the timebase conversion factor
	gSoundTime.base = NULL;
	result = ClockGetTime(gSoundClock, &gSoundTime);
	if (result != noErr)
	{
		gSoundClock = NULL;
		return;
	}
	*(UInt64 *)&gSoundClockConversion = (1000000LL << 32) / (UInt64)gSoundTime.scale;
}


//===============================================================================
//	TearDownSound
//
//	Deallocates a sound channel.
//===============================================================================

void TearDownSound(void)
{
	UInt32 ticks;
	
	// give the sound system time to shut itself down
	sStopSoundPlaying = true;
	Delay(kTicksToWaitBeforeStopping, &ticks);

	// attempt to free the sound buffers
	FreeSoundBuffers();

	// free the channel
	if (sSoundChannel != NULL)
		SndDisposeChannel(sSoundChannel, true);
	sSoundChannel = NULL;
}


//===============================================================================
//	PauseSound
//
//	Pauses/resumes sound output.
//===============================================================================

void PauseSound(Boolean inPause)
{
	// don't do anything at all if we're not playing sound
	if (!gPrefs.playSound)
		return;

	sSoundIsPaused = inPause;
}


//===============================================================================
//	SyncSound
//
//	Synchronizes the sound with the video by killing off all extra sound in the
//	buffer, and then waiting for the next buffer to get queued.
//===============================================================================

void SyncSound(void)
{
	// don't do anything at all if we're not playing sound
	if (!gPrefs.playSound)
		return;

	sSoundIn = sSoundOut;
	sQueuedEmptyBuffer = false;
	while (!sQueuedEmptyBuffer) { }
}


//===============================================================================
//	FillSoundBuffer
//
//	Common code to fill a sound buffer.
//===============================================================================

static void FillSoundBuffer(SInt16 *inBuffer)
{
	SInt32		framesAvailable = sSoundIn - sSoundOut;
	UInt32		framesToZero = 0;
	UInt32		framesToCopy;
	
	// if we're paused for any reason, skip processing
	if (sSoundIsPaused || sSoundIsPausedWithinMAME)
		framesAvailable = 0;

	// account for the circular buffer
	if (framesAvailable < 0)
		framesAvailable += sSoundStreamFrames;
	
	// if we have more than enough data, clip to the maximum per buffer
	if (framesAvailable >= sSoundBufferFrames)
		framesAvailable = sSoundBufferFrames;
	
	// otherwise, we will need to clear out some data
	else
	{
		// in any case, we will wait for a full buffer; this helps prevent farting noises		
		framesAvailable = 0;
		framesToZero = sSoundBufferFrames;
		sQueuedEmptyBuffer = true;
	}
	
	// copy up to the end of the stream buffer
	while (framesAvailable > 0)
	{
		// determine how many frames we can copy
		framesToCopy = sSoundStreamFrames - sSoundOut;
		if (framesToCopy > framesAvailable)
			framesToCopy = framesAvailable;
		
		// copy and count the samples
		memcpy(inBuffer, &sSoundStream[sSoundOut << sSoundIsStereo], framesToCopy << (1 + sSoundIsStereo));
		framesAvailable -= framesToCopy;
		inBuffer += framesToCopy << sSoundIsStereo;
		
		// adjust the output pointer
		sSoundOut += framesToCopy;
		
		// remember the last samples we added
		if (sSoundIsStereo)
		{
			sLastSample[0] = sSoundStream[sSoundOut * 2 - 2];
			sLastSample[1] = sSoundStream[sSoundOut * 2 - 1];
		}
		else
			sLastSample[0] = sSoundStream[sSoundOut - 1];
		
		// wrap the output pointer
		sSoundOutTotalFrames += framesToCopy;
		if (sSoundOut >= sSoundStreamFrames)
			sSoundOut -= sSoundStreamFrames;
	}
	
	// fill in anything else with 0
	if (sSoundIsStereo)
	{
		while (framesToZero--)
		{
			*inBuffer++ = sLastSample[0];
			*inBuffer++ = sLastSample[1];
		}
	}
	else
	{
		while (framesToZero--)
			*inBuffer++ = sLastSample[0];
	}
}


#pragma mark -
#pragma mark ¥ Double Buffer Version

#if USE_DOUBLEBACK

//===============================================================================
//	InitializeSoundChannel
//
//	Initialize the sound channel and any other one-shot data.
//===============================================================================

static Boolean InitializeSoundChannel(void)
{
	OSErr err;

	// allocate a UPP callback
	sDoubleBackCallback = NewSndDoubleBackUPP(DoubleBackCallback);
	
	// now allocate the channel
	sSoundChannel = NULL;
	err = SndNewChannel(&sSoundChannel, sampledSynth, (sSoundIsStereo ? initStereo : initMono) + initNoInterp + initNoDrop, NULL);
	return (err != noErr);
}


//===============================================================================
//	InitializeSoundBuffers
//
//	Initialize the sound buffers and create the sound channel.
//===============================================================================

static Boolean InitializeSoundBuffers(void)
{
	OSErr err;
	int i;

	// initialize the double buffer header
	memset(&sSoundDoubleBuffer, 0, sizeof(sSoundDoubleBuffer));
	sSoundDoubleBuffer.dbhNumChannels = 1 << sSoundIsStereo;
	sSoundDoubleBuffer.dbhSampleSize = 16;
	sSoundDoubleBuffer.dbhSampleRate = (UInt32)gPrefs.sampleRate << 16;
	sSoundDoubleBuffer.dbhDoubleBack = sDoubleBackCallback;
	
	// prepare the sound buffers and commands
	for (i = 0; i < 2; i++)
	{
		// allocate and clear the sound buffer
		sSoundDoubleBuffer.dbhBufferPtr[i] = malloc(sizeof(SndDoubleBuffer) + sSoundBufferBytes);
		memset(sSoundDoubleBuffer.dbhBufferPtr[i], 0, sizeof(SndDoubleBuffer) + sSoundBufferBytes);
		
		// intialize the buffer structures
		sSoundDoubleBuffer.dbhBufferPtr[i]->dbNumFrames	= sSoundBufferFrames;
		sSoundDoubleBuffer.dbhBufferPtr[i]->dbFlags 	= dbBufferReady;
	}
	
	// start stuff playing
	err = SndPlayDoubleBuffer(sSoundChannel, &sSoundDoubleBuffer);
	return (err != noErr);
}


//===============================================================================
//	FreeSoundBuffers
//
//	Free the sound buffers and the sound channel.
//===============================================================================

static void FreeSoundBuffers(void)
{
	UInt32 i;
	
	// free the buffers
	for (i = 0; i < 2; i++)
	{
		if (sSoundDoubleBuffer.dbhBufferPtr[i] != NULL)
			free(sSoundDoubleBuffer.dbhBufferPtr[i]);
		sSoundDoubleBuffer.dbhBufferPtr[i] = NULL;
	}
}


//===============================================================================
//	DoubleBackCallback
//
//	Callback command to stream data.
//===============================================================================

static pascal void DoubleBackCallback(SndChannelPtr inChannel, SndDoubleBufferPtr inDoubleBuffer)
{
	SInt16 *	outputData = (SInt16 *)inDoubleBuffer->dbSoundData;
	
	// fill the sound buffer
	FillSoundBuffer((SInt16 *)inDoubleBuffer->dbSoundData);
	
	// mark it ready and/or done
	inDoubleBuffer->dbFlags |= dbBufferReady;
	if (sStopSoundPlaying)
		inDoubleBuffer->dbFlags |= dbLastBuffer;
}

#endif


#pragma mark -
#pragma mark ¥ Sound Command Version

#if !USE_DOUBLEBACK

//===============================================================================
//	InitializeSoundChannel
//
//	Initialize the sound channel and any other one-shot data.
//===============================================================================

static Boolean InitializeSoundChannel(void)
{
	OSErr err;

	// allocate a UPP callback
	sSoundCallback = NewSndCallBackUPP(SoundCallback);
	
	// now allocate the channel
	sSoundChannel = NULL;
	err = SndNewChannel(&sSoundChannel, sampledSynth, (sSoundIsStereo ? initStereo : initMono) + initNoInterp + initNoDrop, sSoundCallback);
	return (err != noErr);
}


//===============================================================================
//	InitializeSoundBuffers
//
//	Initialize the sound buffers and create the sound channel.
//===============================================================================

static Boolean InitializeSoundBuffers(void)
{
	double doubleFreq = gPrefs.sampleRate;
	extended80 extFreq;
	int i;
	OSErr err;	// R. Nabet 001212

	// prepare the sound buffers and commands
	dtox80(&doubleFreq, &extFreq);
	for (i = 0; i < kTotalSoundBuffers; i++)
	{
		// allocate and clear the sound buffer
		sSoundBuffer[i] = malloc(sizeof(ExtSoundHeader) + sSoundBufferBytes);
		// R. Nabet 0001212 : added error handling
		if (sSoundBuffer[i] == NULL)
		{
			while (--i >= 0)
				free(sSoundBuffer[i]);
			return true;
		}
		memset(sSoundBuffer[i], 0, sizeof(ExtSoundHeader) + sSoundBufferBytes);
		
		// intialize the buffer structures
		sSoundBuffer[i]->numChannels 	= 1 << sSoundIsStereo;
		sSoundBuffer[i]->sampleRate 	= (UInt32)gPrefs.sampleRate << 16;
		sSoundBuffer[i]->encode 		= extSH;
		sSoundBuffer[i]->numFrames 		= sSoundBufferFrames;
		sSoundBuffer[i]->AIFFSampleRate = extFreq;
		sSoundBuffer[i]->sampleSize 	= 16;
		
		// initialize the sound commands
		sSoundBufferCmd[i].cmd 			= bufferCmd;
		sSoundBufferCmd[i].param2 		= (SInt32)sSoundBuffer[i];
		sSoundCallbackCmd[i].cmd 		= callBackCmd;
		sSoundCallbackCmd[i].param1 	= (i + kQueuedSoundBuffers) % kTotalSoundBuffers;
	}
	
	// start stuff playing
	for (i = 0; i < kQueuedSoundBuffers; i++)
	{
		// R. Nabet 001212 : added error handling
		err = SndDoCommand(sSoundChannel, &sSoundBufferCmd[i], true);
		if (err != noErr)
			return true;
		err = SndDoCommand(sSoundChannel, &sSoundCallbackCmd[i], true);
		if (err != noErr)
			return true;
	}
	// R. Nabet 001212 : fixed return value
	return false;
}


//===============================================================================
//	FreeSoundBuffers
//
//	Free the sound buffers and the sound channel.
//===============================================================================

static void FreeSoundBuffers(void)
{
	UInt32 i;
	
	// free the buffers
	for (i = 0; i < kTotalSoundBuffers; i++)
	{
		if (sSoundBuffer[i] != NULL)
			free(sSoundBuffer[i]);
		sSoundBuffer[i] = NULL;
	}
}


//===============================================================================
//	SoundCallback
//
//	Callback command to stream data.
//===============================================================================

static pascal void SoundCallback(SndChannelPtr inChannel, SndCommand *inCommand)
{
	UInt32		bufferIndex = inCommand->param1;

	// fill the sound buffer
	FillSoundBuffer((SInt16 *)sSoundBuffer[bufferIndex]->sampleArea);

	// queue up the commands
	if (!sStopSoundPlaying)
	{
		SndDoCommand(sSoundChannel, &sSoundBufferCmd[bufferIndex], true);
		SndDoCommand(sSoundChannel, &sSoundCallbackCmd[bufferIndex], true);
	}
}

#endif
