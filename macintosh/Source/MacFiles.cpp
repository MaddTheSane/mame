//-----------------------------------------------------------------------------
// MacFiles.cpp
//-----------------------------------------------------------------------------
// Routines for handling all of MAME's platform-specific file needs.
// Rewritten almost entirely from the ground up by Brad Oliver for MacMAME 0.86
//
// TODO:
//
// * Possibly eliminate GetFilenameFromURL
// * Try to eliminate the remaining FSSpec routines.
// * Try to optimize file path searches, particularly for ROM audits.
//

#include <Carbon/Carbon.h>

#include <cstdio>
#include <vector>

using std::vector;

extern "C"
{
	#include "driver.h"
	#include "mac.h"
	#include "macstrings.h"
	#include "macreports.h"
}

static vector<FSRefPtr> sMAMESubfolderList[MAC_FILETYPE_end];
static bool sMAMESubfolderSearched[MAC_FILETYPE_end];

#pragma mark ¥ Mac helper routines

//-----------------------------------------------------------------------------
// ConvertCFStringToUnicode
//-----------------------------------------------------------------------------
// Given a CFString, convert to a UniChar, suitable for FSRef file calls.

static int ConvertCFStringToUnicode (const CFStringRef inString, UniChar *outString)
{
	*outString = 0;
	
	assert(inString);
	assert(outString);
	if (!inString || !outString) return 0;
	
	CFIndex length = CFStringGetBytes (inString, CFRangeMake(0, CFStringGetLength(inString)), kCFStringEncodingUnicode, 0, false, (UInt8*)outString, 255, NULL);
	return length;
}

//-----------------------------------------------------------------------------
// ConvertCStringToUnicode
//-----------------------------------------------------------------------------
// Given a c-string, convert to a UniChar, suitable for FSRef file calls.

int ConvertCStringToUnicode (const char *inString, UniChar *outString)
{
	*outString = 0;
	
	assert(inString);
	assert(outString);
	if (!inString || !outString) return 0;
	
	CFStringRef stringRef = CFStringCreateWithCString (kCFAllocatorDefault, inString, kCFStringEncodingUTF8);
	if (!stringRef) return 0;
	
	CFIndex length = CFStringGetBytes (stringRef, CFRangeMake(0, strlen(inString)), kCFStringEncodingUnicode, 0, false, (UInt8*)outString, 255, NULL);
	CFRelease (stringRef);
	return length;
}

//-----------------------------------------------------------------------------
// CopyFolderStringForFolderType
//-----------------------------------------------------------------------------
// Given a MAME folder/file type, creates a CFString containing the equivalent
// support folder name that MacMAME uses. The caller is responsible for
// releasing the CFStringRef.

CFStringRef CopyFolderStringForFolderType (int inMAMEFolderType)
{
	CFStringRef folderName = NULL;
	
	switch (inMAMEFolderType)
	{
//		case FILETYPE_RAW:

		case FILETYPE_ROM:
			folderName = CFCopyLocalizedString(CFSTR("ROMSFolder"), NULL);
			break;
		case FILETYPE_IMAGE:
			folderName = CFCopyLocalizedString(CFSTR("HardDiskFolder"), NULL);
			break;
		case FILETYPE_IMAGE_DIFF:
			folderName = CFCopyLocalizedString(CFSTR("CHDDiffFolder"), NULL);
			break;
		case FILETYPE_SAMPLE:
			folderName = CFCopyLocalizedString(CFSTR("SamplesFolder"), NULL);
			break;
		case FILETYPE_ARTWORK:
			folderName = CFCopyLocalizedString(CFSTR("ArtworkFolder"), NULL);
			break;
		case FILETYPE_NVRAM:
			folderName = CFCopyLocalizedString(CFSTR("NVRAMFolder"), NULL);
			break;
		case FILETYPE_HIGHSCORE:
			folderName = CFCopyLocalizedString(CFSTR("HighScoresFolder"), NULL);
			break;
		case FILETYPE_HIGHSCORE_DB:
			folderName = CFCopyLocalizedString(CFSTR("MiscSupportFolder"), NULL);
			break;
		case FILETYPE_CONFIG:
			folderName = CFCopyLocalizedString(CFSTR("ConfigFolder"), NULL);
			break;
		case FILETYPE_INPUTLOG:
			folderName = CFCopyLocalizedString(CFSTR("INPFolder"), NULL);
			break;
		case FILETYPE_STATE:
			folderName = CFCopyLocalizedString(CFSTR("SaveStateFolder"), NULL);
			break;
		case FILETYPE_MEMCARD:
			folderName = CFCopyLocalizedString(CFSTR("HighScoresFolder"), NULL);
			break;
		case FILETYPE_SCREENSHOT:
			folderName = CFCopyLocalizedString(CFSTR("ScreenshotFolder"), NULL);
			break;
		case FILETYPE_MOVIE:
			folderName = CFCopyLocalizedString(CFSTR("MovieFolder"), NULL);
			break;
		case FILETYPE_HISTORY:
			folderName = CFCopyLocalizedString(CFSTR("MiscSupportFolder"), NULL);
			break;
		case FILETYPE_CHEAT:
			folderName = CFCopyLocalizedString(CFSTR("MiscSupportFolder"), NULL);
			break;
		case FILETYPE_LANGUAGE:
			folderName = CFCopyLocalizedString(CFSTR("MiscSupportFolder"), NULL);
			break;

//		case FILETYPE_CTRLR:
//		case FILETYPE_INI:
#ifdef MESS
		case FILETYPE_HASH:
			folderName = CFCopyLocalizedString(CFSTR("CRCFolder"), NULL);
			break;
#endif
		case MAC_FILETYPE_MAC_FILES_OLD:
			folderName = CFCopyLocalizedString(CFSTR("MacMAMEFolderOld"), NULL);
			break;
		case MAC_FILETYPE_MAC_FILES:
			folderName = CFCopyLocalizedString(CFSTR("MacMAMEFolder"), NULL);
			break;
		case MAC_FILETYPE_OPENGL_OVERLAY:
			folderName = CFCopyLocalizedString(CFSTR("OpenGLOverlayFolder"), NULL);
			break;
		case MAC_FILETYPE_REPORTS:
			folderName = CFCopyLocalizedString(CFSTR("ReportsFolder"), NULL);
			break;
		case MAC_FILETYPE_SUPPORT:
			folderName = CFCopyLocalizedString(CFSTR("MiscSupportFolder"), NULL);
			break;
		case MAC_FILETYPE_CATEGORIES:
			folderName = CFCopyLocalizedString(CFSTR("CategoriesFolder"), NULL);
			break;
		case MAC_FILETYPE_TITLE_SCREENSHOTS:
			folderName = CFCopyLocalizedString(CFSTR("TitleScreenshotFolder"), NULL);
			break;
		case MAC_FILETYPE_CABINET_ARTWORK:
			folderName = CFCopyLocalizedString(CFSTR("CabinetArtFolder"), NULL);
			break;
		case MAC_FILETYPE_FLYER_ARTWORK:
			folderName = CFCopyLocalizedString(CFSTR("FlyerArtFolder"), NULL);
			break;
		case MAC_FILETYPE_MARQUEE_ARTWORK:
			folderName = CFCopyLocalizedString(CFSTR("MarqueeArtFolder"), NULL);
			break;
		case MAC_FILETYPE_CONTROL_PANEL_ARTWORK:
			folderName = CFCopyLocalizedString(CFSTR("ControlPanelArtFolder"), NULL);
			break;
		default:
			return NULL;
	}

	return folderName;
}

