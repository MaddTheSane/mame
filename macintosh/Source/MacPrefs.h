/*************************************************************************

	MacPrefs.h
	
	This file holds the structure which we stick all our prefs in.
	
*************************************************************************/

#ifndef MacPrefs_h
#define MacPrefs_h

#pragma once

extern CFStringRef gAppPrefID;

typedef struct
{
	Boolean		playSound;
	Boolean		speedThrottle;
	Boolean		interlace;
	Boolean		errorLog;
	Boolean		hideDesktop;
	Boolean		autoSkip;				// enable automatic frameskipping?
	Boolean		qualityAudio;			// Play 22kHz or 44kHz audio
	Boolean		cheat;
	Boolean		antialias;
	Boolean		translucency;
	Boolean		noGameInfo;
	Boolean		noDisclaimer;
	
	short		windowScale;
	short		frameSkip;
	short		attenuation;
	
	int			sampleRate;				// Machine->sample_rate
	int			beamWidth;				// used for vectors
	int			flicker;				// used for vectors
	OSType		textCreator;			// default creator for text files

	Point		windowLocation;
	Point		debugWindowLocation;
	
	UInt32		plugin;
	UInt8		standardRotation;
	UInt8		rotateLeft;
	UInt8		rotateRight;
	UInt8		flipX;
	UInt8		flipY;
	UInt8		noRotation;

	Boolean		artworkAll;
	Boolean		artworkBackdrop;
	Boolean		artworkOverlay;
	Boolean		artworkBezel;
} PrefsRec;	

extern PrefsRec gPrefs;

typedef struct
{
	//
	// ¥¥¥¥¥¥ÊFront-end
	//
	Str255		font;
	Str255		driverSelected;
	FSSpec		groupFilespec;

	short		infoMenu;
	UInt16		fontSize;
	StyleParameter fontFace;
	UInt16		grouping;
	UInt16		groupFlags;

	Point		dialogLocation;
	Point		dialogSize;

	Boolean		fe_displayFileNames;
	Boolean		fe_noItalics;
	Boolean		fe_listUseColors;
	Boolean		fe_listUseBrokenColors;
	Boolean		fe_listSortThe;
	Boolean		fe_listNoParentShots;

	// options for ROM and sample audits -- matches tAuditOptions structure
	short		audSortAlpha;
	Boolean		audProblemsOnly;
	Boolean		audListMissing;
	Boolean		audSuppressWarnings;
	Boolean		audIgnoreCHDs;
	Str255		audFilter;
	
	// options for other text reports -- matches tReportOptions structure
	short		repType;
	Boolean		repExcludeClones;
	Boolean		repExcludeParents;

#ifdef MESS
	// options for MacMESS
	AliasHandle		images[IO_COUNT*MAX_DEV_INSTANCES];
#endif
} FEPrefsRec;
extern FEPrefsRec gFEPrefs;

// Function Prototypes
void SetPrefID_App (void);
void SetPrefID_FE (void);

void GetPrefAsCFString (CFStringRef *outValue, CFStringRef inKey, CFStringRef inDefault);
void GetPrefAsBoolean (Boolean *outValue, CFStringRef inKey, Boolean inDefault);
void GetPrefAsInt (int *outValue, CFStringRef inKey, int inDefault);
void GetPrefAsShort (SInt16 *outValue, CFStringRef inKey, SInt16 inDefault);
void GetPrefAsUShort (UInt16 *outValue, CFStringRef inKey, UInt16 inDefault);
void GetPrefAsUInt8 (UInt8 *outValue, CFStringRef inKey, UInt8 inDefault);
void GetPrefAsUInt32 (UInt32 *outValue, CFStringRef inKey, UInt32 inDefault);
void GetPrefAsFSSpec (FSSpecPtr outValue, CFStringRef inKey);
void GetPrefAsAlias (AliasHandle *outValue, CFStringRef inKey);
void GetPrefAsCFURL (CFURLRef *outValue, CFStringRef inKey);
void GetPrefAsStr255 (StringPtr outValue, CFStringRef inKey);
void SetPrefAsCFString (CFStringRef inValue, CFStringRef inKey);
void SetPrefAsInt (int inValue, CFStringRef inKey);
void SetPrefAsBoolean (Boolean inValue, CFStringRef inKey);
void SetPrefAsFSSpec (FSSpecPtr inValue, CFStringRef inKey);
void SetPrefAsAlias (AliasHandle inValue, CFStringRef inKey);
void SetPrefAsStr255 (StringPtr inValue, CFStringRef inKey);

void LoadPrefs		(void);
void SavePrefs		(void);

#endif
