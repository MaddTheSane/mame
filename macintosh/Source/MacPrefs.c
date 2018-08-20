#include "MacPrefs.h"
#include "MacReports.h" // ¥¥¥¥¥¥ FRONTEND

PrefsRec gPrefs;
FEPrefsRec gFEPrefs;

CFStringRef gAppPrefID;
CFStringRef sFrontEndPrefID;

CFStringRef sPrefID;
Boolean sPrefsInit = false;

void LoadPrefs_FE (void);
void SavePrefs_FE (void);

void InitPrefs (void)
{
#ifdef MESS
	gAppPrefID = CFSTR("org.macmess.macmess"); // kCFPreferencesCurrentApplication
	sFrontEndPrefID = CFSTR("org.macmess.frontend");
#else
	gAppPrefID = CFSTR("org.macmame.macmame"); // kCFPreferencesCurrentApplication
	sFrontEndPrefID = CFSTR("org.macmame.frontend");
#endif
	
	sPrefsInit = true;
}

void SetPrefID_App (void)
{
	sPrefID = gAppPrefID;
}

void SetPrefID_FE (void)
{
	sPrefID = sFrontEndPrefID;
}

void GetPrefAsCFString (CFStringRef *outValue, CFStringRef inKey, CFStringRef inDefault)
{
	// The caller is responsible for calling CFRelease() on the resulting string
	*outValue = CFPreferencesCopyAppValue (inKey, sPrefID);
	if (*outValue)
	{
		if (CFGetTypeID(*outValue) != CFStringGetTypeID())
			*outValue = NULL;
	}
	if (!*outValue)
	{
		*outValue = CFStringCreateCopy (kCFAllocatorDefault, inDefault);
	}
}

void GetPrefAsBoolean (Boolean *outValue, CFStringRef inKey, Boolean inDefault)
{
	Boolean keyFound;
	*outValue = CFPreferencesGetAppBooleanValue (inKey, sPrefID, &keyFound);
	if (!keyFound) *outValue = inDefault;
}

void GetPrefAsInt (int *outValue, CFStringRef inKey, int inDefault)
{
	Boolean keyFound;
	*outValue = CFPreferencesGetAppIntegerValue (inKey, sPrefID, &keyFound);
	if (!keyFound) *outValue = inDefault;
}

void GetPrefAsShort (SInt16 *outValue, CFStringRef inKey, SInt16 inDefault)
{
	Boolean keyFound;
	*outValue = CFPreferencesGetAppIntegerValue (inKey, sPrefID, &keyFound);
	if (!keyFound) *outValue = inDefault;
}

void GetPrefAsUShort (UInt16 *outValue, CFStringRef inKey, UInt16 inDefault)
{
	Boolean keyFound;
	*outValue = CFPreferencesGetAppIntegerValue (inKey, sPrefID, &keyFound);
	if (!keyFound) *outValue = inDefault;
}

void GetPrefAsUInt8 (UInt8 *outValue, CFStringRef inKey, UInt8 inDefault)
{
	Boolean keyFound;
	*outValue = CFPreferencesGetAppIntegerValue (inKey, sPrefID, &keyFound);
	if (!keyFound) *outValue = inDefault;
}

void GetPrefAsUInt32 (UInt32 *outValue, CFStringRef inKey, UInt32 inDefault)
{
	Boolean keyFound;
	*outValue = CFPreferencesGetAppIntegerValue (inKey, sPrefID, &keyFound);
	if (!keyFound) *outValue = inDefault;
}

void GetPrefAsFSSpec (FSSpecPtr outValue, CFStringRef inKey)
{
	CFPropertyListRef tempData;

	memset (outValue, 0, sizeof (FSSpec));

	tempData = CFPreferencesCopyAppValue (inKey, sPrefID);

	if (tempData != NULL)
	{
		if (CFGetTypeID (tempData) == CFDataGetTypeID())
		{
			Size dataSize = CFDataGetLength((CFDataRef) tempData);
			CFDataGetBytes (tempData, CFRangeMake (0, dataSize), (UInt8 *) outValue);
		}
		else
		{
            // The data is not what we expected so remove it from the prefs.
            // This could happen if the structures of the pref data changes.
            CFPreferencesSetAppValue (inKey, NULL, sPrefID);
		}
		CFRelease (tempData);
	}
}

