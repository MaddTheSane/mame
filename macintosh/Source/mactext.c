/*##########################################################################

	mactext.c

	Routines for displaying text results in a dialog using MLTE.
	Implements a scrollable text control using a UserPane control.

##########################################################################*/

#include <Carbon/Carbon.h>

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "driver.h"

#include "mac.h"
#include "macstrings.h"
#include "macfiles.h"
#include "mactext.h"
#include "macutils.h"

#ifdef __MWERKS__
#pragma require_prototypes on
#endif

/*##########################################################################
	MACROS
##########################################################################*/

// controls whether the text wraps or uses a horizontal scrollbar
#define USE_H_SCROLL	1

/*##########################################################################
	CONSTANTS
##########################################################################*/

// results dialog resource ID
enum
{
	rResultsDLOG = 3300
};

// items in the results dialog
enum
{
	iOkButton = 1,
	iTextUserPane = 2,
	iVScrollbar = 3,
	iSaveAsButton = 4,
	iHScrollbar = 5
};

// temporary buffer size
enum
{
	kReportBufferSize 	= 65536,	// total buffer size
	kReportBufferMax	= 1024		// max chars we can printf at once
};


#define kTextInset	5			// margin around text in pixels
#define kMaxLineLength	1280	// maximum length of a line of text in pixels
#define kHScrollPixels	6		// number of pixels to scroll horizontally

#define kMinWindowHSize	300		// minimum horizontal size of dialog window
#define kMinWindowVSize	140		// minimum vertical size of dialog window


/*##########################################################################
	GLOBAL VARIABLES
##########################################################################*/
static EventRecord		sMouseEvent;	// copy of last mouseDown event
static DialogRef		sResultsDialog;	// pointer to results dialog

static char *			sReportBuffer;	// temporary buffer for caching report results
static UInt32			sReportBufferCount;

/*##########################################################################
	MLTE GLOBAL VARIABLES
##########################################################################*/
static TXNObject		sTextObj;		// reference to MLTE object

/*##########################################################################
	FUNCTION PROTOTYPES
##########################################################################*/

//extern void HandleMenuSelection(long inSelection);
static void FlushReportBuffer(void);


/*##########################################################################
	MLTE IMPLEMENTATION
##########################################################################*/
#pragma mark ¥ MLTE implementation


//===============================================================================
//	MLTECalculateTextArea
//
//	Calculate the size of the MLTE text area based on the size
//	of the specified UserPane control. Note that this area *includes* the
//	scrollbar(s).
//===============================================================================

static void MLTECalculateTextArea (ControlRef inUserPane, Rect *outRect)
{
	Rect		paneRect;
	
	// base size of text field on size of UserPane
	(void)GetControlBounds (inUserPane, &paneRect);
	InsetRect (&paneRect, 1, 1);
	*outRect = paneRect;
}


//===============================================================================
//	MLTEResizeTextResultsWindow
//
//	Resize the text results dialog window after a grow operation to the
//	specified width and height.
//===============================================================================

