/*##########################################################################

	macutils.h

	Miscellaneous utility routines for handling strings, dialogs, etc.

##########################################################################*/

#pragma once

#ifndef MACUTILS__	// workaround for gcc, which does not support #pragma once
#define MACUTILS__

/*##########################################################################
	CONSTANTS
##########################################################################*/

extern const RGBColor rgbBlack;
extern const RGBColor rgbWhite;
extern const RGBColor rgbGrey50;
extern const RGBColor rgbRed;
extern const RGBColor rgbYellow;

enum
{
	kScrollbarWidth 			= 15
};
	

/*##########################################################################
	FUNCTION PROTOTYPES
##########################################################################*/

void 			SetDialogControlActive(DialogRef inDialog, DialogItemIndex inItem, Boolean inActive);
Boolean			IsDialogControlActive(DialogRef inDialog, DialogItemIndex inItem);
void 			SetDialogControlVisible(DialogRef inDialog, DialogItemIndex inItem, Boolean inVisible);
Boolean			IsDialogControlVisible(DialogRef inDialog, DialogItemIndex inItem);
void 			SetDialogControlValue(DialogRef inDialog, DialogItemIndex inItem, SInt32 inValue);
SInt32			GetDialogControlValue(DialogRef inDialog, DialogItemIndex inItem);
void 			SetDialogItemTextAppearance(DialogRef inDialog, DialogItemIndex inItem, ConstStr255Param inText);
void 			GetDialogItemTextAppearance(DialogRef inDialog, DialogItemIndex inItem, StringPtr outText);
void 			FakeButtonClick(DialogRef inDialog, DialogItemIndex inItem);
MenuRef 		GetDialogItemMenu(DialogRef inDialog, DialogItemIndex inItem);
void 			GetDialogItemBounds(DialogRef inDialog, DialogItemIndex inItem, Rect *outBounds);
DialogItemType 	GetDialogItemType(DialogRef inDialog, DialogItemIndex inItem);
void			SetButtonValue(DialogRef inDialog, short inItem, short value);
short			GetButtonValue(DialogRef inDialog, short inItem);
Boolean			IsKeyPressed(UInt8 inKeycode);


SInt32 			GetSelectedCell(ListRef inList);
SInt32 			GetListDataBoundsBottom(ListRef inList);
void 			SelectCell(ListRef inList, SInt32 inRow);
Boolean 		ListEmpty(ListRef inList);
void 			InvalidateList(ListRef inList);

char *			GetIndCString(short inResource, short inIndex, const char *inDefault);
StringPtr 		GetIndPString(short inResource, short inIndex, const unsigned char *inDefault);

Boolean 		TestGestaltBit(OSType inSelector, short inBitToCheck);

#ifdef MAC_XCODE
OSErr GetFullOsdPathFromSpec(const FSSpec *spec, UInt8 *fullpath, int maxBufLen);
FILE *FSp_fopen(const FSSpec *fsSpec, const char *open_mode);
#else
#define GetFullOsdPathFromSpec(spec, fullpath, maxBufLen) GetFullPathFromSpec(spec, fullpath)
#endif

#endif
