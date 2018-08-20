/*##########################################################################

	macreports.c

	Routines for reporting various driver info, auditing, etc.
	Written by Brad Oliver, John Butler, and Aaron Giles.

##########################################################################*/

#include <Carbon/Carbon.h>

#include "driver.h"
#include "samples.h"
#include "audit.h"
#include "unzip.h"
#include "zlib.h"

#include "mac.h"
#include "macfiles.h"
#include "mactext.h"
#include "macstrings.h"
#include "macextras.h"
#include "macutils.h"
#include "macreports.h"

#define USE_SHEETS	0

#ifdef __MWERKS__
#pragma require_prototypes on
#endif

/*##########################################################################
	TYPEDEFS
##########################################################################*/

// report options
typedef struct
{
	eReportType	type;
	Boolean		excludeClones;
	Boolean		excludeParents;
} tReportOptions;

// audit options
typedef struct
{
	short	sortAlpha;
	Boolean	problemsOnly;
	Boolean	listMissing;
	Boolean	suppressWarnings;
	Boolean ignoreCHDs;
	Str31	filter;
} tAuditOptions;


/*##########################################################################
	CONSTANTS
##########################################################################*/

// dialog/alert resource IDs
enum
{
	rProgressDialog	= 3000,
	rAuditOptions = 3100,
	rReportOptionsDLOG = 3200,
	rConfirmReplaceALRT = 3201
};

// string index for default name of each report
static const short kReportNameStringID[] =
{
	0,
	kGameList,		// kReportAllGames
	kROMList,		// kReportGameRoms
	kSampleList,	// kReportGameSamples
	kDriverList,	// kReportDriverInfo
	kCloneList,
	kParentList,
	kBrokenList,
	kImpSoundList,
	kNoSoundList,
	kImpColorList,
	kWrongColorList
};

// initial defaults for report options dialog
const static tReportOptions kReportOptionsDefaults =
{
	kReportAllGames,	// type
	false,				// excludeClones
	false				// excludeParents
};

// initial defaults for audit options dialog
const static tAuditOptions kAuditOptionsDefaults =
{
	-1,		/* sortAlpha */		// no, but sort by driver name is default
	true,	/* problemsOnly */	// yes
	false,	/* listMissing */	// no
	true,	/* suppressWarn */	// yes
	false,	/* ignoreCHDs */	// no
	"\p*"	/* filter */		// matches all drivers
};

// dialog items for rProgressDLOG
enum
{
	dProgressIndicator = 1,
	dProgressStopButton = 2,
	dProgressMsgText = 3,
	dProgressCurrentItemText = 4,
	dProgressRemainingText = 5
};

// dialog items for rReportOptionsDLOG
enum
{
	dReportOK = 1,
	dReportCancel,
	dReportAdvancedBox,
	dReportTypeMenu,
	dReportExcludeClones,
	dReportExcludeParents,
	dReportDefaults
};

// dialog items for rAuditOptions DLOG
enum
{
	dAuditOK = 1,
	dAuditCancel = 2,
	dAuditDefaults = 3,
	dAuditAdvancedBox,
	dAuditProblemsOnly,
	dAuditSortAlpha,
	dAuditStaticText1,
	dAuditFilterText,
	dAuditSortByName,
	dAuditSortByDesc,
	dAuditListMissing,
	dAuditSuppressWarnings,
	dAuditIgnoreCHDs
};


/*##########################################################################
	GLOBAL VARIABLES
##########################################################################*/

static UInt32		sLastProgressTicks;

extern game_driver	*gDriversBIOS[32];
extern int					gTotalBIOS;


/*##########################################################################
	FUNCTION PROTOTYPES
##########################################################################*/

// reports dialog
static void WriteReport (const tReportOptions *inOpt);

static const char *GetSpacesString (int inNumSpaces);

static Boolean DriverHasSamples(const game_driver *inDrv);

static void identify_rom(const char* name, int checksum);
static void identify_file(const char* name);
static void identify_zip(const char* zipname);

const char *GetBIOSName (const game_driver *inDrv);

/*##########################################################################
	IMPLEMENTATION
##########################################################################*/

#pragma mark ¥ Reports Routines


//===============================================================================
//	ApplyReportOptions
//
//	Set controls in the report options dialog to match the specified settings.
//===============================================================================

static void ApplyReportOptions (DialogRef inDialog, const tReportOptions *inOpt)
{
	// set "report type" popup menu
	SetDialogControlValue (inDialog, dReportTypeMenu, inOpt->type);

	// set "exclude clones" checkbox
	SetDialogControlValue (inDialog, dReportExcludeClones, inOpt->excludeClones);
	
	// set "exclude parents" checkbox
	SetDialogControlValue (inDialog, dReportExcludeParents, inOpt->excludeParents);
}


//===============================================================================
//	GetReportOptions
//
//	Display a dialog which lets the user specify his preferred report options.
//
//	Current settings are read from the ioOpt parameter.
//	User-specified settings are returned in the ioOpt parameter.
//===============================================================================

static OSErr GetReportOptions (tReportOptions *ioOpt)
{
	DialogRef 		dlog;
	GrafPtr			savePort;
	short			itemHit;
	Boolean			done = false;
	tReportOptions	opt = *ioOpt;
	ModalFilterUPP	modalFilter = NULL;
	//Boolean			replaceConfirm = false;
	OSErr			err = noErr;

	dlog = GetNewDialog (rReportOptionsDLOG, NULL, kWindowToFront);

	if (dlog)
	{
		GetPort (&savePort);
		SetPortDialogPort (dlog);
		SetDialogDefaultItem (dlog, dReportOK);
		
		// setup dialog items according to current options
		ApplyReportOptions (dlog, &opt);
		
#if USE_SHEETS
		ShowSheetWindow (GetDialogWindow(dlog), parentWin);
#else
		ShowWindow (GetDialogWindow(dlog));
#endif

		//modalFilter = NewModalFilterUPP (reportOptionsFilter);
		while (!done)
		{
			ModalDialog (modalFilter, &itemHit);
			
			switch (itemHit)
			{
				case dReportOK:
					*ioOpt = opt;
					done = true;
					break;
				case dReportCancel:
					done = true;
					err = userCanceledErr;
					break;
				case dReportTypeMenu:
					opt.type = (eReportType)GetDialogControlValue (dlog, dReportTypeMenu);
					break;
				case dReportExcludeClones:
					// toggle checkbox
					opt.excludeClones = !(opt.excludeClones);
					SetDialogControlValue (dlog, dReportExcludeClones, opt.excludeClones);
					break;
				case dReportExcludeParents:
					// toggle checkbox
					opt.excludeParents = !(opt.excludeParents);
					SetDialogControlValue (dlog, dReportExcludeParents, opt.excludeParents);
					break;
				case dReportDefaults:
					opt = kReportOptionsDefaults;
					ApplyReportOptions (dlog, &opt);
					break;
			}
		} // end while
		
#if USE_SHEETS
		HideSheetWindow (GetDialogWindow(dlog));
#endif
		if (modalFilter) DisposeModalFilterUPP (modalFilter);
		SetPort (savePort);
		DisposeDialog (dlog);
	}
	return err;
}


//===============================================================================
//	SaveReport
//
//	Displays a dialog which lets the user specify which report to generate
//	various options.
//
//	NOTE: This function is written to accommodate menu items that call for
//	a specific report by setting inType to a non-zero value. All such menu
//	items have been removed, so in practice this is only called with inType == 0,
//	which leaves the report type popup set to whatever it was set to last time
//	we were called.
//===============================================================================

void SaveReport (short inType)
{
	tReportOptions opt;	// keep these around for next time
	OSErr	err;

	// read options from global preferences
	opt.type = (eReportType)gFEPrefs.repType;
	opt.excludeClones = gFEPrefs.repExcludeClones;
	opt.excludeParents = gFEPrefs.repExcludeParents;
	
	if (inType)
	{
		// a specific report type was specified
		opt.type = (eReportType)inType;
	}
		
	// prompt the user for options
	err = GetReportOptions (&opt);

	if (noErr == err)
	{
		Cursor		arrow;
		
		// save options to global preferences
		gFEPrefs.repType = (short)opt.type;
		gFEPrefs.repExcludeClones = opt.excludeClones;
		gFEPrefs.repExcludeParents = opt.excludeParents;
		
		// set the watch cursor
		if (gWatchCursor)
			SetCursor(*gWatchCursor);

		// create the report
		WriteReport (&opt);
		
		// back to an arrow
		SetCursor(GetQDGlobalsArrow(&arrow));
	}
}


//===============================================================================
//	WriteReport
//
//	Generate one of various reports using the specified options and display the 
//	results in a dialog.
//===============================================================================