static bool ShouldFileBeCached (int inMAMEFolderType)
{
	// LBO 9/22/04. Wow! Turns out that FSReadFork is a major hotspot when you're
	// reading one byte at a time, like the cheat file does. Use the cached mechanism
	// for these types of files.
	
	switch (inMAMEFolderType)
	{
		case FILETYPE_CHEAT:
		case FILETYPE_HISTORY:
		case FILETYPE_HIGHSCORE_DB:
			return true;
		default:
			return false;
	}
}

//-----------------------------------------------------------------------------
// GetCStringForMAMEFolder
//-----------------------------------------------------------------------------
// Given a MAME folder/file type, returns a c-string pointing to the appropriate
// MacMAME support folder location. Assumes that outPath is kMacMaxPath bytes big.

extern "C" OSStatus GetCStringForMAMEFolder (int inMAMEFolderType, char *outPath)
{
	OSStatus err = fnfErr;
	
	require (outPath, badParam);
	outPath[0] = 0;
	
	FSRef folderRef;
	require_noerr( err = GetFSRefForMAMEFolder (inMAMEFolderType, &folderRef), cantGetFSRef );

	// Now convert the FSRef for the MAME folder to a full pathname.
	require_noerr( FSRefMakePath (&folderRef, (UInt8 *)outPath, kMacMaxPath), cantMakePath );
			
	err = noErr;
	
badParam:
cantGetFSRef:
cantMakePath:
	return err;
}

//-----------------------------------------------------------------------------
// CopyCFURLForMAMEFolder
//-----------------------------------------------------------------------------
// Given a MAME folder/file type, creates a CFURL pointing to the appropriate
// MacMAME support folder location. The caller is responsible for releasing
// the CFURLRef.

CFURLRef CopyCFURLForMAMEFolder (int inMAMEFolderType)
{
	CFURLRef outURL = NULL;
	FSRef fsRef;

	require_noerr(GetFSRefForMAMEFolder (inMAMEFolderType, &fsRef), cantGetFSRef);
	outURL = CFURLCreateFromFSRef (NULL, &fsRef);

cantGetFSRef:
	
	return outURL;
}

//-----------------------------------------------------------------------------
// GetFSRefForMAMEFolder
//-----------------------------------------------------------------------------
// Given a MAME folder/file type, populates a FSRef pointing to the appropriate
// MacMAME support folder location. Creates any intermediate directories and
// resolves any aliases along the way.

extern "C" OSStatus GetFSRefForMAMEFolder (int inMAMEFolderType, FSRef *outFSRef)
{
	OSStatus err = fnfErr;
	CFStringRef folderString = NULL;
	Boolean isFolder, isAlias;
	
	require(outFSRef, badParam);
	
	// Get FSRef for the user-local Documents folder (our parent)
	FSRef documentsRef;
	err = FSFindFolder (kUserDomain, kDocumentsFolderType, true, &documentsRef);
	require_noerr(err, cantFindDocumentsFolder);
	err = FSResolveAliasFile (&documentsRef, true, &isFolder, &isAlias);

	FSRef parentFolderRef;
	HFSUniStr255 name;
	
	if (inMAMEFolderType != MAC_FILETYPE_MAC_FILES)
	{
		// If we're not looking for the "MacMAME User Data" folder itself, find it and use it as the parent for the folder
		// we're looking for.
		folderString = CopyFolderStringForFolderType (MAC_FILETYPE_MAC_FILES);
		require (folderString, cantGetFolderString);
		name.length = ConvertCFStringToUnicode (folderString, name.unicode);

		CFRelease (folderString);
		folderString = NULL;
		
		// Create a "MacMAME User Data" folder in the Documents folder if necessary
		err = FSCreateDirectoryUnicode (&documentsRef, name.length, name.unicode, kFSCatInfoNone, NULL, &parentFolderRef, NULL, NULL);
		if (err == dupFNErr)
		{
			// Folder already exists, get valid FSRef to it.
			require_noerr (FSMakeFSRefUnicode (&documentsRef, name.length, name.unicode, kTextEncodingUnknown, &parentFolderRef), cantMakeFSRef);
		}
		else if (err) goto cantCreateDirectory;

		err = FSResolveAliasFile (&parentFolderRef, true, &isFolder, &isAlias);
		require_noerr(err, cantResolveAlias);
	}
	else
	{
		// We're looking for the "MacMAME User Data" folder, so the Documents folder is the parent.
		parentFolderRef = documentsRef;
	}

	// Now find the MAME folder we're looking for, creating it if necessary.
	folderString = CopyFolderStringForFolderType (inMAMEFolderType);
	require (folderString, cantGetFolderString);
	
	name.length = ConvertCFStringToUnicode (folderString, name.unicode);
	err = FSCreateDirectoryUnicode (&parentFolderRef, name.length, name.unicode, kFSCatInfoNone, NULL, outFSRef, NULL, NULL);
	if (err == dupFNErr)
	{
		// Folder already exists, get valid FSRef to it.
		require_noerr (FSMakeFSRefUnicode (&parentFolderRef, name.length, name.unicode, kTextEncodingUnknown, outFSRef), cantMakeFSRef2);
	}

	err = FSResolveAliasFile (outFSRef, true, &isFolder, &isAlias);
	require_noerr(err, cantResolveAlias2);

badParam:
cantFindDocumentsFolder:
cantCreateDirectory:
cantGetFolderString:
cantGetFolderString2:
cantMakeFSRef:
cantMakeFSRef2:
cantResolveAlias:
cantResolveAlias2:
	if (folderString) CFRelease (folderString);
	return err;
}

