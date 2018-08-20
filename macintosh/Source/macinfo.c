/*##########################################################################

	macinfo.c

	Handles the "info" tab of the main frontend dialog.

	¥¥¥ÊWe use WASTE now instead of TextEdit because styled TextEdit is
	broken quite badly in OSX (go figure).

##########################################################################*/

#define USE_WASTE	1

#include <Carbon/Carbon.h>
#include <QuickTime/QuickTime.h>

#include <string.h>

#include "driver.h"

#include "mac.h"
#include "macextras.h"
#include "macinfo.h"
#include "macreports.h"
#include "macstrings.h"
#include "macutils.h"

#if USE_WASTE
	#include "WASTE.h"
#endif

#include "audit.h"
#include "datafile.h"
#include "unzip.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#ifdef __MWERKS__
#pragma require_prototypes on
#endif

/*##########################################################################
	CONSTANTS
##########################################################################*/

// info panel popup menu items
enum
{
	kInfoMenuScreenshot		= 1,
	kInfoMenuScreenshotTitle,
	kInfoMenuHardwareInfo,
	kInfoMenuHistory,
	kInfoMenuMiniAudit,
	kInfoMenuCabinetArt,
	kInfoMenuFlyerArt,
	kInfoMenuMarqueeArt,
	kInfoMenuControlPanel
};


// info tab dialog item offsets
enum
{
	kInfoSelectPopup			= 0,
	kInfoDataUser,
	kInfoDataImage,
	kInfoNotWorkingText
};



/*##########################################################################
	TYPEDEFS
##########################################################################*/

typedef struct
{
#if USE_WASTE
	WEReference					te;					// reference to WASTE record
#else
	TEHandle					te;					// handle to TextEdit record
#endif
	Rect						lastBounds;			// the last bounds of the user control
	ControlRef					scrollbar;			// handle to vertical scrollbar
	ControlRef					imageWell;			// handle to screenshot image well
	ControlRef					staticText;			// handle to "(not working)" static text
	short						menu;				// currently selected popup menu item
	short						linesPerPage;		// number of text lines visible
	ControlActionUPP			scrollActionUPP;	// custom action proc for scrollbar
	ControlUserPaneDrawUPP		userPaneDrawUPP;	// custom draw proc for UserPane
	ControlUserPaneHitTestUPP	userPaneHitTestUPP;	// custom hitTestProc for UserPane
	ControlUserPaneTrackingUPP	userPaneTrackingUPP;// custom trackingProc for UserPane
}
tInfoData;


/*##########################################################################
	GLOBALS
##########################################################################*/

static tInfoData	sInfoData;				// private data associated with Info UserPane
#if USE_WASTE
static WEReference	sInfoTE;				// temporary copy of sInfoData.te
#else
static TEHandle		sInfoTE;				// temporary copy of sInfoData.te
#endif

static const game_driver *sCurrentGame;

/*##########################################################################
	FUNCTION PROTOTYPES
##########################################################################*/

static OSErr 		GetScaledPicture(PicHandle srcPic, PicHandle *dstPic, const Rect *dstRect, const Rect *frame);
static int 			compareImageZips(const void *n1, const void *n2);
static void 		GetSortedImageZips(const char *zipname, short vRefNum, long dirID, Str31 **namesArray, int *total);
static OSErr	 	FindImageFile(const char *gamename, int shotType, FSSpec *spec, Handle *hand, OSType *componentSubType);
static OSErr	 	GetImageFileAsPict(const FSSpec *spec, Handle hand, OSType gitype, PicHandle *pict);
static OSErr		LoadScreenshotIntoIW(const char *gamename, int clone, ControlRef imageWell, int shotType, int notWorking);
static OSErr	 	FSpGetFolderDirID(const FSSpec *spec, long *dirID);

static void 		InfoDrawGenericWell(const Rect *inRect, ThemeDrawState inState, Boolean inFillCenter);
static tInfoData *	InfoGetData(ControlRef inUserPane);
static Rect *		InfoGetTextRect(ControlRef inUserPane, Rect *outRect);
static void 		InfoScrollText(ControlRef inUserPane);
static pascal void 	infoScrollAction(ControlRef inScrollbar, ControlPartCode inPart);
static pascal void 	infoDrawProc(ControlRef inUserPane, SInt16 inPart);
static pascal ControlPartCode infoHitTestProc(ControlRef inUserPane, Point inWhere);
static pascal ControlPartCode infoTrackingProc(ControlRef inUserPane, Point inStartPt, ControlActionUPP inActionProc);
static void 		InfoAdjustScrollbar(ControlRef inUserPane);

static void 		txtprint(char *s, ...);
static void 		txtInsert(char *buf);
static void 		txtJust(short align);
static void 		txtFont(StringPtr name);
static void 		txtFace(Style face);
static void 		txtSize(short size);
static void 		txtColor(RGBColor *rgb);
static void 		txtClear(void);

static void 		InfoTextHardwareInfo(const game_driver *inGame);
static void 		InfoTextHistory(const game_driver *inGame);
static void 		InfoTextMiniAudit(const game_driver *inGame);
/*static void 		InfoTextInputs1(const game_driver *inGame);
static void 		InfoTextInputs2(const game_driver *inGame);*/



#pragma mark ¥ Main Interface Functions

//===============================================================================
//	InfoInitializeUserPane
//
//	Initializes and allocates everything needed for the UserPane. Pass in
//	the dialog and the item number of the UserPane control.
//
//	You must call this function before calling other Info functions. Before
//	disposing the UserPane with ShortenDITL(), call InfoTearDownUserPane().
//===============================================================================

void InfoInitializeUserPane(DialogRef inDialog, DialogItemIndex inBaseItem)
{
	ControlRef	userPane;
	Rect		textRect, viewRect, scrollbarRect;
	OSErr		err;
	
	// set the current menu item
	SetDialogControlValue(inDialog, inBaseItem + kInfoSelectPopup, gFEPrefs.infoMenu);
	
	// retrieve the ControlRef of our UserPane item
	err = GetDialogItemAsControl(inDialog, inBaseItem + kInfoDataUser, &userPane);
	if (err) return;
	
	// store a pointer to the private data in the refcon
	SetControlReference(userPane, (UInt32)&sInfoData);
	
	// store the ControlRef for the image well
	err = GetDialogItemAsControl(inDialog, inBaseItem + kInfoDataImage, &sInfoData.imageWell);
	if (err) return;
	
	// store the ControlRef for the "(not working)" static text
	err = GetDialogItemAsControl(inDialog, inBaseItem + kInfoNotWorkingText, &sInfoData.staticText);
	if (err) return;

	// install our custom ControlUserPaneDrawProc
	sInfoData.userPaneDrawUPP = NewControlUserPaneDrawUPP(infoDrawProc);
	if (!sInfoData.userPaneDrawUPP)
	{
		err = memFullErr;
		goto bail;
	}
	err = SetControlData(userPane, kControlEntireControl, kControlUserPaneDrawProcTag,
							sizeof(ControlUserPaneDrawUPP), (Ptr)&sInfoData.userPaneDrawUPP);
	if (err) goto bail;
	
	// install our custom ControlUserPaneHitTestProc
	sInfoData.userPaneHitTestUPP = NewControlUserPaneHitTestUPP(infoHitTestProc);
	if (!sInfoData.userPaneHitTestUPP)
	{
		err = memFullErr;
		goto bail;
	}
	err = SetControlData(userPane, kControlEntireControl,  kControlUserPaneHitTestProcTag,
							sizeof(ControlUserPaneHitTestUPP),
							(Ptr)&sInfoData.userPaneHitTestUPP);
	if (err) goto bail;
	
	// install our custom ControlUserPaneTrackingProc
	sInfoData.userPaneTrackingUPP = NewControlUserPaneTrackingUPP(infoTrackingProc);
	if (!sInfoData.userPaneTrackingUPP)
	{
		err = memFullErr;
		goto bail;
	}
	err = SetControlData(userPane, kControlEntireControl,  kControlUserPaneTrackingProcTag,
							sizeof(ControlUserPaneTrackingUPP),
							(Ptr)&sInfoData.userPaneTrackingUPP);
	if (err) goto bail;


	// calculate the rect that encloses the text area of the pane	
	(void)InfoGetTextRect(userPane, &textRect);

	// create a TextEdit record
	sInfoData.lastBounds = viewRect = textRect;
	InsetRect(&viewRect, 4, 4);
#if USE_WASTE
	{
		LongRect	longViewRect;
		WERectToLongRect (&viewRect, &longViewRect);
		err = WENew (&longViewRect, &longViewRect, weDoUseTempMem, &sInfoData.te);
		if (err != noErr)
			sInfoData.te = NULL;		
	}
#else
	sInfoData.te = TEStyleNew(&viewRect, &viewRect);
#endif
	if (!sInfoData.te)
	{
		err = memFullErr;
		goto bail;
	}
	
	// create a live-feedback variant of an Appearance scrollbar
	scrollbarRect = textRect;
	InsetRect(&scrollbarRect, -1, -1);
	scrollbarRect.left = scrollbarRect.right - kScrollbarWidth - 1;

	sInfoData.scrollbar = NewControl(GetDialogWindow(inDialog),
							&scrollbarRect,
							"\p",
							false,
						 	0,
						 	0,
						 	0,
						 	kControlScrollBarLiveProc,
						 	(UInt32)userPane);

	if (sInfoData.scrollbar)
	{
		ControlRef	parent;
		
		// embed the scrollbar into the the same owner as the user pane
		// this makes the tabs in the main dialog work properly
		GetSuperControl(userPane, &parent);
		EmbedControl(sInfoData.scrollbar, parent);
		
		// initially hide the scrollbar
		SetControlVisibility(sInfoData.scrollbar, false, false);

		// set the control action proc for the scrollbar
		sInfoData.scrollActionUPP = NewControlActionUPP(infoScrollAction);
		if (sInfoData.scrollActionUPP)
		{
			SetControlAction(sInfoData.scrollbar, sInfoData.scrollActionUPP);
		}
		else
		{
			err = memFullErr;
			goto bail;
		}
	}
	else
	{
		err = memFullErr;
		goto bail;
	}
	
	// start with an invalid menu
	sInfoData.menu = -1;
	
	// reset the current game
	sCurrentGame = NULL;
	
bail:
	if (err != noErr)
		InfoTearDownUserPane(inDialog, inBaseItem);
}


