/*
 *	WASTE.h
 *
 *	C/C++ interface to the WASTE text engine
 *
 *	version 3.0.1 (December 2006)
 *
 *	Copyright (c) 1993-2006 Marco Piovanelli & Ovolab
 *	All Rights Reserved
 *
 *	<http://www.merzwaren.com/waste/>
 *	<mailto:marco.piovanelli@pobox.com>
 *
 */

#ifndef __WASTE__
#define __WASTE__

#ifndef __CARBON__
#include <Carbon/Carbon.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __LONGCOORDINATES__
#ifndef _LongCoords_
#define _LongCoords_

typedef struct LongPt
{
	SInt32 v;
	SInt32 h;
} LongPt;

typedef struct LongRect
{
	SInt32 top;
	SInt32 left;
	SInt32 bottom;
	SInt32 right;
} LongRect;

#endif /*_LongCoords_*/
#endif /*__LONGCOORDINATES__*/

#if PRAGMA_STRUCT_ALIGN
#pragma options align=mac68k
#endif

#if PRAGMA_IMPORT
#pragma import on
#endif

/*	The macro WASTE_VERSION expands to the current version of WASTE,		*/
/*	expressed in standard NumVersion format (see <CarbonCore/MacTypes.h>)	*/

#define WASTE_VERSION	0x03018000		/* 3.0.1 */

/*	Set the macro WASTE_DEPRECATED to 0 before #including WASTE.h			*/
/*	to prevent the definition of deprecated APIs and data types				*/

#ifndef WASTE_DEPRECATED
#define WASTE_DEPRECATED		1
#endif

/*	result codes */

enum
{
	weCantUndoErr				=	-10015,	/* undo buffer is clear (= errAECantUndo) */
	weEmptySelectionErr			=	-10013,	/* selection range is empty (= errAENoUserSelection) */
	weUnknownObjectTypeErr		=	-9478,	/* specified object type is not registered */
	weObjectNotFoundErr			=	-9477,	/* no object found at specified offset */
	weReadOnlyErr				=	-9476,	/* instance is read-only */
	weTextNotFoundErr			=	-9474,	/* search string not found */
	weInvalidTextEncodingErr	=	-9473,	/* specified text encoding is invalid or unsupported */
	weDuplicateAttributeErr		=	-9472,	/* one of the attribute selectors was specified more than once */
	weInvalidAttributeSizeErr	=	-9471,	/* attribute value size is invalid */
	weReadOnlyAttributeErr		=	-9470,	/* attribute is read-only */
	weOddByteCountErr			=	-9469,	/* expected an even number of bytes */
	weHandlerNotFoundErr		=	-1717,	/* couldn't find specified handler (= errAEHandlerNotFound) */
	weNotHandledErr				=	-1708,	/* please use default behavior (= errAEEventNotHandled) */
	weNewerVersionErr			=	-1706,	/* version of formatting scrap is too new (= errAENewerVersion) */
	weCorruptDataErr			=	-1702,	/* malformed formatting scrap (= errAECorruptData) */
	weProtocolErr				=	-603,	/* improper call order (= protocolErr) */
	weUndefinedSelectorErr		=	-50		/* unknown selector (= paramErr) */
};

/*	values for the outEdge parameter in WEGetOffset etc. */

enum
{
	kLeadingEdge			=	-1,		/* point is on the leading edge of a glyph */
	kTrailingEdge			=	 0,		/* point is on the trailing edge of a glyph */
	kObjectEdge				=	 2		/* point is in the middle of an embedded object */
};

/*	values for the inFeature parameter in WEFeatureFlag */

enum
{
	weFAutoScroll					=	0,		/* automatically scroll the selection range into view */
	weFOutlineHilite				=	2,		/* frame selection when deactivated */
	weFReadOnly						=	5,		/* disallow modifications */
	weFUndo							=	6,		/* support WEUndo() */
	weFIntCutAndPaste				=	7,		/* use intelligent cut-and-paste rules */
	weFDragAndDrop					=	8,		/* support drag-and-drop text editing */
	weFInhibitRecal					=	9,		/* don't recalculate line starts and don't redraw text */
	weFInhibitRedraw				=	12,		/* don't redraw text */
	weFMonoStyled					=	13,		/* disallow style changes */
	weFMultipleUndo					=	14,		/* enable multiple undo/redo */
	weFLeftMarginClick				=	16,		/* enable single-click selection of lines */
	weFTransientFontMatching		=	17,		/* enable ATSUI transient font matching */
	weFNoAutoTabForHangingIndent	=	28,		/* don't add an automatic tab stop for hanging indents */
	weFNoKeyboardSync				=	29,		/* disable automatic font/keyboard synchronization */
	weFInhibitICSupport				=	30,		/* don't process command-clicks with Internet Config */
	weFInhibitColor					=	31,		/* ignore color information; draw in black & white only */

/*	the following selectors are obsolete and unsupported in WASTE 3.0 : */
	weFAutoIdle						=	1,		/* automatically blink the caret */
	weFUseTempMem					=	10,		/* use temporary memory for main data structures */
	weFDrawOffscreen				=	11		/* draw text offscreen for smoother visual results */
};

/*	values for the inOptions parameter in WENew */

enum
{
	weDoAutoScroll					=	1L << weFAutoScroll,
	weDoOutlineHilite				=	1L << weFOutlineHilite,
	weDoReadOnly					=	1L << weFReadOnly,
	weDoUndo						=	1L << weFUndo,
	weDoIntCutAndPaste				=	1L << weFIntCutAndPaste,
	weDoDragAndDrop					=	1L << weFDragAndDrop,
	weDoInhibitRecal				=	1L << weFInhibitRecal,
	weDoInhibitRedraw				=	1L << weFInhibitRedraw,
	weDoMonoStyled					=	1L << weFMonoStyled,
	weDoMultipleUndo				=	1L << weFMultipleUndo,
	weDoLeftMarginClick				=	1L << weFLeftMarginClick,
	weDoTransientFontMatching		=	1L << weFTransientFontMatching,
	weDoNoAutoTabForHangingIndent	=	1L << weFNoAutoTabForHangingIndent,
	weDoNoKeyboardSync				=	1L << weFNoKeyboardSync,
	weDoInhibitICSupport			=	1L << weFInhibitICSupport,
	weDoInhibitColor				=	1L << weFInhibitColor,

/*	the following selectors are obsolete and unsupported in WASTE 3.0: */
	weDoAutoIdle					=	1L << weFAutoIdle,
	weDoUseTempMem					=	1L << weFUseTempMem,
	weDoDrawOffscreen				=	1L << weFDrawOffscreen
};

/*	values for the inAction parameter in WEFeatureFlag */

enum
{
	weBitToggle				=	-2,		/* toggles the specified feature */
	weBitTest				=	-1,		/* returns the current setting of the specified feature */
	weBitClear				=	 0,		/* disables the specified feature */
	weBitSet				=	 1		/* enables the specified feature */
};

/*	values for the inCase parameter in WEChangeCase */

enum
{
	weLowerCase				=	0,		/* lowercase text */
	weUpperCase				=	1		/* uppercase text */
};

/*	additional values for the WETSMHiliteStyle data type */
/*	the basic values for this can be found in <AE/AERegistry.h>: kTSMHiliteRawText, kTSMHiliteSelectedRawText, etc. */

enum
{
	weRedWigglyUnderline	=	29303,	/* ('rw') red wiggly underline commonly used to mark misspelled words */
	weGreenWigglyUnderline	=	26487,	/* ('gw') green wiggly underline commonly used to mark syntax/grammar errors */
	weOrangeWigglyUnderline	=	28535,	/* ('ow') orange wiggly underline */
	weDeadKeyHilite			=	25707	/* ('dk') Cocoa-style yellow highlight commonly used to mark dead keys */
};

/*	values for the inMatchOptions parameter in WEFind */

enum
{
	weFindWholeWords				=	0x00000001,		/* match whole words only */
	weFindCaseInsensitive			=	0x00000002,		/* ignore case differences */
	weFindDiacriticalInsensitive	=	0x00000004		/* ignore diacritical marks */
};

/*	values for the inPutOptions parameter in WEPut */

enum
{
	wePutIntCutAndPaste				=	0x00000001,		/* apply intelligent cut & paste rules */
	wePutAddToTypingSequence		=	0x00000002,		/* don't break current typing sequence */
	wePutDetectUnicodeBOM			=	0x00000200,		/* detect Unicode byte-order mark */
	wePutRTF						=	0x00000400		/* parse provided text buffer as RTF */
};

/*	values for the inStreamOptions parameter in WEStreamRange */

enum
{
	weStreamDestinationKindMask		=	0x000000FF,		/* the low byte in inStreamOptions is passed on to */
														/* the inDestinationKind parameter of object streaming handlers */
														/* (valid option for the kTypeSoup and kTypeStyleScrap flavor types) */
	weStreamIncludeObjects			=	0x00000100,		/* include descriptions of embedded objects */
														/* (valid option for the kTypeStyleScrap flavor type) */
	weStreamAddUnicodeBOM			=	0x00000200,		/* prepend a Unicode byte-order mark to the text stream */
														/* (valid option for the kTypeUnicodeText flavor type */
	weStreamEndianSwap				=	0x00000400		/* retrieve UTF-16 text in non-native byte order, i.e., little-endian on PowerPC, big-endian on Intel */
														/* (valid option for the kTypeUnicodeText flavor type */

#if WASTE_DEPRECATED
	,
	weGetAddUnicodeBOM				=	0x00000200,		/* deprecated synonym for weStreamAddUnicodeBOM */
	weGetLittleEndian				=	0x00000400		/* deprecated synonym for weStreamEndianSwap -- really a misnomer on little-endian native machines */
#endif /*WASTE_DEPRECATED*/
};

/*	values for the inCopyOptions parameters in WECopyToScrap */

enum
{
	weCopyPromiseFlavors			=	0x00000001		/* install a promise keeper but don't actually copy data to scrap just yet */
};

/*	values for the inSaveOptions parameter in WESave */

enum
{
	weSaveAddResources				=	0x00000001,		/* save formatting resources to the resource fork */
	weSaveCompatibilityResources	=	0x00000002,		/* save additional resources (styl, FISH, SOUP) for compatibility with TextEdit and WASTE 1.x */
	weSaveLittleEndian				=	0x00000004		/* use little-endian (Intel) byte-order (only significant for 'utxt' files) */
};

/*	values for the inSetEventTargetOptions parameter in WESetEventTarget */

enum
{
	weTextInputEvents				=	0x00000001,		/* install kEventClassTextInput event handlers */
	weCommandEvents					=	0x00000002,		/* install kEventClassCommand event handlers */
	weFontPanelEvents				=	0x00000004		/* install kEventClassFont event handlers */
};

/*	WASTE creator */
/*	Pass this value in the propertyCreator parameter of SetMenuItemProperty to set values of attributes to be processed by WASTE. */
/*	For example, if you have a Font menu, call SetMenuItemProperty passing kWASTECreator in propertyCreator, weTagFontFamily in */
/*	propertyTag, and the correct font family number in propertyData. Later, when the user selects an item in the Font menu, you */
/*	can call WEProcessHICommand and WASTE will extract the correct attribute tag and value and change the selection accordingly. */

enum
{
	kWASTECreator				=	'OEDE'
} ;

