/*##########################################################################

	macutils.c

	Miscellaneous utility routines for handling strings, dialogs, etc.

##########################################################################*/

#include <Carbon/Carbon.h>

#include <ctype.h>
#include <string.h>
#include <math.h>

#include "driver.h"

#include "mac.h"
#include "macutils.h"


#ifdef __MWERKS__
#pragma require_prototypes on
#endif

/*##########################################################################
	CONSTANTS
##########################################################################*/

const RGBColor rgbBlack =	{ 0x0000, 0x0000, 0x0000 };
const RGBColor rgbWhite =	{ 0xffff, 0xffff, 0xffff };
const RGBColor rgbGrey50 =	{ 0x7fff, 0x7fff, 0x7fff };
const RGBColor rgbRed =		{ 0xffff, 0x0000, 0x0000 };
const RGBColor rgbYellow =  { 0xffff, 0x7fff, 0x0000 }; // a light orange, actually ;-)


/*##########################################################################
	IMPLEMENTATION
##########################################################################*/

#pragma mark ¥ÊMisc Dialog Routines

//===============================================================================
//	SetDialogControlActive
//
//	Activates or deactivates a dialog item, assuming it has an associated control.
//===============================================================================

void SetDialogControlActive(DialogRef inDialog, DialogItemIndex inItem, Boolean inActive)
{
	ControlRef		controlRef;

	// activate/deactivate the control as appropriate
	if (GetDialogItemAsControl(inDialog, inItem, &controlRef) == noErr)
	{
		if (inActive && !IsControlActive(controlRef))
			ActivateControl(controlRef);
		else if (!inActive && IsControlActive(controlRef))
			DeactivateControl(controlRef);
	}
}


//===============================================================================
//	IsDialogControlActive
//
//	Returns the active state of the control attached to a dialog item
//===============================================================================

Boolean IsDialogControlActive(DialogRef inDialog, DialogItemIndex inItem)
{
	ControlRef		controlRef;

	// activate/deactivate the control as appropriate
	if (GetDialogItemAsControl(inDialog, inItem, &controlRef) == noErr)
		return IsControlActive(controlRef);
	else
		return false;
}


//===============================================================================
//	SetDialogControlVisible
//
//	Activates or deactivates a dialog item, assuming it has an associated control.
//===============================================================================

void SetDialogControlVisible(DialogRef inDialog, DialogItemIndex inItem, Boolean inVisible)
{
	ControlRef		controlRef;

	// activate/deactivate the control as appropriate
	if (GetDialogItemAsControl(inDialog, inItem, &controlRef) == noErr)
	{
		if (inVisible && !IsControlVisible(controlRef))
			ShowControl(controlRef);
		else if (!inVisible && IsControlVisible(controlRef))
			HideControl(controlRef);
	}
}


//===============================================================================
//	IsDialogControlVisible
//
//	Returns the visible state of the control attached to a dialog item
//===============================================================================

Boolean IsDialogControlVisible(DialogRef inDialog, DialogItemIndex inItem)
{
	ControlRef		controlRef;

	// activate/deactivate the control as appropriate
	if (GetDialogItemAsControl(inDialog, inItem, &controlRef) == noErr)
		return IsControlVisible(controlRef);
	else
		return false;
}


//===============================================================================
//	SetDialogControlValue
//
//	Sets the value of the control attached to a dialog item
//===============================================================================

void SetDialogControlValue(DialogRef inDialog, DialogItemIndex inItem, SInt32 inValue)
{
	ControlRef		controlRef;

	// fetch the control handle and set the value
	if (GetDialogItemAsControl(inDialog, inItem, &controlRef) == noErr)
		if (GetControl32BitValue(controlRef) != inValue)
			SetControl32BitValue(controlRef, inValue);
}


//===============================================================================
//	GetDialogControlValue
//
//	Returns the value of the control attached to a dialog item
//===============================================================================

SInt32 GetDialogControlValue(DialogRef inDialog, DialogItemIndex inItem)
{
	ControlRef		controlRef;

	// fetch the control handle and return the value
	if (GetDialogItemAsControl(inDialog, inItem, &controlRef) == noErr)
		return GetControl32BitValue(controlRef);
	else
		return 0;
}


//===============================================================================
//	SetDialogItemTextAppearance
//
//	Sets the text of a static/edit text item assuming the Appearance Manager.
//===============================================================================