//===============================================================================
//	InfoUpdateUserPane
//
//	Change the UserPane content as appropriate for the specified game and menu.
//	Specify a negative game index if no valid game_driver is selected.
//	Pass in the dialog and item number of the UserPane control.
//
//	You must call InfoInitializeUserPane() before calling this function.
//===============================================================================

void InfoUpdateUserPane(DialogRef inDialog, DialogItemIndex inBaseItem, const game_driver *inGame)
{
	SInt16		menuItem = GetDialogControlValue(inDialog, inBaseItem + kInfoSelectPopup);
	ControlRef	userPane;
	tInfoData	*data;
	Rect		paneRect;
	Rect		viewRect;
	Rect		tempRect;
	Rect		empty = { 0, 0, 0, 0 };
	RgnHandle	saveClip;
	OSErr		err;
	
	// retrieve the ControlRef for our UserPane
	err = GetDialogItemAsControl(inDialog, inBaseItem + kInfoDataUser, &userPane);
	if (err) return;
	
	// retrieve a pointer to the private data
	data = InfoGetData(userPane);

	// detect if UserPane moved and move scrollbar to new location
	InfoGetTextRect(userPane, &tempRect);
	if (!EqualRect(&tempRect, &data->lastBounds))
	{
		SInt32		xDelta = tempRect.left - data->lastBounds.left;
		SInt32		yDelta = tempRect.top - data->lastBounds.top;
		
		data->lastBounds = tempRect;
		
		// adjust the bounds of the TextEdit record as well
		if (data->te)
		{
#if USE_WASTE
			LongRect longViewRect, longDestRect;
			WEGetViewRect(&longViewRect, data->te);
			WEGetDestRect(&longDestRect, data->te);
			WEOffsetLongRect(&longViewRect, xDelta, yDelta);
			WEOffsetLongRect(&longDestRect, xDelta, yDelta);
			WESetViewRect(&longViewRect, data->te);
			WESetDestRect(&longDestRect, data->te);
#else
			OffsetRect(&(*data->te)->viewRect, xDelta, yDelta);
			OffsetRect(&(*data->te)->destRect, xDelta, yDelta);
#endif
		}
	}
	
	// skip if we're already displaying this game and this menu
	if (inGame == sCurrentGame && menuItem == data->menu)
		return;
	sCurrentGame = inGame;

	// if there's no game, hide stuff
	if (!inGame)
	{
		// If it's not a game, hide the various controls
		SetControlVisibility(data->imageWell, false, false);
		SetControlVisibility(data->staticText, false, false);
		SetControlVisibility(data->scrollbar, false, false);

		// force UserPane to be redrawn
		(void)GetControlBounds(userPane, &paneRect);
		InsetRect(&paneRect, -2, -2);
		InvalWindowRect(GetDialogWindow(inDialog), &paneRect);

		// make menu invalid if the game is not a valid game
		SetDialogControlActive(inDialog, inBaseItem + kInfoSelectPopup, false);
		data->menu = -1;
		return;
	}

	// make the menu active again	
	SetDialogControlActive(inDialog, inBaseItem + kInfoSelectPopup, true);

	// hide or unhide the "(not working)" static text as appropriate
	SetControlVisibility(data->staticText, IsBroken(inGame), true);

	// handle the image well types
	if (menuItem == kInfoMenuScreenshot ||
		menuItem == kInfoMenuScreenshotTitle ||
		menuItem == kInfoMenuCabinetArt ||
		menuItem == kInfoMenuFlyerArt ||
		menuItem == kInfoMenuControlPanel ||
		menuItem == kInfoMenuMarqueeArt)
	{
		const game_driver *driver = inGame;
		
		// reload the screenshot if something changed
		while (1)
		{
			Boolean isClone = IsClone(driver);
			
			// if parent shots are disabled, don't call this a clone
			if (gFEPrefs.fe_listNoParentShots)
				isClone = false;
			
			// attempt to load this screenshot
			err = LoadScreenshotIntoIW(driver->name, isClone, data->imageWell, menuItem, IsBroken(inGame));
			
			// if we succeeded, or if we've disabled parent shots, break out
			if (err == noErr || gFEPrefs.fe_listNoParentShots)
				break;
			
			// back up a level; stop at the root
			driver = driver->clone_of;
			if (!driver)
				break;
		}

		// do nothing unless the menu changed
		if (menuItem != data->menu)
		{
			// make image well visible
			SetControlVisibility(data->imageWell, true, false);
							
			// make sure text scrollbar is hidden
			SetControlVisibility(data->scrollbar, false, false);

			// force UserPane to be redrawn
			(void)GetControlBounds(userPane, &paneRect);
			InsetRect(&paneRect, -2, -2);
			InvalWindowRect(GetDialogWindow(inDialog), &paneRect);
		}

		// remember the menu			
		data->menu = menuItem;
		return;
	}

	// if we got here, it's one of the text types
	if (menuItem != data->menu)
		// make sure screenshot image well is hidden
		SetControlVisibility(data->imageWell, false, false);
	
	// hack: hide automatic TextEdit updating (when TEInsert is called)
	saveClip = NewRgn();
	SetPortDialogPort(inDialog);
	GetClip(saveClip);
	ClipRect(&empty);
	
	// nullify previous changes to the TERecord
	(void)InfoGetTextRect(userPane, &viewRect);
	InsetRect(&viewRect, 4, 4);
#if USE_WASTE
	{
		LongRect longViewRect;
		WERectToLongRect(&viewRect, &longViewRect);
		WESetViewRect(&longViewRect, data->te);
		WESetDestRect(&longViewRect, data->te);
	}
#else
	(**data->te).viewRect = viewRect;
	(**data->te).destRect = viewRect;
#endif

	// create the specified text
	sInfoTE = data->te;
	if (sInfoTE)
		switch (menuItem)
		{
			case kInfoMenuHardwareInfo:
				InfoTextHardwareInfo(inGame);
				break;
			case kInfoMenuMiniAudit:
				InfoTextMiniAudit(inGame);
				break;
			case kInfoMenuHistory:
				InfoTextHistory(inGame);
				break;
		}

	// restore original clipping region
	SetClip(saveClip);
	DisposeRgn(saveClip);
	
	// let TextEdit recalculate
#if USE_WASTE
	(void)WECalText(data->te);
#else
	TECalText(data->te);
#endif
	
	// adjust the vertical scrollbar (hide if necessary)
	InfoAdjustScrollbar(userPane);

	// remember the menu			
	data->menu = menuItem;

	// update immediately to avoid getting autoKey before updateEvt
	Draw1Control(userPane);
	(void)GetControlBounds(userPane, &paneRect);
	InsetRect(&paneRect, -2, -2);
	ValidWindowRect(GetDialogWindow(inDialog), &paneRect);
}


//===============================================================================
//	InfoHandleUserPane
//
//	Dispose of everything associated with the Info UserPane. Should be called
//	before removing the UserPane with ShortenDITL().
//===============================================================================

void InfoHandleUserPane(DialogRef inDialog, DialogItemIndex inBaseItem, DialogItemIndex inItemHit, const game_driver *inGame)
{
	if (inItemHit == kInfoSelectPopup)
		InfoUpdateUserPane(inDialog, inBaseItem, inGame);
}


//===============================================================================
//	InfoTearDownUserPane
//
//	Dispose of everything associated with the Info UserPane. Should be called
//	before removing the UserPane with ShortenDITL().
//===============================================================================

void InfoTearDownUserPane(DialogRef inDialog, short inBaseItem)
{
	ControlRef	userPane;
	tInfoData	*data;
	OSErr		err;
	
	// get the current menu item
	gFEPrefs.infoMenu = GetDialogControlValue(inDialog, inBaseItem + kInfoSelectPopup);
	
	err = GetDialogItemAsControl(inDialog, inBaseItem + kInfoDataUser, &userPane);
	if (err) return;
	
	data = InfoGetData(userPane);

	if (data->te)
	{
#if USE_WASTE
		WEDispose(data->te);
#else
		TEDispose(data->te);
#endif
		data->te = NULL;
	}
	
	if (data->scrollbar)
	{
		DisposeControl(data->scrollbar);
		data->scrollbar = NULL;
	}
	
	if (data->scrollActionUPP)
	{
		DisposeControlActionUPP(data->scrollActionUPP);
		data->scrollActionUPP = NULL;
	}
	
	if (data->imageWell)
	{
		ControlButtonContentInfo	info;
		OSErr						err;
		
		// hide the image well
		SetControlVisibility(data->imageWell, false, false);

		// dispose of any PicHandle we might have created
		err = GetImageWellContentInfo(data->imageWell, &info);
		if (err == noErr)
		{
			if (info.contentType == kControlContentPictHandle)
			{
				KillPicture(info.u.picture);
				info.contentType = kControlContentPictRes;
				info.u.resID = 129;
				err = SetImageWellContentInfo(data->imageWell, &info);
			}
		}
		data->imageWell = NULL;
	}
	
	if (data->staticText)
	{
		// hide the static text
		SetControlVisibility(data->staticText, false, false);
		data->staticText = NULL;
	}
	
	if (data->userPaneDrawUPP)
	{
		DisposeControlUserPaneDrawUPP(data->userPaneDrawUPP);
		data->userPaneDrawUPP = NULL;
	}
	
	if (data->userPaneHitTestUPP)
	{
		DisposeControlUserPaneHitTestUPP(data->userPaneHitTestUPP);
		data->userPaneHitTestUPP = NULL;
	}
	
	if (data->userPaneTrackingUPP)
	{
		DisposeControlUserPaneTrackingUPP(data->userPaneTrackingUPP);
		data->userPaneTrackingUPP = NULL;
	}
}


//===============================================================================
//	InfoKeyDownHandler
//
//	Handle key down events when the info tab is visible.
//===============================================================================