//-----------------------------------------------------------------------------
// GetFSSpecForMAMEFolder
//-----------------------------------------------------------------------------
// Given a MAME folder/file type, returns a FSSpec pointing to the appropriate
// MacMAME support folder location.

extern "C" OSStatus GetFSSpecForMAMEFolder (int inMAMEFolderType, FSSpec *outFSSpec)
{
	OSStatus err = fnfErr;
	FSRef fsRef;
	
	require(outFSSpec, badParam);
	
	// Simply get the FSRef for the folder, then convert it to an FSSpec.
	require_noerr( err = GetFSRefForMAMEFolder (inMAMEFolderType, &fsRef), cantGetFSRef );
	require_noerr( err = FSGetCatalogInfo(&fsRef, kFSCatInfoNone, NULL, NULL, outFSSpec, NULL), cantCreateFSSpec );

badParam:
cantGetFSRef:
cantCreateFSSpec:
	return err;
}

//-----------------------------------------------------------------------------
// GetFileTypeForMAMEFolder
//-----------------------------------------------------------------------------
// Given a MAME folder/file type, returns the appropriate Mac filetype/creator
// info.

void GetFileTypeForMAMEFolder (int inMAMEFolderType, OSType *outCreator, OSType *outType)
{
	if (!outCreator || !outType) return;
	
	switch (inMAMEFolderType)
	{
		case FILETYPE_IMAGE_DIFF:		*outType = '.DIF'; *outCreator = kCreator; break;
		case FILETYPE_SAMPLE:			*outType = 'WAVE'; *outCreator = 'TVOD'; break;
		case FILETYPE_NVRAM:			*outType = '.NV '; *outCreator = kCreator; break;
		case FILETYPE_HIGHSCORE:		*outType = '.HI '; *outCreator = kCreator; break;
		case FILETYPE_HIGHSCORE_DB:		*outType = 'Data'; *outCreator = kCreator; break;
		case FILETYPE_CONFIG:			*outType = '.CFG'; *outCreator = kCreator; break;
		case FILETYPE_INPUTLOG:			*outType = '.INP'; *outCreator = kCreator; break;
		case FILETYPE_STATE:			*outType = 'Save'; *outCreator = kCreator; break;
		case FILETYPE_MEMCARD:			*outType = '.MEM'; *outCreator = kCreator; break;
		case FILETYPE_SCREENSHOT:		*outType = 'PNGf'; *outCreator = 'prvw'; break;
		case FILETYPE_CHEAT:			*outType = 'TEXT'; *outCreator = 'ttxt'; break;

		default:						*outType = 'Data'; *outCreator = kCreator; break;
	}
}

//-----------------------------------------------------------------------------
// AddFolderPathRecursive
//-----------------------------------------------------------------------------
// Given a MAME folder/file type and a parent FSRef, recursively walks through
// the directory adding any subfolders to the search paths for that MAME folder type.

void AddFolderPathRecursive (int inMAMEFolderType, FSRef *inParent, int inDepth)
{
	FSIterator fsIterator;
	OSStatus err = noErr;
	require_noerr(FSOpenIterator (inParent, kFSIterateFlat, &fsIterator), cantOpenIterator);

	while (1)
	{
		ItemCount itemsFound;
		FSRef subfolderRef;
		Boolean isFolder, isAlias;
		HFSUniStr255 name;

		if ((err = FSGetCatalogInfoBulk (fsIterator, 1, &itemsFound, NULL, 0, NULL, &subfolderRef, NULL, &name)))
			break;
		
		err = FSResolveAliasFile (&subfolderRef, true, &isFolder, &isAlias);
		
		// We only search 4 levels deep to avoid a possible infinite loop with recursion, which
		// could happen if we have a subfolder that is incorrectly aliased up to a folder at
		// a higher level.
		if (!err && isFolder && (inDepth < 4) && (name.unicode[0] != (UniChar)'('))
		{
			FSRefPtr newRef = new FSRef;
			require(newRef, cantAllocateFSRef);

			*newRef = subfolderRef;
			sMAMESubfolderList[inMAMEFolderType].push_back(newRef);
			AddFolderPathRecursive (inMAMEFolderType, &subfolderRef, inDepth + 1);
		}		
	}
	
	require_noerr(FSCloseIterator (fsIterator), cantCloseIterator);

cantOpenIterator:
cantAllocateFSRef:
cantCloseIterator:
	return;
}

extern "C" void RenameOldMacMAMEFolder (void)
{
	OSStatus err = fnfErr;
	CFStringRef folderString = NULL;
	CFStringRef newFolderString = NULL;
	Boolean isFolder, isAlias;
	FSRef newRef;
	
	// Get FSRef for the user-local Documents folder (our parent)
	FSRef documentsRef;
	err = FSFindFolder (kUserDomain, kDocumentsFolderType, true, &documentsRef);
	require_noerr(err, cantFindDocumentsFolder);
	err = FSResolveAliasFile (&documentsRef, true, &isFolder, &isAlias);

	// Get the "MacMAME" folder in the Documents folder if it exists
	FSRef macmameRef;
	HFSUniStr255 name;
	
	// Now create the MAME folder type
	folderString = CopyFolderStringForFolderType (MAC_FILETYPE_MAC_FILES_OLD);
	require (folderString, cantGetFolderString);
	
	name.length = ConvertCFStringToUnicode (folderString, name.unicode);
	// Folder already exists, get valid FSRef to it.
	require_noerr (FSMakeFSRefUnicode (&documentsRef, name.length, name.unicode, kTextEncodingUnknown, &macmameRef), cantMakeFSRef2);

	newFolderString = CopyFolderStringForFolderType (MAC_FILETYPE_MAC_FILES);
	require (newFolderString, cantGetFolderString);
	
	name.length = ConvertCFStringToUnicode (newFolderString, name.unicode);
	// Folder already exists, get valid FSRef to it.
	err = FSRenameUnicode (&macmameRef, name.length, name.unicode, kTextEncodingUnknown, &newRef);

cantFindDocumentsFolder:
cantGetFolderString:
cantMakeFSRef2:
	if (folderString) CFRelease (folderString);
	if (newFolderString) CFRelease (newFolderString);
	return;
}