static void MLTEResizeTextResultsWindow (DialogRef inDialog, int inWidth, int inHeight)
{
	int				hmovers[] = { iOkButton, iSaveAsButton, 0 };
	int				vmovers[] = { iOkButton, iSaveAsButton, 0 };
	WindowRef		wp = GetDialogWindow (inDialog);
	int				delta_x, delta_y;
	ControlRef		control;
	Rect			r;
	int				*item;
	Rect			viewRect, portRect;
	GrafPtr			savePort;
	Rect			empty = { 0, 0, 0, 0 };
	RgnHandle		saveClip;
	CursHandle		watchHandle;
	OSErr			err;

	// make sure the dialog window is the current port
	GetPort (&savePort);
	SetPortDialogPort (inDialog);

	// hide all drawing until we're done
	saveClip = NewRgn ();
	if (saveClip)
	{
		GetClip (saveClip);
		ClipRect (&empty);
	}
	
	// determine how much we are growing/shrinking
	(void)GetWindowPortBounds (wp, &portRect);
	delta_x = inWidth - portRect.right;
	delta_y = inHeight - portRect.bottom;

	// resize the window
	SizeWindow (wp, inWidth, inHeight, true);
	
	// move the controls that need to move horizontally
	for (item = hmovers; *item; item++)
	{
		err = GetDialogItemAsControl (inDialog, *item, &control);
		
		if (noErr == err)
		{
			(void)GetControlBounds (control, &r);
			MoveControl (control, r.left + delta_x, r.top);
		}		
	}
	
	// move the controls that need to move vertically
	for (item = vmovers; *item; item++)
	{
		err = GetDialogItemAsControl (inDialog, *item, &control);
		
		if (noErr == err)
		{
			(void)GetControlBounds (control, &r);
			MoveControl (control, r.left, r.top + delta_y);
		}		
	}

	// get a handle to the user pane control
	err = GetDialogItemAsControl (inDialog, iTextUserPane, &control);
	
	if (noErr == err)
	{
		// resize the user pane control
		(void)GetControlBounds (control, &r);
		OffsetRect (&r, -r.left, -r.top);
		SizeControl (control, r.right + delta_x, r.bottom + delta_y);

		// set the watch cursor because TXNResizeFrame() can take some time
		watchHandle = GetCursor (watchCursor);
		if (watchHandle)
		{
			SetCursor (*watchHandle);
		}

		// resize the MLTE text based on the new user pane control size
		MLTECalculateTextArea (control, &viewRect);

#if USE_H_SCROLL
		// MLTE bug: MLTE apparently assumes the frame starts at (0,0) in the window
		TXNResizeFrame (sTextObj,
						viewRect.left + kMaxLineLength,		// width
						viewRect.bottom,					// height
						0);									// TXNFrameID
#endif
		TXNSetFrameBounds (sTextObj,
							viewRect.top,
							viewRect.left,
							viewRect.bottom, 
							viewRect.right,
							0);					// TXNFrameID
	}
	
	// restore original clipping region and force the entire window to be updated
	if (saveClip)
	{
		ThemeDrawingState themeState;
		(void)GetWindowPortBounds (wp, &portRect);
		GetThemeDrawingState (&themeState);
		SetThemeBackground (kThemeBrushMovableModalBackground, 32, true);
		ClipRect (&portRect);
		EraseRect (&portRect);
		InvalWindowRect (wp, &portRect);
		
		SetClip (saveClip);
		DisposeRgn (saveClip);
	}
	
	SetPort (savePort);
}


//===============================================================================
//	MLTEtextModalFilter
//
//	ModalDialog event filter for the dialog containing the text control.
//===============================================================================

static pascal Boolean MLTEtextModalFilter (DialogRef inDialog, EventRecord *inEvent, short *outItemHit)
{
	Boolean			handled = false;

	// it's not necessary to call IdleControls() here. Maybe StdFilterProc() calls it?
	//IdleControls (GetDialogWindow (inDialog));
	
	// adjust cursor appropriately (i-beam or arrow)
	{
		RgnHandle tempRgn = NewRgn();
		
		if (tempRgn)
		{
			TXNAdjustCursor (sTextObj, tempRgn);
			DisposeRgn (tempRgn);
		}
		else
		{
			InitCursor ();
		}
	}

	if (mouseDown == inEvent->what)
	{
		WindowRef	window;
		short 		clickpart = FindWindow (inEvent->where, &window);

		// save the mouseDown event (for MLTEtextUserPaneTrackingProc)
		sMouseEvent = *inEvent;
		
#if 0
		// handle clicks in menubar
		if (clickpart == inMenuBar)
		{
			HandleMenuSelection (MenuSelect (inEvent->where));
			
			// event was handled here
			*outItemHit = 0;
			handled = true;
		}
		else
#endif
		 if (clickpart == inGrow && window == GetDialogWindow(inDialog))
		{
			// handle growing the dialog
			Rect	limits, screenBounds;
			SInt32	result;
			
			// set appropriate grow limits
			screenBounds = (**GetMainDevice()).gdRect;
			limits.left = kMinWindowHSize;
			limits.top = kMinWindowVSize;
			limits.right = screenBounds.right - screenBounds.left - 20;	 // max x
			limits.bottom = screenBounds.bottom - screenBounds.top - 40; // max y

			// track the grow
			result = GrowWindow (window, inEvent->where, &limits);
			if (result)
			{
				// resize the dialog window
				MLTEResizeTextResultsWindow (inDialog, LoWord (result), HiWord (result));
			}

			// event was handled here
			*outItemHit = 0;
			handled = true;
		}
	}
	else if (keyDown == inEvent->what)
	{
		short	theChar = inEvent->message & charCodeMask;
		
		if (inEvent->modifiers & cmdKey)
		{
			// allow user to copy the text selection using standard copy shortcut
			if ('c' == theChar)
			{
				if (TXNCopy (sTextObj) != noErr)
				{
					SysBeep (1); // copy failed
				}
				else if (TXNConvertToPublicScrap() != noErr)
				{
					SysBeep (1);
				}

				// event was handled here
				*outItemHit = iTextUserPane;
				handled = true;
			}
		}
	}

	// let standard modal filter handle anything we didn't handle ourselves		
	if (!handled)
	{
		handled = StdFilterProc (inDialog, inEvent, outItemHit);
	}
	
	return handled;
}