static void WriteReport (const tReportOptions *inOpt)
{
	Str255						defaultName;
	const game_driver	**	sorted;
	int							index, count, clone_count;
	OSErr						err;
	
	// create an alpha sorted list of indexes for drivers
	sorted = GetSortedDriverArray (kSortedByName);
	if (!sorted)
	{
		printf (GetIndCString (rErrorStrings, kCouldntCreateSortError,
			"Couldn't allocate memory for creating list!"));
		return;
	}
	
	// get the name of the dialog and the default file name for the text file
	GetIndString (defaultName, rStrings, kReportNameStringID[inOpt->type]);

	// create a dialog and WASTE instance to hold our output
	err = CreateTextResults (defaultName);
	if (err) return;

	// stick a header on top
	switch (inOpt->type)
	{
		case kReportAllGames:
		case kReportGameRoms:
		case kReportDriverInfo:
			rPrintf (GetIndCString (rStrings, kReportSupportedGamesHeader,
				"MAME currently supports the following games:"));
			break;
		case kReportGameSamples:
			rPrintf (GetIndCString (rStrings, kReportSoundSamplesHeader,
				"MAME currently supports sound samples for the following games:"));
			break;
		case kReportClones:
			rPrintf (GetIndCString (rStrings, kReportClonesHeader,
				"MAME currently supports the following clones:"));
			break;
		case kReportParents:
			rPrintf (GetIndCString (rStrings, kReportParentsHeader,
				"MAME currently supports the following parents:"));
			break;
		case kReportBroken:
			rPrintf (GetIndCString (rStrings, kReportBrokenHeader,
				"The following games are currently non-working:"));
			break;
		case kReportImpSound:
			rPrintf (GetIndCString (rStrings, kReportImpSoundHeader,
				"The following games have imperfect sound:"));
			break;
		case kReportNoSound:
			rPrintf (GetIndCString (rStrings, kReportNoSoundHeader,
				"The following games have no sound:"));
			break;
		case kReportImpColor:
			rPrintf (GetIndCString (rStrings, kReportImpColorHeader,
				"The following games have imperfect color:"));
			break;
		case kReportWrongColor:
			rPrintf (GetIndCString (rStrings, kReportWrongColorHeader,
				"The following games have totally wrong color:"));
			break;
	}
	rPrintf ("\n\n");
	
	// loop over game drivers
	for (index = count = clone_count = 0; drivers[index]; index++)
	{
		const game_driver *gamedrv = sorted[index];
		Boolean isClone, isParent, isNeo;
		int hasSamples = -1;
		int i;
		machine_config driver;

		expand_machine_driver(gamedrv->drv, &driver);


		isClone = IsClone(gamedrv);
		isParent = IsParent(gamedrv);
		isNeo = IsNeoGeo(gamedrv);
		
		if (isClone && inOpt->excludeClones) continue;
		if (isParent && inOpt->excludeParents) continue;
		
		if (inOpt->type == kReportClones && !isClone) continue;
		if (inOpt->type == kReportParents && !isParent) continue;
		if (inOpt->type == kReportBroken && !IsBroken(gamedrv)) continue;

		if (inOpt->type == kReportImpSound && !(gamedrv->flags & GAME_IMPERFECT_SOUND)) continue;
		if (inOpt->type == kReportNoSound && !(gamedrv->flags & GAME_NO_SOUND)) continue;
		if (inOpt->type == kReportImpColor && !(gamedrv->flags & GAME_IMPERFECT_COLORS)) continue;
		if (inOpt->type == kReportWrongColor && !(gamedrv->flags & GAME_WRONG_COLORS)) continue;

		if (inOpt->type == kReportGameSamples)
		{
			// Loop over the sound systems and see if one of them is SOUND_SAMPLES
			for (i = 0; i < MAX_SOUND; i++)
			{
				const sound_config *sound = &driver.sound[i];
			
				// This assumes that only one sound subsystem uses samples
				if (sound->sound_type == SOUND_SAMPLES)
					hasSamples = i;
			}
		
			if (hasSamples == -1) continue;
		}

		count++;
		if (isClone)
			clone_count++;
			
		// name of the game
		rPrintf ("%-10s\"%s\"\n", gamedrv->name, gamedrv->description);
		
		// display ROMs needed
		if (inOpt->type == kReportGameRoms)
		{
			const rom_entry *romp = gamedrv->rom;
			const rom_entry *region, *rom, *chunk;
			
			if (NULL == romp)
			{
				rPrintf (GetIndCString (rStrings, kReportNoRomsNeededLabel, "<No ROMs needed>"));
				rPrintf ("\n\n");
				continue;
			}
			else if (IsClone(gamedrv))
			{
				rPrintf (GetIndCString (rStrings, kReportRomsNeededCloneLabel,
					"ROMs needed:  (ROMs may be located in '%s' romset)"), 
					gamedrv->clone_of->name);
				rPrintf ("\n");
			}
			else
			{
				rPrintf (GetIndCString (rStrings, kReportRomsNeededLabel, "ROMs needed:"));
				rPrintf ("\n");
			}
			
			// note this loop is basically the code from printromlist() in common.c
			for (region = romp; region; region = rom_next_region(region))
			{
				for (rom = rom_first_file(region); rom; rom = rom_next_file(rom))
				{
					const char *name = ROM_GETNAME(rom);
					const char *hash = ROM_GETHASHDATA(rom);
					int length = -1; /* default is for disks! */
					char buf[512];

					if (ROMREGION_ISROMDATA(region))
					{
						length = 0;
						for (chunk = rom_first_chunk(rom); chunk; chunk = rom_next_chunk(chunk))
							length += ROM_GETLENGTH(chunk);
					}

					rPrintf("%-12s ", name);
					if (length >= 0)
						rPrintf("%7d bytes",length);
						else
						rPrintf("       ");

					if (!hash_data_has_info(hash, HASH_INFO_NO_DUMP))
					{
						hash_data_print(hash, 0, buf);
						rPrintf(" %s\n", buf);
					}
					else
						rPrintf(" %s\n", GetIndCString (rStrings, kReportNoGoodDump, "NO GOOD DUMP KNOWN"));
				}
			}
			rPrintf ("\n");
		}		
		
		// display samples needed
		if (inOpt->type == kReportGameSamples)
		{
			const sound_config *sound = &driver.sound[hasSamples];
			const struct Samplesinterface *samples = sound->config;

			if (samples->samplenames && samples->samplenames[0])
			{
				i = 0;
				if (samples->samplenames[0][0] == '*')
				{
					i++;
					if (strcmp (gamedrv->name, &samples->samplenames[0][1]))
					{
						rPrintf (GetIndCString (rStrings, kReportSamplesNeededCloneLabel,
							"Samples needed:  (Samples may be located in '%s' set)"), 
							&samples->samplenames[0][1]);
						rPrintf ("\n");
					}
					else
					{
						rPrintf (GetIndCString (rStrings, kReportSamplesNeededLabel,
							"Samples needed:"));
						rPrintf ("\n");
					}
				}
				else
				{
					rPrintf (GetIndCString (rStrings, kReportSamplesNeededLabel,
						"Samples needed:"));
					rPrintf ("\n");
				}

				for ( ; samples->samplenames[i]; i++)
					rPrintf ("    %s\n", samples->samplenames[i]);
				rPrintf ("\n");
			}
		}

		// ASG 971130: new case
		// display detailed driver info
		if (inOpt->type == kReportDriverInfo)
		{
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

				rPrintf("    ");
				if (count > 1)
					rPrintf("%d x ",count);

				rPrintf("%s",cputype_name(type));
			
				if (clock >= 1000000)
					rPrintf(" @ %d.%06d MHz",
							clock / 1000000,
							clock % 1000000);
				else
					rPrintf(" @ %d.%03d kHz",
							clock / 1000,
							clock % 1000);
				
				rPrintf(", %d ints/frame", driver.cpu[i].vblank_interrupts_per_frame);

				rPrintf("\n");
			}

			{
				i = 0;
				while (i < MAX_SPEAKER && driver.speaker[i].tag)
					i++;
				
				if	 (i == 1) rPrintf("    1 speaker\n");
				else		  rPrintf("    %d speakers\n", i);
			}
	
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

				rPrintf("    ");
				if (count > 1)
					rPrintf("%d x ",count);

				rPrintf("%s",sndtype_name(type));

				if (clock)
				{
					if (clock >= 1000000)
						rPrintf(" @ %d.%06d MHz",
								clock / 1000000,
								clock % 1000000);
					else
						rPrintf(" @ %d.%03d kHz",
								clock / 1000,
								clock % 1000);
				}
				
				rPrintf("\n");
			}
			
			int isVector = (driver.video_attributes & VIDEO_TYPE_VECTOR);
			
			rPrintf ("    © %s by %s\n", gamedrv->year, gamedrv->manufacturer);
			if (IsClone(gamedrv))
				rPrintf ("    Clone of: %s\n", gamedrv->clone_of->description);
			rPrintf ("    Refresh rate: %f frames/sec\n", driver.frames_per_second);
			rPrintf ("    CPU slices per frame: %d\n", driver.cpu_slices_per_frame);
			rPrintf ("    Video size: %dx%d\n", driver.default_visible_area.max_x+1-isVector-driver.default_visible_area.min_x,
												driver.default_visible_area.max_y+1-isVector-driver.default_visible_area.min_y);
			rPrintf ("    Colors used: %d\n", driver.total_colors);
			rPrintf ("    Colortable size: %d\n", driver.color_table_len);
			if (isVector)
				rPrintf ("    Vector game\n");
			else
				rPrintf ("    Raster game\n");
			if (driver.video_attributes & VIDEO_DUAL_MONITOR)
				rPrintf ("    Multiple-monitor game\n");
			/*if (driver.video_attributes & VIDEO_SUPPORTS_DIRTY)
				rPrintf ("    Dirty rectangle support\n");*/
			if (driver.video_attributes & VIDEO_NEEDS_6BITS_PER_GUN)
				rPrintf ("    Requires 24-bit video\n");
			if (driver.video_attributes & VIDEO_RGB_DIRECT)
				rPrintf ("    Supports direct video (alpha)\n");
			rPrintf ("\n");
		}

		// parent of this clone
		if (inOpt->type == kReportClones)
		{
			rPrintf (GetIndCString (rStrings, kReportCloneOfLabel, "Clone of"));
			rPrintf (": %-10s\"%s\"\n\n", gamedrv->clone_of->name, 
						gamedrv->clone_of->description);
		}
		
		// clones of this parent
		if (inOpt->type == kReportParents)
		{
			rPrintf (GetIndCString (rStrings, kReportParentOfLabel, "Parent of"));
			rPrintf (": ");
			
			for (i = 0; drivers[i]; i++)
			{
				if (drivers[i]->clone_of == gamedrv)
				{
					int numSpaces = 2 + strlen (GetIndCString (rStrings, kReportParentOfLabel, "Parent of"));
					rPrintf ("%-10s\"%s\"\n", drivers[i]->name, drivers[i]->description);
					rPrintf (GetSpacesString (numSpaces));
				}
			}

			rPrintf ("\n");
		}
	} // end for loop
	
	rPrintf ("\n");
	
	// final summary
	switch (inOpt->type)
	{
		case kReportAllGames:
		case kReportGameRoms:
		case kReportDriverInfo:
			rPrintf (GetIndCString (rStrings, kReportTotalGamesSummary, "Total games supported"));
			rPrintf (": %4d\n", count);
			
			rPrintf (GetIndCString (rStrings, kReportUniqueGamesSummary, "Total unique games"));
			rPrintf (": %4d (%3d clones)\n", count - clone_count, clone_count);
			break;
		case kReportGameSamples:
			rPrintf (GetIndCString (rStrings, kReportSamplesSummary, "Total games supporting samples"));
			rPrintf (": %4d\n", count);
			break;
		case kReportClones:
			rPrintf (GetIndCString (rStrings, kReportClonesSummary, "Total number of clones supported"));
			rPrintf (": %4d\n", count);
			break;
		case kReportParents:
			rPrintf (GetIndCString (rStrings, kReportParentsSummary, "Total number of parents supported"));
			rPrintf (": %4d\n", count);
			break;
		case kReportBroken:
			rPrintf (GetIndCString (rStrings, kReportBrokenSummary, "Total number of non-working games"));
			rPrintf (": %4d\n", count);
			break;
		case kReportImpSound:
			rPrintf (GetIndCString (rStrings, kReportImpSoundSummary, "Total number of games with imperfect sound"));
			rPrintf (": %4d\n", count);
			break;
		case kReportNoSound:
			rPrintf (GetIndCString (rStrings, kReportNoSoundSummary, "Total number of games with no sound"));
			rPrintf (": %4d\n", count);
			break;
		case kReportImpColor:
			rPrintf (GetIndCString (rStrings, kReportImpColorSummary, "Total number of games with imperfect color"));
			rPrintf (": %4d\n", count);
			break;
		case kReportWrongColor:
			rPrintf (GetIndCString (rStrings, kReportWrongColorSummary, "Total number of games with totally wrong color"));
			rPrintf (": %4d\n", count);
			break;
	}

	// display our results in a dialog box
	DisplayTextResults (defaultName);
}

#pragma mark -
#pragma mark ¥ Audit Routines


//===============================================================================
//	IsClone
//
//	Returns true if the specified game_driver is a clone.
//===============================================================================

Boolean IsClone(const game_driver *inDrv)
{
	if (inDrv->clone_of && !(inDrv->clone_of->flags & NOT_A_DRIVER))
		return true;
	else
		return false;
}

//===============================================================================
//	IsBIOS
//
//	Returns true if the specified game_driver is a BIOS ROM set.
//===============================================================================

Boolean IsBIOS(const char *inName)
{
	int i;
	char temp[32];
	char *cPtr;
	
	// Make a temp duplicate of the string and strip off the . extension if
	// it exists
	strcpy (temp, inName);
	if ((cPtr = strstr (temp, ".")))
	{
		*cPtr = NULL;
	} 
	
	for (i = 0; i < gTotalBIOS; i ++)
	{
		const char *temp2 = gDriversBIOS[i]->name;
		
		if (strncmp (temp2, temp, strlen (temp)) == 0)
			return true;
	}
	return false;
}

//===============================================================================
//	GetBIOSName
//
//	Gets the driver name of the BIOS set for a ROM, if it exists.
//===============================================================================

const char *GetBIOSName (const game_driver *inDrv)
{
	while (inDrv)
	{
		if (inDrv->flags & NOT_A_DRIVER)
		{
			return inDrv->name;
		}
		
		// Walk up the inheritance list.
		inDrv = inDrv->clone_of;
	}
	return "";
}

//===============================================================================
//	IsNeoGeo
//
//	Returns true if the specified game_driver is a Neo-Geo game.
//===============================================================================

Boolean IsNeoGeo(const game_driver *inDrv)
{
#ifndef MESS
	extern game_driver driver_neogeo;
	const game_driver *parent = inDrv->clone_of;
	
	if (parent && (parent == &driver_neogeo || parent->clone_of == &driver_neogeo))
		return true;
	else
#endif
		return false;
}

//===============================================================================
//	IsBroken
//
//	Returns true if the specified game_driver is not working.
//===============================================================================

Boolean IsBroken(const game_driver *inDrv)
{
	if (inDrv->flags & GAME_NOT_WORKING)
		return true;
	else
		return false;
}


//===============================================================================
//	IsParent
//
//	Returns true if the specified game_driver is a parent (has at least 1 clone).
//===============================================================================

Boolean IsParent(const game_driver *inDrv)
{
	int i;
	
	for (i = 0; drivers[i]; i++)
		if (drivers[i]->clone_of == inDrv)
			return true;
	
	return false;
}


//===============================================================================
//	strwildcmp
//
//	(borrowed from msdos/fronthlp.c)
//	Does wildcard matching of two 8-character strings. Used to filter the ROM
//	audit by driver name.
//===============================================================================

