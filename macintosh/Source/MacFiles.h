/*##########################################################################

	macfiles.h

	Routines for handling files, ROMs and romsets with MacMAME (and MacMESS).

##########################################################################*/

#pragma once

#ifndef MacFiles_h
#define MacFiles_h

#define kMacMaxPath	1024		// Assumed total size in chars of the maximum path length

typedef Boolean (*DisplayFileCallback)(const FSSpec *inSpec, OSType inType, OSType inCreator);

enum
{
	// Here are our Mac-specific MAME folder types.
	MAC_FILETYPE_MAC_FILES_OLD = FILETYPE_end + 1,		// The older "MacMAME" folder meta-type
	MAC_FILETYPE_MAC_FILES,							// The actual "MacMAME User Data" folder meta-type
	MAC_FILETYPE_OPENGL_OVERLAY,					// "OpenGL Overlays"
	MAC_FILETYPE_SUPPORT,							// "Misc Support Files"

	// еее FRONTEND These types are specific to the current Mac front-end.
	MAC_FILETYPE_REPORTS,							// "Reports" - used as a default location for saving
	MAC_FILETYPE_CATEGORIES,						// "Categories"
	MAC_FILETYPE_TITLE_SCREENSHOTS,					// "Title Screenshots"
	MAC_FILETYPE_CABINET_ARTWORK,					// "Cabinet Art"
	MAC_FILETYPE_FLYER_ARTWORK,						// "Game Flyers"
	MAC_FILETYPE_MARQUEE_ARTWORK,					// "Cabinet Marquees"
	MAC_FILETYPE_CONTROL_PANEL_ARTWORK,				// "Control Panels"
	MAC_FILETYPE_end
};

/*##########################################################################
	FUNCTION PROTOTYPES
##########################################################################*/

#ifdef __cplusplus
extern "C" {
#endif

int					ConvertCStringToUnicode (const char *inString, UniChar *outString);
void				GetFileTypeForMAMEFolder (int inMAMEFolderType, OSType *outCreator, OSType *outType);
CFURLRef			CopyCFURLForMAMEFolder (int inMAMEFolderType);
OSStatus			GetCStringForMAMEFolder (int inMAMEFolderType, char *outPath);
OSStatus			GetFSRefForMAMEFolder (int inMAMEFolderType, FSRef *outFSRef);
OSStatus			GetFSSpecForMAMEFolder (int inMAMEFolderType, FSSpec *outFSSpec);
void				RenameOldMacMAMEFolder (void);
Boolean 			IsFolder (const FSSpec *spec);
OSStatus			GetFullPathFromSpec (const FSSpec *spec, UInt8 *fullpath, int maxBufLen);

OSStatus 			GetPartialPathSpec (const FSSpec *base, const char *partpath, FSSpec *spec);
void				GetFilenameFromURL (CFURLRef inRef, char *outName, int inNameLength);
void				SetCreatorForPath (const char *inPath, OSType inCreator, OSType inType);
void				GetHashAsUINT32 (const char *inHash, UINT32 *outCrc);

Boolean				_NavGetOneFile (CFStringRef inMessage, int inMAMEFolderType, CFURLRef *outURL);
Boolean 			_NavPutOneFile (CFStringRef inMessage, CFStringRef inDefaultFileName, int inMAMEFolderType, CFURLRef *outURL, FSSpec *outSpec);

#ifdef __cplusplus
}
#endif

#endif