#pragma mark -


//===============================================================================
//	MLTEdrawTextFrame
//
//	Draw the frame of our text appropriately for the current active/inactive state.
//	TODO: when Appearance 1.1 is available, use the theme API to determine the
//	correct color for the frame when it is inactive.
//===============================================================================

static void MLTEdrawTextFrame (ControlRef inControl, Boolean inActive)
{
	RGBColor	saveFore;
	RGBColor	saveBack;
	Rect		r;

	GetForeColor (&saveFore);
	GetBackColor (&saveBack);
	
	if (inActive)
	{
		// color of frame in the active state
		RGBForeColor (&rgbBlack);
	}
	else
	{
		// color of frame in the inactive state
		RGBColor	grayRGB = rgbBlack;
		
		(void)GetGray (GetGDevice (), &rgbWhite, &grayRGB);
		RGBForeColor (&grayRGB);
	}
	
	(void)GetControlBounds (inControl, &r);
	r.right -= 15; 	// avoid vertical scrollbar
#if USE_H_SCROLL
	r.bottom -= 15; // avoid horizontal scrollbar
#endif
	FrameRect (&r);
	
	// now erase the margins
	InsetRect (&r, 1, 1);
	RGBBackColor (&rgbWhite);
	EraseRect (&r);
	

	RGBForeColor (&saveFore);
	RGBBackColor (&saveBack);
}


//===============================================================================
//	MLTEtextUserPaneDrawProc
//
//	Custom drawProc for our text UserPane control.
//===============================================================================

static pascal void MLTEtextUserPaneDrawProc (ControlRef inControl, SInt16 inPart)
{
	#pragma unused (inPart)
	
	// draw frame around UserPane and erase margins to white
	MLTEdrawTextFrame (inControl, IsControlActive (inControl));

	// let MLTE draw the text and text background
	TXNDraw (sTextObj, /*GWorldPtr iDrawPort*/NULL);
}


//===============================================================================
//	MLTEtextUserPaneHitTestProc
//
//	Custom hitTestProc for our text UserPane control.
//	This allows FindControlUnderMouse() to locate our control, which allows
//	ModalDialog() to call TrackControl() or HandleControlClick() for our control.
//===============================================================================

static pascal ControlPartCode MLTEtextUserPaneHitTestProc (ControlRef inControl, Point inWhere)
{
	ControlPartCode	part;
	Rect			r;

	MLTECalculateTextArea (inControl, &r);
	
	// is the click in our text?
	if (PtInRect (inWhere, &r))
	{
		// return a valid part code so HandleControlClick() will be called
		part = kControlButtonPart;
	}
	else
	{
		// not in our text
		part = kControlNoPart;
	}
	
	return part;
}


//===============================================================================
//	MLTEtextUserPaneTrackingProc
//
//	Custom trackingProc for our text UserPane control.
//	This won't be called for our control unless the kControlRefsTracking feature
//	bit is specified when the userPane is created. You specify feature bits by
//	putting them in the "initial" field of the 'CNTL' resource.
//===============================================================================

static pascal ControlPartCode MLTEtextUserPaneTrackingProc (
					ControlRef inControl,
					Point inStartPt,
					ControlActionUPP inActionProc)
{
	#pragma unused (inControl, inStartPt, inActionProc)
	
	RGBColor saveFore, saveBack;
	
	// save foreground and background colors
	GetBackColor (&saveBack);
	GetForeColor (&saveFore);
	
	// force correct background and foreground colors to make scrolling pretty
	RGBForeColor (&rgbBlack);
	RGBBackColor (&rgbWhite);

	// let MLTE handle the clicks and drags
	TXNClick (sTextObj, &sMouseEvent);
	
	// restore the original colors
	RGBForeColor (&saveFore);
	RGBBackColor (&saveBack);

	return kControlNoPart;
}


