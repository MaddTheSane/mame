/*##########################################################################

	maclists.h

	This code concerns itself with maintaining lists for the frontend.

	Written by Aaron Giles

##########################################################################*/

#pragma once

#include <CoreFoundation/CFBase.h>

/*##########################################################################
	CONSTANTS
##########################################################################*/

typedef CF_ENUM(UInt8, ROMSetFormat)
{
	kROMSetFormatFolder,
	kROMSetFormatZip,
	kROMSetFormatGhost
};


typedef CF_ENUM(UInt8, ROMSetType)
{
	kROMSetTypeNormal,
	kROMSetTypeGhost,
	kROMSetTypeOrphan,
	kROMSetTypeConverted
};


typedef CF_ENUM(UInt8, ListNodeType)
{
	kNodeOpenFolder,
	kNodeClosedFolder,
	kNodeROMSet,
	kNodeDummy
};


typedef CF_ENUM(UInt16, GroupSorting)
{
	kGroupByFolder				= 0,
	kGroupByManufacturer,
	kGroupByDate,
	kGroupByFile
};


typedef CF_OPTIONS(UInt16, GroupFlags)
{
	kGroupFlagHideClones		= 0x0001,
	kGroupFlagShowGhosts		= 0x0002,
	kGroupFlagHideNonWorking	= 0x0004,
	kGroupFlagAttachClones		= 0x0008
};


typedef CF_ENUM(UInt8, DriverSortType)
{
	kSortedByName,
	kSortedByDescription,
	kSortedByPointer
};

#define MAC_MAX_BIOS		32					// The size of our hardcoded BIOS ROM array
extern game_driver	*gDriversBIOS[MAC_MAX_BIOS];
extern int					gTotalBIOS;

/*##########################################################################
	TYPEDEFS
##########################################################################*/

// Memory pools are efficient means of allocating many items of the same size
typedef void *MemoryPool;


typedef struct ROMSetData
{
	struct ROMSetData *			next;		// link to the next in the linear list
	struct ROMSetData *			nextoftype;	// link to the next in the linear list for this type
	struct ROMSetFolderData *	parent;		// pointer to our parent folder (or NULL for the root)
	const game_driver *	driver;				// game driver
	FSSpec						spec;		// rom file/folder
	FSRef						ref;		// rom file/folder
	UInt32						sortkey;	// key for sorting
	UInt8						color;		// Finder label color
	ROMSetFormat				format;   	// romset format
	ROMSetType					type;	  	// romset type
} ROMSetData;


typedef struct ListNode
{
	struct ListNode *			next;		// pointer to next node
	ListNodeType				type;		// the type of node
	ROMSetData *				romset;		// pointer to the core ROMSet
	UInt32						nest;		// the nesting level for display
	Str255						description;// long description, as a Pascal string
} ListNode;


/*##########################################################################
	FUNCTION PROTOTYPES
##########################################################################*/

Boolean					UpdateInvalidLists(void);
void 					InvalidateROMSets(void);
void 					InvalidateListGrouping(void);
void 					InvalidateListDescriptions(void);
void 					InvalidateListSorting(void);

UInt32					GetDriverCount(void);
SInt32 					GetDriverIndex(const game_driver *inDriver);
const game_driver		**GetSortedDriverArray(DriverSortType inSortKey);
const game_driver		*FindDriverForFile(ConstStr255Param inFilename, UInt32 *outIndex);

UInt32					GetROMSetCount(void);
const ROMSetData *		GetFirstROMSet(void);
const ROMSetData *		FindROMSetForDriver(const char *inDriverName);
const ROMSetData *		FindROMSetForGameDriver(const game_driver *inDriver);
void					ConvertFileSpecToROMSet(const FSSpec *inSpec, ROMSetData *outRomset);

UInt32					GetListNodeCount(void);
const ListNode *		GetFirstListNode(void);

Boolean					IsValidCategoryFile(const FSSpec *inSpec);

MemoryPool		 		CreateMemoryPool(UInt32 inItemSize, UInt32 inChunkSize);
void 					DisposeMemoryPool(MemoryPool inPool);
void *					AllocateFromPool(MemoryPool inPool);
UInt32 					MemoryPoolItems(MemoryPool inPool);


/*##########################################################################
	GLOBAL VARIABLES
##########################################################################*/

extern const FSSpec sGenreFilespec;
