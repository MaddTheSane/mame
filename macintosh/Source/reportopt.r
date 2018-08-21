#include <Carbon/Carbon.r>

resource 'CNTL' (3200, "rReportOptions path box", purgeable) {
	{244, 20, 295, 352},
	0,
	visible,
	100,
	0,
	164,
	0,
	"Report will be saved as:"
};

resource 'CNTL' (3201, "rReportOptions popup", purgeable) {
	{212, 12, 228, 280},
	0,
	visible,
	80,
	3200,
	402,
	0,
	"Report type:"
};

resource 'CNTL' (3202, "rReportOptions adv box", purgeable) {
	{120, 8, 192, 283},
	0,
	visible,
	100,
	0,
	160,
	0,
	"Options"
};

resource 'CNTL' (3300, "rResultsDLOG user pane", purgeable) {
	{8, 8, 298, 601},
	56,
	visible,
	0,
	0,
	256,
	0,
	""
};

resource 'CNTL' (3301, "rResultsDLOG scrollbar", purgeable) {
	{8, 500, 298, 516},
	0,
	visible,
	0,
	0,
	386,
	0,
	""
};

resource 'CNTL' (3302, "rResultsDLOG hscrollbar", purgeable) {
	{297, 8, 313, 601},
	0,
	visible,
	0,
	0,
	386,
	0,
	""
};

resource 'dctb' (3200) {
	{	/* array ColorSpec: 5 elements */
		/* [1] */
		wContentColor, 65535, 65535, 65535,
		/* [2] */
		wFrameColor, 0, 0, 0,
		/* [3] */
		wTextColor, 0, 0, 0,
		/* [4] */
		wHiliteColor, 0, 0, 0,
		/* [5] */
		wTitleBarColor, 65535, 65535, 65535
	}
};

resource 'dftb' (3200) {
	versionZero {
		{	/* array FontStyle: 8 elements */
			/* [1] */
			skipItem {

			}			,
			/* [2] */
			skipItem {

			}			,
			/* [3] */
			skipItem {

			}			,
			/* [4] */
			skipItem {

			}			,
			/* [5] */
			skipItem {

			}			,
			/* [6] */
			skipItem {

			}			,
			/* [7] */
			skipItem {

			}			,
			/* [8] */
			skipItem {

			}		}
	}
};

resource 'DITL' (3200, "rReportOptionsDLOG", purgeable) {
	{	/* array DITLarray: 7 elements */
		/* [1] */
		{156, 212, 176, 280},
		Button {
			enabled,
			"OK"
		},
		/* [2] */
		{156, 136, 176, 200},
		Button {
			enabled,
			"Cancel"
		},
		/* [3] */
		{44, 8, 116, 284},
		Control {
			enabled,
			3202
		},
		/* [4] */
		{12, 12, 28, 280},
		Control {
			enabled,
			3201
		},
		/* [5] */
		{68, 20, 84, 270},
		CheckBox {
			enabled,
			"Exclude clones"
		},
		/* [6] */
		{88, 20, 104, 270},
		CheckBox {
			enabled,
			"Exclude parents"
		},
		/* [7] */
		{156, 8, 176, 88},
		Button {
			enabled,
			"Defaults"
		}
	}
};

resource 'DITL' (3300, purgeable) {
	{	/* array DITLarray: 5 elements */
		/* [1] */
		{328, 536, 348, 604},
		Button {
			enabled,
			"Done"
		},
		/* [2] */
		{8, 8, 298, 601},
		Control {
			enabled,
			3300
		},
		/* [3] */
		{8, 600, 298, 616},
		Control {
			enabled,
			3301
		},
		/* [4] */
		{328, 440, 348, 520},
		Button {
			enabled,
			"Save As…"
		},
		/* [5] */
		{297, 8, 313, 601},
		Control {
			enabled,
			3302
		}
	}
};

resource 'dlgx' (3200, "rReportOptionsDLOG", purgeable) {
	versionZero {
		15
	}
};

resource 'dlgx' (3300, "rRomidentDLOG", purgeable) {
	versionZero {
		15
	}
};

