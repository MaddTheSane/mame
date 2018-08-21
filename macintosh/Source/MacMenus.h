/*##########################################################################

	MacMenus.h

	Holds all menu-related stuff

##########################################################################*/

#pragma once

// Menubar defines
enum
{
	rAppleMenu					= 128,
//		kAppleAbout				= 1,

	rGameMenu					= 129,
		kMenuGame_Screenshot	= 'Mscr',
		kMenuGame_Reset			= 'Mrst',
		kMenuGame_Log			= 'Mlog',
		kMenuGame_Pause			= 'Mpau',

	rEditMenu					= 132,

	rPerformanceMenu			= 130,
		kMenuPerf_IncSkip		= 'Mfup',
		kMenuPerf_DecSkip		= 'Mfdn',
		kMenuPerf_AutoSkip		= 'Mfau',
		kMenuPerf_Throttle		= 'Mthr',
		kMenuPerf_ResetAvg		= 'Mavg',

	rVideoMenu					= 131,
		kMenuVideo_1x1			= 'Mv11',
		kMenuVideo_2x2			= 'Mv22',
		kMenuVideo_2x2s			= 'Mv2s',
		kMenuVideo_3x3			= 'Mv33',
		kMenuVideo_3x3s			= 'Mv3s',
		kMenuVideo_IncGamma		= 'Mgup',
		kMenuVideo_DecGamma		= 'Mgdn',
		kMenuVideo_HideDesk		= 'Mhid',

	rWindowMenu					= 133,
};

