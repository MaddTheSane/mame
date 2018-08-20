/*##########################################################################

	maclists.h

	This code concerns itself with maintaining lists for the frontend.

	Written by Aaron Giles

##########################################################################*/

#include <Carbon/Carbon.h>

#include <ctype.h>

#include "driver.h"
#include "audit.h"

#include "mac.h"
#include "maclists.h"
#include "macreports.h"
#include "macutils.h"

#ifdef __MWERKS__
#pragma require_prototypes on
#endif

/*##########################################################################
	TYPE DEFINITIONS
##########################################################################*/

typedef struct MemoryPoolData
{
	UInt32						itemSize;
	UInt32						itemsAllocated;
	UInt32						chunkSize;
	UInt32						chunksAllocated;
	void *						firstChunk;
} MemoryPoolData;


typedef struct ROMSetFolderData
{
	struct ROMSetFolderData *	next;		// link to the next in the linear list
	struct ROMSetFolderData *	parent;		// pointer to our parent folder (or NULL for the root)
	struct ROMSetFolderData *	sibling;	// pointer to the next sibling folder
	struct ROMSetFolderData *	child;		// pointer to the first child folder
	FSSpec						spec;		// vref and parent ID, plus the folder name
	FSRef						ref;		// FSRef to folder
	UInt8						color;		// Finder label color
//	Boolean						open;		// true if the folder is open (visible)
} ROMSetFolderData;


typedef struct ListGroupNode
{
	struct ListGroupNode *		next;		// pointer to next node
	ListNodeType				type;		// the type of node
	ROMSetData *				romset;		// pointer to the core ROMSet
	UInt32						nest;		// the nesting level for display
	Str255						description;// long description, as a Pascal string
	UInt32						id[2];		// identifier for linking groups to nodes
} ListGroupNode;


typedef struct CategoryEntry
{
	const char *				driver;		// pointer to the driver name
	const char *				category;	// pointer to the category
	UInt32						sortkey;	// key for sorting
} CategoryEntry;


typedef struct DriverIndexPair
{
	const game_driver *	driver;		// pointer to the driver
	UInt32						index;		// index of this driver
} DriverIndexPair;


/*##########################################################################
	GLOBAL VARIABLES
##########################################################################*/

// flags for indicating that different pieces of the list data are dirty
static Boolean					sROMSetDataInvalid = true;
static Boolean					sListGroupingInvalid = true;
static Boolean					sListDescriptionsInvalid = true;
static Boolean					sListSortingInvalid = true;

// data about the low-level ROMSets
static MemoryPool				sROMSetPool;
static ROMSetData *				sFirstROMSet;
static ROMSetData *				sFirstOrphanROMSet;
static ROMSetData **			sROMSetForDriver;

// data about the folders containing the ROMSets
static MemoryPool				sROMSetFolderPool;
static ROMSetFolderData *		sFirstROMSetFolder;
static ROMSetFolderData *		sRootROMSetFolder;

// data about the list nodes
static MemoryPool				sListNodePool;
static ListNode *				sFirstListNode;

// data about the list group nodes
static MemoryPool				sListGroupNodePool;
static ListGroupNode *			sFirstListGroupNode;

// sorted array of list nodes
static ListNode **				sSortedListNodeArray;

// parsed category file data
static FSSpec					sCategoryFileSpec;
static UInt8 *					sCategoryFileContents;
static CategoryEntry *			sCategoryFileArray;
static UInt32					sCategoryFileArrayEntries;

// some category file-related constants
static const char *				sCategoryKeyString = "// MacMAME Category Sorting File -- this line must be the first line in the file";
static const char *				sUnsortedString = "{ Unsorted }";
const FSSpec					sGenreFilespec = { 0, 0, "\pgenre.txt" };

game_driver				*gDriversBIOS[MAC_MAX_BIOS];
int								gTotalBIOS;


/*##########################################################################
	FUNCTION PROTOTYPES
##########################################################################*/

static int 					CompareDriverNames(const void *inItem1, const void *inItem2);
static int 					CompareDriverDescriptions(const void *inItem1, const void *inItem2);

static int 					CompareDriverIndexPointers(const void *inItem1, const void *inItem2);

static OSErr 				FindAllROMSets(void);
static OSStatus				FindROMFolder (long *dirID,short *vRefNum);
static OSErr 				ScanFolderRecursive(ROMSetFolderData *inParent, UInt32 inDepth);
static ROMSetData *			AddROMSetToList(ROMSetData *inSource);
static ROMSetFolderData *	AddFolderToGroupList(ROMSetData *inSource, SInt32 inParentID);
static void 				AddMissingCloneROMSets(void);

static void					RebuildListNodes(void);
static void 				SiftROMSetsIntoGroups(Boolean inParentsOnly);
static void					AttachClonesToParents(void);
static ListGroupNode *		AddListGroupNode(UInt32 inNest, const char *inName, UInt32 inID0, UInt32 inID1);
static void 				SortGroupList(void);
static int 					CompareGroupDescriptions(const void *inItem1, const void *inItem2);
static UInt32 				MakeStringHash(const char *inString);
static void 				StripString(const char *inName, char *outStripped);

static void 				RebuildListNodesByFolder(void);

static void 				RebuildListNodesByManufacturer(void);

static void 				RebuildListNodesByDate(void);
static void 				StripDate(const char *inName, char *outStripped);

static void 				RebuildListNodesByCategory(const FSSpec *inSpec);
static Boolean 				LoadAndParseTextFile(const FSSpec *inSpec);
static void 				BuildTextFileArray(UInt32 inMaxCount, UInt8 *inEOF);
static void 				IdentifyROMSetCategories(void);
static int 					CompareCategoryEntries(const void *inItem1, const void *inItem2);

static void					RebuildListDescriptions(void);
static void 				SetListNodeDescription(ListNode *inData);

static void 				SortByFolder(void);
static int 					CompareDescriptions(const void *inItem1, const void *inItem2);


/*##########################################################################
	IMPLEMENTATION
##########################################################################*/

#pragma mark ¥ Overall List Management

//===============================================================================
//	UpdateInvalidLists
//
//	Recursively scans the ROMs folder, picking out all the files and folders and
//	identifying them as ROMs.
//===============================================================================

Boolean UpdateInvalidLists(void)
{
	Boolean			updated = false;
	Cursor			arrow;
	
	// if we're going to do something, change the cursor to a watch
	if (sROMSetDataInvalid || sListGroupingInvalid || sListDescriptionsInvalid || sListSortingInvalid)
		if (gWatchCursor)
			SetCursor(*gWatchCursor);
	
	// if the ROMSet data is invalid, rebuild the list
	if (sROMSetDataInvalid)
	{
		FindAllROMSets();
		updated = true;
	}
	
	// if the grouping is invalid, rebuild the list nodes
	if (updated || sListGroupingInvalid)
	{
		RebuildListNodes();
		updated = true;
	}
	
	// if the descriptions are invalid, rebuild the list nodes
	if (updated || sListDescriptionsInvalid)
	{
		RebuildListDescriptions();
		updated = true;
	}
	
	// if the sorting is invalid, resort the list
	if (updated || sListSortingInvalid)
	{
		SortByFolder();
		updated = true;
	}
	
	// if we're going to do something, change the cursor to a watch
	if (updated)
		SetCursor(GetQDGlobalsArrow(&arrow));
	
	return updated;
}


//===============================================================================
//	InvalidateROMSets
//
//	Marks the ROMSets as needing to be rebuilt.
//===============================================================================

void InvalidateROMSets(void)
{
	sROMSetDataInvalid = true;
}


//===============================================================================
//	InvalidateListGrouping
//
//	Marks the list groups as needing to be rebuilt.
//===============================================================================

