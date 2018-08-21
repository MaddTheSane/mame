#ifndef mac_strings_h_
#define mac_strings_h_

enum {
	rStrings = 128,
	rErrorStrings,
	rKeyboardStrings,
	rGroupMenuStrings,
#ifdef MESS
	rMESSStrings
#endif
};
	
enum {
	kWindowName = 1,
	kSaveList,
	kGameList,
	kROMList,
	kSampleList,
	kDriverList,

	kCloneList,
	kParentList,
	kBrokenList,
	kImpSoundList,
	kNoSoundList,
	kImpColorList,
	kWrongColorList,
	kROMAudit,
	kSampleAudit,

	kRomsetAnalysis,
	kAnalysisProgressTitle,
	kLoadingProgressTitle,
	kROMsRemainingProgressMsg,
	kROMAuditProgressTitle,

	kSampleAuditProgressTitle,
	kScreenshotNotWorkingFileName,
	kScreenshotNotAvailableFileName,

	kZIPImages,
	kZIPCabinets,
	kZIPFlyers,
	kZIPArtwork,
	kZIPMarquees,
	kZIPTitleImages,
	kZIPControlPanels,
	kAuditProgressMsg,
	kAnalysisProgressMsg,

	kROMIdentResultsTitle,
	kROMIdentDefaultFilename,
	
	kRA_AliasLabel,
	kRA_CouldntResolve,
	kRA_FormatLabel,
	kRA_FormatSuperROM,
	kRA_FormatFolder,
	kRA_FormatZipfile,
	kRA_DriverLabel,
	kRA_CloneOfLabel,
	kRA_ParentOfLabel,
	kRA_EmptyFolder,
	kRA_AliasToParent,
	kRA_Unresolved,
	kRA_AnalysisLabel,
	kRA_DupeFileLabel,
	kRA_DupePrefName,
	kRA_RqdByCloneOnly,
	kRA_NotRequiredRom,
	kRA_RqdByCloneAlso,
	kRA_DupeRomInParent,
	kRA_CanMoveToParent,
	
	kReportSupportedGamesHeader,
	kReportSoundSamplesHeader,
	kReportClonesHeader,
	kReportParentsHeader,
	kReportBrokenHeader,
	kReportImpSoundHeader,
	kReportNoSoundHeader,
	kReportImpColorHeader,
	kReportWrongColorHeader,
	kReportNoRomsNeededLabel,
	kReportRomsNeededCloneLabel,
	kReportRomsNeededLabel,
	kReportSamplesNeededCloneLabel,
	kReportSamplesNeededLabel,
	kReportCloneOfLabel,
	kReportParentOfLabel,
	kReportTotalGamesSummary,
	kReportUniqueGamesSummary,
	kReportSamplesSummary,
	kReportClonesSummary,
	kReportParentsSummary,
	kReportBrokenSummary,
	kReportImpSoundSummary,
	kReportNoSoundSummary,
	kReportImpColorSummary,
	kReportWrongColorSummary,
	
	kReportNoGoodDump,
	kReportMultipleRomsetsFound,
	kReportNotDriverName,
	kReportAuditFilter,
	kReportNoRomsetFound,
	kReportROMNotFound,
	kReportNoDumpKnown,
	kReportBadCRC,
	kReportExpected,
	kReportFound,
	kReportBadLength,
	kReportNoChecksum,
	kReportGood,
	kReportLength,
	kReportNoMemory,
	kReportCheckDupes,
	kReportCheckUnknown,
	kReportNoDupes,
	kReportNoUnknowns,
	kReportSampleProblems,
	kReportNoSamplesFound,
	kReportProblem,
	kReportSampleNotFound,
	kReportDiskNotFound,
	
	kInfoStatus,
	kInfoGood,
	kInfoROMNotFound,
	kInfoROMNotFound_Parent,
	kInfoROMNotFound_BIOS,
	kInfoNoGoodDump,
	kInfoNoDumpKnown,
	kInfoBadCRC,
	kInfoBadLength,
	kInfoExpected,
	kInfoMemError,
	kInfoUnknown,
	kInfoNoHistory,
	
	kIdentFilename,
	kIdentDriver,
	kIdentDriverDesc,
	kIdentNoMatch,
	
	kFrontendNotWorking,
	
	kInfoOldCHD
};

enum {
	kCouldntCreateFileError = 1,
	kAnalysisMemFullError,
	kAnalysisCanceledError,
	kCouldntCreateSortError,
	kAuditCanceledError
	};

enum {
	kShowClones = 1,
	kHideClones,
	kShowGhosts,
	kHideGhosts,
	kShowNonWorking,
	kHideNonWorking,
	kAttachClones,
	kDetachClones,
	kGroupBy
};

#ifdef MESS
enum {
	// Button name for file image controls
	kCartButtonName = 1,
	kFloppyButtonName,
	kHardDiskButtonName,
	kCylinderButtonName,
	kTapeButtonName,
	kPunchCardButtonName,
	kPunchTapeButtonName,
	kSnapShotButtonName,
	kQuickLoadButtonName,
	kMemCardButtonName,
	// Group names for serial/parallel port controls
	kSerialGroupName,
	kParallelGroupName,
	kPrinterGroupName,
};
#endif

#endif