const ROMSetData * FindROMFast (const char *inFilename, char *outDriverName, Boolean *outIsZip)
{
	char driverName[2048];

	Boolean isZip;
	isZip = (mame_stricmp (inFilename + strlen (inFilename)-4, ".zip") == 0);

	char *slashPtr;
	if ((slashPtr = strrchr (inFilename, '/')))
	{
		strlcpy (driverName, slashPtr, sizeof(driverName));
	}
	else
		strlcpy (driverName, inFilename, sizeof(driverName));

	if (isZip)
		driverName[strlen(driverName)-4] = 0;

	if (outDriverName)
		strcpy (outDriverName, driverName);
	if (outIsZip)
		*outIsZip = isZip;

	return FindROMSetForDriver(driverName);
}

static osd_file * mac_fopen_cached (int inMAMEFolderType, int pathindex, const char *composite_filename, const char *mode)
{
	FILE *cachedFile = NULL;
	osd_file *newFile = NULL;
	char filePath[kMacMaxPath];
	FSRef folderRef;

	newFile = (osd_file *)calloc(1, sizeof(osd_file));
	if (newFile == NULL) return newFile;

	if (composite_filename[0] == '/')
	{
		// If the first character in the filename is a '/', assume we're dealing with
		// a full path, and create an FSRef for the path portion instead of looking it
		// up in our "~/Documents/MacMAME/*" hierarchy.
		
		strlcpy (filePath, composite_filename, kMacMaxPath);
	}
	else
	{
		// The first char wasn't a '/', so we use the standard MacMAME path rules.
		folderRef = *sMAMESubfolderList[inMAMEFolderType].at(pathindex);
		
		// First, convert the FSRef for the MAME folder to a full pathname.
		require_noerr( FSRefMakePath (&folderRef, (UInt8 *)filePath, sizeof(filePath)), cantMakePath );
			
		// Now append the filename we want to open, including any partial paths it references
		strlcat (filePath, "/", sizeof(filePath));
		strlcat (filePath, composite_filename, sizeof(filePath));
	}

	cachedFile = fopen(filePath, mode);
	require_quiet (cachedFile, cantOpenFile);

	// determine the length
	fseek (cachedFile, 0, SEEK_END);
	newFile->length = ftell (cachedFile);
	fseek (cachedFile, 0, SEEK_SET);
					
	// allocate memory for entire file
	newFile->cachedData = malloc (newFile->length);
	require (newFile->cachedData, cantAllocateBuffer);

	// read and checksum file
	(void)fread(newFile->cachedData, 1, newFile->length, cachedFile);
	newFile->isCached = true;
	goto good;

cantOpenFile:
cantAllocateBuffer:
	if (newFile)
	{
		free (newFile);
		newFile = NULL;
	}

cantMakePath:
good:
	if (cachedFile)
		fclose (cachedFile);
		
	return newFile;
}

#pragma mark -
#pragma mark ¥ MAME routines

extern "C" int osd_get_path_count(int inMAMEFolderType)
{
	if (!sMAMESubfolderSearched[inMAMEFolderType])
	{
		FSRef folderRef;
		
		sMAMESubfolderSearched[inMAMEFolderType] = true;

		require_noerr(GetFSRefForMAMEFolder (inMAMEFolderType, &folderRef), cantGetFSRef);
		FSRefPtr newRef = new FSRef;
		require(newRef, cantAllocateFSRef);

		*newRef = folderRef;
		sMAMESubfolderList[inMAMEFolderType].push_back(newRef);

		AddFolderPathRecursive (inMAMEFolderType, &folderRef, 0);
	}

cantGetFSRef:
cantAllocateFSRef:
	return sMAMESubfolderList[inMAMEFolderType].size();
}

extern "C" int osd_get_path_info(int inMAMEFolderType, int pathindex, const char *composite_filename)
{
	if (inMAMEFolderType == FILETYPE_ROM)
	{
		const ROMSetData *romset;
		romset = NULL;
		Boolean isZip;

		romset = FindROMFast (composite_filename, NULL, &isZip);
		if (romset)
		{
			if (romset->format == kROMSetFormatFolder)
				return isZip ? PATH_NOT_FOUND : PATH_IS_DIRECTORY;
			else if (romset->format == kROMSetFormatZip)
				return isZip ? PATH_IS_FILE : PATH_NOT_FOUND;
		}
		return PATH_NOT_FOUND;
	}

	FSRef folderRef = *sMAMESubfolderList[inMAMEFolderType].at(pathindex);

	require(composite_filename, badParam);

	HFSUniStr255 name;
	name.length = ConvertCStringToUnicode (composite_filename, name.unicode);
	if (name.length == 0) goto bail;

	FSRef fileRef;
	require_noerr_quiet(FSMakeFSRefUnicode (&folderRef, name.length, name.unicode, kTextEncodingUnknown, &fileRef), cantMakeFSRef);

	Boolean isFolder, isAlias;
	require_noerr_quiet(FSResolveAliasFile (&fileRef, true, &isFolder, &isAlias), cantResolveAlias);

	FSCatalogInfo catInfo;
	require_noerr(FSGetCatalogInfo (&fileRef, kFSCatInfoNodeFlags, &catInfo, NULL, NULL, NULL), cantGetCatInfo);

	if (catInfo.nodeFlags & kFSNodeIsDirectoryMask)
		return PATH_IS_DIRECTORY;
	else
		return PATH_IS_FILE;

badParam:
cantMakeFSRef:
cantResolveAlias:
cantGetCatInfo:
bail:	
	return PATH_NOT_FOUND;
}

