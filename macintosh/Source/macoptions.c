/*##########################################################################

	macoptions.c

	This file contains code to handle the front-end options dialog.

##########################################################################*/

#include <Carbon/Carbon.h>

#include <string.h>

#include "driver.h"

#include "mac.h"
#include "macoptions.h"
#include "MacPrefs.h"
#include "macutils.h"
#include "macvideo.h"


#ifdef __MWERKS__
#pragma require_prototypes on
#endif

// Dialog item constants
enum
{
	dGroupList = 3,
	dCheckNames,
	dCheckItalicClones,
	dCheckTheEnd,
	dGroupCheckColors,
	dRadioFinder,
	dRadioMAME,
	dFontMenu,
	dSizeMenu,
	dGroupPlugin,
	dRendererLabel,
	dPluginMenu,
	dPluginAbout,
	dPluginConfigure,
	dGroupMisc,
	dTextCreator,
	dCheckNoParentShots,
	dGroupStyle,
	dCheckBold,
	dCheckItalic,
	dCheckUnderline,
	dCheckOutline,
	dCheckShadow,
	dCheckCondensed,
	dTextCreatorLabel
};


// Prototypes
static void SetOptionsDialog(DialogRef inDialog);
static void GetOptionsDialog(DialogRef inDialog);
static void InitPluginMenu(DialogRef inDialog);
static void InitFontMenu(DialogRef inDialog);
static void GetPopupMenuText(DialogRef inDialog, int inItemNumber, int inMenuItem, StringPtr outString);


//
// OptionsDialog
//
// Present the user with some options specific to the Mac
//

void OptionsDialog (void)
{
	DialogRef theDialog;
	Boolean done;
	short itemHit;
	GrafPtr savePort;
		
	/* Setup the dialog */
	theDialog = GetNewDialog (rOptionsDialog, NULL, kWindowToFront);
	if (!theDialog) return;
	GetPort (&savePort);
	SetPortDialogPort (theDialog);

	/* load the fields with our prefs */
	SetOptionsDialog (theDialog);
	
	/* display and handle the dialog */
	SetDialogDefaultItem (theDialog, ok);
	SetDialogCancelItem (theDialog, cancel);
	{
		Cursor arrow;
		SetCursor(GetQDGlobalsArrow(&arrow));
	}
	ShowWindow (GetDialogWindow(theDialog));
	
	/* select the single edit text item */
	SelectDialogItemText (theDialog, dTextCreator, 0, 32767);

	done = false;
	while (!done)
	{
		ModalDialog (NULL, &itemHit);
		switch (itemHit)
		{
			case ok:
			case cancel:
				done = true;
				break;
			case dRadioFinder:
			case dRadioMAME:
				SetDialogControlValue (theDialog, dRadioFinder, 0);
				SetDialogControlValue (theDialog, dRadioMAME, 0);
				SetDialogControlValue (theDialog, itemHit, 1);
				break;
			case dPluginMenu:
			{
				UInt32 index = GetDialogControlValue (theDialog, dPluginMenu) - 1;
				UInt32 identifier = GetIndexedPlugin(index, NULL, NULL, NULL);
				
				SetDialogControlActive(theDialog, dPluginConfigure, PluginHasConfigDialog (identifier));
				SetDialogControlActive(theDialog, dPluginAbout, PluginHasAboutBox (identifier));
				break;
			}
			case dPluginConfigure:
			{
				UInt32	index = GetDialogControlValue (theDialog, dPluginMenu) - 1;
				UInt32 identifier = GetIndexedPlugin(index, NULL, NULL, NULL);

				ConfigurePlugin (identifier);
				break;
			}
			case dPluginAbout:
			{
				UInt32	index = GetDialogControlValue (theDialog, dPluginMenu) - 1;
				UInt32 identifier = GetIndexedPlugin(index, NULL, NULL, NULL);

				DisplayPluginAboutBox (identifier);
				break;
			}
			case dCheckNames:
			case dCheckItalicClones:
			case dGroupCheckColors:
			case dCheckNoParentShots:
			case dCheckTheEnd:
			case dCheckBold:
			case dCheckItalic:
			case dCheckUnderline:
			case dCheckOutline:
			case dCheckShadow:
			case dCheckCondensed:
				SetDialogControlValue (theDialog, itemHit, 1-GetDialogControlValue (theDialog, itemHit));
				if (itemHit == dGroupCheckColors)
				{
					Boolean		active = (GetDialogControlValue (theDialog, itemHit) != 0);
					SetDialogControlActive(theDialog, dRadioFinder, active);
					SetDialogControlActive(theDialog, dRadioMAME, active);
				}
				break;
		}
	}	
	
	/* if the user hit OK, change our prefs */
	if (itemHit == ok)
		GetOptionsDialog (theDialog);
		
	/* dispose of the dialog */
	SetPort (savePort);
	DisposeDialog (theDialog);
}