/*	selectors for WESetAttributes, WEGetAttributes, WEMatchAttributes, etc. */

enum
{
/*	character-level attributes: font, size, colors, basic styles */
	weTagFontID					=	'ftid', /* ATSUI font ID (ATSUFontID aka FMFont) */
	weTagFontFamily				=	'font',	/* Quickdraw font family number (FMFontFamily) */
	weTagFontSize				=	'ptsz',	/* font size (Fixed) */
	weTagPlain					=	'plan', /* plain text (Boolean) */
	weTagBold					=	'bold', /* bold (Boolean) */
	weTagItalic					=	'ital', /* italic (Boolean) */
	weTagUnderline				=	'undl',	/* underline (Boolean) */
	weTagOutline				=	'outl', /* outline (Boolean) */
	weTagShadow					=	'shad', /* shadow (Boolean) */
	weTagCondensed				=	'cond', /* condensed (Boolean) */
	weTagExtended				=	'pexp', /* extended (Boolean) */
	weTagStrikethrough			=	'strk', /* strikethrough (Boolean) */
	weTagHidden					=	'hidn', /* hidden (Boolean) */
	weTagAllCaps				=	'alcp', /* all caps (Boolean) */
	weTagAllLowercase			=	'lowc', /* all lowercase (Boolean) */
	weTagTextColor				=	'colr',	/* text color (RGBColor) */
	weTagBackgroundColor		=	'pbcl', /* background color (RGBColor) */
	weTagTransferMode			=	'pptm', /* QuickDraw text transfer mode (SInt16) */
	weTagVerticalShift			=	'xshf',	/* vertical shift (Fixed) */
	weTagLanguage				=	'lang', /* language tag (TEXT) */
	weTagGlyphSelector			=	'glyf', /* glyph selector (ATSUGlyphSelector) */
	weTagGroupID				=	'grid',	/* group ID (UInt32) */

/*	character-level attributes: additional styles -- these are used for RTF roundtripping but not currently rendered */
	weTagUnderlineStyle			=	'unds', /* underline style (enumeration: deft/word/dubl/etc.) */
	weTagSmallCaps				=	'smcp', /* small caps (Boolean) */
	weTagDoubleStrikethrough	=	'dstr', /* double strikethrough (Boolean) */
	weTagEmbossed				=	'embo', /* embossed (Boolean) */
	weTagEngraved				=	'engr', /* engraved (Boolean) */

/*	ephemeral (non-persistent) character-level attributes */
	weTagTSMHiliteStyle			=	'tsmh',	/* Text Services Manager hilite style (WETSMHiliteStyle) */

/*	paragraph-level attributes: alignment, direction, line spacing, indents, tabs and borders */
	weTagAlignment				=	'pjst',	/* alignment (enumeration; can be one of deft/left/cent/rght/full) */
	weTagDirection				=	'LDIR', /* primary line direction (enumeration; can be one of deft/L->R/R->L) */
	weTagLineSpacing			=	'ledg', /* line spacing (WELineSpacing) */
	weTagLeftIndent				=	'lein', /* left indent (Fixed) */
	weTagRightIndent			=	'riin',	/* right indent (Fixed) */
	weTagFirstLineIndent		=	'fidt', /* first line indent (Fixed) */
	weTagSpaceBefore			=	'spbe', /* space before (Fixed) */
	weTagSpaceAfter				=	'spaf', /* space after (Fixed) */
	weTagTabList				=	'tabs', /* tab list (WETabList) */
	weTagBottomBorderStyle		=	'BBRD', /* bottom border style (enumeration; can be one of NONE/SLDL/DTDL/THKL) */

/*	paragraph-level attributes: pagination control and other flags -- for future use */
	weTagKeepTogether			= 	'keep', /* keep lines together on the same page (Boolean) */
	weTagKeepWithNext			= 	'kepn', /* keep this paragraph with the next one on the same page (Boolean) */
	weTagPageBreakBefore		= 	'pbrb', /* this paragraph starts a new page (Boolean) */
	weTagWidowOrphanOverride	= 	'wdov', /* override document-wide widow/orphan control (Boolean) */
	weTagWidowOrphanControl		= 	'wido', /* enable or disable widow/orphan control for this paragraph only (Boolean) */
	weTagNoLineNumbering		= 	'!ln#', /* disable document-wide line number for this paragraph only (Boolean) */
	weTagNoHyphenation			= 	'!hyp', /* disable automatic hyphenation for this paragraph only (Boolean) */
	weTagParagraphUserData		= 	'pusr', /* user-defined data (SInt32) */

/*	the following meta-selectors are only valid in calls to WESetAttributes/WESetOneAttribute */
	weTagAddFontSize			=	'+siz',	/* like weTagFontSize, but value is added rather than replaced */
	weTagAddVerticalShift		=	'+shf',	/* like weTagVerticalShift, but value is added rather than replaced */
	weTagAddLeftIndent			=	'+lei',	/* like weTagLeftIndent, but value is added rather than replaced */
	weTagAddRightIndent			=	'+rii', /* like weTagRightIndent, but value is added rather than replaced */
	weTagAddFirstLineIndent		=	'+fid', /* like weTagFirstLineIndent, but value is added rather than replaced */
	weTagAddSpaceBefore			=	'+spb', /* like weTagSpaceBefore, but value is added rather than replaced */
	weTagAddSpaceAfter			=	'+spa', /* like weTagSpaceAfter, but value is added rather than replaced */
	weTagAddLineSpacing			=	'+led', /* add this to line spacing (Fixed) */

/*	selectors for read-only attributes, only valid in calls to WEGetAttributes/WEGetOneAttribute */
	weTagQDStyles				=	'qdst',	/* QuickDraw styles (Style) */
	weTagTETextStyle			=	'tets',	/* TextEdit-compatibile TextStyle record */

/*	the following selectors are obsolete and unsupported in WASTE 3.0 */
	weTagForceFontFamily		=	'ffnt',	/* like weTagFontFamily, but may change text encoding */
	weTagRunDirection			=	'rdir',	/* true if character at specified offset belongs to a right-to-left script (Boolean) */
	weTagTextEncoding			=	'ptxe'	/* text encoding */
};

/*	underline style selectors (use in conjunction with weTagUnderlineStyle) */

enum
{
	weTagUnderlineDefault		=	'deft',
	weTagUnderlineWord			=	'word',
	weTagUnderlineDouble		=	'dubl',
	weTagUnderlineThick			=	'thck',
	weTagUnderlineDash			=	'-   ',
	weTagUnderlineDot			=	'.   ',
	weTagUnderlineDotDash		=	'.-  ',
	weTagUnderlineDotDotDash	=	'..- ',
	weTagUnderlineWave			=	'wave'
};

/*	alignment selectors (use in conjunction with weTagAlignment and weTagTabList) */

enum
{
	weTagAlignmentDefault		=	'deft',	/* align according to system direction */
	weTagAlignmentLeft			=	'left',	/* flush left */
	weTagAlignmentCenter		=	'cent', /* center */
	weTagAlignmentRight			=	'rght',	/* flush right */
	weTagAlignmentFull			=	'full',	/* justify */
	weTagAlignmentForced		=	'kiwa',	/* forced justify */
	weTagAlignmentDecimal		=	'decm'	/* decimal (tabs only) */
};

/*	direction selectors (use in conjunction with weTagDirection) */

enum
{
	weTagDirectionDefault		=	'deft',	/* arrange bidi text according to system direction */
	weTagDirectionLeftToRight	=	'L->R', /* primary line direction is left-to-right */
	weTagDirectionRightToLeft	=	'R->L'	/* primary line direction is right-to-left */
};

/*	tab leader selectors (use in conjunction with weTagTabList) */

enum
{
	weTagLeaderNone				= 	'NONE',	/* no tab leader (default) */
	weTagLeaderDots				= 	'DOTS',	/* dots */
	weTagLeaderHyphens			= 	'HYPH',	/* hyphens */
	weTagLeaderUnderline		= 	'UNDL', /* underline */
	weTagLeaderThickLine		= 	'THKL', /* thick line */
	weTagLeaderEqualSigns		= 	'=   '	/* equal signs */
};

/*	border style selectors (use in conjunction with weTagBottomBorderStyle) */

enum
{
	weTagBorderStyleNone		=	'NONE', /* no border */
	weTagBorderStyleThin		=	'SLDL', /* thin line */
	weTagBorderStyleDotted		=	'DTDL', /* dotted line */
	weTagBorderStyleThick		=	'THKL'	/* thick line */
};

/*	line spacing mode selectors (possible values of the mode field of a WELineSpacing structure) */

enum
{
	weTagLineSpacingAbsolute	=	'abso',	/* use specified value */
	weTagLineSpacingAtLeast		=	'atle',	/* use specified value or height of tallest character, whichever is greater */
	weTagLineSpacingRelative	=	'rele'	/* multiple of natural height (0.0 = single spacing; 1.0 = double spacing; etc.) */
};

/*	commonly used values for the line spacing attribute (these are only significant when mode == weTagLineSpacingRelative) */

enum
{
	weLineSpacingSingle			=	0x00000000,				/* single space */
	weLineSpacingOneAndHalf		=	0x00008000,				/* one and half space */
	weLineSpacingDouble			=	0x00010000				/* double space */
};

/*	possible values for the inAttributeOptions parameter to WERegisterCustomAttribute */

enum
{
	weAttributeIsRuler			=	0x00000001,				/* attribute is a paragraph-level (ruler) attribute */
	weAttributeIsPersistent		=	0x00000002,				/* attribute should be saved in scraps */
	weAttributeDontExtend		=	0x00000004				/* attribute doesn't propagate to text inserted at the following run boundary */
};

/*	selectors for WEGetInfo and WESetInfo */

enum
{
	weAutoScrollDelay			=	'ausd', /* for use by the click loop callback (in ticks) */
	weBusyProc					=	'busy', /* callback invoked during lengthy operations */
	weBusyInterval				=	'bzin', /* minimum interval between calls to the busy proc (in ticks) */
	weClickLoop					=	'clik',	/* click loop callback */
	weCurrentDrag				=	'drag',	/* drag currently being tracked from WEClick() */
	weCGEraseHook				=	'cera',	/* background erasing hook */
	weFluxProc					=	'flux', /* flux proc */
	weHiliteDropAreaHook		=	'hidr', /* drop area highlighting hook */
	weLineBreakHook				=	'lbrk',	/* line breaking hook */
	wePixelToCharHook			=	'p2c ', /* PixelToChar hook */
	wePort						=	'port',	/* graphics port */
	wePreTrackDragHook			=	'ptrk', /* pre-TrackDrag hook */
	wePrepareViewHook			=	'prep', /* invoked immediately before drawing into a view */
	weRefCon					=	'refc',	/* reference constant for use by application */
	weScrollProc				=	'scrl',	/* auto-scroll callback */
	weTextFilterProc			=	'fltr',	/* text input filter */
	weTranslateDragHook 		=	'xdrg', /* drag translation callback */
	weTSMDocumentID				=	'tsmd',	/* Text Services Manager document ID */
	weTSMPreUpdate				=	'pre ',	/* Text Services Manager pre-update callback */
	weTSMPostUpdate				=	'post',	/* Text Services Manager post-update callback */
	weUndoProc					=	'undo', /* undo notification proc */
	weURLHint					=	'urlh',	/* URL hint string for Internet Config */

/*	the following selectors are unsupported in WASTE 3.0:	*/
	weCharByteHook				=	'cbyt', /* CharByte hook */
	weCharToPixelHook			=	'c2p ',	/* CharToPixel hook */
	weCharTypeHook				=	'ctyp', /* CharType hook */
	weDrawTextHook				=	'draw', /* text drawing hook */
	weDrawTSMHiliteHook			=	'dtsm',	/* hook for drawing Text Services Manager underlines */
	weEraseHook					=	'eras', /* background erasing hook -- use weCGEraseHook instead */
	weFontFamilyToNameHook		=	'ff2n', /* hook for mapping font family numbers to font names */
	weFontNameToFamilyHook		=	'fn2f', /* hook for mapping font names to font family numbers */
	weText						=	'text',	/* text handle */
	weTranslucencyThreshold		=	'tluc', /* area threshold for translucent drags */
	weWordBreakHook				=	'wbrk'	/* word breaking hook */
};

