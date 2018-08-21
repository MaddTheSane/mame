/*##########################################################################

	macfrontend.c

	This code concerns itself with the user-interface dialog
	to choose a game.

	Original code by Evan Olcott, Brad Oliver, John Butler and Aaron Giles

##########################################################################*/

#include "driver.h"
#include "info.h"

#include "mac.h"
#include "macfrontend.h"
#include "macinfo.h"
#include "macinput.h"
#include "maclists.h"
#include "macoptions.h"
#include "macreports.h"
#include "macstrings.h"
#include "macutils.h"
#include "macvideo.h"

#ifdef MESS
#include "macmessfrontend.h"
#include "macmessdrop.h"
#include "macmessresources.h"
#endif

#include <ctype.h>
#include <string.h>

#ifdef __MWERKS__
#pragma require_prototypes on
#endif

/*##########################################################################
	MACROS
##########################################################################*/

#define AE_NULLIFY(a)							\
	do {										\
		(a)->descriptorType = typeNull;			\
		(a)->dataHandle = NULL;					\
	} while (0)
	
#define AE_DISPOSE(a)							\
	do {										\
		if ((a)->descriptorType != typeNull)	\
			(void)AEDisposeDesc(a);				\
		(a)->descriptorType = typeNull;			\
		(a)->dataHandle = NULL;					\
	} while (0)


/*##########################################################################
	CONSTANTS
##########################################################################*/

// main dialog constants 
enum 
{
	kMainOKButton 				= 1,
	kMainQuitButton,
	kMainListUser,
	kMainGroupPopup,
	kMainOptionsButton,
	kMainTabControl,
	kMainInfoTabGroup,
	kMainVideoTabGroup,
	kMainAudioTabGroup,
	kMainReportsTabGroup,
	kMainMiscTabGroup
#ifdef MESS
	,
	kMainMESSTabGroup
#endif
};

// video tab constants
enum
{
	kVideoScalePopup			= 0,
	kVideoHideDesktopCheck,
	kVideoRotateGroupCheck,
	kVideoRotateLeftCheck,
	kVideoRotateRightCheck,
	kVideoRotateFlipXCheck,
	kVideoRotateFlipYCheck,
	kVideoRotateNoneCheck,
	kVideoVectorGroup,
	kVideoVectorFlickerText,
	kVideoVectorFlickerSlider,
	kVideoVectorBeamWidthText,
	kVideoVectorBeamWidthSlider,
	kVideoVectorAntialiasCheck,
	kVideoVectorTranslucentCheck,
	kVideoArtGroupCheck,
	kVideoArtBackdropCheck,
	kVideoArtOverlayCheck,
	kVideoArtBezelCheck
};

// audio tab constants
enum
{
	kAudioEnableCheck			= 0,
	kAudioSampleRateMenu,
	kAudioVolumeSlider,
	kAudioVolumeLowIcon,
	kAudioVolumeHighIcon
};

// reports tab constants
enum
{
	kReportsReportsButton		= 0,
	kReportsAuditROMsButton,
	kReportsAuditSamplesButton,
	kReportsAnalyzeROMsButton,
	kReportsIdentifyROMsButton,
	kReportMAMEGameInfo
};

// misc tab constants
enum
{
	kMiscAutoFrameskipCheck		= 0,
	kMiscThrottleCheck,
	kMiscErrorlogCheck,
	kMiscEnableCheatsCheck,
	kMiscLoadReplayButton,
	kMiscLoadReplayText,
	kMiscSaveReplayButton,
	kMiscSaveReplayText,
	kMiscLoadLangButton,
	kMiscLoadLangText,
	kMiscLoadCheatButton,
	kMiscLoadCheatText,
	kMiscClearLoadReplay,
	kMiscClearSaveReplay,
	kMiscClearLoadLang,
	kMiscClearLoadCheat,
	kMiscBIOSPopup,
	kMiscNoGameInfoCheck,
	kMiscNoDisclaimerCheck
//	kMiscDisableCyclingCheck
};

// group popup menu
enum
{
	kGroupMenuByFolder				= 1,
	kGroupMenuByManufacturer,
	kGroupMenuByDate,
	kGroupMenuDivider,
	kGroupMenuShowClones,
	kGroupMenuShowGhosts,
	kGroupMenuShowNonWorking,
	kGroupMenuItemCountPlusOne
};

// depth popup menu
enum
{
	kDepthMenuAuto		= 1,
	kDepthMenu16Bit,
	kDepthMenu32Bit
};


/*##########################################################################
	TYPE DEFINITIONS
##########################################################################*/

typedef struct CategoryFileSpec
{
	struct CategoryFileSpec		*next;
	FSSpec						spec;
} CategoryFileSpec;


/*##########################################################################
	GLOBAL VARIABLES
##########################################################################*/

// global references to the dialog and the main list & control
static DialogRef				sMainDialog;
static ListRef					sMainList;
static ControlRef				sMainListControl;

// tab information
static DialogItemIndex			sTabItemBase[kTabCountPlusOne];
static UInt32					sActiveTabControls;

// flag for indicating that the list needs to be rebuilt
static Boolean					sListFilteringInvalid;

// rembmering the last selected ROMSet
static ROMSetData *				sLastSelectedROMSet;

// kludge for communicating a double click
static DialogItemIndex			sFakeClickOnThis;

// list color info
static RGBColor					sFinderLabelColor[8];
static const RGBColor			sListBrokenGameColor = {0xffff, 0x0000, 0x0000};

// list font info
static SInt16					sListFontID;
static FontInfo					sListFontInfo;

// list drawing data
static Handle					sListIcons[kTotalROMSetIcons];
static SInt16					sListBottomRedraw;

// grouping data
static MemoryPool				sCategoryPool;
static CategoryFileSpec *		sFirstCategorySpec;

static const bios_entry *sLastBios;

/*##########################################################################
	FUNCTION PROTOTYPES
##########################################################################*/

static Boolean 				PrepareMainDialog(void);
static void 				RestoreDialogSizeAndPosition(void);
static void 				TearDownMainDialog(void);
static void 				ResizeDialog(SInt32 inWidth, SInt32 inHeight);
static void 				RebuildDirtyLists(void);
static UInt32 				GetMenuItemForScaleInterlace(void);
static DialogItemIndex 		HandleMainDialogItem(DialogItemIndex inItemHit, ListNode *inSelected);
static pascal Boolean 		MainDialogFilterProc(DialogRef theDialog, EventRecord *theEvent, DialogItemIndex *itemHit);
static Boolean 				MainDialogMouseDownHandler(EventRecord *inEvent, DialogItemIndex *outItemHit);
static Boolean 				MainDialogKeyDownHandler(EventRecord *inEvent, DialogItemIndex *outItemHit);
static pascal OSStatus		MainDialogMouseWheelMovedProc(EventHandlerCallRef inHandlerCallRef, EventRef inEvent, void *inUserData);

static Boolean 				CreateMainList(void);
static void 				DisposeMainList(void);
static void 				RebuildList(void);
static Boolean 				IncludeNodeInList(const ListNode *inList);
static void					SaveListStates(void);
static void 				UpdateListSize(SInt32 inDeltaWidth);
static pascal void			MainListDrawProc(ControlHandle control, SInt16 part);
static pascal ControlPartCode	MainListKeyDownProc(ControlHandle inControl, SInt16 inKeyCode, SInt16 inCharCode, SInt16 inModifiers);
static pascal ControlPartCode	MainListHitTestProc(ControlHandle inControl, Point inWhere);
static pascal ControlPartCode	MainListTrackingProc(ControlHandle inControl, Point inStartPoint, ControlActionUPP inActionProc);
static pascal void			MainListActivateProc(ControlHandle inControl, Boolean inActivating);
static pascal ControlPartCode	MainListFocusProc(ControlHandle inControl, ControlFocusPart inAction);
static pascal OSStatus		MainListMouseWheelMovedHandler(Point QDLocation, EventMouseWheelAxis axis, SInt32 delta);

static Boolean 				PrepareTabGroups(void);
static void 				SetActiveTab(UInt32 inActiveTab);
static void					InitializeTabControls(UInt32 inTabIndex, DialogItemIndex inBaseIndex);
static void					UpdateTabControls(UInt32 inTabIndex, DialogItemIndex inBaseIndex);
static void 				HandleTabControls(UInt32 inTabIndex, DialogItemIndex inBaseIndex, DialogItemIndex inItemHit);
static void					TearDownTabControls(UInt32 inTabIndex, DialogItemIndex inBaseIndex);

static void 				InitializeGroupMenu(void);
static void 				HandleSortingChange(void);
static void 				UpdateGroupMenu(void);
static Boolean 				FSSpecsMatch(const FSSpec *inSpec1, const FSSpec *inSpec2);

static Boolean 				HandleListContextualClick(const EventRecord *inEvent, DialogItemIndex *outItemHit);
static OSErr 				DoRomsetContextualMenu(const EventRecord *inEvent, const ROMSetData *inRomset, ListRef inList);
static OSErr 				LocateProcess(ProcessSerialNumber *outPSN, OSType inProcType, OSType inCreator);
static OSErr 				SendRevealAE(const FSSpec *inSpec);
static OSErr 				MoveToTrash(const FSSpec *inSpec);

static ListNode *			GetIndexedNodeFromList(UInt32 inIndex);
static ListNode *			GetSelectedNode(void);
static const ROMSetData *	GetSelectedROMSet(void);
static void 				SelectROMSet(const ROMSetData *inROMSet);
static void 				SelectNode(const ListNode *inROMSet);
static const ROMSetData * 	FindDriverInList(const game_driver *inDriver);
static SInt32				MatchCellText(const char *inStringToFind);

static pascal void 			CustomListProc(SInt16 inMessage, Boolean inSelected, Rect *inCellBounds, Cell inCell, SInt16 inDataOffset, SInt16 inDataSize, ListHandle inList);
static void 				DrawTextForNode(ListNode *inNode, const Rect *inItemRect, Boolean inEnabled);
static void 				DrawIconForNode(ListNode *inNode, const Rect *inItemRect, Boolean inSelected, Boolean inEnabled);


/*##########################################################################
	IMPLEMENTATION
##########################################################################*/

#pragma mark ¥ Main Dialog UI

//===============================================================================
//	ChooseGame
//
//	Displays the main user interface dialog.
//===============================================================================

Boolean ChooseGame(ROMSetData *outSelection)
{
	EventHandlerUPP MainDialogMouseWheelMovedUPP;
	ModalFilterUPP	filterUPP;
	GrafPtr			savePort;
	Boolean			result;
	
	// save the previous port
	GetPort(&savePort);
	
	// allocate a filter UPP
	filterUPP = NewModalFilterUPP(MainDialogFilterProc);
	if (!filterUPP)
		return false;

	// prepare the dialog
	if (!PrepareMainDialog())
		return false;

	// install wheel handler
	{
		const EventTypeSpec eventTypeSpec = {kEventClassMouse, kEventMouseWheelMoved};
		MainDialogMouseWheelMovedUPP = NewEventHandlerUPP(MainDialogMouseWheelMovedProc);
		/*theErr =*/ InstallEventHandler(GetWindowEventTarget(GetDialogWindow(sMainDialog)),
										MainDialogMouseWheelMovedUPP,
										1, &eventTypeSpec, NULL, NULL);
	}

	// loop indefinitely
	while (1)
	{
		ListNode *			selected;
		UInt32				tabIndex;
		DialogItemIndex		itemHit;
		
		// if something in the list is invalid, rebuild the necessary parts
		RebuildDirtyLists();
		
		// update any controls for the current selection
		for (tabIndex = 1; tabIndex < kTabCountPlusOne; tabIndex++)
			UpdateTabControls(tabIndex, sTabItemBase[tabIndex]);
		
		// handle the modal dialog
		sFakeClickOnThis = 0;
		ModalDialog(filterUPP, &itemHit);
		
		// if we got some kind of fakery, use it instead of the item
		if (sFakeClickOnThis != 0)
			itemHit = sFakeClickOnThis;

		// handle the item
		selected = GetSelectedNode();
		itemHit = HandleMainDialogItem(itemHit, selected);
		
		// handle OK and Cancel buttons
		if (itemHit == kMainOKButton)
		{
			sLastSelectedROMSet = selected->romset;
			*outSelection = *selected->romset;
			result = true;
			break;
		}
		else if (itemHit == kMainQuitButton)
		{
			result = false;
			break;
		}
	}

	// tear it down and get out
	TearDownMainDialog();
	DisposeEventHandlerUPP(MainDialogMouseWheelMovedUPP);
	DisposeModalFilterUPP(filterUPP);
	SetPort(savePort);
	
	return result;
}
	
	
//===============================================================================
//	PrepareMainDialog
//
//	Initializes the main dialog.
//===============================================================================

static Boolean PrepareMainDialog(void)
{
	Str255			labelString;
	Cursor 			arrow;
	UInt32			index;

	// intialize globals
	sMainDialog 			= NULL;
	sMainList 				= NULL;
	sMainListControl 		= NULL;
	sActiveTabControls	 	= 0;

	// determine all Finder label colors 
	for (index = 0; index < 8; index++)
		GetLabel(index, &sFinderLabelColor[index], labelString);
	
	// setup the dialog 
	sMainDialog = GetNewDialog(rNewChooseDialog, NULL, kWindowToFront);
	if (!sMainDialog)
		return false;
	SetPortDialogPort(sMainDialog);

	// mark the OK button as default
	SetDialogDefaultItem(sMainDialog, kMainOKButton);
	SetDialogCancelItem(sMainDialog, kMainQuitButton);

#ifdef MESS
	{
		ControlRef controlRef;
		Handle handle;
		Str255 newCaption;

		handle = Get1Resource('STR ', rOpenGameMESSQuitCaption);
		if (handle)
		{
			memcpy(newCaption, *handle, (*handle)[0]+1);

			if (GetDialogItemAsControl(sMainDialog, kMainQuitButton, &controlRef) == noErr)
				SetControlTitle(controlRef, newCaption);
		}
	}

#endif


	// create the tabs
	if (!PrepareTabGroups())
	{
		DisposeDialog(sMainDialog);
		sMainDialog = NULL;
		return false;
	}
	
	// create the list
	if (!CreateMainList())
	{
		DisposeDialog(sMainDialog);
		sMainDialog = NULL;
		return false;
	}

	// initialize update the group menu
	InitializeGroupMenu();
	UpdateGroupMenu();

	// prepare the list
	RebuildDirtyLists();

	// set the initial tab
#ifdef MESS
	SetActiveTab(kTabMESS);
#else
	SetActiveTab(kTabInfo);
#endif
	
	// restore the dialog size and position
	RestoreDialogSizeAndPosition();
	
	// if the list is empty, disable the OK button, otherwise, select the last ROMSet
	if (ListEmpty(sMainList))
		SetDialogControlActive(sMainDialog, kMainOKButton, false);		
	else
		SelectROMSet(sLastSelectedROMSet);
	
	// display the window and set the focus to the main list
	ShowWindow(GetDialogWindow(sMainDialog));
	SetKeyboardFocus(GetDialogWindow(sMainDialog), sMainListControl, kControlFocusNextPart);
	
	// back to an arrow
	SetCursor(GetQDGlobalsArrow(&arrow));

#ifdef MESS
	// Set up drag and drop
	MESSInstallDragDrop( sMainDialog, sTabItemBase[ kTabMESS ] );
#endif

	return true;
}