Boolean InfoKeyDownHandler(DialogRef inDialog, DialogItemIndex inBaseItem, EventRecord *inEvent, DialogItemIndex *outItemHit)
{
	UInt8		charCode = inEvent->message & charCodeMask;

	// allow arrow keys to change info panel popup
	if ((charCode == kLeftArrowCharCode || charCode == kRightArrowCharCode) && ((inEvent->modifiers & cmdKey) == 0))
	{
		ControlRef 		popupControl;
		SInt32			newValue;
	
		// get a handle to the info popup
		GetDialogItemAsControl(inDialog, inBaseItem + kInfoSelectPopup, &popupControl);
		newValue = GetControlValue(popupControl);

		// adjust the new value
		if (charCode == kLeftArrowCharCode)
			newValue--;
		else
			newValue++;
			
		// wrap around
		if (newValue < GetControlMinimum(popupControl))
			newValue = GetControlMaximum(popupControl);
		else if (newValue > GetControlMaximum(popupControl))
			newValue = GetControlMinimum(popupControl);
			
		// change the popup value and pretend it was clicked
		SetControlValue(popupControl, newValue);
		*outItemHit = kInfoSelectPopup;
		return true;
	}
	
	// if we get here, we didn't handle it
	return false;
}


//===============================================================================
//	InfoHandleMouseWheelMoved
//
//	Mousewheel handler for the info tab: scrolls the text in the Info UserPane.
//===============================================================================

OSStatus InfoHandleMouseWheelMoved(DialogRef inDialog, DialogItemIndex inBaseItem,
									Point QDLocation, EventMouseWheelAxis axis, SInt32 delta)
{
	OSStatus result;
	ControlRef userPane;
	const tInfoData *data;
	Rect bounds;


	// retrieve the ControlRef for our UserPane
	result = GetDialogItemAsControl(inDialog, inBaseItem + kInfoDataUser, &userPane);

	if (result == noErr)
	{
		// retrieve a pointer to the private data
		data = InfoGetData(userPane);

		// check that the scroll bar is visible
		if (IsControlVisible(data->scrollbar))
		{
			GetControlBounds(userPane, &bounds);
			if (PtInRect(QDLocation, &bounds))
			{
				switch (axis)
				{
				case kEventMouseWheelAxisX:
					// We don't care about this event, but we can pass it on
					result = eventNotHandledErr;
					break;

				case kEventMouseWheelAxisY:
					// Now, this is interesting.

					// adjust the scrollbar value
					SetControlValue(data->scrollbar, GetControlValue(data->scrollbar) - delta);

					// scroll text to match the new scrollbar value
					InfoScrollText(userPane);
					break;
				}
			}
		}
	}

	return result;
}

#pragma mark -
#pragma mark ¥ÊScreenshot Routines

static OSErr GetScaledPicture(PicHandle srcPic, PicHandle *dstPic, const Rect *dstRect,
	const Rect *frame)
{
	OpenCPicParams	header;
	CGrafPtr		oldPort;
	GDHandle		oldDevice;
	GWorldPtr		world;
	PixMapHandle	pmh;
	Rect			r;
	OSErr			err;

	// create an offscreen GWorld for drawing pic into (use temporary memory) 	
	GetGWorld(&oldPort, &oldDevice);
	err = NewGWorld(&world, 0, frame, NULL, NULL, useTempMem);
	if (err==noErr)
	{
		SetGWorld(world, NULL);
		pmh = GetGWorldPixMap(world);
		LockPixels(pmh);
		
		// start with black background 
		PaintRect(frame);

		// center destination rect within frame 
		r = *dstRect;
		OffsetRect(&r, (frame->right - dstRect->right) / 2, (frame->bottom - dstRect->bottom) / 2);
		
		// draw original picture, scaled to destination rect 
		DrawPicture(srcPic, &r);
			
		// make a new picture from the scaled image 
		header.srcRect = *frame;
		header.hRes = header.vRes = 0x00480000;	// 72 dpi 
		header.version = -2;
		header.reserved1 = header.reserved2 = NULL;
	
		// first try creating PICT in application heap 
		*dstPic = OpenCPicture(&header);
		ClipRect(frame);
		CopyBits((BitMapPtr)*pmh, (BitMapPtr)*pmh, frame, frame, srcCopy, NULL);
		ClosePicture();
			
		// if it failed, try again using the system heap 
		if (EmptyRect(&(**dstPic)->picFrame))
		{
			// if it failed, no mas 
			if (EmptyRect(&(**dstPic)->picFrame))
			{
				KillPicture(*dstPic);
				*dstPic = NULL;
				err = memFullErr;
			}
		}

		SetGWorld(oldPort, oldDevice);
		DisposeGWorld(world);
	}
	
	return err;
}


//===============================================================================
//	compareImageZips
//
//	qsort compare function for sorting image zipfiles into specific sort order.
//	See GetSortedImageZips() below.
//===============================================================================

static int compareImageZips(const void *n1, const void *n2)
{
	// n1 is compared against n2.
	// These are the return values:
	//
	// -1 moves n1 closer to the start of the list
	// 0 indicates a tie between n1 and n2
	// 1 moves n1 closer to the end of the list
	
	char name1[32], name2[32], basename[32], temp[32];
	char *p;
	int c1, c2;
	
	// convert names into C strings for easier handling
	p2cstrcpy(name1, (unsigned char *)n1);
	p2cstrcpy(name2, (unsigned char *)n2);

	// extrapolate basename by looking for first '_' or '.' in the name
	strcpy(basename, name1);
	for (p = basename; *p; p++)
	{
		if (*p == '_' || *p == '.')
		{
			*p = '\0';
			break;
		}
	}
	
	// "images.zip" is last in search order, so it always loses
	sprintf(temp, "%s.zip", basename);
	if (mame_stricmp(name1, temp) == 0) return 1;
	if (mame_stricmp(name2, temp) == 0) return -1;
	if (strlen (name2) > strlen (temp))
	{
		// If it's a longer string than the other, it always wins (e.g. images_10 vs. images_9)
		if (strlen (name1) > strlen (name2)) return -1;
		
		// If we get here, they're both 2-digit numbered files, so we compare those numbers
		c1 = (name1[strlen(basename) + 1] - 0x30) * 10 + (name1[strlen(basename) + 2] - 0x30);
		c2 = (name2[strlen(basename) + 1] - 0x30) * 10 + (name2[strlen(basename) + 2] - 0x30);
	}
	else
	{
		// we have to compare the magic character after the underscore
		c1 = tolower(name1[strlen(basename) + 1]);
		c2 = tolower(name2[strlen(basename) + 1]);

		// force alpha characters [a-z] to lose to digits [0-9]
		if (c1 >= 'a' && c1 <= 'z') c1 -= 'a';
		if (c2 >= 'a' && c2 <= 'z') c2 -= 'a';
	}

	// higher digits win, and digits win over alpha characters
	return c2 - c1;
}


//===============================================================================
//	GetSortedImageZips
//
//	Searches for all valid image zipfiles in the specified folder, sorts
//	them, sets filesArray to point to an array of the sorted names, and sets
//	total to the number of valid screenshot zipfiles found.
//
//	Pass the base name (i.e., "images.zip") in the zipname param. Pass the
//	vRefNum and dirID of the Screenshots (or whatever) folder.
//
//	The sort order is images_[9-0].zip, then images_[z-a].zip, then images.zip.
//	This is a feature added specifically to make it easier for the maintainers of
//	the screenshot archive to provide update zipfiles that override the previous
//	zipfiles, and to put clone screenshots in "images_c.zip" and neogeo
//	screenshots in "images_n.zip".
//===============================================================================

static void GetSortedImageZips(const char *zipname, short vRefNum, long dirID,
									Str31 **namesArray, int *total)
{
	char			basename[32];
	static Str31	filenames[20];
	int				matches = 0;
	
	// remove the .zip extension to get the basename
	strcpy(basename, zipname);
	basename[strlen(zipname) - 4] = '\0';
	
	// find all image zipfiles in specified folder with valid names
	{
		CInfoPBRec	pb;
		Str255		name;
		short		x = 1;
		OSErr		err = noErr;

		while ((noErr == err) && (matches < 20))
		{	
			pb.hFileInfo.ioNamePtr = name;
			pb.hFileInfo.ioVRefNum = vRefNum;
			pb.hFileInfo.ioFDirIndex = x;
			pb.hFileInfo.ioDirID = dirID;
			pb.hFileInfo.ioFlAttrib = 0;

			err = PBGetCatInfoSync(&pb);
			if (err) continue;
			
			// only files
			if (0 == (pb.hFileInfo.ioFlAttrib & ioDirMask))
			{
				char temp[32];
				char matchname[32];
				Boolean	valid = false;

				// check for a match against the base zipname (i.e., "images.zip")
				p2cstrcpy(temp, name);				
				if (mame_stricmp(temp, zipname) == 0)
				{
					valid = true;
				}
				else
				{
					// check for other valid names (i.e., "images_0.zip" through images_9.zip, _a through _z)
					sprintf(matchname, "%s_?.zip", basename);
					if (strlen(temp) == strlen(matchname))
					{
						temp[strlen(basename)+1] = '?';
						if (mame_stricmp(temp, matchname) == 0)
						{
							int c = tolower(name[strlen(basename)+2]);
							
							if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z'))
							{
								valid = true;
							}
						}
					}

					// check for other valid names (i.e., "images_10.zip" through images_99.zip)
					sprintf(matchname, "%s_??.zip", basename);
					if (strlen(temp) == strlen(matchname))
					{
						temp[strlen(basename)+1] = '?';
						temp[strlen(basename)+2] = '?';
						if (mame_stricmp(temp, matchname) == 0)
						{
							int c1 = tolower(name[strlen(basename)+2]);
							int c2 = tolower(name[strlen(basename)+3]);
							
							if ((c1 >= '1' && c1 <= '9') && (c2 >= '0' && c2 <= '9'))
							{
								valid = true;
							}
						}
					}
				}
				
				if (valid)
				{
					// the filename is valid, so add it to the list
					PLstrcpy(filenames[matches], name);
					matches++;
				}
			}
				
			// advance to next item in directory
			x++;
		}
	}
	
	// sort the found filenames into appropriate sort order
	if (matches)
	{
		qsort(filenames, matches, sizeof(Str31), compareImageZips);
	}
	
	// return a pointer to the filenames array and the number of matching files
	*namesArray = filenames;
	*total = matches;
}


