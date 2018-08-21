/*##########################################################################

	macfrontend.h

	This code concerns itself with the user-interface dialog
	to choose a game.

	Original code by Evan Olcott, Brad Oliver, John Butler and Aaron Giles

##########################################################################*/

#pragma once

#include "maclists.h"

/*##########################################################################
	CONSTANTS
##########################################################################*/

enum
{
	kIconROMSetResource,
	kIconROMSetFolder,
	kIconROMSetZip,
	kIconROMSetGhost,
	kIconFolderOpen,
	kIconFolderClose,
	kIconBad,
	kTotalROMSetIcons
};

// general constants
enum
{
	kMainDialogMinimumWidth 	= 685,
	kMainDialogMinimumHeight 	= 518,
	
	kTabDITLResourceBase		= 2000,

	// LDEF drawing constants
	kIconHeight					= 16,
	kIconWidth					= 16,
	kCellXSlop					= 4,
	kCellYSlop					= 1,
	kIndentPixels				= 20
};

// tab constants
enum
{
	kTabInfo					= 1,
	kTabVideo,
	kTabAudio,
	kTabReports,
	kTabMisc,
#ifdef MESS
	kTabMESS,
#endif
	kTabCountPlusOne
};


/*##########################################################################
	FUNCTION PROTOTYPES
##########################################################################*/

Boolean 		ChooseGame(ROMSetData *romset);
UInt32			GetCurrentPane( void );
DialogItemIndex GetBaseIndex( UInt32 pane );