//===============================================================================
//	SetOptionsDialog
//
//	Set the controls in the dialog to the current state of the preferences.
//===============================================================================

static void SetOptionsDialog(DialogRef inDialog)
{
	static Str15	creatorString;
	UInt32			style = gFEPrefs.fontFace;
	
	// set miscellaneous checkboxes
	SetDialogControlValue(inDialog, dCheckNoParentShots, gFEPrefs.fe_listNoParentShots);

	// set list-related checkboxes
	SetDialogControlValue(inDialog, dCheckNames, !gFEPrefs.fe_displayFileNames);
	SetDialogControlValue(inDialog, dCheckItalicClones, !gFEPrefs.fe_noItalics);
	SetDialogControlValue(inDialog, dCheckTheEnd, gFEPrefs.fe_listSortThe);

	// set "Color-coded" groupbox checkbox and embedded radio buttons
	SetDialogControlValue(inDialog, dGroupCheckColors, gFEPrefs.fe_listUseColors);
	SetDialogControlValue(inDialog, dRadioFinder, !gFEPrefs.fe_listUseBrokenColors);
	SetDialogControlValue(inDialog, dRadioMAME, gFEPrefs.fe_listUseBrokenColors);
	SetDialogControlActive(inDialog, dRadioFinder, GetDialogControlValue(inDialog, dGroupCheckColors));
	SetDialogControlActive(inDialog, dRadioMAME, GetDialogControlValue(inDialog, dGroupCheckColors));

	// set "Style" checkboxes
	SetDialogControlValue(inDialog, dCheckBold, (style & bold) ? 1 : 0);
	SetDialogControlValue(inDialog, dCheckItalic, (style & italic) ? 1 : 0);
	SetDialogControlValue(inDialog, dCheckUnderline, (style & underline) ? 1 : 0);
	SetDialogControlValue(inDialog, dCheckOutline, (style & outline) ? 1 : 0);
	SetDialogControlValue(inDialog, dCheckShadow, (style & shadow) ? 1 : 0);
	SetDialogControlValue(inDialog, dCheckCondensed, (style & condense) ? 1 : 0);
	
	// set text creator editfield
	creatorString[0] = 4;
	memcpy (&creatorString[1], &gPrefs.textCreator, 4);	
	SetDialogItemTextAppearance(inDialog, dTextCreator, creatorString);

	// populate the renderer plugin menu
	InitPluginMenu(inDialog);
	
	// populate the font menu
	InitFontMenu(inDialog);
	
	// set the size menu to the proper size
	{
		MenuHandle	sizeMenu = GetDialogItemMenu(inDialog, dSizeMenu);
		
		if (sizeMenu)
		{
			int	i, count = CountMenuItems(sizeMenu);
			for (i = 1; i <= count; i++)
			{
				Str255 tempStr;
				long result;
				GetPopupMenuText(inDialog, dSizeMenu, i, tempStr);
				StringToNum(tempStr, &result);
				if (result == gFEPrefs.fontSize)
				{
					SetDialogControlValue(inDialog, dSizeMenu, i);
					break;
				}
			}
		}
	}
}