// Locate the appropriate image file for this gamename and shotType.
// Result is either (1) an FSSpec to the file, or (2) a handle to the image data 
//		and a componentSubtype identifying the appropriate GraphicsImporterComponent.

static OSErr FindImageFile(const char *gamename, int shotType, FSSpec *spec, Handle *hand,
	OSType *componentSubType)
{
	FSSpec	folder, zipfile;
	Str31	*zipnames;
	int		numzips;
	OSErr	err;
	
	// determine the appropriate folder and appropriate zipfile
	{
		char	zipName[32];
		
		switch (shotType)
		{
			case kInfoMenuScreenshot:
				err = GetFSSpecForMAMEFolder (FILETYPE_SCREENSHOT, &folder);
				strcpy(zipName, GetIndCString(rStrings, kZIPImages, "images.zip"));
				break;
			case kInfoMenuScreenshotTitle:
				err = GetFSSpecForMAMEFolder (MAC_FILETYPE_TITLE_SCREENSHOTS, &folder);
				strcpy(zipName, GetIndCString(rStrings, kZIPTitleImages, "titles.zip"));
				break;
			case kInfoMenuCabinetArt:
				err = GetFSSpecForMAMEFolder (MAC_FILETYPE_CABINET_ARTWORK, &folder);
				strcpy(zipName, GetIndCString(rStrings, kZIPCabinets, "cabinets.zip"));
				break;
			case kInfoMenuFlyerArt:
				err = GetFSSpecForMAMEFolder (MAC_FILETYPE_FLYER_ARTWORK, &folder);
				strcpy(zipName, GetIndCString(rStrings, kZIPFlyers, "flyers.zip"));
				break;
			case kInfoMenuMarqueeArt:
				err = GetFSSpecForMAMEFolder (MAC_FILETYPE_MARQUEE_ARTWORK, &folder);
				strcpy(zipName, GetIndCString(rStrings, kZIPMarquees, "marquees.zip"));
				break;
			case kInfoMenuControlPanel:
				err = GetFSSpecForMAMEFolder (MAC_FILETYPE_CONTROL_PANEL_ARTWORK, &folder);
				strcpy(zipName, GetIndCString(rStrings, kZIPControlPanels, "cpanel.zip"));
				break;
			default:
				return paramErr;
				break;
		}
		
		// get an FSSpec for the image folder
		if (err) return err;
		
		// form an incomplete FSSpec for possible zipfiles within that folder
		zipfile.vRefNum = folder.vRefNum;
		err = FSpGetFolderDirID(&folder, &zipfile.parID);
		if (err) return err;
		
		// get a sorted list of image zipfiles in that folder
		GetSortedImageZips(zipName, zipfile.vRefNum, zipfile.parID, &zipnames, &numzips);
	}		

	{
		// first look for an individual file with appropriate extension
		char *ext[] = { "jpg", "gif", "png", "bmp", "pic", 0 };
		char filename[32];
		int i, j;

		// try each possible extension
		for (i = 0; ext[i] != 0; i++)
		{
			sprintf(filename, "%s.%s", gamename, ext[i]);
			err = GetPartialPathSpec(&folder, filename, spec);
			if (noErr == err)
			{
				*hand = NULL;
				return noErr;
			}
		}
		
		// no individual file was found, so look for files within zipfiles
		err = fnfErr;
		for (j = 0; j < numzips; j++)
		{
			static OSType	gitype[] = { 'JPG ', 'GIF ', 'PNG ', 'BMP ', 'PIC ' };
			char 			path[kMacMaxPath];
			zip_file		*zip = NULL;
			Boolean			found = false;
			zip_entry	 	*entry = nil;
			OSErr			tempErr;
		
			// complete the zipfile FSSpec using the current zipfile name
			PLstrcpy(zipfile.name, zipnames[j]);
			
			// get a full path to the zipfile suitable for openzip()
			tempErr = GetFullPathFromSpec(&zipfile, (UInt8 *)path, sizeof (path));
			if (tempErr) continue;

			// try to open the zipfile
			zip = openzip(-1, 0, path);
			if (!zip) continue;

			// check each file in the zipfile for a match
			while (!found && (entry = readzip(zip)) != 0)
			{
				// ignore directories within zipfile
				if (entry->name[strlen(entry->name) - 1] != '/')
				{
					const char *base = strrchr(entry->name, '/');
					if (base)
						++base;
					else
						base = entry->name;
					
					// check against each possible filename
					for (i = 0; (ext[i] != 0) && (!found); i++)
					{
						sprintf (filename, "%s.%s", gamename, ext[i]);
						if (!mame_stricmp(base, filename))
						{
							found = true;
							*componentSubType = gitype[i];
						}
					}								
				}
			}
			
			if (found)
			{
				// found zipped image file -- try to uncompress
				*hand = TempNewHandle(entry->uncompressed_size, &tempErr);
				
				if (noErr == tempErr)
				{
					HLockHi(*hand);
					if (readuncompresszip(zip, entry, **hand))
					{
						// uncompress failed
						err = badFileFormat;
						DisposeHandle(*hand);
					}
					else
					{
						// successfully uncompressed
						spec->name[0] = '\0';
						HUnlock(*hand);
						closezip(zip);
						return noErr;
					}
				}
			}
			closezip(zip);
		} // end for
	}
	
	return err;
}


// Creates a PICT from either (1) an FSSpec to an image file, or (2) a handle to image data
//		and a componentSubtype identifying the appropriate GraphicsImporterComponent.

static OSErr GetImageFileAsPict(const FSSpec *spec, Handle hand, OSType gitype, PicHandle *pict)
{
	OSErr	err;
	GraphicsImportComponent ci;

	if (hand)
	{
		// setup component for data in a handle (read from a zipped file)
		err = OpenADefaultComponent(GraphicsImporterComponentType, gitype, &ci);
		if (noErr == err)
			(void)GraphicsImportSetDataHandle(ci, hand);
	}
	else
	{
		// setup component to read image data from a file
		err = GetGraphicsImporterForFile(spec, &ci);
	}
		
	if (noErr == err)
	{
		// first try to create PICT in application heap
		err = GraphicsImportGetAsPicture(ci, pict);
		CloseComponent(ci);
	}
		
	if (hand)
		DisposeHandle(hand);

	return err;
}


static OSErr LoadScreenshotIntoIW(const char *gamename, int clone, ControlRef imageWell, int shotType, int notWorking)
{
	ControlButtonContentInfo info;
	FSSpec		spec;
	Handle		hand;
	OSType		componentSubType;
	PicHandle	pict;
	OSErr		err;
	char		ssNotAvailableName[256];
	
	// Dispose of the prior PicHandle (if any)
	err = GetImageWellContentInfo(imageWell, &info);
	if (err) return err;
	if (info.contentType == kControlContentPictHandle) {
		KillPicture(info.u.picture);
		info.contentType = kControlNoContent;	// avoid double free if clone pict isn't found
		SetImageWellContentInfo(imageWell, &info);
	}

	// Try to find the appropriate image for this gamename and shotType
	err = FindImageFile(gamename, shotType, &spec, &hand, &componentSubType);

    // Image not found, look for "Not Working" image in folder if game not working
	if ((fnfErr == err) && notWorking) // -43 file not found
	{
		strcpy(ssNotAvailableName,
			GetIndCString(rStrings, kScreenshotNotWorkingFileName,
				"Not Working")); //mamelang.rsrc rStrings: insert STR#128/30

		err = FindImageFile(ssNotAvailableName, shotType, &spec, &hand, &componentSubType);
	}

	// Image still not found or game is working and no image found,
	// look for "Not Available" image in folder
	if (fnfErr == err) // -43 file not found
	{
		strcpy(ssNotAvailableName,
			GetIndCString(rStrings, kScreenshotNotAvailableFileName,
				"Not Available")); //mamelang.rsrc rStrings: insert STR#128/31

		err = FindImageFile(ssNotAvailableName, shotType, &spec, &hand, &componentSubType);
	}

	if (noErr == err)
	{
		// Try to load image as a PICT
		err = GetImageFileAsPict(&spec, hand, componentSubType, &pict);
	}

	if (noErr == err)
	{
		Rect		srcRect = (**pict).picFrame;
		Rect		dstRect;
		Rect		frame;
		PicHandle	newPic;

		(void)GetControlBounds(imageWell, &dstRect);
		(void)GetControlBounds(imageWell, &frame);
		
		// leave a small border (for looks)
		InsetRect(&dstRect, 2, 2);
		
		// normalize rects
		OffsetRect(&srcRect, -srcRect.left, -srcRect.top);
		OffsetRect(&dstRect, -dstRect.left, -dstRect.top);
		OffsetRect(&frame, -frame.left, -frame.top);
		
		// if it won't fit in image well rect
		if (srcRect.bottom > dstRect.bottom || srcRect.right > dstRect.right)
		{
			// scale properly to fit within image well rect
			float scale;
	
			if (srcRect.bottom > srcRect.right)
			{
				scale = (float)dstRect.bottom / (float)srcRect.bottom;
				dstRect.right = (float)srcRect.right * scale;
			}
			else
			{
				scale = (float)dstRect.right / (float)srcRect.right;
				dstRect.bottom = (float)srcRect.bottom * scale;
			}
		}
		else
		{
			// otherwise use original size
			dstRect = srcRect;
		}
		
		err = GetScaledPicture(pict, &newPic, &dstRect, &frame);
		KillPicture(pict);
		pict = newPic;
	}

	// for clones, return failure immediately -- we'll be called again with the parent name
	if (err && clone && !gFEPrefs.fe_listNoParentShots) return err;
	
	if (err)
	{
		// If it failed, set the image well back to our "None Found" PICT
		info.contentType = kControlContentPictRes;
		info.u.resID = 129;
	}
	else
	{
		// Otherwise, set the image well to our new picture handle
		info.contentType = kControlContentPictHandle;
		info.u.picture = pict;
	}
	
	// Update the contents of the image well
	if (noErr == SetImageWellContentInfo(imageWell, &info))
		DrawOneControl(imageWell);

	return err;
}