void GetPrefAsAlias (AliasHandle *outValue, CFStringRef inKey)
{
	CFPropertyListRef tempData;
	
	tempData = CFPreferencesCopyAppValue (inKey, sPrefID);
	if (tempData != NULL)
	{
		if (CFGetTypeID (tempData) == CFDataGetTypeID())
		{
			Size dataSize = CFDataGetLength((CFDataRef) tempData);
			*outValue = (AliasHandle) NewHandle (dataSize);
			HLock ((Handle)*outValue);
			
			if (*outValue != NULL)
			{
				CFDataGetBytes (tempData, CFRangeMake (0, dataSize), (UInt8 *) **outValue);
			}
			HUnlock ((Handle)*outValue);
		}
		else
		{
            // The data is not what we expected so remove it from the prefs.
            // This could happen if the structures of the pref data changes.
            CFPreferencesSetAppValue (inKey, NULL, sPrefID);
			*outValue = NULL;
		}
		CFRelease (tempData);
	}
	else
		*outValue = NULL;
}


void GetPrefAsStr255 (StringPtr outValue, CFStringRef inKey)
{
	CFPropertyListRef tempData;

	memset (outValue, 0, sizeof (Str255));

	tempData = CFPreferencesCopyAppValue (inKey, sPrefID);

	if (tempData != NULL)
	{
		if (CFGetTypeID (tempData) == CFDataGetTypeID())
		{
			Size dataSize = CFDataGetLength((CFDataRef) tempData);
			CFDataGetBytes (tempData, CFRangeMake (0, dataSize), (UInt8 *) outValue);
		}
		else
		{
            // The data is not what we expected so remove it from the prefs.
            // This could happen if the structures of the pref data changes.
            CFPreferencesSetAppValue (inKey, NULL, sPrefID);
		}
		CFRelease (tempData);
	}
}

void SetPrefAsCFString (CFStringRef inValue, CFStringRef inKey)
{
	if (inValue)
		CFPreferencesSetAppValue (inKey, inValue, sPrefID);
	else
		CFPreferencesSetAppValue (inKey, CFSTR(""), sPrefID);
}

void SetPrefAsInt (int inValue, CFStringRef inKey)
{
	CFNumberRef tempNumber;
	
	tempNumber = CFNumberCreate (NULL, kCFNumberIntType, &inValue);
	if (tempNumber)
	{
		CFPreferencesSetAppValue (inKey, tempNumber, sPrefID);
		CFRelease (tempNumber);
	}
}

void SetPrefAsBoolean (Boolean inValue, CFStringRef inKey)
{
	if (inValue)
		CFPreferencesSetAppValue (inKey, kCFBooleanTrue, sPrefID);
	else
		CFPreferencesSetAppValue (inKey, kCFBooleanFalse, sPrefID);
}

void SetPrefAsFSSpec (FSSpecPtr inValue, CFStringRef inKey)
{
	CFPropertyListRef tempData;

	tempData = CFDataCreate (NULL, (const UInt8*) (inValue), sizeof (FSSpec));

	if (tempData != NULL)
	{
		CFPreferencesSetAppValue (inKey, tempData, sPrefID);
	}
}

void SetPrefAsAlias (AliasHandle inValue, CFStringRef inKey)
{
	CFPropertyListRef tempData;
	
	if (inValue == NULL)
		CFPreferencesSetAppValue (inKey, NULL, sPrefID);
	else
	{
		HLock ((Handle) inValue);
		tempData = CFDataCreate (NULL, (const UInt8*)*inValue, GetHandleSize ((Handle) inValue));
		HUnlock ((Handle) inValue);

		if (tempData != NULL)
		{
			CFPreferencesSetAppValue (inKey, tempData, sPrefID);
		}
	}
}