void SetDialogItemTextAppearance(DialogRef inDialog, DialogItemIndex inItem, ConstStr255Param inText)
{
	ControlRef		controlRef;

	// fetch the control handle and set the value
	if (GetDialogItemAsControl(inDialog, inItem, &controlRef) == noErr)
		SetDialogItemText((Handle)controlRef, inText);
}


//===============================================================================
//	GetDialogItemTextAppearance
//
//	Returns the text of a static/edit text item assuming the Appearance Manager.
//===============================================================================

void GetDialogItemTextAppearance(DialogRef inDialog, DialogItemIndex inItem, StringPtr outText)
{
	ControlRef		controlRef;

	// fetch the control handle and set the value
	if (GetDialogItemAsControl(inDialog, inItem, &controlRef) == noErr)
		GetDialogItemText((Handle)controlRef, outText);
}


//===============================================================================
//	GetDialogItemBounds
//
//	Returns the bounds of a dialog item.
//===============================================================================

void GetDialogItemBounds(DialogRef inDialog, DialogItemIndex inItem, Rect *outBounds)
{
	DialogItemType	itemType;
	Handle			itemHandle;
	
	GetDialogItem(inDialog, inItem, &itemType, &itemHandle, outBounds);
}


//===============================================================================
//	GetDialogItemType
//
//	Returns the type of a dialog item.
//===============================================================================

DialogItemType GetDialogItemType(DialogRef inDialog, DialogItemIndex inItem)
{
	DialogItemType	itemType;
	Handle			itemHandle;
	Rect			itemBounds;
	
	GetDialogItem(inDialog, inItem, &itemType, &itemHandle, &itemBounds);
	return itemType;
}


//===============================================================================
//	FakeButtonClick
//
//	Fakes a button click in a button.
//===============================================================================

void FakeButtonClick(DialogRef inDialog, DialogItemIndex inItem)
{
	ControlRef		controlRef;

	// fetch the control handle and set the value
	if (GetDialogItemAsControl(inDialog, inItem, &controlRef) == noErr)
	{
		UInt32			ticks;
		
		HiliteControl(controlRef, 1);
		Delay(8, &ticks);
		HiliteControl(controlRef, 0);
	}
}


//===============================================================================
//	GetDialogItemMenu
//
//	Returns a handle to the popup menu owned by a popup menu control.
//===============================================================================

MenuRef GetDialogItemMenu(DialogRef inDialog, short inItem)
{
	ControlHandle	menuControl;
	
	// get the ControlRef of the plugin popup menu
	if (GetDialogItemAsControl(inDialog, inItem, &menuControl) == noErr)
	{
		MenuRef			menu;
		Size			size;

		// get the MenuHandle of the plugin popup menu
		if (GetControlData(menuControl, kControlMenuPart, kControlPopupButtonMenuHandleTag, sizeof(MenuRef), (Ptr)&menu, &size) == noErr)
			return menu;
	}
	return NULL;
}

//===============================================================================
//	SetButtonValue
//
//	Appearance helper to set the value of a dialog control item.
//===============================================================================

void SetButtonValue(DialogRef inDialog, short inItem, short value)
{
	ControlHandle	control;
	OSErr			err;
	
	err = GetDialogItemAsControl(inDialog, inItem, &control);
	if (noErr == err)
	{
		SetControlValue(control, value);
	}
}


//===============================================================================
//	GetButtonValue
//
//	Appearance helper to retrieve the value of a dialog control item.
//===============================================================================

short GetButtonValue(DialogRef inDialog, short inItem)
{
	ControlHandle	control;
	short			value = 0;
	OSErr			err;
	
	err = GetDialogItemAsControl(inDialog, inItem, &control);
	if (noErr == err)
	{
		value = GetControlValue(control);
	}
	
	return value;
}

//===============================================================================
//	IsKeyPressed
//
//	Returns true if the key specified by inKeycode is currently pressed.
//===============================================================================

Boolean IsKeyPressed(UInt8 inKeycode)
{
	static UInt8 keyStates[16];
	
	GetKeys((SInt32 *)keyStates);
	return (keyStates[inKeycode >> 3] >> (inKeycode & 7)) & 1;
}


#pragma mark -
#pragma mark ¥ÊMisc List Manager Routines

//===============================================================================
//	GetSelectedCell
//
//	Returns the index of the selected cell (or -1 if no selection)
//===============================================================================

SInt32 GetSelectedCell(ListRef inList)
{
	Cell 		cell = { 0, 0 };

	// if no list, return an error
	if (!inList)
		return -1;

	// return the selection if we got one
	if (LGetSelect(true, &cell, inList))
		return cell.v;

	// otherwise, -1
	return -1;
}