//===============================================================================
//	SetBit
//
//	Helper function for setting or resetting a specific bit in a UInt32 value.
//===============================================================================

static void SetBit(UInt32 *ioValue, UInt32 inBitMask, Boolean inFlag)
{
	if (inFlag)
		*ioValue |= inBitMask;
	else
		*ioValue &= ~inBitMask;
}


//===============================================================================
//	GetOptionsDialog
//
//	Read the dialog control values back into the preferences.
//===============================================================================

static void GetOptionsDialog(DialogRef inDialog)
{
	UInt32	style;
	UInt32	identifier;
	Str15	creatorString;
	Str255	tempStr;
	SInt32	tempNumber;
	
	// read miscellaneous checkboxes
	gFEPrefs.fe_listNoParentShots = GetDialogControlValue(inDialog, dCheckNoParentShots);

	// read list-related checkboxes
	gFEPrefs.fe_displayFileNames = !GetDialogControlValue(inDialog, dCheckNames);
	gFEPrefs.fe_noItalics = !GetDialogControlValue(inDialog, dCheckItalicClones);
	gFEPrefs.fe_listUseColors = GetDialogControlValue(inDialog, dGroupCheckColors);
	gFEPrefs.fe_listUseBrokenColors = GetDialogControlValue(inDialog, dRadioMAME);
	gFEPrefs.fe_listSortThe = GetDialogControlValue(inDialog, dCheckTheEnd);
	
	// read style checkboxes
	style = gFEPrefs.fontFace;
	SetBit(&style, bold, GetDialogControlValue(inDialog, dCheckBold));
	SetBit(&style, italic, GetDialogControlValue(inDialog, dCheckItalic));
	SetBit(&style, underline, GetDialogControlValue(inDialog, dCheckUnderline));
	SetBit(&style, outline, GetDialogControlValue(inDialog, dCheckOutline));
	SetBit(&style, shadow, GetDialogControlValue(inDialog, dCheckShadow));
	SetBit(&style, condense, GetDialogControlValue(inDialog, dCheckCondensed));
	gFEPrefs.fontFace = style;

	// read text creator editfield
	GetDialogItemTextAppearance(inDialog, dTextCreator, creatorString);
	memcpy (&gPrefs.textCreator, &creatorString[1], 4);

	// read plugin popup menu
	identifier = GetIndexedPlugin(GetDialogControlValue(inDialog, dPluginMenu) - 1, NULL, NULL, NULL);
	SetActivePlugin(identifier);
	
	// read font selection
	GetPopupMenuText(inDialog, dFontMenu, GetDialogControlValue(inDialog, dFontMenu), tempStr);
	PLstrcpy(gFEPrefs.font, tempStr);
	
	// read font size selection
	GetPopupMenuText(inDialog, dSizeMenu, GetDialogControlValue(inDialog, dSizeMenu), tempStr);
	StringToNum(tempStr, &tempNumber);
	gFEPrefs.fontSize = tempNumber;
}


//===============================================================================
//	InitPluginMenu
//
//	Initializes the plugin menu.
//===============================================================================