// returns the dirID of the folder described by the FSSpec 
static OSErr FSpGetFolderDirID(const FSSpec *spec, long *dirID)
{
	CInfoPBRec	pb;
	Str255		fName;
	OSErr		err;

	PLstrcpy(fName, spec->name);
	pb.hFileInfo.ioNamePtr = fName;
	pb.hFileInfo.ioVRefNum = spec->vRefNum;
	pb.hFileInfo.ioFDirIndex = 0;
	pb.hFileInfo.ioDirID = spec->parID;
	pb.hFileInfo.ioCompletion = NULL;
	err = PBGetCatInfoSync(&pb);
	
	if (err == noErr)
	{
		if (pb.hFileInfo.ioFlAttrib & ioDirMask)
			*dirID = pb.hFileInfo.ioDirID;
		else
			err = paramErr;
	}	
	return err;
}


#pragma mark -
#pragma mark ¥ Info UserPane


//===============================================================================
//	InfoDrawGenericWell
//
//	Draws an image well frame. Specify the rectangle around which to draw the
//	image well frame, the state (active or inactive), and whether to fill the
//	frame with white.
//
//	Calls DrawThemeGenericWell() when available.
//===============================================================================

static void InfoDrawGenericWell(const Rect *inRect, ThemeDrawState inState, Boolean inFillCenter)
{
	Rect	r, temp;
	OSErr	err;

	// call Appearance 1.1 function if available
	if ((void *)DrawThemeGenericWell != (void *)kUnresolvedCFragSymbolAddress)
	{
		err = DrawThemeGenericWell(inRect, inState, inFillCenter);
		return;
	}
		
	PushRGB();
	PushPenState();

	// make sure our pen is normal
	PenNormal();
	
	// draw white center if requested
	if (inFillCenter)
	{
		RGBForeColor(&rgbWhite);
		PaintRect(inRect);
	}

	// inRect specifies the rectangle around which to draw the image well frame
 	r = *inRect;
	InsetRect(&r, -2, -2);

	if (inState == kThemeStateActive)
	{	
		// draw gray shadow	
		RGBForeColor(&rgbGrey50);
		SetRect(&temp, r.left, r.top, r.right - 1, r.bottom - 1);
		FrameRect(&temp);
		
		// draw white hilite
		RGBForeColor(&rgbWhite);
		SetRect (&temp, r.left + 1, r.top + 1, r.right, r.bottom);
		FrameRect (&temp);
		
		// draw black frame
		RGBForeColor(&rgbBlack);
		InsetRect(&r, 1, 1);
		FrameRect(&r);
	}
	else if (inState == kThemeStateInactive)
	{
		// erase frame area to background color
		SetRect(&temp, r.left, r.top, r.left + 2, r.bottom);
		EraseRect(&temp);
		SetRect(&temp, r.left, r.top, r.right, r.top + 2);
		EraseRect(&temp);
		SetRect(&temp, r.right - 2, r.top, r.right, r.bottom);
		EraseRect(&temp);
		SetRect(&temp, r.left, r.bottom - 2, r.right, r.bottom);
		EraseRect(&temp);
		
		// draw gray frame
		RGBForeColor(&rgbGrey50);
		InsetRect(&r, 1, 1);
		FrameRect(&r);
	}
	
	PopPenState();
	PopRGB();	
}


//===============================================================================
//	InfoGetData
//
//	Retrieve a pointer to the private data of the Info UserPane control.
//===============================================================================

static tInfoData *InfoGetData(ControlRef inUserPane)
{
	return (tInfoData *)GetControlReference(inUserPane);
}


//===============================================================================
//	InfoGetTextRect
//
//	Calculate the rectangle enclosing the text area of the Info UserPane.
//===============================================================================

static Rect *InfoGetTextRect(ControlRef inUserPane, Rect *outRect)
{
	// start with the entire UserPane rect
	(void)GetControlBounds(inUserPane, outRect);

	// leave room at the bottom for the "(not working)" static text control
	outRect->bottom -= (5 + 16 + 5);
	
	return outRect;
}


//===============================================================================
//	InfoScrollText
//
//	Scrolls the text vertically an appropriate amount based on the value of the
//	UserPane's vertical scrollbar control.
//===============================================================================

static void InfoScrollText(ControlRef inUserPane)
{
	const tInfoData	*data = InfoGetData(inUserPane);
	
	if (data->te)
	{
		short	lines;
		short	old_y, new_y;

		// get the number of lines scrolled from the scrollbar control
		lines = GetControlValue(data->scrollbar);
		if (lines == 0)
		{
			// pixel height of 0 lines is 0
			new_y = 0;
		}
		else
		{
			// calculate pixel height of lines
#if USE_WASTE
			new_y = WEGetHeight(0L, lines, data->te);
#else
			new_y = TEGetHeight(lines, 1, data->te);
#endif
		}
		
		// calculate pixel amount currently scrolled
#if USE_WASTE
	{
		LongRect viewRect, destRect;
		WEGetViewRect (&viewRect, data->te);
		WEGetDestRect (&destRect, data->te);
		old_y = viewRect.top - destRect.top;
	}
#else
		old_y = (**data->te).viewRect.top - (**data->te).destRect.top;
#endif
		
		// hack: restore white background color before calling TEScroll
		PushRGB();
		RGBBackColor(&rgbWhite);

		// scroll to the new pixel amount
#if USE_WASTE
		WEScroll(0L, old_y - new_y, data->te);
#else
		TEScroll (0, old_y - new_y, data->te);
#endif
		
		PopRGB();
	}
}


//===============================================================================
//	infoScrollAction
//
//	Control action proc for scrolling the text in the Info UserPane.
//===============================================================================

static pascal void infoScrollAction(ControlRef inScrollbar, ControlPartCode inPart)
{
	ControlRef		userPane;
	const tInfoData	*data;
	short			delta = 0;

	// retrieve a reference to the UserPane and a pointer to its private data
	userPane = (ControlRef)GetControlReference(inScrollbar);
	data = InfoGetData(userPane);
	
	// determine number of lines and which direction to scroll
	switch (inPart)
	{
		case kControlUpButtonPart: 
			delta = -1;
			break;
		case kControlDownButtonPart: 
			delta = 1;
			break;
		case kControlPageUpPart: 
			delta = -(data->linesPerPage);
			break;
		case kControlPageDownPart: 
			delta = data->linesPerPage;
			break;
		case kControlNoPart:
			return;
		default:
			break;
	}
	
	// adjust the scrollbar value
	if (delta)
		SetControlValue(inScrollbar, delta + GetControlValue(inScrollbar));

	// scroll text to match the new scrollbar value
	InfoScrollText(userPane);
}


//===============================================================================
//	infoDrawProc
//
//	Custom UserPaneDrawProc for our Info UserPane control.
//===============================================================================

static pascal void infoDrawProc(ControlRef inUserPane, SInt16 inPart)
{
	#pragma unused (inPart)
	const tInfoData	*data;
	Rect			paneRect, textRect;

	// retrieve a pointer to the private data
	data = InfoGetData(inUserPane);
	
	// start by erasing entire UserPane bounds to the background color
	(void)GetControlBounds(inUserPane, &paneRect);
	InsetRect(&paneRect, -2, -2);
	EraseRect(&paneRect);
	
	// draw the static text control (which may be hidden and therefore not drawn)
	if (data->staticText)
		Draw1Control(data->staticText);
	
	switch (data->menu)
	{
		case kInfoMenuScreenshot:
		case kInfoMenuCabinetArt:
		case kInfoMenuFlyerArt:
		case kInfoMenuMarqueeArt:
		case kInfoMenuScreenshotTitle:
		case kInfoMenuControlPanel:
			// just draw the image well and static text controls
			if (data->imageWell)
				Draw1Control(data->imageWell);
			break;

		case kInfoMenuHardwareInfo:
		case kInfoMenuHistory:
		case kInfoMenuMiniAudit:
		{
			ThemeDrawState	drawState;
			
			// draw a "well" border around the text area, filled with white
			drawState = IsControlActive(inUserPane) ? kThemeStateActive : kThemeStateInactive;
			(void)InfoGetTextRect(inUserPane, &textRect);
			InfoDrawGenericWell(&textRect, drawState, true); 

			// draw the scrollbar (which may be invisible and therefore not drawn)
			if (data->scrollbar)
				Draw1Control(data->scrollbar);
			
			// draw the text itself
			if (data->te)
			{
				PushRGB();
				RGBForeColor(&rgbBlack);
				RGBBackColor(&rgbWhite);
#if USE_WASTE
				{
					Pattern whitePat;
					BackPat(GetQDGlobalsWhite(&whitePat));
					
					PushPenState();
					PenNormal();
				
					WEUpdate(NULL, data->te);
					
					PopPenState();				
				}
#else
				TEUpdate(&textRect, data->te);
#endif
				PopRGB();
			}
			break;
		}
	}
}


//===============================================================================
//	infoHitTestProc
//
//	Custom hitTestProc for our Info UserPane control.
//	This allows FindControlUnderMouse() to locate our control, which allows
//	ModalDialog() to call TrackControl() or HandleControlClick() for our control.
//===============================================================================

static pascal ControlPartCode infoHitTestProc(ControlRef inUserPane, Point inWhere)
{
	const tInfoData	*data;
	ControlPartCode	part;
	Rect			scrollbarRect;

	data = InfoGetData(inUserPane);
	
	// is the click in our scrollbar?
	(void)GetControlBounds(data->scrollbar, &scrollbarRect);
	if (IsControlVisible(data->scrollbar) && PtInRect(inWhere, &scrollbarRect))
	{
		// return a valid part code so HandleControlClick() will be called
		part = kControlIndicatorPart;
	}
	else
	{
		// not in our text
		part = kControlNoPart;
	}
	
	return part;
}


//===============================================================================
//	infoTrackingProc
//
//	Custom trackingProc for our Info UserPane control.
//	This won't be called for our control unless the kControlHandlesTracking feature
//	bit is specified when the userPane is created. You specify feature bits by
//	putting them in the "initial" field of the 'CNTL' resource.
//===============================================================================