void SetPrefAsStr255 (StringPtr inValue, CFStringRef inKey)
{
	CFPropertyListRef tempData;

	tempData = CFDataCreate (NULL, (const UInt8*) (inValue), sizeof (Str255));

	if (tempData != NULL)
	{
		CFPreferencesSetAppValue (inKey, tempData, sPrefID);
	}
}


void LoadPrefs (void)
{
	if (!sPrefsInit) InitPrefs ();
	
	memset (&gPrefs, 0, sizeof (gPrefs));
	
	SetPrefID_App();
	
	//
	// Video-related prefs
	//
	GetPrefAsBoolean(&gPrefs.interlace, CFSTR("interlacedBlit"), false);
	GetPrefAsBoolean(&gPrefs.antialias, CFSTR("vectorAntiAlias"), true);
	GetPrefAsBoolean(&gPrefs.translucency, CFSTR("vectorTranslucency"), false);
	GetPrefAsBoolean(&gPrefs.hideDesktop, CFSTR("hideDesktop"), true);
	GetPrefAsBoolean(&gPrefs.autoSkip, CFSTR("autoSkip"), true);
	GetPrefAsShort(&gPrefs.frameSkip, CFSTR("manualFrameSkip"), 0);
	GetPrefAsShort(&gPrefs.windowScale, CFSTR("windowScale"), 2);
	GetPrefAsInt(&gPrefs.flicker, CFSTR("vectorFlicker"), 0);
	GetPrefAsInt(&gPrefs.beamWidth, CFSTR("vectorBeamWidth"), 0x100);

	GetPrefAsShort(&gPrefs.windowLocation.h, CFSTR("window_x"), 24);
	GetPrefAsShort(&gPrefs.windowLocation.v, CFSTR("window_y"), 40);
	GetPrefAsShort(&gPrefs.debugWindowLocation.h, CFSTR("debugWindow_x"), 50);
	GetPrefAsShort(&gPrefs.debugWindowLocation.v, CFSTR("debugWindow_y"), 50);
	
	GetPrefAsBoolean(&gPrefs.standardRotation, CFSTR("standardRotation"), true);
	GetPrefAsBoolean(&gPrefs.rotateLeft, CFSTR("rotateLeft"), false);
	GetPrefAsBoolean(&gPrefs.rotateRight, CFSTR("rotateRight"), false);
	GetPrefAsBoolean(&gPrefs.flipX, CFSTR("flipX"), false);
	GetPrefAsBoolean(&gPrefs.flipY, CFSTR("flipY"), false);
	GetPrefAsBoolean(&gPrefs.noRotation, CFSTR("noRotation"), false);

	GetPrefAsUInt32(&gPrefs.plugin, CFSTR("videoPlugin"), 'OGL ');

	GetPrefAsBoolean(&gPrefs.artworkAll, CFSTR("artworkAll"), false);
	GetPrefAsBoolean(&gPrefs.artworkBackdrop, CFSTR("artworkBackdrop"), false);
	GetPrefAsBoolean(&gPrefs.artworkOverlay, CFSTR("artworkOverlay"), false);
	GetPrefAsBoolean(&gPrefs.artworkBezel, CFSTR("artworkBezel"), false);

	//
	// Audio-related prefs
	//
	GetPrefAsBoolean(&gPrefs.playSound, CFSTR("playSound"), true);
	GetPrefAsBoolean(&gPrefs.qualityAudio, CFSTR("qualityAudio"), true);
	GetPrefAsShort(&gPrefs.attenuation, CFSTR("attenuation"), 0);
	GetPrefAsInt(&gPrefs.sampleRate, CFSTR("audioSampleRate"), 44100);
	
	//
	// Misc MAME core prefs
	//
	GetPrefAsBoolean(&gPrefs.speedThrottle, CFSTR("speedThrottle"), true);
	GetPrefAsBoolean(&gPrefs.errorLog, CFSTR("errorLog"), false);
	GetPrefAsBoolean(&gPrefs.cheat, CFSTR("enableCheats"), false);
	GetPrefAsBoolean(&gPrefs.noGameInfo, CFSTR("noGameInfo"), false);
	GetPrefAsBoolean(&gPrefs.noDisclaimer, CFSTR("noDisclaimer"), false);

	//
	// Misc MacMAME core prefs
	//
	GetPrefAsUInt32(&gPrefs.textCreator, CFSTR("textFileCreator"), 'ttxt');

	// ¥¥¥¥¥¥ FRONTEND
	LoadPrefs_FE();
}