/*	selectors for WEGetViewInfo and WESetViewInfo */

enum
{
	weCGContext					=	'ctxt',	/* Core Graphics context (CGContextRef) */
	weBackgroundColor			=	'back',	/* background color (ATSURGBAlphaColor) */
	weHiliteColor				=	'hili',	/* highlight color (ATSURGBAlphaColor) */
	weInactiveHiliteColor		=	'inhi',	/* inactive highlight color (ATSURGBAlphaColor) */
	weCaretColor				=	'cart'	/* caret color (ATSURGBAlphaColor) */
};

/*	values for the inHandlerSelector parameter in WEInstallObjectHandler */

enum
{
	weNewHandler				=	'new ',	/* new handler */
	weDisposeHandler			=	'free',	/* dispose handler */
	weDrawHandler				=	'draw',	/* draw handler -- obsolete and unsupported in WASTE 3.0 -- please use weCGDrawHandler instead */
	weCGDrawHandler				=	'cdrw',	/* draw handler -- Core Graphics version -- WASTE 3.0 clients should use this */
	weClickHandler				=	'clik',	/* click handler */
	weStreamHandler				=	'strm',	/* stream handler */
	weHoverHandler				=	'hovr'	/* hover handler */
};

/*	values for the inRequestedType parameter in WEStreamRange */

enum
{
	kTypeText					=	'TEXT',	/* raw text */
	kTypeTEStyleScrap 			=	'styl',	/* TextEdit-compatible style scrap */
	kTypeStyles					=	kTypeTEStyleScrap,		/* synonym of kTypeTEStyleScrap */
	kTypeSoup 					=	'SOUP',	/* "soup" of embedded objects */
	kTypeFontTable				=	'FISH',	/* font table */
	kTypeParaFormat				=	'WEpf',	/* WASTE 2.0 paragraph formatting */
	kTypeRulerScrap				=	'WEru',	/* WASTE 2.0 ruler table */
	kTypeCharFormat				=	'WEcf', /* WASTE 2.0 character formatting */
	kTypeStyleScrap				=	'WEst',	/* WASTE 2.0 style table */
	kTypeUnicodeText			=	'utxt',	/* raw text in UTF-16 Unicode */
	kTypeUTF8Text				=	'UTF8',	/* raw text in UTF-8 Unicode */
	kTypeStyledText				=	'STXT',	/* AppleScript-style styled text */
	kTypeRTF					=	'RTF ',	/* RTF (Rich Text Format) */
	kTypeRTFD					=	'RTFD'	/* RTFD (RTF with external attachments) */
};

/*	action kinds */

enum
{
	weAKNone			=	0,		/* null action */
	weAKUnspecified		=	1,		/* action of unspecified nature */
	weAKTyping			=	2,		/* some text has been typed in */
	weAKCut				=	3,		/* the selection range has been cut */
	weAKPaste			=	4,		/* something has been pasted */
	weAKClear			=	5,		/* the selection range has been deleted */
	weAKDrag			=	6,		/* drag and drop operation */
	weAKSetStyle		=	7,		/* some style has been applied to a text range */
	weAKSetRuler		=	8,		/* some ruler has been applied to a text range */
	weAKBackspace		=	9,		/* text deleted by backspace */
	weAKFwdDelete		=	10,		/* text deleted by forward delete */
	weAKCaseChange		=	11,		/* case transformation */
	weAKObjectChange	=	12,		/* an embedded object was resized */
	weAKCorrect			=	13		/* a misspelled word was corrected */
	/* values above 1023 are free for use by client applications */
};

/*	undo events passed to undo callback */

enum
{
	weUndoNewAction		=	0,		/*	a new action is about to be pushed onto the undo or redo stack */
	weUndoNewSubAction	=	1,		/*	a new action is about to be added to an open sequence */
	weUndoBeginSequence	=	2,		/*	an action sequence is starting */
	weUndoSetUndoString	=	3		/*	please set the undo string for this action */
};

/*	values for the inDestionationKind parameter passed to object streaming handlers */

enum
{
	weToScrap			=	0,		/* called from WECopy to copy object to the desk scrap */
	weToDrag			=	1,		/* called from WEClick to copy object to a drag */
	weToSoup			=	2		/* called to create a private SOUP for internal use (e.g., for undo/redo) */
	/* values above 127 are free for use by client applications */
};

/*	mouse actions passed to object "hover" handlers */

enum
{
	weMouseEnter		=	0,		/*	mouse has entered object frame */
	weMouseWithin		=	1,		/*	mouse is still within object frame */
	weMouseLeave		=	2		/*	mouse has exited object frame */
};

/*	busy actions passed to busy callback */

enum
{
	weBusyRecalBreaks	=	0		/*	busy recalculating line breaks */
};

/*	document property tags (these are currently only used by the RTF I/O module) */

enum
{
	weCreatorDocumentInfo		= 'Info',		/*	creator for the following property tags */

	weTagDocumentTitle			= 'Titl',
	weTagDocumentSubject		= 'Subj',
	weTagDocumentAuthor			= 'Auth',
	weTagDocumentManager		= 'Mngr',
	weTagDocumentCompany		= 'Cmpy',
	weTagDocumentCategory		= 'Ctgy',
	weTagDocumentKeywords		= 'Keyw',
	weTagDocumentComments		= 'Cmnt',
	weTagDocumentOperator		= 'Oper',
	weTagDocumentBaseAddress	= 'Hlnk',
	weTagPageInfo				= 'Page',
	weTagMacPrintRecord			= 'PRec'
};

enum
{
/*	kCurrentSelection is a meta-value that can be passed to some calls, */
/*	like WEStreamRange and WEGetHiliteRgn, to signify "use current selection range" */
	kCurrentSelection	=	-1,

/*	kNullStyle is a meta-value that can be passed to WEGetAttributes / WEGetOneAttribute */
/*	to retrieve the "null" style (the style that would be applied to the next typed character) */
	kNullStyle			=	-2,

/*	kMaxTabCount is the maximum number of tabs that can be set for each paragraph */
	kMaxTabCount		=	20,

/*	kMaxLanguageTagSize is the maximum length of a language tag */
	kMaxLanguageTagSize	=	32
};

#ifdef WE3_NOOPAQUETYPES

typedef class WE3_TextController *				WEReference;
typedef class WE3_EmbeddedObject *				WEObjectReference;
typedef class WE3_PrintSession *				WEPrintSession;
typedef class WE3_Action *						WEActionReference;
typedef class WE3_TextView *					WEViewReference;

#else

typedef struct OpaqueWEReference *				WEReference;
typedef struct OpaqueWEObjectReference *		WEObjectReference;
typedef struct OpaqueWEPrintSession *			WEPrintSession;
typedef struct OpaqueWEActionReference *		WEActionReference;
typedef struct OpaqueWEViewReference *			WEViewReference;

#endif

typedef Handle									WESoupHandle;
typedef Handle									WEFontTableHandle;
typedef SInt16									WEActionKind;
typedef SInt8									WEEdge;
typedef FourCharCode							WESelector;
typedef SInt16									WETSMHiliteStyle;

#if WASTE_DEPRECATED

/*	WERunInfo is deprecated in WASTE 3.0 */

typedef struct WERunInfo
{
	SInt32 				runStart;		/* byte offset to first character of style run */
	SInt32 				runEnd;			/* byte offset past last character of style run */
	SInt16 				runHeight;		/* line height (ascent + descent + leading) */
	SInt16 				runAscent;		/* font ascent */
	TextStyle 			runStyle;		/* text attributes */
	WEObjectReference	runObject;		/* either nil or reference to embedded object */
} WERunInfo;

#endif /*WASTE_DEPRECATED*/

typedef struct WEPrintOptions
{
	Rect			pageRect;			/* destination rectangle for printing */
	SInt16			firstPageOffset;	/* pixel height of area to be left blank at top of first page */
	PMRect			paperRect;			/* paper rectangle, needed for Core Graphics printing */
	char			reserved[22];		/* reserved for future use: set to zero! */
} WEPrintOptions;

/*	the first parameter to a 'new' embedded object handler is declared as a (Point *) for compatibility */
/*	with older versions of WASTE, but is really a (WEObjectPlacement *) starting from WASTE 2.1a5 */
/*	if the value of the objectBaseline field is set to a nonzero value, WASTE will place the bottom */
/*	of the bounding rectangle of the object below (negative value) or above (positive value) the text baseline */

typedef struct WEObjectPlacement
{
	SInt16			objectHeight;		/* height of embedded object */
	SInt16			objectWidth;		/* width of embedded object */
	SInt16			objectBaseline;		/* baseline (intrinsic vertical shift) of embedded object */
} WEObjectPlacement;

/*	WELineSpacing is used in conjunction with the weTagLineSpacing attribute */

typedef struct WELineSpacing
{
	WESelector		mode;				/* one of weTagLineSpacingRelative (default), weTagLineSpacingAbsolute or weTagLineSpacingAtLeast */
	Fixed			value;				/* absolute line height or multiplier (0.0 = auto) */
} WELineSpacing;

/*	WETab describes a single tab stop in a paragraph */

typedef struct WETab
{
	Fixed			tabIndent;			/* indent from the left side of the destination rectangle */
	WESelector		tabAlignment;		/* alignment (left, center, right or decimal) */
	WESelector		tabLeader;			/* leader */
} WETab;

/*	WETabList describes the array of tabs associated with a paragraph */

typedef struct WETabList
{
	ItemCount		tabCount;			/* actual number of valid tabs in this structure (0 to kMaxTabCount) */
	WETab			tabArray[kMaxTabCount];
} WETabList;

/*	WASTE callbacks: prototypes */

typedef CALLBACK_API ( Boolean,				WEClickLoopProcPtr )
	(	WEReference			inWE ) ;

typedef CALLBACK_API ( void,				WEScrollProcPtr )
	(	WEReference			inWE ) ;

typedef CALLBACK_API ( void,				WETSMPreUpdateProcPtr )
	(	WEReference			inWE ) ;

typedef CALLBACK_API ( void,				WETSMPostUpdateProcPtr )
	(	WEReference			inWE,
		SInt32				inFixLength,
		SInt32				inInputAreaStart,
		SInt32				inInputAreaEnd,
		SInt32				inPinRangeStart,
		SInt32				inPinRangeEnd ) ;