void InvalidateListGrouping(void)
{
	sListGroupingInvalid = true;
}


//===============================================================================
//	InvalidateListDescriptions
//
//	Marks the list descriptions as needing to be rebuilt.
//===============================================================================

void InvalidateListDescriptions(void)
{
	sListDescriptionsInvalid = true;
}


//===============================================================================
//	InvalidateListSorting
//
//	Marks the list sorting as needing to be redone.
//===============================================================================

void InvalidateListSorting(void)
{
	sListSortingInvalid = true;
}


#pragma mark -
#pragma mark ¥ Driver List Management

//===============================================================================
//	GetDriverCount
//
//	Returns the total number of drivers.
//===============================================================================

UInt32 GetDriverCount(void)
{
	static UInt32					sTotalDrivers;

	// count the drivers if we haven't yet
	if (sTotalDrivers == 0)
		while (drivers[sTotalDrivers] != NULL)
			sTotalDrivers++;
	
	return sTotalDrivers;
}


//===============================================================================
//	GetDriverIndex
//
//	Given a driver, returns the index in the main list.
//===============================================================================

SInt32 GetDriverIndex(const game_driver *inDriver)
{
	static DriverIndexPair *	sSortedDriversByPointer;

	DriverIndexPair *			found;
	DriverIndexPair				comparePair;
	UInt32						driverCount = GetDriverCount();
	UInt32						index;
	
	// if the sorted array doesn't exist, make it
	if (sSortedDriversByPointer == NULL)
	{
		// first make an array of the ROMSet pointers
		sSortedDriversByPointer = malloc(driverCount * sizeof(DriverIndexPair));
		for (index = 0; index < driverCount; index++)
		{
			sSortedDriversByPointer[index].driver = drivers[index];
			sSortedDriversByPointer[index].index = index;
		}
			
		// then quicksort it
		qsort(sSortedDriversByPointer, driverCount, sizeof(DriverIndexPair), CompareDriverIndexPointers);
	}
	
	// binary search the sorted list
	comparePair.driver = inDriver;
	found = bsearch(&comparePair, sSortedDriversByPointer, driverCount, sizeof(DriverIndexPair), CompareDriverIndexPointers);
	
	return found ? found->index : -1;
}

		
//===============================================================================
//	GetSortedDriverArray
//
//	Returns a an array of pointers to drivers, sorted in the desired way.
//===============================================================================

const game_driver **GetSortedDriverArray(DriverSortType inSortKey)
{
	static const game_driver **	sSortedDriversByName;
	static const game_driver **	sSortedDriversByDescription;

	const game_driver ***			arrayBase;
	UInt32								driverCount = GetDriverCount();
	int 								(*sortCallback)(const void *, const void *);
	
	// determine which type of sorting we want
	switch (inSortKey)
	{
		default:
		case kSortedByName:
			arrayBase = &sSortedDriversByName;
			sortCallback = CompareDriverNames;
			break;
			
		case kSortedByDescription:
			arrayBase = &sSortedDriversByDescription;
			sortCallback = CompareDriverDescriptions;
			break;
	}

	// if we've already done it, just return
	if (*arrayBase != NULL)
		return *arrayBase;

	// allocate an array of driver pointers
	*arrayBase = malloc(driverCount * sizeof(const game_driver *));
	
	// fill the array with the original pointers
	memcpy(*arrayBase, drivers, driverCount * sizeof(const game_driver *));

	// quicksort the array
	qsort(*arrayBase, driverCount, sizeof(const game_driver *), sortCallback);

	return *arrayBase;
}


//===============================================================================
//	FindDriverForFile
//
//	Given a filename, finds a driver to match.
//===============================================================================

const game_driver *FindDriverForFile(ConstStr255Param inFilename, UInt32 *outIndex)
{
	const game_driver **	sortedArray = GetSortedDriverArray(kSortedByName);
	const game_driver **	found;
	const game_driver *	driver;
	game_driver 			searchDriver;
	char 						driverName[256];
	UInt32						driverCount = GetDriverCount();
	int 						index;

	// make a C version in lowercase, stopping at the end or at a period (strips off any extension)
	for (index = 0; index < inFilename[0] && inFilename[index + 1] != '.'; index++)
		driverName[index] = tolower(inFilename[index + 1]);
	driverName[index] = 0;
	
	// first find the driver name in the sorted driver list
	searchDriver.name = driverName;
	driver = &searchDriver; 
	found = bsearch(&driver, sortedArray, driverCount, sizeof(const game_driver *), CompareDriverNames);

	// if we didn't find it, tough	
	if (found == NULL)
		return NULL;
		
	// if they want the index, return that
	if (outIndex != NULL)
		*outIndex = GetDriverIndex(*found);
	
	return *found;
}


//===============================================================================
//	CompareDriverNames
//
//	Compare function for qsort() that alphabetically compares the names of
//	two drivers.
//===============================================================================

static int CompareDriverNames(const void *inItem1, const void *inItem2)
{
	const game_driver *drv1 = *(const game_driver **)inItem1;
	const game_driver *drv2 = *(const game_driver **)inItem2;
	return strcmp(drv1->name, drv2->name);
}


//===============================================================================
//	CompareDriverDescriptions
//
//	Compare function for qsort() that alphabetically compares the descriptions of
//	two drivers.
//===============================================================================

static int CompareDriverDescriptions(const void *inItem1, const void *inItem2)
{
	const game_driver *drv1 = *(const game_driver **)inItem1;
	const game_driver *drv2 = *(const game_driver **)inItem2;
	return strcmp(drv1->description, drv2->description);
}


//===============================================================================
//	CompareDriverIndexPointers
//
//	Compare function for qsort() that numerically compares the pointers of
//	the DriverIndexPair pointer to by two ROMSets.
//===============================================================================

static int CompareDriverIndexPointers(const void *inItem1, const void *inItem2)
{
	const DriverIndexPair *drv1 = (const DriverIndexPair *)inItem1;
	const DriverIndexPair *drv2 = (const DriverIndexPair *)inItem2;
	return (drv1->driver < drv2->driver) ? -1 : (drv1->driver > drv2->driver) ? 1 : 0;
}



#pragma mark -
#pragma mark ¥ ROMSet Management

//===============================================================================
//	GetROMSetCount
//
//	Returns the number of ROMSets.
//===============================================================================

UInt32 GetROMSetCount(void)
{
	// if no ROMSets, find them now
	if (sFirstROMSet == NULL)
		UpdateInvalidLists();

	return sROMSetPool ? MemoryPoolItems(sROMSetPool) : 0;
}


//===============================================================================
//	GetFirstROMSet
//
//	Returns a pointer to the first ROMSet, rebuilding if necessary.
//===============================================================================

const ROMSetData *GetFirstROMSet(void)
{
	// if no ROMSets, find them now
	if (sFirstROMSet == NULL)
		UpdateInvalidLists();

	return sFirstROMSet;
}


//===============================================================================
//	FindROMSetForDriver
//
//	Finds and returns a ROMSet for the given driver.
//===============================================================================