static pascal ControlPartCode infoTrackingProc (
					ControlRef inUserPane,
					Point inStartPt,
					ControlActionUPP inActionProc)
{
	#pragma unused (inActionProc)
	const tInfoData	*data = InfoGetData(inUserPane);
	
	TrackControl(data->scrollbar, inStartPt, (ControlActionUPP)-1);
	
	return kControlNoPart;
}


//===============================================================================
//	InfoAdjustScrollbar
//
//	Adjust the Info text scrollbar for the amount of text in the TextEdit record.
//===============================================================================

static void InfoAdjustScrollbar(ControlRef inUserPane)
{
	tInfoData	*data;
	short		lines;

	// retrieve a pointer to the private data
	data = InfoGetData(inUserPane);

#if USE_WASTE
	lines = WECountLines(data->te);
#else
	lines = (**data->te).nLines;
#endif
	if (lines)
	{
#if USE_WASTE
		LongRect longViewRect;
		short	viewHeight;
		short	textHeight;
		
		WEGetViewRect(&longViewRect, data->te);
		viewHeight = longViewRect.bottom - longViewRect.top;
		textHeight = WEGetHeight(0L, lines, data->te);
#else
		short	viewHeight = (**data->te).viewRect.bottom - (**data->te).viewRect.top;
		short	textHeight = TEGetHeight(lines, 1, data->te);
#endif
		
		if (textHeight > viewHeight)
		{
			// text won't fit -- recalculate with smaller rect (to allow for scrollbar)
#if USE_WASTE
			WEGetViewRect(&longViewRect, data->te);
			longViewRect.right -= kScrollbarWidth;
			WESetViewRect(&longViewRect, data->te);
			
			WEGetDestRect(&longViewRect, data->te);
			longViewRect.right -= kScrollbarWidth;
			WESetDestRect(&longViewRect, data->te);
			
			(void)WECalText(data->te);
			lines = WECountLines(data->te);
			textHeight = WEGetHeight(0L, lines, data->te);
#else
			(**data->te).viewRect.right -= kScrollbarWidth;
			(**data->te).destRect.right -= kScrollbarWidth;
			TECalText(data->te);
			lines = (**data->te).nLines;
			textHeight = TEGetHeight(lines, 1, data->te);
#endif
			
			// remember number of lines per page
			data->linesPerPage = viewHeight / (textHeight / lines);

			// set control value, minimum, maximum, view size
			SetControlValue(data->scrollbar, 0);
			SetControlMinimum(data->scrollbar, 0);
			SetControlMaximum(data->scrollbar, lines - data->linesPerPage);
			if ((void *)SetControlViewSize != (void *)kUnresolvedCFragSymbolAddress)
				SetControlViewSize (data->scrollbar, data->linesPerPage);
			
			// make the scrollbar visible
			SetControlVisibility(data->scrollbar, true, false);
			
			return;
		}
	}

	// text fits -- hide the scrollbar
	SetControlVisibility(data->scrollbar, false, false);
}



#pragma mark -


//===============================================================================
//	txtprint
//
//	Treat the Info TextEdit record like stdout.
//
//	Don't try to print more than 511 characters.
//===============================================================================

static void txtprint(char *s, ...)
{
	char buf[512];
	int		i;
	va_list	ap;

	va_start(ap, s);
	vsprintf(buf, s, ap);
	va_end(ap);
	
	/* fix newlines */
	for (i = 0; i < strlen(buf); i++)
		if (buf[i] == '\n')
			buf[i] = '\r';
			
	if (sInfoTE)
#if USE_WASTE
		WEInsert(buf, strlen(buf), NULL, NULL, sInfoTE);
#else
		TEInsert(buf, strlen(buf), sInfoTE);
#endif
}


//===============================================================================
//	txtInsert
//
//	Insert the specified zero-terminated buffer into the Info TextEdit record.
//===============================================================================

static void txtInsert(char *buf)
{
	int i;
	
	/* fix newlines */
	for (i = 0; i < strlen(buf); i++)
		if (buf[i] == '\n')
			buf[i] = '\r';
			
	if (sInfoTE)
#if USE_WASTE
		WEInsert(buf, strlen(buf), NULL, NULL, sInfoTE);
#else
		TEInsert(buf, strlen(buf), sInfoTE);
#endif
}


//===============================================================================
//	txtJust
//
//	Change justification for the entire Info TextEdit record.
//===============================================================================

static void txtJust(short align)
{
	if (sInfoTE)
#if USE_WASTE
		WESetAlignment(align, sInfoTE);
#else
		TESetAlignment(align, sInfoTE);
#endif
}


//===============================================================================
//	txtFont
//
//	Change font of all text subsequently txtprinted or txtInserted.
//===============================================================================

static void txtFont(StringPtr name)
{
	TextStyle	style;
	
	if (sInfoTE)
	{
		GetFNum(name, &style.tsFont);
#if USE_WASTE
		(void)WESetStyle(weDoFont, &style, sInfoTE);
#else
		TESetStyle(doFont, &style, false, sInfoTE);
#endif
	}
}


//===============================================================================
//	txtFace
//
//	Change style (face) of all text subsequently txtprinted or txtInserted.
//===============================================================================

static void txtFace(Style face)
{
	TextStyle	style;
	
	if (sInfoTE)
	{
		style.tsFace = face;
#if USE_WASTE
		(void)WESetStyle(weDoFace, &style, sInfoTE);
#else
		TESetStyle(doFace, &style, false, sInfoTE);
#endif
	}
}


//===============================================================================
//	txtSize
//
//	Change size of all text subsequently txtprinted or txtInserted.
//===============================================================================

static void txtSize(short size)
{
	TextStyle	style;
	
	if (sInfoTE)
	{
		style.tsSize = size;
#if USE_WASTE
		(void)WESetStyle(weDoSize, &style, sInfoTE);
#else
		TESetStyle(doSize, &style, false, sInfoTE);
#endif
	}
}


//===============================================================================
//	txtColor
//
//	Change color of all text subsequently txtprinted or txtInserted.
//===============================================================================

static void txtColor(RGBColor *rgb)
{
	TextStyle	style;
	
	if (sInfoTE)
	{
		style.tsColor = *rgb;
#if USE_WASTE
		(void)WESetStyle(weDoColor, &style, sInfoTE);
#else
		TESetStyle(doColor, &style, false, sInfoTE);
#endif
	}
}


//===============================================================================
//	txtClear
//
//	Empty the Info TextEdit record.
//===============================================================================

static void txtClear(void)
{
	if (sInfoTE)
	{
#if USE_WASTE
		WESetSelection(0L, WEGetTextLength(sInfoTE), sInfoTE);
		(void)WEDelete(sInfoTE);
		txtFace(normal);
#else
		unsigned char buf[1] = { 0 };
		
		// set text to empty
		TESetText(buf, 0, sInfoTE);
#endif
	}
}


//===============================================================================
//	InfoTextHardwareInfo
//
//	Must be kept up to date with displaygameinfo() in usrintrf.c
//  TODO: UI labels need localization lookup
//===============================================================================