extern "C" osd_file *osd_fopen(int inMAMEFolderType, int pathindex, const char *composite_filename, const char *mode)
{
	osd_file *newFile = NULL;
	OSStatus err = noErr;

	require (composite_filename, badParam);
	require (mode, badParam);
	
	if (ShouldFileBeCached (inMAMEFolderType))
		return mac_fopen_cached (inMAMEFolderType, pathindex, composite_filename, mode);
	
	// create a file handle structure
	newFile = (osd_file *)calloc(1, sizeof(osd_file));
	if (newFile == NULL) return newFile;
	
	if (inMAMEFolderType == FILETYPE_ROM)
	{
		const ROMSetData *romset;
		Boolean isZip;

		romset = FindROMFast (composite_filename, NULL, &isZip);
		if (romset && isZip)
		{
			if ( FSpMakeFSRef( &romset->spec, &newFile->fileRef) == noErr )
				goto shortcut;
		}
	}

	FSRef folderRef;
	char fileName[kMacMaxPath];
	char parentPath[kMacMaxPath];
	Boolean isDirectory;

	if (composite_filename[0] == '/')
	{
		// If the first character in the filename is a '/', assume we're dealing with
		// a full path, and create an FSRef for the path portion instead of looking it
		// up in our "~/Documents/MacMAME/*" hierarchy.
		
		strlcpy (parentPath, composite_filename, kMacMaxPath);
		
		// Split the full path into the parent folder path and the ending filename component.
		char *lastSlash = strrchr (parentPath, '/');
		if (lastSlash)
		{
			// Copy the actual filename component.
			strlcpy (fileName, lastSlash+1, kMacMaxPath);
			
			// Terminate the folder path string component.
			*(lastSlash+1) = 0x00;
		}
				
		require_noerr( FSPathMakeRef((UInt8*)parentPath, &folderRef, &isDirectory), cantGetParent );
	}
	else
	{
		// The first char wasn't a '/', so we use the standard MacMAME path rules.
		folderRef = *sMAMESubfolderList[inMAMEFolderType].at(pathindex);
		
		// If composite_filename is a partial path (i.e. contains a '/'), we have to take extra steps
		char *lastSlash = strrchr (composite_filename, '/');
		if (lastSlash)
		{
			// First, convert the FSRef for the MAME folder to a full pathname.
			require_noerr( FSRefMakePath (&folderRef, (UInt8 *)parentPath, sizeof(parentPath)), cantMakePath );
			
			strlcat (parentPath, "/", sizeof(parentPath));
			
			// Now append the filename we want to open, including any partial paths it references
			strlcat (parentPath, composite_filename, sizeof(parentPath));
			
			lastSlash = strrchr (parentPath, '/');

			// Copy the actual filename component.
			strlcpy (fileName, lastSlash+1, kMacMaxPath);
			
			// Separate the entire path from the filename.
			*(lastSlash+1) = 0x00;
			
			// Finally, make an FSRef out of the combined path components.
			require_noerr( FSPathMakeRef((UInt8*)parentPath, &folderRef, &isDirectory), cantGetParent );
		}
		else
			strlcpy (fileName, composite_filename, sizeof (fileName));
	}
	
	HFSUniStr255 name;
	name.length = ConvertCStringToUnicode (fileName, name.unicode);
	if (name.length == 0) goto bail;

	err = FSMakeFSRefUnicode (&folderRef, name.length, name.unicode, kTextEncodingUnknown, &newFile->fileRef);

	Boolean isFolder, isAlias;
	err = FSResolveAliasFile (&newFile->fileRef, true, &isFolder, &isAlias);

shortcut:
	HFSUniStr255 dataForkName;
	FSGetDataForkName (&dataForkName);
	
	// convert the mode into disposition and access
	SInt8 perm; perm = fsRdPerm;
	bool write; write = false;
	if (strchr(mode, 'w'))
	{
		perm = fsWrPerm;
		write = true;
	}
	if (strchr(mode, '+'))
	{
		perm = fsRdWrPerm;
		write = true;
	}

	if (write)
	{
		FSCatalogInfo catInfo;
		FInfo *finderInfo = (FInfo*)&catInfo.finderInfo;
		memset (finderInfo, 0, sizeof (FInfo));

		GetFileTypeForMAMEFolder (inMAMEFolderType, &finderInfo->fdType, &finderInfo->fdCreator);

		(void)FSCreateFileUnicode (&folderRef, name.length, name.unicode, kFSCatInfoFinderInfo, &catInfo, &newFile->fileRef, NULL);
	}
	
	err = FSOpenFork (&newFile->fileRef, dataForkName.length, dataForkName.unicode, perm, &newFile->refNum);
	if (err == noErr) goto done;

bail:
badParam:
cantMakePath:
cantGetParent:
	if (newFile)
	{
		free (newFile);
		newFile = NULL;
	}
	newFile = NULL;

done:
	return newFile;
}

extern "C" UINT32 osd_fread(osd_file *f, void *buffer, UINT32 length)
{
	assert(f);
	if (!f) return 0;
	ByteCount found;
	
	if (f->isCached)
	{
		if (f->cachedData)
		{
			if ((length + f->offset) > f->length)
			{
				length = f->length - f->offset;
				f->eof = true;
			}
			
			BlockMoveData((void *)(f->offset + (UInt32)f->cachedData), buffer, length);
			f->offset += length;
			found = length;
		}
	}
	else
	{
		// LBO 9/22/04. Wow! Turns out that FSReadFork is a major hotspot when you're
		// reading one byte at a time, like the cheat file does. Use the cached mechanism
		// for these types of files.
		OSStatus err = FSReadFork (f->refNum, fsAtMark, 0, length, buffer, &found);
	}
	return found;
}