//===============================================================================
//	MLTEtextUserPaneIdleProc
//
//	Custom idleProc for our text UserPane control.
//	This won't be called for our control unless the kControlWantsIdle feature
//	bit is specified when the userPane is created. You specify feature bits by
//	putting them in the "initial" field of the 'CNTL' resource.
//===============================================================================

static pascal void MLTEtextUserPaneIdleProc (ControlRef inControl)
{
	#pragma unused (inControl)

	// blink the caret for our text
	TXNIdle (sTextObj);
}


//===============================================================================
//	MLTEtextUserPaneKeyDownProc
//
//	Custom keyDownProc for our text UserPane control.
//	This won't be called for our control unless the kControlSupportsFocus feature
//	bit is specified when the userPane is created. You specify feature bits by
//	putting them in the "initial" field of the 'CNTL' resource.
//===============================================================================

static pascal ControlPartCode MLTEtextUserPaneKeyDownProc (
					ControlRef inControl,
					SInt16 inKeyCode,
					SInt16 inCharCode,
					SInt16 inModifiers)
{
	#pragma unused (inControl, inKeyCode, inCharCode, inModifiers)
	
//PROCEDURE TEKey (key: Char; hTE: TEHandle); 
	return kControlNoPart;
}


//===============================================================================
//	MLTEtextUserPaneActivateProc
//
//	Custom activateProc for our text UserPane control.
//	This won't be called for our control unless the kControlWantsActivate feature
//	bit is specified when the userPane is created. You specify feature bits by
//	putting them in the "initial" field of the 'CNTL' resource.
//===============================================================================

static pascal void MLTEtextUserPaneActivateProc (ControlRef inControl, Boolean inActivating)
{
	#pragma unused (inControl)

	// MLTE Bug: The last parameter to TXNActivate is of type TXNScrollBarState,
	// and is supposed to determine whether the scrollbars are always active
	// regardless of focus, or activate and deactivate when focus changes. But
	// in reality, if the parameter is kScrollBarsSyncWithFocus the scrollbars never 
	// activate, regardless of focus changes. Sigh.
	(void)TXNActivate (sTextObj,
						0, 											// TXNFrameID
						inActivating	? kScrollBarsAlwaysActive	// TXNScrollBarState
										: kScrollBarsSyncWithFocus);
	
	// Note: if you don't set the focus, you won't be able to make selections
	TXNFocus (sTextObj, /*Boolean iBecomingFocused*/ inActivating);
}


#pragma mark -
		

//===============================================================================
//	MLTECreateTextResults
//
//	Create a results dialog (invisibly) and an MLTE object to hold results.
//	If you call this routine and it returns noErr, you _must_ call either
//	DisplayTextResults() or DisposeTextResults() eventually.
//===============================================================================

