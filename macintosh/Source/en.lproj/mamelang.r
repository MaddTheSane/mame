#include <Carbon/Carbon.r>

resource 'STR#' (128, "rStrings", purgeable) {
	{	/* array StringArray: 122 elements */
		/* [1] */
		"MAME: ",
		/* [2] */
		"Save list",
		/* [3] */
		"MAME Game List",
		/* [4] */
		"MAME ROM List",
		/* [5] */
		"MAME Sample List",
		/* [6] */
		"MAME Driver List",
		/* [7] */
		"MAME Clone List",
		/* [8] */
		"MAME Parent List",
		/* [9] */
		"MAME Non-Working List",
		/* [10] */
		"MAME Imperfect Sound List",
		/* [11] */
		"MAME No Sound List",
		/* [12] */
		"MAME Imperfect Color List",
		/* [13] */
		"MAME Wrong Color List",
		/* [14] */
		"ROM Audit",
		/* [15] */
		"MAME Sample Audit",
		/* [16] */
		"Romset Analysis",
		/* [17] */
		"Analyzing Romsets…",
		/* [18] */
		"Loading",
		/* [19] */
		"ROMs remaining to load:",
		/* [20] */
		"Auditing ROMs…",
		/* [21] */
		"Auditing Samples…",
		/* [22] */
		"Not Working",
		/* [23] */
		"Not Available",
		/* [24] */
		"images.zip",
		/* [25] */
		"cabinets.zip",
		/* [26] */
		"flyers.zip",
		/* [27] */
		"artwork.zip",
		/* [28] */
		"marquees.zip",
		/* [29] */
		"titles.zip",
		/* [30] */
		"cpanel.zip",
		/* [31] */
		"Drivers remaining to audit:",
		/* [32] */
		"Romsets remaining to analyze:",
		/* [33] */
		"ROM Identification Results",
		/* [34] */
		"MAME Identified ROMs",
		/* [35] */
		"Alias points to",
		/* [36] */
		"(Couldn't resolve alias -- original item"
		" not found)",
		/* [37] */
		"Romset format",
		/* [38] */
		"SuperROM (obsolete)",
		/* [39] */
		"Folder",
		/* [40] */
		"Zipfile",
		/* [41] */
		"Driver",
		/* [42] */
		"Clone of",
		/* [43] */
		"Parent of",
		/* [44] */
		"Romset Analysis: N/A  (empty folder)",
		/* [45] */
		"Romset Analysis: N/A  (alias to parent r"
		"omset)",
		/* [46] */
		"Romset Analysis: N/A  (unresolved alias)",
		/* [47] */
		"Romset Analysis:",
		/* [48] */
		"<duplicate file %d of %d, ",
		/* [49] */
		"preferred name: %s>  ",
		/* [50] */
		"isn't reqd for this driver, but is for c"
		"lone(s): ",
		/* [51] */
		"is not a required ROM for this driver",
		/* [52] */
		"is required by this driver and clone(s):"
		" ",
		/* [53] */
		"duplicate exists in parent romset",
		/* [54] */
		"can be moved to parent romset",
		/* [55] */
		"MAME currently supports the following ga"
		"mes:",
		/* [56] */
		"MAME currently supports sound samples fo"
		"r the following games:",
		/* [57] */
		"MAME currently supports the following cl"
		"ones:",
		/* [58] */
		"MAME currently supports the following pa"
		"rents:",
		/* [59] */
		"The following games are currently non-wo"
		"rking:",
		/* [60] */
		"The following games have imperfect sound"
		":",
		/* [61] */
		"The following games have no sound:",
		/* [62] */
		"The following games have imperfect color"
		":",
		/* [63] */
		"The following games have totally wrong c"
		"olor:",
		/* [64] */
		"<No ROMs needed>",
		/* [65] */
		"ROMs needed:  (ROMs may be located in '%"
		"s' romset)",
		/* [66] */
		"ROMs needed:",
		/* [67] */
		"Samples needed:  (Samples may be located"
		" in '%s' set)",
		/* [68] */
		"Samples needed:",
		/* [69] */
		"Clone of",
		/* [70] */
		"Parent of",
		/* [71] */
		"Total games supported",
		/* [72] */
		"Total unique games",
		/* [73] */
		"Total games supporting samples",
		/* [74] */
		"Total number of clones supported",
		/* [75] */
		"Total number of parents supported",
		/* [76] */
		"Total number of non-working games",
		/* [77] */
		"Total number of games with imperfect sou"
		"nd",
		/* [78] */
		"Total number of games with no sound",
		/* [79] */
		"Total number of games with imperfect col"
		"or",
		/* [80] */
		"Total number of games with totally wrong"
		" color",
		/* [81] */
		"NO GOOD DUMP KNOWN",
		/* [82] */
		"Multiple romsets found for driver",
		/* [83] */
		"is not a recognized driver name",
		/* [84] */
		"Audit for drivers matching filter",
		/* [85] */
		"No romset found in ROMs folder",
		/* [86] */
		"ROM not found",
		/* [87] */
		"NOTE: No valid dump of this ROM is known"
		" to exist",
		/* [88] */
		"Incorrect checksum",
		/* [89] */
		"Expected",
		/* [90] */
		"Found",
		/* [91] */
		"Length mismatch",
		/* [92] */
		"Couldn't verify checksum (valid checksum"
		" is not known)",
		/* [93] */
		"Good",
		/* [94] */
		"Length",
		/* [95] */
		"Out of memory reading ROM",
		/* [96] */
		"Checking for duplicate romsets in ROMs f"
		"older",
		/* [97] */
		"Checking for unknown romsets in ROMs fol"
		"der",
		/* [98] */
		"No duplicates found",
		/* [99] */
		"No unknown romsets found",
		/* [100] */
		"MAME found problems with samples for the"
		" following games",
		/* [101] */
		"No samples found for this driver",
		/* [102] */
		"Problem",
		/* [103] */
		"Sample file not found",
		/* [104] */
		"No disk image found",
		/* [105] */
		"STATUS",
		/* [106] */
		"good",
		/* [107] */
		"not found",
		/* [108] */
		"not found (p)",
		/* [109] */
		"not found (BIOS)",
		/* [110] */
		"No good dump exists",
		/* [111] */
		"No dump exists",
		/* [112] */
		"bad crc",
		/* [113] */
		"bad length",
		/* [114] */
		"expected",
		/* [115] */
		"memory error",
		/* [116] */
		"unknown",
		/* [117] */
		"history not available",
		/* [118] */
		"Filename",
		/* [119] */
		"Driver",
		/* [120] */
		"Driver Description",
		/* [121] */
		"NO MATCH",
		/* [122] */
		"CHD is old version"
	}
};