extern "C" UINT32 osd_fwrite (osd_file *f, const void *buffer, UINT32 length)
{
	assert(f);
	if (!f) return 0;
	ByteCount found;
	
	if (f->isCached)
	{
		// Writing isn't supported for cached files yet.
		return 0;
	}
	else
	{
		OSStatus err = FSWriteFork (f->refNum, fsAtMark, 0, length, buffer, &found);
	}
		
	return found;
}

extern "C" int osd_feof(osd_file *f)
{
	if (!f) return 0;
	
	if (f->isCached)
		return f->eof;
	
	SInt64 position, size;
	OSStatus err = FSGetForkPosition (f->refNum, &position);
	FSGetForkSize(f->refNum, &size);
	
	return position >= size;
}

extern "C" UINT64 osd_ftell(osd_file *f)
{
	assert(f);
	if (!f) return 0;
	
	if (f->isCached)
		return f->offset;
	
	SInt64 position;
	OSStatus err = FSGetForkPosition (f->refNum, &position);
	return position;
}

extern "C" int osd_fseek(osd_file *f, INT64 offset, int whence)
{
	assert(f);
	if (!f) return 1;
	
	if (f->isCached)
	{
		switch (whence)
		{
			case SEEK_SET:
				f->offset = offset;
				break;
			case SEEK_CUR:
				f->offset += offset;
				break;
			case SEEK_END:
				f->offset = f->length + offset;
				break;
		}
		if (f->offset < 0)
			f->offset = 0;
		if (f->offset > f->length)
			f->offset = f->length;
		f->eof = false;
	}
	else
	{
		UInt16 mode;
		switch (whence)
		{
			default:
			case SEEK_SET:	mode = fsFromStart;			break;
			case SEEK_CUR:	mode = fsAtMark;			break;
			case SEEK_END:	mode = fsFromLEOF;			break;
		}
		OSStatus err = FSSetForkPosition (f->refNum, mode, offset);
		if (err) return 1;
	}
	
	return 0;
}

extern "C" void osd_fclose(osd_file *f)
{
	assert(f);
	if (!f) return;
	
	if (f->isCached)
	{
		if (f->cachedData)
			free(f->cachedData);
	}
	else
		FSCloseFork (f->refNum);
	free (f);
}

extern "C" int osd_display_loading_rom_message(const char *name, rom_load_data *romdata)
{
	static UInt32		displayTicks = NULL;
	static DialogRef	dlog = NULL;
	OSErr				err = noErr;

	if (romdata->romsloaded == 1)
	{
		// just starting; we won't create a progress dialog until 2 seconds have elapsed
		displayTicks = TickCount() + 120UL;
	}

	// cleanup if the process is finished
	if (0 == name)
	{
		if (dlog)
		{
			DestroyProgressDialog (dlog);
			dlog = NULL;
		}
		return 0;
	}

	// if enough time has elapsed, create a progress dialog
	if (NULL == dlog && TickCount() > displayTicks)
	{
		Str255	title, msg;
		
		// create an appropriate title for the progress dialog
		sprintf ((char *)title, "%s '%s'É", GetIndCString (rStrings, kLoadingProgressTitle, 
					"Loading"), Machine->gamedrv->name);
		c2pstrcpy(title, (char *)title);
		
		// get the appropriate status message for the progress dialog
		PLstrcpy (msg, GetIndPString (rStrings, kROMsRemainingProgressMsg,
					"\pROMs remaining to load:"));

		// create a progress dialog
		dlog = CreateProgressDialog (title, msg, romdata->romstotal);
	}
	
	if (dlog)
	{
		// update the progress dialog to reflect the current status
		UpdateProgressStatus (dlog, name, romdata->romsloaded);

		// handle events related to the progress dialog and detect "Stop" button
		err = HandleProgressEvents (dlog);
		if (err == userCanceledErr)
		{
			DestroyProgressDialog (dlog);
			dlog = NULL;
		}
	}				

	return err;
}

#pragma mark -
#pragma mark ¥ Navigation Services Routines

//===============================================================================
//	_NavGetOneFile
//
//	Call this function to ask user for a file to open.
//
//	Returns the file in outURL.
//===============================================================================

extern "C" Boolean _NavGetOneFile (CFStringRef inMessage, int inMAMEFolderType, CFURLRef *outURL)
{
	Boolean retVal = false;

	require(outURL, badParam);
	*outURL = NULL;
	
	NavDialogCreationOptions navOptions;
	NavDialogRef navDialog;
	NavReplyRecord navReply;
	
	// Set up the Nav Services options for this load dialog.
	require_noerr( NavGetDefaultDialogCreationOptions (&navOptions), cantGetDefaultOptions );
	navOptions.clientName = CFSTR(kAppName);
	if (inMessage)
		navOptions.message = inMessage;
	
	// Get the type/creator info for this MAME file type
	OSType creator, filetype;
	GetFileTypeForMAMEFolder (inMAMEFolderType, &creator, &filetype);
	
	// Create our nav services dialog
	require_noerr( NavCreateChooseFileDialog (&navOptions, NULL, NULL, NULL, NULL, NULL, &navDialog), cantCreateDialog );

	// Set the default location for where we normally would want to save the file.
	AEDesc fileLocation;
	FSRef defaultFSRef;

	if (GetFSRefForMAMEFolder (inMAMEFolderType, &defaultFSRef) == noErr)
	{
		require_noerr( AECreateDesc (typeFSRef, &defaultFSRef, sizeof (FSRef), &fileLocation), cantCreateDesc );
		require_noerr( NavCustomControl (navDialog, kNavCtlSetLocation, (void*) &fileLocation), cantSetControl );
	}
	
	// Run the Nav Services dialog and get the reply when it has finished.
	require_noerr( NavDialogRun (navDialog), cantRunDialog );
	require_noerr_quiet( NavDialogGetReply (navDialog, &navReply), cantGetReply );
	
	if (navReply.validRecord)
	{
		AEKeyword aeKeyword;
		DescType typeCode;
		Size actualSize;
		FSRef fileRef;
		
		// Get FSRef for selection
		require_noerr( AEGetNthPtr (&navReply.selection, 1, typeFSRef, &aeKeyword, &typeCode, &fileRef, sizeof (FSRef), &actualSize), cantGetAEPtr );
	
		// Convert that FSRef to a CFURLRef
		*outURL = CFURLCreateFromFSRef (NULL, &fileRef);
		if (*outURL)
			retVal = true;
	}

cantGetAEPtr:
	(void)NavDisposeReply (&navReply);

cantCreateDesc:
cantSetControl:
cantRunDialog:
cantGetReply:
	NavDialogDispose (navDialog);

badParam:
cantGetDefaultOptions:
cantCreateDialog:
	return retVal;
}