const ROMSetData *FindROMSetForDriver(const char *inDriverName)
{
	const game_driver **	sortedArray = GetSortedDriverArray(kSortedByName);
	const game_driver **	found;
	const game_driver *	driver;
	game_driver 			searchDriver;
	ROMSetData *				romset;
	ROMSetData *				last;
	UInt32						driverCount = GetDriverCount();
	
	// first find the driver name in the sorted driver list
	searchDriver.name = inDriverName;
	driver = &searchDriver; 
	found = bsearch(&driver, sortedArray, driverCount, sizeof(const game_driver *), CompareDriverNames);

	// if we found it, return a pointer to the owning ROMSet
	if (found != NULL)
		return FindROMSetForGameDriver(*found);

	// look linearly through the orphan list; hopefully there aren't too many of these guys
	for (romset = sFirstOrphanROMSet, last = NULL; romset != NULL; last = romset, romset = romset->nextoftype)
	{
		char					tempString[256];
		char *					extension;

		// copy the string and strip off the extension
		p2cstrcpy(tempString, romset->spec.name);
		extension = strrchr(tempString, '.');
		if (extension != NULL)
			*extension = 0;
		
		// if we got a match, we're golden
		if (!mame_stricmp(tempString, inDriverName))
		{
			// move this to the head of the list, so that commonly-accessed entries
			// end up at the front
			if (last != NULL)
			{
				last->nextoftype = romset->nextoftype;
				romset->nextoftype = sFirstOrphanROMSet;
				sFirstOrphanROMSet = romset;
			}
			return romset;
		}
	}
	
	// now we've really failed!
	return NULL;
}


//===============================================================================
//	FindROMSetForGameDriver
//
//	Finds and returns a ROMSet for the given driver.
//===============================================================================

const ROMSetData *FindROMSetForGameDriver(const game_driver *inDriver)
{
	SInt32				driverIndex = GetDriverIndex(inDriver);
	
	// if we couldn't find the driver, no need to proceed
	if (driverIndex == -1)
		return NULL;

	// if no ROMSets, force one to be generated
	if (sFirstROMSet == NULL)
		FindAllROMSets();

	// return the ROMSet for this driver index
	return sROMSetForDriver[driverIndex];
}


//===============================================================================
//	ConvertFileSpecToROMSet
//
//	Converts a filespec into a ROMSet.
//===============================================================================

void ConvertFileSpecToROMSet(const FSSpec *inSpec, ROMSetData *outRomset)
{
	outRomset->next 		= NULL;
	outRomset->parent 		= NULL;
	outRomset->driver 		= FindDriverForFile(inSpec->name, NULL);
	outRomset->spec			= *inSpec;
	outRomset->color		= 0;
	outRomset->format		= IsFolder(inSpec) ? kROMSetFormatFolder : kROMSetFormatZip;
	outRomset->type			= kROMSetTypeConverted;
}



#pragma mark -
#pragma mark ¥ List Node Management

//===============================================================================
//	GetListNodeCount
//
//	Returns the number of ListNodes.
//===============================================================================

UInt32 GetListNodeCount(void)
{
	// if no nodes, rebuild
	if (!sSortedListNodeArray)
		UpdateInvalidLists();

	return sListNodePool ? MemoryPoolItems(sListNodePool) : 0;
}


//===============================================================================
//	GetFirstListNode
//
//	Returns a pointer to the first ROMSet, rebuilding if necessary.
//===============================================================================

const ListNode *GetFirstListNode(void)
{
	// if no nodes, rebuild
	if (sFirstROMSet == NULL)
		UpdateInvalidLists();

	return sFirstListNode;
}



#pragma mark -
#pragma mark ¥ ROM Folder Scanning

//===============================================================================
//	FindAllROMSets
//
//	Recursively scans the ROMs folder, picking out all the files and folders and
//	identifying them as ROMs.
//===============================================================================

static OSErr FindAllROMSets(void)
{
	extern game_driver driver_0;

	UInt32				driverCount = GetDriverCount();
	ROMSetData			romset;
	OSStatus			err;
	int					i, j;

	// if we already have pools, free them
	if (sROMSetPool)
		DisposeMemoryPool(sROMSetPool);
	if (sROMSetFolderPool)
		DisposeMemoryPool(sROMSetFolderPool);
	
	// allocate a pool for ROMSets
	sROMSetPool = CreateMemoryPool(sizeof(ROMSetData), 100);
	sFirstROMSet = NULL;
	sFirstOrphanROMSet = NULL;

	// allocate a pool for ROMSetFolders
	sROMSetFolderPool = CreateMemoryPool(sizeof(ROMSetFolderData), 10);
	sFirstROMSetFolder = NULL;
	
	// allocate the ROMSetForDriver array if we haven't yet
	if (sROMSetForDriver == NULL)
		sROMSetForDriver = malloc(driverCount * sizeof(ROMSetData *));
	
	// clear the ROMSetForDriver array
	memset(sROMSetForDriver, 0, driverCount * sizeof(ROMSetData *));
	
	// find the ROM folder and use that as a root group
	err = FindROMFolder(&romset.spec.parID, &romset.spec.vRefNum);
	if (err != noErr) return err;

	// build a ROMSet and allocate it
	romset.parent = NULL;
	romset.color = 0;
	romset.spec.name[0] = 0;
	sRootROMSetFolder = AddFolderToGroupList(&romset, romset.spec.parID);

	// find all romsets, including those in nested subfolders
	err = ScanFolderRecursive(sRootROMSetFolder, 0);
	
	// add any missing clones
	AddMissingCloneROMSets();
	
	// Build a driver list containing all the NOT_A_DRIVER entries
	for (i = 0, gTotalBIOS = 0; drivers[i]; i++)
	{
		if (drivers[i]->clone_of && (drivers[i]->clone_of->flags & NOT_A_DRIVER) && (drivers[i]->clone_of != &driver_0))
		{
			for (j = 0; j < gTotalBIOS; j ++)
			{
				if (gDriversBIOS[j] == drivers[i]->clone_of)
					break;
			}
			if (j == gTotalBIOS)
				gDriversBIOS[gTotalBIOS++] = (game_driver *)drivers[i]->clone_of;
				
			if (gTotalBIOS == MAC_MAX_BIOS)
			{
				printf ("Too many BIOS files found. Increase size of MAC_MAX_BIOS.");
				break;
			}
		}
	}
	
	// mark the user interface list invalid and return the result
	sROMSetDataInvalid = false;
	sListGroupingInvalid = true;
	return err;
}


//===============================================================================
//	FindROMFolder
//
//	Gets the directory ID and volume refNum of the MacMAME ROMs folder.Ä
//===============================================================================

static OSStatus FindROMFolder (long *dirID, short *vRefNum)
{
	OSStatus err = fnfErr;
	
	FSRef folderRef;
	err = GetFSRefForMAMEFolder (FILETYPE_ROM, &folderRef);

	FSCatalogInfo catInfo;
	err = FSGetCatalogInfo (&folderRef, kFSCatInfoVolume | kFSCatInfoNodeID, &catInfo, NULL, NULL, NULL);
	__Require_noErr(err, cantGetCatInfo);
	
	*dirID = catInfo.nodeID;
	*vRefNum = catInfo.volume;

cantGetCatInfo:
	return err;
}

//===============================================================================
//	ScanFolderRecursive
//
//	Scans and processes a single folder, identifying ROMsets.
//===============================================================================