//===============================================================================
//	RestoreDialogSizeAndPosition
//
//	Attempts to use the size and position saved in the preferences.
//===============================================================================

static void RestoreDialogSizeAndPosition(void)
{
	Point 		bottomRight;
	Rect 		testRect;

	// make sure we have the minimum size
	if (gFEPrefs.dialogSize.h < kMainDialogMinimumWidth)
		gFEPrefs.dialogSize.h = kMainDialogMinimumWidth;
	if (gFEPrefs.dialogSize.v < kMainDialogMinimumHeight)
		gFEPrefs.dialogSize.v = kMainDialogMinimumHeight;

	// get an approximation of the desktop region
	GetRegionBounds(GetGrayRgn(), &testRect);
	testRect.top += GetMBarHeight();

	// if the location is still valid, check the size
	if (PtInRect(gFEPrefs.dialogLocation, &testRect))
	{
		// set the bottom right	
		bottomRight.h = gFEPrefs.dialogLocation.h + gFEPrefs.dialogSize.h;
		bottomRight.v = gFEPrefs.dialogLocation.v + gFEPrefs.dialogSize.v;

		// if that's visible as well, we'll keep the original location
		if (PtInRect(bottomRight, &testRect))
			MoveWindow(GetDialogWindow(sMainDialog), gFEPrefs.dialogLocation.h, gFEPrefs.dialogLocation.v, true);
	}

	// then do the standard resize
	ResizeDialog(gFEPrefs.dialogSize.h, gFEPrefs.dialogSize.v);
}


//===============================================================================
//	TearDownMainDialog
//
//	Frees up all memory and resources allocated for the dialog.
//===============================================================================

static void TearDownMainDialog(void)
{
	WindowRef 		window = GetDialogWindow(sMainDialog);
	Rect 			windowBounds;
	UInt32			tabIndex;
	ListNode 		*listNode;
	
	/* Save name of last driver used */
	listNode = GetSelectedNode();
	if (listNode)
		//BlockMoveData(listNode->description, gFEPrefs.driverSelected, 256);
		PLstrcpy(gFEPrefs.driverSelected, listNode->description);
	
	// Save the open/closed state of each folder
	SaveListStates();
	
	// tear down the tabs
	for (tabIndex = 1; tabIndex < kTabCountPlusOne; tabIndex++)
		TearDownTabControls(tabIndex, sTabItemBase[tabIndex]);

	// free up the list items
	DisposeMainList();

	// save dialog position and size to the prefs 
	GetWindowPortBounds(window, &windowBounds);
	gFEPrefs.dialogLocation.h = windowBounds.left;
	gFEPrefs.dialogLocation.v = windowBounds.top;
	LocalToGlobal(&gFEPrefs.dialogLocation);
	gFEPrefs.dialogSize.h = windowBounds.right - windowBounds.left;
	gFEPrefs.dialogSize.v = windowBounds.bottom - windowBounds.top;

#ifdef MESS
	// Remove MESS' Drag and Drop handlers
	MESSRemoveDragDrop( sMainDialog );
#endif

	// free the dialog, which should take care of everything else
	DisposeDialog(sMainDialog);
}


//===============================================================================
//	ResizeDialog
//
//	Resizes the main dialog after the user changes it.
//===============================================================================

static void ResizeDialog(SInt32 inWidth, SInt32 inHeight)
{
	WindowRef 		window = GetDialogWindow(sMainDialog);
	Rect			itemRect, windowRect, emptyRect = { 0, 0, 0, 0 };
	RgnHandle 		saveClip;
	Handle			itemHandle;
	DialogItemType	itemType;
	UInt32			tabIndex;
	SInt32			xDelta, yDelta;
	ThemeDrawingState themeState;

	// hide all drawing until we're done
	saveClip = NewRgn();
	GetClip(saveClip);
	ClipRect(&emptyRect);

	// determine the X and Y deltas
	GetWindowPortBounds(window, &windowRect);
	xDelta = inWidth - (windowRect.right - windowRect.left);
	yDelta = inHeight - (windowRect.bottom - windowRect.top);
	
	// resize the dialog window and get the new bounds
	SizeWindow(window, inWidth, inHeight, true);
	GetWindowPortBounds(window, &windowRect);
	
	// move the tab items; the docs say calling MoveDialogItem adjusts the control size as well
	GetDialogItem(sMainDialog, kMainTabControl, &itemType, &itemHandle, &itemRect);
	MoveDialogItem(sMainDialog, kMainTabControl, itemRect.left + xDelta, itemRect.top);
	for (tabIndex = 1; tabIndex < kTabCountPlusOne; tabIndex++)
	{
		GetDialogItem(sMainDialog, kMainInfoTabGroup + tabIndex - 1, &itemType, &itemHandle, &itemRect);
		MoveDialogItem(sMainDialog, kMainInfoTabGroup + tabIndex - 1, itemRect.left + xDelta, itemRect.top);
	}

	// resize list control
	UpdateListSize(xDelta);
	
	// move ok, cancel and options buttons
	GetDialogItem(sMainDialog, kMainOKButton, &itemType, &itemHandle, &itemRect);
	MoveDialogItem(sMainDialog, kMainOKButton, itemRect.left + xDelta, itemRect.top + yDelta);

	GetDialogItem(sMainDialog, kMainQuitButton, &itemType, &itemHandle, &itemRect);
	MoveDialogItem(sMainDialog, kMainQuitButton, itemRect.left + xDelta, itemRect.top + yDelta);

	GetDialogItem(sMainDialog, kMainOptionsButton, &itemType, &itemHandle, &itemRect);
	MoveDialogItem(sMainDialog, kMainOptionsButton, itemRect.left + xDelta, itemRect.top + yDelta);

	// restore original clipping region
	SetClip(saveClip);
	DisposeRgn(saveClip);
	
	// force a redraw
	GetThemeDrawingState (&themeState);
	SetThemeBackground (kThemeBrushMovableModalBackground, 32, true);
	EraseRect(&windowRect);
	SetThemeDrawingState (themeState, true);
	InvalWindowRect(window, &windowRect);
}


//===============================================================================
//	RebuildDirtyLists
//
//	Rebuilds any lists are are marked invalid.
//===============================================================================

static void RebuildDirtyLists(void)
{
	const ROMSetData *	selected = GetSelectedROMSet();
	Boolean				updated;
	
	// Save the open/closed state of each folder for the current list
	SaveListStates();
	
	// update any invalid lists
	updated = UpdateInvalidLists();
	
	// if the visibility of items is invalid, rebuild the list
	if (updated || sListFilteringInvalid)
	{
		RebuildList();
		updated = true;
	}
	
	// if we updated, re-find the ROMSet
	if (updated)
		SelectROMSet(selected);
}
		

//===============================================================================
//	GetMenuItemForScaleInterlace
//
//	Determines the menu item index for the current scale and interlace settings.
//===============================================================================

static UInt32 GetMenuItemForScaleInterlace(void)
{
	if (gPrefs.windowScale == 1)
		return 1;
	else if (gPrefs.windowScale == 2)
		return gPrefs.interlace ? 3 : 2;
	else
		return gPrefs.interlace ? 5 : 4;
}


//===============================================================================
//	HandleMainDialogItem
//
//	Handles a hit in on a dialog item in the main dialog.
//===============================================================================

static DialogItemIndex HandleMainDialogItem(DialogItemIndex inItemHit, ListNode *inSelected)
{
	Boolean			diffFlags1, diffFlags2;
	UInt32			tabIndex;
	
	// switch off the item
	switch (inItemHit)
	{
		// handle the Play/Open/Close button
		case kMainOKButton:
		
			// for most cases, we will return something other than kMainOKButton
			inItemHit = 0;
		
			// should never happen, but why not be safe
			if (inSelected == NULL)
				SysBeep(1);

			// if it was a folder, toggle it
			else if (inSelected->type == kNodeOpenFolder || inSelected->type == kNodeClosedFolder)
			{
				ListNode *selected = GetSelectedNode();
				inSelected->type = 1 - inSelected->type;
				sListFilteringInvalid = true;
				RebuildDirtyLists();
				SelectNode(selected);
			}
			
			// if it was anything else other than a ROMSet, it's an error
			else if (inSelected->type != kNodeROMSet || inSelected->romset == NULL)
				SysBeep(1);

			// otherwise, we're done
			else
				return kMainOKButton;
			break;

		// a hit on the tab control means we have to switch tabs
		case kMainTabControl:
			SetActiveTab(GetDialogControlValue(sMainDialog, kMainTabControl));
			break;

		// a hit on the options button means we display the options dialog
		case kMainOptionsButton:

			// remember the original flags
			diffFlags1 = gFEPrefs.fe_displayFileNames;
			diffFlags2 = gFEPrefs.fe_listSortThe;

			// display the options dialog
			OptionsDialog();

			// If the descriptive name format changed, recompute all the descriptions and resort
			if ((diffFlags1 != gFEPrefs.fe_displayFileNames) || (diffFlags2 != gFEPrefs.fe_listSortThe))
			{
//				InvalidateListDescriptions();
				// LBO 4/21/05. This works around a rather complicated bug whereby changing the
				// descriptive name status doesn't re-sort the list properly. SortByFolder in
				// maclists.c doesn't deal well with > 1 level of nesting when it comes time to
				// resort. So instead we invalidate the entire list and let it be re-created.
				InvalidateListGrouping();
			}
			
			// update the list size, since the font may have changed
			UpdateListSize(0);
			break;
		
		// a hit here will change our group sorting
		case kMainGroupPopup:
			HandleSortingChange();
			break;

		// if it's anything else, look for a hit in a tabbed item
		default:
			for (tabIndex = kTabCountPlusOne - 1; tabIndex >= 1; tabIndex--)
				if (inItemHit >= sTabItemBase[tabIndex])
				{
					HandleTabControls(tabIndex, sTabItemBase[tabIndex], inItemHit - sTabItemBase[tabIndex]);
					break;
				}
			break;
	}
	
	return inItemHit;
}


//===============================================================================
//	MainDialogFilterProc
//
//	The main filter proc for the dialog.
//===============================================================================

static pascal Boolean MainDialogFilterProc(DialogRef inDialog, EventRecord *inEvent, DialogItemIndex *outItemHit)
{
	Boolean		result = false;
#ifndef MESS
	UInt32		tabIndex;
#endif

	// update any controls for the current selection; we need to do this here because
	// bringing up the options dialog causes some enabled/disabled paradoxes
#ifndef MESS
	// R Nabet 000512 : This code currently causes some annoying flicker in the MESS tab.
	// I don't know which problem it is supposed to fix, I could not reproduce it.
	for (tabIndex = 1; tabIndex < kTabCountPlusOne; tabIndex++)
		UpdateTabControls(tabIndex, sTabItemBase[tabIndex]);
#endif
		
	// switch off the event type
	switch (inEvent->what)
	{
		// handle mouse down events
		case mouseDown:
			result = MainDialogMouseDownHandler(inEvent, outItemHit);
			break;
		
		// handle key down events
		case keyDown:
		case autoKey:
			result = MainDialogKeyDownHandler(inEvent, outItemHit);
			break;

		// high level events; let the AppleEvent Manager do the work
		case kHighLevelEvent:
			AEProcessAppleEvent(&gLastEvent);
			result = true;
			
			// Did a quit event tell us to exit?
			if (gExitToShell)
			{
				*outItemHit = GetDialogCancelItem(inDialog);
				FakeButtonClick(inDialog, *outItemHit);
			}
			break;
	}
	
	// if we get here, we didn't handle it
	return result ? true : StdFilterProc(inDialog, inEvent, outItemHit);
}

			
//===============================================================================
//	MainDialogMouseDownHandler
//
//	Handle mouse down events in the main dialog.
//===============================================================================

static Boolean MainDialogMouseDownHandler(EventRecord *inEvent, DialogItemIndex *outItemHit)
{
	Rect			limitRect;
	SInt32			growResult;
	WindowRef		window;

	// switch off the part
	switch (FindWindow(inEvent->where, &window))
	{
		// handle menubar clicks
//		case inMenuBar:
//			HandleMenuSelection(MenuSelect(inEvent->where));
//			return true;
		
		// handle growing the dialog window
		case inGrow:
		
			// skip if this isn't the corrent window
			if (window != GetDialogWindow(sMainDialog))
				break;

			// set appropriate grow limits
			GetRegionBounds(GetGrayRgn(), &limitRect);
			limitRect.right = limitRect.right - limitRect.left - 20;		// max x
			limitRect.bottom = limitRect.bottom - limitRect.top - 40;		// max y
			limitRect.left = kMainDialogMinimumWidth;						// min x
			limitRect.top = kMainDialogMinimumHeight;						// min y

			// standard grow window
			growResult = GrowWindow(window, inEvent->where, &limitRect);
			if (growResult != 0)
				ResizeDialog(LoWord(growResult), HiWord(growResult));

			// all done & handled			
			*outItemHit = 0;
			return true;
		
		// handle contextual menus
		case inContent:

			// skip if this isn't the corrent window
			if (window != GetDialogWindow(sMainDialog))
				break;
#ifdef MESS
			if( sActiveTabControls == kTabMESS )
				if (MESSHandleInContent( sMainDialog, sTabItemBase[sActiveTabControls], inEvent, outItemHit ) )
					return true;
#endif
			// the only thing we handle at this level is contextual menus
			return HandleListContextualClick(inEvent, outItemHit);
	}
	
	// if we get here, we didn't handle it
	return false;
}


//===============================================================================
//	MainDialogKeyDownHandler
//
//	Handle key down events in the main dialog.
//===============================================================================