static int strwildcmp(const char *sp1, const char *sp2)
{
	char s1[9], s2[9];
	int i, l1, l2;
	char *p;

	strncpy(s1, sp1, 8); s1[8] = 0; if (s1[0] == 0) strcpy(s1, "*");

	strncpy(s2, sp2, 8); s2[8] = 0; if (s2[0] == 0) strcpy(s2, "*");

	p = strchr(s1, '*');
	if (p)
	{
		for (i = p - s1; i < 8; i++) s1[i] = '?';
		s1[8] = 0;
	}

	p = strchr(s2, '*');
	if (p)
	{
		for (i = p - s2; i < 8; i++) s2[i] = '?';
		s2[8] = 0;
	}

	l1 = strlen(s1);
	if (l1 < 8)
	{
		for (i = l1 + 1; i < 8; i++) s1[i] = ' ';
		s1[8] = 0;
	}

	l2 = strlen(s2);
	if (l2 < 8)
	{
		for (i = l2 + 1; i < 8; i++) s2[i] = ' ';
		s2[8] = 0;
	}

	for (i = 0; i < 8; i++)
	{
		if (s1[i] == '?' && s2[i] != '?') s1[i] = s2[i];
		if (s2[i] == '?' && s1[i] != '?') s2[i] = s1[i];
	}

	return mame_stricmp(s1, s2);
}


//===============================================================================
//	auditOptionsFilter
//
//	ModalFilterProc for the audit options dialog.
//===============================================================================

static pascal Boolean auditOptionsFilter (DialogRef inDialog, EventRecord *inEvent, short *outItemHit)
{
	Handle		filter;
	char		eventChar;
	Str31		text;
	OSErr		err;

	if (inEvent->what==keyDown || inEvent->what==autoKey)
	{
		eventChar = inEvent->message & charCodeMask;
		
		if (eventChar == 0x1b)
		{
			// user canceled by pressing escape key
			*outItemHit = dAuditCancel;
			FakeButtonClick (inDialog, dAuditCancel);
			return true;
		}
		
		if (eventChar >= ' ')
		{
			// user is attempting to type into the EditText control
			
			// get a handle to the EditText control
			err = GetDialogItemAsControl (inDialog, dAuditFilterText, (ControlRef*) &filter);
			
			if (noErr == err)
			{
				// get the current text
				GetDialogItemText (filter, text);
				
				// limit filter text to 8 characters max
				if (text[0] >= 8)
				{
					// Get the TEHandle shared by all the EditText items in the dialog
					// (See TN 1041 - IM: Files Errata)
					TEHandle teHand = GetDialogTextEditHandle(inDialog);

					// only allow the keystroke if there is a non-empty selection
					if ((**teHand).selEnd == (**teHand).selStart)
					{
						// allowing the keystroke would exceed the maximum text length
						SysBeep (1);
						*outItemHit = dAuditFilterText;
						return true;
					}
				}
			}
		}
	}
	
	// let the standard dialog filter handle anything else
	return StdFilterProc (inDialog, inEvent, outItemHit);
}


//===============================================================================
//	ApplyAuditOptions
//
//	Adjust the controls in the options dialog to match the settings in the 
//	specified tAuditOptions struct.
//===============================================================================

static void ApplyAuditOptions (DialogRef inDialog, const tAuditOptions *inOpt)
{
	// set "sort drivers alphabetically" option
	SetDialogControlValue (inDialog, dAuditSortAlpha, inOpt->sortAlpha > 0);
	SetDialogControlValue (inDialog, dAuditSortByName, inOpt->sortAlpha==1 || inOpt->sortAlpha==-1);
	SetDialogControlValue (inDialog, dAuditSortByDesc, inOpt->sortAlpha==2 || inOpt->sortAlpha==-2);
	SetDialogControlActive (inDialog, dAuditSortByName, inOpt->sortAlpha > 0);
	SetDialogControlActive (inDialog, dAuditSortByDesc, inOpt->sortAlpha > 0);
	
	// set "list problems only" option
	SetDialogControlValue (inDialog, dAuditProblemsOnly, inOpt->problemsOnly);
	
	// set "list romsets not found in ROMs folder" option
	SetDialogControlValue (inDialog, dAuditListMissing, inOpt->listMissing);
#ifndef MAME_DEBUG
	{
		OSErr err;
		Handle hand;
		
		// Hide this control in non-debug builds	
		err = GetDialogItemAsControl (inDialog, dAuditListMissing, (ControlRef *) & hand);
		if (err == noErr)
			HideControl ((ControlRef) hand);
	}
#endif
	
	// set "suppress warnings for known problems" option
	SetDialogControlValue (inDialog, dAuditSuppressWarnings, inOpt->suppressWarnings);
	
	// set "suppress warnings for known problems" option
	SetDialogControlValue (inDialog, dAuditIgnoreCHDs, inOpt->ignoreCHDs);
	
	// set Audit Filter text
	SetDialogItemTextAppearance (inDialog, dAuditFilterText, inOpt->filter);
}


//===============================================================================
//	GetAuditOptions
//
//	Display a dialog that lets the user specify options for the ROM audit.
//	The current options settings are read from the ioOpt parameter.
//	The user-specified options settings are returned in the ioOpt parameter.
//===============================================================================
extern WindowRef parentWin;
static OSErr GetAuditOptions (tAuditOptions *ioOpt)
{
	DialogRef 		dlog;
	GrafPtr			savePort;
	short			itemHit;
	Boolean			done = false;
	tAuditOptions	opt = *ioOpt;
	ModalFilterUPP	modalFilter;
	OSErr			err = noErr;

	// load the audit options dialog
	dlog = GetNewDialog (rAuditOptions, NULL, kWindowToFront);

	if (dlog)
	{
		GetPort (&savePort);
		SetPortDialogPort (dlog);
		SetDialogDefaultItem (dlog, dAuditOK);
		
		// setup dialog controls according to current options
		ApplyAuditOptions (dlog, &opt);
		
		// select the single edit text item
		SelectDialogItemText (dlog, dAuditFilterText, 0, 32767);

		// now make the dialog visible
#if USE_SHEETS
		ShowSheetWindow (GetDialogWindow(dlog), parentWin);
#else
		ShowWindow (GetDialogWindow(dlog));
#endif

		modalFilter = NewModalFilterUPP (auditOptionsFilter);

		while (!done)
		{
			ModalDialog (modalFilter, &itemHit);
			
			switch (itemHit)
			{
				case dAuditOK:
				{
					Handle hand;

					// get the EditText specifying the filter option
					(void)GetDialogItemAsControl (dlog, dAuditFilterText, (ControlRef*)&hand);					
					if (hand)
					{
						GetDialogItemText (hand, opt.filter);
					}
					
					// set the resulting options
					*ioOpt = opt;
					done = true;
					break;
				}
				case dAuditCancel:
					// user canceled the dialog
					done = true;
					err = userCanceledErr;
					break;
				case dAuditDefaults:
					// change the dialog controls to match the default options
					opt = kAuditOptionsDefaults;
					ApplyAuditOptions (dlog, &opt);
					break;
				case dAuditProblemsOnly:
					// toggle checkbox
					opt.problemsOnly = !(opt.problemsOnly);
					SetDialogControlValue (dlog, dAuditProblemsOnly, opt.problemsOnly);
					break;
				case dAuditSortAlpha:
					// toggle checkbox and enable/disable dependent controls
					opt.sortAlpha = -opt.sortAlpha;
					SetDialogControlValue (dlog, dAuditSortAlpha, opt.sortAlpha > 0);
					SetDialogControlActive (dlog, dAuditSortByName, opt.sortAlpha > 0);
					SetDialogControlActive (dlog, dAuditSortByDesc, opt.sortAlpha > 0);
					break;
				case dAuditSortByName:
					// handle radio button
					if (opt.sortAlpha != 1)
					{
						opt.sortAlpha = 1;
						SetDialogControlValue (dlog, dAuditSortByName, true);
						SetDialogControlValue (dlog, dAuditSortByDesc, false);
					}
					break;
				case dAuditSortByDesc:
					// handle radio button
					if (opt.sortAlpha != 2)
					{
						opt.sortAlpha = 2;
						SetDialogControlValue (dlog, dAuditSortByName, false);
						SetDialogControlValue (dlog, dAuditSortByDesc, true);
					}
					break;
				case dAuditListMissing:
					// toggle checkbox
					opt.listMissing = !(opt.listMissing);
					SetDialogControlValue (dlog, dAuditListMissing, opt.listMissing);
					break;
				case dAuditSuppressWarnings:
					// toggle checkbox
					opt.suppressWarnings = !(opt.suppressWarnings);
					SetDialogControlValue (dlog, dAuditSuppressWarnings, opt.suppressWarnings);
					break;
				case dAuditIgnoreCHDs:
					// toggle checkbox
					opt.ignoreCHDs = !(opt.ignoreCHDs);
					SetDialogControlValue (dlog, dAuditIgnoreCHDs, opt.ignoreCHDs);
					break;
				case dAuditFilterText:
					break;
			}
		} // end while
		
#if USE_SHEETS
		HideSheetWindow (GetDialogWindow(dlog));
#endif
		DisposeModalFilterUPP (modalFilter);
		SetPort (savePort);
		DisposeDialog (dlog);
	}
	return err;
}


//===============================================================================
//	ListDuplicateRomsets
//
//	Called by AuditROMs to print duplicate romsets found in ROMs folder. Returns
//	true if any duplicates were found.
//
//	Pass in a sort map created by calling GetSortedDriverArray() if you want
//	to check drivers in sorted order, or NULL if you don't care.
//===============================================================================

static Boolean ListDuplicateRomsets (const game_driver **inSorted)
{
	Boolean	dupesFound = false;
	const ROMSetData *romset;
	int		i, pass;
	const game_driver **driverPtr;
	
	
	// Do two passes. On the first, check for dupes in the regular drivers[] structure.
	// On the second, check for dupes in our BIOS drivers structure, gDriversBIOS.
	for (pass = 0; pass < 2; pass ++)
	{
		if (pass == 1)
		{
			// Second pass, check for BIOS dupes
			inSorted = (const game_driver **)gDriversBIOS;
			driverPtr = (const game_driver **)gDriversBIOS;
		}
		else
		{
			// First pass, check for regular ROM set dupes, using an optional
			// sorting map.
			if (!inSorted)
				inSorted = drivers;
			driverPtr = drivers;
		}
	
		// loop through each defined game_driver
		for (i = 0; driverPtr[i]; i++)
		{
			const game_driver *	game;
			UInt8 						path[kMacMaxPath];
			FSSpec						saveSpec;
			int 						setsfound = 0;
		
			// use sort map if requested
			game = inSorted[i];
		
			// loop through gRomsets (contents of ROMs folder)
			for (romset = GetFirstROMSet(); romset != NULL; romset = romset->next)
			{
				char temp[32];
		
				if (pass == 1)
				{
					char *cPtr;

					// Turn the spec name into a C string, make a dupe and strip off any
					// . extension
					p2cstrcpy (temp, romset->spec.name);
	
					if ((cPtr = strstr (temp, ".")))
						*cPtr = NULL;
				}

				// does the index of this game_driver match the index of this romset?
				// Or does the filespec name match the BIOS name for this entry?
				if (((pass == 0) && (game == romset->driver)) ||
					((pass == 1) && (strcmp (temp, game->name) == 0)))
				{
					// yes, bump the number of matching romsets found
					setsfound++;
				
					if (setsfound == 1)
					{
						// no duplicates found (yet), but save the FSSpec for this romset
						saveSpec = romset->spec;
					}
					else if (setsfound == 2)
					{
						// first duplicate romset found -- report it
						(void)GetFullPathFromSpec (&saveSpec, path, sizeof (path));
						rPrintf ("| %s '%s':\n", GetIndCString (rStrings, kReportMultipleRomsetsFound, "Multiple romsets found for driver"), game->name);
						rPrintf ("|    1. %s\n", path);
						(void)GetFullPathFromSpec (&romset->spec, path, sizeof (path));
						rPrintf ("|    2. %s\n", path);
					}
					else
					{
						// another duplicate -- add it to the list
						(void)GetFullPathFromSpec (&romset->spec, path, sizeof (path));
						rPrintf ("|    %d. %s\n", setsfound, path);
					}
				}
			}
		
			if (setsfound > 1)
			{
				rPrintf ("|--------------------------------------------------------------------------------------\n");
				dupesFound = true;
			}
		}
	}
	
	return dupesFound;			
}


//===============================================================================
//	ListUnknownRomsets
//
//	Called by AuditROMs() to print unknown romsets found in the ROMs folder.
//	Returns true if any unknown romsets were found.
//===============================================================================