static OSErr ScanFolderRecursive(ROMSetFolderData *inParent, UInt32 inDepth)
{
	Str255			fileName;
	int				index;
	CInfoPBRec		pb;
	SInt16			vRefNum;
	SInt32			parID;
	OSStatus		err = noErr;

	// extract the directory parameters from the parent
	vRefNum = inParent->spec.vRefNum;
	parID = inParent->spec.parID;
	
	// clear the parameter block the first time through
	memset(&pb, 0, sizeof(pb));

	// loop over all files in this folder
	for (index = 1; index < 100000; index++)
	{
		Boolean			wasFolder;
		UInt32			driverIndex;
		ROMSetData		romset;

		// fill in the parameter block
		pb.hFileInfo.ioNamePtr = fileName;
		pb.hFileInfo.ioVRefNum = inParent->spec.vRefNum;
		pb.hFileInfo.ioFDirIndex = index;
		pb.hFileInfo.ioDirID = inParent->spec.parID;
		
		// if we find nothing, break out now
		err = PBGetCatInfoSync(&pb);
		if (err != noErr)
			break;
		
		// skip invisible files and folders completely 
		if (pb.hFileInfo.ioFlFndrInfo.fdFlags & kIsInvisible)
			continue;
		
		// copy in additional data
		romset.parent 	= inParent;
		romset.color 	= (pb.hFileInfo.ioFlFndrInfo.fdFlags >> 1) & 7;
		romset.driver 	= FindDriverForFile(fileName, &driverIndex);
		romset.type		= kROMSetTypeNormal;
		
		// make a filespec out of the result
		FSMakeFSSpec(inParent->spec.vRefNum, inParent->spec.parID, fileName, &romset.spec);
		
		// if we didn't find a driver for this one, try to resolve the alias and look for
		// a driver matching the aliased filename
		if (!romset.driver)
		{
			Boolean			wasAlias;
			FSSpec			aliasSpec = romset.spec;

			ResolveAliasFile(&aliasSpec, true, &wasFolder, &wasAlias);
			if (wasAlias)
			{
				romset.driver = FindDriverForFile(aliasSpec.name, &driverIndex);
				if (romset.driver == NULL && wasFolder)
				{
					// if it was an alias to a non-driver folder, consider it a subfolder
					romset.spec = aliasSpec;
					PLstrcpy(fileName, romset.spec.name);
				}
			}
		}
		
		// otherwise, just see if it's a folder or not
		else
			wasFolder = ((pb.hFileInfo.ioFlAttrib & ioDirMask) != 0);
		
		// if we have a driver, add it to the list and set the type
		// also add non-driver files, just to track them
		if (romset.driver || !wasFolder)
		{
			ROMSetData *allocatedSet;

			// set the format and allocate the ROMSet			
			romset.format = wasFolder ? kROMSetFormatFolder : kROMSetFormatZip;
			allocatedSet = AddROMSetToList(&romset);
			
			// if we had a driver, track it so that we know which clones to add as ghosts
			if (romset.driver)
				sROMSetForDriver[driverIndex] = allocatedSet;
		}

		// if we now have a folder that isn't a driver, consider it a subfolder
		else if (wasFolder && inDepth < 4 && (fileName[1] != '('))
		{
			OSErr err;
			
			// look up the parent ID of this folder
			pb.hFileInfo.ioVRefNum = romset.spec.vRefNum;
			pb.hFileInfo.ioDirID = romset.spec.parID;
			pb.hFileInfo.ioFDirIndex = 0;
			err = PBGetCatInfoSync(&pb);
			
			// add it to the group list and then scan it
			if (err == noErr)
			{
				ROMSetFolderData *folder = AddFolderToGroupList(&romset, pb.hFileInfo.ioDirID);
				err = ScanFolderRecursive(folder, inDepth + 1);
			}
		}
	}
	
	return noErr;
}


//===============================================================================
//	AddROMSetToList
//
//	Adds a ROMSet to the linked master list of sets.
//===============================================================================

static ROMSetData *AddROMSetToList(ROMSetData *inSource)
{
	// allocate from the pool
	ROMSetData *romset = AllocateFromPool(sROMSetPool);
	
	// copy the source data
	*romset = *inSource;
	romset->next = sFirstROMSet;
	sFirstROMSet = romset;
	
	// if this guy doesn't have a driver, make him an orphan and add him to the orphan list
	if (romset->driver == NULL)
	{
		romset->type = kROMSetTypeOrphan;
		romset->nextoftype = sFirstOrphanROMSet;
		sFirstOrphanROMSet = romset;
	}
	
	return romset;
}


//===============================================================================
//	AddFolderToGroupList
//
//	Adds a folder to the linked master list of folders.
//===============================================================================

static ROMSetFolderData *AddFolderToGroupList(ROMSetData *inSource, SInt32 inParentID)
{
	// allocate from the pool
	ROMSetFolderData *folder = AllocateFromPool(sROMSetFolderPool);
	
	// hook us up in the linear list
	folder->next = sFirstROMSetFolder;
	sFirstROMSetFolder = folder;
	
	// fill out our lineage
	folder->parent = inSource->parent;
	folder->sibling = NULL;
	folder->child = NULL;
	if (folder->parent)
	{
		folder->sibling = folder->parent->child;
		folder->parent->child = folder;
	}
	
	// fill in the rest of the data
	folder->spec = inSource->spec;
	folder->spec.parID = inParentID;
	folder->color = inSource->color;
	
	return folder;
}

//===============================================================================
//	AddFolderToGroupList
//
//	Adds a folder to the linked master list of folders.
//===============================================================================

static ROMSetFolderData *AddFolderToGroupList_FSRef(ROMSetData *inSource)
{
	// allocate from the pool
	ROMSetFolderData *folder = AllocateFromPool(sROMSetFolderPool);
	
	// hook us up in the linear list
	folder->next = sFirstROMSetFolder;
	sFirstROMSetFolder = folder;
	
	// fill out our lineage
	folder->parent = inSource->parent;
	folder->sibling = NULL;
	folder->child = NULL;
	if (folder->parent)
	{
		folder->sibling = folder->parent->child;
		folder->parent->child = folder;
	}
	
	// fill in the rest of the data
	folder->ref = inSource->ref;
	folder->color = inSource->color;
	
	return folder;
}

//===============================================================================
//	CountROMs
//
//	returns the total number of ROMs defined by a driver, or 0 if none.
//===============================================================================

static int CountROMs (int index)
{
	const rom_entry *region, *rom, *romp;
	int count = 0;

	romp = drivers[index]->rom;
	
	if (!romp) return 0;
	
	for (region = romp; region; region = rom_next_region(region))
		for (rom = rom_first_file(region); rom; rom = rom_next_file(rom))
			count ++;
	
	return count;
}

//===============================================================================
//	AddMissingCloneROMSets
//
//	Looks for clones we don't own and creates ROMSets for them.
//===============================================================================

static void AddMissingCloneROMSets(void)
{
	UInt32			driverCount = GetDriverCount();
	ROMSetData 		romset;
	int 			index;
	
	// loop over the drivers array, looking for missing entries
	for (index = 0; index < driverCount; index++)
	{
		if (sROMSetForDriver[index] == NULL)
		{
			const game_driver *	parent;
			UInt32						parentIndex = -1;

			// special case: games without a ROM just get added
			if (CountROMs (index) == 0)
			{
				romset.parent	 	= sRootROMSetFolder;
				romset.spec.vRefNum	= 0;
				romset.spec.parID	= 0;
				romset.spec.name[0]	= 0;
				romset.color		= 0;
				romset.format		= kROMSetFormatGhost;
			}
			
			// otherwise, check for the parent's existence
			else
			{
				// see if we own the parent
				for (parent = drivers[index]->clone_of; parent != NULL; parent = parent->clone_of)
				{
					parentIndex = GetDriverIndex(parent);
					if (parentIndex == -1 || sROMSetForDriver[parentIndex] != NULL)
						break;
				}
				
				// if not, continue
				if (parent == NULL || parentIndex == -1)
					continue;

// LBO 5/3/05. Disabled. For users with fully-merged ROMs, this was a performance killer.
// Also, one user reported that with a fully-merged set, galpanib was missing. It shares
// all its roms with the parent and a sibling. More trouble to fix than it's worth for now.
#if 0
				// If we don't have any ROMs that are unique to this clone set, don't
				// add it.
				if (RomsetMissing (index))
					continue;
#endif
				
				// make a copy of the parent ROMSet
				romset = *sROMSetForDriver[parentIndex];
			}
			
			// but point it to the clone
			romset.driver = drivers[index];
			romset.type = kROMSetTypeGhost;
			
			// add it to the list
			sROMSetForDriver[index] = AddROMSetToList(&romset);
		}
	}
}


#pragma mark -
#pragma mark ¥ Group-Building Exercises