static Boolean MainDialogKeyDownHandler(EventRecord *inEvent, DialogItemIndex *outItemHit)
{
	UInt8		charCode = inEvent->message & charCodeMask;

	// let the info tab have a crack at it
	if (sActiveTabControls == kTabInfo)
		if (InfoKeyDownHandler(sMainDialog, sTabItemBase[kTabInfo], inEvent, outItemHit))
			return true;

	if ((inEvent->modifiers & (cmdKey | optionKey)) == (cmdKey | optionKey))
	{
		switch (charCode)
		{
			case kLeftArrowCharCode:
			{
				// Cmd-Opt-Left arrow = close all groups
				int i;
				ListNode *selected = GetSelectedNode();
				for (i = 0; i < GetListDataBoundsBottom (sMainList); i ++)
				{
					ListNode *node = GetIndexedNodeFromList (i);
					if (node && node->type == kNodeOpenFolder)
						node->type = kNodeClosedFolder;
				}
				sListFilteringInvalid = true;
				RebuildDirtyLists();
				SelectNode(selected);
				return true;
			}
			case kRightArrowCharCode:
			{
				// Cmd-Opt-Right arrow = open all groups
				int i;
				ListNode *selected = GetSelectedNode();
				for (i = 0; i < GetListDataBoundsBottom (sMainList); i ++)
				{
					ListNode *node = GetIndexedNodeFromList (i);
					if (node && node->type == kNodeClosedFolder)
						node->type = kNodeOpenFolder;
				}
				sListFilteringInvalid = true;
				RebuildDirtyLists();
				SelectNode(selected);
				return true;
			}
		}
	}

	else if (inEvent->modifiers & cmdKey)
	{
		switch (charCode)
		{
			case 'Q':
			case 'q':
				// accept cmd-Q for quit 
				*outItemHit = cancel;
				FakeButtonClick(sMainDialog, *outItemHit);
				#ifdef MESS
					// force quit
					gExitToShell = true;
				#endif
				return true;
			case kLeftArrowCharCode:
			{
				// Cycle through the tabs in decreasing order
				int newTab = sActiveTabControls - 1;
				
				if (newTab == 0)
					newTab = kTabCountPlusOne - 1;
				SetActiveTab (newTab); 
				return true;
			}
			case kRightArrowCharCode:
			{
				// Cycle through the tabs in increasing order
				int newTab = sActiveTabControls + 1;
				
				if (newTab == kTabCountPlusOne)
					newTab = 1;
				SetActiveTab (newTab); 
				return true;
			}
		}
	}

	// Handle misc control characters in the sMainList 
	switch (charCode)
	{
		// return/enter handle like the default item
		case kReturnCharCode:
		case kEnterCharCode:
			*outItemHit = GetDialogDefaultItem(sMainDialog);
			FakeButtonClick(sMainDialog, *outItemHit);
			return true;

		// escape/clear handle like the cancel item			
		case kEscapeCharCode:
			*outItemHit = GetDialogCancelItem(sMainDialog);
			FakeButtonClick(sMainDialog, *outItemHit);
			return true;
	}
	
	// if we get here, we didn't handle it
	return false;
}


//===============================================================================
//	MainDialogMouseWheelMovedProc
//
//	Handle mouse wheel events in the main dialog.
//
//	I am still wondering why MacOS X.1 does such a poor job of dispatching
//	wheel events to the correct control: it always dispatches these events to
//	the control with focus, whereas the idea with the wheel is rather "point
//	and scroll".  Still, it is better than X.2 which does not dispatch wheel
//	events to controls at all.  Anyway, we work around this crap by installing
//	a window event handler and checking by hand if the click was in the control
//	bounds.
//===============================================================================

static pascal OSStatus MainDialogMouseWheelMovedProc(EventHandlerCallRef inHandlerCallRef,
														EventRef inEvent, void *inUserData)
{
	OSStatus result;
	HIPoint location;
	EventMouseWheelAxis axis;
	SInt32 delta;
	Point QDLocation;
	Rect bounds;
	ControlRef theControl;


	// Get location
	result = GetEventParameter(inEvent, kEventParamMouseLocation, typeHIPoint, NULL,
									sizeof(location), NULL, & location);
	if (result == noErr)
	{
		// Get axis
		result = GetEventParameter(inEvent, kEventParamMouseWheelAxis, typeMouseWheelAxis, NULL,
									sizeof(axis), NULL, & axis);
	}
	if (result == noErr)
	{
		// Get delta
		result = GetEventParameter(inEvent, kEventParamMouseWheelDelta, typeLongInteger, NULL,
									sizeof(delta), NULL, & delta);
	}
	if (result == noErr)
	{
		result = eventNotHandledErr;

		QDLocation.h = location.x;
		QDLocation.v = location.y;
		QDGlobalToLocalPoint(GetDialogPort(sMainDialog), &QDLocation);

		// test main list
		GetControlBounds(sMainListControl, &bounds);
		if (PtInRect(QDLocation, &bounds))
		{
			result = MainListMouseWheelMovedHandler(QDLocation, axis, delta);
		}

		// test active tab
		if (result == eventNotHandledErr)
		{
			if (GetDialogItemAsControl(sMainDialog, kMainInfoTabGroup + sActiveTabControls - 1, &theControl) == noErr)
			{
				GetControlBounds(theControl, &bounds);
				if (PtInRect(QDLocation, &bounds))
				{
					switch (sActiveTabControls)
					{
					case kTabInfo:
						result = InfoHandleMouseWheelMoved(sMainDialog, sTabItemBase[sActiveTabControls], QDLocation, axis, delta);
						break;

#ifdef MESS
					case kTabMESS:
						result = MESSHandleMouseWheelMoved(QDLocation, axis, delta);
						break;
#endif
					}
				}
			}
		}
	}

	return result;
}

#pragma mark -
#pragma mark ¥ Main List UI

//===============================================================================
//	CreateMainList
//
//	Create the main list and attach it to the user item.
//===============================================================================

static Boolean CreateMainList(void)
{
	static ListDefSpec					sListDefSpec = { kListDefUserProcType };
	static ControlUserPaneDrawUPP 		sDrawUPP = NULL;
	static ControlUserPaneKeyDownUPP 	sKeyDownUPP = NULL;
	static ControlUserPaneHitTestUPP	sHitTestUPP = NULL;
	static ControlUserPaneTrackingUPP	sTrackingUPP = NULL;
	static ControlUserPaneActivateUPP	sActivateUPP = NULL;
	static ControlUserPaneFocusUPP		sFocusUPP = NULL;
	
	ControlHandle	scrollbar;
	ListBounds		listBounds = { 0, 0, 1, 1 };
	Point			cellSize = { 16, 16 };
	Rect			controlRect;
	OSErr			err;
	int				i;

	// make sure we have UPPs for all the callbacks
	if (!sDrawUPP || !sKeyDownUPP || !sHitTestUPP || !sTrackingUPP || !sActivateUPP || !sFocusUPP || !sListDefSpec.u.userProc)
	{
		sDrawUPP = NewControlUserPaneDrawUPP(MainListDrawProc);
		sKeyDownUPP = NewControlUserPaneKeyDownUPP(MainListKeyDownProc);
		sHitTestUPP = NewControlUserPaneHitTestUPP(MainListHitTestProc);
		sTrackingUPP = NewControlUserPaneTrackingUPP(MainListTrackingProc);
		sActivateUPP = NewControlUserPaneActivateUPP(MainListActivateProc);
		sFocusUPP = NewControlUserPaneFocusUPP(MainListFocusProc);
		sListDefSpec.u.userProc = NewListDefUPP(CustomListProc);
	}
	if (!sDrawUPP || !sKeyDownUPP || !sHitTestUPP || !sTrackingUPP || !sActivateUPP || !sFocusUPP || !sListDefSpec.u.userProc)
		return false;
	
	// read the icon suites
	for (i = 0; i < kTotalROMSetIcons; i++)
		GetIconSuite(&sListIcons[i], 1000 + i, kSelectorSmall1Bit + kSelectorSmall8Bit);
	
	// get the dialog item data
	if (GetDialogItemAsControl(sMainDialog, kMainListUser, &sMainListControl) != noErr)
		return false;
	GetControlBounds(sMainListControl, &controlRect);
	cellSize.h = controlRect.right - controlRect.left;

	// create the list; we use the carbon API and fake it down below
	err = CreateCustomList(	&controlRect,
							&listBounds,
							cellSize,
							&sListDefSpec,
							GetDialogWindow(sMainDialog),
							false,						// Boolean drawIt
							false,						// Boolean hasGrow
							false,						// Boolean scrollHoriz
							true,						// Boolean scrollVert
							&sMainList);
	if (err != noErr || !sMainList)
		return false;

	// set the flags
	SetListSelectionFlags(sMainList, lOnlyOne);

	// set the control data
	SetControlData(sMainListControl, kControlEntireControl, kControlUserPaneDrawProcTag, sizeof(sDrawUPP), &sDrawUPP);
	SetControlData(sMainListControl, kControlEntireControl, kControlUserPaneKeyDownProcTag, sizeof(sKeyDownUPP), &sKeyDownUPP);
	SetControlData(sMainListControl, kControlEntireControl, kControlUserPaneHitTestProcTag, sizeof(sHitTestUPP), &sHitTestUPP);
	SetControlData(sMainListControl, kControlEntireControl, kControlUserPaneTrackingProcTag, sizeof(sTrackingUPP), &sTrackingUPP);
	SetControlData(sMainListControl, kControlEntireControl, kControlUserPaneActivateProcTag, sizeof(sActivateUPP), &sActivateUPP);
	SetControlData(sMainListControl, kControlEntireControl, kControlUserPaneFocusProcTag, sizeof(sFocusUPP), &sFocusUPP);

	// embed the scrollbar and route mouse clicks for the scrollbar thru the user pane
	scrollbar = GetListVerticalScrollBar(sMainList);
	if (scrollbar != NULL)
	{
		EmbedControl(scrollbar, sMainListControl);
		SetControlSupervisor(scrollbar, sMainListControl);
	}

	// at this point, we need to fill in the list; mark the final stage invalid
	sListFilteringInvalid = true;

	return true;
}


//===============================================================================
//	DisposeMainList
//
//	Frees up memory taken by the main list.
//===============================================================================

static void DisposeMainList(void)
{
	UInt32			iconIndex;
	
	// kill the list itself
	// Apparently, the list box control takes care of disposing of the list itself
	if (sMainList != NULL)
		LDispose(sMainList);
	sMainList = NULL;
	
	// kill the icon suites
	for (iconIndex = 0; iconIndex < kTotalROMSetIcons; iconIndex++)
		if (sListIcons[iconIndex] != NULL)
		{
			DisposeIconSuite(sListIcons[iconIndex], true);
			sListIcons[iconIndex] = NULL;
		}
}


//===============================================================================
//	UpdateListSize
//
//	Recomputes the size of the list, based on all the relevant parameters.
//===============================================================================

static void UpdateListSize(SInt32 inDeltaWidth)
{
	SInt16 		oldFont = GetPortTextFont(GetDialogPort(sMainDialog));
	Style 		oldFace = GetPortTextFace(GetDialogPort(sMainDialog));
	SInt16 		oldSize = GetPortTextSize(GetDialogPort(sMainDialog));
	UInt32		newWidth, newHeight;
	Rect		listRect;
	Point		cellSize;
	Str255 		tempStr;
	Rect		portRect;

	// first lookup the font ID and save it away
	PLstrcpy(tempStr, gFEPrefs.font);
	GetFNum(tempStr, &sListFontID);
	
	// get the font info for this font
	SetPortDialogPort(sMainDialog);
	TextFont(sListFontID);
	TextSize(gFEPrefs.fontSize);
	TextFace(gFEPrefs.fontFace);
	GetFontInfo(&sListFontInfo);
	TextFont(oldFont);
	TextFace(oldFace);
	TextSize(oldSize);
	
	// get the size of the window and the size of the control
	GetControlBounds(sMainListControl, &listRect);
	GetWindowPortBounds(GetDialogWindow(sMainDialog), &portRect);
	
	// get the original cell size
	GetListCellSize(sMainList, &cellSize);
	
	// compute the new cell size height
	cellSize.v = kIconHeight;
	if (sListFontInfo.ascent + sListFontInfo.descent + sListFontInfo.leading > cellSize.v)
		cellSize.v = sListFontInfo.ascent + sListFontInfo.descent + sListFontInfo.leading;
	cellSize.v += 2 * kCellYSlop;
	
	// compute the new cell width
	cellSize.h += inDeltaWidth;
	LCellSize(cellSize, sMainList);
	
	// calculate new height as a multiple of the cell height
	newHeight = (portRect.bottom - 12) - listRect.top;
	newHeight = (newHeight / cellSize.v) * cellSize.v;
	
	// calculate new width
	newWidth = listRect.right - listRect.left;
	newWidth += inDeltaWidth;
	
	// resize the sMainList dialog item (and control)
	SizeDialogItem(sMainDialog, kMainListUser, newWidth, newHeight);

	// account for the scrollbar and set the list size
	LSize(newWidth - kScrollbarWidth, newHeight, sMainList);
	
	// erase the list area
	if (newWidth > listRect.right - listRect.left)
		listRect.right = listRect.left + newWidth;
	if (newHeight > listRect.bottom - listRect.top)
		listRect.bottom = listRect.top + newHeight;
	InsetRect(&listRect, -4, -4);
	EraseRect(&listRect);
}


//===============================================================================
//	RebuildList
//
//	Scans through the list, filling in pointers to the ROMSets.
//===============================================================================

static void RebuildList(void)
{
	UInt32			nodeCount = GetListNodeCount();
	const ListNode *lastNode = NULL;
	int 			waitingFor = -1, index;
	Cell 			cell = { 0, 0 };
	const ListNode *node;
	
	// make sure we're pointing to the dialog port
	SetPortDialogPort(sMainDialog);

	// turn off drawing while we're doing this
	LSetDrawingMode(false, sMainList);

	// make sure the sMainList is maximum size to start
	if (GetListDataBoundsBottom(sMainList) < nodeCount)
		LAddRow(nodeCount - GetListDataBoundsBottom(sMainList), 32767, sMainList);

	// loop over all sets
	node = GetFirstListNode();
	for (index = 0; index < nodeCount; index++, node = node->next)
	{
		// if we're waiting to get back to a nest level, check it
		if (waitingFor != -1 && node->nest <= waitingFor)
			waitingFor = -1;
		
		// if we're not waiting for anything, display the current item
		if (waitingFor == -1)
			if (IncludeNodeInList(node))
			{
				// if the last node was a folder, and this one is a folder at the same
				// or lesser nesting, then overwrite the previous (empty) folder
				if (lastNode != NULL && lastNode->nest >= node->nest && lastNode->type == kNodeOpenFolder)
					cell.v--;
				lastNode = node;
			
				// set the cell data and increment
				LSetCell(&node, sizeof(ListNode *), cell, sMainList);
				cell.v++;
			}
		
		// if this is a closed subfolder and we're not waiting for anything, start waiting
		if (node->type == kNodeClosedFolder && waitingFor == -1)
			waitingFor = node->nest;
	}
	
	// if the last node was a folder, nuke it
	if (lastNode != NULL && lastNode->type == kNodeOpenFolder)
		cell.v--;

	// delete extra rows at the bottom
	if (cell.v < GetListDataBoundsBottom(sMainList))
		LDelRow(GetListDataBoundsBottom(sMainList) - cell.v, cell.v, sMainList);
	
	// autoscroll
	LAutoScroll(sMainList);

	// allow drawing and invalidate
	LSetDrawingMode(true, sMainList);
	InvalidateList(sMainList);

	// mark the filtering invalid and return the result
	sListFilteringInvalid = false;
}