static void InfoTextHardwareInfo(const game_driver *inGame)
{
	int		i, speakercount;
	machine_config driver;

	expand_machine_driver(inGame->drv, &driver);
	
	txtClear();
	txtJust(teCenter);
	txtFont("\pTimes");
	txtFace(bold);
	txtSize(14);

	txtprint("%s\n%s %s\n\nCPU:\n",inGame->description,inGame->year,inGame->manufacturer);
	i = 0;
	while (i < MAX_CPU && driver.cpu[i].cpu_type)
	{
		int type,clock,count;

		type = driver.cpu[i].cpu_type;
		clock = driver.cpu[i].cpu_clock;
		count = 1;
		i++;

		while (i < MAX_CPU
				&& driver.cpu[i].cpu_type == type
				&& driver.cpu[i].cpu_clock == clock)
		{
			count++;
			i++;
		}

		if (count > 1)
			txtprint("%d x ",count);

		txtprint("%s",cputype_name(type));
	
		if (clock >= 1000000)
			txtprint(" %d.%06d MHz",
					clock / 1000000,
					clock % 1000000);
		else
			txtprint(" %d.%03d kHz",
					clock / 1000,
					clock % 1000);
		
		txtprint("\n");
	}

	i = 0;
	speakercount = 0;
	while (i < MAX_SPEAKER && driver.speaker[i].tag)
	{
		speakercount++;
		i++;
	}
	
	txtprint("\nSound");
	if (speakercount)
	{
		if		(speakercount == 1) txtprint(" (mono)");
		else if (speakercount == 2) txtprint(" (stereo)");
		else						txtprint(" (%d speakers)", speakercount);
	}
	txtprint(":\n");
	
	i = 0;
	while (i < MAX_SOUND && driver.sound[i].sound_type)
	{	
		int type,clock,count;

		type = driver.sound[i].sound_type;
		clock = driver.sound[i].clock;
		count = 1;
		i++;

		while (i < MAX_SOUND
				&& driver.sound[i].sound_type == type
				&& driver.sound[i].clock == clock)
		{
			count++;
			i++;
		}

		if (count > 1)
			txtprint("%d x ",count);

		txtprint("%s",sndtype_name(type));

		if (clock)
		{
			if (clock >= 1000000)
				txtprint(" %d.%06d MHz",
						clock / 1000000,
						clock % 1000000);
			else
				txtprint(" %d.%03d kHz",
						clock / 1000,
						clock % 1000);
		}
		
		txtprint("\n");
	}

	if (driver.video_attributes & VIDEO_TYPE_VECTOR)
		txtprint("\nVector Game\n");
	else
	{
		int pixelx,pixely,tmax,tmin,rem;

		pixelx = 4 * (driver.default_visible_area.max_y - driver.default_visible_area.min_y + 1);
		pixely = 3 * (driver.default_visible_area.max_x - driver.default_visible_area.min_x + 1);

		/* calculate MCD */
		if (pixelx >= pixely)
		{
			tmax = pixelx;
			tmin = pixely;
		}
		else
		{
			tmax = pixely;
			tmin = pixelx;
		}
		while ((rem = tmax % tmin) != 0)
		{
			tmax = tmin;
			tmin = rem;
		}
		/* tmin is now the MCD */

		pixelx /= tmin;
		pixely /= tmin;

		/* If it's a horizontal game but the vertical size is at least twice as large */
		/* assume it's a dual-monitor setup like Punchout */
		if (!(inGame->flags & ORIENTATION_SWAP_XY) && (pixelx / 2 > pixely))
			pixelx /= 2;

		txtprint("\nScreen resolution:\n");
		txtprint("%d x %d (%s) %f Hz\npixel aspect ratio %d:%d\n",
				driver.default_visible_area.max_x - driver.default_visible_area.min_x + 1,
				driver.default_visible_area.max_y - driver.default_visible_area.min_y + 1,
				(inGame->flags & ORIENTATION_SWAP_XY) ? "V" : "H",
				driver.frames_per_second,
				pixelx,pixely);
	}

	if (inGame->bios)
	{
		txtprint("\nBIOS:\n");
		if (!IsClone(inGame))
			txtprint("%s (%s)\n", inGame->clone_of->description,
					 inGame->clone_of->name);
		else
			txtprint("%s (%s)\n", inGame->clone_of->clone_of->description,
					 inGame->clone_of->clone_of->name);
	}

	/**
	 * Check for analog & digital controls
	 */
	/* Reference:
	 * src/inptport.h: enum {}
	 * src/info.c: print_game_input()
	 */
	{
		const input_port_entry* input;
		begin_resource_tracking();
		input = input_port_allocate(inGame->construct_ipt, NULL);
#ifndef MAX_PLAYERS
#	define MAX_PLAYERS	16 // proper way to do this? 16 is a guess based on 0.77
#endif	/* MAX_PLAYERS */
		
		/* Note that each player could have different controls, although this
		 * would be rare. e.g. the two-seater version of 'firetrk' "Fire Truck"
		 */
		const char* control[MAX_PLAYERS];
		int nbutton[MAX_PLAYERS];
		int maxplayersfound = 0;
		
		int num;
		Boolean analog_found = false;
		int nstart = 0;
		int ncoin = 0;
		int ndip = 0;
		Boolean service = false;
		int ncoinservice = 0;
		Boolean tilt = false;
		int nunknown = 0;
		Boolean analog_adjuster = false;
		int i;
		for (i=0; i<=MAX_PLAYERS; i++) {
			nbutton[i] = 0;
			control[i] = 0;
		}

		num = 0;
		while (input->type != IPT_END)
		{
			int playernum = 0;
			
			if (!analog_found
				&& ((input->type < __ipt_analog_end) && (input->type > __ipt_analog_start)))
			{
				analog_found = true;
					txtprint ("\nThis game has analog controls:\n");
			}
			
			if (playernum < input->player)
				playernum = input->player;
			
			if (playernum > maxplayersfound)
				maxplayersfound = playernum;
			if (input->type < IPT_UI_CONFIGURE)
			{
				switch (input->type)
				{
					// enum order from inptport.h may change in future!
					case IPT_JOYSTICK_UP:
					case IPT_JOYSTICK_DOWN:
					case IPT_JOYSTICK_LEFT:
					case IPT_JOYSTICK_RIGHT:
						if (input->four_way)
							control[playernum] = "joystick (4-way)"; // how to handle localization? l8n
						else
							control[playernum] = "joystick (8-way)";	// l8n?
						break;
					case IPT_JOYSTICKRIGHT_UP:
					case IPT_JOYSTICKRIGHT_DOWN:
					case IPT_JOYSTICKRIGHT_LEFT:
					case IPT_JOYSTICKRIGHT_RIGHT:
					case IPT_JOYSTICKLEFT_UP:
					case IPT_JOYSTICKLEFT_DOWN:
					case IPT_JOYSTICKLEFT_LEFT:
					case IPT_JOYSTICKLEFT_RIGHT:
						if (input->four_way)
							control[playernum] = "double joystick (4-way)";	// l8n?
						else
							control[playernum] = "double joystick (8-way)";	// l8n?
						break;
						
						// analog
					case IPT_PADDLE:
						txtprint("Paddle (Player %d)\n", playernum+1);
						break;
					case IPT_DIAL:
						txtprint("Dial (Player %d)\n", playernum+1);
						break;
					case IPT_TRACKBALL_X:
						txtprint("Trackball (Player %d)\n", playernum+1);
						break;
					case IPT_AD_STICK_X:
						txtprint("Analog joystick (Player %d)\n", playernum+1);
						break;
					case IPT_PEDAL:
						txtprint("Pedal (Player %d)\n", playernum+1);
						break;
					case IPT_PEDAL2:
						txtprint("Pedal 2 (Player %d)\n", playernum+1);
						break;
					case IPT_LIGHTGUN_X:
						txtprint("Light Gun (Player %d)\n", playernum+1);
						break;
						
					// digital
					case IPT_BUTTON1:
					case IPT_BUTTON2:
					case IPT_BUTTON3:
					case IPT_BUTTON4:
					case IPT_BUTTON5:
					case IPT_BUTTON6:
					case IPT_BUTTON7:
					case IPT_BUTTON8:
					case IPT_BUTTON9:
					case IPT_BUTTON10:
						if (nbutton[playernum] < (input->type - IPT_BUTTON1 + 1))
							nbutton[playernum] = input->type - IPT_BUTTON1 + 1;
						break;
						
					// start buttons
					case IPT_START1:
					case IPT_START2:
					case IPT_START3:
					case IPT_START4:
					case IPT_START5:
					case IPT_START6:
					case IPT_START7:
					case IPT_START8:
						if (nstart < (input->type - IPT_START1 + 1))
							nstart = input->type - IPT_START1 + 1;
						break;
						
					// coin slots
					case IPT_COIN1:
					case IPT_COIN2:
					case IPT_COIN3:
					case IPT_COIN4:
					case IPT_COIN5:
					case IPT_COIN6:
					case IPT_COIN7:
					case IPT_COIN8:
						if (ncoin < (input->type - IPT_COIN1 + 1))
							ncoin = input->type - IPT_COIN1 + 1;
						break;
						
					case IPT_ADJUSTER:
						analog_adjuster = true;
						break;
					
					// Service "coin"
					case IPT_SERVICE1:
						if (ncoinservice < 1) ncoinservice = 1;
						break;
					case IPT_SERVICE2:
						if (ncoinservice < 2) ncoinservice = 2;
						break;
					case IPT_SERVICE3:
						if (ncoinservice < 3) ncoinservice = 3;
						break;
					case IPT_SERVICE4:
						if (ncoinservice < 4) ncoinservice = 4;
						break;
						
					// other
					case IPT_SERVICE:
						service = true;
						break;
					case IPT_TILT:
						tilt = true;
						break;
						
					case IPT_DIPSWITCH_NAME:
						ndip++;
						break;
					case IPT_UNKNOWN:
						nunknown++;
						break;
						
					case IPT_PADDLE_V:
					case IPT_DIAL_V:
					case IPT_TRACKBALL_Y:
					case IPT_AD_STICK_Y:
					case IPT_AD_STICK_Z:
					case IPT_LIGHTGUN_Y:
					case IPT_PORT:
					case IPT_UNUSED:
					case IPT_DIPSWITCH_SETTING:
					case IPT_VBLANK:
						// ignore this "control"
						break;
					default:
#if MAME_DEBUG
						txtprint("Unknown input type: %d\n", input->type);
#endif
						break;
				}
			}
			input ++;
		}
		
		{
			txtprint ("\nThis game has digital controls:\n"); //always? I think usually at least 1 button or start button--ck
			int p = 0;
			for (p = 0; p <= maxplayersfound; ++p)
			{
				switch (nbutton[p])
				{
					case 0:
						txtprint("Player %d: (no buttons)\n");
						break;
					case 1:
						txtprint("Player %d: one button\n", p+1);
						break;
						//case 2: // future localization
					default:
						txtprint("Player %d: (%d) buttons\n", p+1, nbutton[p]);
						break;
				}
				// digital joysticks
				if (control[p])
					txtprint("Player %d: %s\n", p+1, control[p]);
			}
			
			switch (nstart)
			{
				case 0:
					break;
				case 1:
					txtprint("Start button: one\n");
					break;
					//case 2: // future localization
				default:
					txtprint("Start buttons: %d\n", nstart);
					break;
			}
			
#if MAME_DEBUG
			if (tilt || nunknown)
#else
			if (tilt)
#endif
				txtprint("\nOther inputs:\n");
			if (tilt)
				txtprint("Tilt sensor\n");

#if MAME_DEBUG
			switch (nunknown)
			{
				case 0:
					break;
				case 1:
					txtprint("Unknown control: one\n");
					break;
					//case 2: // future localization
				default:
					txtprint("Unknown controls: %d\n", nunknown);
					break;
			}
#endif
			
			if (ncoin || ncoinservice || ndip || service)
				txtprint("\nMisc:\n");
			if (analog_adjuster)
				txtprint("Analog adjuster\n");
			switch (ncoin)
			{
				case 0:
					break;
				case 1:
					txtprint("Coin slot: one\n");
					break;
					//case 2: // future localization
					// http://www.gnu.org/software/gettext/manual/html_node/gettext_150.html
				default:
					txtprint("Coin slots: %d\n", ncoin);
					break;
			}
			switch (ncoinservice)
			{
				case 0:
					break;
				case 1:
					txtprint("Service coin: one\n");
					break;
					//case 2: // future localization
				default:
					txtprint("Service coin: %d\n", ncoinservice);
					break;
			}
			switch (ndip)
			{
				case 0:
					break;
				case 1:
					txtprint("Dip-switch: one\n");
					break;
					//case 2: // future localization
				default:
					txtprint("Dip-switches: %d\n", ndip);
					break;
			}
			if (service)
				txtprint("Service switch\n");
		}
		end_resource_tracking();
	}
}


//===============================================================================
//	InfoTextHistory
//
//	Displays entry from history.dat and/or mameinfo.dat.
//===============================================================================

// 128k of storage for the history buffer.
#define kHistoryBufferSize	(128*1024)