static Boolean ListUnknownRomsets (void)
{
	Boolean	found = false;
	const ROMSetData *romset;
	
	// loop through gRomsets (contents of ROMs folder)
	for (romset = GetFirstROMSet(); romset != NULL; romset = romset->next)
	{
		unsigned char path[kMacMaxPath];
		
		// unrecognized sets will have a NULL driver
		if (romset->driver == NULL)
		{
			char temp[32];
		
			p2cstrcpy (temp, romset->spec.name);
			
			if (!IsBIOS ((const char *)temp))
			{
				found = true;
				
				// found one -- report it
				(void)GetFullPathFromSpec (&romset->spec, path, sizeof (path));
				rPrintf ("| '%s' :\n", temp, GetIndCString (rStrings, kReportNotDriverName, "is not a recognized driver name"));
				rPrintf ("|    [%s]\n", path);
				rPrintf ("|--------------------------------------------------------------------------------------\n");
			}
		}
	}
	return found;			
}


//===============================================================================
//	AuditROMs
//
//	Performs an audit of the ROMs for every defined game_driver and displays the
//	results in a dialog.
//===============================================================================

void AuditROMs (void)
{
	Str255					defaultName;
	int						index, count, unsorted_game;
	char 					scratch[1000];
	DialogRef				dlog = NULL;
	tAuditOptions			opt;
	char 					filter[256];
	const game_driver **sorted = NULL;
	OSErr					err;
	int						i;
	char					hashBuff[512], hashBuff2[512];

	// read options from global preferences
	opt.sortAlpha = gFEPrefs.audSortAlpha;
	opt.problemsOnly = gFEPrefs.audProblemsOnly;
	opt.listMissing = gFEPrefs.audListMissing;
	opt.suppressWarnings = gFEPrefs.audSuppressWarnings;
	opt.ignoreCHDs = gFEPrefs.audIgnoreCHDs;
	PLstrcpy (opt.filter, gFEPrefs.audFilter);

	// let user specify his preferred audit options
	if (GetAuditOptions (&opt) != noErr)
		return;

	// save options to global preferences
	gFEPrefs.audSortAlpha = opt.sortAlpha;
	gFEPrefs.audProblemsOnly = opt.problemsOnly;
	gFEPrefs.audListMissing = opt.listMissing;
	gFEPrefs.audSuppressWarnings = opt.suppressWarnings;
	gFEPrefs.audIgnoreCHDs = opt.ignoreCHDs;
	PLstrcpy (gFEPrefs.audFilter, opt.filter);
		
	// make a c-string copy of the filter text chosen
	p2cstrcpy (filter, opt.filter);
	
	// get the name of the results dialog and the default file name for the text file
	PLstrcpy (defaultName, GetIndPString (rStrings, kROMAudit, "\pMAME ROM Audit"));	

	// create a dialog and WASTE instance to hold our output
	err = CreateTextResults (defaultName);
	if (err) return;

	// count total number of drivers (for progress info)
	for (index=count=0; drivers[index]; index++)
	{
		// filter driver names with user-specified wildcard filter
		if (strwildcmp (filter, drivers[index]->name)) continue;
		
		count++;
	}

	// show a progress dialog
	{
		Str255	title, msg;
		
		PLstrcpy (title, GetIndPString (rStrings, kROMAuditProgressTitle, "\pAuditing ROMsÉ"));
		PLstrcpy (msg, GetIndPString (rStrings, kAuditProgressMsg, "\pDrivers remaining to audit:"));

		dlog = CreateProgressDialog (title, msg, count);
	}

	// stick a header on top
	rPrintf ("%s: '%s'\n\n", GetIndCString (rStrings, kReportAuditFilter, "Audit for drivers matching filter"), filter);
	rPrintf ("=======================================================================================\n");

	// sort drivers alphabetically if requested, by driver name or description
	if (opt.sortAlpha > 0)
		sorted = GetSortedDriverArray (opt.sortAlpha == 1 ? kSortedByName : kSortedByDescription);
	else
		sorted = drivers;

	// scan through driver[] array of GameDrivers
	for (index = unsorted_game = 0; drivers[unsorted_game]; unsorted_game++)
	{
		audit_record	*aud;
		int				total;
		int				cloneRomsFound = 0;
		const game_driver *drv;
		
		drv = sorted[unsorted_game];

		// filter driver names with user-specified wildcard filter
		if (strwildcmp (filter, drv->name)) continue;

		// update progress bar and driver name
		if (dlog)
		{
			UpdateProgressStatus (dlog, drv->description, index);
		}
		
		// process events relating to progress dialog
		if (HandleProgressEvents (dlog) == userCanceledErr)
		{
			// user stopped audit
			rPrintf (GetIndCString (rErrorStrings, kAuditCanceledError, "**** AUDIT STOPPED AT USER'S REQUEST ****"));
			rPrintf ("\n");
			break;
		}

		// audit the roms for this game (quietly)
		gUnzipQuiet = true;
		total = audit_roms (GetDriverIndex(drv), &aud);
		gUnzipQuiet = false;
		
		// special handling for clones
		if (IsClone(drv))
		{
			// count number of roms found that are unique to clone
			cloneRomsFound = 0;
			for (i = 0; i < total; i++)
				if (aud[i].status != AUD_ROM_NOT_FOUND)
					if (!audit_is_rom_used (drv->clone_of, aud[i].exphash))
						cloneRomsFound++;	

			// setup "clone of" message
			sprintf (scratch, "[clone of '%s']", drv->clone_of->name);
		}
		else
		{
			scratch[0] = '\0';
		}
		
		if (total == 0)
		{
#ifdef MAME_DEBUG
			// romset not found in ROMs folder
			if (opt.listMissing)
			{
				rPrintf ("| %-32.32s| %s  %s\n", drv->name, drv->description, scratch);
				rPrintf ("| %-32.32s| %s\n", "", GetIndCString (rStrings, kReportNoRomsetFound, "No romset found in ROMs folder"));
				rPrintf ("|--------------------------------------------------------------------------------------\n");
			}
#endif
		}
		else if (IsClone(drv) && (!FindROMSetForDriver (drv->name)) && (cloneRomsFound==0))
		{
#ifdef MAME_DEBUG
			// clones are also treated as missing when we only found parent roms
			if (opt.listMissing)
			{
				rPrintf ("| %-32.32s| %s  %s\n", drv->name, drv->description, scratch);
				rPrintf ("| %-32.32s| %s\n", "", GetIndCString (rStrings, kReportNoRomsetFound, "No romset found in ROMs folder"));
				rPrintf ("|--------------------------------------------------------------------------------------\n");
			}
#endif
		}
		else if (total == -1)
		{
			// possibly print something for games with no ROMs?
		}
		else
		{
			// romset wasn't missing, so loop through each ROM that was audited
			int	problems = 0;
			int	divider = 0;
			int biosProblem = 0;
			
			while (total--)
			{
				if ((opt.problemsOnly == false) || ((aud->status != AUD_ROM_GOOD) && (aud->status != AUD_DISK_GOOD)))
				{
					if ((opt.suppressWarnings == false) || (!(aud->status & 
						(AUD_NOT_AVAILABLE|AUD_ROM_NEED_REDUMP|AUD_ROM_NEED_DUMP))))
					{
						if (!(opt.ignoreCHDs && (aud->status & (AUD_DISK_NOT_FOUND|AUD_DISK_BAD_MD5|AUD_DISK_NOT_AVAILABLE|AUD_DISK_NEED_DUMP))))
						{
							// this ROM is going to be listed, so print it's name
							if (divider == 0)
							{
								// first ROM listed for this driver, so print driver name & description
								rPrintf ("| %-8.8s | %20s | %s  %s\n", drv->name, "", drv->description, scratch);
							}
							// list the ROM name
							rPrintf ("| %8s | %-20.20s | ", "", aud->rom);
							divider++;
						}
					}
				}
				
				// print the status of this ROM
				switch (aud->status)
				{
					char temp1[256], temp2[256], temp3[256];
				
					case AUD_ROM_NOT_FOUND:
						strcpy (temp1, GetIndCString (rStrings, kReportROMNotFound, "ROM not found"));
						strcpy (temp2, GetIndCString (rStrings, kReportLength, "Length"));
						hash_data_print (aud->exphash, 0, hashBuff);
						rPrintf ("%s.  %s: 0x%08x  %s\n", temp1, temp2, aud->explength, hashBuff);
						problems++;
						break;
					case AUD_ROM_NOT_FOUND_PARENT:
						strcpy (temp1, GetIndCString (rStrings, kReportROMNotFound, "ROM not found"));
						strcpy (temp2, GetIndCString (rStrings, kReportLength, "Length"));
						hash_data_print (aud->exphash, 0, hashBuff);
						rPrintf ("%s (shared with parent).  %s: 0x%08x  %s\n", temp1, temp2, aud->explength, hashBuff);
						problems++;
						break;
					case AUD_ROM_NOT_FOUND_BIOS:
						strcpy (temp1, GetIndCString (rStrings, kReportROMNotFound, "ROM not found"));
						strcpy (temp2, GetIndCString (rStrings, kReportLength, "Length"));
						hash_data_print (aud->exphash, 0, hashBuff);
						rPrintf ("%s (BIOS).  %s: 0x%08x  %s\n", temp1, temp2, aud->explength, hashBuff);
						problems++;
						biosProblem = 1;
						break;
					case AUD_NOT_AVAILABLE:
						if (!opt.suppressWarnings)
						{
							rPrintf ("%s\n", GetIndCString (rStrings, kReportROMNotFound, "ROM not found"));
							rPrintf ("| %8s | %20s | %s\n", "", "", GetIndCString (rStrings, kReportNoDumpKnown, "NOTE: No valid dump of this ROM is known to exist"));
							problems++;
						}
						break;
					case AUD_BAD_CHECKSUM:
						strcpy (temp1, GetIndCString (rStrings, kReportBadCRC, "Incorrect checksum"));
						strcpy (temp2, GetIndCString (rStrings, kReportExpected, "Expected"));
						strcpy (temp3, GetIndCString (rStrings, kReportFound, "Found"));
						hash_data_print (aud->exphash, 0, hashBuff);
						hash_data_print (aud->hash, 0, hashBuff2);
						rPrintf ("%s. %s: %s  %s: %s\n", temp1, temp2, hashBuff, temp3, hashBuff2);
						problems++;
						break;
					case AUD_ROM_NEED_REDUMP:
					case AUD_ROM_NEED_DUMP:
						if (!opt.suppressWarnings)
						{
							rPrintf ("%s\n", GetIndCString (rStrings, kReportNoChecksum, "Couldn't verify checksum (valid checksum is not known)"));
							problems++;
						}
						break;
					case AUD_MEM_ERROR:
						rPrintf ("%s (%d bytes)\n",
							GetIndCString (rStrings, kReportNoMemory, "Out of memory reading ROM"),
							aud->explength);
						problems++;
						break;
					case AUD_LENGTH_MISMATCH:
						strcpy (temp1, GetIndCString (rStrings, kReportBadLength, "Length mismatch"));
						strcpy (temp2, GetIndCString (rStrings, kReportExpected, "Expected"));
						strcpy (temp3, GetIndCString (rStrings, kReportFound, "Found"));
						rPrintf ("%s. %s: 0x%08x  %s: 0x%08x\n", temp1, temp2, aud->explength, temp3, aud->length);
						problems++;
						break;
					case AUD_ROM_GOOD:
						if (opt.problemsOnly == false)
						{
							strcpy (temp1, GetIndCString (rStrings, kReportGood, "Good"));
							strcpy (temp2, GetIndCString (rStrings, kReportLength, "Length"));
							hash_data_print (aud->hash, 0, hashBuff);
							rPrintf ("%s.  %s: 0x%08x  %s\n", temp1, temp2, aud->length, hashBuff);
						}
						break;				
					case AUD_DISK_GOOD:
						if (opt.problemsOnly == false)
						{
							strcpy (temp1, GetIndCString (rStrings, kReportGood, "Good"));
							hash_data_print (aud->hash, 0, hashBuff);
							rPrintf ("%s.  %s\n", temp1, hashBuff);
						}
						break;				
					case AUD_DISK_NOT_FOUND:
						if (!opt.ignoreCHDs)
						{
							strcpy (temp1, GetIndCString (rStrings, kReportDiskNotFound, "Disk not found"));
							hash_data_print (aud->exphash, 0, hashBuff);
							rPrintf ("%s.  %s\n", temp1, hashBuff);
							problems++;
						}
						break;
					case AUD_DISK_BAD_MD5:
						if (!opt.ignoreCHDs)
						{
							strcpy (temp1, GetIndCString (rStrings, kReportBadCRC, "Incorrect checksum"));
							strcpy (temp2, GetIndCString (rStrings, kReportExpected, "Expected"));
							hash_data_print (aud->exphash, 0, hashBuff);
							hash_data_print (aud->hash, 0, hashBuff2);
							rPrintf ("%s. %s: %s", temp1, temp2, hashBuff);
							strcpy (temp1, GetIndCString (rStrings, kReportFound, "Found"));
							rPrintf (" %s: %s\n", temp1, hashBuff2);
							problems++;
						}
						break;
					default:
						SysBeep (0);
						break;	
				}
				aud++;	// advance to next audited ROM for this driver
			} // end while
			
			if (biosProblem)
				rPrintf ("| %8s | %20s | BIOS : %s\n", "", "", GetBIOSName(drv));

			
			if (divider)
				rPrintf ("|--------------------------------------------------------------------------------------\n");

		}
		index++;	// advance to next game_driver
	}
	
	// tack on a list of duplicate romsets in ROMs folder at bottom of audit
	rPrintf ("\n\n%s...\n", GetIndCString (rStrings, kReportCheckDupes, "Checking for duplicate romsets in ROMs folder"));
	rPrintf ("---------------------------------------------------------------------------------------\n");
	if (!ListDuplicateRomsets (sorted))
	{
		rPrintf ("%s\n", GetIndCString (rStrings, kReportNoDupes, "No duplicates found"));
	}
		
	// tack on a list of unknown romsets in ROMs folder
	rPrintf ("\n\n%s...\n", GetIndCString (rStrings, kReportCheckUnknown, "Checking for unknown romsets in ROMs folder"));
	rPrintf ("---------------------------------------------------------------------------------------\n");
	if (!ListUnknownRomsets ())
	{
		rPrintf ("%s\n", GetIndCString (rStrings, kReportNoUnknowns, "No unknown romsets found"));
	}

	DestroyProgressDialog (dlog);

	// display our results in a dialog box
	DisplayTextResults (defaultName);
}