//===============================================================================
//	IncludeNodeInList
//
//	Returns true if we should include a node in the list.
//===============================================================================

static Boolean IncludeNodeInList(const ListNode *inNode)
{
	// always include non-ROMs
	if (inNode->type != kNodeROMSet)
		return true;
	
	// never show ROMs that don't have drivers
	if (!inNode->romset || !inNode->romset->driver)
		return false;
	
	// show/hide clones
	if ((gFEPrefs.groupFlags & kGroupFlagHideClones) && IsClone(inNode->romset->driver))
		return false;

	// show/hide virtual clones
	if (inNode->romset->type == kROMSetTypeGhost)
		if (!(gFEPrefs.groupFlags & kGroupFlagShowGhosts) && IsClone(inNode->romset->driver))
			return false;

	// show/hide non-working
	if ((gFEPrefs.groupFlags & kGroupFlagHideNonWorking) && IsBroken(inNode->romset->driver))
	{
		const ListNode *tempNode = inNode->next;
		
		// special case: if we're attaching clones, only remove the parent 
		// if we have no working clones
		if (!IsClone(inNode->romset->driver))
		{
			while (tempNode)
			{
				if (tempNode->type != kNodeROMSet || !tempNode->romset || !tempNode->romset->driver)
					break;
				if (!IsClone(tempNode->romset->driver))
					break;
				if (IncludeNodeInList(tempNode))
				{
					if (!IsBroken(tempNode->romset->driver))
						return true;
				}
				tempNode = tempNode->next;
			}
		}
		return false;
	}
		
	return true;
}


//===============================================================================
//	SaveListStates
//
//	Saves the open/closed state of the folder nodes to the prefs
//===============================================================================

static void SaveListStates(void)
{
	UInt32			nodeCount = GetListNodeCount();
	const ListNode *node;
	int index;
	
	node = GetFirstListNode();
	for (index = 0; index < nodeCount; index++, node = node->next)
	{
		CFStringRef tempCFString;
		Boolean state;
		
		// If this node is not a folder, skip to the next one
		if ((node->type != kNodeClosedFolder) && (node->type != kNodeOpenFolder)) continue;

		tempCFString = CFStringCreateWithPascalString (kCFAllocatorDefault, node->description, kCFStringEncodingMacRoman);
		if (tempCFString)
		{
			if (node->type == kNodeClosedFolder)
				state = false;
			else
				state = true;

			SetPrefID_FE ();
			SetPrefAsBoolean (state, tempCFString);
			CFRelease (tempCFString);
		}
	}
}


//===============================================================================
//	MainListDrawProc
//
//	Redraws the list when requested.
//===============================================================================

static pascal void MainListDrawProc(ControlHandle inControl, SInt16 inPart)
{
	static RgnHandle 	sRegion;
	Boolean				listActive = GetListActive(sMainList);
	Rect 				listRect;
	Rect 				viewRect;
	GrafPtr 			savePort;
	RGBColor			saveBackColor;
	ControlHandle currentFocus;

	// allocate the region if we haven't yet
	if (!sRegion)
		sRegion = NewRgn();
	if (!sRegion)
		return;

	// just redraw the list
	GetPort(&savePort);
	SetPortDialogPort(sMainDialog);

	// get the view bounds of the list and set the bottommost to the top
	GetListViewBounds(sMainList, &viewRect);
	sListBottomRedraw = viewRect.top;

	// set the background color to white; this reduces flicker during scrolling
	GetBackColor(&saveBackColor);
	RGBBackColor(&rgbWhite);

	// update the entire bounds
	GetControlBounds(inControl, &listRect);
	RectRgn(sRegion, &listRect);
	LUpdate(sRegion, sMainList);
	
	// if we didn't draw the whole thing, erase what's left
	viewRect.top = sListBottomRedraw;
	EraseRect(&viewRect);

	// restore the background color, or else the theme controls will be messed up
	RGBBackColor(&saveBackColor);

	// get the bounds and draw the theme parts
	DrawThemeListBoxFrame(&listRect, listActive ? kThemeStateInactive : kThemeStateActive);
	GetKeyboardFocus(GetControlOwner(inControl), & currentFocus);
	DrawThemeFocusRect(&listRect, listActive && (currentFocus == inControl));

	SetPort(savePort);
}


//===============================================================================
//	MainListKeyDownProc
//
//	Handles key down events when the main list has focus.
//===============================================================================

static pascal ControlPartCode MainListKeyDownProc(ControlHandle inControl, SInt16 inKeyCode, SInt16 inCharCode, SInt16 inModifiers)
{
	static char		sListSearchString[32];
	static UInt32	sLastKeyEventTime;

	GrafPtr 		savePort;
	RGBColor		saveBackColor;
	UInt32			current = GetSelectedCell(sMainList);
	ListBounds		visible;
	SInt32			delta;
	
	GetPort(&savePort);
	SetPortDialogPort(sMainDialog);

	// set the background color to white; this reduces flicker during scrolling
	GetBackColor(&saveBackColor);
	RGBBackColor(&rgbWhite);

	// determine which cells are visible
	GetListVisibleCells(sMainList, &visible);

	// switch off the code
	switch (inCharCode)
	{
		// handle the up arrow by selecting the previous cell
		case kUpArrowCharCode:
			SelectCell(sMainList, current - 1);
			break;

		// handle the down arrow by selecting the next cell
		case kDownArrowCharCode:
			SelectCell(sMainList, current + 1);
			break;
		
		// handle the page up by scrolling up one page
		case kPageUpCharCode:
			delta = visible.bottom - visible.top - 1;
			if (visible.top - delta >= 0)
				LScroll(0, -delta, sMainList);
			else
				LScroll(0, -visible.top, sMainList);
			break;
	
		// handle the page down by scrolling down one page
		case kPageDownCharCode:
			delta = visible.bottom - visible.top - 1;
			if (visible.bottom + delta < GetListDataBoundsBottom(sMainList))
				LScroll(0, delta, sMainList);
			else
				LScroll(0, GetListDataBoundsBottom(sMainList) - visible.bottom, sMainList);
			break;
	
		// handle home by scrolling to the top of the list
		case kHomeCharCode:
			LScroll(0, -visible.top, sMainList);
			break;
	
		// handle end by scrolling to the bottom of the list
		case kEndCharCode:
			delta = visible.bottom - visible.top;
			LScroll(0, GetListDataBoundsBottom(sMainList) - visible.bottom, sMainList);
			break;

		// handle tab by advancing or reversing control focus
		case kTabCharCode:
			if (inModifiers & shiftKey)
				ReverseKeyboardFocus(GetDialogWindow(sMainDialog));
			else
				AdvanceKeyboardFocus(GetDialogWindow(sMainDialog));
			break;

		// check everything else for ASCII characters and handle typeahead	
		default:
			if (inCharCode >= ' ')
			{
				UInt32			currentTime = TickCount();
				UInt32			index;

				// if too long has passed since the last key, reset the list string 
				if (currentTime > sLastKeyEventTime + LMGetKeyThresh() * 2)
					memset(sListSearchString, 0, sizeof(sListSearchString));
				sLastKeyEventTime = currentTime;

				// convert this character to lower case and add it to the string 
				inCharCode = tolower(inCharCode);
				sListSearchString[strlen(sListSearchString)] = inCharCode;
				
				// check for a match and move
				index = MatchCellText(sListSearchString);
				if (index != -1)
					SelectCell(sMainList, index);
			}
			break;
	}
	RGBBackColor(&saveBackColor);
	SetPort(savePort);

	return 1;
}


//===============================================================================
//	MainListHitTestProc
//
//	Performs hit testing.
//===============================================================================

static pascal ControlPartCode MainListHitTestProc(ControlHandle inControl, Point inWhere)
{
	// just return the generic part code of 1 to indicate that we got a hit
	return 1;
}


//===============================================================================
//	MainListTrackingProc
//
//	Performs hit testing.
//===============================================================================

static pascal ControlPartCode MainListTrackingProc(ControlHandle inControl, Point inStartPoint, ControlActionUPP inActionProc)
{
	ControlHandle	scrollbar;
	GrafPtr 		savePort;
	RGBColor		saveBackColor;

	GetPort(&savePort);
	SetPortDialogPort(sMainDialog);

	// Kludge:
	// Temporarily remove the user pane as the scrollbar's supervisor so that
	// LClick() will properly track it. After calling LClick(), reinstate the
	// user pane as supervisor so that this function gets called for clicks
	// in the scrollbar.
	scrollbar = GetListVerticalScrollBar(sMainList);
	if (scrollbar != NULL)
		SetControlSupervisor(scrollbar, NULL);

	// set the background color to white; this reduces flicker during scrolling
	GetBackColor(&saveBackColor);
	RGBBackColor(&rgbWhite);

	// handle the click in a standard manner
	if (LClick(inStartPoint, 0, sMainList))
		sFakeClickOnThis = kMainOKButton;

	// once again force clicks in scrollbar to be routed thru the user pane
	if (scrollbar != NULL)
		SetControlSupervisor(scrollbar, sMainListControl);

	RGBBackColor(&saveBackColor);
	SetPort(savePort);
	return 1;
}


//===============================================================================
//	MainListActivateProc
//
//	Performs activation handling.
//===============================================================================

static pascal void MainListActivateProc(ControlHandle inControl, Boolean inActivating)
{
	GrafPtr 		savePort;

	GetPort(&savePort);
	SetPortDialogPort(sMainDialog);

	// activate the list and redraw it (and the focus)
	LActivate(inActivating, sMainList);
	InvalidateList(sMainList);

	SetPort(savePort);
}


//===============================================================================
//	MainListFocusProc
//
//	Handles focus adjustments.
//===============================================================================

static pascal ControlPartCode MainListFocusProc(ControlRef inControl, ControlFocusPart inAction)
{
	Boolean gotFocus;
	OSErr theErr;
	ControlRef focusedControl;

	switch (inAction)
	{
		case kControlFocusNoPart:
			gotFocus = false;
			break;
		case kControlFocusNextPart:
		case kControlFocusPrevPart:
			theErr = GetKeyboardFocus(GetControlOwner(inControl), &focusedControl);
			if (focusedControl != inControl)
				gotFocus = true;
			else
				gotFocus = false;
			break;
		default:
			gotFocus = true;
			break;
	}

	// update the list (and its focus ring)
	{
		Rect listRect;
		Boolean listActive = IsControlActive(inControl);

		GetControlBounds(inControl, &listRect);
		DrawThemeListBoxFrame(&listRect, listActive ? kThemeStateInactive : kThemeStateActive);
		DrawThemeFocusRect(&listRect, listActive && gotFocus);
	}

	// return no part if we don't have focus
	return gotFocus ? 1 : kControlFocusNoPart;
}

static pascal OSStatus MainListMouseWheelMovedHandler(Point QDLocation, EventMouseWheelAxis axis, SInt32 delta)
{
	OSStatus result = noErr;

	switch (axis)
	{
		case kEventMouseWheelAxisX:
			// We don't care about this event, but we can pass it on
			result = eventNotHandledErr;
			break;

		case kEventMouseWheelAxisY:
			// Now, this is interesting.
			LScroll(0, -delta, sMainList);
			break;
	}

	return result;
}


#pragma mark -
#pragma mark ¥ Tabbed Pane Management

//===============================================================================
//	PrepareTabGroups
//
//	Loads and appends the DITLs for the various tabs.
//===============================================================================

static Boolean PrepareTabGroups(void)
{
	ControlRef		rootControl;
	UInt32			tabIndex;
	
	// get the root control
	if (GetRootControl(GetDialogWindow(sMainDialog), &rootControl) != noErr)
		return false;
	
	// loop over all tabs
	for (tabIndex = 1; tabIndex < kTabCountPlusOne; tabIndex++)
	{
		DialogItemIndex		startItem;
		DialogItemIndex		stopItem;
		DialogItemIndex		itemIndex;
		ControlRef			embedderControl;
		Handle				ditlResource;
		OSErr				err;

		// count the number of items we started with
		startItem = sTabItemBase[tabIndex] = CountDITL(sMainDialog) + 1;
		
		// get the new DITL for this pane
		if ((void *)AppendDialogItemList != (void *)kUnresolvedCFragSymbolAddress)
		{
			// use newer API if available
			err = AppendDialogItemList(sMainDialog, kTabDITLResourceBase + tabIndex, -(kMainInfoTabGroup + tabIndex - 1));
		}
		else
		{
			// fall back to old API if necessary
			ditlResource = Get1Resource('DITL', kTabDITLResourceBase + tabIndex);
			if (ditlResource)
			{
				AppendDITL(sMainDialog, ditlResource, -(kMainInfoTabGroup + tabIndex - 1));
				err = noErr;
			}
			else
			{
				OSErr temp = ResError();
				err = temp ? temp : resNotFound;
			}
		}
			
		if (err == noErr)
		{
			// get new control count
			stopItem = CountDITL(sMainDialog);
			
			// get the embedder control for this pane
			if (GetDialogItemAsControl(sMainDialog, kMainInfoTabGroup + tabIndex - 1, &embedderControl) != noErr)
				return false;
			
			// now embed all those items in the appropriate user pane
			for (itemIndex = startItem; itemIndex <= stopItem; itemIndex++)
			{
				ControlRef			superControlRef;
				ControlRef			controlRef;

				// get the control for this item
				if (GetDialogItemAsControl(sMainDialog, itemIndex, &controlRef) == noErr)
				{
					// scan up the hierarchy; if we hit the root control without hitting our
					// embedder control, then we need to manually embed the control in our user pane
					superControlRef = controlRef;
					while (GetSuperControl(superControlRef, &superControlRef) == noErr)
					{
						if (superControlRef == embedderControl)
							break;
						if (superControlRef == rootControl)
							break;
					}

					// if we didn't stop on our desired embedder, assume we have to do it manually
					if (superControlRef != embedderControl)
						EmbedControl(controlRef, embedderControl);
				}
			}
			
			// update the state of those tabs
			InitializeTabControls(tabIndex, sTabItemBase[tabIndex]);
		}
#if 0
		else
		{
			if (err == memFullErr)
				printf ("MacMAME has encountered a known bug with MacOS X 10.1. Please downgrade to either 10.0.4 or update to 10.1.2");
			else
				printf ("MacOS error #%d setting up the front-end", err);
		}
#endif
			
		// finally, hide the owner dialog item
		HideDialogItem(sMainDialog, kMainInfoTabGroup + tabIndex - 1);
	}

	return true;
}