//===============================================================================
//	RebuildListNodes
//
//	Rebuilds the set of list nodes.
//===============================================================================

static void RebuildListNodes(void)
{
	FSSpec		tempFilespec;
	
	// if we already have pools, free them
	if (sListNodePool)
		DisposeMemoryPool(sListNodePool);
	if (sListGroupNodePool)
		DisposeMemoryPool(sListGroupNodePool);
	
	// allocate a pool for ListNodes
	sListNodePool = CreateMemoryPool(sizeof(ListNode), 100);
	sFirstListNode = NULL;

	// allocate a pool for ListGroupNodes
	sListGroupNodePool = CreateMemoryPool(sizeof(ListGroupNode), 10);
	sFirstListGroupNode = NULL;
	
	// switch off the type of grouping
	switch (gFEPrefs.grouping)
	{
		default:
		case kGroupByFolder:
			RebuildListNodesByFolder();
			break;
			
		case kGroupByManufacturer:
			RebuildListNodesByManufacturer();
			break;
	
		case kGroupByDate:
			RebuildListNodesByDate();
			break;
	
		case kGroupByFile:
			tempFilespec = gFEPrefs.groupFilespec;
			RebuildListNodesByCategory(&tempFilespec);
			break;
	}
	
#if 0
	// if we're not attaching parents, just sift normally
	if (!(gPrefs.groupFlags & kGroupFlagAttachClones))
		SiftROMSetsIntoGroups(false);

	// otherwise, just sift the parents, then attach the clones
	else
#endif
	{
		SiftROMSetsIntoGroups(true);
		
		// sort by descriptions now; just the parents
		RebuildListDescriptions();
		SortByFolder();
		
		// then attach the clones underneath them
		AttachClonesToParents();
	}

	// mark the grouping valid and the sorting invalid
	sListGroupingInvalid = false;
	sListDescriptionsInvalid = true;
}


//===============================================================================
//	AddListGroupNode
//
//	Adds a node to the list groups.
//===============================================================================

static ListGroupNode *AddListGroupNode(UInt32 inNest, const char *inName, UInt32 inID0, UInt32 inID1)
{
	ListGroupNode *group = AllocateFromPool(sListGroupNodePool);
	CFStringRef tempCFString;
	Boolean tempBool = true;
	
	// add it to the group list
	group->next = sFirstListGroupNode;
	sFirstListGroupNode = group;
	
	// fill in the data for this group item
//	group->type = kNodeOpenFolder;
	group->romset = NULL;
	group->nest = inNest;
	c2pstrcpy(group->description, inName);
	
	// zero-terminate the pascal string for speed
	group->description[group->description[0] + 1] = 0;
	
	// determine the ID
	group->id[0] = inID0;
	group->id[1] = inID1;

	// determine if we're open or not
	tempCFString = CFStringCreateWithCString (kCFAllocatorDefault, inName, kCFStringEncodingMacRoman);
	if (tempCFString)
	{
		SetPrefID_FE ();
		GetPrefAsBoolean (&tempBool, tempCFString, true);
		CFRelease (tempCFString);
	}
	
	if (tempBool)
		group->type = kNodeOpenFolder;
	else
		group->type = kNodeClosedFolder;
		
	return group;
}


//===============================================================================
//	SortGroupList
//
//	Sorts the list of groups alphabetically be description.
//===============================================================================

static void SortGroupList(void)
{
	UInt32					nodeCount = MemoryPoolItems(sListGroupNodePool);
	ListGroupNode **		indexList;
	ListGroupNode *			node;
	UInt32					index;
	
	if (nodeCount) {	// avoid crash when zero roms.
	// allocate a temporary index list
	indexList = malloc(nodeCount * sizeof(ListGroupNode *));
	
	// add all the group nodes to the list
	for (node = sFirstListGroupNode, index = 0; node != NULL; node = node->next, index++)
		indexList[index] = node;
	
	// now qsort them
	qsort(indexList, nodeCount, sizeof(indexList[0]), CompareGroupDescriptions);
	
	// then recreate the linked list
	sFirstListGroupNode = indexList[0];
	for (index = 1; index < nodeCount; index++)
		indexList[index - 1]->next = indexList[index];
	indexList[nodeCount - 1]->next = NULL;
	
	// free the temporary memory
	free(indexList);
}
}


//===============================================================================
//	SiftROMSetsIntoGroups
//
//	Scans through the group list and adds ROMSets in batches.
//===============================================================================

static void SiftROMSetsIntoGroups(Boolean inParentsOnly)
{
	ListNode *			last = NULL;
	ListGroupNode *		group;

	// now build the list in order
	for (group = sFirstListGroupNode; group != NULL; group = group->next)
	{
		ListNode *			parent = NULL;
		ROMSetData *		romset;

		// if this is any folder other than the root, add it as its own node
		if (parent == NULL && group->nest != 0)
		{
			// allocate a new node
			parent = AllocateFromPool(sListNodePool);
			if (last)
				last->next = parent;
			else
				sFirstListNode = parent;
			last = parent;
			
			// fill it in
			parent->next = NULL;
			parent->type = group->type;
			parent->romset = NULL;
			parent->nest = group->nest - 1;
			PLstrcpy(parent->description, group->description);
		}
		
		// now add all the ROMSets owned by this group
		// at this point we filter out all the non-ROMsets
		for (romset = sFirstROMSet; romset != NULL; romset = romset->next)
			if (romset->driver != NULL &&
				((group->id[0] == romset->sortkey) || (romset->sortkey == 0 && romset->format == kROMSetFormatGhost && group == sFirstListGroupNode)))
			{
				ListNode *			node;
				
				// Is this a clone ROM for which we don't have the parent ROM set?
				if (inParentsOnly && IsClone (romset->driver))
				{
					const game_driver *parentDriver;
					SInt32 parentIndex = -1;
					
					// see if we own the parent
					for (parentDriver = romset->driver->clone_of; parentDriver != NULL; parentDriver = parentDriver->clone_of)
					{
						parentIndex = GetDriverIndex (parentDriver);
						if (parentIndex == -1 || sROMSetForDriver[parentIndex] != NULL)
							break;
					}

					// Found a parent, so don't add this ROM to the list yet. it'll get added later on when
					// we attach clones to parents.
					if (parentDriver != NULL && parentIndex != -1)
						continue;
						
					// If we get here, we found a clone for which we didn't have a parent ROM set.
					// In this event, we'll go ahead and add it to the list now as a standalone entry.
				}
				
				// allocate a new node
				node = AllocateFromPool(sListNodePool);
				if (last)
					last->next = node;
				else
					sFirstListNode = node;
				last = node;
				
				// fill it in; descriptions are generated later
				node->next = NULL;
				node->type = kNodeROMSet;
				node->romset = romset;
				node->nest = parent ? parent->nest + 1 : 0;
			}
	}
}


//===============================================================================
//	AttachClonesToParents
//
//	Loops through all drivers, added nodes for clones underneath their parents.
//===============================================================================

static void AttachClonesToParents(void)
{
	ROMSetData *					romset;
	
	// loop over ROMSets, looking for clone entries
	for (romset = sFirstROMSet; romset != NULL; romset = romset->next)
		if (romset->driver != NULL && IsClone(romset->driver))
		{
			const game_driver *	parent;
			UInt32						parentIndex;
			ListNode *					node;
			ListNode *					parentNode;

			node = NULL;

			// see if we own the parent
			for (parent = romset->driver->clone_of; parent != NULL; parent = parent->clone_of)
			{
				parentIndex = GetDriverIndex(parent);
				if (parentIndex == -1 || sROMSetForDriver[parentIndex] != NULL)
					break;
			}
			
			// now find the parent's list node
			for (parentNode = sFirstListNode; parentNode != NULL; parentNode = parentNode->next)
				if (parentNode->romset && (parentNode->romset->driver == parent))
					break;
			
			// If parent not found, continue
			if ((parentNode == NULL) || (parentNode->romset == NULL))
			{
				continue;
			}
			else
			{	
				// allocate a new node
				node = AllocateFromPool(sListNodePool);
				node->type = kNodeROMSet;
				node->romset = romset;

				// hook in it after the parent
				node->next = parentNode->next;
				parentNode->next = node;
			
				// fill it in; descriptions are generated later
				node->nest = parentNode->nest + 1;
			}
		}
}