//===============================================================================
//	DriverHasSamples
//
//	Returns true if the specified driver uses sound samples.
//===============================================================================

Boolean DriverHasSamples(const game_driver *inDrv)
{
	Boolean hasSamples = false;
	int		i;
	
	// Loop over the sound systems and see if one of them is SOUND_SAMPLES
	for (i = 0; i < MAX_SOUND; i++)
	{
		const sound_config *sound;
		machine_config driver;

		expand_machine_driver(inDrv->drv, &driver);
		sound = &driver.sound[i];
		
		if (!sound->sound_type) break;
		
		if (sound->sound_type == SOUND_SAMPLES)
		{
			const char **samplenames;
			
			samplenames = ((struct Samplesinterface *)sound->config)->samplenames;
			
			if ((samplenames != 0) && (samplenames[0] != 0))
				hasSamples = true;
		}
	}
	
	return hasSamples;
}


//===============================================================================
//	AuditSamples
//
//	Performs an audit of the samples for every defined game_driver that needs
//	them and display the results in a dialog.
//===============================================================================

void AuditSamples (void)
{
	Str255		defaultName;
	int			i, count, game;
	DialogRef	dlog;
	OSErr		err;

	// get the name of the dialog and the default file name for the text file
	PLstrcpy (defaultName, GetIndPString (rStrings, kSampleAudit, "\pMAME Sample Audit"));	

	// create a dialog and WASTE/MLTE instance to hold our output
	err = CreateTextResults (defaultName);
	if (err) return;

	// count total number of drivers using samples (for progress info)
	for (i=count=0; drivers[i]; i++)
	{
		if (DriverHasSamples(drivers[i]))
			count++;
	}

	// show a progress dialog
	{
		Str255	title, msg;
		
		PLstrcpy (title, GetIndPString (rStrings, kSampleAuditProgressTitle, "\pAuditing SamplesÉ"));
		PLstrcpy (msg, GetIndPString (rStrings, kAuditProgressMsg, "\pDrivers remaining to audit:"));

		dlog = CreateProgressDialog (title, msg, count);
	}

	// stick a header on top
	rPrintf ("%s:\n\n", GetIndCString (rStrings, kReportSampleProblems, "MAME found problems with samples for the following games"));
	rPrintf ("============================================================\n");
	rPrintf ("| Driver   | %s\n", GetIndCString (rStrings, kReportProblem, "Problem"));
	rPrintf ("============================================================\n");

	// scan through driver[] array
	for (i = game = 0; drivers[game]; game++)
	{
		const game_driver *drv = drivers[game];
		int				missing;
		missing_sample	*aud;
		char			temp[20];			
		
		strcpy (temp, drv->name);
		
		// update progress bar and driver name
		if (dlog)
		{
			if (DriverHasSamples(drv))
			{
				UpdateProgressStatus (dlog, drivers[game]->description, i);
				i++;
			}
		}
		
		// process events relating to progress dialog
		if (HandleProgressEvents (dlog) == userCanceledErr)
		{
			// user stopped audit
			rPrintf (GetIndCString (rErrorStrings, kAuditCanceledError, "**** AUDIT STOPPED AT USER'S REQUEST ****"));
			rPrintf ("\n");
			break;
		}

		gUnzipQuiet = true;
		missing = audit_samples (game, &aud);
		gUnzipQuiet = false;
		
		if (missing != 0)
		{
			if (missing == -1)
				rPrintf ("| %-8s | %s\n", temp, GetIndCString (rStrings, kReportNoSamplesFound, "No samples found for this driver"));
			else
			{
				// list all missing samples for this set
				while (missing--)
				{
					rPrintf ("| %-8s | ", temp);
					temp[0] = '\0'; 	// list driver name only once
					
					rPrintf ("%s: '%s'\n", GetIndCString (rStrings, kReportSampleNotFound, "Sample file not found"), aud->name);
					aud++;
				}
			}
			rPrintf ("|----------|------------------------------------------------\n");
		}
	}

	DestroyProgressDialog (dlog);

	// display our results in a dialog box
	DisplayTextResults (defaultName);
}


#pragma mark -
#pragma mark ¥ Progress Dialog Routines


//===============================================================================
//	SetProgressIndicatorState
//
//	Tells the progress bar control whether it is determinate or indeterminate
//	(barber pole).
//===============================================================================

static void SetProgressIndicatorState (ControlRef inControl, Boolean inIsDeterminate)
{
	OSStatus	err;
	Boolean		state;
	
	if (inControl == NULL) return;

	state = !inIsDeterminate;	
	err = SetControlData (inControl, 0, kControlProgressBarIndeterminateTag, sizeof (state),
			(Ptr)&state);
}


//===============================================================================
//	CreateProgressDialog
//
//	Creates a dialog displaying a progress bar and other status text. Specify the
//	title of the dialog, the status message, and the value that represents 
//	100 percent progress.
//
//	Returns NULL is the dialog couldn't be created, or a pointer to the dialog.
//===============================================================================

DialogRef CreateProgressDialog (ConstStr255Param inTitle, ConstStr255Param inMsg, short inMax)
{
	DialogRef	dlog;
	ControlRef	progress;
	ControlRef	staticText;
	OSErr		err;
	
	// load the progress dialog
	dlog = GetNewDialog (rProgressDialog, NULL, kWindowToFront);
	
	if (dlog)
	{
		// get a handle to the progress bar control
		err = GetDialogItemAsControl (dlog, dProgressIndicator, &progress);
		
		if (noErr == err)
		{
			// set the progress bar control to the "determinate" type
			SetProgressIndicatorState (progress, true);
			
			// set the value representing 100 percent progress as the maximum
			SetControlMaximum (progress, inMax);
		}
			
		// get a handle to the static text remaining count	
		err = GetDialogItemAsControl (dlog, dProgressRemainingText, &staticText);
		
		if (noErr == err)
		{
			ControlFontStyleRec	style;
			
			style.just = teFlushRight;
			style.flags = kControlUseJustMask;
			
			// change the remaining count text to right-justified
			SetControlFontStyle (staticText, &style);
		}
		
		// get a handle to the static text describing the remaining count
		err = GetDialogItemAsControl (dlog, dProgressMsgText, &staticText);
		
		if (noErr == err)
		{
			// change the static text to an appropriate description
			SetDialogItemText ((Handle)staticText, inMsg);
		}
		
		// change the dialog window title and make dialog visible
		SetWTitle (GetDialogWindow(dlog), inTitle);
		ShowWindow (GetDialogWindow(dlog));

#if 0		
		// adjust menus for a movable modal dialog
		DisableMenuItem (GetMenuHandle (rAppleMenu), kAppleAbout);
		DisableMenuItem (GetMenuHandle (rFileMenu), 0);
		DisableMenuItem (GetMenuHandle (rEditMenu), 0);
		DisableMenuItem (GetMenuHandle (rPerformanceMenu), 0);
		DisableMenuItem (GetMenuHandle (rVideoMenu), 0);
		DrawMenuBar ();
#endif
	}
	
	sLastProgressTicks = TickCount();

	return dlog;
}


//===============================================================================
//	UpdateProgressStatus
//
//	Update the progress bar control and static text items in the progress dialog 
//	to reflect the current status. Pass the name of the item currently being 
//	processed and the number of items already processed.
//===============================================================================

void UpdateProgressStatus (DialogRef inDialog, const char *inName, int inProcessed)
{
	ControlRef		progress, staticText;
	SInt32			remaining;
	Str255			text;
	OSErr			err;
	
	if (!inDialog) return;

	// don't update unless it's been 1/10th of a second
	if (TickCount() < sLastProgressTicks + 6)
		return;
	sLastProgressTicks = TickCount();

	// get a handle to the progress bar control
	err = GetDialogItemAsControl (inDialog, dProgressIndicator, &progress);
	
	if (noErr == err)
	{
		// update the control to reflect the number of items already processed
		SetControlValue (progress, inProcessed);

		// get a handle to the static text remaining count	
		err = GetDialogItemAsControl (inDialog, dProgressRemainingText, &staticText);
		
		if (noErr == err)
		{
			// calculate the number of items remaining to process
			remaining = GetControlMaximum (progress) - inProcessed;
			
			// convert the remaining count to a string and change the static text
			NumToString (remaining, text);
			SetDialogItemText ((Handle)staticText, text);
		}
	}
	
	// get a handle to the static text of the current item's name	
	err = GetDialogItemAsControl (inDialog, dProgressCurrentItemText, &staticText);
	
	if (noErr == err)
	{
		// convert the name to a Pascal string and change the static text
		c2pstrcpy (text, inName);
		SetDialogItemText ((Handle)staticText, text);
	}
}


//===============================================================================
//	HandleProgressEvents
//
//	Handle events that occur while a progress dialog is displayed.
//	Treat the dialog as a movable modal dialog.
//
//	Returns noErr, or userCanceledErr if the user clicks the "Stop" button.
//===============================================================================

OSErr HandleProgressEvents (DialogRef inDialog)
{
	EventRecord	evt;
	WindowRef	wp;
	short		item;
	OSErr		err = noErr;

	if (!inDialog) return noErr;
	
	// handle events until event queue is empty
	while (WaitNextEvent (everyEvent, &evt, 0, NULL))
	{
		switch (evt.what)
		{
			case nullEvent:
				// you don't ever get a nullEvent if WaitNextEvent() returns true
				break;
			case mouseDown:
				switch (FindWindow (evt.where, &wp))
				{
					case inMenuBar:
					{
						// handle clicks in menu bar
						(void)MenuSelect (evt.where);
						break;
					}
					case inContent:
						if (wp == GetDialogWindow(inDialog))
						{
							DialogRef dialog;
							// handle clicks within the progress dialog
							if (DialogSelect (&evt, &dialog, &item))
							{
								if (dProgressStopButton == item)
								{
									// user clicked "Stop" button
									err = userCanceledErr;
								}
							}
						}
						else
						{
							// activating any other application window is illegal
							SysBeep (1);
						}
						break;
					case inDrag:
						if (wp == GetDialogWindow (inDialog))
						{
							Rect bounds;
							
							// handle dragging of dialog window
							DragWindow (wp, evt.where, GetRegionBounds(GetGrayRgn(), &bounds));
						}
						else
						{
							// dragging any other application window is illegal
							SysBeep (1);
						}
						break;
				}
				break;
			case keyDown:
			case autoKey:
				if ((evt.modifiers & cmdKey) && (evt.message & charCodeMask) == '.')
				{
					// user pressed Command-period to cancel
					err = userCanceledErr;
				}
				else if ((evt.message & charCodeMask)==0x1B)
				{
					// user pressed escape to cancel
					err = userCanceledErr;
				}
				
				if (userCanceledErr == err)
				{
					// simulate a click of the "Stop" button
					FakeButtonClick (inDialog, dProgressStopButton);
				}
				break;
			case updateEvt:
				if (IsDialogEvent (&evt))
				{
					DialogRef dialog = GetDialogFromWindow((WindowRef)evt.message);
					// Dialog Manager will call BeginUpdate/DrawDialog/EndUpdate
					(void)DialogSelect (&evt, &dialog, &item);
				}
				break;
			case activateEvt:
				if (IsDialogEvent (&evt))
				{
					DialogRef dialog = GetDialogFromWindow((WindowRef)evt.message);
					// Dialog Manager will handle activate/deactivate
					(void)DialogSelect (&evt, &dialog, &item);
				}
				break;
			case osEvt:
				if (((evt.message >> 24) & 0x0ff) == suspendResumeMessage)
				{
					// we have to handle activate/deactivate ourselves
					if (inDialog)
					{
						ControlRef rootControl;
						
						if (noErr == GetRootControl (GetDialogWindow (inDialog), &rootControl))
						{
							if (evt.message & resumeFlag)
							{
								// activate entire contents of dialog
								ActivateControl (rootControl);
							}
							else
							{
								// deactivate entire contents of dialog
								DeactivateControl (rootControl);
							}
						}
					}
				}
				break;
		}	// end switch
	}	// end while
	
	return err;
}


