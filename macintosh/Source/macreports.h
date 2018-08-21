/*##########################################################################

	macreports.h

	Routines for listing various driver info, auditing, etc.
	Written by Brad Oliver, John Butler, and Aaron Giles.

##########################################################################*/

#pragma once

#include "macprefs.h"


/*##########################################################################
	ENUMS
##########################################################################*/

// available reports
typedef enum
{
	kReportAllGames = 1,
	kReportGameRoms,
	kReportGameSamples,
	kReportDriverInfo,
	kReportClones,
	kReportParents,
	kReportBroken,
	kReportImpSound,
	kReportNoSound,
	kReportImpColor,
	kReportWrongColor
}
eReportType;


/*##########################################################################
	GLOBAL VARIABLES
##########################################################################*/

extern Boolean gForceRomsetUpdate;	// an alias was created by the audit


/*##########################################################################
	FUNCTION PROTOTYPES
##########################################################################*/

extern Boolean 		IsClone(const game_driver *inDrv);
extern Boolean 		IsNeoGeo(const game_driver *inDrv);
extern Boolean 		IsParent(const game_driver *inDrv);
extern Boolean 		IsBroken(const game_driver *inDrv);
extern Boolean		IsBIOS(const char *inName);

extern void 		SaveReport(short type);
extern void 		AuditROMs(void);
extern void 		AuditSamples(void);
extern void 		AnalyzeRomsets(void);
extern void 		DoRomident(void);

// progress dialog
extern DialogRef 	CreateProgressDialog(ConstStr255Param inTitle, ConstStr255Param inMsg, short inMax);
extern void 		UpdateProgressStatus(DialogRef inDialog, const char *inName, int inProcessed);
extern OSErr 		HandleProgressEvents(DialogRef inDialog);
extern void 		DestroyProgressDialog(DialogRef inDialog);