static OSErr MLTECreateTextResults (ConstStr255Param inTitle)
{
	ControlRef		userPane;
	ControlRef		control;
	GrafPtr			savePort;
	Rect			r;
	OSErr			err;

	// try to create the results dialog (invisibly)	
	sResultsDialog = GetNewDialog (rResultsDLOG, NULL, kWindowToFront);
	if (!sResultsDialog)
	{
		return resNotFound;
	}
	
	// retitle the dialog window as requested
	SetWTitle (GetDialogWindow (sResultsDialog), inTitle);

	// temporarily switch to the dialog's GrafPort
	GetPort (&savePort);
	SetPortDialogPort (sResultsDialog);
	
	// hide the horizontal and vertical scrollbars (they're for WASTE only)
	if (noErr == GetDialogItemAsControl (sResultsDialog, iVScrollbar, &control))
		HideControl (control);
	if (noErr == GetDialogItemAsControl (sResultsDialog, iHScrollbar, &control))
		HideControl (control);
	
	// setup an MLTE object to fit the userPane control
	err = GetDialogItemAsControl (sResultsDialog, iTextUserPane, &userPane);
	if (err) goto bail;

	// resize the user pane control to occupy scrollbar areas
	(void)GetControlBounds (userPane, &r);
	OffsetRect (&r, -r.left, -r.top);
#if USE_H_SCROLL
	SizeControl (userPane, r.right + 15, r.bottom + 15);
#else
	SizeControl (userPane, r.right + 15, r.bottom);
#endif
	
	// initialize MLTE (would be better done at program launch, but hey...)
	{
		TXNInitOptions	initOptions;
		TXNMacOSPreferredFontDescription defaults;  // fontID, pointSize, encoding, and fontStyle

		defaults.fontID = NULL;
  		defaults.pointSize = kTXNDefaultFontSize;	// Note that this is a Fixed value
  		defaults.encoding = CreateTextEncoding(kTextEncodingMacRoman, kTextEncodingDefaultVariant, kTextEncodingDefaultFormat);
  		defaults.fontStyle 	= kTXNDefaultFontStyle;

		initOptions = 0L;
						
		err = TXNInitTextension (&defaults, 1, initOptions);

		if (kTXNAlreadyInitializedErr == err)
			err = noErr;
			
		if (err) goto bail;
	}
	
	// Create a new MLTE object for our text
	{
		Rect			viewRect;
		TXNFrameOptions	options;
		TXNFrameID		frameID;

		sTextObj = NULL;
	
		// MLTE bug: kTXNDrawGrowIconMask doesn't seem to work
		
		// MLTE bug: kTXNNoTSMEverMask causes TXNNewObject to return an error of
		//			 kTXNCannotTurnTSMOffWhenUsingUnicodeErr = -22013, even though
		//			 we specify kTXNMacEncoding, not Unicode
		
		// Note: Don't use kTXNReadOnlyMask because if you do then you can't make
		// any changes like changing the background color or font, etc. Set the
		// read-only option later, in MLTEDisplayTextResults, instead.
		
		options =	kTXNWantVScrollBarMask |
					// kTXNReadOnlyMask |
					kTXNNoKeyboardSyncMask |
					kTXNAlwaysWrapAtViewEdgeMask;
		#if USE_H_SCROLL
		options |=	kTXNWantHScrollBarMask;
//		options |=  kTXNDrawGrowIconMask;
		options &=	~kTXNAlwaysWrapAtViewEdgeMask;
		#endif

		// calculate frame viewRect based on size of UserPane control
		MLTECalculateTextArea (userPane, &viewRect);
		
		err = TXNNewObject 
					(NULL, 								// const FSSpec *iFileSpec
					GetDialogWindow (sResultsDialog), 	// WindowPtr iWindow
					&viewRect, 							// Rect *iFrame
					options, 							// TXNFrameOptions iFrameOptions
					kTXNTextEditStyleFrameType, 		// TXNFrameType iFrameType
					kTXNTextFile, 						// TXNFileType iFileType
					kTXNMacOSEncoding, 					// TXNPermanentTextEncodingType
					&sTextObj, 							// TXNObject *oTXNObject
					&frameID, 							// TXNFrameID *oTXNFrameID,
					0);									// TXNObjectRefcon iRefCon
						
		if (err) goto bail;
	}

	// set the text bacground to white		
	{
		TXNBackground	bg;
		
		bg.bgType = kTXNBackgroundTypeRGB;
		bg.bg.color = rgbWhite;
		
		(void)TXNSetBackground (sTextObj, &bg);
	}

	// set the text font to 10 point Monaco
	{
		TXNTypeAttributes	attrib[2];
		short				fontID;
		
   		GetFNum ("\pcourier", &fontID);			
		attrib[0].tag = kTXNQDFontFamilyIDAttribute;
		attrib[0].size = kTXNQDFontFamilyIDAttributeSize;
		attrib[0].data.dataValue = fontID;
		
		// MLTE bug: kTXNQDFontSizeAttributeSize is sizeof(SInt16), but in reality
		// the parameter is a Fixed, which is 32 bits.
		attrib[1].tag = kTXNQDFontSizeAttribute;
		attrib[1].size = kTXNQDFontSizeAttributeSize;
		attrib[1].data.dataValue = 12 << 16;
		
		(void)TXNSetTypeAttributes (sTextObj, 2, attrib, 0, 0);
	}

	// restore the previous GrafPort
	SetPort (savePort);
	
bail:
	if (err)
	{
		DisposeDialog (sResultsDialog);
		sResultsDialog = NULL;
		SetPort (savePort);
		return err;
	}
	return err;
}