//===============================================================================
//	GetListDataBoundsBottom
//
//	Returns the dataBounds.bottom for the list
//===============================================================================

SInt32 GetListDataBoundsBottom(ListRef inList)
{
	ListBounds dataBounds;
	
	GetListDataBounds(inList, &dataBounds);
	return dataBounds.bottom;
}


//===============================================================================
//	SelectCell
//
//	Selects the indexed cell.
//===============================================================================

void SelectCell(ListRef inList, SInt32 inRow)
{
	Cell	cell;

	// skip if no list
	if (!inList)
		return;
	
	// skip if out of range
	if (inRow < 0 || inRow >= GetListDataBoundsBottom(inList))
		return;

	// first deselect any existing cell
	cell.h = 0;
	cell.v = GetSelectedCell(inList);
	if (cell.v != -1)
		LSetSelect(false, cell, inList);
	
	// now select the new cell
	cell.h = 0;
	cell.v = inRow;
	LSetSelect(true, cell, inList);
	
	// autoscroll to make it visible
	LAutoScroll(inList);
}


//===============================================================================
//	SelectCell
//
//	Returns true if the list contains no cells.
//===============================================================================

Boolean ListEmpty(ListRef inList)
{
	return !(inList && GetListDataBoundsBottom(inList));
}


//===============================================================================
//	InvalidateList
//
//	Force a redraw of the list (including scrollbar areas).
//===============================================================================

void InvalidateList(ListRef inList)
{
	Rect		bounds;

	// skip if no list
	if (!inList)
		return;

	// get the view bounds
	GetListViewBounds(inList, &bounds);
	
	// add the vertical scroll bar rea
	if (GetListVerticalScrollBar(inList))
		bounds.right += kScrollbarWidth;

	// add the horizontal scroll bar area
	if (GetListHorizontalScrollBar(inList))
		bounds.bottom += kScrollbarWidth;
		
	// outset the rect to include the focus ring and borders
	InsetRect(&bounds, -4, -4);

	// invalidate the region on the window
	InvalWindowRect(GetWindowFromPort(GetListPort(inList)), &bounds);
}


#pragma mark -
#pragma mark ¥ÊMisc String Routines


//===============================================================================
//	GetIndCString
//
//	Gets a string from an STR# index as a C string.
//===============================================================================

char *GetIndCString(short inResource, short inIndex, const char *inDefault)
{
	static Str255 s;
	
	// fetch the string
	GetIndString(s, inResource, inIndex);

	// use the default if we got nothing
	if (inDefault && !s[0])
		strcpy((char *)s, inDefault);
	
	// else convert it to a C string
	else
		p2cstrcpy((char *)s, s);

	return (char *)s;
}


//===============================================================================
//	GetIndPString
//
//	Gets a string from an STR# index as a Pascal string.
//===============================================================================

StringPtr GetIndPString(short inResource, short inIndex, const unsigned char *inDefault)
{
	static Str255 s;
	
	// fetch the string
	GetIndString(s, inResource, inIndex);

	// use the default if we got nothing
	if (inDefault && !s[0])
		PLstrcpy(s, inDefault);

	return s;
}

#pragma mark -
#pragma mark ¥ Misc OS Utility Routines


//===============================================================================
//	TestGestaltBit
//
//	Queries Gestalt and tests a specific bit.
//===============================================================================

Boolean TestGestaltBit(OSType inSelector, short inBitToCheck)
{	
	Boolean	result = false;
	SInt32 returnValue;
	OSErr err = Gestalt(inSelector, &returnValue);
		
	// if it succeeded, test the bit
	if (err == noErr && BitTst(&returnValue, 31 - inBitToCheck))
		result = true;

	return result;
}

#pragma mark -
#pragma mark ¥ÊOSX helpers

#if MAC_XCODE
// This exists in libmx, but that requires 10.3+.
float sqrtf (float val)
{
	return sqrt (val);
}
#endif

#if __MSL__

int strnicmp(const char *s1, const char *s2, size_t n)
{
    int i;
    char c1, c2;
    for (i=0; i<n; i++)
    {
        c1 = tolower(*s1++);
        c2 = tolower(*s2++);
        if (c1 < c2) return -1;
        if (c1 > c2) return 1;
        if (c1 == 0) return 0;
    }
    return 0;
}

#endif