//===============================================================================
//	DestroyProgressDialog
//
//	Close and dispose of the dialog created by CreateProgressDialog().
//
//	Resets menus to previous enabled/disabled state.
//===============================================================================

void DestroyProgressDialog (DialogRef inDialog)
{
	if (inDialog)
	{
		DisposeDialog (inDialog);
	}
}


#pragma mark -
#pragma mark ¥ Analyze Romsets Routines


//===============================================================================
//	GetSpacesString
//
//	Returns a pointer to a C string containing the specified number of spaces.
//===============================================================================

static const char *GetSpacesString (int inNumSpaces)
{
	static char s[64];
	int	i;
	
	if (inNumSpaces > 63) inNumSpaces = 63;
	
	for (i = 0; i < inNumSpaces; i++)
	{
		s[i] = ' ';
	}
	s[i] = '\0';
	return s;
}


//===============================================================================
//	ListChildrenOfDriver
//
//	If driver is a parent (has clones), list each of its children.
//===============================================================================

static void ListChildrenOfDriver (const game_driver *inDrv)
{
	Boolean	once = true;
	int i;
	const game_driver *child;

	// check the clone_of field of every driver
	for (i = 0; drivers[i]; i++)
	{
		child = drivers[i];
		
		if (child->clone_of == inDrv)
		{
			// found a clone of inDrv
			if (once)
			{
				rPrintf (GetIndCString (rStrings, kRA_ParentOfLabel, "Parent of"));
				rPrintf (": %-8.8s  \"%s\"\n", child->name, child->description);
				once = false;
			}
			else
			{
				int	numSpaces;
				
				numSpaces = 2 + strlen (GetIndCString (rStrings, kRA_ParentOfLabel, "Parent of"));
				rPrintf (GetSpacesString (numSpaces));
				rPrintf ("%-8.8s  \"%s\"\n", child->name, child->description);
			}
		}
	}			
}


//===============================================================================
//	IsAlias
//
//	Returns true if inSpec refers to an alias.
//===============================================================================

static Boolean IsAlias (const FSSpec *inSpec)
{
	CInfoPBRec	pb;
	Str63		fName;
	OSErr		err;

	PLstrcpy (fName, inSpec->name);
	pb.hFileInfo.ioNamePtr = fName;
	pb.hFileInfo.ioVRefNum = inSpec->vRefNum;
	pb.hFileInfo.ioFDirIndex = 0;
	pb.hFileInfo.ioDirID = inSpec->parID;
	pb.hFileInfo.ioCompletion = NULL;
	err = PBGetCatInfoSync (&pb);
	
	if (err==noErr)
	{
		// directories can't be aliases
		if (pb.hFileInfo.ioFlAttrib & ioDirMask)
			return false;
		
		if (pb.hFileInfo.ioFlFndrInfo.fdFlags & kIsAlias)
			return true;
	}
	
	return false;
}


//===============================================================================
//	CountItemsInFolder
//
//	Returns the number of files in a folder specified by inSpec.
//	NOTE: Invisible items are included in the count.
//===============================================================================

static short CountItemsInFolder (const FSSpec *inSpec)
{
	CInfoPBRec	pb;
	Str63		fName;
	OSErr		err;
	
	PLstrcpy (fName, inSpec->name);
	pb.hFileInfo.ioNamePtr = fName;
	pb.dirInfo.ioVRefNum = inSpec->vRefNum;
	pb.dirInfo.ioFDirIndex = 0;
	pb.dirInfo.ioDrDirID = inSpec->parID;
	pb.dirInfo.ioCompletion = NULL;
	
	err = PBGetCatInfoSync (&pb);
	if (err)
		return 0;
	else
		return pb.dirInfo.ioDrNmFls;
}


//===============================================================================
//	FolderSpecToDirID
//
//	Given an FSSpec describing a folder, return the folder's dirID.
//===============================================================================

static OSErr FolderSpecToDirID (const FSSpec *inSpec, long *outDirID)
{
	CInfoPBRec	pb;
	Str63		name;
	OSErr		err;
	
	PLstrcpy (name, inSpec->name);
	pb.dirInfo.ioVRefNum = inSpec->vRefNum;
	pb.dirInfo.ioDrDirID = inSpec->parID;
	pb.dirInfo.ioNamePtr = name;
	pb.dirInfo.ioFDirIndex = 0;
	pb.dirInfo.ioCompletion = NULL;

	err = PBGetCatInfoSync (&pb);
	
	if (err == noErr)
		*outDirID = pb.dirInfo.ioDrDirID;

	return err;
}
	

//===============================================================================
//	GetIndexedFileFromFolder
//
//	Returns the name of the n-th file in the folder described by inSpec, or a
//	non-zero error result if no file exists at that index.
//	NOTE: index is zero-based
//===============================================================================

static OSErr GetIndexedFileFromFolder (const FSSpec *inSpec, short inIndex, char *outName)
{
	CInfoPBRec	pb;
	long		dirID;
	int			i, index;
	OSErr		err = noErr;

	// get the dirID of the specified folder
	err = FolderSpecToDirID (inSpec, &dirID);
	if (err)
		return err;

	for (i = 1, index = 0; noErr == err; i++)
	{
		// query File Manager for the file at index	i
		outName[0] = 0;
		pb.hFileInfo.ioNamePtr = (StringPtr)outName;
		pb.hFileInfo.ioVRefNum = inSpec->vRefNum;
		pb.hFileInfo.ioFDirIndex = i;
		pb.hFileInfo.ioDirID = dirID;
		pb.hFileInfo.ioFlAttrib = 0;
		pb.hFileInfo.ioCompletion = NULL;
	
		err = PBGetCatInfoSync (&pb);
		
		if (noErr == err)
		{
			// skip over invisible items
			if (pb.hFileInfo.ioFlFndrInfo.fdFlags & kIsInvisible)
				continue;

			// skip over folders
			if (pb.hFileInfo.ioFlAttrib & ioDirMask)
				continue;

			// found a file -- is it the one?
			if (index == inIndex)
			{
				// yes, this is the requested file
				break;
			}
			else
			{
				// no, bump index
				index++;
			}
		}
	}

	// convert the filename to a C string
	if (noErr == err)
		p2cstrcpy(outName, (StringPtr)outName);

	return err;	
}


//===============================================================================
//	GetIndexedFileFromSuperROM
//
//	Returns the name of the n-th "file" contained in the resource fork of the
//	SuperROM described by inSpec, or a non-zero error result if no file exists
//	at that index.
//	NOTE: index is zero-based
//===============================================================================

/*static OSErr GetIndexedFileFromSuperROM (const FSSpec *inSpec, short inIndex, char *outName)
{
	Handle	rom;
	short	refnum;
	OSErr	err;

	// open the pROM file resource fork
	refnum = FSpOpenResFile (inSpec, fsRdPerm);
	if (refnum == -1)
		return fnfErr;

	// get a handle to the n-th 'ROM ' resource, without actually loading it
	SetResLoad (false);
	rom = Get1IndResource ('ROM ', inIndex + 1);
	SetResLoad (true);
	
	if (!rom)
	{
		// no resource at the specified index
		err = fnfErr;
	}
	else
	{
		short	id;
		ResType	type;
		
		// get the name of the resource, which will be the "file" name
		GetResInfo (rom, &id, &type, (StringPtr)outName);
		err = ResError ();
		if (err == noErr)
			p2cstrcpy(outName, (StringPtr)outName);
		ReleaseResource (rom);
	}
	CloseResFile (refnum);
	return err;
}*/


//===============================================================================
//	GetIndexedFileFromZip
//
//	Returns the name and CRC-32 of the n-th file contained in the zipfile 
//	described by inSpec, or a non-zero error result if no file exists at that index.
//	NOTE: index is zero-based
//===============================================================================

static OSErr GetIndexedFileFromZip (const FSSpec *inSpec, short inIndex, char *outName, 
				UInt32 *outCrc)
{
	UInt8 			path[kMacMaxPath];
	zip_file		*zip = NULL;
	zip_entry	*entry;
	int				i;
	OSErr			err;
		
	// turn the FSSpec into a full path
	err = GetFullPathFromSpec (inSpec, path, sizeof (path));
	if (err)
		return err;

	// open the zipfile directory
	zip = openzip (-1, 0, (char *)path);
	if (!zip)
		return fnfErr;

	// loop through each entry in the zipfile
	for (i = 0; (err == noErr) && (i <= inIndex); /* this space intentionally blank */)
	{
		// get the next entry
		entry = readzip (zip);
		
		if (entry)
		{
			if (entry->name[strlen (entry->name) - 1] == '/')
			{
				// count only files (skip directories)
				continue;	
			}
			else if (inIndex == i)
			{
				// this is the index we're interested in; get filename (sans path)
				const char *base = strrchr (entry->name, '/');
				if (base)
					++base;
				else
					base = entry->name;
				strcpy (outName, base);
				*outCrc = entry->crc32;
			}
		}
		else
		{
			err = fnfErr;
		}
		i++; 	// <-- leave this here so directories won't bump i
	}
	
	closezip (zip);
	return err;
}


//===============================================================================
//	GetIndexedFile
//
//	Returns the name and CRC-32 of the n-th file contained in inRomset, or a 
//	non-zero error result if no file exists at that index.
//	NOTES:	index is zero-based
//			only zipfiles return a correct CRC-32
//===============================================================================

static OSErr GetIndexedFile (const ROMSetData *inRomset, short inIndex, char *outName,
				UInt32 *outCrc)
{
	OSErr	err;
	
	switch (inRomset->format)
	{
		case kROMSetFormatFolder:
			*outCrc = 0;
			err = GetIndexedFileFromFolder (&inRomset->spec, inIndex, outName);
			break;
		case kROMSetFormatZip:
			err = GetIndexedFileFromZip (&inRomset->spec, inIndex, outName, outCrc);
			break;
		default:
			err = paramErr;
	}
	return err;
}


//===============================================================================
//	RomDefinedInDriver
//
//	Returns true if the ROM specified by inName and inCRC is a required ROM for
//	the game_driver specified by inDrv.
//	Pass 0 for inCRC if you don't know the correct checksum.
//===============================================================================

static Boolean RomDefinedInDriver (const game_driver *inDrv, const char *inName, 
					unsigned int inCrc)
{
	const rom_entry *romp = inDrv->rom;
	const rom_entry *region, *rom;

	if (NULL == romp) return false;
	
	for (region = romp; region; region = rom_next_region(region))
	{
		for (rom = rom_first_file(region); rom; rom = rom_next_file(rom))
		{
			const char *name = ROM_GETNAME(rom);
			const char* hash = ROM_GETHASHDATA(rom);
			UINT32 expchecksum;
			GetHashAsUINT32 (hash, &expchecksum);
			
			if (!mame_stricmp (inName, name)) return true;			
			if (inCrc && (expchecksum == inCrc)) return true;
		}
	}
	return false;

}


//===============================================================================
//	RomPreferredName
//
//	Returns the name defined in inDrv->rom for the ROM specified by inCrc, 
//	or the name defined in the first clone game_driver->rom found, or NULL if 
//	the specified ROM isn't found in inDrv->rom or any clone game_driver->rom.
//===============================================================================