//===============================================================================
//	CompareGroupDescriptions
//
//	qsort compare function that just compares descriptions from the group nodes.
//===============================================================================

static int CompareGroupDescriptions(const void *inItem1, const void *inItem2)
{
	ListGroupNode *data1 = *(ListGroupNode **)inItem1;
	ListGroupNode *data2 = *(ListGroupNode **)inItem2;
	return RelString(data1->description, data2->description, false, true);
}


//===============================================================================
//	MakeStringHash
//
//	Makes a 32-bit string hash.
//===============================================================================

static UInt32 MakeStringHash(const char *inString)
{
	UInt32		hash = strlen(inString) + 1;
	
	// loop until done
	while (*inString)
	{
		hash = __rlwinm(hash, 3, 0, 31) ^ *inString;
		inString++;
	}
	
	// return the hash
	return hash;
}


//===============================================================================
//	StripString
//
//	Attempts to semi-intelligently strip parentheticals and leading/trailing
//	spaces from a string.
//===============================================================================

static void StripString(const char *inName, char *outStripped)
{
	Boolean			parenthetical = false;
	char			closeParen = '\0';
	const char *	src;
	char *			dst;
	char			c;
	
	// first remove all parenthetical expressions
	src = inName;
	dst = outStripped;
	while ((c = *src++) != 0)
	{
		// look for an open paren
		if (!parenthetical)
		{
			if (c == '(')
				parenthetical = true, closeParen = ')';
			else if (c == '{')
				parenthetical = true, closeParen = '}';
			else if (c == '[')
				parenthetical = true, closeParen = ']';
			else
				*dst++ = c;
		}
		
		// look for a close paren
		else if (c == closeParen)
			parenthetical = false;
	}
	*dst++ = 0;
	
	// now strip all leading spaces
	src = dst = outStripped;
	while (*src != 0 && isspace(*src))
		src++;
	if (src != dst)
		memmove(dst, src, strlen(src) + 1);
	
	// now strip all trailing spaces
	dst = outStripped + strlen(outStripped) - 1;
	while (dst >= outStripped && isspace(*dst))
		*dst-- = 0;
	
	// special case: if we deleted the whole thing, put it back
	if (outStripped[0] == 0)
		strcpy(outStripped, inName);
}


#pragma mark -
#pragma mark ¥ Simple Sorts

//===============================================================================
//	RebuildListNodesByFolder
//
//	Rebuilds the set of list nodes, sorted hierarchically by folder.
//===============================================================================

static void RebuildListNodesByFolder(void)
{
	//ListNode *			last = NULL;
	ROMSetFolderData *	folder;
	ROMSetFolderData *	parent;
	ROMSetData *		romset;
	ListGroupNode *		group;
	
	// first create the group hierarchy; we assume that the folder list was built backwards,
	// so that by parsing it backwards, we get the original folder list in order
	for (folder = sFirstROMSetFolder; folder != NULL; folder = folder->next)
	{
		char		cName[256];
		UInt32		nest = 0;

		// make a C string from the folder name
		p2cstrcpy(cName, folder->spec.name);

		// determine the nesting
		for (parent = folder->parent; parent != NULL; parent = parent->parent)
			nest++;
		
		// add the group node
		group = AddListGroupNode(nest, cName, (folder->spec.vRefNum << 16) ^ folder->spec.parID, 0);
		
		// if the folder is closed, close it
//		if (!folder->open)
//			group->type = kNodeClosedFolder;
	}
	
	// loop over ROMSets and compute the ID
	for (romset = sFirstROMSet; romset != NULL; romset = romset->next)
		romset->sortkey = (romset->spec.vRefNum << 16) ^ romset->spec.parID;
}


//===============================================================================
//	RebuildListNodesByManufacturer
//
//	Rebuilds the set of list nodes, sorted by manufacturer.
//===============================================================================

static void RebuildListNodesByManufacturer(void)
{
	UInt32				currentID = 1;
	//ListNode *			last = NULL;
	ROMSetData *		romset;
	ListGroupNode *		group;
	
	// loop over all drivers, hashing manufacturers names
	for (romset = sFirstROMSet; romset != NULL; romset = romset->next)
	{
		char			stripped[256];
		UInt32			hash;
		
		// only process those with drivers
		if (romset->driver == NULL)
			continue;
		
		// strip the name and hash it
		StripString(romset->driver->manufacturer, stripped);
		hash = MakeStringHash(stripped);

		// see if we hit this one already
		for (group = sFirstListGroupNode; group != NULL; group = group->next)
			if (group->id[1] == hash && !mame_stricmp((char *)&group->description[1], stripped))
				break;
		
		// if we didn't find it, add a new group
		if (group == NULL)
			group = AddListGroupNode(1, stripped, currentID++, hash);
		
		// set the ROMSet's sortkey to the group ID
		romset->sortkey = group->id[0];
	}

	// sort the list of groups we just produced
	SortGroupList();
}


//===============================================================================
//	RebuildListNodesByDate
//
//	Rebuilds the set of list nodes, sorted by date.
//===============================================================================

static void RebuildListNodesByDate(void)
{
	UInt32				currentID = 1;
	//ListNode *			last = NULL;
	ROMSetData *		romset;
	ListGroupNode *		group;
	
	// loop over all drivers, hashing manufacturers names
	for (romset = sFirstROMSet; romset != NULL; romset = romset->next)
	{
		char			stripped[256];
		UInt32			hash;
		
		// only process those with drivers
		if (romset->driver == NULL)
			continue;
		
		// strip the name and hash it
		StripDate(romset->driver->year, stripped);
		hash = MakeStringHash(stripped);

		// see if we hit this one already
		for (group = sFirstListGroupNode; group != NULL; group = group->next)
			if (group->id[1] == hash && !mame_stricmp((char *)&group->description[1], stripped))
				break;
		
		// if we didn't find it, add a new group
		if (group == NULL)
			group = AddListGroupNode(1, stripped, currentID++, hash);
		
		// set the ROMSet's sortkey to the group ID
		romset->sortkey = group->id[0];
	}

	// sort the list of groups we just produced
	SortGroupList();
}


//===============================================================================
//	StripDate
//
//	Attempts to semi-intelligently strip the crap from a manufacturer name.
//===============================================================================

static void StripDate(const char *inName, char *outStripped)
{
	// first strip the string
	StripString(inName, outStripped);
	
	// now fix 1984?-type dates
	if (outStripped[4] == '?')
		outStripped[4] = 0;
}


#pragma mark -
#pragma mark ¥ Sort by File

//===============================================================================
//	RebuildListNodesByCategory
//
//	Rebuilds the set of list nodes, sorted by category.
//===============================================================================

