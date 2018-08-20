/*##########################################################################

	MacAppleEvents.c

	Handles all AppleEvents for MacMAME.

##########################################################################*/

#include <Carbon/Carbon.h>

#include "driver.h"
#include "info.h"

#include "mac.h"

// AppleEvent Support
static pascal OSErr HandleOpenApplication(const AppleEvent *theAppleEvent, AppleEvent *reply, long myRefCon);
static pascal OSErr HandleOpenDocuments(const AppleEvent *theAppleEvent, AppleEvent *reply, long myRefCon);
static pascal OSErr HandlePrintDocuments(const AppleEvent *theAppleEvent, AppleEvent *reply, long myRefCon);
static pascal OSErr HandleQuit(const AppleEvent *theAppleEvent, AppleEvent *reply, long myRefCon);
static pascal OSErr HandleGetDriverList(const AppleEvent *theAppleEvent, AppleEvent *reply, long myRefCon);
static OSErr 		RequiredCheck(const AppleEvent *theAppleEvent);

#pragma mark е AppleEvent Support

//===============================================================================
//	InitializeAppleEvents
//
//	Install handlers for the 4 basic AppleEvents.
//===============================================================================

void InitializeAppleEvents(void)
{
	AEInstallEventHandler(kCoreEventClass, kAEOpenApplication, NewAEEventHandlerUPP(HandleOpenApplication), 0, false);
	AEInstallEventHandler(kCoreEventClass, kAEOpenDocuments, NewAEEventHandlerUPP(HandleOpenDocuments), 0, false);
	AEInstallEventHandler(kCoreEventClass, kAEPrintDocuments, NewAEEventHandlerUPP(HandlePrintDocuments), 0, false);
	AEInstallEventHandler(kCoreEventClass, kAEQuitApplication, NewAEEventHandlerUPP(HandleQuit), 0, false);

	AEInstallEventHandler(kMAMEEventClass, kMAMEGetDriverList, NewAEEventHandlerUPP(HandleGetDriverList), 0, false);
}							


//===============================================================================
//	HandleOpenApplication
//
//	Initialize app through the OAPP AppleEvent.
//===============================================================================

pascal OSErr HandleOpenApplication(const AppleEvent *inAppleEvent, AppleEvent *outReply, long inRefCon)
{
	OSErr err;

	(void)outReply;
	(void)inRefCon;

	// check for required parameters
	err = RequiredCheck(inAppleEvent);
	if (err != noErr)
		return err;
	
	// force us to exit back to the frontend
	gDragAndDropMode = false;
	gEmulationPaused = false;
	gExitToFrontend = true;

	// reset the active ROMset
	gActiveRomset.driver = NULL;

	return noErr;
}


//===============================================================================
//	HandleOpenDocuments
//
//	Open a file through the ODOC AppleEvent.
//===============================================================================

pascal OSErr HandleOpenDocuments(const AppleEvent *inAppleEvent, AppleEvent *outReply, long inRefCon)
{
	SInt32		itemsInList;
	Size		actualSize;
	DescType	typeCode;
	FSSpec 		fileSpec;
	AEDescList	docList;
	AEKeyword	keyword;
	OSErr		err;

	(void)outReply;
	(void)inRefCon;

	// get the list as a descriptor
	err = AEGetParamDesc(inAppleEvent, keyDirectObject, typeAEList, &docList);
	if (err != noErr)
		return err;
	
	// check for required parameters
	err = RequiredCheck(inAppleEvent);
	if (err != noErr)
		return err;

	// count the number of items	
	err = AECountItems(&docList, &itemsInList);
	if (err != noErr)
		return err;

	// just grab the first item, coercing it to a filespec
	err = AEGetNthPtr(&docList, 1, typeFSS, &keyword, &typeCode, (Ptr)&fileSpec, sizeof(FSSpec), &actualSize);
	if (err != noErr)
		return err;

#if 1 // FRONTEND ееееее
	// convert it to a ROMset
	ConvertFileSpecToROMSet(&fileSpec, &gActiveRomset);
#endif

	// force us to exit to the frontend and handle it as a drag & drop event
	gEmulationPaused = false;
	gExitToFrontend = true;
	gDragAndDropMode = true;
	
	return noErr;
}


//===============================================================================
//	HandlePrintDocuments
//
//	Print a file through the PDOC AppleEvent.
//===============================================================================

pascal OSErr HandlePrintDocuments(const AppleEvent *inAppleEvent, AppleEvent *outReply, long inRefCon)
{
	(void)inAppleEvent;
	(void)outReply;
	(void)inRefCon;

	return errAEEventNotHandled;
}


//===============================================================================
//	HandleQuit
//
//	Quit app through the QUIT AppleEvent.
//===============================================================================

pascal OSErr HandleQuit(const AppleEvent *inAppleEvent, AppleEvent *outReply, long inRefCon)
{
	OSErr err;

	(void)outReply;
	(void)inRefCon;

	// check for required parameters
	err = RequiredCheck (inAppleEvent);
	if (err != noErr)
		return err;
	
	// set us up to go away
	gExitToShell = true;	
	return noErr;
}

//===============================================================================
//	HandleGetDriverList
//
// TODO
//===============================================================================

pascal OSErr HandleGetDriverList(const AppleEvent *inAppleEvent, AppleEvent *outReply, long inRefCon)
{
	OSErr err;
	int i, count;
	MacMAMEGameDriver *newList;
	MacMAMEGameDriver *curList;
	const game_driver *driver;
	
	(void)outReply;
	(void)inRefCon;

	// count total number of drivers
	for (count=0; drivers[count]; count++) {}
	
	newList = malloc (count * sizeof (MacMAMEGameDriver));
	if (!newList) return memFullErr;
	
	i = 0;
	curList = newList;
	while (drivers[i])
	{
		driver = drivers[i];
		strcpy (curList->name, driver->name);
		strcpy (curList->source_file, driver->source_file);
		strcpy (curList->clone_of, driver->clone_of->name);
		strcpy (curList->year, driver->year);
		strcpy (curList->manufacturer, driver->manufacturer);
		strcpy (curList->description, driver->description);
		curList->flags = driver->flags;

		curList ++;
		i ++;
	}
	
	err = AEPutParamPtr (outReply, kMAMEGetDriverList, typeChar, newList, count * sizeof (MacMAMEGameDriver));
	free (newList);
	
	return noErr;
}



//===============================================================================
//	RequiredCheck
//
//	Make sure all the required parameters were passed to us.
//===============================================================================

OSErr RequiredCheck(const AppleEvent *inAppleEvent)
{
	DescType typeCode;
	Size actualSize;
	OSErr err;

	// try to get a wildcard attribute for missed keywords
	err = AEGetAttributePtr(inAppleEvent, keyMissedKeywordAttr, typeWildCard, &typeCode, 0L, 0, &actualSize);
	
	// if we couldn't, that's great
	if (err == errAEDescNotFound)
		return noErr;
		
	// if we could, that's bad
	if (err == noErr) 
		return errAEEventNotHandled;

	return err;
}