static char *RomPreferredName (const game_driver *inDrv, unsigned int inCrc)
{
	static char name[20];
	const rom_entry *romp = inDrv->rom;
	const rom_entry *region, *rom;
	int i;

	if (NULL == romp) return NULL;
	
	// loop through the RomModule of inDrv, looking for inCrc
	for (region = romp; region; region = rom_next_region(region))
	{
		for (rom = rom_first_file(region); rom; rom = rom_next_file(rom))
		{
			const char *exphash = ROM_GETHASHDATA(rom);
			UINT32 expchecksum;
			
			GetHashAsUINT32 (exphash, &expchecksum);
			if (expchecksum == inCrc)
			{
				// found the ROM specified by inCrc -- return it's name
				strcpy (name, ROM_GETNAME(rom));
				return name;
			}
		}
	}

	// couldn't find inCrc in specified game_driver -- look for it in clones (if any)
	for (i = 0; drivers[i]; i++)
	{
		const game_driver *cloneDrv = drivers[i];
		
		if (cloneDrv->clone_of == inDrv)
		{
			romp = cloneDrv->rom;
			
			if (NULL == romp) break;
			
			// loop through the RomModule of cloneDrv, looking for inCrc
			// NOTE: could probably just recurse here, but for safety just duplicate above loop
			for (region = romp; region; region = rom_next_region(region))
			{
				for (rom = rom_first_file(region); rom; rom = rom_next_file(rom))
				{
					const char *exphash = ROM_GETHASHDATA(rom);
					UINT32 expchecksum;
			
					GetHashAsUINT32 (exphash, &expchecksum);
					if (expchecksum == inCrc)
					{
						// found the ROM specified by inCrc -- return it's name
						strcpy (name, ROM_GETNAME(rom));
						return name;
					}
				}
			}
		}
	}

	// couldn't find inCrc in inDrv->rom or any clone game_driver->rom -- signal failure
	return NULL;
}


//===============================================================================
//	RomDefinedInAnyChildDriver
//
//	Returns true if the ROM specified by inName and inCrc is required by any
//	of the clones of the game_driver inDrv. If inListChildren is true, prints each
//	child (clone) for which the ROM is required.
//===============================================================================

static Boolean RomDefinedInAnyChildDriver (const game_driver *inDrv,
		const char *inName, unsigned int inCrc, Boolean inListChildren)
{
	int i;
	const game_driver *child;
	Boolean	defined = false;
	Boolean	once = true;
	
	for (i = 0; drivers[i]; i++)
	{
		child = drivers[i];
		
		if (child->clone_of == inDrv)
		{
			if (RomDefinedInDriver (child, inName, inCrc))
			{
				if (!inListChildren)
				{
					return true;
				}
				else
				{
					defined = true;
					if (once)
					{
						once = false;
						rPrintf (child->name);
					}
					else
						rPrintf (", %s", child->name);
				}
			}
		}
	}
	return defined;		
}


//===============================================================================
//	RomExistsForDriver
//
//	Returns true if the ROM specified by inDrv, inROMName and inROMCrc can be found.
//===============================================================================

static Boolean RomExistsForDriver(const game_driver *inDrv, const char *inROMName, UInt32 inROMCrc)
{
	unsigned int	length;
	char tempHash[32];
	
	sprintf (tempHash, "c:%08x", (unsigned int)inROMCrc);
	// try locating rom by name and crc
	return (mame_fchecksum(inDrv->name, inROMName, &length, tempHash) == 0);
}


//===============================================================================
//	SpecMatchesParentSpec
//
//	Returns true if inDrv is a clone and inResolvedSpec is identical to its
//	parent's FSSpec (i.e., they refer to the same file).
//===============================================================================

static Boolean SpecMatchesParentSpec (const game_driver *inDrv, 
					const FSSpec *inResolvedSpec)
{
	if (IsClone(inDrv))
	{
		const ROMSetData *parent = FindROMSetForDriver (inDrv->clone_of->name);
		
		if (parent)
		{
			if ((inResolvedSpec->vRefNum == parent->spec.vRefNum) &&
				(inResolvedSpec->parID == parent->spec.parID) &&
				EqualString (inResolvedSpec->name, parent->spec.name, false, true))
			{
				// inDrv is a clone and inResolvedSpec matches the parent's FSSpec
				return true;
			}
			
			// parent could be an alias itself -- resolve it and test again
			{
				Boolean	isFolder, wasAliased;
				FSSpec	resolvedParent;
				OSErr	err;
			
				resolvedParent = parent->spec;
				err = ResolveAliasFile (&resolvedParent, true, &isFolder, &wasAliased);
				
				if ((noErr == err) && wasAliased)
				{
					if ((inResolvedSpec->vRefNum == resolvedParent.vRefNum) &&
						(inResolvedSpec->parID == resolvedParent.parID) &&
						EqualString (inResolvedSpec->name, resolvedParent.name, false, true))
					{
						// inResolvedSpec matches the parent's resolved FSSpec
						return true;
					}
				}
			}
		}
	}
	return false;
}


//===============================================================================
//	WordwrapTextBuffer
//
//	Word-wraps the text in the specified buffer to fit in maxwidth characters
//	per line. The contents of the buffer are modified.
//	Known limitations: Words longer than maxwidth cause the function to fail.
//===============================================================================

static void WordwrapTextBuffer (char *inBuffer, int inMaxwidth)
{
	int width = 0;

	while (*inBuffer)
	{
		if (*inBuffer == '\n' || *inBuffer == '\r')
		{
			inBuffer++;
			width = 0;
			continue;
		}

		width++;

		if (width > inMaxwidth)
		{
			// backtrack until a space is found
			while (*inBuffer != ' ')
			{
				inBuffer--;
				width--;
			}
			if (width < 1) return;	// word too long, bail

			// replace space with a newline to cause wrap
			*inBuffer = '\n';
		}
		else
			inBuffer++;
	}
}


//===============================================================================
//	OutputAnalysisNotes
//
//	Outputs canned text explaining the romset analysis. Reads the text from a
//	'TEXT' resource with ID 128 for easy modification/internationalization.
//===============================================================================

static void OutputAnalysisNotes (void)
{
	Handle	notes;
	
	// grab the text from our resource fork
	notes = Get1Resource ('TEXT', 128);

	if (notes)
	{
		char temp[256];
		char *p;
		long length = GetHandleSize (notes);
		
		// overwrite last character of TEXT resource with null terminator
		*((*notes) + length - 1) = '\0';
		
		// wordwrap the text for 70-character lines
		WordwrapTextBuffer (*notes, 70);
		
		// lock the handle down
		HLockHi (notes);
		p = *notes;
		
		// output all of the text in the handle
		while (length > 1)
		{
			strncpy (temp, p, 255);
			temp[255] = '\0';
			rPrintf (temp);
			length -= strlen (temp);
			p += strlen (temp);
		}
		
		// free the resource
		ReleaseResource (notes);			
	}
}


//===============================================================================
//	AnalyzeRomsets
//
//	Performs an analysis of every romset in the user's "ROMs" folder and displays
//	the results. The analysis is useful for merging or unmerging romsets, or
//	removing duplicates.
//===============================================================================