typedef CALLBACK_API ( Boolean,				WETextFilterProcPtr )
	(	ComponentInstance	inTextInputComponent,
		Handle				inFilterableText,
		UInt32				inModifiers,
		UInt32				inKeyCode,
		WEReference			inWE ) ;

typedef CALLBACK_API ( OSErr,				WEPreTrackDragProcPtr )
	(	DragRef				inDrag,
		WEReference			inWE ) ;

typedef CALLBACK_API ( void,				WEPrepareViewProcPtr )
	(	WEViewReference		inViewRef ) ;

typedef CALLBACK_API ( OSErr,				WETranslateDragProcPtr )
	(	DragRef				inDrag,
		DragItemRef			inDragItem,
		FlavorType			inRequestedType,
		Handle				outData,
		SInt32				inDropOffset,
		WEReference			inWE ) ;

typedef CALLBACK_API ( OSErr,				WEHiliteDropAreaProcPtr )
	(	DragRef				inDrag,
		Boolean				inHiliteFlag,
		WEReference			inWE ) ;

typedef CALLBACK_API ( OSErr,				WEFontIDToNameProcPtr )
	(	FMFontFamily		inFontFamilyNumber,
		Str255				ioQDFontName ) ;

typedef CALLBACK_API ( OSErr,				WEFontNameToIDProcPtr )
	(	ConstStr255Param	inQDFontName,
		FMFontFamily		inOldFontFamilyNumber,
		FMFontFamily *		outNewFontFamilyNumber ) ;

typedef CALLBACK_API ( OSErr,				WEBusyProcPtr )
	(	SInt16				inBusyAction,
		const Float32 *		inPercentDone,
		WEReference			inWE ) ;

typedef CALLBACK_API ( void, 				WEFluxProcPtr )
	(	SInt32				inOffset,
		SInt32				inDelta,
		WEReference			inWE ) ;

typedef CALLBACK_API ( OSErr,				WENewObjectProcPtr )
	(	WEObjectPlacement *	outPlacement,
		WEObjectReference	inObject ) ;

typedef CALLBACK_API ( OSErr,				WEDisposeObjectProcPtr )
	(	WEObjectReference	inObject ) ;

typedef CALLBACK_API ( OSErr,				WEDrawObjectProcPtr )
	(	const Rect *		inDestRect,
		WEObjectReference	inObject ) ;

typedef CALLBACK_API ( OSErr,				WECGDrawObjectProcPtr )
	(	CGContextRef		inCGContext,
		CGRect				inBounds,
		OptionBits			inOptions,
		WEObjectReference	inObject ) ;

typedef CALLBACK_API ( Boolean,				WEClickObjectProcPtr )
	(	Point				inHitPoint,
		EventModifiers		inModifiers,
		UInt32				inClickTime,
		WEObjectReference	inObject ) ;

typedef CALLBACK_API ( OSErr,				WEStreamObjectProcPtr )
	(	SInt16				inDestinationKind,
		FlavorType *		outStreamedFlavorType,
		Handle				outStreamedData,
		WEObjectReference	inObject ) ;

typedef CALLBACK_API ( OSErr,				WEHoverObjectProcPtr )
	(	SInt16				inMouseAction,
		Point				inMouseLoc,
		RgnHandle			inMouseRgn,
		WEObjectReference	inObject ) ;

typedef CALLBACK_API ( OSErr,				WEUndoProcPtr )
	(	SInt16				inUndoEvent,
		WEActionReference	inAction ) ;

typedef CALLBACK_API ( void,				WECGEraseProcPtr )
	(	CGContextRef		inCGContext,
		CGRect				inDirtyRect,
		WEViewReference		inView,
		WEReference			inWE ) ;

#if WASTE_DEPRECATED

//	the following callbacks are obsolete in WASTE 3.0, and should not be used
//	they will be removed from a future version of this header file

typedef CALLBACK_API ( void,				WEDrawTextProcPtr )
	(	const char *		inTextPtr,
		SInt32				inTextLength,
		Fixed				inSlop,
		JustStyleCode		inStyleRunPosition,
		WEReference			inWE ) ;

typedef CALLBACK_API ( void,				WEDrawTSMHiliteProcPtr )
	(	const Rect *		inSegmentRect,
		WETSMHiliteStyle	inHiliteStyle,
		WEReference			inWE ) ;

typedef CALLBACK_API ( SInt32,				WEPixelToCharProcPtr )
	(	const char *		inTextPtr,
		SInt32				inTextLength,
		Fixed				inSlop,
		Fixed *				ioPixelWidth,
		WEEdge *			outEdge,
		JustStyleCode		inStyleRunPosition,
		Fixed				inHorizontalPosition,
		WEReference			inWE ) ;

typedef CALLBACK_API ( SInt16,				WECharToPixelProcPtr )
	(	const char *		inTextPtr,
		SInt32				inTextLength,
		Fixed				inSlop,
		SInt32				inOffset,
		SInt16				inDirection,
		JustStyleCode		inStyleRunPosition,
		SInt16				inHorizontalOffset,
		WEReference			inWE ) ;

typedef CALLBACK_API ( StyledLineBreakCode,	WELineBreakProcPtr )
	(	const char *		inTextPtr,
		SInt32				inTextLength,
		SInt32				inTextStart,
		SInt32				inTextEnd,
		Fixed *				ioTextWidth,
		SInt32 *			ioTextOffset,
		WEReference			inWE ) ;

typedef CALLBACK_API ( void,				WEWordBreakProcPtr )
	(	const char *		inTextPtr,
		SInt16				inTextLength,
		SInt16				inOffset,
		WEEdge				inEdge,
		OffsetTable			outBreakOffsets,
		ScriptCode			inScript,
		WEReference			inWE ) ;

typedef CALLBACK_API ( SInt16,				WECharByteProcPtr )
	(	const char *		inTextPtr,
		SInt16				inTextOffset,
		ScriptCode			inScript,
		WEReference			inWE ) ;

typedef CALLBACK_API ( SInt16,				WECharTypeProcPtr )
	(	const char *		inTextPtr,
		SInt16				inTextOffset,
		ScriptCode			inScript,
		WEReference			inWE ) ;

typedef CALLBACK_API ( void, 				WEEraseProcPtr )
	(	const Rect *		inDirtyRect,
		WEReference			inWE ) ;

#endif /*WASTE_DEPRECATED*/

/*	WASTE callbacks: UPP types */

typedef WEClickLoopProcPtr			WEClickLoopUPP;
typedef WEScrollProcPtr				WEScrollUPP;
typedef WETSMPreUpdateProcPtr		WETSMPreUpdateUPP;
typedef WETSMPostUpdateProcPtr		WETSMPostUpdateUPP;
typedef WETextFilterProcPtr			WETextFilterUPP;
typedef WEPreTrackDragProcPtr		WEPreTrackDragUPP;
typedef WEPrepareViewProcPtr		WEPrepareViewUPP;
typedef WETranslateDragProcPtr		WETranslateDragUPP;
typedef WEHiliteDropAreaProcPtr		WEHiliteDropAreaUPP;
typedef WEFontIDToNameProcPtr		WEFontIDToNameUPP;
typedef WEFontNameToIDProcPtr		WEFontNameToIDUPP;
typedef WEBusyProcPtr				WEBusyUPP;
typedef WEFluxProcPtr				WEFluxUPP;
typedef WENewObjectProcPtr			WENewObjectUPP;
typedef WEDisposeObjectProcPtr		WEDisposeObjectUPP;
typedef WEDrawObjectProcPtr			WEDrawObjectUPP;
typedef WECGDrawObjectProcPtr		WECGDrawObjectUPP;
typedef WEClickObjectProcPtr		WEClickObjectUPP;
typedef WEStreamObjectProcPtr		WEStreamObjectUPP;
typedef WEHoverObjectProcPtr		WEHoverObjectUPP;
typedef WEUndoProcPtr				WEUndoUPP;
typedef WECGEraseProcPtr			WECGEraseUPP;

#if WASTE_DEPRECATED

typedef WEDrawTextProcPtr			WEDrawTextUPP;
typedef WEDrawTSMHiliteProcPtr		WEDrawTSMHiliteUPP;
typedef WEPixelToCharProcPtr		WEPixelToCharUPP;
typedef WECharToPixelProcPtr		WECharToPixelUPP;
typedef WELineBreakProcPtr			WELineBreakUPP;
typedef WEWordBreakProcPtr			WEWordBreakUPP;
typedef WECharByteProcPtr			WECharByteUPP;
typedef WECharTypeProcPtr			WECharTypeUPP;
typedef WEEraseProcPtr				WEEraseUPP;

#endif /*WASTE_DEPRECATED*/

/*	WASTE public calls */

/*	getting the shared library version number */

EXTERN_API ( UInt32 )
WEVersion (						void ) ;

/*	creating UPPs for callback functions */

EXTERN_API ( WEClickLoopUPP )
NewWEClickLoopUPP (				WEClickLoopProcPtr		inProcPtr ) ;

EXTERN_API ( WEScrollUPP )
NewWEScrollUPP (				WEScrollProcPtr			inProcPtr ) ;

EXTERN_API ( WETSMPreUpdateUPP )
NewWETSMPreUpdateUPP (			WETSMPreUpdateProcPtr	inProcPtr ) ;

EXTERN_API ( WETSMPostUpdateUPP )
NewWETSMPostUpdateUPP (			WETSMPostUpdateProcPtr	inProcPtr ) ;

EXTERN_API ( WETextFilterUPP )
NewWETextFilterUPP (			WETextFilterProcPtr		inProcPtr ) ;

EXTERN_API ( WEPreTrackDragUPP )
NewWEPreTrackDragUPP (			WEPreTrackDragProcPtr	inProcPtr ) ;

EXTERN_API ( WEPrepareViewUPP )
NewWEPrepareViewUPP (			WEPrepareViewProcPtr	inProcPtr ) ;

EXTERN_API ( WETranslateDragUPP )
NewWETranslateDragUPP (			WETranslateDragProcPtr	inProcPtr ) ;

EXTERN_API ( WEHiliteDropAreaUPP )
NewWEHiliteDropAreaUPP (		WEHiliteDropAreaProcPtr	inProcPtr ) ;

EXTERN_API ( WEFontIDToNameUPP )
NewWEFontIDToNameUPP (			WEFontIDToNameProcPtr	inProcPtr ) ;

EXTERN_API ( WEFontNameToIDUPP )
NewWEFontNameToIDUPP (			WEFontNameToIDProcPtr	inProcPtr ) ;

EXTERN_API ( WEBusyUPP )
NewWEBusyUPP (					WEBusyProcPtr			inProcPtr ) ;

EXTERN_API ( WEFluxUPP )
NewWEFluxUPP (					WEFluxProcPtr			inProcPtr ) ;

EXTERN_API ( WENewObjectUPP )
NewWENewObjectUPP (				WENewObjectProcPtr		inProcPtr ) ;

EXTERN_API ( WEDisposeObjectUPP )
NewWEDisposeObjectUPP (			WEDisposeObjectProcPtr	inProcPtr ) ;

EXTERN_API ( WECGDrawObjectUPP )
NewWECGDrawObjectUPP (			WECGDrawObjectProcPtr	inProcPtr ) ;