resource 'STR#' (129, "error strings", purgeable) {
	{	/* array StringArray: 5 elements */
		/* [1] */
		"Couldn’t create file",
		/* [2] */
		"Unable to allocate memory for romset ana"
		"lysis!",
		/* [3] */
		"**** ANALYSIS STOPPED AT USER'S REQUEST "
		"****",
		/* [4] */
		"Couldn't allocate memory for creating li"
		"st!",
		/* [5] */
		"**** AUDIT STOPPED AT USER'S REQUEST ***"
		"*"
	}
};

resource 'STR#' (130, "keyboard strings", purgeable) {
	{	/* array StringArray: 128 elements */
		/* [1] */
		"A",
		/* [2] */
		"S",
		/* [3] */
		"D",
		/* [4] */
		"F",
		/* [5] */
		"H",
		/* [6] */
		"G",
		/* [7] */
		"Z",
		/* [8] */
		"X",
		/* [9] */
		"C",
		/* [10] */
		"V",
		/* [11] */
		"?",
		/* [12] */
		"B",
		/* [13] */
		"Q",
		/* [14] */
		"W",
		/* [15] */
		"E",
		/* [16] */
		"R",
		/* [17] */
		"Y",
		/* [18] */
		"T",
		/* [19] */
		"1",
		/* [20] */
		"2",
		/* [21] */
		"3",
		/* [22] */
		"4",
		/* [23] */
		"6",
		/* [24] */
		"5",
		/* [25] */
		"=",
		/* [26] */
		"9",
		/* [27] */
		"7",
		/* [28] */
		"-",
		/* [29] */
		"8",
		/* [30] */
		"0",
		/* [31] */
		"]",
		/* [32] */
		"O",
		/* [33] */
		"U",
		/* [34] */
		"[",
		/* [35] */
		"I",
		/* [36] */
		"P",
		/* [37] */
		"Return",
		/* [38] */
		"L",
		/* [39] */
		"J",
		/* [40] */
		"'",
		/* [41] */
		"K",
		/* [42] */
		";",
		/* [43] */
		"\\",
		/* [44] */
		",",
		/* [45] */
		"/",
		/* [46] */
		"N",
		/* [47] */
		"M",
		/* [48] */
		".",
		/* [49] */
		"Tab",
		/* [50] */
		"Space",
		/* [51] */
		"`",
		/* [52] */
		"Delete",
		/* [53] */
		"?",
		/* [54] */
		"Esc",
		/* [55] */
		"?",
		/* [56] */
		"Command",
		/* [57] */
		"L Shift",
		/* [58] */
		"Caps Lock",
		/* [59] */
		"L Option",
		/* [60] */
		"L Control",
		/* [61] */
		"R Shift",
		/* [62] */
		"R Option",
		/* [63] */
		"R Control",
		/* [64] */
		"?",
		/* [65] */
		"?",
		/* [66] */
		"KP .",
		/* [67] */
		"?",
		/* [68] */
		"KP *",
		/* [69] */
		"?",
		/* [70] */
		"KP +",
		/* [71] */
		"?",
		/* [72] */
		"Clear",
		/* [73] */
		"?",
		/* [74] */
		"?",
		/* [75] */
		"?",
		/* [76] */
		"KP /",
		/* [77] */
		"Enter",
		/* [78] */
		"?",
		/* [79] */
		"KP -",
		/* [80] */
		"?",
		/* [81] */
		"?",
		/* [82] */
		"KP =",
		/* [83] */
		"KP 0",
		/* [84] */
		"KP 1",
		/* [85] */
		"KP 2",
		/* [86] */
		"KP 3",
		/* [87] */
		"KP 4",
		/* [88] */
		"KP 5",
		/* [89] */
		"KP 6",
		/* [90] */
		"KP 7",
		/* [91] */
		"?",
		/* [92] */
		"KP 8",
		/* [93] */
		"KP 9",
		/* [94] */
		"?",
		/* [95] */
		"?",
		/* [96] */
		"?",
		/* [97] */
		"F5",
		/* [98] */
		"F6",
		/* [99] */
		"F7",
		/* [100] */
		"F3",
		/* [101] */
		"F8",
		/* [102] */
		"F9",
		/* [103] */
		"?",
		/* [104] */
		"F11",
		/* [105] */
		"?",
		/* [106] */
		"F13",
		/* [107] */
		"?",
		/* [108] */
		"F14",
		/* [109] */
		"?",
		/* [110] */
		"F10",
		/* [111] */
		"?",
		/* [112] */
		"F12",
		/* [113] */
		"?",
		/* [114] */
		"F15",
		/* [115] */
		"Help",
		/* [116] */
		"Home",
		/* [117] */
		"Page Up",
		/* [118] */
		"Del",
		/* [119] */
		"F4",
		/* [120] */
		"End",
		/* [121] */
		"F2",
		/* [122] */
		"Page Down",
		/* [123] */
		"F1",
		/* [124] */
		"Left",
		/* [125] */
		"Right",
		/* [126] */
		"Down",
		/* [127] */
		"Up",
		/* [128] */
		"?"
	}
};