static inline Boolean TestKeyPressed(UInt8 *inKeyState, UInt8 inKeycode)
{
	return (inKeyState[inKeycode >> 3] >> (inKeycode & 7)) & 1;
}


//===============================================================================
//	MLTESaveTextResults
//
//	Saves the MLTE text to a file specified by the user.
//===============================================================================

static OSErr MLTESaveTextResults (ConstStr255Param inDefaultFileName)
{
	OSErr err = noErr;
	CFStringRef defaultFileName = NULL;
	CFStringRef promptString = NULL;
	FSSpec fsSpec;

	if (!sTextObj) return paramErr;
	
	TXNFocus (sTextObj, false);

	// make a reasonable default name
	defaultFileName = CFStringCreateWithPascalString (NULL, inDefaultFileName, kCFStringEncodingMacRoman);
	if (!defaultFileName) return paramErr;
	
	promptString = CFCopyLocalizedString(CFSTR("PromptToSaveFile"), NULL);
	
	if (!_NavPutOneFile (promptString, defaultFileName, MAC_FILETYPE_REPORTS, NULL, &fsSpec))
	{
		err = userCanceledErr;
	}

	if (noErr == err)
	{
		short refnum, resRefnum;
		
		// delete the file and recreate it
		(void)FSpDelete (&fsSpec);
		FSpCreateResFile (&fsSpec, gPrefs.textCreator, 'TEXT', smSystemScript);
		
		// try to open the file for write access
		err = FSpOpenDF (&fsSpec, fsRdWrPerm, &refnum);
		
		// now open the resource fork also
		if (noErr == err)
			resRefnum = FSpOpenResFile (&fsSpec, fsRdWrPerm);
		
		if (noErr == err)
		{
			err = TXNSave (sTextObj,
							kTXNTextFile,
							kTXNSingleStylePerTextDocumentResType,
							kTXNMacOSEncoding,
							&fsSpec,
							refnum,
							resRefnum);
							
			// close the file
			(void)FSClose (refnum);
			CloseResFile (resRefnum);
		}
	}
	
	// tell user if we failed to create the file
	if (err && err != userCanceledErr)
	{
		char errmsg[256];
		
		sprintf (errmsg, "%s (%d)\n",
					GetIndCString (rErrorStrings, kCouldntCreateFileError,"Couldn't create file"), err);
		printf (errmsg);
	}

	TXNFocus (sTextObj, true);

bail:

	if (defaultFileName)
		CFRelease (defaultFileName);
	if (promptString)
		CFRelease (promptString);

	return err;
}


//===============================================================================
//	MLTEDisplayTextResults
//
//	Displays the dialog containing the MLTE text created by MLTECreateTextResults().
//===============================================================================