EXTERN_API ( WEClickObjectUPP )
NewWEClickObjectUPP (			WEClickObjectProcPtr	inProcPtr ) ;

EXTERN_API ( WEStreamObjectUPP )
NewWEStreamObjectUPP (			WEStreamObjectProcPtr	inProcPtr ) ;

EXTERN_API ( WEHoverObjectUPP )
NewWEHoverObjectUPP (			WEHoverObjectProcPtr	inProcPtr ) ;

EXTERN_API ( WEUndoUPP )
NewWEUndoUPP (					WEUndoProcPtr			inProcPtr ) ;

EXTERN_API ( WECGEraseUPP )
NewWECGEraseUPP (				WECGEraseProcPtr		inProcPtr ) ;

#if WASTE_DEPRECATED

EXTERN_API ( WEDrawTextUPP )
NewWEDrawTextUPP (				WEDrawTextProcPtr		inProcPtr ) ;

EXTERN_API ( WEDrawTSMHiliteUPP )
NewWEDrawTSMHiliteUPP (			WEDrawTSMHiliteProcPtr	inProcPtr ) ;

EXTERN_API ( WEPixelToCharUPP )
NewWEPixelToCharUPP (			WEPixelToCharProcPtr	inProcPtr ) ;

EXTERN_API ( WECharToPixelUPP )
NewWECharToPixelUPP (			WECharToPixelProcPtr	inProcPtr ) ;

EXTERN_API ( WELineBreakUPP )
NewWELineBreakUPP (				WELineBreakProcPtr		inProcPtr ) ;

EXTERN_API ( WEWordBreakUPP )
NewWEWordBreakUPP (				WEWordBreakProcPtr		inProcPtr ) ;

EXTERN_API ( WECharByteUPP )
NewWECharByteUPP (				WECharByteProcPtr		inProcPtr ) ;

EXTERN_API ( WECharTypeUPP )
NewWECharTypeUPP (				WECharTypeProcPtr		inProcPtr ) ;

EXTERN_API ( WEEraseUPP )
NewWEEraseUPP (					WEEraseProcPtr			inProcPtr ) ;

EXTERN_API ( WEDrawObjectUPP )
NewWEDrawObjectUPP (			WEDrawObjectProcPtr		inProcPtr ) ;

#endif /*WASTE_DEPRECATED*/

/*	destroying UPPs for callback functions */

EXTERN_API ( void )
DisposeWEClickLoopUPP (			WEClickLoopUPP			inUPP ) ;

EXTERN_API ( void )
DisposeWEScrollUPP (			WEScrollUPP				inUPP ) ;

EXTERN_API ( void )
DisposeWETSMPreUpdateUPP (		WETSMPreUpdateUPP		inUPP ) ;

EXTERN_API ( void )
DisposeWETSMPostUpdateUPP (		WETSMPostUpdateUPP		inUPP ) ;

EXTERN_API ( void )
DisposeWETextFilterUPP (		WETextFilterUPP			inUPP ) ;

EXTERN_API ( void )
DisposeWEPreTrackDragUPP (		WEPreTrackDragUPP		inUPP ) ;

EXTERN_API ( void )
DisposeWEPrepareViewUPP (		WEPrepareViewUPP		inUPP ) ;

EXTERN_API ( void )
DisposeWETranslateDragUPP (		WETranslateDragUPP		inUPP ) ;

EXTERN_API ( void )
DisposeWEHiliteDropAreaUPP (	WEHiliteDropAreaUPP		inUPP ) ;

EXTERN_API ( void )
DisposeWEFontIDToNameUPP (		WEFontIDToNameUPP		inUPP ) ;

EXTERN_API ( void )
DisposeWEFontNameToIDUPP (		WEFontNameToIDUPP		inUPP ) ;

EXTERN_API ( void )
DisposeWEBusyUPP (				WEBusyUPP				inUPP ) ;

EXTERN_API ( void )
DisposeWEFluxUPP (				WEFluxUPP				inUPP ) ;

EXTERN_API ( void )
DisposeWENewObjectUPP (			WENewObjectUPP			inUPP ) ;

EXTERN_API ( void )
DisposeWEDisposeObjectUPP (		WEDisposeObjectUPP		inUPP ) ;

EXTERN_API ( void )
DisposeWECGDrawObjectUPP (		WECGDrawObjectUPP		inUPP ) ;

EXTERN_API ( void )
DisposeWEClickObjectUPP (		WEClickObjectUPP		inUPP ) ;

EXTERN_API ( void )
DisposeWEStreamObjectUPP (		WEStreamObjectUPP		inUPP ) ;

EXTERN_API ( void )
DisposeWEHoverObjectUPP (		WEHoverObjectUPP		inUPP ) ;

EXTERN_API ( void )
DisposeWEUndoUPP (				WEUndoUPP				inUPP ) ;

EXTERN_API ( void )
DisposeWECGEraseUPP (			WECGEraseUPP			inUPP ) ;

#if WASTE_DEPRECATED

EXTERN_API ( void )
DisposeWEDrawTextUPP (			WEDrawTextUPP			inUPP ) ;

EXTERN_API ( void )
DisposeWEDrawTSMHiliteUPP (		WEDrawTSMHiliteUPP		inUPP ) ;

EXTERN_API ( void )
DisposeWEPixelToCharUPP (		WEPixelToCharUPP		inUPP ) ;

EXTERN_API ( void )
DisposeWECharToPixelUPP (		WECharToPixelUPP		inUPP ) ;

EXTERN_API ( void )
DisposeWELineBreakUPP (			WELineBreakUPP			inUPP ) ;

EXTERN_API ( void )
DisposeWEWordBreakUPP (			WEWordBreakUPP			inUPP ) ;

EXTERN_API ( void )
DisposeWECharByteUPP (			WECharByteUPP			inUPP ) ;

EXTERN_API ( void )
DisposeWECharTypeUPP (			WECharTypeUPP			inUPP ) ;

EXTERN_API ( void )
DisposeWEEraseUPP (				WEEraseUPP				inUPP ) ;

EXTERN_API ( void )
DisposeWEDrawObjectUPP (		WEDrawObjectUPP			inUPP ) ;

#endif /*WASTE_DEPRECATED*/

/*	invoking callback functions through UPPs */

EXTERN_API ( Boolean )
InvokeWEClickLoopUPP (			WEReference				inWE,
								WEClickLoopUPP			inUPP ) ;

EXTERN_API ( void )
InvokeWEScrollUPP (				WEReference				inWE,
								WEScrollUPP				inUPP ) ;

EXTERN_API ( void )
InvokeWETSMPreUpdateUPP (		WEReference				inWE,
								WETSMPreUpdateUPP		inUPP ) ;

EXTERN_API ( void )
InvokeWETSMPostUpdateUPP (		WEReference				inWE,
								SInt32					inFixLength,
								SInt32					inInputAreaStart,
								SInt32					inInputAreaEnd,
								SInt32					inPinRangeStart,
								SInt32					inPinRangeEnd,
								WETSMPostUpdateUPP		inUPP ) ;

EXTERN_API ( Boolean )
InvokeWETextFilterUPP (			ComponentInstance		inTextInputComponent,
								Handle					inFilterableText,
								UInt32					inModifiers,
								UInt32					inKeyCode,
								WEReference				inWE,
								WETextFilterUPP			inUPP ) ;

EXTERN_API ( OSErr )
InvokeWEPreTrackDragUPP (		DragRef					inDrag,
								WEReference				inWE,
								WEPreTrackDragUPP		inUPP ) ;

EXTERN_API ( void )
InvokeWEPrepareViewUPP (		WEViewReference			inView,
								WEPrepareViewUPP		inUPP ) ;

EXTERN_API ( OSErr )
InvokeWETranslateDragUPP (		DragRef					inDrag,
								DragItemRef				inDragItem,
								FlavorType				inRequestedType,
								Handle					outData,
								SInt32					inDropOffset,
								WEReference				inWE,
								WETranslateDragUPP		inUPP ) ;

EXTERN_API ( OSErr )
InvokeWEHiliteDropAreaUPP (		DragRef					inDrag,
								Boolean					inHiliteFlag,
								WEReference				inWE,
								WEHiliteDropAreaUPP		inUPP ) ;

EXTERN_API ( OSErr )
InvokeWEFontIDToNameUPP (		FMFontFamily			inFontFamilyNumber,
								StringPtr				ioQDFontName,
								WEFontIDToNameUPP		inUPP ) ;

EXTERN_API ( OSErr )
InvokeWEFontNameToIDUPP (		ConstStr255Param		inQDFontName,
								FMFontFamily			inOldFontFamilyNumber,
								FMFontFamily *			outNewFontFamilyNumber,
								WEFontNameToIDUPP		inUPP ) ;

EXTERN_API ( OSErr )
InvokeWEBusyUPP (				SInt16					inBusyAction,
								const Float32 *			inPercentDone,
								WEReference				inWE,
								WEBusyUPP				inUPP ) ;

EXTERN_API ( void )
InvokeWEFluxUPP (				SInt32					inOffset,
								SInt32					inDelta,
								WEReference				inWE,
								WEFluxUPP				inUPP ) ;

EXTERN_API ( OSErr )
InvokeWENewObjectUPP (			WEObjectPlacement *		outObjectPlacement,
								WEObjectReference		inObject,
								WENewObjectUPP			inUPP ) ;

EXTERN_API ( OSErr )
InvokeWEDisposeObjectUPP (		WEObjectReference		inObject,
								WEDisposeObjectUPP		inUPP ) ;

EXTERN_API ( OSErr )
InvokeWECGDrawObjectUPP (		CGContextRef			inCGContext,
								CGRect					inBounds,
								OptionBits				inOptions,
								WEObjectReference		inObject,
								WECGDrawObjectUPP		inUPP ) ;

EXTERN_API ( Boolean )
InvokeWEClickObjectUPP (		Point					inHitPoint,
								EventModifiers			inModifiers,
								UInt32					inClickTime,
								WEObjectReference		inObject,
								WEClickObjectUPP		inUPP ) ;

EXTERN_API ( OSErr )
InvokeWEStreamObjectUPP (		SInt16					inDestinationKind,
								FlavorType *			outStreamedFlavorType,
								Handle					outStreamedData,
								WEObjectReference		inObject,
								WEStreamObjectUPP		inUPP ) ;

EXTERN_API ( OSErr )
InvokeWEHoverObjectUPP (		SInt16					inMouseAction,
								Point					inMouseLoc,
								RgnHandle				inMouseRgn,
								WEObjectReference		inObject,
								WEHoverObjectUPP		inUPP ) ;

EXTERN_API ( OSErr )
InvokeWEUndoUPP (				SInt16					inUndoEvent,
								WEActionReference		inAction,
								WEUndoUPP				inUPP ) ;

EXTERN_API ( void )
InvokeWECGEraseUPP (			CGContextRef			inCGContext,
								CGRect					inDirtyRect,
								WEViewReference			inView,
								WEReference				inWE,
								WECGEraseUPP			inUPP ) ;

#if WASTE_DEPRECATED

EXTERN_API ( void )
InvokeWEDrawTextUPP (			const char *			inTextPtr,
								SInt32					inTextLength,
								Fixed					inSlop,
								JustStyleCode			inStyleRunPosition,
								WEReference				inWE,
								WEDrawTextUPP			inUPP ) ;