//===============================================================================
//	_NavPutOneFile
//
// Call this function to ask user to specify a location and name for saving a file.
// inMessage and inDefaultFileName are optional. outURL is required.
//===============================================================================

extern "C" Boolean _NavPutOneFile (CFStringRef inMessage, CFStringRef inDefaultFileName, int inMAMEFolderType, CFURLRef *outURL, FSSpec *outSpec)
{
	Boolean retVal = false;

	if (outURL)
		*outURL = NULL;
	
	NavDialogCreationOptions navOptions;
	NavDialogRef navDialog;
	NavReplyRecord navReply;
	
	// Set up the Nav Services options for this save dialog.
	require_noerr( NavGetDefaultDialogCreationOptions (&navOptions), cantGetDefaultOptions );
	navOptions.clientName = CFSTR(kAppName);
	if (inDefaultFileName)
		navOptions.saveFileName = inDefaultFileName;
	if (inMessage)
		navOptions.message = inMessage;
	
	// Get the type/creator info for this MAME file type
	OSType creator, filetype;
	GetFileTypeForMAMEFolder (inMAMEFolderType, &creator, &filetype);
	
	// Create our nav services dialog
	require_noerr( NavCreatePutFileDialog (&navOptions, creator, filetype, NULL, NULL, &navDialog), cantCreateDialog );

	// Set the default location for where we normally would want to save the file.
	AEDesc fileLocation;
	FSRef defaultFSRef;
				
	if (GetFSRefForMAMEFolder (inMAMEFolderType, &defaultFSRef) == noErr)
	{
		require_noerr( AECreateDesc (typeFSRef, &defaultFSRef, sizeof (FSRef), &fileLocation), cantCreateDesc );
		require_noerr( NavCustomControl (navDialog, kNavCtlSetLocation, (void*) &fileLocation), cantSetControl );
	}
	
	// Run the Nav Services dialog and get the reply when it has finished.
	require_noerr( NavDialogRun (navDialog), cantRunDialog );
	require_noerr( NavDialogGetReply (navDialog, &navReply), cantGetReply );
	
	if (navReply.validRecord)
	{
		AEKeyword aeKeyword;
		DescType typeCode;
		Size actualSize;

		if (outSpec)
		{
			FSSpec tempSpec;
			char tempName[256];
			
			require_noerr( AEGetNthPtr (&navReply.selection, 1, typeFSS, &aeKeyword, &typeCode, &tempSpec, sizeof (FSSpec), &actualSize), cantGetAEPtr );
			if (CFStringGetCString (NavDialogGetSaveFileName (navDialog), tempName, sizeof(tempName), kCFStringEncodingMacRoman))
			{
				OSErr err;
				
				err = GetPartialPathSpec (&tempSpec, tempName, outSpec);
				if (!err || (err = fnfErr)) retVal = true;
			}
		}

		if (outURL)
		{
			FSRef folderRef;
		
			// Get FSRef for parent folder
			require_noerr( AEGetNthPtr (&navReply.selection, 1, typeFSRef, &aeKeyword, &typeCode, &folderRef, sizeof (FSRef), &actualSize), cantGetAEPtr );
		
			// Convert that FSRef to a CFURLRef
			CFURLRef folderURL;
			folderURL = CFURLCreateFromFSRef (NULL, &folderRef);
			require (folderURL, cantCreateFolderURL);
			
			// Append new filename to the CFURL. This is what we'll return to the caller.
			*outURL = CFURLCreateCopyAppendingPathComponent (NULL, folderURL, navReply.saveFileName, false);
			retVal = true;

			CFRelease (folderURL);
		}
	}

cantGetAEPtr:
cantCreateFolderURL:
	(void)NavDisposeReply (&navReply);

cantCreateDesc:
cantSetControl:
cantRunDialog:
cantGetReply:
	NavDialogDispose (navDialog);

badParam:
cantGetDefaultOptions:
cantCreateDialog:
	return retVal;
}

#pragma mark -
#pragma mark ¥ File Utility Routines

extern "C" OSStatus GetApplicationPackageFSSpecFromBundle (FSSpecPtr theFSSpecPtr)
{
	FSRef myBundleRef;
    CFBundleRef refMainBundle = NULL;
    CFURLRef refMainBundleURL = NULL;
	Boolean ok;

	// get app bundle
	refMainBundle = CFBundleGetMainBundle();
	if (!refMainBundle) return paramErr;

	// create a URL to the app bundle
	refMainBundleURL = CFBundleCopyBundleURL (refMainBundle);
	if (!refMainBundleURL) return paramErr;

	ok = CFURLGetFSRef (refMainBundleURL, &myBundleRef);
	CFRelease (refMainBundleURL);
	if (!ok) return fnfErr;

	return FSGetCatalogInfo(&myBundleRef, kFSCatInfoNone, NULL, NULL, theFSSpecPtr, NULL);
}

//===============================================================================
//	GetHashAsUINT32
//
//	Converts a hash string into a CRC32 and returns that as a UINT32, if possible
//===============================================================================

extern "C" void GetHashAsUINT32 (const char *inHash, UINT32 *outCrc)
{
	UINT8 crcs[4];
	*outCrc = 0;

	if (inHash && hash_data_extract_binary_checksum (inHash, HASH_CRC, crcs) != 0)
	{
		// Store the crc as a UINT32
		*outCrc = ((unsigned long)crcs[0] << 24) |
			 	  ((unsigned long)crcs[1] << 16) |
				  ((unsigned long)crcs[2] <<  8) |
				  ((unsigned long)crcs[3] <<  0);
	}
}