static void MLTEDisplayTextResults (ConstStr255Param inDefaultFileName)
{
	ControlUserPaneDrawUPP		userPaneDrawUPP = NULL;
	ControlUserPaneHitTestUPP	userPaneHitTestUPP = NULL;
	ControlUserPaneTrackingUPP	userPaneTrackingUPP = NULL;
	ControlUserPaneIdleUPP		userPaneIdleUPP = NULL;
	ControlUserPaneKeyDownUPP	userPaneKeyDownUPP = NULL;
	ControlUserPaneActivateUPP	userPaneActivateUPP = NULL;
	ModalFilterUPP				modalFilter = NULL;

	ControlRef		userPane;
	GrafPtr			savePort;
	short			itemHit;
	Boolean			done = false;
	OSErr			err;

	// fatal error if we don't have a results dialog and MLTE object
	if (NULL == sResultsDialog || NULL == sTextObj)
	{
		SysBeep (1);
		return;
	}
	
	// bring dialog window to the front again
	SelectWindow (GetDialogWindow (sResultsDialog));
	
	// make it the current GrafPort
	GetPort (&savePort);
	SetPortDialogPort (sResultsDialog);
	
	// set default button
	SetDialogDefaultItem (sResultsDialog, iOkButton);

	// setup the custom UserPane control
	err = GetDialogItemAsControl (sResultsDialog, iTextUserPane, &userPane);
	if (err) goto bail;
	
	// install all of the custom procedures for our UserPane control
	userPaneDrawUPP = NewControlUserPaneDrawUPP (MLTEtextUserPaneDrawProc);
	err = SetControlData (userPane, kControlEntireControl, kControlUserPaneDrawProcTag,
							sizeof (ControlUserPaneDrawUPP), (Ptr)&userPaneDrawUPP);

	userPaneHitTestUPP = NewControlUserPaneHitTestUPP (MLTEtextUserPaneHitTestProc);
	err = SetControlData (	userPane,
							kControlEntireControl, 
							kControlUserPaneHitTestProcTag,
							sizeof (ControlUserPaneHitTestUPP),
							(Ptr)&userPaneHitTestUPP);
	
	userPaneTrackingUPP = NewControlUserPaneTrackingUPP (MLTEtextUserPaneTrackingProc);
	err = SetControlData (	userPane,
							kControlEntireControl, 
							kControlUserPaneTrackingProcTag,
							sizeof (ControlUserPaneTrackingUPP),
							(Ptr)&userPaneTrackingUPP);
	
	userPaneIdleUPP = NewControlUserPaneIdleUPP (MLTEtextUserPaneIdleProc);
	err = SetControlData (userPane, kControlEntireControl, kControlUserPaneIdleProcTag,
							sizeof (ControlUserPaneIdleUPP), (Ptr)&userPaneIdleUPP);

	userPaneKeyDownUPP = NewControlUserPaneKeyDownUPP (MLTEtextUserPaneKeyDownProc);
	err = SetControlData (	userPane,
							kControlEntireControl, 
							kControlUserPaneKeyDownProcTag,
							sizeof (ControlUserPaneKeyDownUPP),
							(Ptr)&userPaneKeyDownUPP);
	
	userPaneActivateUPP = NewControlUserPaneActivateUPP (MLTEtextUserPaneActivateProc);
	err = SetControlData (	userPane,
							kControlEntireControl, 
							kControlUserPaneActivateProcTag,
							sizeof (ControlUserPaneActivateUPP),
							(Ptr)&userPaneActivateUPP);

	// apply some attributes to our TXNObject
	{
		TXNControlTag	tag[3];
		TXNControlData	data[3];
		TXNMargins		margins = { kTextInset, kTextInset, kTextInset, 0 };
		
		// set the text to read-only
		tag[0] = kTXNIOPrivilegesTag;
		data[0].uValue = kTXNReadOnly;
		
		// set the margins to kTextInset pixels
		tag[1] = kTXNMarginsTag;
		data[1].marginsPtr = &margins;
		
		// set the word-wrap on or off
		tag[2] = kTXNWordWrapStateTag;
		#if USE_H_SCROLL
			data[2].uValue = kTXNNoAutoWrap;
		#else
			data[2].uValue = kTXNAutoWrap;
		#endif
		
		// apply the above attributes
		(void)TXNSetTXNObjectControls (sTextObj, false, 3, tag, data);
	}

#if USE_H_SCROLL
	// set MLTE viewrect appropriately since we aren't wrapping long lines
	{
		Rect	viewRect;
		
		MLTECalculateTextArea (userPane, &viewRect);
		
		// How these functions work is poorly documented, but this combination forces
		// MLTE to do the right thing with long lines and no wrapping
		TXNResizeFrame (sTextObj,
						viewRect.left + kMaxLineLength,	// width
						viewRect.bottom,				// height
						0);								// TXNFrameID

		TXNSetFrameBounds (sTextObj,
							viewRect.top,
							viewRect.left,
							viewRect.bottom,
							viewRect.right,
							0);					// TXNFrameID
	}
#endif
	
	// scroll the text back to the top
	if (noErr == TXNSetSelection (sTextObj, 0, 0))
		TXNShowSelection (sTextObj, false);
	

	// now make the results dialog visible
	ShowWindow (GetDialogWindow (sResultsDialog));

	// handle events in the dialog until it's dismissed
	modalFilter = NewModalFilterUPP (MLTEtextModalFilter);
	
	while (!done)
	{
		ModalDialog (modalFilter, &itemHit);
		
		switch (itemHit)
		{
			case iOkButton:
				done = true;
				// make sure we end up with an arrow cursor (not i-beam)
				InitCursor ();
				break;
			case iTextUserPane:
				break;
			case iSaveAsButton:
				// save the MLTE text to a text file
				(void)MLTESaveTextResults (inDefaultFileName);
				break;
		}
	}
	
bail:
	// clean up
	if (modalFilter) DisposeModalFilterUPP (modalFilter);

	if (userPaneDrawUPP) DisposeControlUserPaneDrawUPP (userPaneDrawUPP);
	if (userPaneTrackingUPP) DisposeControlUserPaneTrackingUPP (userPaneTrackingUPP);
	if (userPaneIdleUPP) DisposeControlUserPaneIdleUPP (userPaneIdleUPP);
	if (userPaneKeyDownUPP) DisposeControlUserPaneKeyDownUPP (userPaneKeyDownUPP);
	if (userPaneActivateUPP) DisposeControlUserPaneActivateUPP (userPaneActivateUPP);

	if (sTextObj)
	{
		TXNDeleteObject (sTextObj);
		sTextObj = NULL;
	}
	
	SetPort (savePort);
	DisposeDialog (sResultsDialog);
	sResultsDialog = NULL;
}