EXTERN_API ( void )
InvokeWEDrawTSMHiliteUPP (		const Rect *			inSegmentRect,
								WETSMHiliteStyle		inHiliteStyle,
								WEReference				inWE,
								WEDrawTSMHiliteUPP		inUPP ) ;

EXTERN_API ( SInt32 )
InvokeWEPixelToCharUPP (		const char *			inTextPtr,
								SInt32					inTextLength,
								Fixed					inSlop,
								Fixed *					ioPixelWidth,
								WEEdge *				outEdge,
								JustStyleCode			inStyleRunPosition,
								Fixed					inHorizontalPosition,
								WEReference				inWE,
								WEPixelToCharUPP		inUPP ) ;

EXTERN_API ( SInt16 )
InvokeWECharToPixelUPP (		const char *			inTextPtr,
								SInt32					inTextLength,
								Fixed					inSlop,
								SInt32					inOffset,
								SInt16					inDirection,
								JustStyleCode			inStyleRunPosition,
								SInt16					inHorizontalPosition,
								WEReference				inWE,
								WECharToPixelUPP		inUPP ) ;

EXTERN_API ( StyledLineBreakCode )
InvokeWELineBreakUPP (			const char *			inTextPtr,
								SInt32					inTextLength,
								SInt32					inTextStart,
								SInt32					inTextEnd,
								Fixed *					ioTextWidth,
								SInt32 *				ioTextOffset,
								WEReference				inWE,
								WELineBreakUPP			inUPP ) ;

EXTERN_API ( void )
InvokeWEWordBreakUPP (			const char *			inTextPtr,
								SInt16					inTextLength,
								SInt16					inOffset,
								WEEdge					inEdge,
								OffsetTable				outBreakOffsets,
								ScriptCode				inScript,
								WEReference				inWE,
								WEWordBreakUPP			inUPP ) ;

EXTERN_API ( SInt16 )
InvokeWECharByteUPP (			const char *			inTextPtr,
								SInt16					inTextOffset,
								ScriptCode				inScript,
								WEReference				inWE,
								WECharByteUPP			inUPP ) ;

EXTERN_API ( SInt16 )
InvokeWECharTypeUPP (			const char *			inTextPtr,
								SInt16					inTextOffset,
								ScriptCode				inScript,
								WEReference				inWE,
								WECharTypeUPP			inUPP ) ;

EXTERN_API ( void )
InvokeWEEraseUPP (				const Rect *			inDirtyRect,
								WEReference				inWE,
								WEEraseUPP				inUPP ) ;

EXTERN_API ( OSErr )
InvokeWEDrawObjectUPP (			const Rect *			inDestRect,
								WEObjectReference		inObject,
								WEDrawObjectUPP			inUPP ) ;

#endif /*WASTE_DEPRECATED*/

/*	creation and destruction */

EXTERN_API ( OSErr )
WENew (							const LongRect *		inDestRect,
								const LongRect *		inViewRect,
								OptionBits				inOptions,
								WEReference *			outWE ) ;

EXTERN_API ( void )
WEDispose (						WEReference				inWE ) ;

/*	getting variables */

EXTERN_API ( SInt16 )
WEGetChar ( 					SInt32					inOffset,
								WEReference 			inWE ) ;

EXTERN_API ( SInt32 )
WEGetTextLength ( 				WEReference 			inWE ) ;

EXTERN_API ( void )
WEGetSelection (				SInt32 *				outSelStart,
								SInt32 *				outSelEnd,
								WEReference				inWE ) ;

EXTERN_API ( SInt32 )
WEGetSelectionAnchor (			WEReference				inWE ) ;

EXTERN_API ( void )
WEGetDestRect (					LongRect *				outDestRect,
								WEReference 			inWE ) ;

EXTERN_API ( void )
WEGetViewRect (					LongRect *				outViewRect,
								WEReference 			inWE ) ;

EXTERN_API ( Boolean )
WEIsActive (					WEReference 			inWE ) ;

EXTERN_API ( UInt16 )
WEGetClickCount (				WEReference 			inWE ) ;

EXTERN_API ( Fixed )
WEGetDefaultTabWidth (			WEReference				inWE ) ;

EXTERN_API ( void )
WESetDefaultTabWidth (			Fixed					inDefaultTabWidth,
								WEReference				inWE ) ;

/*	setting variables */

EXTERN_API ( void )
WESetSelection (				SInt32					inSelStart,
								SInt32					inSelEnd,
								WEReference				inWE ) ;

EXTERN_API ( void )
WESelectAll (					WEReference				inWE ) ;

EXTERN_API ( void )
WESetDestRect (					const LongRect *		inDestRect,
								WEReference 			inWE ) ;

EXTERN_API ( void )
WESetViewRect (					const LongRect *		inViewRect,
								WEReference				inWE ) ;

/*	creating and destroying views */

EXTERN_API ( OSStatus )
WENewView (						CGrafPtr				inQDPort,
								const LongRect *		inDestRect,
								const LongRect *		inViewRect,
								WEReference				inWE,
								WEViewReference *		outView ) ;

EXTERN_API ( OSStatus )
WENewViewWithCGContext (		CGContextRef			inCGContext,
								CGRect					inDestRect,
								CGRect					inViewRect,
								WEReference				inWE,
								WEViewReference *		outView ) ;

EXTERN_API ( void )
WEDisposeView (					WEViewReference			inView ) ;

/*	working with views */

EXTERN_API ( WEReference )
WEGetViewOwner (				WEViewReference			inView ) ;

EXTERN_API ( WEViewReference )
WEGetCurrentView (				WEReference				inWE ) ;

EXTERN_API ( void )
WESetCurrentView (				WEViewReference			inView ) ;

EXTERN_API ( OSStatus )
WEGetViewInfo (					WESelector				inSelector,
								void *					outInfo,
								WEViewReference			inView ) ;

EXTERN_API ( OSStatus )
WESetViewInfo (					WESelector				inSelector,
								const void *			inInfo,
								WEViewReference			inView ) ;

EXTERN_API ( OSStatus )
WEGetViewProperty (				OSType					inPropertyCreator,
								OSType					inPropertyTag,
								ByteCount				inPropertySize,
								ByteCount *				outActualSize,		/* can be NULL */
								void *					outPropertyBuffer,
								WEViewReference			inView ) ;

EXTERN_API ( OSStatus )
WESetViewProperty (				OSType					inPropertyCreator,
								OSType					inPropertyTag,
								ByteCount				inPropertySize,
								const void *			inPropertyBuffer,
								WEViewReference			inView ) ;

EXTERN_API ( OSStatus )
WERemoveViewProperty (			OSType					inPropertyCreator,
								OSType					inPropertyTag,
								WEViewReference			inView ) ;

/*	getting style attributes */

EXTERN_API ( OSErr )
WEGetAttributes (				SInt32					inOffset,
								ItemCount				inAttributeCount,
								const WESelector		inAttributeSelectors [ ],
								void * const			outAttributeValues [ ],
								const ByteCount			inAttributeValueSizes [ ],
								WEReference				inWE ) ;

EXTERN_API ( OSErr )
WEGetOneAttribute (				SInt32					inOffset,
								WESelector				inAttributeSelector,
								void *					outAttributeValue,
								ByteCount				inAttributeValueSize,
								WEReference				inWE ) ;

/*	matching attributes over a text range against an arbitrary set of values */

EXTERN_API ( OSErr )
WEMatchAttributes (				SInt32					inRangeStart,
								SInt32					inRangeEnd,
								WESelector				inAttributeSelector,
								ByteCount				inAttributeValueSize,
								ItemCount				inArraySize,
								const void *			inValueArray,
								Boolean					outWhichValuesArePresent [ ],
								Boolean *				outIsContinuous,
								WEReference				inWE ) ;

/*	low-level access to style run information */

EXTERN_API ( SInt32 )
WECountRuns (					WEReference 			inWE ) ;

EXTERN_API ( SInt32 )
WEOffsetToRun (					SInt32					inOffset,
								WEReference				inWE ) ;

EXTERN_API ( void )
WEGetRunRange (					SInt32					inStyleRunIndex,
								SInt32 *				outStyleRunStart,
								SInt32 *				outStyleRunEnd,
								WEReference				inWE ) ;

#if WASTE_DEPRECATED

/*	WEGetRunInfo is deprecated in WASTE 3.0 -- please use WEGetAttributes instead */

EXTERN_API ( void )
WEGetRunInfo (					SInt32					inOffset,
								WERunInfo *				outStyleRunInfo,
								WEReference				inWE ) ;

/*	WEGetIndRunInfo is deprecated in WASTE 3.0 -- please use WEOffsetToRun/WEGetAttributes instead */

EXTERN_API ( void )
WEGetIndRunInfo (				SInt32					inStyleRunIndex,
								WERunInfo *				outStyleRunInfo,
								WEReference				inWE ) ;

#endif /*WASTE_DEPRECATED*/

/*	low-level access to paragraph run information */

EXTERN_API ( SInt32 )
WECountParaRuns (				WEReference				inWE ) ;

EXTERN_API ( SInt32 )
WEOffsetToParaRun (				SInt32					inOffset,
								WEReference				inWE ) ;

EXTERN_API ( void )
WEGetParaRunRange (				SInt32					inParagraphRunIndex,
								SInt32 *				outParagraphRunStart,
								SInt32 *				outParagraphRunEnd,
								WEReference				inWE ) ;

/*	access to line layout information */

EXTERN_API ( SInt32 )
WECountLines (					WEReference 			inWE ) ;

EXTERN_API ( SInt32 )
WEOffsetToLine (				SInt32 					inOffset,
								WEReference 			inWE ) ;

EXTERN_API ( void )
WEGetLineRange (				SInt32					inLineIndex,
								SInt32 *				outLineStart,
								SInt32 *				outLineEnd,
								WEReference				inWE ) ;

EXTERN_API ( SInt32 )
WEGetHeight (					SInt32					inStartLineIndex,
								SInt32					inEndLineIndex,
								WEReference 			inWE ) ;

EXTERN_API ( SInt32 )
WEGetLineWidth (				SInt32					inLineIndex,
								WEReference				inWE ) ;

EXTERN_API ( SInt16 )
WEGetLineAscent (				SInt32					inLineIndex,
								WEReference				inWE ) ;

EXTERN_API ( SInt32 )
WEGetMaxLineWidth (				WEReference				inWE ) ;

/*	converting byte offsets to screen position and vice versa */

EXTERN_API ( SInt32 )
WEGetOffset (					const LongPt *			inPoint,
								WEEdge *				outEdge,
								WEReference				inWE ) ;

EXTERN_API ( void )
WEGetPoint (					SInt32					inOffset,
								SInt16					inDirection,
								LongPt *				outPoint,
								SInt16 *				outLineHeight,
								WEReference				inWE ) ;

/*	finding words, lines and paragraphs */

EXTERN_API ( void )
WEFindWord (					SInt32					inOffset,
								WEEdge					inEdge,
								SInt32 *				outWordStart,
								SInt32 *				outWordEnd,
								WEReference				inWE ) ;

EXTERN_API ( void )
WEFindLine (					SInt32					inOffset,
								WEEdge					inEdge,
								SInt32 *				outLineStart,
								SInt32 *				outLineEnd,
								WEReference				inWE ) ;