/*
	Convert a FSSpec to a POSIX path
*/
extern "C" OSStatus GetFullPathFromSpec(const FSSpec *spec, UInt8 *fullpath, int maxBufLen)
{
	OSStatus err;
	FSRef theRef;
	FSSpec tempSpec;
	Boolean isFolder, wasAliased;
	
	// Make a copy of the spec and attempt to resolive it if it's an alias
	tempSpec = *spec;
	err = ResolveAliasFile (&tempSpec, true, &isFolder, &wasAliased);

	// Conver the FSSpec to an FSRef and attempt to get a full path from it.
	err = FSpMakeFSRef(&tempSpec, &theRef);
	if (err == noErr)
	{
		FSRefMakePath (&theRef, (UInt8 *) fullpath, maxBufLen);
	}
	
	// It's not possible to have an FSRef for files that don't exist yet. In this
	// case, we create an FSRef for the parent directory, convert that to a full path
	// and append the filename to that.
	if (err == fnfErr)
	{
		FSRefParam paramBlock;
		CFURLRef theParentURL;
		CFStringRef theFileName;
		CFURLRef theFileURL;
		Boolean successful;

		paramBlock.ioVRefNum = spec->vRefNum;
		paramBlock.ioNamePtr = NULL;
		paramBlock.ioDirID = spec->parID;
		paramBlock.newRef = &theRef;
		err = PBMakeFSRefSync(& paramBlock);
		if (err == noErr)
		{
			theParentURL = CFURLCreateFromFSRef(CFAllocatorGetDefault(), paramBlock.newRef);
			theFileName = CFStringCreateWithBytes(CFAllocatorGetDefault(), spec->name+1,
													spec->name[0], kCFStringEncodingMacRoman,
													false);
			theFileURL = CFURLCreateWithFileSystemPathRelativeToBase(CFAllocatorGetDefault(),
																		theFileName,
																		kCFURLPOSIXPathStyle, false,
																		theParentURL);
			successful = CFURLGetFileSystemRepresentation(theFileURL, true, fullpath, maxBufLen);
			CFRelease(theFileURL);
			CFRelease(theFileName);
			CFRelease(theParentURL);
		}
	}

	return err;
}

/* given an FSSpec, make an FSSpec for a partial path under it */
/* Pass in a C string for the partial path. Resolves all aliases. */
extern "C" OSStatus GetPartialPathSpec (const FSSpec *base, const char *partpath, FSSpec *spec)
{
	Str255	path;
	char temp[256];
	OSStatus	err;
	FSSpec appSpec;
	
	appSpec.parID = base->parID;
	appSpec.vRefNum = base->vRefNum;
	PLstrcpy (appSpec.name, base->name);

	if ((appSpec.parID == 0) && (appSpec.vRefNum == 0) && (appSpec.name[0] == 0))
	{
		// We're looking for the partial path for the app directory. Since
		// we're a bundle now, get an FSSpec for the bundle directory.
		err = GetApplicationPackageFSSpecFromBundle (&appSpec);
		appSpec.name[0] = 0;
	}

	if (appSpec.name[0])
	{	
		p2cstrcpy (temp, appSpec.name);
		sprintf ((char *)path,":%s:%s", temp, partpath);
	}
	else
	{
		sprintf ((char *)path, ":%s", partpath);
	}
	c2pstrcpy(path, (char *)path);
	
	err = FSMakeFSSpec (appSpec.vRefNum, appSpec.parID, path, spec);
	if (err==noErr)
	{
		Boolean isFolder, wasAliased;
		err = ResolveAliasFile (spec, true, &isFolder, &wasAliased);
	}
	return err;
}

void GetFilenameFromURL (CFURLRef inRef, char *outName, int inNameLength)
{
	require(outName, badParam);
	require(inNameLength, badParam);
	
	char fullPath[kMacMaxPath];
	Boolean successful; successful = CFURLGetFileSystemRepresentation(inRef, true, (UInt8 *)fullPath, sizeof(fullPath));
	if (successful)
	{
		char *lastSlash = strrchr (fullPath, '/');
		if (lastSlash)
		{
			// Copy the actual filename component.
			strlcpy (outName, lastSlash+1, inNameLength);
		}
		else
			strlcpy (outName, fullPath, inNameLength);
	}
	else
		outName[0] = 0;
		
badParam:
	return;
}

void SetCreatorForPath (const char *inPath, OSType inCreator, OSType inType)
{
	CFStringRef theFileName = NULL;
	FSRef fileRef;
	FSSpec fileSpec;
	CFURLRef theFileURL = NULL;
	Boolean success;
	OSErr err;

	theFileName = CFStringCreateWithCString (CFAllocatorGetDefault(), inPath, kCFStringEncodingMacRoman);
	if (!theFileName) goto bail;
	theFileURL = CFURLCreateWithFileSystemPath (CFAllocatorGetDefault(), theFileName, kCFURLPOSIXPathStyle, false);
	if (!theFileURL) goto bail;
	success = CFURLGetFSRef (theFileURL, &fileRef);
	if (!success) goto bail;
	
	err = FSGetCatalogInfo( &fileRef, kFSCatInfoNone, NULL, NULL, &fileSpec, NULL );

	FInfo finderInfo;
	
	FSpGetFInfo (&fileSpec, &finderInfo);
	finderInfo.fdCreator = inCreator;
	finderInfo.fdType = inType;
	FSpSetFInfo (&fileSpec, &finderInfo);
	
bail:
	if (theFileURL) CFRelease (theFileURL);
	if (theFileName) CFRelease (theFileName);
	
	return;
}


/* returns true if the FSSpec is a folder */
extern "C" Boolean IsFolder (const FSSpec *spec)
{
	CInfoPBRec	pb;
	Str63		fName;
	OSStatus		err;

	PLstrcpy (fName, spec->name);
	pb.hFileInfo.ioNamePtr = fName;
	pb.hFileInfo.ioVRefNum = spec->vRefNum;
	pb.hFileInfo.ioFDirIndex = 0;
	pb.hFileInfo.ioDirID = spec->parID;
	pb.hFileInfo.ioCompletion = NULL;
	err = PBGetCatInfoSync (&pb);
	
	if (err==noErr)
		if (pb.hFileInfo.ioFlAttrib & ioDirMask) return true;
	
	return false;
}