static void RebuildListNodesByCategory(const FSSpec *inSpec)
{
	UInt32				currentID = 0;
	CategoryEntry *		entry;
	//ListNode *			last = NULL;
	ListGroupNode *		group;
	UInt32				index;
	
	// if we weren't able to parse the text file, fall back to a manufacturer sort
	if (!LoadAndParseTextFile(inSpec))
	{
		RebuildListNodesByFolder();
		return;
	}
	
	// create an unsorted group
	group = AddListGroupNode(1, sUnsortedString, currentID++, MakeStringHash(sUnsortedString));
//	group->type = kNodeClosedFolder;

	// loop over all groupings
	for (index = 0, entry = sCategoryFileArray; index < sCategoryFileArrayEntries; index++, entry++)
	{
		UInt32			hash;
		
		// strip the name and hash it
		hash = MakeStringHash(entry->category);

		// see if we hit this one already
		for (group = sFirstListGroupNode; group != NULL; group = group->next)
			if (group->id[1] == hash && !mame_stricmp((char *)&group->description[1], entry->category))
				break;
		
		// if we didn't find it, add a new group
		if (group == NULL)
			group = AddListGroupNode(1, entry->category, currentID++, hash);
		
		// set the category's sortkey to the group ID
		entry->sortkey = group->id[0];
	}

#if 0
	// close folders ending in "-" (RS 010330)
	for (group = sFirstListGroupNode; group != NULL; group = group->next)
		if (group->description[group->description[0]] == '-')
		{
			group->type = kNodeClosedFolder;
			group->description[group->description[0]] = 0;
		}
#endif

	// sort the list of groups we just produced
	SortGroupList();
	
	// determine the sortkeys for all ROMSets
	IdentifyROMSetCategories();
}


//===============================================================================
//	LoadAndParseTextFile
//
//	Loads a text file and parses it to extract the driver information.
//===============================================================================

static Boolean LoadAndParseTextFile(const FSSpec *inSpec)
{
	UInt32			count, eolCount;
	const UInt8 *	data;
	SInt32			size;
	SInt16			file;
	OSErr			err;
	
	// if this is the same as the last file we did, just return true
	if (inSpec->vRefNum == sCategoryFileSpec.vRefNum &&
		inSpec->parID == sCategoryFileSpec.parID &&
		EqualString(inSpec->name, sCategoryFileSpec.name, false, true))
		return true;
	sCategoryFileSpec = *inSpec;

	// free any existing data
	if (sCategoryFileContents != NULL)
		free(sCategoryFileContents);
	sCategoryFileContents = NULL;
	if (sCategoryFileArray != NULL)
		free(sCategoryFileArray);
	sCategoryFileArray = NULL;

	// open the file
	err = FSpOpenDF(inSpec, fsRdPerm, &file);
	if (err != noErr)
		return false;
	
	// determine the length
	err = GetEOF(file, &size);
	if (err != noErr)
		goto eofFailed;
	
	// allocate memory and read in the data
	sCategoryFileContents = malloc(size);
	err = FSRead(file, &size, sCategoryFileContents);
	if (err != noErr)
		goto readFailed;
	
	// close the file
	FSClose(file);
	
	// count end-of-lines to determine the number of array elements to allocate
	eolCount = 0;
	for (count = 1, data = &sCategoryFileContents[1]; count < size; count++, data++)
		if (data[0] == 13 || (data[0] == 10 && data[-1] != 13))
			eolCount++;
	
	// allocate them
	sCategoryFileArray = malloc(eolCount * sizeof(CategoryEntry));
	
	// build the array
	BuildTextFileArray(eolCount, &sCategoryFileContents[size]);
	return true;
	
readFailed:
	free(sCategoryFileContents);
	sCategoryFileContents = NULL;
	
eofFailed:
	FSClose(file);
	return false;
}
	

//===============================================================================
//	BuildTextFileArray
//
//	Parses the loaded text file and extracts an array of file group entries
//	from it.
//===============================================================================

static void BuildTextFileArray(UInt32 inMaxCount, UInt8 *inEOF)
{
	char *			lastCategory = NULL;
	Boolean			isSpace[256];
	Boolean			isEOL[256];
	CategoryEntry *	entry;
	UInt8 *			data;

	// make a whitespace and EOL array that is logical
	memset(isSpace, 0, sizeof(isSpace));
	memset(isEOL, 0, sizeof(isSpace));
	isSpace[9] = isSpace[32] = 1;
	isEOL[10] = isEOL[13] = 1;
	
	// now parse the file
	data = (UInt8 *)sCategoryFileContents;
	entry = sCategoryFileArray;
	sCategoryFileArrayEntries = 0;
	while (data < inEOF)
	{
		Boolean		overwroteEOL = false;
		
		// skip any spaces
		while (isSpace[*data])
			if (++data >= inEOF) goto hitEOF;
		
		// if we didn't hit an EOL, and this isn't a comment line, we can proceed
		if (!isEOL[*data] && (data[0] != '/' || data[1] != '/'))
		{
			// assume this is the start of a driver name
			entry->driver = (char *)data;
			
			// look for spaces or EOL
			while (!(isSpace[*data] | isEOL[*data]))
				if (++data >= inEOF) goto hitEOF;
			
			// if that's a valid length for a driver name (<=8 chars, proceed)
			if ((char *)data - entry->driver <= 8)
			{
				// zero-terminate the string
				overwroteEOL = isEOL[*data];
				*data++ = 0;
				
				// skip any spaces
				while (isSpace[*data])
					if (++data >= inEOF) goto hitEOF;
				
				// if we didn't hit an EOL, we can proceed
				if (!isEOL[*data] && !overwroteEOL)
				{
					// assume this is the start of a description
					entry->category = lastCategory = (char *)data;

					// look for EOL
					while (!isEOL[*data])
						if (++data >= inEOF) goto hitEOF;
						
					// back up until we hit a non-space
					while (isSpace[data[-1]])
						data--;
					
					// zero-terminate and count this entry
					overwroteEOL = isEOL[*data];
					*data++ = 0;
					entry++;
					sCategoryFileArrayEntries++;
				}
				
				// otherwise, use the previous category
				else if (lastCategory != NULL)
				{
					entry->category = lastCategory;
					entry++;
					sCategoryFileArrayEntries++;
				}

				// check for overflow (shouldn't happen)
				if (sCategoryFileArrayEntries >= inMaxCount)
					goto hitEOF;
			}
		}
		

		// advance to the EOL if we're not there already
		if (!overwroteEOL)
			while (!isEOL[*data])
				if (++data >= inEOF) goto hitEOF;
		
		// skip any EOL characters
		while (isEOL[*data])
			if (++data >= inEOF) goto hitEOF;
	}

hitEOF:
	// quicksort the array
	if (sCategoryFileArrayEntries > 1)
		qsort(sCategoryFileArray, sCategoryFileArrayEntries, sizeof(sCategoryFileArray[0]), CompareCategoryEntries);
}


//===============================================================================
//	IdentifyROMSetCategories
//
//	Scans all ROMSets and puts them into categories via their sortkeys.
//===============================================================================

static void IdentifyROMSetCategories(void)
{
	ROMSetData *		romset;
	
	// loop over ROMSets and put a pointer to the category record in the sortkey
	for (romset = sFirstROMSet; romset != NULL; romset = romset->next)
	{
		const game_driver *	driver = romset->driver;
		CategoryEntry				searchEntry;
		CategoryEntry *				category;

		// reset the sortkey
		romset->sortkey = 0;

		// loop up the tree until we get a hit
		while (driver != NULL && romset->sortkey == 0)
		{
			// binary search for this driver name
			searchEntry.driver = driver->name;
			category = bsearch(&searchEntry, sCategoryFileArray, sCategoryFileArrayEntries, sizeof(sCategoryFileArray[0]), CompareCategoryEntries);

			// if we got it use that as the sortkey
			if (category != NULL)
				romset->sortkey = category->sortkey;
			driver = IsClone(driver) ? driver->clone_of : NULL;
		}
	}
}	


//===============================================================================
//	CompareCategoryEntries
//
//	qsort compare function that just compares driver names from two file groups.
//===============================================================================