void AnalyzeRomsets (void)
{
	typedef struct { char filename[256]; UInt32 crc; } tRomsetFile;
	Str255		defaultName;
	UInt8 		path[kMacMaxPath];
	DialogRef	dlog;
	int			progress, total;
	const ROMSetData 	*romset;
	tRomsetFile	*fileList = NULL;
	OSErr		err;
	
	// get the name of the dialog and the default file name for the text file
	PLstrcpy (defaultName, GetIndPString (rStrings, kRomsetAnalysis, "\pRomset Analysis"));	

	// create a dialog and WASTE instance to hold our output
	err = CreateTextResults (defaultName);
	if (err) return;
	
	// stick notes on top
	OutputAnalysisNotes ();
		
	// Allocate memory to hold a filename and crc for every file in a romset
	fileList = (tRomsetFile *)malloc (AUD_MAX_ROMS * 2 * sizeof (tRomsetFile));
	if (NULL == fileList)
	{
		printf (GetIndCString (rErrorStrings, kAnalysisMemFullError,
			"Unable to allocate memory for romset analysis!"), err);
		DisposeTextResults ();
		return;
	}

	// get total number of romsets we'll be analyzing
	for (romset = GetFirstROMSet(), total = 0; romset != NULL; romset = romset->next)
	{
		if (romset->driver == NULL) continue; // unrecognized sets will have an index of -2
		
		// also skip ghosts, since they're not real romsets
		if (romset->type == kROMSetTypeGhost) continue;
		
		total++;
	}
		
	// show a progress dialog
	{
		Str255	title, msg;
		
		PLstrcpy (title, GetIndPString (rStrings, kAnalysisProgressTitle, "\pAnalyzing RomsetsÉ"));
		PLstrcpy (msg, GetIndPString (rStrings, kAnalysisProgressMsg, "\pRomsets remaining to analyze:"));

		dlog = CreateProgressDialog (title, msg, total);
	}

	// loop through gRomsets (contents of ROMs folder)
	for (romset = GetFirstROMSet(), progress = 0; romset != NULL; romset = romset->next)
	{
		ROMSetFormat format = romset->format;
		const FSSpec *spec = &romset->spec;
		const game_driver *drv;
		FSSpec	resolvedSpec;
		
		// skip invalid romsets
		if (romset->driver == NULL) continue; // unrecognized sets will have an index of -2
		
		// also skip ghosts, since they're not real romsets
		if (romset->type == kROMSetTypeGhost) continue;
		
		// update progress bar and driver name
		if (dlog)
		{
			UpdateProgressStatus (dlog, romset->driver->description, progress);
			progress++;
		}
		
		// process events
		if (HandleProgressEvents (dlog) == userCanceledErr)
		{
			// detect user canceling audit
			rPrintf (GetIndCString (rErrorStrings, kAnalysisCanceledError,
				"**** ANALYSIS STOPPED AT USER'S REQUEST ****"));
			break;
		}

		drv = romset->driver;

		// list full path to romset	
		(void)GetFullPathFromSpec (spec, path, sizeof (path));
		rPrintf ("Romset: %s\n", path);

		// try to handle aliases intelligently
		if (IsAlias (spec))
		{
			Boolean	isFolder, wasAliased;
			
			resolvedSpec = *spec;
			err = ResolveAliasFile (&resolvedSpec, true, &isFolder, &wasAliased);
			if (err == noErr)
			{
				// did alias resolved to the FSSpec of the parent romset (if any)?
				
				if (!SpecMatchesParentSpec (drv, &resolvedSpec))
				{
					// resolved alias isn't the parent romset, so we'll be analyzing it
					spec = &resolvedSpec;
					(void)GetFullPathFromSpec (spec, path, sizeof (path));
					rPrintf (GetIndCString (rStrings, kRA_AliasLabel, "Alias points to"));
					rPrintf (": %s\n", path);
				}
				else
				{
					// The alias resolved to the parent romset -- no need to analyze it.			
					// Note that 'spec' still references an alias file (important,
					// because we test for this later to detect this situation)
				}
			}
			else
			{
				rPrintf (GetIndCString (rStrings, kRA_CouldntResolve,
					"(Couldn't resolve alias -- original item not found)"));
				rPrintf ("\n");
			}
			
			// NOTE: the value of 'err' here is needed later in this function
		}
		
		// list the format of this romset
		rPrintf (GetIndCString (rStrings, kRA_FormatLabel, "Romset format"));
		rPrintf (": ");
		if (format == kROMSetFormatFolder)
		{
			rPrintf (GetIndCString (rStrings, kRA_FormatFolder, "Folder"));
			rPrintf ("\n");
		}
		else
		{
			rPrintf (GetIndCString (rStrings, kRA_FormatZipfile, "Zipfile"));
			rPrintf ("\n");
		}
		
		// list the driver referenced by this romset
		rPrintf (GetIndCString (rStrings, kRA_DriverLabel, "Driver"));
		rPrintf (": %s  \"%s\"\n", drv->name, drv->description);
		
		// list the driver's parent (if a clone) or clones (if a parent)
		if (IsClone(drv))
		{
			rPrintf (GetIndCString (rStrings, kRA_CloneOfLabel, "Clone of"));
			rPrintf (": %s  \"%s\"\n", drv->clone_of->name, drv->clone_of->description);
		}
		else
		{
			ListChildrenOfDriver (drv);
		}

		// analyze individual roms unless the romset is an empty folder or an alias
		
		if (IsFolder (spec) && CountItemsInFolder (spec) == 0)
		{
			// 'spec' references an empty folder -- don't analyze it
			rPrintf (GetIndCString (rStrings, kRA_EmptyFolder, "Romset Analysis: N/A  (empty folder)"));
			rPrintf ("\n");
		}
		else if (IsAlias (spec))
		{
			// 'spec' references an alias file, therefore the file can only be
			// (1) an alias to its parent romset; or (2) an unresolvable alias
			if (err == noErr)
			{
				// we earlier determined that the alias points to the parent romset
				rPrintf (GetIndCString (rStrings, kRA_AliasToParent, "Romset Analysis: N/A  (alias to parent romset)"));
				rPrintf ("\n");
			}
			else
			{
				// we were unable to resolve the alias earlier
				rPrintf (GetIndCString (rStrings, kRA_Unresolved, "Romset Analysis: N/A  (unresolved alias)"));
				rPrintf ("\n");
			}
		}
		else
		{
			// 'spec' references a valid romset -- analyze every file within it
			int		next = 0;
			int		count = 0;
			
			// determine the filename (and if zipfile, the crc) of every file in this romset
			for (count = 0; count < (AUD_MAX_ROMS * 2); count++)
			{
				tRomsetFile *thisFile = &fileList[count];
				
				err = GetIndexedFile (romset, count, thisFile->filename, &thisFile->crc);
				if (err) break;				
			}
			
			rPrintf (GetIndCString (rStrings, kRA_AnalysisLabel, "Romset Analysis:"));
			rPrintf ("\n");
			
			// loop for each file in this romset
			for (next = 0; next < count; next++)
			{
				UInt32	crc = fileList[next].crc;
				char 	*filename = fileList[next].filename;
				int		matches = 0, which = 0;
				
				// display the name of the file
				rPrintf ("    %-12s  ", filename);
				
				// if crc is known, display it
				if (crc)
				{
					rPrintf ("(%08x)  ", crc);
				}
				
				// check crc (if available) to see if file is duplicated within this romset
				if (crc)
				{
					int i;
					
					// loop through the list of crcs of files in this romset
					for (i = 0; i < count; i++)
					{
						if (fileList[i].crc == crc)
						{
							matches++;
							if (next == i)
							{
								// remember which of the duplicates this is
								which = matches;
							}
						}
					}
				}
				
				// if more than one crc in this romset matches, we have duplicates
				if (matches > 1)
				{
					char *prefname;
					
					// file is duplicated within this romset
					rPrintf (GetIndCString (rStrings, kRA_DupeFileLabel,
						"<duplicate file %d of %d, "), which, matches);
					
					// determine "preferred" name for the ROM
					prefname = RomPreferredName (drv, crc);
					
					if (prefname)
					{
						rPrintf (GetIndCString (rStrings, kRA_DupePrefName, 
							"preferred name: %s>  "), prefname);
					}
					else
					{
						rPrintf (GetIndCString (rStrings, kRA_DupePrefName, 
							"preferred name: %s>  "), "n/a");
					}
					
					if (which > 1)
					{
						// don't need to reanalyze the file if it's just another duplicate,
						// so continue to next file in romset
						rPrintf ("\n");
						continue;
					}
				}			

				// determine if the driver requires this file
				if (!RomDefinedInDriver (drv, filename, crc))
				{
					// driver doesn't require this file, but it might be needed by its clones
					if (RomDefinedInAnyChildDriver (drv, filename, crc, false))
					{
						// this file is needed by one or more clones
						rPrintf (GetIndCString (rStrings, kRA_RqdByCloneOnly,
							"isn't reqd for this driver, but is for clone(s): "));
						(void)RomDefinedInAnyChildDriver (drv, filename, crc, true);
						rPrintf ("\n");
					}
					else
					{
						// the file is not needed by the driver or its clones
						rPrintf (GetIndCString (rStrings, kRA_NotRequiredRom,
							"is not a required ROM for this driver"));
						rPrintf ("\n");
					}
				}
				else
				{
					// this file is required by the romset's driver
					
					// do extra analysis if the romset's driver is a clone
					if (IsClone(drv))
					{
						const game_driver *parentDrv = drv->clone_of;
						const ROMSetData *parent = FindROMSetForDriver (parentDrv->name);
						
						// is the file also required by the parent driver?
						if (RomDefinedInDriver (parentDrv, filename, crc))
						{
							// this file is also required by the parent driver
							rPrintf ("[*] ");
						}
						else
						{
							// this file is required only by the clone driver
							rPrintf ("[ ] ");
						}
					
						// is this file also present in the parent romset?
						if (parent && RomExistsForDriver(parentDrv, filename, crc))
						{
							// file exists in both the clone and parent romsets
							rPrintf (GetIndCString (rStrings, kRA_DupeRomInParent,
								"duplicate exists in parent romset"));
							rPrintf ("\n");
						}
						else
						{
							// suggest that this clone's roms can be moved to parent set
							rPrintf (GetIndCString (rStrings, kRA_CanMoveToParent,
								"can be moved to parent romset"));
							rPrintf ("\n");
						}
					}
					else
					{
						// the romset's driver is not a clone but it may be a parent
						
						// do clones (if any) of this romset also require this file?
						if (RomDefinedInAnyChildDriver (drv, filename, crc, false))
						{
							// list each clone of this romset that requires the file
							rPrintf (GetIndCString (rStrings, kRA_RqdByCloneAlso,
								"is required by this driver and clone(s): "));
							(void)RomDefinedInAnyChildDriver (drv, filename, crc, true);
						}

						rPrintf ("\n");
					}
				} // end else
				// loop to the next file in this romset
			} // end for
		} // end else
		
		// loop to next romset in ROMs folder
		rPrintf ("\n-----------------------------------------------------------------------\n");
	}

	if (fileList)
	{
		free (fileList);
	}	

	DestroyProgressDialog (dlog);
	
	// display our results in a dialog box
	DisplayTextResults (defaultName);
}


#pragma mark -
#pragma mark ¥ Romident routines


// do this so we can use the functions basically as-is from the DOS code
#undef printf
#define printf	rPrintf

/* Identifies a rom from from this checksum */
void identify_rom(const char* name, int checksum)
{
/* Nicola output format */
#if 1
	int found = 0;

	/* remove directory name */
	int i;
	for (i = strlen(name)-1;i >= 0;i--)
	{
		if (name[i] == '/' || name[i] == '\\' || name[i] == ':')////
		{
			i++;
			break;
		}
	}
	////printf("%-12s ",&name[i]);
	printf("%-12s  %08x ",&name[i],checksum);

	for (i = 0; drivers[i]; i++)
	{
		const rom_entry *region, *rom;

		for (region = rom_first_region(drivers[i]); region; region = rom_next_region(region))
			for (rom = rom_first_file(region); rom; rom = rom_next_file(rom))
			{
				const char *hash = ROM_GETHASHDATA(rom);
				UINT32 expchecksum;
				GetHashAsUINT32 (hash, &expchecksum);
				if (checksum == expchecksum)
				{
					if (found != 0)
						////printf("             ");
						printf("                       ");
					////printf("= %-12s  %s\n",ROM_GETNAME(rom),drivers[i]->description);
					printf("= %-12s  %-8s  %s\n",ROM_GETNAME(rom),drivers[i]->name,drivers[i]->description);
					found++;
				}
			}
	}
	if (found == 0)
		////printf("NO MATCH\n");
		printf("  %s\n",
			GetIndCString (rStrings, kIdentNoMatch, "NO MATCH"));
#else
/* New output format */
	int i;
	printf("%s\n",name);

	for (i = 0; drivers[i]; i++) {
		const rom_entry *romp;

		romp = drivers[i]->rom;
		
		if (NULL == romp) break;

		while (romp->name || romp->offset || romp->length)
		{
			if (romp->name && romp->name != (char *)-1 && checksum == romp->crc)
			{
				printf("\t%s/%s %s, %s, %s\n",drivers[i]->name,romp->name,
					drivers[i]->description,
					drivers[i]->manufacturer,
					drivers[i]->year);
			}
			romp++;
		}
	}
#endif
}

/* Identifies a file from from this checksum */
void identify_file(const char* name)
{
	FILE *f;
	int length;
	char* data;

	f = fopen(name,"rb");
	if (!f) {
		return;
	}

	/* determine length of file */
	if (fseek (f, 0L, SEEK_END)!=0)	{
		fclose(f);
		return;
	}

	length = ftell(f);
	if (length == -1L) {
		fclose(f);
		return;
	}

	/* empty file */
	if (!length) {
		fclose(f);
		return;
	}

	/* allocate space for entire file */
	data = (char*)malloc(length);
	if (!data) {
		fclose(f);
		return;
	}

	if (fseek (f, 0L, SEEK_SET)!=0) {
		free(data);
		fclose(f);
		return;
	}

	if (fread(data, 1, length, f) != length) {
		free(data);
		fclose(f);
		return;
	}

	fclose(f);

	identify_rom(name, crc32(0L,(const unsigned char*)data,length));

	free(data);
}

void identify_zip(const char* zipname)
{
	zip_entry* ent;

	zip_file* zip = openzip(-1, 0, zipname);
	if (!zip)
		return;

	while ((ent = readzip(zip)) != 0) {
		/* Skip empty file and directory */
		if (ent->uncompressed_size!=0) {
			char* buf = (char*)malloc(strlen(zipname)+1+strlen(ent->name)+1);
			sprintf(buf,"%s/%s",zipname,ent->name);
			identify_rom(buf,ent->crc32);
			free(buf);
		}
	}

	closezip(zip);
}

#if 0
void romident(const char* name, int enter_dirs);

void identify_dir(const char* dirname)
{
	DIR *dir;
	struct dirent *ent;

	dir = opendir(dirname);
	if (!dir) {
		return;
	}

	ent = readdir(dir);
	while (ent) {
		/* Skip special files */
		if (ent->d_name[0]!='.') {
			char* buf = (char*)malloc(strlen(dirname)+1+strlen(ent->d_name)+1);
			sprintf(buf,"%s/%s",dirname,ent->d_name);
			romident(buf,0);
			free(buf);
		}

		ent = readdir(dir);
	}
	closedir(dir);
}

void romident(const char* name,int enter_dirs) {
	struct stat s;

	if (stat(name,&s) != 0)	{
		printf("%s: %s\n",name,strerror(errno));
		return;
	}

	if (S_ISDIR(s.st_mode)) {
		if (enter_dirs)
			identify_dir(name);
	} else {
		unsigned l = strlen(name);
		if (l>=4 && mame_stricmp(name+l-4,".zip")==0)
			identify_zip(name);
		else
			identify_file(name);
		return;
	}
}

#endif


//===============================================================================
//	DoRomident
//
//	Equivalent of romident() in msdos/fronthlp.c
//===============================================================================

void DoRomident (void)
{
	OSErr err;
	CFURLRef fileURL;

	CFStringRef promptString = CFCopyLocalizedString(CFSTR("PromptToChooseROMToIdent"), NULL);
	if (_NavGetOneFile(promptString, FILETYPE_ROM, &fileURL))
	{
		UInt8 path[kMacMaxPath];

		// turn the selected CFURLRef into a full pathname
		Boolean successful = CFURLGetFileSystemRepresentation(fileURL, true, path, sizeof(path));
		
		if (successful)
		{
			Str255	name;
			
			// get an appropriate title for the results dialog
			PLstrcpy (name, GetIndPString (rStrings, kROMIdentResultsTitle, "\pROM Identification Results"));
			
			// create a dialog and WASTE instance to hold our output
			err = CreateTextResults (name);

			if (noErr == err)
			{
				int len;
				char temp1[256], temp2[256], temp3[256];
				
				// stick a header on top
				strcpy (temp1, GetIndCString (rStrings, kIdentFilename, "Filename"));
				strcpy (temp2, GetIndCString (rStrings, kIdentDriver, "Driver"));
				strcpy (temp3, GetIndCString (rStrings, kIdentDriverDesc, "Driver Description"));
				rPrintf ("%12s      CRC-32     ROM           %8s    %s\n", temp1, temp2, temp3);
				rPrintf ("--------------------------------------------------------------------------------\n");
				
				// decide whether it's a zipfile or individual rom file
				len = strlen ((char *)path);	
				if (len > 4 && mame_stricmp ((char *)path + len - 4, ".zip") == 0)
				{
					// do the actual identification
					identify_zip ((char *)path);
				}
				else
				{
					// do the actual identification
					identify_file ((char *)path);
				}
				
				// get an appropriate default file name
				PLstrcpy (name, GetIndPString (rStrings, kROMIdentDefaultFilename, "\pMAME Identified ROMs"));
							
				// display our results in a dialog box
				DisplayTextResults (name);
			}
		}
	}
}