//===============================================================================
//	SetActiveTab
//
//	Changes which tab is currently active, appending the appropriate DITL.
//===============================================================================

static void SetActiveTab(UInt32 inActiveTab)
{
	// if this is the same one that's up, skip it
	if (inActiveTab == sActiveTabControls)
		return;
		
	// hide the previous pane
	if (sActiveTabControls >= 1 && sActiveTabControls < kTabCountPlusOne)
		HideDialogItem(sMainDialog, kMainInfoTabGroup + sActiveTabControls - 1);

	// set the new active set
	sActiveTabControls = inActiveTab;
	SetDialogControlValue(sMainDialog, kMainTabControl, inActiveTab);

	// make it visible
	if (sActiveTabControls >= 1 && sActiveTabControls < kTabCountPlusOne)
		ShowDialogItem(sMainDialog, kMainInfoTabGroup + sActiveTabControls - 1);
}
	

//===============================================================================
//	InitializeTabControls
//
//	Initializes the state of all the controls in a tab.
//===============================================================================

static void InitializeTabControls(UInt32 inTabIndex, DialogItemIndex inBaseIndex)
{
	char tempFileName[256];
	
	switch (inTabIndex)
	{
		// initialize the info tab
		case kTabInfo:
			InfoInitializeUserPane(sMainDialog, inBaseIndex);
			break;
		
		// initialize the video tab
		case kTabVideo:
			SetDialogControlValue(sMainDialog, inBaseIndex + kVideoScalePopup, GetMenuItemForScaleInterlace());
			SetDialogControlValue(sMainDialog, inBaseIndex + kVideoHideDesktopCheck, gPrefs.hideDesktop);
			SetDialogControlValue(sMainDialog, inBaseIndex + kVideoRotateGroupCheck, gPrefs.standardRotation);
			SetDialogControlValue(sMainDialog, inBaseIndex + kVideoRotateLeftCheck, gPrefs.rotateLeft);
			SetDialogControlValue(sMainDialog, inBaseIndex + kVideoRotateRightCheck, gPrefs.rotateRight);
			SetDialogControlValue(sMainDialog, inBaseIndex + kVideoRotateFlipXCheck, gPrefs.flipX);
			SetDialogControlValue(sMainDialog, inBaseIndex + kVideoRotateFlipYCheck, gPrefs.flipY);
			SetDialogControlValue(sMainDialog, inBaseIndex + kVideoRotateNoneCheck, gPrefs.noRotation);
			SetDialogControlValue(sMainDialog, inBaseIndex + kVideoVectorFlickerSlider, gPrefs.flicker);
			SetDialogControlValue(sMainDialog, inBaseIndex + kVideoVectorBeamWidthSlider, gPrefs.beamWidth);
			SetDialogControlValue(sMainDialog, inBaseIndex + kVideoVectorAntialiasCheck, gPrefs.antialias);
			SetDialogControlValue(sMainDialog, inBaseIndex + kVideoVectorTranslucentCheck, gPrefs.translucency);
			SetDialogControlValue(sMainDialog, inBaseIndex + kVideoArtGroupCheck, gPrefs.artworkAll);
			SetDialogControlValue(sMainDialog, inBaseIndex + kVideoArtBackdropCheck, gPrefs.artworkBackdrop);
			SetDialogControlValue(sMainDialog, inBaseIndex + kVideoArtOverlayCheck, gPrefs.artworkOverlay);
			SetDialogControlValue(sMainDialog, inBaseIndex + kVideoArtBezelCheck, gPrefs.artworkBezel);
			break;
			
		// initialize the audio tab
		case kTabAudio:
			SetDialogControlValue(sMainDialog, inBaseIndex + kAudioEnableCheck, gPrefs.playSound);
			if (gPrefs.sampleRate == 11025)
				SetDialogControlValue(sMainDialog, inBaseIndex + kAudioSampleRateMenu, 1);
			else if (gPrefs.sampleRate == 22050)
				SetDialogControlValue(sMainDialog, inBaseIndex + kAudioSampleRateMenu, 2);
			else
				SetDialogControlValue(sMainDialog, inBaseIndex + kAudioSampleRateMenu, 3);
			SetDialogControlValue(sMainDialog, inBaseIndex + kAudioVolumeSlider, gPrefs.attenuation);
			break;
		
		// initialize the reports tab
		case kTabReports:
			break;
		
		// initialize the misc tab
		case kTabMisc:
			SetDialogControlValue(sMainDialog, inBaseIndex + kMiscAutoFrameskipCheck, gPrefs.autoSkip);
			SetDialogControlValue(sMainDialog, inBaseIndex + kMiscThrottleCheck, gPrefs.speedThrottle);
			SetDialogControlValue(sMainDialog, inBaseIndex + kMiscErrorlogCheck, gPrefs.errorLog);
			SetDialogControlValue(sMainDialog, inBaseIndex + kMiscEnableCheatsCheck, gPrefs.cheat);
			SetDialogControlValue(sMainDialog, inBaseIndex + kMiscNoGameInfoCheck, gPrefs.noGameInfo);
			SetDialogControlValue(sMainDialog, inBaseIndex + kMiscNoDisclaimerCheck, gPrefs.noDisclaimer);
			if (gRecordingFileURL)
			{
				GetFilenameFromURL (gRecordingFileURL, tempFileName, sizeof(tempFileName));
				c2pstrcpy ((unsigned char *)tempFileName, tempFileName);
			}
			SetDialogItemTextAppearance(sMainDialog, inBaseIndex + kMiscLoadReplayText, 
						(gRecordingState == kRecordingInput) ? (StringPtr)tempFileName : "\p");
			SetDialogItemTextAppearance(sMainDialog, inBaseIndex + kMiscSaveReplayText, 
						(gRecordingState == kRecordingOutput) ? (StringPtr)tempFileName : "\p");
			if (gLangURL)
			{
				GetFilenameFromURL (gLangURL, tempFileName, sizeof(tempFileName));
				c2pstrcpy ((unsigned char *)tempFileName, tempFileName);
			}
			SetDialogItemTextAppearance(sMainDialog, inBaseIndex + kMiscLoadLangText, gLangURL ? (StringPtr)tempFileName : "\p");
			if (gCheatURL)
			{
				GetFilenameFromURL (gCheatURL, tempFileName, sizeof(tempFileName));
				c2pstrcpy ((unsigned char *)tempFileName, tempFileName);
			}
			SetDialogItemTextAppearance(sMainDialog, inBaseIndex + kMiscLoadCheatText, gCheatURL ? (StringPtr)tempFileName : "\p");
			sLastBios = NULL;
			break;

#ifdef MESS
		// initialize the MESS tab
		case kTabMESS:
			MESSInitializeTab(sMainDialog, inBaseIndex);
			break;
#endif		
	}
}


//===============================================================================
//	UpdateTabControls
//
//	Updates the state of all the controls in a tab.
//===============================================================================

static void UpdateTabControls(UInt32 inTabIndex, DialogItemIndex inBaseIndex)
{
	const ListNode *			node = GetSelectedNode();
	const ROMSetData *			romset = node ? node->romset : NULL;
	const game_driver *	driver = romset ? romset->driver : NULL;
	machine_config machine;

	if (driver)
		expand_machine_driver(driver->drv, &machine);
	
	switch (inTabIndex)
	{
		// update the info controls
		case kTabInfo:
			if (sActiveTabControls == inTabIndex)
				InfoUpdateUserPane(sMainDialog, inBaseIndex, driver);
			break;
		
		// update the video controls
		case kTabVideo:
		{
			const DisplayDescription *	plugin 				= GetActivePluginDescription();
			Boolean						rotateGroupEnable 	= !GetDialogControlValue(sMainDialog, inBaseIndex + kVideoRotateGroupCheck);
			Boolean						artGroupEnable 		= !GetDialogControlValue(sMainDialog, inBaseIndex + kVideoArtGroupCheck);
			Boolean						isVector 			= driver && ((machine.video_attributes & VIDEO_TYPE_VECTOR) != 0);

			SetDialogControlActive(sMainDialog, inBaseIndex + kVideoHideDesktopCheck, !plugin->fullscreen);
			SetDialogControlActive(sMainDialog, inBaseIndex + kVideoScalePopup, plugin->scale);
			SetDialogControlActive(sMainDialog, inBaseIndex + kVideoRotateLeftCheck, rotateGroupEnable);
			SetDialogControlActive(sMainDialog, inBaseIndex + kVideoRotateRightCheck, rotateGroupEnable);
			SetDialogControlActive(sMainDialog, inBaseIndex + kVideoRotateFlipXCheck, rotateGroupEnable);
			SetDialogControlActive(sMainDialog, inBaseIndex + kVideoRotateFlipYCheck, rotateGroupEnable);
			SetDialogControlActive(sMainDialog, inBaseIndex + kVideoRotateNoneCheck, rotateGroupEnable);
			SetDialogControlActive(sMainDialog, inBaseIndex + kVideoVectorGroup, isVector);
			SetDialogControlActive(sMainDialog, inBaseIndex + kVideoArtBackdropCheck, artGroupEnable);
			SetDialogControlActive(sMainDialog, inBaseIndex + kVideoArtOverlayCheck, artGroupEnable);
			SetDialogControlActive(sMainDialog, inBaseIndex + kVideoArtBezelCheck, artGroupEnable);

			if (plugin->fullscreen) SetDialogControlValue(sMainDialog, inBaseIndex + kVideoHideDesktopCheck, true);
			if (!plugin->scale) SetDialogControlValue(sMainDialog, inBaseIndex + kVideoScalePopup, 1);
			break;
		}
			
		// update the audio controls
		case kTabAudio:
		{
			Boolean	soundEnabled = GetDialogControlValue(sMainDialog, inBaseIndex + kAudioEnableCheck);
			SetDialogControlActive(sMainDialog, inBaseIndex + kAudioSampleRateMenu, soundEnabled);
			SetDialogControlActive(sMainDialog, inBaseIndex + kAudioVolumeSlider, soundEnabled);
			SetDialogControlActive(sMainDialog, inBaseIndex + kAudioVolumeLowIcon, soundEnabled);
			SetDialogControlActive(sMainDialog, inBaseIndex + kAudioVolumeHighIcon, soundEnabled);
			break;
		}
		
		// update the reports controls
		case kTabReports:
			break;
		
		// update the misc controls
		case kTabMisc:
		{
			const bios_entry *bios;

			if (driver)
				bios = driver->bios;
			else
				bios = NULL;

			if (bios && (sLastBios != bios))
			{
				MenuRef biosMenu = GetDialogItemMenu (sMainDialog, inBaseIndex + kMiscBIOSPopup);
				ControlRef menuControl;
				OSErr err;

				sLastBios = bios;
				err = GetDialogItemAsControl (sMainDialog, inBaseIndex + kMiscBIOSPopup, &menuControl);
				
				if ((err == noErr) && biosMenu)
				{
					Str255 itemName;
					UInt32 index = 0;
					
					DeleteMenuItems (biosMenu, 1, CountMenuItems (biosMenu));
					
					while (!BIOSENTRY_ISEND(bios))
					{
						c2pstrcpy (itemName, bios->_description);
						(void)AppendMenuItemText (biosMenu, itemName);
						EnableMenuItem (biosMenu, index + 1);
						bios ++; index ++;
					}
					
					// fix the minimum and maximum
					SetControlMinimum (menuControl, 1);
					SetControlMaximum (menuControl, index);

					// set the menu to a reasonable BIOS selection
					if (gBIOSSelection <= index)
						SetControlValue (menuControl, gBIOSSelection);
					else
						SetControlValue (menuControl, 1);
				}
			}

			SetDialogControlActive(sMainDialog, inBaseIndex + kMiscLoadCheatButton, false);
			SetDialogControlActive(sMainDialog, inBaseIndex + kMiscBIOSPopup, bios != NULL);
			break;
		}
		
#ifdef MESS
		// update the MESS tab
		case kTabMESS:
			if( sActiveTabControls == kTabMESS )
				MESSUpdateTab(sMainDialog, inBaseIndex, driver, false);
			break;
#endif		
	}
}