void LoadPrefs_FE (void)
{
	SetPrefID_FE();
	
	//
	// Front-end related prefs
	//
	GetPrefAsShort(&gFEPrefs.infoMenu, CFSTR("fe_infoMenu"), 1);
	GetPrefAsUShort(&gFEPrefs.fontSize, CFSTR("fe_listFontSize"), 12);
	GetPrefAsShort(&gFEPrefs.fontFace, CFSTR("fe_listFontFace"), normal);
	GetPrefAsUShort(&gFEPrefs.grouping, CFSTR("fe_groupSorting"), 0);
	GetPrefAsUShort(&gFEPrefs.groupFlags, CFSTR("fe_groupFlags"), 0);
	GetPrefAsFSSpec(&gFEPrefs.groupFilespec, CFSTR("fe_groupFilespec"));
	GetPrefAsStr255(gFEPrefs.font, CFSTR("fe_font"));
	GetPrefAsStr255(gFEPrefs.driverSelected, CFSTR("fe_driverSelected"));

	GetPrefAsShort(&gFEPrefs.dialogLocation.h, CFSTR("fe_dialog_x"), 0);
	GetPrefAsShort(&gFEPrefs.dialogLocation.v, CFSTR("fe_dialog_y"), 0);
	GetPrefAsShort(&gFEPrefs.dialogSize.h, CFSTR("fe_dialog_width"), 0);
	GetPrefAsShort(&gFEPrefs.dialogSize.v, CFSTR("fe_dialog_height"), 0);

	GetPrefAsBoolean(&gFEPrefs.fe_displayFileNames, CFSTR("fe_displayFileNames"), false);
	GetPrefAsBoolean(&gFEPrefs.fe_noItalics, CFSTR("fe_noItalics"), false);
	GetPrefAsBoolean(&gFEPrefs.fe_listUseColors, CFSTR("fe_listUseColors"), false);
	GetPrefAsBoolean(&gFEPrefs.fe_listUseBrokenColors, CFSTR("fe_listUseBrokenColors"), false);
	GetPrefAsBoolean(&gFEPrefs.fe_listSortThe, CFSTR("fe_listSortThe"), false);
	GetPrefAsBoolean(&gFEPrefs.fe_listNoParentShots, CFSTR("fe_listNoParentShots"), false);
	
	GetPrefAsShort(&gFEPrefs.audSortAlpha, CFSTR("fe_sortAlpha"), -1);
	GetPrefAsBoolean(&gFEPrefs.audProblemsOnly, CFSTR("fe_auditProblemsOnly"), true);
	GetPrefAsBoolean(&gFEPrefs.audListMissing, CFSTR("fe_auditListMissing"), false);
	GetPrefAsBoolean(&gFEPrefs.audSuppressWarnings, CFSTR("fe_auditSuppressWarnings"), false);
	GetPrefAsBoolean(&gFEPrefs.audIgnoreCHDs, CFSTR("fe_auditIgnoreCHDs"), false);
	GetPrefAsStr255(gFEPrefs.audFilter, CFSTR("fe_auditFilter"));

	GetPrefAsShort(&gFEPrefs.repType, CFSTR("fe_reportType"), kReportAllGames);
	GetPrefAsBoolean(&gFEPrefs.repExcludeClones, CFSTR("fe_reportExcludeClones"), false);
	GetPrefAsBoolean(&gFEPrefs.repExcludeParents, CFSTR("fe_reportExcludeParents"), false);

#ifdef MESS
	{
		int i;
		CFStringRef inKey;

		for( i = 0; i<(IO_COUNT*MAX_DEV_INSTANCES); i++ )
		{
			inKey = CFStringCreateWithFormat( kCFAllocatorDefault, NULL, CFSTR("fe_MESS_image_%d"), i );
			GetPrefAsAlias( &(gFEPrefs.images[i]), inKey);
			CFRelease(inKey);
		}
	}
#endif
}