//===============================================================================
//	MLTEDisposeTextResults
//
//	Call this if you called MLTECreateTextResults() but don't want to call
//	MLTEDisplayTextResults().
//===============================================================================

static void MLTEDisposeTextResults (void)
{
	if (sTextObj)
	{
		TXNDeleteObject (sTextObj);
		sTextObj = NULL;
	}
	if (sResultsDialog)
	{
		DisposeDialog (sResultsDialog);
		sResultsDialog = NULL;
	}
}


/*##########################################################################
	COMMON IMPLEMENTATION
##########################################################################*/
#pragma mark -
#pragma mark ¥ Common implementation


//===============================================================================
//	rPrintf
//
//	Print formatted text (maximum 512 characters) directly into sTextWE or sTextObj.
//===============================================================================

OSErr rPrintf (const char *s, ...)
{
	va_list		ap;
	
	// flush if we might go over the limit
	if (sReportBufferCount + kReportBufferMax >= kReportBufferSize)
		FlushReportBuffer();

	// just add to the buffer
	if (sReportBuffer != NULL)
	{
		va_start(ap, s);
		sReportBufferCount += vsprintf(&sReportBuffer[sReportBufferCount], s, ap);
		va_end(ap);
	}
	
	return noErr;
}


//===============================================================================
//	CreateTextResults
//
//	Create a results dialog (invisibly) and a text object to hold results.
//	If you call this routine and it returns noErr, you _must_ call either
//	DisplayTextResults() or DisposeTextResults() eventually.
//===============================================================================
OSErr CreateTextResults (ConstStr255Param inTitle)
{
	// allocate a temporary report buffer
	sReportBuffer = malloc(kReportBufferSize);
	if (sReportBuffer == NULL)
		return MemError();
	sReportBufferCount = 0;

	return MLTECreateTextResults (inTitle);
}


//===============================================================================
//	DisplayTextResults
//
//	Displays the dialog containing the text object created by CreateTextResults().
//===============================================================================

void DisplayTextResults (ConstStr255Param inDefaultFileName)
{
	// flush any leftover data
	FlushReportBuffer();

	// free the report buffer
	if (sReportBuffer != NULL)
		free(sReportBuffer);
	sReportBuffer = NULL;

	MLTEDisplayTextResults (inDefaultFileName);
}


//===============================================================================
//	DisposeTextResults
//
//	Call this if you called CreateTextResults() but don't want to call
//	DisplayTextResults().
//===============================================================================

void DisposeTextResults (void)
{
	// free the report buffer
	if (sReportBuffer != NULL)
		free(sReportBuffer);
	sReportBuffer = NULL;

	MLTEDisposeTextResults ();
}


//===============================================================================
//	FlushReportBuffer
//
//	Flushes text in the report buffer to the appropriate object.
//===============================================================================

static void FlushReportBuffer(void)
{
	UInt32		index;
	
	// skip if no buffer
	if (sReportBuffer == NULL)
	{
		sReportBufferCount = 0;
		return;
	}
	
	// fix newlines
	for (index = 0; index < sReportBufferCount; index++)
		if (sReportBuffer[index] == '\n')
			sReportBuffer[index] = '\r';

	if (sTextObj)
	{
		TXNOffset startOffset, endOffset;
		
		TXNGetSelection (sTextObj, &startOffset, &endOffset);
		TXNSetData (sTextObj, kTXNTextData, sReportBuffer, sReportBufferCount,
			startOffset, endOffset);
	}
	
	sReportBufferCount = 0;
}