static void InitPluginMenu(DialogRef inDialog)
{
	MenuHandle		pluginMenu = GetDialogItemMenu(inDialog, dPluginMenu);
	ControlHandle	menuControl;
	OSErr			err;

	err = GetDialogItemAsControl(inDialog, dPluginMenu, &menuControl);

	if (err == noErr && pluginMenu)
	{
		UInt32	identifier;
		UInt32	activePluginItem = 0;
		UInt32	numPlugins = 0;

		// get the names of all installed plugins
		do
		{
			Boolean enable;
			Str255	name;
			
			identifier = GetIndexedPlugin(numPlugins, name, NULL, &enable);
			if (identifier)
			{
				// add the short plugin name to the menu
				numPlugins++;
				AppendMenuItemText(pluginMenu, "\pempty");
				SetMenuItemText(pluginMenu, numPlugins, name);
				if (enable)
					EnableMenuItem(pluginMenu, numPlugins);
				else
					DisableMenuItem(pluginMenu, numPlugins);
				
				// check to see if this is the active plugin
				if (identifier == GetActivePlugin())
				{
					activePluginItem = numPlugins;
				}
			}					
		}
		while (identifier);

		if (numPlugins)
		{				
			// fix the minimum and maximum
			SetControlMinimum(menuControl, 1);
			SetControlMaximum(menuControl, numPlugins);
			
			// set the menu to the currently active plugin
			SetControlValue(menuControl, activePluginItem);
			
			// enable or disable configure button appropriately
			SetDialogControlActive(inDialog, dPluginConfigure, PluginHasConfigDialog(GetActivePlugin()));
			SetDialogControlActive(inDialog, dPluginAbout, PluginHasAboutBox(GetActivePlugin()));
		}
		else
		{
			// no installed plugins!
			SetDialogControlActive(inDialog, dPluginMenu, false);
			SetDialogControlActive(inDialog, dPluginConfigure, false);
			SetDialogControlActive(inDialog, dPluginAbout, false);
		}
	}
	
	if (err)
	{
		SetDialogControlActive(inDialog, dPluginMenu, false);
		SetDialogControlActive(inDialog, dPluginConfigure, false);
		SetDialogControlActive(inDialog, dPluginAbout, false);
	}
}


//===============================================================================
//	InitFontMenu
//
//	Initializes the font menu.
//===============================================================================

static void InitFontMenu(DialogRef inDialog)
{
	MenuHandle		fontMenu = GetDialogItemMenu(inDialog, dFontMenu);
	ControlHandle	menuControl;
	OSErr			err;
	Str255			selectedName;
	int				i, count;

	// get the ControlRef of the plugin popup menu
	err = GetDialogItemAsControl(inDialog, dFontMenu, &menuControl);
	if (err == noErr && fontMenu)
	{
		AppendResMenu(fontMenu, 'FONT');

		// fix the minimum and maximum
		SetControlMinimum(menuControl, 1);
		count = CountMenuItems(fontMenu);
		SetControlMaximum(menuControl, count);
		
		// set the menu to the current font
		PLstrcpy(selectedName, gFEPrefs.font);
		while (1)
		{
			Str255 tempStr;
			
			// scan all the menu items for a match
			for (i = 1; i <= count; i++)
			{
				GetMenuItemText(fontMenu, i, tempStr);
				if (EqualString(selectedName, tempStr, true, true))
				{
					SetControlValue(menuControl, i);
					break;
				}
			}
			
			// if we got a match, break
			if (i <= count)
				break;
			
			// if we're not yet set to the system font, set it now; otherwise, break
			GetFontName(systemFont, tempStr);
			if (EqualString(tempStr, selectedName, true, true))
				break;
			PLstrcpy(selectedName, tempStr);
		}
	}
}


//===============================================================================
//	GetPopupMenuText
//
//	Returns the text of the selected item.
//===============================================================================

static void GetPopupMenuText(DialogRef inDialog, int inItemNumber, int inMenuItem, StringPtr outString)
{
	ControlHandle	menuControl;
	MenuHandle		fontMenu;
	Size			size;
	OSErr			err;

	// get the ControlRef of the plugin popup menu
	err = GetDialogItemAsControl(inDialog, inItemNumber, &menuControl);
	if (noErr == err)
	{
		// get the MenuHandle of the plugin popup menu
		err = GetControlData(menuControl, kControlMenuPart, 
				kControlPopupButtonMenuHandleTag, sizeof (MenuHandle), (Ptr)&fontMenu, &size); 

		if (err == noErr)
			GetMenuItemText(fontMenu, inMenuItem, outString);
	}
}