void SavePrefs (void)
{
	SetPrefID_App ();

	//
	// Video-related prefs
	//
	SetPrefAsBoolean(gPrefs.interlace,CFSTR("interlacedBlit"));
	SetPrefAsBoolean(gPrefs.antialias,CFSTR("vectorAntiAlias"));
	SetPrefAsBoolean(gPrefs.translucency,CFSTR("vectorTranslucency"));
	SetPrefAsBoolean(gPrefs.hideDesktop,CFSTR("hideDesktop"));
	SetPrefAsBoolean(gPrefs.autoSkip,CFSTR("autoSkip"));
	SetPrefAsInt(gPrefs.frameSkip,CFSTR("manualFrameSkip"));
	SetPrefAsInt(gPrefs.windowScale,CFSTR("windowScale"));
	SetPrefAsInt(gPrefs.flicker,CFSTR("vectorFlicker"));
	SetPrefAsInt(gPrefs.beamWidth,CFSTR("vectorBeamWidth"));

	SetPrefAsInt(gPrefs.windowLocation.h,CFSTR("window_x"));
	SetPrefAsInt(gPrefs.windowLocation.v,CFSTR("window_y"));
	SetPrefAsInt(gPrefs.debugWindowLocation.h,CFSTR("debugWindow_x"));
	SetPrefAsInt(gPrefs.debugWindowLocation.v,CFSTR("debugWindow_y"));
	
	SetPrefAsBoolean(gPrefs.standardRotation,CFSTR("standardRotation"));
	SetPrefAsBoolean(gPrefs.rotateLeft,CFSTR("rotateLeft"));
	SetPrefAsBoolean(gPrefs.rotateRight,CFSTR("rotateRight"));
	SetPrefAsBoolean(gPrefs.flipX,CFSTR("flipX"));
	SetPrefAsBoolean(gPrefs.flipY,CFSTR("flipY"));
	SetPrefAsBoolean(gPrefs.noRotation,CFSTR("noRotation"));

	SetPrefAsInt(gPrefs.plugin,CFSTR("videoPlugin"));

	SetPrefAsBoolean(gPrefs.artworkAll,CFSTR("artworkAll"));
	SetPrefAsBoolean(gPrefs.artworkBackdrop,CFSTR("artworkBackdrop"));
	SetPrefAsBoolean(gPrefs.artworkOverlay,CFSTR("artworkOverlay"));
	SetPrefAsBoolean(gPrefs.artworkBezel,CFSTR("artworkBezel"));

	//
	// Audio-related prefs
	//
	SetPrefAsBoolean(gPrefs.playSound,CFSTR("playSound"));
	SetPrefAsBoolean(gPrefs.qualityAudio,CFSTR("qualityAudio"));
	SetPrefAsInt(gPrefs.attenuation,CFSTR("attenuation"));
	SetPrefAsInt(gPrefs.sampleRate,CFSTR("audioSampleRate"));
	
	//
	// Misc MAME core prefs
	//
	SetPrefAsBoolean(gPrefs.speedThrottle,CFSTR("speedThrottle"));
	SetPrefAsBoolean(gPrefs.errorLog,CFSTR("errorLog"));
	SetPrefAsBoolean(gPrefs.cheat,CFSTR("enableCheats"));
	SetPrefAsBoolean(gPrefs.noGameInfo,CFSTR("noGameInfo"));
	SetPrefAsBoolean(gPrefs.noDisclaimer,CFSTR("noDisclaimer"));

	//
	// Misc MacMAME core prefs
	//
	SetPrefAsInt(gPrefs.textCreator,CFSTR("textFileCreator"));

	CFPreferencesAppSynchronize (CFSTR("org.macmame.macmame"));

	// ¥¥¥¥¥¥ FRONTEND
	SavePrefs_FE();
}