EXTERN_API ( void )
WEFindParagraph (				SInt32					inOffset,
								WEEdge					inEdge,
								SInt32 *				outParagraphStart,
								SInt32 *				outParagraphEnd,
								WEReference				inWE ) ;

/*	matching strings */

EXTERN_API ( OSErr )
WEFind (						const char *			inKey,
								SInt32					inKeyLength,
								TextEncoding			inKeyEncoding,
								OptionBits				inMatchOptions,
								SInt32					inRangeStart,
								SInt32					inRangeEnd,
								SInt32 *				outMatchStart,
								SInt32 *				outMatchEnd,
								WEReference				inWE ) ;

/*	making a copy of a text range */

EXTERN_API ( OSErr )
WEStreamRange (					SInt32					inRangeStart,
								SInt32					inRangeEnd,
								FlavorType				inRequestedType,
								OptionBits				inStreamOptions,
								Handle					outData,
								WEReference				inWE ) ;

#if WASTE_DEPRECATED

/*	WECopyRange is deprecated in WASTE 3.0 -- please use WEStreamRange instead */

EXTERN_API ( OSErr )
WECopyRange (					SInt32					inRangeStart,
								SInt32					inRangeEnd,
								Handle					outText,
								StScrpHandle			outStyles,
								WESoupHandle			outSoup,
								WEReference				inWE ) ;

/*	WEGetTextRangeAsUnicode() is only partially supported in WASTE 3.0 */
/*	For most purposes, you can probably use WEStreamRange() with the kTypeUnicodeText selector. */

EXTERN_API ( OSErr )
WEGetTextRangeAsUnicode (		SInt32					inRangeStart,
								SInt32					inRangeEnd,
								Handle					outUnicodeText,
								Handle					ioCharFormat,
								Handle					ioParaFormat,
								TextEncodingVariant		inUnicodeVariant,
								TextEncodingFormat		inTransformationFormat,
								OptionBits				inGetOptions,
								WEReference				inWE )
								DEPRECATED_ATTRIBUTE ;

#endif /*WASTE_DEPRECATED*/

/*	recalculating line breaks, drawing and scrolling */

EXTERN_API ( OSErr )
WECalText (						WEReference 			inWE ) ;

EXTERN_API ( void )
WEUpdate (						RgnHandle				inUpdateRgn,
								WEReference 			inWE ) ;

EXTERN_API ( void )
WEScroll (						SInt32					inHorizontalOffset,
								SInt32					inVerticalOffset,
								WEReference				inWE ) ;

EXTERN_API ( void )
WEPinScroll (					SInt32					inHorizontalOffset,
								SInt32					inVerticalOffset,
								WEReference				inWE ) ;

EXTERN_API ( void )
WESelView (						WEReference				inWE ) ;

/*	handling activate / deactivate events */

EXTERN_API ( void )
WEActivate (					WEReference				inWE ) ;

EXTERN_API ( void )
WEDeactivate (					WEReference 			inWE ) ;

/* 	handling key-down events */

#if WASTE_DEPRECATED

/*	WEKey is deprecated in WASTE 3.0 -- instead of feeding raw keyboard events to WEKey, you should install */
/*	text input event handlers on an event target by using WESetEventTarget */

EXTERN_API ( void )
WEKey (							CharParameter			inKey,
								EventModifiers			inModifiers,
								WEReference				inWE )
								DEPRECATED_ATTRIBUTE ;

#endif /*WASTE_DEPRECATED*/

/*	installing Carbon event handlers */

EXTERN_API ( OSStatus )
WESetEventTarget (				EventTargetRef			inEventTarget,
								OptionBits				inSetEventTargetOptions,
								WEReference				inWE ) ;

/*	handling mouse-down events and mouse tracking */

EXTERN_API ( void )
WEClick (						Point					inHitPoint,
								EventModifiers			inModifiers,
								UInt32					inClickTime,
								WEReference				inWE ) ;

/*	processing HI commands */

EXTERN_API ( OSStatus )
WEProcessHICommand (			const HICommand *		inHICommand,
								WEReference				inWE ) ;

/*	adjusting the cursor shape */

EXTERN_API ( Boolean )
WEAdjustCursor (				Point					inMouseLoc,
								RgnHandle				ioMouseRgn,
								WEReference				inWE ) ;

#if WASTE_DEPRECATED

/*	blinking the caret */
/*	this API is a no-op in WASTE 3.0 -- CPU time is obtained automatically when needed by installing suitable Carbon event timers */

EXTERN_API ( void )
WEIdle (						UInt32 *				outMaxSleep,
								WEReference				inWE )
								DEPRECATED_ATTRIBUTE ;

#endif /*WASTE_DEPRECATED*/

/*	modifying the text and the styles */

EXTERN_API ( OSErr )
WEPut (							SInt32					inRangeStart,
								SInt32					inRangeEnd,
								const void *			inTextPtr,
								SInt32					inTextLength,
								TextEncoding			inTextEncoding,
								OptionBits				inPutOptions,
								ItemCount				inFlavorCount,
								const FlavorType *		inFlavorTypes,
								const Handle *			inFlavorHandles,
								WEReference				inWE ) ;

EXTERN_API ( OSErr )
WEDelete (						WEReference				inWE ) ;

#if WASTE_DEPRECATED

/*	WEInsert is deprecated in WASTE 3.0 -- please use WEPut instead */

EXTERN_API ( OSErr )
WEInsert (						const void *			inTextPtr,
								SInt32					inTextLength,
								StScrpHandle			inStyles,
								WESoupHandle			inSoup,
								WEReference				inWE ) ;

/*	WEUseText is deprecated in WASTE 3.0 -- please use WEPut instead */

EXTERN_API ( OSErr )
WEUseText (						Handle					inText,
								WEReference				inWE )
								DEPRECATED_ATTRIBUTE ;

#endif /*WASTE_DEPRECATED*/

EXTERN_API ( OSErr )
WEChangeCase (					SInt16					inCase,
								WEReference				inWE ) ;

/*	modifying text attributes */

EXTERN_API ( OSErr )
WESetAttributes (				SInt32					inRangeStart,
								SInt32					inRangeEnd,
								ItemCount				inAttributeCount,
								const WESelector		inAttributeSelectors [ ],
								const void * const		inAttributeValues [ ],
								const ByteCount			inAttributeValueSizes [ ],
								WEReference				inWE ) ;

EXTERN_API ( OSErr )
WESetOneAttribute (				SInt32					inRangeStart,
								SInt32					inRangeEnd,
								WESelector				inAttributeSelector,
								const void *			inAttributeValue,
								ByteCount				inAttributeValueSize,
								WEReference				inWE ) ;

/*	registering custom attributes */

EXTERN_API ( OSStatus )
WERegisterCustomAttribute (		WESelector				inAttributeSelector,
								OptionBits				inAttributeOptions,
								ByteCount				inAttributeValueSize,
								DescType				inAttributeType,
								WEReference				inWE ) ;

#if WASTE_DEPRECATED

/*	WEUseStyleScrap is deprecated in WASTE 3.0 -- please use WEPut instead */

EXTERN_API ( OSErr )
WEUseStyleScrap (				StScrpHandle			inStyles,
								WEReference				inWE )
								DEPRECATED_ATTRIBUTE ;

#endif /*WASTE_DEPRECATED*/

/*	undo */

EXTERN_API ( OSErr )
WEUndo (						WEReference				inWE ) ;

EXTERN_API ( OSErr )
WERedo (						WEReference				inWE ) ;

EXTERN_API ( void )
WEClearUndo (					WEReference				inWE ) ;

EXTERN_API ( WEActionKind )
WEGetUndoInfo (					Boolean *				outRedoFlag,
								WEReference 			inWE ) ;

EXTERN_API ( WEActionKind )
WEGetIndUndoInfo (				SInt32					inUndoLevel,
								WEReference				inWE ) ;

EXTERN_API ( Boolean )
WEIsTyping (					WEReference				inWE ) ;

EXTERN_API ( OSStatus )
WEBreakTypingSequence (			WEReference				inWE ) ;

EXTERN_API ( OSErr )
WEBeginAction (					WEReference				inWE ) ;

EXTERN_API ( OSErr )
WEEndAction (					WEActionKind			inActionKind,
								WEReference				inWE ) ;

/*	undo action accessors */

EXTERN_API ( WEActionKind )
WEGetActionKind (				WEActionReference		inAction ) ;

EXTERN_API ( Boolean )
WEActionIsRedo (				WEActionReference		inAction ) ;

EXTERN_API ( WEReference )
WEGetActionOwner (				WEActionReference		inAction ) ;

EXTERN_API ( CFStringRef )
WEGetActionUndoString (			WEActionReference		inAction ) ;

EXTERN_API ( void )
WESetActionUndoString (			CFStringRef				inUndoString,
								WEActionReference		inAction ) ;

/*	keeping track of changes */

EXTERN_API ( UInt32 )
WEGetModCount (					WEReference				inWE ) ;

EXTERN_API ( void )
WEResetModCount (				WEReference				inWE ) ;

/*	embedded objects */

EXTERN_API ( OSErr )
WEInstallObjectHandler (		FlavorType				inObjectType,
								WESelector				inHandlerSelector,
								UniversalProcPtr		inHandler,
								WEReference				inWE ) ;

EXTERN_API ( OSErr )
WEInstallStandardObjectHandlers (	FlavorType			inObjectType,
								WEReference				inWE ) ;

EXTERN_API ( OSErr )
WERemoveObjectHandler (			FlavorType				inObjectType,
								WESelector				inHandlerSelector,
								UniversalProcPtr		inHandler,
								WEReference				inWE ) ;

EXTERN_API ( OSErr )
WERemoveStandardObjectHandlers (	FlavorType			inObjectType,
								WEReference				inWE ) ;

EXTERN_API ( OSErr )
WEGetObjectHandler (			FlavorType				inObjecType,
								WESelector				inHandlerSelector,
								UniversalProcPtr *		outHandler,
								WEReference				inWE ) ;

EXTERN_API ( OSErr )
WEInsertObject (				FlavorType				inObjectType,
								Handle					inObjectDataHandle,
								Point					inObjectSize,
								WEReference				inWE ) ;

EXTERN_API ( OSErr )
WEGetSelectedObject (			WEObjectReference *		outObject,
								WEReference				inWE ) ;

EXTERN_API ( OSErr )
WEGetObjectAtOffset (			SInt32					inOffset,
								WEObjectReference *		outObject,
								WEReference				inWE ) ;

EXTERN_API ( SInt32 )
WEFindNextObject (				SInt32					inOffset,
								WEObjectReference *		outObject,
								WEReference				inWE ) ;

EXTERN_API ( SInt32 )
WEFindPreviousObject (			SInt32					inOffset,
								WEObjectReference *		outObject,
								WEReference				inWE ) ;

EXTERN_API ( OSErr )
WEUseSoup (						WESoupHandle			inSoup,
								WEReference				inWE ) ;

/*	accessing embedded object attributes */

EXTERN_API ( FlavorType )
WEGetObjectType (				WEObjectReference		inObject ) ;

EXTERN_API ( Handle )
WEGetObjectDataHandle (			WEObjectReference		inObject ) ;

EXTERN_API ( WEReference )
WEGetObjectOwner (				WEObjectReference		inObject ) ;