static int CompareCategoryEntries(const void *inItem1, const void *inItem2)
{
	CategoryEntry *data1 = (CategoryEntry *)inItem1;
	CategoryEntry *data2 = (CategoryEntry *)inItem2;
	return mame_stricmp(data1->driver, data2->driver);
}


//===============================================================================
//	IsValidCategoryFile
//
//	Returns true if the given file is a valid category file.
//===============================================================================

Boolean	IsValidCategoryFile(const FSSpec *inSpec)
{
	char				tempString[256];
	SInt32				size;
	SInt16				file;
	OSErr				err;
	
	// open the file
	err = FSpOpenDF(inSpec, fsRdPerm, &file);
	if (err != noErr)
		return false;
	
	// read in the data
	size = strlen(sCategoryKeyString);
	err = FSRead(file, &size, tempString);
	if (err != noErr)
	{
		FSClose(file);
		return false;
	}
	
	// close the file
	FSClose(file);
	
	// count end-of-lines to determine the number of array elements to allocate
	return !strncmp(sCategoryKeyString, tempString, strlen(sCategoryKeyString));
}


#pragma mark -
#pragma mark ¥ List Descriptions

//===============================================================================
//	RebuildListDescriptions
//
//	Rebuilds the set of list nodes.
//===============================================================================

static void RebuildListDescriptions(void)
{
	ListNode *			node;

	// after we're done, recompute the descriptions of each node
	for (node = sFirstListNode; node != NULL; node = node->next)
		SetListNodeDescription(node);

	// mark the grouping valid and the sorting invalid
	sListDescriptionsInvalid = false;
	sListSortingInvalid = true;
}


//===============================================================================
//	SetListNodeDescription
//
//	Sets the description field of the given cell node.
//===============================================================================

static void SetListNodeDescription(ListNode *inNode)
{
	// first copy either the driver description or the filename
	if (inNode->romset)
	{
		if (inNode->romset->driver)
		{
			if (gFEPrefs.fe_displayFileNames)
				c2pstrcpy(inNode->description, inNode->romset->driver->name);
			else
				c2pstrcpy(inNode->description, inNode->romset->driver->description);
		}
		else
			PLstrcpy(inNode->description, inNode->romset->spec.name);

		// then modify it according to the prefs
		if (gFEPrefs.fe_listSortThe)
		{
			if (inNode->description[0] > 4 &&
				tolower(inNode->description[1]) == 't' &&
				tolower(inNode->description[2]) == 'h' &&
				tolower(inNode->description[3]) == 'e' &&
				tolower(inNode->description[4]) == ' ')
			{
				int length = inNode->description[0];
				memmove(&inNode->description[1], &inNode->description[5], length - 4);
				memcpy(&inNode->description[length - 3], ", The", 5);
				inNode->description[0] += 1;
			}
		}
	}
}



#pragma mark -
#pragma mark ¥ List Sorting

//===============================================================================
//	SortByFolder
//
//	Sorts the ROMSetData by folder.
//===============================================================================

static void SortByFolder(void)
{
	UInt32			nodeCount = MemoryPoolItems(sListNodePool);
	ListNode *		node;
	UInt32			currentNest = 0;
	UInt32 			index;
	UInt32			start;
	SInt32 			count;

	// free any old array and allocate a new one
	if (sSortedListNodeArray != NULL)
		free(sSortedListNodeArray);
	sSortedListNodeArray = malloc(nodeCount * sizeof(ListNode *));
	
	// initialize the indexes 
	for (node = sFirstListNode, index = 0; node != NULL; node = node->next)
		sSortedListNodeArray[index++] = node;

	// loop over folders
	for (index = start = 0; index < nodeCount; index++)
	{
		// if this entry is not the same parent as the previous guy, it's time to sort
		if (sSortedListNodeArray[index]->nest != currentNest && index != 0)
		{
			// if the new entry is more inward, don't include the previous entry in the sort
			if (sSortedListNodeArray[index]->nest > currentNest)
				count = index - start - 1;
			
			// otherwise, the new entry has backed out; sort everything
			else
				count = index - start;

			// if this isn't an empty folder, sort it
			if (count > 1)
				qsort(&sSortedListNodeArray[start], count, sizeof(sSortedListNodeArray[0]), CompareDescriptions);
			
			// remember where we left off
			start = index;
			currentNest = sSortedListNodeArray[index]->nest;
		}
	}
	
	// if there's anything left, sort that
	count = index - start;
	if (count > 1)
		qsort(&sSortedListNodeArray[start], count, sizeof(sSortedListNodeArray[0]), CompareDescriptions);
	
	// reassemble all the node links in order
	sFirstListNode = NULL;
	for (index = 0; index < nodeCount; index++)
	{
		node = sSortedListNodeArray[nodeCount - index - 1];
		node->next = sFirstListNode;
		sFirstListNode = node;
	}
	
	// reinitialize the indexes 
	for (node = sFirstListNode, index = 0; node != NULL; node = node->next)
		sSortedListNodeArray[index++] = node;

	// mark the sorting valid
	sListSortingInvalid = false;
}


//===============================================================================
//	CompareDescriptions
//
//	qsort compare function that just compares descriptions from the drivers.
//===============================================================================

static int CompareDescriptions(const void *inItem1, const void *inItem2)
{
	ListNode *data1 = *(ListNode **)inItem1;
	ListNode *data2 = *(ListNode **)inItem2;
	return RelString(data1->description, data2->description, false, true);
}


#pragma mark -
#pragma mark ¥ Memory Pools

//===============================================================================
//	CreateMemoryPool
//
//	Creates a memory pool, which is a more optimal way of allocating arrays
//	of same-sized items.
//===============================================================================

MemoryPool CreateMemoryPool(UInt32 inItemSize, UInt32 inChunkSize)
{
	MemoryPoolData *data;

	// first allocate the memory pool data structure
	data = malloc(sizeof(MemoryPoolData));
	if (!data)
		return NULL;
	
	// fill in the rest of the data
	data->itemSize = inItemSize;
	data->itemsAllocated = 0;
	data->chunkSize = inChunkSize;
	data->chunksAllocated = 0;
	data->firstChunk = 0;
	
	return data;
}


//===============================================================================
//	DisposeMemoryPool
//
//	Frees the memory allocated by a memory pool.
//===============================================================================

void DisposeMemoryPool(MemoryPool inPool)
{
	MemoryPoolData *data = inPool;
	void *curChunk, *nextChunk;
	int index;

	// loop over chunks and dispose of them
	curChunk = data->firstChunk;
	for (index = 0; index < data->chunksAllocated; index++)
	{
		nextChunk = *(void **)curChunk;
		free(curChunk);
		curChunk = nextChunk;
	}

	// free the pool
	free(inPool);
}


//===============================================================================
//	AllocateFromPool
//
//	Allocates a new item from a memory pool.
//===============================================================================

void *AllocateFromPool(MemoryPool inPool)
{
	MemoryPoolData *data = inPool;
	void *newChunk;
	UInt32 index;

	// allocate a new chunk if we need to
	if (data->itemsAllocated >= data->chunkSize * data->chunksAllocated)
	{
		newChunk = malloc(sizeof(void *) + data->itemSize * data->chunkSize);
		*(void **)newChunk = data->firstChunk;
		data->firstChunk = newChunk;
		data->chunksAllocated++;	
	}

	// if we haven't yet maxed out, just grab from the current chunk
	index = data->itemsAllocated % data->chunkSize;
	data->itemsAllocated++;
	return (void *)((UInt8 *)data->firstChunk + sizeof(void *) + index * data->itemSize);
}


//===============================================================================
//	MemoryPoolItems
//
//	Returns the number of items currently allocated in a pool.
//===============================================================================

UInt32 MemoryPoolItems(MemoryPool inPool)
{
	MemoryPoolData *data = inPool;
	return data->itemsAllocated;
}