static void InfoTextHistory(const game_driver *inGame)
{
	char *buf = 0;
	
	txtClear();

	/* allocate a buffer for history text */
	buf = malloc(kHistoryBufferSize);
	if (buf)
	{
		/* try to read driver history */
		if (!load_driver_history(inGame, buf, kHistoryBufferSize))
		{
			txtJust(teFlushLeft);
			txtFont("\pTimes");
			txtFace(0);
			txtSize(12);
			txtInsert(buf);
		}
		else
		{
			free(buf);
			buf = 0;
		}
	}
	
	if (buf)
	{
		free(buf);
		buf = 0;
	}
	else
	{
		txtJust(teCenter);
		txtFont("\pTimes");
		txtFace(bold + italic);
		txtSize(14);
		txtprint("\n\n\n\n\n%s",
			GetIndCString (rStrings, kInfoNoHistory, "history not available"));
	}
}


//===============================================================================
//	InfoTextMiniAudit
//
//	Displays a ROM audit for the specified game.
//===============================================================================

static void InfoTextMiniAudit(const game_driver *inGame)
{
	audit_record *aud;
	int total = -1;
	int index;
	char hashBuff[512];
	
	txtClear();
	txtJust(teFlushLeft);
	txtFont("\pMonaco");
	txtFace(bold);
	txtSize(9);

	txtprint("ROMS                %s\n\n",
		GetIndCString (rStrings, kInfoStatus, "STATUS"));
	
	/* audit the roms for this game (quietly) */
	gUnzipQuiet = true;
	index = GetDriverIndex(inGame);
	if (index != -1)
		total = audit_roms(index, &aud);
	gUnzipQuiet = false;
	
	// if the game has no ROMs, bail
	if (total == -1)
	{
		txtprint("This game uses no ROMs\n");
		return;
	}

	while (total--)
	{
		if ((aud->status == AUD_NOT_AVAILABLE) || (aud->status == AUD_ROM_NEED_REDUMP) || ((aud->status == AUD_OPTIONAL_ROM_NOT_FOUND)))
		{
			txtColor((struct RGBColor *) &rgbYellow);
			txtprint("%-20s", aud->rom);
			txtColor((struct RGBColor *) &rgbBlack);
		}
		else if ((aud->status != AUD_ROM_GOOD) && (aud->status != AUD_DISK_GOOD))
		{
			txtColor((struct RGBColor *) &rgbRed);
			txtprint("%-20s", aud->rom);
			txtColor((struct RGBColor *) &rgbBlack);
		}
		else
			txtprint("%-20s", aud->rom);

		switch (aud->status)
		{
			case AUD_ROM_NOT_FOUND:
			case AUD_DISK_NOT_FOUND:
				txtprint("%s\n",
					GetIndCString (rStrings, kInfoROMNotFound, "not found"));
				break;
			case AUD_ROM_NOT_FOUND_PARENT:
				txtprint("%s\n",
					GetIndCString (rStrings, kInfoROMNotFound_Parent, "not found (shared with parent)"));
				break;
			case AUD_ROM_NOT_FOUND_BIOS:
				txtprint("%s\n",
					GetIndCString (rStrings, kInfoROMNotFound_BIOS, "not found (BIOS)"));
				break;
			case AUD_NOT_AVAILABLE:
				txtprint("%s\n",
					GetIndCString (rStrings, kInfoROMNotFound, "not found"));
				txtprint("                    %s\n",
					GetIndCString (rStrings, kInfoNoGoodDump, "No good dump exists"));
				break;
			case AUD_BAD_CHECKSUM:
				hash_data_print (aud->hash, 0, hashBuff);
				txtprint("%s: %s\n",
					GetIndCString (rStrings, kInfoBadCRC, "bad crc"), hashBuff);
				hash_data_print (aud->exphash, 0, hashBuff);
				txtprint("                    %s: %s\n",
					GetIndCString (rStrings, kInfoExpected, "expected"), hashBuff);
				break;
			case AUD_ROM_NEED_REDUMP:
				txtprint("%s\n",
					GetIndCString (rStrings, kInfoNoGoodDump, "No good dump exists"));
				break;
			case AUD_ROM_NEED_DUMP:
				txtprint("%s\n",
					GetIndCString (rStrings, kInfoNoDumpKnown, "No dump exists"));
				break;
			case AUD_MEM_ERROR:
				txtprint("*%s*\n",
					GetIndCString (rStrings, kInfoMemError, "memory error"));
				break;
			case AUD_LENGTH_MISMATCH:
				txtprint("%s: 0x%06x\n", GetIndCString (rStrings, kInfoBadLength, "bad length"), aud->length);
				txtprint("                    %s: 0x%06x\n", GetIndCString (rStrings, kInfoExpected, "expected"), aud->explength);
				break;
// ¥¥¥ AUD_DISK_BAD_MD5 ?
//			case AUD_DISK_OLD_CHD:
//				txtprint("%s\n",
//					GetIndCString (rStrings, kInfoOldCHD, "CHD is old version"));
//				break;
			case AUD_ROM_GOOD:
			case AUD_DISK_GOOD:
				txtprint("%s\n",
					GetIndCString (rStrings, kInfoGood, "good"));
				break;
			default:
				txtprint("** %s **\n",
					GetIndCString (rStrings, kInfoUnknown, "unknown"));
				break;
		}
		aud++;
	}
}

#if 0

static void InfoTextInputs1 (const game_driver *inGame)
{
	input_port_entry *in;
	int p;
	int j;

	txtClear ();

	for (p = 1; p <= 4; ++p)
	{
		in = inGame->input_ports;
		
		while (in->type != IPT_END)
		{
			int playernum;
			UINT32 type;

			type = in->type & 0xff;
			playernum = ((in->type & IPF_PLAYERMASK) >> 16) + 1;
	//		if (((in->type & 0xff) > IPT_ANALOG_START) && ((in->type & 0xff) < IPT_ANALOG_END))
			if (playernum == p && ((in->type & 0xff) >= IPT_JOYSTICK_UP) && ((in->type & 0xff) < IPT_ANALOG_END))
			{
				const char * name;
				name = input_port_name(in);
				if (name == NULL || name == IP_NAME_DEFAULT)
					name = "(no name)";
				txtprint("%s\n", name);
			}
			
			in ++;
		}
	}
}

static void InfoTextInputs2 (const game_driver *inGame)
{
	input_port_entry *in;
	int p;
	int j;

	txtClear ();

	for (p = 1; p <= 4; ++p)
	{
		unsigned int buttons = 0;
		unsigned int joystick = 0;

		in = inGame->input_ports;
		
		while (in->type != IPT_END)
		{
			int playernum;
			UINT32 type;

			type = in->type & 0xff;
			playernum = ((in->type & IPF_PLAYERMASK) >> 16) + 1;
	//		if (((in->type & 0xff) > IPT_ANALOG_START) && ((in->type & 0xff) < IPT_ANALOG_END))
			if (playernum == p && ((in->type & 0xff) >= IPT_JOYSTICK_UP) && ((in->type & 0xff) < IPT_ANALOG_END))
			{
				switch (type)
				{
					case IPT_JOYSTICK_UP:
					case IPT_JOYSTICK_DOWN:
					case IPT_JOYSTICK_LEFT:
					case IPT_JOYSTICK_RIGHT:
					case IPT_JOYSTICKRIGHT_UP:
					case IPT_JOYSTICKRIGHT_DOWN:
					case IPT_JOYSTICKRIGHT_LEFT:
					case IPT_JOYSTICKRIGHT_RIGHT:
					case IPT_JOYSTICKLEFT_UP:
					case IPT_JOYSTICKLEFT_DOWN:
					case IPT_JOYSTICKLEFT_LEFT:
					case IPT_JOYSTICKLEFT_RIGHT:
						joystick |= (1 << (type - IPT_JOYSTICK_UP));
						break;
					
					case IPT_BUTTON1:
					case IPT_BUTTON2:
					case IPT_BUTTON3:
					case IPT_BUTTON4:
					case IPT_BUTTON5:
					case IPT_BUTTON6:
					case IPT_BUTTON7:
					case IPT_BUTTON8:
						buttons |= (1 << (type - IPT_BUTTON1));
						break;
					
					case IPT_PADDLE:
						txtprint ("P%d Analog Paddle\n", playernum);
						break;
					case IPT_DIAL:
						txtprint ("P%d Analog Dial\n", playernum);
						break;
					case IPT_TRACKBALL_X:
						txtprint ("P%d Analog Trackball X\n", playernum);
						break;
					case IPT_TRACKBALL_Y:
						txtprint ("P%d Analog Trackball Y\n", playernum);
						break;
					case IPT_AD_STICK_X:
						txtprint ("P%d Analog joystick X\n", playernum);
						break;
					case IPT_AD_STICK_Y:
						txtprint ("P%d Analog joystick Y\n", playernum);
						break;
					case IPT_PEDAL:
						txtprint ("P%d Pedal\n", playernum);
						break;
				}
			}
			
			in ++;
		}

		if (buttons != 0)
		{
			int i;
			const char * prefix = "";
			
			txtprint("p%d Buttons (", p);
			for (i = 0; i < 8; ++i)
			{
				if ((buttons & (1 << i)) != 0)
				{
					char numStr[2];
					numStr[0] = '1' + i;
					numStr[1] = 0;
					txtprint("%s%s", prefix, numStr);
					prefix = ", ";
				}
			}
			txtprint (")\n");
		}
		
		for (j = 0; j < 3; ++j)
		{
			const char * str = "";
			int flags = joystick & 0x0F;

			if (j == 1)
			{
				str = "Right ";
				flags = (joystick >> 4) & 0x0F;
			}
			else if (j == 2)
			{
				str = "Left ";
				flags = (joystick >> 8) & 0x0F;
			}

			if (flags != 0)
			{
				int i;
				const char * prefix = "";
				
				txtprint("p%d %sJoystick (", p, str);
				for (i = 0; i < 4; ++i)
				{
					if ((flags & (1 << i)) != 0)
					{
						switch (i)
						{
							case 0:		str = "Up";		break;
							case 1:		str = "Down";	break;
							case 2:		str = "Left";	break;
							case 3:		str = "Right";	break;
						}
						txtprint("%s%s", prefix, str);
						prefix = ", ";
					}
				}
				txtprint (")\n");
			}
		}
	}
}

#endif