//===============================================================================
//	HandleTabControl
//
//	Handles user interaction with a tab control.
//===============================================================================
WindowRef parentWin;
static void HandleTabControls(UInt32 inTabIndex, DialogItemIndex inBaseIndex, DialogItemIndex inItemHit)
{
	const ListNode *			node = GetSelectedNode();
	const ROMSetData *			romset = node ? node->romset : NULL;
	const game_driver *	driver = romset ? romset->driver : NULL;
	
	CFStringRef defaultFileName = NULL;
	CFStringRef promptString = NULL;
	CFURLRef tempURL = NULL;
	char tempFileName[kMacMaxPath];

parentWin = GetDialogWindow(sMainDialog);

	// handle checkboxes very generically
	if (GetDialogItemType(sMainDialog, inBaseIndex + inItemHit) == kCheckBoxDialogItem)
	{
		SetDialogControlValue(sMainDialog, inBaseIndex + inItemHit,
					!GetDialogControlValue(sMainDialog, inBaseIndex + inItemHit));
	}
	
	// otherwise, do the full switch
	else
	{
		switch (inTabIndex)
		{
			// handle item hits in the info controls
			case kTabInfo:
				InfoHandleUserPane(sMainDialog, inBaseIndex, inItemHit, driver);
				break;
			
			// handle item hits in the video controls
			case kTabVideo:
				if ((inItemHit == kVideoRotateGroupCheck) || (inItemHit == kVideoArtGroupCheck))
					SetDialogControlValue(sMainDialog, inBaseIndex + inItemHit,
								!GetDialogControlValue(sMainDialog, inBaseIndex + inItemHit));
				break;
				
			// handle item hits in the audio controls
			case kTabAudio:
				break;
				
			// handle item hits in the reports controls
			case kTabReports:
				switch (inItemHit)
				{
					case kReportsReportsButton:
						SaveReport(0);
						break;

					case kReportsAuditROMsButton:
						AuditROMs();
						break;

					case kReportsAuditSamplesButton:
						AuditSamples();
						break;

					case kReportsAnalyzeROMsButton:
						AnalyzeRomsets();
						break;

					case kReportsIdentifyROMsButton:
						DoRomident();
						break;

					case kReportMAMEGameInfo:
					{
						CFURLRef tempURL = NULL;
						if (_NavPutOneFile(promptString, CFCopyLocalizedString(CFSTR("MAMEGameInfoXML"), NULL), MAC_FILETYPE_REPORTS, &tempURL, NULL))
						{
							FILE *tempFile = NULL;
							Boolean successful;
							
							successful = CFURLGetFileSystemRepresentation(tempURL, true, (UInt8 *)tempFileName, sizeof(tempFileName));
							tempFile = fopen (tempFileName, "w");
							if (tempFile)
							{
								print_mame_xml (tempFile, drivers);
								fclose (tempFile);
							}
						}
						break;
					}
				}
				break;
				
			// handle item hits in the misc controls
			case kTabMisc:
				switch (inItemHit)
				{
					case kMiscLoadReplayButton:
						promptString = CFCopyLocalizedString(CFSTR("PromptToLoadReplayFile"), NULL);

						if (_NavGetOneFile(promptString, FILETYPE_INPUTLOG, &tempURL))
						{
							gRecordingState = kRecordingInput;
							if (gRecordingFileURL)
							{
								CFRelease (gRecordingFileURL);
							}
							gRecordingFileURL = tempURL;
							GetFilenameFromURL (gRecordingFileURL, tempFileName, sizeof(tempFileName));
							c2pstrcpy ((unsigned char *)tempFileName, tempFileName);
							SetDialogItemTextAppearance(sMainDialog, inBaseIndex + kMiscLoadReplayText, (ConstStr255Param)tempFileName);
							SetDialogItemTextAppearance(sMainDialog, inBaseIndex + kMiscSaveReplayText, "\p");							
						}
						break;

					case kMiscClearLoadReplay:
						gRecordingState = kRecordingNone;
						if (gRecordingFileURL)
						{
							CFRelease (gRecordingFileURL);
							gRecordingFileURL = NULL;
						}
						SetDialogItemTextAppearance(sMainDialog, inBaseIndex + kMiscLoadReplayText, "\p");
						break;

					case kMiscSaveReplayButton:
					{
						const ROMSetData *romSet;
						
						romSet = GetSelectedROMSet();
						defaultFileName = CFStringCreateWithFormat (NULL, NULL, CFSTR("%s.inp"), romSet ? romSet->driver->name : "pacman");
						promptString = CFCopyLocalizedString(CFSTR("PromptToSaveFile"), NULL);

						if (_NavPutOneFile(promptString, defaultFileName, FILETYPE_INPUTLOG, &tempURL, NULL))
						{
							gRecordingState = kRecordingOutput;
							if (gRecordingFileURL)
							{
								CFRelease (gRecordingFileURL);
							}
							gRecordingFileURL = tempURL;
							SetDialogItemTextAppearance(sMainDialog, inBaseIndex + kMiscLoadReplayText, "\p");
							GetFilenameFromURL (gRecordingFileURL, tempFileName, sizeof(tempFileName));
							c2pstrcpy ((unsigned char *)tempFileName, tempFileName);
							SetDialogItemTextAppearance(sMainDialog, inBaseIndex + kMiscSaveReplayText, (ConstStr255Param)tempFileName);
						}
						break;
					}
					
					case kMiscClearSaveReplay:
						gRecordingState = kRecordingNone;
						if (gRecordingFileURL)
						{
							CFRelease (gRecordingFileURL);
							gRecordingFileURL = NULL;
						}
						SetDialogItemTextAppearance(sMainDialog, inBaseIndex + kMiscSaveReplayText, "\p");
						break;

					case kMiscLoadLangButton:
						promptString = CFCopyLocalizedString(CFSTR("PromptToLoadLangFile"), NULL);

						if (_NavGetOneFile(promptString, FILETYPE_LANGUAGE, &tempURL))
						{
							if (gLangURL)
							{
								CFRelease (gLangURL);
							}
							gLangURL = tempURL;
							GetFilenameFromURL (gLangURL, tempFileName, sizeof(tempFileName));
							c2pstrcpy ((unsigned char *)tempFileName, tempFileName);
							SetDialogItemTextAppearance(sMainDialog, inBaseIndex + kMiscLoadLangText, (ConstStr255Param)tempFileName);
						}
						break;

					case kMiscClearLoadLang:
						if (gLangURL)
						{
							CFRelease (gLangURL);
							gLangURL = NULL;
						}
						SetDialogItemTextAppearance(sMainDialog, inBaseIndex + kMiscLoadLangText, "\p");
						break;

					case kMiscLoadCheatButton:
						promptString = CFCopyLocalizedString(CFSTR("PromptToLoadCheatFile"), NULL);

						if (_NavGetOneFile(promptString, FILETYPE_CHEAT, &tempURL))
						{
							if (gCheatURL)
							{
								CFRelease (gCheatURL);
							}
							gCheatURL = tempURL;
							GetFilenameFromURL (gCheatURL, tempFileName, sizeof(tempFileName));
							c2pstrcpy ((unsigned char *)tempFileName, tempFileName);
							SetDialogItemTextAppearance(sMainDialog, inBaseIndex + kMiscLoadCheatText, (ConstStr255Param)tempFileName);
						}
						break;

					case kMiscClearLoadCheat:
						if (gCheatURL)
						{
							CFRelease (gCheatURL);
							gCheatURL = NULL;
						}
						SetDialogItemTextAppearance(sMainDialog, inBaseIndex + kMiscLoadCheatText, "\p");
						break;
				}
				break;

#ifdef MESS
			// handle item hits in the MESS controls
			case kTabMESS:
				MESSHandleTab(sMainDialog, inBaseIndex, inItemHit, driver);
				break;
#endif			
		}
	}

	// since we may display a sub-dialog, reset the port	
	SetPortDialogPort(sMainDialog);
	
	// update the state of things
	UpdateTabControls(inTabIndex, inBaseIndex);
	
	if (defaultFileName)
		CFRelease (defaultFileName);
	if (promptString)
		CFRelease (promptString);
}


//===============================================================================
//	GetCurrentPane
//
//	Called by drag&drop (MESS) to determine which pane is displayed.
//===============================================================================

UInt32 GetCurrentPane( void )
{
	return sActiveTabControls;
}

//===============================================================================
//	GetBaseIndex
//
//	Called by drag&drop (MESS) to determine the base index for the panel.
//===============================================================================

DialogItemIndex GetBaseIndex( UInt32 pane )
{
	return sTabItemBase[ pane ];
}

//===============================================================================
//	TearDownTabControls
//
//	Called as the tab controls are being torn down.
//===============================================================================

static void TearDownTabControls(UInt32 inTabIndex, DialogItemIndex inBaseIndex)
{
	switch (inTabIndex)
	{
		// tear down the info controls
		case kTabInfo:
			InfoTearDownUserPane(sMainDialog, inBaseIndex);
			break;
		
		// tear down the video controls
		case kTabVideo:
		{
			gPrefs.windowScale 	= GetDialogControlValue(sMainDialog, inBaseIndex + kVideoScalePopup) / 2 + 1;
			gPrefs.hideDesktop 	= GetDialogControlValue(sMainDialog, inBaseIndex + kVideoHideDesktopCheck);
			gPrefs.interlace 	= (gPrefs.windowScale != 1 &&
						GetDialogControlValue(sMainDialog, inBaseIndex + kVideoScalePopup) % 2 == 1);
			gPrefs.flicker 		= GetDialogControlValue(sMainDialog, inBaseIndex + kVideoVectorFlickerSlider);
			gPrefs.beamWidth 	= GetDialogControlValue(sMainDialog, inBaseIndex + kVideoVectorBeamWidthSlider);
			gPrefs.antialias 	= GetDialogControlValue(sMainDialog, inBaseIndex + kVideoVectorAntialiasCheck);
			gPrefs.translucency = GetDialogControlValue(sMainDialog, inBaseIndex + kVideoVectorTranslucentCheck);

			gPrefs.standardRotation = GetDialogControlValue(sMainDialog, inBaseIndex + kVideoRotateGroupCheck);
			gPrefs.rotateLeft 	= GetDialogControlValue(sMainDialog, inBaseIndex + kVideoRotateLeftCheck);
			gPrefs.rotateRight 	= GetDialogControlValue(sMainDialog, inBaseIndex + kVideoRotateRightCheck);
			gPrefs.flipX 		= GetDialogControlValue(sMainDialog, inBaseIndex + kVideoRotateFlipXCheck);
			gPrefs.flipY 		= GetDialogControlValue(sMainDialog, inBaseIndex + kVideoRotateFlipYCheck);
			gPrefs.noRotation 	= GetDialogControlValue(sMainDialog, inBaseIndex + kVideoRotateNoneCheck);

			gPrefs.artworkAll 		= GetDialogControlValue(sMainDialog, inBaseIndex + kVideoArtGroupCheck);
			gPrefs.artworkBackdrop 	= GetDialogControlValue(sMainDialog, inBaseIndex + kVideoArtBackdropCheck);
			gPrefs.artworkOverlay 	= GetDialogControlValue(sMainDialog, inBaseIndex + kVideoArtOverlayCheck);
			gPrefs.artworkBezel 	= GetDialogControlValue(sMainDialog, inBaseIndex + kVideoArtBezelCheck);
			break;
		}
		
		// tear down the audio controls
		case kTabAudio:
			gPrefs.playSound	 	= GetDialogControlValue(sMainDialog, inBaseIndex + kAudioEnableCheck);
			gPrefs.sampleRate	 	= 11025 << (GetDialogControlValue(sMainDialog, inBaseIndex + kAudioSampleRateMenu) - 1);
			gPrefs.attenuation		= GetDialogControlValue(sMainDialog, inBaseIndex + kAudioVolumeSlider);
			break;
		
		// tear down the reports controls
		case kTabReports:
			break;

		// tear down the misc controls
		case kTabMisc:
			gPrefs.autoSkip 		= GetDialogControlValue(sMainDialog, inBaseIndex + kMiscAutoFrameskipCheck);
			gPrefs.speedThrottle 	= GetDialogControlValue(sMainDialog, inBaseIndex + kMiscThrottleCheck);
			gPrefs.errorLog	 		= GetDialogControlValue(sMainDialog, inBaseIndex + kMiscErrorlogCheck);
			gPrefs.cheat		 	= GetDialogControlValue(sMainDialog, inBaseIndex + kMiscEnableCheatsCheck);
			gPrefs.noGameInfo	 	= GetDialogControlValue(sMainDialog, inBaseIndex + kMiscNoGameInfoCheck);
			gPrefs.noDisclaimer	 	= GetDialogControlValue(sMainDialog, inBaseIndex + kMiscNoDisclaimerCheck);

			gBIOSSelection 		 	= GetDialogControlValue(sMainDialog, inBaseIndex + kMiscBIOSPopup);
			break;

#ifdef MESS
		// tear down the MESS controls
		case kTabMESS:
			MESSTearDownTab(sMainDialog, inBaseIndex);
			break;
#endif
	}
}


#pragma mark -
#pragma mark ¥ Grouping & Sorting

//===============================================================================
//	InitializeGroupMenu
//
//	Searches for categories and initializes the grouping menu.
//===============================================================================

void InitializeGroupMenu(void)
{
	MenuRef				menuRef = GetDialogItemMenu(sMainDialog, kMainGroupPopup);
	CategoryFileSpec *	lastCategory = NULL;
	CategoryFileSpec *	category;
	FSSpec				tempSpec, folderSpec;
	SInt32 				folderID;
	CInfoPBRec 			pb;
	UInt32 				index;
	Str255 				name;
	OSErr 				err;
	ControlRef			controlRef;

	// free any existing pools and create a new one to allocate from
	if (sCategoryPool != NULL)
		DisposeMemoryPool(sCategoryPool);
	sCategoryPool = CreateMemoryPool(sizeof(CategoryFileSpec), 10);
	sFirstCategorySpec = NULL;

	// now find "Categories" folder
	err = GetFSSpecForMAMEFolder(MAC_FILETYPE_CATEGORIES, &folderSpec);
	if (err != noErr)
		return;
	
	// get the directory ID of this directory
	memset(&pb, 0, sizeof(pb));
	pb.hFileInfo.ioNamePtr = folderSpec.name;
	pb.hFileInfo.ioVRefNum = folderSpec.vRefNum;
	pb.hFileInfo.ioFDirIndex = 0;
	pb.hFileInfo.ioDirID = folderSpec.parID;
	err = PBGetCatInfoSync(&pb);
	if (err != noErr)
		return;
	folderID = pb.hFileInfo.ioDirID;

	// scan for files with the proper filetype
	for (index = 1; index < 100000; index++)
	{
		// get the next indexed file
		pb.hFileInfo.ioNamePtr = name;
		pb.hFileInfo.ioVRefNum = folderSpec.vRefNum;
		pb.hFileInfo.ioFDirIndex = index;
		pb.hFileInfo.ioDirID = folderID;
		err = PBGetCatInfoSync(&pb);
		if (err != noErr)
			break;
		
		// make a spec from the found file and skip if it's not a valid category
		FSMakeFSSpec(pb.hFileInfo.ioVRefNum, pb.hFileInfo.ioFlParID, name, &tempSpec);
		if (!IsValidCategoryFile(&tempSpec))
			continue;

		// allocate a new entry
		category = AllocateFromPool(sCategoryPool);
		category->next = NULL;
		category->spec = tempSpec;
		
		// link it in
		if (lastCategory != NULL)
			lastCategory->next = category;
		else
			sFirstCategorySpec = category;
		lastCategory = category;
	}
	
	// now add the menu items
	for (index = 0, category = sFirstCategorySpec; category != NULL; category = category->next, index++)
	{
		char	temp[256];
		
		p2cstrcpy(temp, category->spec.name);
		name[0] = sprintf((char *)&name[1], GetIndCString(rGroupMenuStrings, kGroupBy, "Group by %s"), temp);
		InsertMenuItemText(menuRef, name, kGroupMenuDivider - 1 + index);
	}

	// set the new control maximum
	if (GetDialogItemAsControl(sMainDialog, kMainGroupPopup, &controlRef) == noErr)
		SetControlMaximum(controlRef, MemoryPoolItems(sCategoryPool) + kGroupMenuItemCountPlusOne - 1);
}