resource 'STR#' (131, "Group menu strings", purgeable) {
	{	/* array StringArray: 9 elements */
		/* [1] */
		"Show clones",
		/* [2] */
		"Hide clones",
		/* [3] */
		"     Show virtual clones",
		/* [4] */
		"     Hide virtual clones",
		/* [5] */
		"Show non-working games",
		/* [6] */
		"Hide non-working games",
		/* [7] */
		"     Attach clones to parent",
		/* [8] */
		"     Detach clones from parent",
		/* [9] */
		"Group by %s"
	}
};

data 'TEXT' (128, "analyze romsets notice", purgeable) {
	$"414E 414C 5953 4953 204F 4620 524F 4D53"            /* ANALYSIS OF ROMS */
	$"4554 5320 494E 2052 4F4D 5320 464F 4C44"            /* ETS IN ROMS FOLD */
	$"4552 0D0D 4E4F 5449 4345 3A0D 0D50 6C65"            /* ER..NOTICE:..Ple */
	$"6173 6520 6E6F 7465 2074 6861 7420 616C"            /* ase note that al */
	$"7468 6F75 6768 2061 2066 696C 6520 6D61"            /* though a file ma */
	$"7920 6265 2072 6570 6F72 7465 6420 6173"            /* y be reported as */
	$"20D2 6E6F 7420 6120 7265 7175 6972 6564"            /*  “not a required */
	$"2052 4F4D 2066 6F72 2074 6869 7320 6472"            /*  ROM for this dr */
	$"6976 6572 D32C 2079 6F75 2061 7265 2064"            /* iver”, you are d */
	$"6973 636F 7572 6167 6564 2066 726F 6D20"            /* iscouraged from  */
	$"7265 6D6F 7669 6E67 2074 6865 7365 2066"            /* removing these f */
	$"696C 6573 2066 726F 6D20 796F 7572 2072"            /* iles from your r */
	$"6F6D 7365 742E 204F 6674 656E 2074 6865"            /* omset. Often the */
	$"7365 2066 696C 6573 2063 6F6E 7461 696E"            /* se files contain */
	$"2069 6E66 6F72 6D61 7469 6F6E 2065 7373"            /*  information ess */
	$"656E 7469 616C 2074 6F20 5043 4220 6F77"            /* ential to PCB ow */
	$"6E65 7273 2074 7279 696E 6720 746F 2072"            /* ners trying to r */
	$"6570 6169 7220 7468 6569 7220 626F 6172"            /* epair their boar */
	$"642C 2073 696E 6365 2074 6865 7920 6D61"            /* d, since they ma */
	$"7920 636F 6E74 6169 6E20 7061 7274 206E"            /* y contain part n */
	$"756D 6265 7273 2061 6E64 206C 6F63 6174"            /* umbers and locat */
	$"696F 6E20 696E 666F 726D 6174 696F 6E2E"            /* ion information. */
	$"2041 6464 6974 696F 6E61 6C6C 792C 2073"            /*  Additionally, s */
	$"6F6D 6574 696D 6573 2052 4F4D 7320 6172"            /* ometimes ROMs ar */
	$"6520 6E6F 7420 7965 7420 7265 7175 6972"            /* e not yet requir */
	$"6564 2062 6563 6175 7365 2074 6865 2065"            /* ed because the e */
	$"6D75 6C61 7469 6F6E 2069 7320 696E 636F"            /* mulation is inco */
	$"6D70 6C65 7465 2C20 6275 7420 6D61 7920"            /* mplete, but may  */
	$"6C61 7465 7220 6265 2072 6571 7569 7265"            /* later be require */
	$"642E 2043 6F6C 6F72 2050 524F 4D73 2C20"            /* d. Color PROMs,  */
	$"736F 756E 6420 524F 4D73 2C20 6574 632E"            /* sound ROMs, etc. */
	$"206D 6179 2066 616C 6C20 696E 746F 2074"            /*  may fall into t */
	$"6869 7320 6361 7465 676F 7279 2E0D 0D55"            /* his category...U */
	$"4E44 4552 5354 414E 4449 4E47 2054 4849"            /* NDERSTANDING THI */
	$"5320 5245 504F 5254 3A0D 0D59 6F75 276C"            /* S REPORT:..You'l */
	$"6C20 6E65 6564 2074 6F20 6265 2066 616D"            /* l need to be fam */
	$"696C 6961 7220 7769 7468 2074 6865 2063"            /* iliar with the c */
	$"6F6E 6365 7074 206F 6620 20D2 726F 6D20"            /* oncept of  “rom  */
	$"6D65 7267 696E 67D3 2069 6E20 6F72 6465"            /* merging” in orde */
	$"7220 746F 2066 756C 6C79 2075 6E64 6572"            /* r to fully under */
	$"7374 616E 6420 7468 6973 2072 6570 6F72"            /* stand this repor */
	$"742E 2054 6869 7320 7265 6665 7273 2074"            /* t. This refers t */
	$"6F20 7468 6520 4D41 4D45 2066 6561 7475"            /* o the MAME featu */
	$"7265 2077 6869 6368 2061 6C6C 6F77 7320"            /* re which allows  */
	$"796F 7520 746F 2073 746F 7265 20D2 636C"            /* you to store “cl */
	$"6F6E 65D3 2052 4F4D 7320 696E 2065 6974"            /* one” ROMs in eit */
	$"6865 7220 7468 6520 636C 6F6E 6520 7365"            /* her the clone se */
	$"742C 2074 6865 2063 6C6F 6E65 2773 2070"            /* t, the clone's p */
	$"6172 656E 7420 7365 742C 206F 7220 626F"            /* arent set, or bo */
	$"7468 2E0D 0D4F 6674 656E 2063 6C6F 6E65"            /* th...Often clone */
	$"7320 6861 7665 2073 6F6D 6520 524F 4D73"            /* s have some ROMs */
	$"2069 6E20 636F 6D6D 6F6E 2077 6974 6820"            /*  in common with  */
	$"7468 6520 7061 7265 6E74 2073 6574 2028"            /* the parent set ( */
	$"692E 652E 2C20 626F 7468 2074 6865 2070"            /* i.e., both the p */
	$"6172 656E 7420 616E 6420 7468 6520 636C"            /* arent and the cl */
	$"6F6E 6520 7265 7175 6972 6520 7468 6520"            /* one require the  */
	$"7361 6D65 2052 4F4D 2E29 2049 6620 6120"            /* same ROM.) If a  */
	$"726F 6D20 6C6F 6361 7465 6420 696E 2061"            /* rom located in a */
	$"2063 6C6F 6E65 2073 6574 2069 7320 6120"            /*  clone set is a  */
	$"636F 6D6D 6F6E 2052 4F4D 2C20 7468 6520"            /* common ROM, the  */
	$"7379 6D62 6F6C 205B 2A5D 2077 696C 6C20"            /* symbol [*] will  */
	$"6265 2075 7365 6420 746F 2069 6E64 6963"            /* be used to indic */
	$"6174 6520 6974 2E20 436C 6F6E 6520 726F"            /* ate it. Clone ro */
	$"6D73 206E 6F74 2072 6571 7569 7265 6420"            /* ms not required  */
	$"6279 2074 6865 2070 6172 656E 7420 7365"            /* by the parent se */
	$"7420 7769 6C6C 2075 7365 2074 6865 2073"            /* t will use the s */
	$"796D 626F 6C20 5B20 5D20 696E 7374 6561"            /* ymbol [ ] instea */
	$"642E 0D0D 5468 6520 4352 432D 3332 2066"            /* d...The CRC-32 f */
	$"6F72 2066 696C 6573 2069 7320 7368 6F77"            /* or files is show */
	$"6E20 6F6E 6C79 2066 6F72 2072 6F6D 7365"            /* n only for romse */
	$"7473 2069 6E20 7A69 7066 696C 6520 666F"            /* ts in zipfile fo */
	$"726D 6174 2E20 4D41 4D45 2063 616E 2066"            /* rmat. MAME can f */
	$"696E 6420 7468 6573 6520 524F 4D73 2075"            /* ind these ROMs u */
	$"7369 6E67 2074 6865 6972 2043 5243 2065"            /* sing their CRC e */
	$"7665 6E20 7768 656E 2074 6865 206E 616D"            /* ven when the nam */
	$"6520 646F 6573 206E 6F74 206D 6174 6368"            /* e does not match */
	$"2074 6865 206E 616D 6520 7370 6563 6966"            /*  the name specif */
	$"6965 6420 6279 2074 6865 2064 7269 7665"            /* ied by the drive */
	$"722E 0D0D 4669 6C65 7320 7769 7468 206E"            /* r...Files with n */
	$"6F20 636F 6D6D 656E 7473 2061 7265 2052"            /* o comments are R */
	$"4F4D 7320 7768 6963 6820 6172 6520 7265"            /* OMs which are re */
	$"7175 6972 6564 2062 7920 7468 6520 6472"            /* quired by the dr */
	$"6976 6572 2E0D 0D54 6869 7320 7265 706F"            /* iver...This repo */
	$"7274 2064 6F65 7320 6E6F 7420 6174 7465"            /* rt does not atte */
	$"6D70 7420 746F 206C 6973 7420 7072 6F62"            /* mpt to list prob */
	$"6C65 6D73 2074 6861 7420 7769 6C6C 2070"            /* lems that will p */
	$"7265 7665 6E74 2072 756E 6E69 6E67 2074"            /* revent running t */
	$"6865 2074 6865 2064 7269 7665 722E 2049"            /* he the driver. I */
	$"7420 7265 706F 7274 7320 6F6E 6C79 206F"            /* t reports only o */
	$"6E20 7468 6520 524F 4D73 2074 6861 7420"            /* n the ROMs that  */
	$"6163 7475 616C 6C79 2065 7869 7374 2069"            /* actually exist i */
	$"6E20 796F 7572 2072 6F6D 7365 7473 2E20"            /* n your romsets.  */
	$"4974 2077 696C 6C20 6E6F 7420 6C69 7374"            /* It will not list */
	$"206D 6973 7369 6E67 2052 4F4D 732E 2049"            /*  missing ROMs. I */
	$"6620 796F 7520 6E65 6564 2074 6861 7420"            /* f you need that  */
	$"696E 666F 726D 6174 696F 6E2C 2075 7365"            /* information, use */
	$"2074 6865 2052 4F4D 2041 7564 6974 2069"            /*  the ROM Audit i */
	$"6E73 7465 6164 2C20 7768 6963 6820 7761"            /* nstead, which wa */
	$"7320 6465 7369 676E 6564 2066 6F72 2061"            /* s designed for a */
	$"6E61 6C79 7A69 6E67 2074 686F 7365 2070"            /* nalyzing those p */
	$"726F 626C 656D 732E 0D0D 5741 524E 494E"            /* roblems...WARNIN */
	$"473A 0D0D 5765 2064 6F20 6E6F 7420 636F"            /* G:..We do not co */
	$"6E64 6F6E 6520 7468 6520 756E 6175 7468"            /* ndone the unauth */
	$"6F72 697A 6564 206F 7220 756E 6C69 6365"            /* orized or unlice */
	$"6E73 6564 2075 7365 206F 6620 524F 4D73"            /* nsed use of ROMs */
	$"2E20 596F 7520 6172 6520 7265 7370 6F6E"            /* . You are respon */
	$"7369 626C 6520 666F 7220 796F 7572 206F"            /* sible for your o */
	$"776E 2061 6374 696F 6E73 2072 6567 6172"            /* wn actions regar */
	$"6469 6E67 2070 6F73 6573 7369 6F6E 2061"            /* ding posession a */
	$"6E64 2075 7365 206F 6620 524F 4D73 2E0D"            /* nd use of ROMs.. */
	$"0D2D 2D2D 2D2D 2D2D 2D2D 2D2D 2D2D 2D2D"            /* .--------------- */
	$"2D2D 2D2D 2D2D 2D2D 2D2D 2D2D 2D2D 2D2D"            /* ---------------- */
	$"2D2D 2D2D 2D2D 2D2D 2D2D 2D2D 2D2D 2D2D"            /* ---------------- */
	$"2D2D 2D2D 2D2D 2D2D 2D2D 2D2D 2D2D 2D2D"            /* ---------------- */
	$"2D2D 2D2D 2D2D 2D2D 0D30"                           /* --------.0 */
};

