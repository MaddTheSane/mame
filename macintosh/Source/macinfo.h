/*##########################################################################

	macinfo.h

	Handles the "info" tab of the main frontend dialog.

##########################################################################*/

#pragma once


/*##########################################################################
	FUNCTION PROTOTYPES
##########################################################################*/

void 		InfoInitializeUserPane(DialogRef inDialog, DialogItemIndex inBaseItem);
void 		InfoUpdateUserPane(DialogRef inDialog, DialogItemIndex inBaseItem, const game_driver *inGame);
void 		InfoHandleUserPane(DialogRef inDialog, DialogItemIndex inBaseItem, DialogItemIndex inItemHit, const game_driver *inGame);
void 		InfoTearDownUserPane(DialogRef inDialog, DialogItemIndex inBaseItem);
Boolean 	InfoKeyDownHandler(DialogRef inDialog, DialogItemIndex inBaseItem, EventRecord *inEvent, DialogItemIndex *outItemHit);
OSStatus	InfoHandleMouseWheelMoved(DialogRef inDialog, DialogItemIndex inBaseItem, Point QDLocation, EventMouseWheelAxis axis, SInt32 delta);