//===============================================================================
//	HandleSortingChange
//
//	Handles a change in the sorting menu, which will affect the sorting order.
//===============================================================================

static void HandleSortingChange(void)
{
	UInt32				menuItem = GetDialogControlValue(sMainDialog, kMainGroupPopup);
	GroupSorting		oldSort = gFEPrefs.grouping;
	GroupFlags			oldFlags = gFEPrefs.groupFlags;
	CategoryFileSpec *	category;
	
	// switch off the menu item
	switch (menuItem)
	{
		// group by folder
		case kGroupMenuByFolder:
			gFEPrefs.grouping = kGroupByFolder;
			break;

		// group by manufacturer
		case kGroupMenuByManufacturer:
			gFEPrefs.grouping = kGroupByManufacturer;
			break;

		// group by date
		case kGroupMenuByDate:
			gFEPrefs.grouping = kGroupByDate;
			break;
		
		// anything else
		default:
			
			// scan for category files
			for (category = sFirstCategorySpec; category != NULL; category = category->next, menuItem--)
				if (menuItem == kGroupMenuDivider)
				{
					gFEPrefs.grouping = kGroupByFile;
					if (!FSSpecsMatch(&gFEPrefs.groupFilespec, &category->spec))
					{
						gFEPrefs.groupFilespec = category->spec;
						oldSort = 0;
					}
					break;
				}
			
			// if we exhausted all of those, switch off the rest
			if (category == NULL)
			{
				switch (menuItem)
				{
					// show/hide clones
					case kGroupMenuShowClones:
						gFEPrefs.groupFlags ^= kGroupFlagHideClones;
						break;

					// show/hide ghosts
					case kGroupMenuShowGhosts:
						gFEPrefs.groupFlags ^= kGroupFlagShowGhosts;
						break;

					// show/hide non-working
					case kGroupMenuShowNonWorking:
						gFEPrefs.groupFlags ^= kGroupFlagHideNonWorking;
						break;
				}
			}
			break;
	}
	
	// invalidate if changed
	if (oldSort != gFEPrefs.grouping)
		InvalidateListGrouping();
	if (oldFlags != gFEPrefs.groupFlags)
		sListFilteringInvalid = true;
	
	// adjust the menu
	UpdateGroupMenu();
}


//===============================================================================
//	UpdateGroupMenu
//
//	Updates the state of the group menu.
//===============================================================================

static void UpdateGroupMenu(void)
{
	MenuRef				menuRef = GetDialogItemMenu(sMainDialog, kMainGroupPopup);
	UInt32				dynamicItems = MemoryPoolItems(sCategoryPool);
	CategoryFileSpec *	category;
	UInt32				menuIndex;
	
	// first, pick the menu item based on the grouping
	switch (gFEPrefs.grouping)
	{
		default:
		case kGroupByFolder:
			menuIndex = kGroupMenuByFolder;
			break;
			
		case kGroupByManufacturer:
			menuIndex = kGroupMenuByManufacturer;
			break;
			
		case kGroupByDate:
			menuIndex = kGroupMenuByDate;
			break;
			
		case kGroupByFile:
			menuIndex = kGroupMenuDivider;
			for (category = sFirstCategorySpec; category != NULL; category = category->next, menuIndex++)
				if (FSSpecsMatch(&gFEPrefs.groupFilespec, &category->spec))
					break;
			
			// if we didn't find it, default to folder
			if (category == NULL)
			{
				gFEPrefs.grouping = kGroupByFolder;
				SetDialogControlValue(sMainDialog, kMainGroupPopup, kGroupMenuByFolder);
				HandleSortingChange();
				return;
			}
			break;
	}
	
	// set the current menu item
	SetDialogControlValue(sMainDialog, kMainGroupPopup, menuIndex);
	
	// adjust the show/hide clones item, disabling show/hide virtual clones if all clones are hidden
	if (gFEPrefs.groupFlags & kGroupFlagHideClones)
	{
		SetMenuItemText(menuRef, kGroupMenuShowClones + dynamicItems, GetIndPString(rGroupMenuStrings, kShowClones, "\pShow clones"));
		DisableMenuItem(menuRef, kGroupMenuShowGhosts + dynamicItems);
	}
	else
	{
		SetMenuItemText(menuRef, kGroupMenuShowClones + dynamicItems, GetIndPString(rGroupMenuStrings, kHideClones, "\pHide clones"));
		EnableMenuItem(menuRef, kGroupMenuShowGhosts + dynamicItems);
	}

	// update show/hide virtual clones
	if (gFEPrefs.groupFlags & kGroupFlagShowGhosts)
		SetMenuItemText(menuRef, kGroupMenuShowGhosts + dynamicItems, GetIndPString(rGroupMenuStrings, kHideGhosts, "\pHide ghost clones"));
	else
		SetMenuItemText(menuRef, kGroupMenuShowGhosts + dynamicItems, GetIndPString(rGroupMenuStrings, kShowGhosts, "\pShow ghost clones"));

	// update show/hide non-working drivers
	if (gFEPrefs.groupFlags & kGroupFlagHideNonWorking)
		SetMenuItemText(menuRef, kGroupMenuShowNonWorking + dynamicItems, GetIndPString(rGroupMenuStrings, kShowNonWorking, "\pShow non-working games"));
	else
		SetMenuItemText(menuRef, kGroupMenuShowNonWorking + dynamicItems, GetIndPString(rGroupMenuStrings, kHideNonWorking, "\pHide non-working games"));
}


//===============================================================================
//	FSSpecsMatch
//
//	Returns true if two FSSpecs match.
//===============================================================================

static Boolean FSSpecsMatch(const FSSpec *inSpec1, const FSSpec *inSpec2)
{
	return (inSpec1->vRefNum == inSpec2->vRefNum && inSpec1->parID == inSpec2->parID && 
			EqualString(inSpec1->name, inSpec2->name, false, true));
}



#pragma mark -
#pragma mark ¥ Contextual Menus

//===============================================================================
//	HandleListContextualClick
//
//	Handles contextual menus for the frontend list.  Returns true if the event
//	was handled.
//===============================================================================

static Boolean HandleListContextualClick(const EventRecord *inEvent, DialogItemIndex *outItemHit)
{
	Point			location = inEvent->where;
	Rect			listRect;
	ListNode *		node = NULL;
	
	// see if we're actually within the list
	SetPortDialogPort(sMainDialog);
	GlobalToLocal(&location);
	GetListViewBounds(sMainList, &listRect);
	if (!PtInRect(location, &listRect))
		return false;

	// if it's an actual contextual menu click, process it	
	if (IsShowContextualMenuClick(inEvent))
	{
		ListBounds	visible;
		Cell		theCell = { 0, 0 };
		
		(void)GetListVisibleCells(sMainList, &visible);
		
		// for each visible list cell, check to see	if the click was in it
		for (theCell.v = visible.top; theCell.v < visible.bottom; theCell.v++)
		{
			Rect	cellRect;
			
			LRect(&cellRect, theCell, sMainList);
			if (PtInRect(location, &cellRect))
			{
				// select the clicked cell
				SelectCell(sMainList, theCell.v);
				
				// get the clicked node
				node = GetIndexedNodeFromList(theCell.v);
				break;
			}
		}
	}
	else
		return false;

	// if the newly selected node has a driver, display the menu
	if (node != NULL && node->romset != NULL && node->romset->driver != NULL)
		DoRomsetContextualMenu(inEvent, node->romset, sMainList);
	
	// tell calling function that we handled the event
	*outItemHit = 0;
	return true;
}


//===============================================================================
//	DoRomsetContextualMenu
//
//	Builds and displays a contextual menu for the specified romset.
//===============================================================================

static OSErr DoRomsetContextualMenu(const EventRecord *inEvent, const ROMSetData *inRomset, ListRef inList)
{
	const ROMSetData *menuRomsets[20] = { 0 };
	const game_driver	*driver = inRomset->driver;
	MenuHandle	theMenu;
	MenuItemIndex	item, moveToTrashItem, revealInFinderItem = -1;
	UInt32		selectionType;
	SInt16		selectionID;
	UInt16		selectionItem;
	OSErr		err;
	char		tempString[256];

	// create a new menu
	theMenu = NewMenu(200, "\pUntitled");
	if (!theMenu) return false;

	if (inRomset->format != kROMSetFormatGhost)
	{
		// add "Reveal in Finder..."
		AppendMenuItemTextWithCFString (theMenu, CFCopyLocalizedString(CFSTR("FE_ContextMenu_RevealInFinder"), NULL), 0, NULL, &item);
		revealInFinderItem = item;
		
		// add a divider
		AppendMenuItemTextWithCFString (theMenu, CFSTR(""), kMenuItemAttrSeparator, NULL, &item);
	}
	
	// add the driver name
	AppendMenuItemTextWithCFString (theMenu, CFStringCreateWithFormat(NULL, 0, CFCopyLocalizedString(CFSTR("FE_ContextMenu_Driver"), NULL), driver->name),
		kMenuItemAttrDisabled, NULL, &item);
	
	// add the driver description
	AppendMenuItemTextWithCFString (theMenu, CFStringCreateWithFormat(NULL, 0, CFCopyLocalizedString(CFSTR("FE_ContextMenu_Description"), NULL), driver->description),
		kMenuItemAttrDisabled, NULL, &item);
	
	// add the filename
	p2cstrcpy (tempString, inRomset->spec.name);
	AppendMenuItemTextWithCFString (theMenu, CFStringCreateWithFormat(NULL, 0, CFCopyLocalizedString(CFSTR("FE_ContextMenu_Filename"), NULL), tempString),
		kMenuItemAttrDisabled, NULL, &item);
	
	// if it's a clone, list the parent
	if (IsClone(driver))
	{
		const game_driver *parent = driver->clone_of;
		AppendMenuItemTextWithCFString (theMenu, CFStringCreateWithFormat(NULL, 0, CFCopyLocalizedString(CFSTR("FE_ContextMenu_CloneOf"), NULL), parent->name, parent->description),
			kMenuItemAttrDisabled, NULL, &item);

		// if the parent romset exists, enable the menu item
		menuRomsets[item] = FindDriverInList(parent);
		if (menuRomsets[item])
			EnableMenuItem(theMenu, item);
	}
	
	// if it's a parent, list all of its clones
	{
		Boolean	once = true;
		int i;

		// check the clone_of field of every driver
		for (i = 0; drivers[i]; i++)
		{
			const game_driver *clone = drivers[i];
			
			if (clone->clone_of == driver)
			{
				// found a clone
				if (once)
				{
					AppendMenuItemTextWithCFString (theMenu, CFCopyLocalizedString(CFSTR("FE_ContextMenu_ClonesOfThisDriver"), NULL),
						kMenuItemAttrDisabled, NULL, &item);
					once = false;
				}
				AppendMenuItemTextWithCFString (theMenu, CFStringCreateWithFormat(NULL, 0, CFCopyLocalizedString(CFSTR("FE_ContextMenu_CloneList"), NULL), clone->name, clone->description),
					kMenuItemAttrDisabled, NULL, &item);

				// if the clone romset exists, enable the menu item
				menuRomsets[item] = FindDriverInList(clone);
				if (menuRomsets[item])
					EnableMenuItem(theMenu, item);
			}
		}			
	}

#ifdef MAME_DEBUG
	// add a divider
	AppendMenuItemTextWithCFString (theMenu, CFSTR(""), kMenuItemAttrSeparator, NULL, &item);

	// add the source filename
	AppendMenuItemTextWithCFString (theMenu, CFStringCreateWithFormat(NULL, 0, CFCopyLocalizedString(CFSTR("FE_ContextMenu_SourceFile"), NULL), driver->source_file),
		kMenuItemAttrDisabled, NULL, &item);
#endif
	
	// add a divider
	AppendMenuItemTextWithCFString (theMenu, CFSTR(""), kMenuItemAttrSeparator, NULL, &item);
	
	// add move to trash
	AppendMenuItemTextWithCFString (theMenu, CFCopyLocalizedString(CFSTR("FE_ContextMenu_MoveToTrash"), NULL), 0, NULL, &item);
	moveToTrashItem = item;
	
	// insert menu into application menulist
	InsertMenu(theMenu, -1); 

{
	short resourceRef = CurResFile();
	
	// display the contextual menu
	err = ContextualMenuSelect(theMenu,		// MenuHandle inMenuRef
			inEvent->where,					// Point inGlobalLocation
			false,							// Boolean inReserved
			kCMHelpItemRemoveHelp,			// UInt32 inHelpType
			NULL,							// ConstStr255Param inHelpItemString
			NULL,							// const AEDesc* inSelection
			&selectionType,					// UInt32* outUserSelectionType
			&selectionID,					// SInt16* outMenuID
			&selectionItem);				// UInt16* outMenuItem
	
	UseResFile(resourceRef);
}
	if (noErr == err)
	{
		if (kCMMenuItemSelected == selectionType)
		{
			if(selectionItem == moveToTrashItem)
			{
				// user selected move to trash
				err = MoveToTrash(&inRomset->spec);
				
				InvalidateROMSets();
				
				RebuildDirtyLists();
			}
			else if(selectionItem == revealInFinderItem)
			{
				// user selected "Reveal in Finder"
				err = SendRevealAE(&inRomset->spec);
			}
			else if (menuRomsets[selectionItem])
			{
				// user selected a parent or a clone romset
				SelectROMSet(menuRomsets[selectionItem]);
			}
		}
	}	

	// remove menu from the menulist and dispose it
	DeleteMenu(200);
	DisposeMenu(theMenu);
	
	return err;
}


//===============================================================================
//	LocateProcess
//
//	Returns the ProcessSerialNumber of the specified running process.
//===============================================================================

static OSErr LocateProcess(ProcessSerialNumber *outPSN, OSType inProcType, OSType inCreator)
{
	ProcessInfoRec	proc;
	OSErr			err;
	
	outPSN->highLongOfPSN = 0L;
	outPSN->lowLongOfPSN = kNoProcess;
	
	proc.processInfoLength = sizeof(ProcessInfoRec);
	proc.processName = NULL;
	proc.processAppSpec = NULL;
	proc.processLocation = NULL;
	
	do
	{
		err = GetNextProcess(outPSN);
		if (noErr == err)
		{
			if (noErr == GetProcessInformation(outPSN, &proc))
			{
				if (inProcType == proc.processType && inCreator == proc.processSignature)
					return noErr;
			}
		}
	}
	while (noErr == err);
	return err;
}


//===============================================================================
//	SendRevealAE
//
//	Creates and sends an AppleEvent to the Finder to reveal the 
//	specified file.
//===============================================================================