data 'DLGX' (3200) {
	$"0743 6869 6361 676F 0000 0000 0000 0000"            /* .Chicago........ */
	$"0000 0000 0000 0000 0000 0000 0000 0000"            /* ................ */
	$"0000 0000 0000 0000 0000 0000 0000 0000"            /* ................ */
	$"0000 0000 0000 0000 0000 0000 0000 0000"            /* ................ */
	$"000C 0000 0000 0000 0004 0004 0000 0000"            /* ................ */
	$"0007 0000 0000 0000 0000 0000 0000 0001"            /* ................ */
	$"0000 0000 0000 0000 0000 0005 8000 0000"            /* ............Ä... */
	$"0000 0000 0000 0005 0000 0000 0000 0000"            /* ................ */
	$"0000 0003 0000 0000 0000 0000 0000 0003"            /* ................ */
	$"0000 0000 0000 0000 0000 0002 0000 0000"            /* ................ */
	$"0000 0000 0000"                                     /* ...... */
};

data 'DLGX' (3300) {
	$"0743 6869 6361 676F 0000 0000 0000 0000"            /* .Chicago........ */
	$"0000 0000 0000 0000 0000 0000 0000 0000"            /* ................ */
	$"0000 0000 0000 0000 0000 0000 0000 0000"            /* ................ */
	$"0000 0000 0000 0000 0000 0000 0000 0000"            /* ................ */
	$"000C 0000 0000 0000 0004 0004 0000 0000"            /* ................ */
	$"0005 0000 0000 0000 0000 0000 0000 0005"            /* ................ */
	$"0000 0000 0000 0000 0000 0005 0000 0000"            /* ................ */
	$"0000 0000 0000 0002 0000 0000 0000 0000"            /* ................ */
	$"0000 0005 0000 0000 0000 0000 0000"                 /* .............. */
};

resource 'DLOG' (3200, "rReportOptionsDLOG", purgeable) {
	{391, 598, 581, 890},
	movableDBoxProc,
	invisible,
	goAway,
	0x0,
	3200,
	"Reports",
	centerParentWindow
};

resource 'DLOG' (3300, "rResultsDLOG", purgeable) {
	{225, 221, 583, 844},
	1046,
	invisible,
	noGoAway,
	0x0,
	3300,
	"Results",
	centerParentWindowScreen
};

data 'ictb' (3200) {
	$"0000 0000 0000 0000 0000 0000 0000 0000"            /* ................ */
	$"0000 0000 0000 0000 0000 0000 0000 0000"            /* ................ */
};

data 'Mcmd' (3200) {
	$"0004 0000 0000 0000 0000 0000 0000 0000"            /* ................ */
	$"0000"                                               /* .. */
};

resource 'mctb' (3200) {
	{	/* array MCTBArray: 1 elements */
		/* [1] */
		mctbLast, 0,
		{	/* array: 4 elements */
			/* [1] */
			0, 0, 0,
			/* [2] */
			0, 0, 0,
			/* [3] */
			0, 0, 0,
			/* [4] */
			0, 0, 0
		}
	}
};

resource 'MENU' (3200) {
	3200,
	textMenuProc,
	allEnabled,
	enabled,
	"Untitled",
	{	/* array: 11 elements */
		/* [1] */
		"Supported Games", noIcon, noKey, noMark, plain,
		/* [2] */
		"Game ROMs", noIcon, noKey, noMark, plain,
		/* [3] */
		"Game Samples", noIcon, noKey, noMark, plain,
		/* [4] */
		"Driver Info", noIcon, noKey, noMark, plain,
		/* [5] */
		"Clones", noIcon, noKey, noMark, plain,
		/* [6] */
		"Parents", noIcon, noKey, noMark, plain,
		/* [7] */
		"Non-Working Games", noIcon, noKey, noMark, plain,
		/* [8] */
		"Imperfect Sound", noIcon, noKey, noMark, plain,
		/* [9] */
		"No Sound", noIcon, noKey, noMark, plain,
		/* [10] */
		"Imperfect Color", noIcon, noKey, noMark, plain,
		/* [11] */
		"Wrong Color", noIcon, noKey, noMark, plain
	}
};