void SavePrefs_FE (void)
{
	//
	// Front-end related prefs
	//
	SetPrefID_FE ();

	SetPrefAsInt(gFEPrefs.infoMenu,CFSTR("fe_infoMenu"));
	SetPrefAsInt(gFEPrefs.fontSize,CFSTR("fe_listFontSize"));
	SetPrefAsInt(gFEPrefs.fontFace,CFSTR("fe_listFontFace"));
	SetPrefAsInt(gFEPrefs.grouping,CFSTR("fe_groupSorting"));
	SetPrefAsInt(gFEPrefs.groupFlags,CFSTR("fe_groupFlags"));
	SetPrefAsFSSpec(&gFEPrefs.groupFilespec,CFSTR("fe_groupFilespec"));
	SetPrefAsStr255(gFEPrefs.font, CFSTR("fe_font"));
	SetPrefAsStr255(gFEPrefs.driverSelected, CFSTR("fe_driverSelected"));

	SetPrefAsInt(gFEPrefs.dialogLocation.h,CFSTR("fe_dialog_x"));
	SetPrefAsInt(gFEPrefs.dialogLocation.v,CFSTR("fe_dialog_y"));
	SetPrefAsInt(gFEPrefs.dialogSize.h,CFSTR("fe_dialog_width"));
	SetPrefAsInt(gFEPrefs.dialogSize.v,CFSTR("fe_dialog_height"));

	SetPrefAsBoolean(gFEPrefs.fe_displayFileNames,CFSTR("fe_displayFileNames"));
	SetPrefAsBoolean(gFEPrefs.fe_noItalics,CFSTR("fe_noItalics"));
	SetPrefAsBoolean(gFEPrefs.fe_listUseColors,CFSTR("fe_listUseColors"));
	SetPrefAsBoolean(gFEPrefs.fe_listUseBrokenColors,CFSTR("fe_listUseBrokenColors"));
	SetPrefAsBoolean(gFEPrefs.fe_listSortThe,CFSTR("fe_listSortThe"));
	SetPrefAsBoolean(gFEPrefs.fe_listNoParentShots,CFSTR("fe_listNoParentShots"));
	
	SetPrefAsInt(gFEPrefs.audSortAlpha,CFSTR("fe_sortAlpha"));
	SetPrefAsBoolean(gFEPrefs.audProblemsOnly,CFSTR("fe_auditProblemsOnly"));
	SetPrefAsBoolean(gFEPrefs.audListMissing,CFSTR("fe_auditListMissing"));
	SetPrefAsBoolean(gFEPrefs.audSuppressWarnings,CFSTR("fe_auditSuppressWarnings"));
	SetPrefAsBoolean(gFEPrefs.audIgnoreCHDs,CFSTR("fe_auditIgnoreCHDs"));
	SetPrefAsStr255(gFEPrefs.audFilter, CFSTR("fe_auditFilter"));

	SetPrefAsInt(gFEPrefs.repType,CFSTR("fe_reportType"));
	SetPrefAsBoolean(gFEPrefs.repExcludeClones,CFSTR("fe_reportExcludeClones"));
	SetPrefAsBoolean(gFEPrefs.repExcludeParents,CFSTR("fe_reportExcludeParents"));
	
#ifdef MESS
	{
		int i;
		CFStringRef inKey;
		
		for( i = 0; i<IO_COUNT*MAX_DEV_INSTANCES; i++ )
		{
			inKey = CFStringCreateWithFormat( kCFAllocatorDefault, NULL, CFSTR("fe_MESS_image_%d"), i );
			SetPrefAsAlias( gFEPrefs.images[i], inKey );
			CFRelease(inKey);
		}
	}
#endif

	CFPreferencesAppSynchronize (CFSTR("org.macmame.frontend"));
}