static OSErr SendRevealAE(const FSSpec *inSpec)
{
	ProcessSerialNumber	finderPSN;
	AppleEvent	aEvent, reply;
	AEDesc		addrDesc;
	OSErr		err;

	AE_NULLIFY(&aEvent);
	AE_NULLIFY(&reply);
	AE_NULLIFY(&addrDesc);
	
	// locate the running Finder
	err = LocateProcess(&finderPSN, 'FNDR', 'MACS');
	if (err) goto bail;
	
	// create an address descriptor for the Finder
	err = AECreateDesc(typeProcessSerialNumber, &finderPSN, sizeof(finderPSN), &addrDesc);
	if (err) goto bail;
	
	// create the event, addressed to the Finder
	err = AECreateAppleEvent(kAEMiscStandards, kAEMakeObjectsVisible, &addrDesc,
								kAutoGenerateReturnID, kAnyTransactionID, &aEvent);
	if (err) goto bail;

	// add the direct parameter
	err = AEPutParamPtr(&aEvent, keyDirectObject, typeFSS, inSpec, sizeof(FSSpec));
	if (err) goto bail;
	
	// send the event
	err = AESend(&aEvent,
					&reply,
					kAECanSwitchLayer+kAEAlwaysInteract+kAENoReply,
					kAENormalPriority,
					kAEDefaultTimeout,
					NULL,
					NULL);
	if (err) goto bail;
	
	// bring Finder to front at next opportunity
	err = SetFrontProcess(&finderPSN);

bail:
	AE_DISPOSE(&aEvent);
	AE_DISPOSE(&reply);
	AE_DISPOSE(&addrDesc);
	
	return err;
}

//===============================================================================
//	MoveToTrash
//
//	Moves the specified file to the trash.
//===============================================================================

static OSErr MoveToTrash(const FSSpec *inSpec)
{
	OSErr		theErr;
	short		trashVRefNum;
	long		trashDirID;
	FSSpec		trashSpec;
	
	if (!inSpec)
		return noErr;
	
	theErr = FindFolder (inSpec->vRefNum, kTrashFolderType, kCreateFolder, &trashVRefNum, &trashDirID);
	if(theErr)
	{
		return theErr;
	}
	
	theErr = FSMakeFSSpec (trashVRefNum, trashDirID, NULL, &trashSpec);
	if (theErr == noErr)
	{
		theErr = FSpCatMove (inSpec, &trashSpec);
	}

	return theErr;
}

#pragma mark -
#pragma mark ¥ÊMisc List Routines

//===============================================================================
//	GetIndexedNodeFromList
//
//	Looks up the selected list node.
//===============================================================================

static ListNode *GetIndexedNodeFromList(UInt32 inIndex)
{
	ListNode	*node = NULL;
	SInt16		size;
	Cell		cell;

	// bail if we can't do it
	if (!sMainList || inIndex >= GetListDataBoundsBottom(sMainList))
		return NULL;
	
	// get the cell's data from the list
	cell.h = 0;
	cell.v = inIndex;
	size = sizeof(ListNode *);
	LGetCell(&node, &size, cell, sMainList);
	
	// return the node we got
	return node;
}


//===============================================================================
//	GetSelectedNode
//
//	Returns a pointer to the currently selected node.
//===============================================================================

static ListNode *GetSelectedNode(void)
{
	return GetIndexedNodeFromList(GetSelectedCell(sMainList));
}


//===============================================================================
//	GetSelectedROMSet
//
//	Returns a pointer to the currently selected ROMSet.
//===============================================================================

static const ROMSetData *GetSelectedROMSet(void)
{
	ListNode *node = GetSelectedNode();
	return node ? node->romset : NULL;
}


//===============================================================================
//	SelectROMSet
//
//	Selects the specified ROMSet in the list.
//===============================================================================

static void SelectROMSet(const ROMSetData *inROMSet)
{
	UInt32 count = GetListDataBoundsBottom(sMainList);
	UInt32 index;
	
	// loop over nodes
	for (index = 0; index < count; index++)
	{
		ListNode *node = GetIndexedNodeFromList(index);
		
		// if we got a match, select the cell and get out of here
		if (node && node->romset && node->romset == inROMSet)
		{
			SelectCell(sMainList, index);
			return;
		}
	}
	
	// didn't find it; Go to Prefs file name get name of last driver

	for (index = 0; index < count; index++)
	{
		ListNode *node = GetIndexedNodeFromList(index);
		
		// if we got a match, select the cell and get out of here
		if ( EqualString( gFEPrefs.driverSelected, node->description, true, true ) )
		{
			SelectCell(sMainList, index);
			return;
		}
	}

	// didn't find it; Select first item in list
	SelectCell(sMainList, 0);
}


//===============================================================================
//	SelectNode
//
//	Selects the specified ListNode in the list.
//===============================================================================

static void SelectNode(const ListNode *inNode)
{
	UInt32 count = GetListDataBoundsBottom(sMainList);
	UInt32 index;
	
	// loop over nodes
	for (index = 0; index < count; index++)
	{
		ListNode *node = GetIndexedNodeFromList(index);
		
		// if we got a match, select the cell and get out of here
		if (node == inNode)
		{
			SelectCell(sMainList, index);
			return;
		}
	}
	
	// didn't find it; select the first item
	SelectCell(sMainList, 0);
}


//===============================================================================
//	FindDriverInList
//
//	Returns MULL or a pointer to the ROMSet in the list that connects to the
//	given driver.
//===============================================================================

static const ROMSetData *FindDriverInList(const game_driver *inDriver)
{
	UInt32 count = GetListDataBoundsBottom(sMainList);
	UInt32 index;
	
	// loop over nodes
	for (index = 0; index < count; index++)
	{
		ListNode *node = GetIndexedNodeFromList(index);
		
		// if we got a match, return true
		if (node && node->romset && node->romset->driver == inDriver)
			return node->romset;
	}
	
	// otherwise, bad news
	return false;
}


//===============================================================================
//	MatchCellText
//
//	Searches the list for a description that matches the characters in the input
//	string.
//===============================================================================

static SInt32 MatchCellText(const char *inStringToFind)
{
	UInt32		length = strlen(inStringToFind);
	SInt32		totalRows, row;
	
	// loop over all the cells in the list
	totalRows = GetListDataBoundsBottom(sMainList);
	for (row = 0; row < totalRows; row++)
	{
		ListNode *node = GetIndexedNodeFromList(row);
		
		// if we got one, do a compare
		if (node)
		{
			char name[256];
			char *s;

			// first copy and convert to lowercase
			p2cstrcpy(name, node->description);
			for (s = name; *s; s++)
				*s = tolower(*s);
			
			// return at the first match 
			if (strncmp(inStringToFind, name, length) == 0)
				return row;
		}
	}
	
	// if nothing was found, just return -1
	return -1;
}


#pragma mark -
#pragma mark ¥ Custom List Drawing

//===============================================================================
//	CustomListProc
//
//	This is the custom list definition procedure.
//===============================================================================

static pascal void CustomListProc(
	SInt16 			inMessage, 
	Boolean 		inSelected, 
	Rect *			inCellBounds, 
	Cell			inCell,
	SInt16			inDataOffset,
	SInt16			inDataSize,
	ListRef 		inList)
{
	#pragma unused (inDataOffset)
	
	SInt16 			oldFont, oldSize;
	Style 			oldFace;
	PenState		oldPenState;
	RGBColor		oldFore, oldBack;
	CGrafPtr		currentPort;
	ListNode *		node;
   	Pattern			whitePattern;
   	SInt32			oldPenMode;
   
	// we only handle draw and select messages
	if ((inMessage != lDrawMsg && inMessage != lHiliteMsg) || inDataSize <= 0 || inCellBounds == NULL)
		return;
   
	// get a pointer to the node
	LGetCell(&node, &inDataSize, inCell, inList);
	if (inDataSize != 4)
	{
		SysBeep(0);
		return;
	}
  		
    // save the current environment
    GetPenState (&oldPenState);
    GetForeColor (&oldFore);
    GetBackColor (&oldBack);
	GetPort ((GrafPtr *)&currentPort);
	oldFont = GetPortTextFont (currentPort);
	oldFace = GetPortTextFace (currentPort);
	oldSize = GetPortTextSize (currentPort);

	// now set our own environment	
   	BackPat (GetQDGlobalsWhite (&whitePattern));
	RGBForeColor (&rgbBlack);
    RGBBackColor (&rgbWhite);
	TextFont (sListFontID);
	TextSize (gFEPrefs.fontSize);
	TextFace (gFEPrefs.fontFace);
	
	// switch off the message
	switch (inMessage)
	{
		// draw: perform a full refresh of the cell
		// We do the same for a hilight because our anti-aliasing
		// scheme requires it.
		case lDrawMsg:
		case lHiliteMsg:

			// erase before we go any further
			EraseRect (inCellBounds);
			if (inCellBounds->bottom > sListBottomRedraw)
				sListBottomRedraw = inCellBounds->bottom;

			// add appropriate hilighting
			if (inSelected)
			{
				oldPenMode = GetPortPenMode (currentPort);
				SetPortPenMode (currentPort, hilitetransfermode);
				PaintRect (inCellBounds);
				SetPortPenMode (currentPort, oldPenMode);
			}
				
	  		// draw the cell's filename
	  		DrawTextForNode (node, inCellBounds, GetListActive(inList));

			// draw the icon
			DrawIconForNode (node, inCellBounds, inSelected, GetListActive(inList));
		  	break;

		default:
			break;
	}
    
    /* restore the old environment */
	TextFont (oldFont);
	TextFace (oldFace);
	TextSize (oldSize);
    SetPenState (&oldPenState);
    RGBForeColor (&oldFore);
    RGBBackColor (&oldBack);
}


//===============================================================================
//	DrawTextForNode
//
//	Draws the appropriate text in the list for a given node.
//===============================================================================

static void DrawTextForNode(ListNode *inNode, const Rect *inItemRect, Boolean inEnabled)
{
	Point			textLoc;
	Rect			textRect;
	Str255			description;
	short			width, maxWidth;
	CGrafPtr		currentPort;
	CFStringRef		stringRef;
	
	GetPort((GrafPtr *)&currentPort);
	
	// set the drawing mode
	TextMode(inEnabled ? srcOr : grayishTextOr);
	
	// compute where to start drawing
	textLoc.h = inItemRect->left + kCellXSlop + kIconWidth + kCellXSlop + inNode->nest * kIndentPixels;
	SetRect (&textRect, textLoc.h, inItemRect->top + kCellYSlop, inItemRect->right, inItemRect->bottom);

	// modify the text drawing based on the preferences
	if (inNode->romset != NULL && inNode->romset->driver != NULL)
	{
		const game_driver *gamedrv = inNode->romset->driver;
		
		// if the user wishes, toggle the italics state if it's a clone
		if (!gFEPrefs.fe_noItalics)
			if (IsClone(gamedrv))
				TextFace(GetPortTextFace(currentPort) ^ italic);

		// if the user wants colors, select them
		if (gFEPrefs.fe_listUseColors)
		{
			// if the game is not working, draw it in red text
			if (gFEPrefs.fe_listUseBrokenColors)
			{
				if (IsBroken(gamedrv))
					RGBForeColor(&sListBrokenGameColor);
			}

			// otherwise, use the Finder label color
			else
				RGBForeColor(&sFinderLabelColor[inNode->romset->color]);
		}
	}
	
	// adjust string as necessary to fit in available width
	PLstrcpy(description, inNode->description);
	
	// see if we're too long
	maxWidth = inItemRect->right - textLoc.h;
	width = StringWidth(description);
	if (width > maxWidth)
	{
		// if so, try it condensed and compare again
		TextFace(GetPortTextFace(currentPort) | condense);
		width = StringWidth (description);
		if (width > maxWidth)
		{
			Boolean 	truncEnd = true;
			int 		index;
			
			// if the description contains a parenthesis, truncate in the middle
			for (index = 1; index <= description[0]; index++)
				if (description[index] == '(')
					truncEnd = false;
			
			// do the truncation
			if (truncEnd)
				TruncString(maxWidth, description, smTruncEnd);
			else
				TruncString(maxWidth, description, smTruncMiddle);
		}
	}

	// now draw it	
	stringRef = CFStringCreateWithPascalString (NULL, description, CFStringGetSystemEncoding());
	DrawThemeTextBox (stringRef, kThemeCurrentPortFont, inEnabled ? kThemeStateActive : kThemeStateInactive, true, &textRect, teFlushDefault, NULL);
	if (stringRef) CFRelease (stringRef);

	// reset the colors	
	RGBForeColor(&rgbBlack);
}


//===============================================================================
//	DrawIconForNode
//
//	Draws the appropriate icon in the list for a given node.
//===============================================================================

static void DrawIconForNode(ListNode *inNode, const Rect *inItemRect, Boolean inSelected, Boolean inEnabled)
{
	IconTransformType 	transform = inEnabled ? kTransformNone : kTransformDisabled;
	UInt32 				iconIndex;
	
	// add in the finder label color
	if (inNode->romset)
		if (gFEPrefs.fe_listUseColors)
			if (!gFEPrefs.fe_listUseBrokenColors)
				transform |= inNode->romset->color << 8;
	
	// determine which base icon
	if (inNode->type == kNodeOpenFolder)
		iconIndex = kIconFolderOpen;
	else if (inNode->type == kNodeClosedFolder)
		iconIndex = kIconFolderClose;
	else if (inNode->romset && inNode->romset->format == kROMSetFormatFolder)
		iconIndex = kIconROMSetFolder;
	else if (inNode->romset && inNode->romset->format == kROMSetFormatGhost)
		iconIndex = kIconROMSetGhost;
	else if (inNode->romset && inNode->romset->format == kROMSetFormatZip)
		iconIndex = kIconROMSetZip;
	else
		iconIndex = kIconROMSetResource;
	
	// if we have that icon, draw it
	if (sListIcons[iconIndex])
	{
		Rect	bounds = *inItemRect;
		
		// adjust the left size of the icon bounds
		bounds.left = inItemRect->left + kCellXSlop + inNode->nest * kIndentPixels;
		bounds.right = bounds.left + kIconWidth;
		bounds.top = inItemRect->top + (inItemRect->bottom - inItemRect->top - kIconHeight) / 2;
		bounds.bottom = bounds.top + kIconHeight;

		// draw it
		PlotIconSuite(&bounds, kAlignCenterLeft, transform, sListIcons[iconIndex]);
		
		// if we have a disabled driver, draw a red X
		if (inNode->romset && inNode->romset->driver != NULL && IsBroken(inNode->romset->driver))
			PlotIconSuite(&bounds, kAlignNone, transform, sListIcons[kIconBad]);
	}
}