EXTERN_API ( SInt32 )
WEGetObjectOffset (				WEObjectReference		inObject ) ;

EXTERN_API ( Point )
WEGetObjectSize (				WEObjectReference		inObject ) ;

EXTERN_API ( OSErr )
WESetObjectSize (				WEObjectReference		inObject,
								Point					inObjectSize ) ;

EXTERN_API ( SInt16 )
WEGetObjectBaseline (			WEObjectReference		inObject ) ;

EXTERN_API ( OSErr )
WESetObjectBaseline (			WEObjectReference		inObject,
								SInt16					inObjectBaseline ) ;

EXTERN_API ( OSErr )
WESetObjectPlacement (			WEObjectReference		inObject,
								const WEObjectPlacement *	inPlacement ) ;

EXTERN_API ( OSErr )
WEGetObjectFrame (				WEObjectReference		inObject,
								LongRect *				outObjectFrame ) ;

EXTERN_API ( SInt32 )
WEGetObjectRefCon (				WEObjectReference		inObject ) ;

EXTERN_API ( void )
WESetObjectRefCon (				WEObjectReference		inObject,
								SInt32					inRefCon ) ;

EXTERN_API ( OSStatus )
WEGetObjectProperty (			WEObjectReference		inObject,
								OSType					inPropertyCreator,
								OSType					inPropertyTag,
								ByteCount				inPropertySize,
								ByteCount *				outActualSize,		/* can be NULL */
								void *					outPropertyBuffer ) ;

EXTERN_API ( OSStatus )
WESetObjectProperty (			WEObjectReference		inObject,
								OSType					inPropertyCreator,
								OSType					inPropertyTag,
								ByteCount				inPropertySize,
								const void *			inPropertyBuffer ) ;

EXTERN_API ( OSStatus )
WERemoveObjectProperty (		WEObjectReference		inObject,
								OSType					inPropertyCreator,
								OSType					inPropertyTag ) ;

/*	clipboard operations */

EXTERN_API ( OSErr )
WECut (							WEReference				inWE ) ;

EXTERN_API ( OSErr )
WECopy (						WEReference				inWE ) ;

EXTERN_API ( OSStatus )
WECopyToScrap (					ScrapRef				inScrap,			/* can be NULL */
								const ScrapFlavorType *	inScrapFlavors,		/* can be NULL */
								OptionBits				inCopyOptions,
								WEReference				inWE ) ;

EXTERN_API ( OSErr )
WEPaste (						WEReference				inWE ) ;

EXTERN_API ( OSStatus )
WEPasteFromScrap (				ScrapRef				inScrap,
								OptionBits				inPasteOptions,
								WEReference				inWE ) ;

EXTERN_API ( Boolean )
WECanPaste (					WEReference				inWE ) ;

EXTERN_API ( Boolean )
WECanPasteFromScrap (			ScrapRef				inScrap,
								OptionBits				inPasteOptions,
								WEReference				inWE ) ;

/*	Drag Manager support */

EXTERN_API ( RgnHandle )
WEGetHiliteRgn (				SInt32					inRangeStart,
								SInt32					inRangeEnd,
								WEReference				inWE ) ;

EXTERN_API ( OSErr )
WETrackDrag (					DragTrackingMessage		inMessage,
								DragRef					inDrag,
								WEReference				inWE ) ;

EXTERN_API ( OSErr )
WEReceiveDrag (					DragRef					inDrag,
								WEReference 			inWE ) ;

EXTERN_API ( Boolean )
WECanAcceptDrag (				DragRef					inDrag,
								WEReference				inWE ) ;

EXTERN_API ( Boolean )
WEDraggedToTrash (				DragRef					inDrag ) ;

EXTERN_API ( WEViewReference )
WEGetDragSourceView (			DragRef					inDrag ) ;

/*	font tables */

EXTERN_API ( OSErr )
WEBuildFontTable (				WEFontTableHandle		outFontTable,
								WEFontIDToNameUPP		inFontIDToNameProc,
								WEReference				inWE ) ;

EXTERN_API ( OSErr )
WEUpdateFontTable (				WEFontTableHandle		ioFontTable,
								WEFontNameToIDUPP		inFontNameToIDProc,
								Boolean *				outWasChanged ) ;

EXTERN_API ( OSErr )
WEUpdateStyleScrap (			StScrpHandle			ioStyles,
								WEFontTableHandle		inFontTable ) ;

/*	Unicode utilities */

EXTERN_API ( OSStatus )
WEGetCharacterProperty (		SInt32					inOffset,
								UCCharPropertyType		inPropertyType,
								UCCharPropertyValue *	outPropertyValue,
								WEReference				inWE ) ;

/*	Script Manager utilities */

#if WASTE_DEPRECATED

/*	WECharByte is deprecated in WASTE 3.0 since the underlying text is stored in UTF-16 format */
/*	It can still be used to test whether the UTF-16 code unit at the specified offset is a low surrogate, */
/*	a high surrogate, or not a surrogate. */

EXTERN_API ( SInt16 )
WECharByte (					SInt32					inOffset,
								WEReference				inWE )
								DEPRECATED_ATTRIBUTE ;

/*	WECharType is deprecated in WASTE 3.0 -- please use WEGetCharacterProperty instead */

EXTERN_API ( SInt16 )
WECharType (					SInt32					inOffset,
								WEReference				inWE )
								DEPRECATED_ATTRIBUTE ;

/*	Text Services Manager support */

/*	WEInstallTSMHandlers, WERemoveTSMHandlers and WEHandleTSMEvents are deprecated and no-ops in WASTE 3.0 */
/*	TSM Apple event handlers have been superseded by text input Carbon event handlers, installed using WESetEventTarget. */

EXTERN_API ( OSErr )
WEInstallTSMHandlers ( 			void )
								DEPRECATED_ATTRIBUTE ;

EXTERN_API ( OSErr )
WERemoveTSMHandlers (			void )
								DEPRECATED_ATTRIBUTE ;

EXTERN_API ( OSErr )
WEHandleTSMEvent (				const AppleEvent *		inAppleEvent,
								AppleEvent *			ioReply )
								DEPRECATED_ATTRIBUTE ;

#endif /*WASTE_DEPRECATED*/

EXTERN_API ( void )
WEStopInlineSession (			WEReference				inWE ) ;

EXTERN_API ( OSStatus )
WESetTSMHilite (				SInt32					inRangeStart,
								SInt32					inRangeEnd,
								WETSMHiliteStyle		inTSMHiliteStyle,
								WEReference				inWE ) ;

/*	printing support */

EXTERN_API ( OSErr )
WENewPrintSession (				const WEPrintOptions *	inPrintOptions,
								WEReference				inWE,
								WEPrintSession *		outPrintSession ) ;

EXTERN_API ( void )
WEDisposePrintSession (			WEPrintSession			inPrintSession ) ;

EXTERN_API ( SInt32 )
WECountPages (					WEPrintSession			inPrintSession ) ;

EXTERN_API ( SInt32 )
WEGetPageHeight (				SInt32					inPageIndex,
								WEPrintSession			inPrintSession ) ;

EXTERN_API ( OSStatus )
WEPrintPageWithCGContext (		SInt32					inPageIndex,
								CGContextRef			inPrintContext,
								const CGPoint *			inPageOrigin,		/* can be NULL */
								WEPrintSession			inPrintSession ) ;

#if WASTE_DEPRECATED

EXTERN_API ( OSErr )
WEPrintPage (					SInt32					inPageIndex,
								GrafPtr					inPrintPort,
								const Rect *			inPageRect,
								WEPrintSession			inPrintSession )
								DEPRECATED_ATTRIBUTE ;

#endif /*WASTE_DEPRECATED*/

/*	filing (Mac OS 9.1 and later) */

EXTERN_API ( OSStatus )
WESave (						SInt32					inRangeStart,
								SInt32					inRangeEnd,
								const FSRef *			inFileRef,
								OSType					inFileType,
								TextEncoding			inTextEncoding,
								OptionBits				inSaveOptions,
								WEReference				inWE ) ;

EXTERN_API ( OSStatus )
WELoad (						SInt32					inRangeStart,
								SInt32					inRangeEnd,
								const FSRef *			inFileRef,
								OSType *				ioFileType,
								TextEncoding *			ioTextEncoding,
								OptionBits *			ioLoadOptions,
								WEReference				inWE ) ;

/*	additional features */

EXTERN_API ( SInt16 )
WEFeatureFlag (					SInt16					inFeature,
								SInt16					inAction,
								WEReference				inWE ) ;

EXTERN_API ( OSErr )
WEGetInfo (						WESelector				inSelector,
								void *					outInfo,
								WEReference				inWE ) ;

EXTERN_API ( OSErr )
WESetInfo (						WESelector				inSelector,
								const void *			inInfo,
								WEReference				inWE ) ;

/*	getting and setting user-defined info */

EXTERN_API ( OSErr )
WEGetUserInfo (					WESelector				inUserTag,
								SInt32 *				outUserInfo,
								WEReference				inWE ) ;

EXTERN_API ( OSErr )
WESetUserInfo (					WESelector				inUserTag,
								SInt32					inUserInfo,
								WEReference				inWE ) ;

EXTERN_API ( OSErr )
WERemoveUserInfo (				WESelector				inUserTag,
								WEReference				inWE ) ;

EXTERN_API ( OSStatus )
WEGetProperty (					OSType					inPropertyCreator,
								OSType					inPropertyTag,
								ByteCount				inPropertySize,
								ByteCount *				outActualSize,		/* can be NULL */
								void *					outPropertyBuffer,
								WEReference				inWE ) ;

EXTERN_API ( OSStatus )
WESetProperty (					OSType					inPropertyCreator,
								OSType					inPropertyTag,
								ByteCount				inPropertySize,
								const void *			inPropertyBuffer,
								WEReference				inWE ) ;

EXTERN_API ( OSStatus )
WERemoveProperty (				OSType					inPropertyCreator,
								OSType					inPropertyTag,
								WEReference				inWE ) ;

/*	long coordinate utilities */

EXTERN_API ( void )
WELongPointToPoint (			const LongPt *			inLongPoint,
								Point *					outPoint ) ;

EXTERN_API ( void )
WEPointToLongPoint (			Point					inPoint,
								LongPt *				outLongPoint ) ;

EXTERN_API ( void )
WESetLongRect (					LongRect *				outLongRect,
								SInt32					inLeft,
								SInt32					inTop,
								SInt32					inRight,
								SInt32					inBottom ) ;

EXTERN_API ( void )
WELongRectToRect (				const LongRect *		inLongRect,
								Rect *					outRect ) ;

EXTERN_API ( void )
WERectToLongRect (				const Rect *			inRect,
								LongRect *				outLongRect ) ;

EXTERN_API ( void )
WEOffsetLongRect (				LongRect *				ioLongRect,
								SInt32					inHorizontalOffset,
								SInt32					inVerticalOffset ) ;

EXTERN_API ( Boolean )
WELongPointInLongRect (			const LongPt *			inLongPoint,
								const LongRect *		inLongRect ) ;

#if PRAGMA_IMPORT
#pragma import off
#endif

#if PRAGMA_STRUCT_ALIGN
#pragma options align=reset
#endif

#ifdef __cplusplus
}
#endif

#endif	/*__WASTE__*/
