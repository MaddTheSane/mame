/*##########################################################################

	macinput.c

	Routines for handling keyboard, joystick and mouse input with MacMAME.

##########################################################################*/

#include <Carbon/Carbon.h>
#include <IOKit/hid/IOHIDLib.h>
#include <IOKit/hid/IOHIDUsageTables.h>
#include "HID_Utilities_External.h"

#include <cctype>
#include <vector>

using std::vector;

extern "C"
{
	#include "mac.h"
	#include "MacInput.h"
	#include "macstrings.h"
}

#define ELEMENTS(x)			(sizeof(x) / sizeof((x)[0]))

/*##########################################################################
	CONSTANTS
##########################################################################*/

// general constants
enum
{
	kMaxMouseButtons		= 6,
	
	kMaxMAMEJoysticks		= 8,			// Total # of joysticks that the core supports

	kMaxMAMEButtons			= 32,			// Total # of digital buttons per stick
	kMaxMAMEAnalogAxes		= 8,			// Total # of axes per analog stick
	kMaxMAMEPOVs			= 4,			// Total # of hatswitches per stick
	
	kMaxNeeds				= kMaxMAMEButtons + kMaxMAMEAnalogAxes + kMaxMAMEPOVs,
											// Total number of needs per stick
};
   

// keyboard constants
enum
{
	kMaxKeys				= 128,
#ifdef MESS
	kSpecialKeys			= 16,
#else
	kSpecialKeys			= 1,
#endif
	kScanCodeOffset			= 1,			// offset added to each code

	kMacKeyA				= 0x00,
	kMacKeyS				= 0x01,
	kMacKeyD				= 0x02,
	kMacKeyF				= 0x03,
	kMacKeyH				= 0x04,
	kMacKeyG				= 0x05,
	kMacKeyZ				= 0x06,
	kMacKeyX				= 0x07,
	kMacKeyC				= 0x08,
	kMacKeyV				= 0x09,
	kMacKeyB				= 0x0b,
	kMacKeyQ				= 0x0c,
	kMacKeyW				= 0x0d,
	kMacKeyE				= 0x0e,
	kMacKeyR				= 0x0f,
	kMacKeyY				= 0x10,
	kMacKeyT				= 0x11,
	kMacKey1				= 0x12,
	kMacKey2				= 0x13,
	kMacKey3				= 0x14,
	kMacKey4				= 0x15,
	kMacKey6				= 0x16,
	kMacKey5				= 0x17,
	kMacKey9				= 0x19,
	kMacKey7				= 0x1a,
	kMacKey8				= 0x1c,
	kMacKey0				= 0x1d,
	kMacKeyO				= 0x1f,
	kMacKeyU				= 0x20,
	kMacKeyI				= 0x22,
	kMacKeyP				= 0x23,
	kMacKeyReturn			= 0x24,
	kMacKeyL				= 0x25,
	kMacKeyJ				= 0x26,
	kMacKeyK				= 0x28,
	kMacKeyN				= 0x2d,
	kMacKeyM				= 0x2e,
	kMacKeyTab				= 0x30,
	kMacKeySpace			= 0x31,
#ifdef MESS
	kMacKeyTilde			= /*0x32*/0x0a,	// R. Nabet 000601
#else
	kMacKeyTilde			= 0x32,
#endif
	kMacKeyBackspace		= 0x33,
	kMacKeyEscape			= 0x35,
	kMacKeyCommand			= 0x37,
	kMacKeyLeftShift		= 0x38,
	kMacKeyLeftOption		= 0x3a,
	kMacKeyLeftControl		= 0x3b,
	kMacKeyRightShift		= 0x3c,
	kMacKeyRightOption		= 0x3d,
	kMacKeyRightControl		= 0x3e,
	kMacKeyKeypad0			= 0x52,
	kMacKeyKeypad1			= 0x53,
	kMacKeyKeypad2			= 0x54,
	kMacKeyKeypad3			= 0x55,
	kMacKeyKeypad4			= 0x56,
	kMacKeyKeypad5			= 0x57,
	kMacKeyKeypad6			= 0x58,
	kMacKeyKeypad7			= 0x59,
	kMacKeyKeypad8			= 0x5b,
	kMacKeyKeypad9			= 0x5c,
	kMacKeyF1				= 0x7a,
	kMacKeyF2				= 0x78,
	kMacKeyF3				= 0x63,
	kMacKeyF4				= 0x76,
	kMacKeyF5				= 0x60,
	kMacKeyF6				= 0x61,
	kMacKeyF7				= 0x62,
	kMacKeyF8				= 0x64,
	kMacKeyF9				= 0x65,
	kMacKeyF10				= 0x6d,
	kMacKeyF11				= 0x67,
	kMacKeyF12				= 0x6f,
	kMacKeyInsert			= 0x72,
	kMacKeyHome				= 0x73,
	kMacKeyPageUp			= 0x74,
	kMacKeyDel				= 0x75,
	kMacKeyEnd				= 0x77,
	kMacKeyPageDown			= 0x79,
	kMacKeyLeft				= 0x7b,
	kMacKeyRight			= 0x7c,
	kMacKeyDown				= 0x7d,
	kMacKeyUp				= 0x7e,
	kMacKeyMinus            = 0x1B,
	kMacKeyEquals           = 0x18,
	kMacKeyOpenBrace        = 0x21,
	kMacKeyCloseBrace       = 0x1E,
	kMacKeyColon            = 0x29,
	kMacKeyQuote            = 0x27,
	kMacKeyBackslash        = 0x2A,
#ifdef MESS
	kMacKeyBackslash2		= /*0x0a*/0x32,	// R. Nabet 000601
#else
//	kMacKeyBackslash2
#endif
	kMacKeyComma            = 0x2B,
	kMacKeyStop				= 0x2F,
	kMacKeySlash            = 0x2C,
	kMacKeyKeypadSlash      = 0x4B,
	kMacKeyKeypadAsterisk   = 0x43,
	kMacKeyKeypadMinus      = 0x4E,
	kMacKeyKeypadPlus       = 0x45,
	kMacKeyKeypadStop		= 0x41,
	kMacKeyKeypadEnter		= 0x4C,
	kMacKeyPrintScreen		= 0x69,
	kMacKeyPause			= 0x71,
	kMacKeyScreenLock		= 0x6B,
	kMacKeyNumLock			= 0x47,
	kMacKeyCapsLock			= 0x39,

#ifdef MESS
	kMacKeyKeypadEquals		= 0x51,		// no equivalent on PC keyboards, mapped to menu
#else
	//kMacKeyKeypadEquals		= 0x51,	// no equivalent on PC keyboards
#endif
	kMacKeySpecialExit		= 0x80,
#ifdef MESS
	kMacKeySpecialPause,
	kMacKeySpecialUIConfigure,
	kMacKeySpecialUIOnScreenDisplay,
	kMacKeySpecialUIResetMachine,
	kMacKeySpecialUIShowGfx,
	kMacKeySpecialUIFrameskipDec,
	kMacKeySpecialUIFrameskipInc,
	kMacKeySpecialUIThrottle,
	kMacKeySpecialUIShowFps,
	kMacKeySpecialUIShowProfiler,
	kMacKeySpecialUISnapshot,
	kMacKeySpecialUIToggleCheat,
	kMacKeySpecialUIToggleDebug,
	kMacKeySpecialUISaveState,
	kMacKeySpecialUILoadState,
	kMacKeySpecialUIToggleCrosshair
#endif
};

enum
{
	kHIDElementUsageType_Axes = 0,
	kHIDElementUsageType_Buttons,
	kHIDElementUsageType_POVs,
	
	AXIS_TYPE_INVALID = 0,
	AXIS_TYPE_DIGITAL,
	AXIS_TYPE_ANALOG
};

/*##########################################################################
	TYPEDEFS
##########################################################################*/

typedef struct
{
	int			fMacCode;
	int			fMAMECode;
} KeyboardLookup;

typedef struct
{
    pRecDevice			device;
    vector<pRecElement>	axisElements;
    vector<long>		axisElementValue;
    vector<pRecElement>	buttonElements;
    vector<long>		buttonElementValue;
    vector<pRecElement>	povElements;
    vector<long>		povElementValue;
} t_hidJoyDevice;

typedef struct
{
	vector<pRecElement> element;
	long				value;
	int					keyCode;
} t_hidKeyElement;

typedef struct
{
    pRecDevice			device;
    t_hidKeyElement		modifierElements[6];
    pRecElement			ledElements[3];
    long				ledElementValue[3];
} t_hidKeyDevice;

/*##########################################################################
	GLOBAL VARIABLES
##########################################################################*/

// joystick globals
static Boolean			sHIDInitialized;
static int				sMouseDeltaXAxis[kMaxMAMEJoysticks];
static int				sMouseDeltaYAxis[kMaxMAMEJoysticks];

static vector<t_hidJoyDevice *>	sJoysticks;
static vector<t_hidKeyDevice *>	sKeyboards;

static UInt8			sKeyState[256];
static int				sTotalCodes;
static bool				sFoundRightModifiers;

#define MAX_JOY_ENTRIES (kMaxMAMEJoysticks * (kMaxNeeds) + kMaxMouseButtons)

#define MAX_KEY_ENTRIES (kMaxKeys + kSpecialKeys + 1)

static os_code_info	sCodeList[MAX_KEY_ENTRIES+MAX_JOY_ENTRIES];

// list of keys that have fixed scancodes, together with their MAME code
static const KeyboardLookup sFixedKeyList[] =
{
	{ kMacKeyA, 			KEYCODE_A },
	{ kMacKeyB, 			KEYCODE_B },
	{ kMacKeyC, 			KEYCODE_C },
	{ kMacKeyD, 			KEYCODE_D },
	{ kMacKeyE, 			KEYCODE_E },
	{ kMacKeyF, 			KEYCODE_F },
	{ kMacKeyG, 			KEYCODE_G },
	{ kMacKeyH, 			KEYCODE_H },
	{ kMacKeyI, 			KEYCODE_I },
	{ kMacKeyJ, 			KEYCODE_J },
	{ kMacKeyK, 			KEYCODE_K },
	{ kMacKeyL, 			KEYCODE_L },
	{ kMacKeyM, 			KEYCODE_M },
	{ kMacKeyN, 			KEYCODE_N },
	{ kMacKeyO, 			KEYCODE_O },
	{ kMacKeyP, 			KEYCODE_P },
	{ kMacKeyQ, 			KEYCODE_Q },
	{ kMacKeyR, 			KEYCODE_R },
	{ kMacKeyS, 			KEYCODE_S },
	{ kMacKeyT, 			KEYCODE_T },
	{ kMacKeyU, 			KEYCODE_U },
	{ kMacKeyV, 			KEYCODE_V },
	{ kMacKeyW, 			KEYCODE_W },
	{ kMacKeyX, 			KEYCODE_X },
	{ kMacKeyY, 			KEYCODE_Y },
	{ kMacKeyZ, 			KEYCODE_Z },

	{ kMacKey0, 			KEYCODE_0 },
	{ kMacKey1, 			KEYCODE_1 },
	{ kMacKey2, 			KEYCODE_2 },
	{ kMacKey3, 			KEYCODE_3 },
	{ kMacKey4, 			KEYCODE_4 },
	{ kMacKey5, 			KEYCODE_5 },
	{ kMacKey6, 			KEYCODE_6 },
	{ kMacKey7, 			KEYCODE_7 },
	{ kMacKey8, 			KEYCODE_8 },
	{ kMacKey9, 			KEYCODE_9 },

	{ kMacKeyKeypad0, 		KEYCODE_0_PAD },
	{ kMacKeyKeypad1, 		KEYCODE_1_PAD },
	{ kMacKeyKeypad2, 		KEYCODE_2_PAD },
	{ kMacKeyKeypad3, 		KEYCODE_3_PAD },
	{ kMacKeyKeypad4, 		KEYCODE_4_PAD },
	{ kMacKeyKeypad5, 		KEYCODE_5_PAD },
	{ kMacKeyKeypad6, 		KEYCODE_6_PAD },
	{ kMacKeyKeypad7, 		KEYCODE_7_PAD },
	{ kMacKeyKeypad8, 		KEYCODE_8_PAD },
	{ kMacKeyKeypad9, 		KEYCODE_9_PAD },

	{ kMacKeyF1, 			KEYCODE_F1 },
	{ kMacKeyF2, 			KEYCODE_F2 },
	{ kMacKeyF3, 			KEYCODE_F3 },
	{ kMacKeyF4,			KEYCODE_F4 },
	{ kMacKeyF5, 			KEYCODE_F5 },
	{ kMacKeyF6, 			KEYCODE_F6 },
	{ kMacKeyF7, 			KEYCODE_F7 },
	{ kMacKeyF8, 			KEYCODE_F8 },
	{ kMacKeyF9, 			KEYCODE_F9 },
	{ kMacKeyF10, 			KEYCODE_F10 },
	{ kMacKeyF11,			KEYCODE_F11 },
	{ kMacKeyF12, 			KEYCODE_F12 },
	{ kMacKeyEscape, 		KEYCODE_ESC },

	{ kMacKeyTilde, 		KEYCODE_TILDE },
	{ kMacKeyMinus,			KEYCODE_MINUS },
	{ kMacKeyEquals,		KEYCODE_EQUALS },
	{ kMacKeyBackspace, 	KEYCODE_BACKSPACE },
	{ kMacKeyTab, 			KEYCODE_TAB },
	{ kMacKeyOpenBrace,		KEYCODE_OPENBRACE },
	{ kMacKeyCloseBrace,    KEYCODE_CLOSEBRACE },
	{ kMacKeyReturn, 		KEYCODE_ENTER },
	{ kMacKeyColon,			KEYCODE_COLON },
	{ kMacKeyQuote,			KEYCODE_QUOTE },
	{ kMacKeyBackslash,		KEYCODE_BACKSLASH },
#ifdef MESS
	{ kMacKeyBackslash2,    KEYCODE_BACKSLASH2 },
#endif
	{ kMacKeyComma,			KEYCODE_COMMA },
	{ kMacKeyStop,			KEYCODE_STOP },
	{ kMacKeySlash,			KEYCODE_SLASH },
	{ kMacKeySpace, 		KEYCODE_SPACE },

	{ kMacKeyInsert, 		KEYCODE_INSERT },
	{ kMacKeyDel, 			KEYCODE_DEL },
	{ kMacKeyHome, 			KEYCODE_HOME },
	{ kMacKeyEnd, 			KEYCODE_END },
	{ kMacKeyPageUp, 		KEYCODE_PGUP },
	{ kMacKeyPageDown, 		KEYCODE_PGDN },
	{ kMacKeyLeft, 			KEYCODE_LEFT },
	{ kMacKeyRight, 		KEYCODE_RIGHT },
	{ kMacKeyUp, 			KEYCODE_UP },
	{ kMacKeyDown, 			KEYCODE_DOWN },

	{ kMacKeyKeypadSlash,   KEYCODE_SLASH_PAD },
	{ kMacKeyKeypadAsterisk,KEYCODE_ASTERISK },
	{ kMacKeyKeypadMinus,   KEYCODE_MINUS_PAD },
	{ kMacKeyKeypadPlus,    KEYCODE_PLUS_PAD },
	{ kMacKeyKeypadStop,    KEYCODE_DEL_PAD },
	{ kMacKeyKeypadEnter,   KEYCODE_ENTER_PAD },
	{ kMacKeyPrintScreen,   KEYCODE_PRTSCR },
	{ kMacKeyPause,			KEYCODE_PAUSE },

	{ kMacKeyLeftShift, 	KEYCODE_LSHIFT },
	{ kMacKeyRightShift, 	KEYCODE_RSHIFT },
	{ kMacKeyLeftControl, 	KEYCODE_LCONTROL },
	{ kMacKeyRightControl,	KEYCODE_RCONTROL },
	{ kMacKeyLeftOption, 	KEYCODE_LALT },
	{ kMacKeyRightOption, 	KEYCODE_RALT },


	{ kMacKeyScreenLock,    KEYCODE_SCRLOCK },
	{ kMacKeyNumLock,		KEYCODE_NUMLOCK },
	{ kMacKeyCapsLock,		KEYCODE_CAPSLOCK },

	{ kMacKeySpecialExit,	KEYCODE_MENU },
#ifdef MESS
	{ kMacKeyKeypadEquals,	KEYCODE_MENU },
#endif
	{ -1, 					-1 }
};

// macros for differentiating the two
#define IS_KEYBOARD_CODE(code)		(((code) & 0x80000000) == 0)
#define IS_JOYSTICK_CODE(code)		(((code) & 0x80000000) != 0)

// macros for building/mapping joystick codes
#define JOYCODE(joy, type, index)	((index) | ((type) << 8) | ((joy) << 12) | 0x80000000)
#define JOYINDEX(joycode)			((joycode) & 0xff)
#define CODETYPE(joycode)			(((joycode) >> 8) & 0xf)
#define JOYNUM(joycode)				(((joycode) >> 12) & 0xf)

// joystick types
#define CODETYPE_KEYBOARD			0
#define CODETYPE_AXIS_NEG			1
#define CODETYPE_AXIS_POS			2
#define CODETYPE_POV_UP				3
#define CODETYPE_POV_DOWN			4
#define CODETYPE_POV_LEFT			5
#define CODETYPE_POV_RIGHT			6
#define CODETYPE_BUTTON				7
#define CODETYPE_JOYAXIS			8
#define CODETYPE_MOUSEAXIS			9
#define CODETYPE_MOUSEBUTTON		10
#define CODETYPE_GUNAXIS			11

// master joystick translation table
static os_code joy_trans_table[][2] =
{
	// internal code					MAME code
	{ JOYCODE(0, CODETYPE_AXIS_NEG, 0),	JOYCODE_1_LEFT },
	{ JOYCODE(0, CODETYPE_AXIS_POS, 0),	JOYCODE_1_RIGHT },
	{ JOYCODE(0, CODETYPE_AXIS_NEG, 1),	JOYCODE_1_UP },
	{ JOYCODE(0, CODETYPE_AXIS_POS, 1),	JOYCODE_1_DOWN },
	{ JOYCODE(0, CODETYPE_BUTTON, 0),	JOYCODE_1_BUTTON1 },
	{ JOYCODE(0, CODETYPE_BUTTON, 1),	JOYCODE_1_BUTTON2 },
	{ JOYCODE(0, CODETYPE_BUTTON, 2),	JOYCODE_1_BUTTON3 },
	{ JOYCODE(0, CODETYPE_BUTTON, 3),	JOYCODE_1_BUTTON4 },
	{ JOYCODE(0, CODETYPE_BUTTON, 4),	JOYCODE_1_BUTTON5 },
	{ JOYCODE(0, CODETYPE_BUTTON, 5),	JOYCODE_1_BUTTON6 },
	{ JOYCODE(0, CODETYPE_BUTTON, 6),	JOYCODE_1_BUTTON7 },
	{ JOYCODE(0, CODETYPE_BUTTON, 7),	JOYCODE_1_BUTTON8 },
	{ JOYCODE(0, CODETYPE_BUTTON, 8),	JOYCODE_1_BUTTON9 },
	{ JOYCODE(0, CODETYPE_BUTTON, 9),	JOYCODE_1_BUTTON10 },
	{ JOYCODE(0, CODETYPE_BUTTON, 10),	JOYCODE_1_BUTTON11 },
	{ JOYCODE(0, CODETYPE_BUTTON, 11),	JOYCODE_1_BUTTON12 },
	{ JOYCODE(0, CODETYPE_BUTTON, 12),	JOYCODE_1_BUTTON13 },
	{ JOYCODE(0, CODETYPE_BUTTON, 13),	JOYCODE_1_BUTTON14 },
	{ JOYCODE(0, CODETYPE_BUTTON, 14),	JOYCODE_1_BUTTON15 },
	{ JOYCODE(0, CODETYPE_BUTTON, 15),	JOYCODE_1_BUTTON16 },
	{ JOYCODE(0, CODETYPE_JOYAXIS, 0),	JOYCODE_1_ANALOG_X },
	{ JOYCODE(0, CODETYPE_JOYAXIS, 1),	JOYCODE_1_ANALOG_Y },
	{ JOYCODE(0, CODETYPE_JOYAXIS, 2),	JOYCODE_1_ANALOG_Z },

	{ JOYCODE(1, CODETYPE_AXIS_NEG, 0),	JOYCODE_2_LEFT },
	{ JOYCODE(1, CODETYPE_AXIS_POS, 0),	JOYCODE_2_RIGHT },
	{ JOYCODE(1, CODETYPE_AXIS_NEG, 1),	JOYCODE_2_UP },
	{ JOYCODE(1, CODETYPE_AXIS_POS, 1),	JOYCODE_2_DOWN },
	{ JOYCODE(1, CODETYPE_BUTTON, 0),	JOYCODE_2_BUTTON1 },
	{ JOYCODE(1, CODETYPE_BUTTON, 1),	JOYCODE_2_BUTTON2 },
	{ JOYCODE(1, CODETYPE_BUTTON, 2),	JOYCODE_2_BUTTON3 },
	{ JOYCODE(1, CODETYPE_BUTTON, 3),	JOYCODE_2_BUTTON4 },
	{ JOYCODE(1, CODETYPE_BUTTON, 4),	JOYCODE_2_BUTTON5 },
	{ JOYCODE(1, CODETYPE_BUTTON, 5),	JOYCODE_2_BUTTON6 },
	{ JOYCODE(1, CODETYPE_BUTTON, 6),	JOYCODE_2_BUTTON7 },
	{ JOYCODE(1, CODETYPE_BUTTON, 7),	JOYCODE_2_BUTTON8 },
	{ JOYCODE(1, CODETYPE_BUTTON, 8),	JOYCODE_2_BUTTON9 },
	{ JOYCODE(1, CODETYPE_BUTTON, 9),	JOYCODE_2_BUTTON10 },
	{ JOYCODE(1, CODETYPE_BUTTON, 10),	JOYCODE_2_BUTTON11 },
	{ JOYCODE(1, CODETYPE_BUTTON, 11),	JOYCODE_2_BUTTON12 },
	{ JOYCODE(1, CODETYPE_BUTTON, 12),	JOYCODE_2_BUTTON13 },
	{ JOYCODE(1, CODETYPE_BUTTON, 13),	JOYCODE_2_BUTTON14 },
	{ JOYCODE(1, CODETYPE_BUTTON, 14),	JOYCODE_2_BUTTON15 },
	{ JOYCODE(1, CODETYPE_BUTTON, 15),	JOYCODE_2_BUTTON16 },
	{ JOYCODE(1, CODETYPE_JOYAXIS, 0),	JOYCODE_2_ANALOG_X },
	{ JOYCODE(1, CODETYPE_JOYAXIS, 1),	JOYCODE_2_ANALOG_Y },
	{ JOYCODE(1, CODETYPE_JOYAXIS, 2),	JOYCODE_2_ANALOG_Z },

	{ JOYCODE(2, CODETYPE_AXIS_NEG, 0),	JOYCODE_3_LEFT },
	{ JOYCODE(2, CODETYPE_AXIS_POS, 0),	JOYCODE_3_RIGHT },
	{ JOYCODE(2, CODETYPE_AXIS_NEG, 1),	JOYCODE_3_UP },
	{ JOYCODE(2, CODETYPE_AXIS_POS, 1),	JOYCODE_3_DOWN },
	{ JOYCODE(2, CODETYPE_BUTTON, 0),	JOYCODE_3_BUTTON1 },
	{ JOYCODE(2, CODETYPE_BUTTON, 1),	JOYCODE_3_BUTTON2 },
	{ JOYCODE(2, CODETYPE_BUTTON, 2),	JOYCODE_3_BUTTON3 },
	{ JOYCODE(2, CODETYPE_BUTTON, 3),	JOYCODE_3_BUTTON4 },
	{ JOYCODE(2, CODETYPE_BUTTON, 4),	JOYCODE_3_BUTTON5 },
	{ JOYCODE(2, CODETYPE_BUTTON, 5),	JOYCODE_3_BUTTON6 },
	{ JOYCODE(2, CODETYPE_BUTTON, 6),	JOYCODE_3_BUTTON7 },
	{ JOYCODE(2, CODETYPE_BUTTON, 7),	JOYCODE_3_BUTTON8 },
	{ JOYCODE(2, CODETYPE_BUTTON, 8),	JOYCODE_3_BUTTON9 },
	{ JOYCODE(2, CODETYPE_BUTTON, 9),	JOYCODE_3_BUTTON10 },
	{ JOYCODE(2, CODETYPE_BUTTON, 10),	JOYCODE_3_BUTTON11 },
	{ JOYCODE(2, CODETYPE_BUTTON, 11),	JOYCODE_3_BUTTON12 },
	{ JOYCODE(2, CODETYPE_BUTTON, 12),	JOYCODE_3_BUTTON13 },
	{ JOYCODE(2, CODETYPE_BUTTON, 13),	JOYCODE_3_BUTTON14 },
	{ JOYCODE(2, CODETYPE_BUTTON, 14),	JOYCODE_3_BUTTON15 },
	{ JOYCODE(2, CODETYPE_BUTTON, 15),	JOYCODE_3_BUTTON16 },
	{ JOYCODE(2, CODETYPE_JOYAXIS, 0),	JOYCODE_3_ANALOG_X },
	{ JOYCODE(2, CODETYPE_JOYAXIS, 1),	JOYCODE_3_ANALOG_Y },
	{ JOYCODE(2, CODETYPE_JOYAXIS, 2),	JOYCODE_3_ANALOG_Z },

	{ JOYCODE(3, CODETYPE_AXIS_NEG, 0),	JOYCODE_4_LEFT },
	{ JOYCODE(3, CODETYPE_AXIS_POS, 0),	JOYCODE_4_RIGHT },
	{ JOYCODE(3, CODETYPE_AXIS_NEG, 1),	JOYCODE_4_UP },
	{ JOYCODE(3, CODETYPE_AXIS_POS, 1),	JOYCODE_4_DOWN },
	{ JOYCODE(3, CODETYPE_BUTTON, 0),	JOYCODE_4_BUTTON1 },
	{ JOYCODE(3, CODETYPE_BUTTON, 1),	JOYCODE_4_BUTTON2 },
	{ JOYCODE(3, CODETYPE_BUTTON, 2),	JOYCODE_4_BUTTON3 },
	{ JOYCODE(3, CODETYPE_BUTTON, 3),	JOYCODE_4_BUTTON4 },
	{ JOYCODE(3, CODETYPE_BUTTON, 4),	JOYCODE_4_BUTTON5 },
	{ JOYCODE(3, CODETYPE_BUTTON, 5),	JOYCODE_4_BUTTON6 },
	{ JOYCODE(3, CODETYPE_BUTTON, 6),	JOYCODE_4_BUTTON7 },
	{ JOYCODE(3, CODETYPE_BUTTON, 7),	JOYCODE_4_BUTTON8 },
	{ JOYCODE(3, CODETYPE_BUTTON, 8),	JOYCODE_4_BUTTON9 },
	{ JOYCODE(3, CODETYPE_BUTTON, 9),	JOYCODE_4_BUTTON10 },
	{ JOYCODE(3, CODETYPE_BUTTON, 10),	JOYCODE_4_BUTTON11 },
	{ JOYCODE(3, CODETYPE_BUTTON, 11),	JOYCODE_4_BUTTON12 },
	{ JOYCODE(3, CODETYPE_BUTTON, 12),	JOYCODE_4_BUTTON13 },
	{ JOYCODE(3, CODETYPE_BUTTON, 13),	JOYCODE_4_BUTTON14 },
	{ JOYCODE(3, CODETYPE_BUTTON, 14),	JOYCODE_4_BUTTON15 },
	{ JOYCODE(3, CODETYPE_BUTTON, 15),	JOYCODE_4_BUTTON16 },
	{ JOYCODE(3, CODETYPE_JOYAXIS, 0),	JOYCODE_4_ANALOG_X },
	{ JOYCODE(3, CODETYPE_JOYAXIS, 1),	JOYCODE_4_ANALOG_Y },
	{ JOYCODE(3, CODETYPE_JOYAXIS, 2),	JOYCODE_4_ANALOG_Z },

	{ JOYCODE(4, CODETYPE_AXIS_NEG, 0),	JOYCODE_5_LEFT },
	{ JOYCODE(4, CODETYPE_AXIS_POS, 0),	JOYCODE_5_RIGHT },
	{ JOYCODE(4, CODETYPE_AXIS_NEG, 1),	JOYCODE_5_UP },
	{ JOYCODE(4, CODETYPE_AXIS_POS, 1),	JOYCODE_5_DOWN },
	{ JOYCODE(4, CODETYPE_BUTTON, 0),	JOYCODE_5_BUTTON1 },
	{ JOYCODE(4, CODETYPE_BUTTON, 1),	JOYCODE_5_BUTTON2 },
	{ JOYCODE(4, CODETYPE_BUTTON, 2),	JOYCODE_5_BUTTON3 },
	{ JOYCODE(4, CODETYPE_BUTTON, 3),	JOYCODE_5_BUTTON4 },
	{ JOYCODE(4, CODETYPE_BUTTON, 4),	JOYCODE_5_BUTTON5 },
	{ JOYCODE(4, CODETYPE_BUTTON, 5),	JOYCODE_5_BUTTON6 },
	{ JOYCODE(4, CODETYPE_BUTTON, 6),	JOYCODE_5_BUTTON7 },
	{ JOYCODE(4, CODETYPE_BUTTON, 7),	JOYCODE_5_BUTTON8 },
	{ JOYCODE(4, CODETYPE_BUTTON, 8),	JOYCODE_5_BUTTON9 },
	{ JOYCODE(4, CODETYPE_BUTTON, 9),	JOYCODE_5_BUTTON10 },
	{ JOYCODE(4, CODETYPE_BUTTON, 10),	JOYCODE_5_BUTTON11 },
	{ JOYCODE(4, CODETYPE_BUTTON, 11),	JOYCODE_5_BUTTON12 },
	{ JOYCODE(4, CODETYPE_BUTTON, 12),	JOYCODE_5_BUTTON13 },
	{ JOYCODE(4, CODETYPE_BUTTON, 13),	JOYCODE_5_BUTTON14 },
	{ JOYCODE(4, CODETYPE_BUTTON, 14),	JOYCODE_5_BUTTON15 },
	{ JOYCODE(4, CODETYPE_BUTTON, 15),	JOYCODE_5_BUTTON16 },
	{ JOYCODE(4, CODETYPE_JOYAXIS, 0),	JOYCODE_5_ANALOG_X },
	{ JOYCODE(4, CODETYPE_JOYAXIS, 1), 	JOYCODE_5_ANALOG_Y },
	{ JOYCODE(4, CODETYPE_JOYAXIS, 2),	JOYCODE_5_ANALOG_Z },

	{ JOYCODE(5, CODETYPE_AXIS_NEG, 0),	JOYCODE_6_LEFT },
	{ JOYCODE(5, CODETYPE_AXIS_POS, 0),	JOYCODE_6_RIGHT },
	{ JOYCODE(5, CODETYPE_AXIS_NEG, 1),	JOYCODE_6_UP },
	{ JOYCODE(5, CODETYPE_AXIS_POS, 1),	JOYCODE_6_DOWN },
	{ JOYCODE(5, CODETYPE_BUTTON, 0),	JOYCODE_6_BUTTON1 },
	{ JOYCODE(5, CODETYPE_BUTTON, 1),	JOYCODE_6_BUTTON2 },
	{ JOYCODE(5, CODETYPE_BUTTON, 2),	JOYCODE_6_BUTTON3 },
	{ JOYCODE(5, CODETYPE_BUTTON, 3),	JOYCODE_6_BUTTON4 },
	{ JOYCODE(5, CODETYPE_BUTTON, 4),	JOYCODE_6_BUTTON5 },
	{ JOYCODE(5, CODETYPE_BUTTON, 5),	JOYCODE_6_BUTTON6 },
	{ JOYCODE(5, CODETYPE_BUTTON, 6),	JOYCODE_6_BUTTON7 },
	{ JOYCODE(5, CODETYPE_BUTTON, 7),	JOYCODE_6_BUTTON8 },
	{ JOYCODE(5, CODETYPE_BUTTON, 8),	JOYCODE_6_BUTTON9 },
	{ JOYCODE(5, CODETYPE_BUTTON, 9),	JOYCODE_6_BUTTON10 },
	{ JOYCODE(5, CODETYPE_BUTTON, 10),	JOYCODE_6_BUTTON11 },
	{ JOYCODE(5, CODETYPE_BUTTON, 11),	JOYCODE_6_BUTTON12 },
	{ JOYCODE(5, CODETYPE_BUTTON, 12),	JOYCODE_6_BUTTON13 },
	{ JOYCODE(5, CODETYPE_BUTTON, 13),	JOYCODE_6_BUTTON14 },
	{ JOYCODE(5, CODETYPE_BUTTON, 14),	JOYCODE_6_BUTTON15 },
	{ JOYCODE(5, CODETYPE_BUTTON, 15),	JOYCODE_6_BUTTON16 },
	{ JOYCODE(5, CODETYPE_JOYAXIS, 0),	JOYCODE_6_ANALOG_X },
	{ JOYCODE(5, CODETYPE_JOYAXIS, 1),	JOYCODE_6_ANALOG_Y },
	{ JOYCODE(5, CODETYPE_JOYAXIS, 2),	JOYCODE_6_ANALOG_Z },

	{ JOYCODE(6, CODETYPE_AXIS_NEG, 0),	JOYCODE_7_LEFT },
	{ JOYCODE(6, CODETYPE_AXIS_POS, 0),	JOYCODE_7_RIGHT },
	{ JOYCODE(6, CODETYPE_AXIS_NEG, 1),	JOYCODE_7_UP },
	{ JOYCODE(6, CODETYPE_AXIS_POS, 1),	JOYCODE_7_DOWN },
	{ JOYCODE(6, CODETYPE_BUTTON, 0),	JOYCODE_7_BUTTON1 },
	{ JOYCODE(6, CODETYPE_BUTTON, 1),	JOYCODE_7_BUTTON2 },
	{ JOYCODE(6, CODETYPE_BUTTON, 2),	JOYCODE_7_BUTTON3 },
	{ JOYCODE(6, CODETYPE_BUTTON, 3),	JOYCODE_7_BUTTON4 },
	{ JOYCODE(6, CODETYPE_BUTTON, 4),	JOYCODE_7_BUTTON5 },
	{ JOYCODE(6, CODETYPE_BUTTON, 5),	JOYCODE_7_BUTTON6 },
	{ JOYCODE(6, CODETYPE_BUTTON, 6),	JOYCODE_7_BUTTON7 },
	{ JOYCODE(6, CODETYPE_BUTTON, 7),	JOYCODE_7_BUTTON8 },
	{ JOYCODE(6, CODETYPE_BUTTON, 8),	JOYCODE_7_BUTTON9 },
	{ JOYCODE(6, CODETYPE_BUTTON, 9),	JOYCODE_7_BUTTON10 },
	{ JOYCODE(6, CODETYPE_BUTTON, 10),	JOYCODE_7_BUTTON11 },
	{ JOYCODE(6, CODETYPE_BUTTON, 11),	JOYCODE_7_BUTTON12 },
	{ JOYCODE(6, CODETYPE_BUTTON, 12),	JOYCODE_7_BUTTON13 },
	{ JOYCODE(6, CODETYPE_BUTTON, 13),	JOYCODE_7_BUTTON14 },
	{ JOYCODE(6, CODETYPE_BUTTON, 14),	JOYCODE_7_BUTTON15 },
	{ JOYCODE(6, CODETYPE_BUTTON, 15),	JOYCODE_7_BUTTON16 },
	{ JOYCODE(6, CODETYPE_JOYAXIS, 0),	JOYCODE_7_ANALOG_X },
	{ JOYCODE(6, CODETYPE_JOYAXIS, 1),	JOYCODE_7_ANALOG_Y },
	{ JOYCODE(6, CODETYPE_JOYAXIS, 2),	JOYCODE_7_ANALOG_Z },

	{ JOYCODE(7, CODETYPE_AXIS_NEG, 0),	JOYCODE_8_LEFT },
	{ JOYCODE(7, CODETYPE_AXIS_POS, 0),	JOYCODE_8_RIGHT },
	{ JOYCODE(7, CODETYPE_AXIS_NEG, 1),	JOYCODE_8_UP },
	{ JOYCODE(7, CODETYPE_AXIS_POS, 1),	JOYCODE_8_DOWN },
	{ JOYCODE(7, CODETYPE_BUTTON, 0),	JOYCODE_8_BUTTON1 },
	{ JOYCODE(7, CODETYPE_BUTTON, 1),	JOYCODE_8_BUTTON2 },
	{ JOYCODE(7, CODETYPE_BUTTON, 2),	JOYCODE_8_BUTTON3 },
	{ JOYCODE(7, CODETYPE_BUTTON, 3),	JOYCODE_8_BUTTON4 },
	{ JOYCODE(7, CODETYPE_BUTTON, 4),	JOYCODE_8_BUTTON5 },
	{ JOYCODE(7, CODETYPE_BUTTON, 5),	JOYCODE_8_BUTTON6 },
	{ JOYCODE(7, CODETYPE_BUTTON, 6),	JOYCODE_8_BUTTON7 },
	{ JOYCODE(7, CODETYPE_BUTTON, 7),	JOYCODE_8_BUTTON8 },
	{ JOYCODE(7, CODETYPE_BUTTON, 8),	JOYCODE_8_BUTTON9 },
	{ JOYCODE(7, CODETYPE_BUTTON, 9),	JOYCODE_8_BUTTON10 },
	{ JOYCODE(7, CODETYPE_BUTTON, 10),	JOYCODE_8_BUTTON11 },
	{ JOYCODE(7, CODETYPE_BUTTON, 11),	JOYCODE_8_BUTTON12 },
	{ JOYCODE(7, CODETYPE_BUTTON, 12),	JOYCODE_8_BUTTON13 },
	{ JOYCODE(7, CODETYPE_BUTTON, 13),	JOYCODE_8_BUTTON14 },
	{ JOYCODE(7, CODETYPE_BUTTON, 14),	JOYCODE_8_BUTTON15 },
	{ JOYCODE(7, CODETYPE_BUTTON, 15),	JOYCODE_8_BUTTON16 },
	{ JOYCODE(7, CODETYPE_JOYAXIS, 0),	JOYCODE_8_ANALOG_X },
	{ JOYCODE(7, CODETYPE_JOYAXIS, 1),	JOYCODE_8_ANALOG_Y },
	{ JOYCODE(7, CODETYPE_JOYAXIS, 2),	JOYCODE_8_ANALOG_Z },

	{ JOYCODE(0, CODETYPE_MOUSEBUTTON, 0), 	MOUSECODE_1_BUTTON1 },
	{ JOYCODE(0, CODETYPE_MOUSEBUTTON, 1), 	MOUSECODE_1_BUTTON2 },
	{ JOYCODE(0, CODETYPE_MOUSEBUTTON, 2), 	MOUSECODE_1_BUTTON3 },
	{ JOYCODE(0, CODETYPE_MOUSEBUTTON, 3), 	MOUSECODE_1_BUTTON4 },
	{ JOYCODE(0, CODETYPE_MOUSEBUTTON, 4), 	MOUSECODE_1_BUTTON5 },
	{ JOYCODE(0, CODETYPE_MOUSEAXIS, 0),	MOUSECODE_1_ANALOG_X },
	{ JOYCODE(0, CODETYPE_MOUSEAXIS, 1),	MOUSECODE_1_ANALOG_Y },
	{ JOYCODE(0, CODETYPE_MOUSEAXIS, 2),	MOUSECODE_1_ANALOG_Z },

	{ JOYCODE(1, CODETYPE_MOUSEBUTTON, 0), 	MOUSECODE_2_BUTTON1 },
	{ JOYCODE(1, CODETYPE_MOUSEBUTTON, 1), 	MOUSECODE_2_BUTTON2 },
	{ JOYCODE(1, CODETYPE_MOUSEBUTTON, 2), 	MOUSECODE_2_BUTTON3 },
	{ JOYCODE(1, CODETYPE_MOUSEBUTTON, 3), 	MOUSECODE_2_BUTTON4 },
	{ JOYCODE(1, CODETYPE_MOUSEBUTTON, 4), 	MOUSECODE_2_BUTTON5 },
	{ JOYCODE(1, CODETYPE_MOUSEAXIS, 0),	MOUSECODE_2_ANALOG_X },
	{ JOYCODE(1, CODETYPE_MOUSEAXIS, 1),	MOUSECODE_2_ANALOG_Y },
	{ JOYCODE(1, CODETYPE_MOUSEAXIS, 2),	MOUSECODE_2_ANALOG_Z },

	{ JOYCODE(0, CODETYPE_GUNAXIS, 0),		GUNCODE_1_ANALOG_X },
	{ JOYCODE(0, CODETYPE_GUNAXIS, 1),		GUNCODE_1_ANALOG_Y },

	{ JOYCODE(1, CODETYPE_GUNAXIS, 0),		GUNCODE_2_ANALOG_X },
	{ JOYCODE(1, CODETYPE_GUNAXIS, 1),		GUNCODE_2_ANALOG_Y },
	{ 0,0 }
};

/*##########################################################################
	FUNCTION PROTOTYPES
##########################################################################*/

// Keyboard Setup/Tear Down
static void			InitializeKeyboard(void);
static void			TearDownKeyboard(void);
static void 		BuildKeyList(void);
static INT32		mac_is_key_pressed(os_code inKeycode);

// Joystick routines
static void			InitializeHID(void);
static void			TearDownHID(void);
static void 		BuildJoyList(void);
static INT32		mac_is_joy_pressed(os_code inKeycode);

// Mouse routines
static void			ReadMouseDeltas (SInt32 *deltaX, SInt32 *deltaY);
static Boolean		ReadMouseButton (int inButtonNum);
static void			EnableMouse (Boolean inState);
static Boolean		InitializeMouse (void);

static void			AddHIDJoystickDevice (pRecDevice inDevice);
static void			AddHIDKeyboardDevice (pRecDevice inDevice);

/*##########################################################################
	INLINE FUNCTIONS
##########################################################################*/

inline Boolean TestKeyPressed(UInt8 inKeycode)
{
	return (sKeyState[inKeycode]);
}

/*##########################################################################
	IMPLEMENTATION
##########################################################################*/

static input_code keyoscode_to_code(int oscode)
{
	char str[256];
	sprintf(str, "SPECIAL%d", oscode - kMaxKeys - kScanCodeOffset);
	return token_to_code(str);
}

void MacMAME_InitInput (void)
{
	sTotalCodes = 0;
	
	InitializeKeyboard ();
	InitializeMouse ();
	InitializeHID ();
}

void MacMAME_ShutdownInput (void)
{
	TearDownHID ();
	TearDownKeyboard ();

	for (int i = 0; i < sTotalCodes; i ++)
		delete[] sCodeList[i].name;

	sTotalCodes = 0;
}

#pragma mark -
#pragma mark ¥ MAME Keyboard Handling

//===============================================================================
//	osd_get_code_list
//
//	Return a list of all available input codes (see input.h)
//===============================================================================

const os_code_info *osd_get_code_list(void)
{
	return sCodeList;
}

//===============================================================================
//	osd_get_code_value
//
//	Returns the current value of an input code
//===============================================================================

INT32 osd_get_code_value(os_code code)
{
	if (IS_KEYBOARD_CODE(code))
		return mac_is_key_pressed(code);
	else
		return mac_is_joy_pressed(code);
}


//===============================================================================
//	osd_customize_inputport_list
//
//	inptport.c defines some general purpose defaults for key bindings. They may be
//	further adjusted by the OS dependent code to better match the available keyboard,
//	e.g. one could map pause to the Pause key instead of P, or snapshot to PrtScr
//	instead of F12. Of course the user can further change the settings to anything
//	he/she likes.
//
//	This function is called on startup, before reading the configuration from disk.
//	Scan the list, and change the keys you want.
//===============================================================================

void osd_customize_inputport_list(input_port_default_entry *ioDefaults)
{
	while (ioDefaults->type != IPT_END)
	{
		// pause key: set to the escape key and set the name to NULL so it
		// doesn't show up in the configuration menu
		if (ioDefaults->type == IPT_UI_PAUSE)
		{
			ioDefaults->name = NULL;
#ifdef MESS
			seq_set_1(&ioDefaults->defaultseq, keyoscode_to_code(kMacKeySpecialPause + kScanCodeOffset));
#else
			seq_set_1(&ioDefaults->defaultseq, KEYCODE_ESC);
#endif
		}

		// cancel key: set to KEYCODE_MENU (kMacKeySpecialExit) and set the name to NULL so it
		// doesn't show up in the configuration menu
		if (ioDefaults->type == IPT_UI_CANCEL)
		{
			ioDefaults->name = NULL;
			seq_set_1(&ioDefaults->defaultseq, KEYCODE_MENU);
		}

#ifdef MESS
		// UI toggle key: set to KEYCODE_NONE and set the name to NULL so it
		// doesn't show up in the configuration menu
		if (ioDefaults->type == IPT_UI_TOGGLE_UI)
		{
			ioDefaults->name = NULL;
			seq_set_1(&ioDefaults->defaultseq, CODE_NONE);
		}

		if (gUseCoreCommandMenu)
		{
			int key = 0;

			switch (ioDefaults->type)
			{
			case IPT_UI_CONFIGURE:
				key = kMacKeySpecialUIConfigure;
				break;
			case IPT_UI_ON_SCREEN_DISPLAY:
				key = kMacKeySpecialUIOnScreenDisplay;
				break;
			case IPT_UI_RESET_MACHINE:
				key = kMacKeySpecialUIResetMachine;
				break;
			case IPT_UI_SHOW_GFX:
				key = kMacKeySpecialUIShowGfx;
				break;
			case IPT_UI_FRAMESKIP_DEC:
				key = kMacKeySpecialUIFrameskipDec;
				break;
			case IPT_UI_FRAMESKIP_INC:
				key = kMacKeySpecialUIFrameskipInc;
				break;
			case IPT_UI_THROTTLE:
				key = kMacKeySpecialUIThrottle;
				break;
			case IPT_UI_SHOW_FPS:
				key = kMacKeySpecialUIShowFps;
				break;
			case IPT_UI_SHOW_PROFILER:
				key = kMacKeySpecialUIShowProfiler;
				break;
			case IPT_UI_SNAPSHOT:
				key = kMacKeySpecialUISnapshot;
				break;
			case IPT_UI_TOGGLE_CHEAT:
				key = kMacKeySpecialUIToggleCheat;
				break;
			case IPT_UI_TOGGLE_DEBUG:
				key = kMacKeySpecialUIToggleDebug;
				break;
			case IPT_UI_SAVE_STATE:
				key = kMacKeySpecialUISaveState;
				break;
			case IPT_UI_LOAD_STATE:
				key = kMacKeySpecialUILoadState;
				break;
			case IPT_UI_TOGGLE_CROSSHAIR:
				key = kMacKeySpecialUIToggleCrosshair;
				break;
			}

			if (key)
			{
				ioDefaults->name = NULL;
				seq_set_1(&ioDefaults->defaultseq, keyoscode_to_code(key + kScanCodeOffset));
			}
		}
#endif
		ioDefaults++;
	}
}

//===============================================================================
//	mac_is_key_pressed
//
// 	Tell whether the specified key is pressed or not. keycode is the OS dependent
//	code specified in the list returned by osd_customize_inputport_defaults().
//===============================================================================

INT32 mac_is_key_pressed(os_code inKeycode)
{
	static Boolean sReturnedEscapeAsExit = false;
#ifdef MESS
	static Boolean sReturnedEscapeAsPause = false;
#endif
	
	// adjust for our offset
	inKeycode -= kScanCodeOffset;

	// if we're in the background, bail
	if (gInBackground)
		return false;
	
	// if we're out of bounds, bail
	if (inKeycode >= MAX_KEY_ENTRIES)
		return false;
	
	// handle special keys
	if (inKeycode >= kMaxKeys)
	{
		// special case for the exit "key"
		if (inKeycode == kMacKeySpecialExit)
		{
			// if we're trying to get out, always return like we're toggling the key very fast;
			// we have to do this because if we just always return true, MAME will get confused
			// waiting for the key to go back up before allowing you to quit if you try to, say,
			// quit from within the setup menu
			if (gExitToFrontend || gExitToShell)
			{
				static Boolean sLastFakeState = false;
				sLastFakeState = !sLastFakeState;
				return sLastFakeState;
			}
		
			// if the setup menu is active, just return the escape key state; but also remember
			// that we did this so that we can prevent ourselves from pausing as soon as the
			// menu disappears
			if (ui_is_setup_active())
			{
				Boolean result = TestKeyPressed(kMacKeyEscape);
				if (result)
					sReturnedEscapeAsExit = true;
				return result;
			}
			
			// otherwise, we return false
			return false;
		}
#ifdef MESS
		else if (inKeycode == kMacKeySpecialPause)
		{
			Boolean result;

			result = (TestKeyPressed(kMacKeyEscape)
							&& (TestKeyPressed(kMacKeyCommand)
								|| (gEmulationPaused
										? false
										: ! (Machine->gamedrv->flags & GAME_COMPUTER))));

			// if the setup menu is active, don't pause; escape means "exit" in this case
			if (ui_is_setup_active())
				return false;

			if (! TestKeyPressed(kMacKeyEscape))
				sReturnedEscapeAsExit = false;
			// if we just used escape to get out of menus, don't let it pause us; since we use
			// escape to get out of the menus, the key will likely still be down immediately after 
			// the menu is closed; we don't want to pause in that case
			if (sReturnedEscapeAsExit)
				return false;

			// if we're waiting to pause (or resume), do it now
			if (gPauseEmulationASAP)
			{
				gPauseEmulationASAP = false;
				return true;
			}

			if (result)
				sReturnedEscapeAsPause = true;
			return result;
		}
		/*else if ((! gUseCoreCommandMenu) && (inKeycode == kMacKeySpecialUIToggle))
		{
			Boolean result = TestKeyPressed(kMacKeyCommand) && TestKeyPressed(kMacKeyReturn);
			return result;
		}*/
		else if (gUseCoreCommandMenu)
		{
//			if ((inKeycode == sCurrentCommand) && (sCurrentCommand != -1))
//			{	// Menu command: this is more a proof of concept than something
//				// useful, yet it is already nice as it is.
//				sCurrentCommand = -1;
//				return true;
//			}
//			else
			{	// otherwise, test for Command+key combo
				if (TestKeyPressed(kMacKeyCommand))
				{
					switch (inKeycode)
					{
					case kMacKeySpecialUIConfigure:
						return TestKeyPressed(kMacKeyReturn);
						break;
					case kMacKeySpecialUIOnScreenDisplay:
						return TestKeyPressed(kMacKeyTilde);
						break;
					case kMacKeySpecialUIResetMachine:
						return TestKeyPressed(kMacKeyF3);
						break;
					case kMacKeySpecialUIShowGfx:
						return TestKeyPressed(kMacKeyF4);
						break;
					case kMacKeySpecialUIFrameskipDec:
						return TestKeyPressed(kMacKeyF8);
						break;
					case kMacKeySpecialUIFrameskipInc:
						return TestKeyPressed(kMacKeyF9);
						break;
					case kMacKeySpecialUIThrottle:
						return TestKeyPressed(kMacKeyF10);
						break;
					case kMacKeySpecialUIShowFps:
						return TestKeyPressed(kMacKeyF11)
								&& (! TestKeyPressed(kMacKeyLeftControl))
								&& (! TestKeyPressed(kMacKeyRightControl))
								&& (! TestKeyPressed(kMacKeyLeftShift))
								&& (! TestKeyPressed(kMacKeyRightShift));
						break;
					case kMacKeySpecialUIShowProfiler:
						return TestKeyPressed(kMacKeyF11)
								&& (TestKeyPressed(kMacKeyLeftShift)
									|| TestKeyPressed(kMacKeyRightShift));
						break;
					case kMacKeySpecialUISnapshot:
						return TestKeyPressed(kMacKeyF12);
						break;
					case kMacKeySpecialUIToggleCheat:
						return TestKeyPressed(kMacKeyF6);
						break;
					case kMacKeySpecialUIToggleDebug:
						return TestKeyPressed(kMacKeyF5);
						break;
					case kMacKeySpecialUISaveState:
						return TestKeyPressed(kMacKeyF7)
								&& (TestKeyPressed(kMacKeyLeftShift)
									|| TestKeyPressed(kMacKeyRightShift));
						break;
					case kMacKeySpecialUILoadState:
						return TestKeyPressed(kMacKeyF7)
								&& (! TestKeyPressed(kMacKeyLeftShift))
								&& (! TestKeyPressed(kMacKeyRightShift));
						break;
					case kMacKeySpecialUIToggleCrosshair:
						return TestKeyPressed(kMacKeyF1);
						break;
					}
				}
			}
		}
#endif
		// false for everyone else
		return false;
	}
	
#ifndef MESS
	// special case for the escape key
	if (inKeycode == kMacKeyEscape)
	{
		Boolean result = TestKeyPressed(kMacKeyEscape);

		// if the setup menu is active, don't pause; escape means "exit" in this case
		if (ui_is_setup_active())
			return false;

		// if we just used escape to get out of menus, don't let it pause us; since we use
		// escape to get out of the menus, the key will likely still be down immediately after 
		// the menu is closed; we don't want to pause in that case
		if (sReturnedEscapeAsExit && result)
			return false;
		sReturnedEscapeAsExit = false;
	
		// if we're waiting to pause (or resume), do it now
		if (gPauseEmulationASAP)
		{
			gPauseEmulationASAP = false;
			return true;
		}
		return result;
	}
#endif
	
	// if the command key is down, we don't register any keys; this is so that command-key
	// equivalents in the menus don't cause unintended side-effects
	if (TestKeyPressed(kMacKeyCommand))
		return false;

#ifdef MESS
	// R. Nabet 000601 :
	// There was a serious problem in MESS with CapsLock which is not handled the same in Mac
	// and PC.  On a PC, CapsLock is set when the key is pressed.  On a Mac, CapsLock is set
	// when Caps are on.  This code attempts to fix this.
	// Another possible fix would be hacking the revelant keyboard ressources.
	if (inKeycode == kMacKeyCapsLock)
	{
		static Boolean oldState = false;
		Boolean newState = TestKeyPressed(kMacKeyCapsLock);

		if (newState != oldState)
		{
			oldState = newState;
			return true;
		}
		else
			return false;
	}

	// special case for the escape key
	if (inKeycode == kMacKeyEscape)
	{
		Boolean result = TestKeyPressed(kMacKeyEscape);

		if (! result)
			sReturnedEscapeAsPause = sReturnedEscapeAsExit = false;

		if (sReturnedEscapeAsPause || sReturnedEscapeAsExit)
			return false;

		return result;
	}
#endif

	return TestKeyPressed(inKeycode);
}


//===============================================================================
//	osd_readkey_unicode
//
//	Returns the unicode value of the last key pressed.
//===============================================================================

int osd_readkey_unicode(int flush)
{
	return 0;
}

//===============================================================================
//	UpdateLEDs
//
//	Called once a frame to send the new LED status to the keyboard
//===============================================================================

void UpdateLEDs(int inLEDState)
{
	static int sLastLEDState;

	int diff = inLEDState ^ sLastLEDState;
	if (!diff) return;
	
	if (sKeyboards.size() == 0)
		return;
	
	for (int i = 0; i < ELEMENTS(sKeyboards[0]->ledElements); i ++)
	{
		if ((diff & (1 << i)) && sKeyboards[0]->ledElements[i])
		{
			IOHIDEventStruct hidstruct = {kIOHIDElementTypeOutput}; 
			IOReturn result = 0; 

			hidstruct.type = (IOHIDElementType)sKeyboards[0]->ledElements[i]->type; 
			hidstruct.elementCookie = (IOHIDElementCookie) sKeyboards[0]->ledElements[i]->cookie; 
			hidstruct.value = (inLEDState & (1 << i)) ? 1 : 0;
			result = HIDSetElementValue (sKeyboards[0]->device, sKeyboards[0]->ledElements[i], &hidstruct); 
		}
	}
	
	sLastLEDState = inLEDState;
}

#pragma mark -
#pragma mark ¥ Keyboard Setup/Tear Down

//===============================================================================
//	InitializeKeyboard
//===============================================================================

static void InitializeKeyboard(void)
{
	sKeyboards.clear();

	// build a list of keys
	BuildKeyList();
}

void SetModifierState (UInt32 inModifiers)
{
	static UInt32 lastModifiers;
	
	if (inModifiers == lastModifiers) return;
	
	sKeyState[kMacKeyCommand]		= (inModifiers & cmdKey)			? 1 : 0;
	sKeyState[kMacKeyCapsLock]		= (inModifiers & alphaLock)			? 1 : 0;

	// if sFoundRightModifiers is true, we're using the HID Manager to determine
	// the modifier key states.
	if (!sFoundRightModifiers)
	{
		sKeyState[kMacKeyLeftShift]		= (inModifiers & shiftKey)			? 1 : 0;
		sKeyState[kMacKeyLeftOption]	= (inModifiers & optionKey)			? 1 : 0;
		sKeyState[kMacKeyLeftControl]	= (inModifiers & controlKey)		? 1 : 0;
		sKeyState[kMacKeyRightShift]	= (inModifiers & rightShiftKey)		? 1 : 0;
		sKeyState[kMacKeyRightOption]	= (inModifiers & rightOptionKey)	? 1 : 0;
		sKeyState[kMacKeyRightControl]	= (inModifiers & rightControlKey)	? 1 : 0;
	}
	
	lastModifiers = inModifiers;
}
   	
static pascal OSStatus keyboardEventHandler (EventHandlerCallRef handlerChain, EventRef event, void *userData)
{
	UInt32 event_kind;
	UInt32 event_class;
	OSStatus err = eventNotHandledErr;
	UInt32 modifiers;
	UInt32 keyCode;
	UInt32 charCode;
	static EventTime delayTime = 0;

	/* tjl: Consume events that happen within 0.5 seconds of a window activation */
	if( delayTime > GetEventTime ( event ) )
		return noErr;
	
	event_class = GetEventClass (event);
	event_kind = GetEventKind (event);
	
	switch( event_class )
	{
		case kEventClassKeyboard:

			(void)GetEventParameter (event, kEventParamKeyCode, typeUInt32, NULL, sizeof(keyCode), NULL, &keyCode);
			(void)GetEventParameter (event, kEventParamKeyMacCharCodes, typeUInt32, NULL, sizeof(charCode), NULL, &charCode);
			(void)GetEventParameter (event, kEventParamKeyModifiers, typeUInt32, NULL, sizeof(modifiers), NULL, &modifiers);
				
			switch (event_kind)
			{
				case kEventRawKeyDown:
				case kEventRawKeyRepeat:
					sKeyState[keyCode] = 1;
					SetModifierState (modifiers);
					err = noErr;
					break;
					
				case kEventRawKeyUp:
					sKeyState[keyCode] = 0;
					SetModifierState (modifiers);
					err = noErr;
					break;
						
				case kEventRawKeyModifiersChanged:
					SetModifierState (modifiers);
					err = noErr;
					break;
			}
			break;
		
		case kEventClassWindow:
			switch (event_kind)
			{
				case kEventWindowDeactivated:
					/* tjl: clear state array when deactivating window*/
					memset (sKeyState, 0, sizeof (sKeyState));
					err = noErr;
					break;
				case kEventWindowActivated:
					/* tjl: record time of window reactivation */
					delayTime = GetEventTime ( event ) + 0.5;
					err = noErr;
					break;
			}
			break;
	}
					

	return err;
}

void InitializeKeyboardEvents (WindowRef inWindow)
{
	EventTypeSpec keyboardEventList[] =
	{
		{ kEventClassKeyboard, kEventRawKeyDown },
		{ kEventClassKeyboard, kEventRawKeyUp },
		{ kEventClassKeyboard, kEventRawKeyRepeat },
		{ kEventClassKeyboard, kEventRawKeyModifiersChanged },
		{ kEventClassWindow, kEventWindowDeactivated },
		{ kEventClassWindow, kEventWindowActivated }
	};
	
	// Attach the MAME core keyboard handler to this window
	InstallWindowEventHandler(inWindow, NewEventHandlerUPP(keyboardEventHandler), GetEventTypeCount(keyboardEventList), keyboardEventList, 0, NULL);
	
	// Clear the key state array
	memset (sKeyState, 0, sizeof (sKeyState));
}

//===============================================================================
//	TearDownKeyboard
//
//	Resets the keyboard to its original state.
//===============================================================================

static void TearDownKeyboard(void)
{
	return;
}
	

//===============================================================================
//	BuildKeyList
//
//	Builds a static list of keys and their MAME keycode equivalents.
//===============================================================================

static void BuildKeyList(void)
{
	int i, j;
	
	// loop over all the available keys
	for (i = 0; i < kMaxKeys; i++, sTotalCodes ++)
	{
	    input_code mameCode = CODE_OTHER_DIGITAL;
		UInt32 kchrState = 0;
		Str255 fullString;
	    SInt32 result;
	    UInt8 charCode;

		// we'll assume no modifiers are down to keep from assigning complex keystrokes to the actions
		result = KeyTranslate((Ptr)GetScriptManagerVariable(smKCHRCache), i, &kchrState);
		charCode = toupper(result & 0xff);
		
		// also look up in a resource what the proper name for this key is
		GetIndString(fullString, 130, i + 1);
		
		// if the full string was only 1 character, it is replaced by the code we found
		if (fullString[0] == 1 && charCode)
			fullString[1] = charCode;
		
		// if this code is in the fixed key list, assign the appropriate code
		for (j = 0; sFixedKeyList[j].fMacCode != -1; j++)
			if (sFixedKeyList[j].fMacCode == i)
			{
				mameCode = sFixedKeyList[j].fMAMECode;
				break;
			}
		
		// convert the final string into a C string
		p2cstrcpy((char *)fullString, fullString);
		
		// now fill in the record
		sCodeList[sTotalCodes].name = new char[strlen((char *)fullString) + 1];
		if (sCodeList[sTotalCodes].name)
			strcpy(sCodeList[sTotalCodes].name, (char *)fullString);
		sCodeList[sTotalCodes].oscode = i + kScanCodeOffset;
		sCodeList[sTotalCodes].inputcode = mameCode;
	}

	// add the special key entries
	for (i = 0; i < kSpecialKeys; i++, sTotalCodes++)
	{
		char str[256];
	    input_code mameCode = CODE_OTHER_DIGITAL;

		sprintf(str, "SPECIAL%d", i); /* This allows up  to reverse search for this key later */

		// if this code is in the fixed key list, assign the appropriate code
		for (j = 0; sFixedKeyList[j].fMacCode != -1; j++)
			if (sFixedKeyList[j].fMacCode == i + kMaxKeys)
			{
				mameCode = sFixedKeyList[j].fMAMECode;
				break;
			}
		
		sCodeList[sTotalCodes].name = new char[strlen(str)+1];
		strcpy( sCodeList[sTotalCodes].name, str );
		sCodeList[sTotalCodes].oscode = i + kMaxKeys + kScanCodeOffset;
		sCodeList[sTotalCodes].inputcode = mameCode;
	}
}


#pragma mark -
#pragma mark ¥ MAME Joystick Handling

//============================================================
//	add_joylist_entry
//============================================================

static void add_joylist_entry(const char *name, os_code code, input_code standardcode)
{
	// copy the name
	char *namecopy = new char[strlen(name) + 1];
	if (namecopy)
	{
		unsigned int entry;

		// find the table entry, if there is one
		for (entry = 0; entry < ELEMENTS(joy_trans_table); entry++)
			if (joy_trans_table[entry][0] == code)
				break;

		// fill in the joy description
		sCodeList[sTotalCodes].name = strcpy(namecopy, name);
		sCodeList[sTotalCodes].oscode = code;
		if (entry < ELEMENTS(joy_trans_table))
			standardcode = joy_trans_table[entry][1];
		sCodeList[sTotalCodes].inputcode = standardcode;
		sTotalCodes++;
	}
}

//===============================================================================
//	mac_is_joy_pressed
//
//	Tell whether the specified joystick direction/button is pressed or not.
//	joycode is the OS dependent code specified in the list returned by
//	mac_get_joy_list().
//===============================================================================

INT32 mac_is_joy_pressed(os_code joycode)
{
	int joyindex = JOYINDEX(joycode);
	int codetype = CODETYPE(joycode);
	int joynum = JOYNUM(joycode);
	INT32 val = ANALOG_VALUE_INVALID;
	INT32 top, bottom;
	INT32 pov;
	
	switch (codetype)
	{
		case CODETYPE_MOUSEBUTTON:
			return ReadMouseButton (joyindex);

		case CODETYPE_AXIS_POS:
		case CODETYPE_AXIS_NEG:
		{
			val = sJoysticks[joynum]->axisElementValue[joyindex];
			top = sJoysticks[joynum]->axisElements[joyindex]->max;
			bottom = sJoysticks[joynum]->axisElements[joyindex]->min;
			int middle = (top + bottom) / 2;
			double a2d_deadzone = 0.35;

			// watch for movement greater "a2d_deadzone" along either axis
			// FIXME in the two-axis joystick case, we need to find out
			// the angle. Anything else is unprecise.
			if (codetype == CODETYPE_AXIS_POS)
				return (val > middle + ((top - middle) * a2d_deadzone));
			else
				return (val < middle - ((middle - bottom) * a2d_deadzone));
		}

		case CODETYPE_BUTTON:
			return (sJoysticks[joynum]->buttonElementValue[joyindex] != 0);

		// anywhere from 0-45 (315) deg to 0+45 (45) deg
		case CODETYPE_POV_UP:
			pov = sJoysticks[joynum]->povElementValue[joyindex];
			return (pov != -1 && (pov >= 31500 || pov <= 4500));

		// anywhere from 90-45 (45) deg to 90+45 (135) deg
		case CODETYPE_POV_RIGHT:
			pov = sJoysticks[joynum]->povElementValue[joyindex];
			return (pov != -1 && (pov >= 4500 && pov <= 13500));

		// anywhere from 180-45 (135) deg to 180+45 (225) deg
		case CODETYPE_POV_DOWN:
			pov = sJoysticks[joynum]->povElementValue[joyindex];
			return (pov != -1 && (pov >= 13500 && pov <= 22500));

		// anywhere from 270-45 (225) deg to 270+45 (315) deg
		case CODETYPE_POV_LEFT:
			pov = sJoysticks[joynum]->povElementValue[joyindex];
			return (pov != -1 && (pov >= 22500 && pov <= 31500));

		case CODETYPE_JOYAXIS:
			gAnalogInputActive = true;

			val = sJoysticks[joynum]->axisElementValue[joyindex];
			top = sJoysticks[joynum]->axisElements[joyindex]->max;
			bottom = sJoysticks[joynum]->axisElements[joyindex]->min;

			val = (INT64)val * (INT64)(ANALOG_VALUE_MAX - ANALOG_VALUE_MIN) / (INT64)(top - bottom) + ANALOG_VALUE_MIN;

			if (val < ANALOG_VALUE_MIN) val = ANALOG_VALUE_MIN;
			if (val > ANALOG_VALUE_MAX) val = ANALOG_VALUE_MAX;
			return val;
			
		case CODETYPE_MOUSEAXIS:
			gAnalogInputActive = true;
			if (joyindex == 0)
				return sMouseDeltaXAxis[joynum] * 1024;
			if (joyindex == 1)
				return sMouseDeltaYAxis[joynum] * 1024;
			return 0;
	}

	return 0;
}

//===============================================================================
//	osd_joystick_needs_calibration
//
//	Do we need to calibrate the joystick at all?
//===============================================================================

int osd_joystick_needs_calibration(void)
{
	// we don't support joystick calibration
	return false;
}


//===============================================================================
//	osd_joystick_start_calibration
//
//	Preprocessing for joystick calibration.
//===============================================================================

void osd_joystick_start_calibration(void)
{
}


//===============================================================================
//	osd_joystick_calibrate_next
//
//	Prepare the next calibration step. Return a description of this step.
//	(e.g. "move to upper left")
//===============================================================================

const char *osd_joystick_calibrate_next(void)
{
	return "";
}


//===============================================================================
//	osd_joystick_calibrate
//
//	Get the actual joystick calibration data for the current position.
//===============================================================================

void osd_joystick_calibrate(void)
{
}


//===============================================================================
//	osd_joystick_end_calibration
//
//	Postprocessing (e.g. saving joystick data to config).
//===============================================================================

void osd_joystick_end_calibration(void)
{
}


//===============================================================================
//	PollInputs
//
//	Called periodically to update the mouse, joystick and keyboard states.
//===============================================================================

void PollInputs(void)
{
	// Read the mouse (via CarbonEvents) into player 1's delta data
	ReadMouseDeltas ((long *)&sMouseDeltaXAxis[0], (long *)&sMouseDeltaYAxis[0]);
	
	// If the HID Manager is not initialized, we can't go any further.
	if (!sHIDInitialized)
		return;

	long value;
	pRecDevice device;
	pRecElement element;

	for (int stick = 0; stick < sJoysticks.size(); stick++)
	{
		device = sJoysticks[stick]->device;
	
		// loop over all axes
		for (unsigned int axis = 0; axis < sJoysticks[stick]->axisElements.size(); axis++)
		{
			element = sJoysticks[stick]->axisElements[axis];
			value = HIDGetElementValue (device, element);
			value = HIDCalibrateValue (value, element);
			
			sJoysticks[stick]->axisElementValue[axis] = value;
		}

		// loop over all buttons
		for (unsigned int button = 0; button < sJoysticks[stick]->buttonElements.size(); button++)
		{
			element = sJoysticks[stick]->buttonElements[button];
			value = HIDGetElementValue (device, element);
			
			sJoysticks[stick]->buttonElementValue[button] = value;
		}

		// loop over all pov hatswitches
		for (unsigned int pov = 0; pov < sJoysticks[stick]->povElements.size(); pov++)
		{
			element = sJoysticks[stick]->povElements[pov];
			value = HIDGetElementValue (device, element);
			
			// If the value is outside the min/max range, it's probably in a centered/NULL state.
			if ((value < element->min) || (value > element->max))
			{
				if (element->nullState)
					value = -1;
			}
			else
			{
				// Do like DirectInput and express the hatswitch value in hundredths of a
				// degree, clockwise from north.
				value = 36000 / (element->max - element->min + 1) * (value - element->min);
			}
			
			sJoysticks[stick]->povElementValue[pov] = value;
		}
	}
	
	if (sFoundRightModifiers)
	{
		// We walk a list of all modifier key elements found on all keyboards and check their values.
		// We first assume that none of the modifiers are pressed, and as we find elements that report
		// as pressed, increment the value. This allows us to treat the same modifier keys on 2 different
		// keyboards as effectively the same key.
		
		// First, mark none of the modifier keys as being pressed.
		sKeyState[kMacKeyLeftControl] = 0;
		sKeyState[kMacKeyRightControl] = 0;
		sKeyState[kMacKeyLeftOption] = 0;
		sKeyState[kMacKeyRightOption] = 0;
		sKeyState[kMacKeyLeftShift] = 0;
		sKeyState[kMacKeyRightShift] = 0;
		
		// Second, loop over every keyboard device we found through the HID Manager.
		for (int keyboard = 0; keyboard < sKeyboards.size(); keyboard++)
		{
			device = sKeyboards[keyboard]->device;
	
			// Now check each keybooard for each of the 6 disctinct modifier keys we track with the HID Manager.
			for (int i = 0; i < ELEMENTS(sKeyboards[keyboard]->modifierElements); i ++)
			{
				value = 0;
				
				// Finally, some keyboards have 2 or more elements for the same modifier key, so we all of those here.
				for (int j = 0; j < sKeyboards[keyboard]->modifierElements[i].element.size(); j ++)
				{
					value += HIDGetElementValue (sKeyboards[keyboard]->device, sKeyboards[keyboard]->modifierElements[i].element[j]);
				}
				if (value)
					sKeyState[sKeyboards[keyboard]->modifierElements[i].keyCode] = 1;
			}
		}
	}
}


#pragma mark -
#pragma mark ¥ HID Manager Routines

//===============================================================================
//	InitializeHID
//
//	Initialize HID support
//===============================================================================

static void InitializeHID(void)
{
	UInt32 usagePages[] = { kHIDPage_GenericDesktop, kHIDPage_GenericDesktop, kHIDPage_GenericDesktop };
	UInt32 usages[] = { kHIDUsage_GD_GamePad, kHIDUsage_GD_Joystick, kHIDUsage_GD_Keyboard };
	
	// Try to build a list of all connected joysticks and game pads.
	sHIDInitialized = HIDBuildMultiDeviceList (usagePages, usages, ELEMENTS(usages));
	
	long deviceCount = 0;
	
	if (sHIDInitialized)
		deviceCount = HIDCountDevices();

	if (deviceCount > 0)
	{
		sHIDInitialized = true;
		
		// Walk the joystick list and add them to our sJoysticks array.
		pRecDevice curDevice = HIDGetFirstDevice();
		while (curDevice)
		{
			if ((curDevice->usage == kHIDUsage_GD_GamePad) || (curDevice->usage == kHIDUsage_GD_Joystick))
				AddHIDJoystickDevice (curDevice);
			else if (curDevice->usage == kHIDUsage_GD_Keyboard)
				AddHIDKeyboardDevice (curDevice);

			curDevice = HIDGetNextDevice (curDevice);
		}
	}

	BuildJoyList ();
}

static void AddHIDJoystickDevice (pRecDevice inDevice)
{
	if (inDevice == NULL) return;

	t_hidJoyDevice *newDevice = new t_hidJoyDevice;
	if (!newDevice) return;

	newDevice->device = inDevice;
	
#if MAME_DEBUG
	fprintf(stderr, "Joystick Device: %s\n", inDevice->product);
#endif

	// Now add all the elements we recognize
	pRecElement curElement = HIDGetFirstDeviceElement (inDevice, kHIDElementTypeIO);
	while (curElement)
	{
#if MAME_DEBUG
		fprintf(stderr, " element: %s (usage: %04x, usagePage: %04x)\n", curElement->name, curElement->usage, curElement->usagePage);
#endif

		if (curElement->type == kIOHIDElementTypeInput_Button)
		{
			// Found a button
			newDevice->buttonElements.push_back(curElement);
		}
		else if ((curElement->usagePage == kHIDPage_GenericDesktop) && (curElement->usage == kHIDUsage_GD_Hatswitch))
		{
			// Found a POV hatswitch
			newDevice->povElements.push_back(curElement);
		}
		else if (curElement->usagePage == kHIDPage_GenericDesktop)
		{
			switch (curElement->usage)
			{
				// The standard axis types for the generic desktop usage page.
				case kHIDUsage_GD_X:
				case kHIDUsage_GD_Y:
				case kHIDUsage_GD_Z:
				case kHIDUsage_GD_Rx:
				case kHIDUsage_GD_Ry:
				case kHIDUsage_GD_Rz:
				case kHIDUsage_GD_Slider:
				case kHIDUsage_GD_Dial:
				case kHIDUsage_GD_Wheel:
					// Found an analog axis
					newDevice->axisElements.push_back(curElement);
					break;
			}
		}
		else if (curElement->usagePage == kHIDPage_Simulation)
		{
			switch (curElement->usage)
			{
				// The standard axis types for the simulation usage page.
				// ¥¥¥ÊAre more usage types actually present in the wild?
				case kHIDUsage_Sim_Aileron:
				case kHIDUsage_Sim_Elevator:
				case kHIDUsage_Sim_Rudder:
				case kHIDUsage_Sim_Throttle:
					// Found an analog axis
					newDevice->axisElements.push_back(curElement);
					break;
			}
		}
		
		curElement = HIDGetNextDeviceElement (curElement, kHIDElementTypeIO);
	}
	
	sJoysticks.push_back(newDevice);
}

static void AddHIDKeyboardDevice (pRecDevice inDevice)
{
	if (inDevice == NULL) return;

	t_hidKeyDevice *newDevice = new t_hidKeyDevice;
	if (!newDevice) return;

	newDevice->device = inDevice;
	for (int i = 0; i < ELEMENTS(newDevice->ledElements); i ++)
		newDevice->ledElements[i] = NULL;
	
#if MAME_DEBUG
	fprintf(stderr, "Keyboard Device: %s\n", inDevice->product);
#endif

	// Now add all the elements we recognize
	pRecElement curElement = HIDGetFirstDeviceElement (inDevice, kHIDElementTypeIO);
	while (curElement)
	{
#if MAME_DEBUG
		fprintf(stderr, " element: %s (usage: %04x, usagePage: %04x)\n", curElement->name, curElement->usage, curElement->usagePage);
#endif

		if (curElement->usagePage == kHIDPage_LEDs)
		{
			switch (curElement->usage)
			{
				case kHIDUsage_LED_NumLock:
					newDevice->ledElements[0] = curElement;
					newDevice->ledElementValue[0] = HIDGetElementValue (inDevice, curElement);
					break;
				case kHIDUsage_LED_CapsLock:
					newDevice->ledElements[1] = curElement;
					newDevice->ledElementValue[1] = HIDGetElementValue (inDevice, curElement);
					break;
				case kHIDUsage_LED_ScrollLock:
					newDevice->ledElements[2] = curElement;
					newDevice->ledElementValue[2] = HIDGetElementValue (inDevice, curElement);
					break;
			}
		}
		else if (curElement->usagePage == kHIDPage_KeyboardOrKeypad)
		{
			switch (curElement->usage)
			{
				// It's possible for one keyboard to have multiple elements for the same
				// modifier key. For example, my MacAlly keyboard has 2 different elements
				// for each modifier key. We'll maintain a vector of elements for each modifer
				// and set the state based on the aggregate value of all. LBO 6/18/05.
				case kHIDUsage_KeyboardLeftControl:
					newDevice->modifierElements[0].element.push_back(curElement);
					newDevice->modifierElements[0].keyCode = kMacKeyLeftControl;
					break;
				case kHIDUsage_KeyboardLeftShift:
					newDevice->modifierElements[1].element.push_back(curElement);
					newDevice->modifierElements[1].keyCode = kMacKeyLeftShift;
					break;
				case kHIDUsage_KeyboardLeftAlt:
					newDevice->modifierElements[2].element.push_back(curElement);
					newDevice->modifierElements[2].keyCode = kMacKeyLeftOption;
					break;
				case kHIDUsage_KeyboardRightControl:
					sFoundRightModifiers = true;
					newDevice->modifierElements[3].element.push_back(curElement);
					newDevice->modifierElements[3].keyCode = kMacKeyRightControl;
					break;
				case kHIDUsage_KeyboardRightShift:
					sFoundRightModifiers = true;
					newDevice->modifierElements[4].element.push_back(curElement);
					newDevice->modifierElements[4].keyCode = kMacKeyRightShift;
					break;
				case kHIDUsage_KeyboardRightAlt:
					sFoundRightModifiers = true;
					newDevice->modifierElements[5].element.push_back(curElement);
					newDevice->modifierElements[5].keyCode = kMacKeyRightOption;
					break;
			}
		}
		
		curElement = HIDGetNextDeviceElement (curElement, kHIDElementTypeIO);
	}
	
	sKeyboards.push_back(newDevice);
}

//===============================================================================
//	TearDownHID
//
//	Tears down HID support.
//===============================================================================

static void TearDownHID(void)
{
	// don't bother if not initialized
	if (!sHIDInitialized)
		return;

	// Reset the keyboard LEDs to the state they were in when MacMAME was launched.
	if (sKeyboards.size() > 0)
	{
		for (int i = 0; i < ELEMENTS(sKeyboards[0]->ledElements); i ++)
		{
			if (sKeyboards[0]->ledElements[i])
			{
				IOHIDEventStruct hidstruct = {kIOHIDElementTypeOutput}; 
				IOReturn result = 0; 

				hidstruct.type = (IOHIDElementType)sKeyboards[0]->ledElements[i]->type; 
				hidstruct.elementCookie = (IOHIDElementCookie) sKeyboards[0]->ledElements[i]->cookie; 
				hidstruct.value = sKeyboards[0]->ledElementValue[i];
				result = HIDSetElementValue (sKeyboards[0]->device, sKeyboards[0]->ledElements[i], &hidstruct); 
			}
		}
	}
	
	// ¥¥¥ TODO deallocate keyboard vectors.
	
	vector<t_hidJoyDevice*>::iterator iter;
	for (iter = sJoysticks.begin(); iter != sJoysticks.end(); iter++)
	{
		(*iter)->axisElements.clear();
		(*iter)->buttonElements.clear();
		(*iter)->povElements.clear();
		(*iter)->axisElementValue.clear();
		(*iter)->buttonElementValue.clear();
		(*iter)->povElementValue.clear();
	}
	sJoysticks.clear();

	HIDReleaseDeviceList ();

	sHIDInitialized = false;
}


//===============================================================================
//	BuildJoyList
//
//	Builds a static list of joystick needs and their MAME joyycode equivalents.
//===============================================================================

static void BuildJoyList (void)
{
	char tempname[256];
	int verbose = 0;
#if MAME_DEBUG
	verbose = 1;
#endif

	// map mice first
	int mouse_count = 1; // ¥¥¥
	for (int mouse = 0; mouse < mouse_count; mouse++)
	{
		// add analog axes (fix me -- should enumerate these)
		sprintf(tempname, "Mouse %d X", mouse + 1);
		add_joylist_entry(tempname, JOYCODE(mouse, CODETYPE_MOUSEAXIS, 0), CODE_OTHER_ANALOG_RELATIVE);
		sprintf(tempname, "Mouse %d Y", mouse + 1);
		add_joylist_entry(tempname, JOYCODE(mouse, CODETYPE_MOUSEAXIS, 1), CODE_OTHER_ANALOG_RELATIVE);

		// add mouse buttons
		for (int button = 0; button < kMaxMouseButtons; button++)
		{
			// add mouse number to the name
			if (mouse_count > 1)
				sprintf(tempname, "Mouse #%d %d", mouse + 1, button + 1);
			else
				sprintf(tempname, "Mouse %d", button + 1);
			add_joylist_entry(tempname, JOYCODE(mouse, CODETYPE_MOUSEBUTTON, button), CODE_OTHER_DIGITAL);
		}
	}

	// map lightguns second
	int lightgun_count = 0; // ¥¥¥
	for (int gun = 0; gun < lightgun_count; gun++)
	{
		// add lightgun axes (fix me -- should enumerate these)
		sprintf(tempname, "Lightgun %d X", gun + 1);
		add_joylist_entry(tempname, JOYCODE(gun, CODETYPE_GUNAXIS, 0), CODE_OTHER_ANALOG_ABSOLUTE);
		sprintf(tempname, "Lightgun %d Y", gun + 1);
		add_joylist_entry(tempname, JOYCODE(gun, CODETYPE_GUNAXIS, 1), CODE_OTHER_ANALOG_ABSOLUTE);
	}

	// now map joysticks
	for (int stick = 0; stick < sJoysticks.size(); stick++)
	{
		// log the info
		if (verbose)
			fprintf(stderr, "Joystick %d: %s (%d axes, %d buttons, %d POVs)\n", stick + 1,
				sJoysticks[stick]->device->product,
				sJoysticks[stick]->axisElements.size(),
				sJoysticks[stick]->buttonElements.size(),
				sJoysticks[stick]->povElements.size());

		sJoysticks[stick]->axisElementValue.resize(sJoysticks[stick]->axisElements.size());

		// loop over all axes
		for (unsigned int axis = 0; axis < sJoysticks[stick]->axisElements.size(); axis++)
		{
			if (axis >= kMaxMAMEAnalogAxes) break;
			// reset the type
//			joystick_type[stick][axis] = AXIS_TYPE_INVALID;

			// attempt to get the object info
			pRecElement element = sJoysticks[stick]->axisElements[axis];
			if (element)
			{
//				if (verbose)
//					fprintf(stderr, "  Axis %d (%s)%s\n", axis, instance.tszName, joystick_digital[stick][axis] ? " - digital" : "");

				// add analog axis
//				if (!joystick_digital[stick][axis])
				{
					sprintf(tempname, "J%d %s", stick + 1, element->name);
					add_joylist_entry(tempname, JOYCODE(stick, CODETYPE_JOYAXIS, axis), CODE_OTHER_ANALOG_ABSOLUTE);
				}

				// add negative value
				sprintf(tempname, "J%d %s -", stick + 1, element->name);
				add_joylist_entry(tempname, JOYCODE(stick, CODETYPE_AXIS_NEG, axis), CODE_OTHER_DIGITAL);

				// add positive value
				sprintf(tempname, "J%d %s +", stick + 1, element->name);
				add_joylist_entry(tempname, JOYCODE(stick, CODETYPE_AXIS_POS, axis), CODE_OTHER_DIGITAL);
			}
		}

		sJoysticks[stick]->buttonElementValue.resize(sJoysticks[stick]->buttonElements.size());

		// loop over all buttons
		for (unsigned int button = 0; button < sJoysticks[stick]->buttonElements.size(); button++)
		{
			if (button >= kMaxMAMEButtons) break;

			// attempt to get the object info
			pRecElement element = sJoysticks[stick]->buttonElements[button];
			if (element)
			{
				// make the name for this item
				sprintf(tempname, "J%d %s", stick + 1, element->name);
				add_joylist_entry(tempname, JOYCODE(stick, CODETYPE_BUTTON, button), CODE_OTHER_DIGITAL);
			}
		}

		sJoysticks[stick]->povElementValue.resize(sJoysticks[stick]->povElements.size());

		// check POV hats
		for (unsigned int pov = 0; pov < sJoysticks[stick]->povElements.size(); pov++)
		{
			if (pov >= kMaxMAMEPOVs) break;

			// attempt to get the object info
			pRecElement element = sJoysticks[stick]->povElements[pov];
			if (element)
			{
				// add up direction
				sprintf(tempname, "J%d %s U", stick + 1, element->name);
				add_joylist_entry(tempname, JOYCODE(stick, CODETYPE_POV_UP, pov), CODE_OTHER_DIGITAL);

				// add down direction
				sprintf(tempname, "J%d %s D", stick + 1, element->name);
				add_joylist_entry(tempname, JOYCODE(stick, CODETYPE_POV_DOWN, pov), CODE_OTHER_DIGITAL);

				// add left direction
				sprintf(tempname, "J%d %s L", stick + 1, element->name);
				add_joylist_entry(tempname, JOYCODE(stick, CODETYPE_POV_LEFT, pov), CODE_OTHER_DIGITAL);

				// add right direction
				sprintf(tempname, "J%d %s R", stick + 1, element->name);
				add_joylist_entry(tempname, JOYCODE(stick, CODETYPE_POV_RIGHT, pov), CODE_OTHER_DIGITAL);
			}
		}
	}
}

//===============================================================================
//	ActivateInputDevices
//===============================================================================

void ActivateInputDevices(Boolean fullPause)
{
	EnableMouse (true);
}


//===============================================================================
//	DeactivateInputDevices
//===============================================================================

void DeactivateInputDevices(Boolean fullPause)
{
	EnableMouse (false);
}

#pragma mark -
#pragma mark ¥ Mouse handling

#define MAX_BUTTONS 32		// Carbon events supports up to 65536 buttons,
							// but we're a little more pragmatic

static EventHandlerUPP sMouseEventHandlerUPP;

static Point sMouseDelta;						// current delta value
static Boolean sMouseButtons[MAX_BUTTONS];		// current button state
static EventTime sMouseTime[MAX_BUTTONS];		// timestamp of last button event
static EventTime sMouseDeltaTime;				// timestamp of last delta event

static Boolean sMouseEnabled;

//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
// appMouseEventHandler
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
// This event handler is installed by InitializeMouse (below) to respond to
// Carbon events that affect the application as a whole. It does all the work
// of keeping track of mouse deltas and button states.
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

static pascal OSStatus appMouseEventHandler (EventHandlerCallRef myHandler, EventRef event, void* userData)
{
#pragma unused (myHandler, userData)
	OSStatus result = eventNotHandledErr;
	EventMouseButton theButton;
	Point mouseDelta;
	UInt32 eventKind;
	
	if (!sMouseEnabled) return eventNotHandledErr;
	
	eventKind = GetEventKind (event);
	
	switch (eventKind)
	{
		case kEventMouseMoved:
		case kEventMouseDragged:
		{
			EventTime evtTime = GetEventTime (event);
			
			result = GetEventParameter(event, kEventParamMouseDelta, typeQDPoint, NULL,
						  sizeof(mouseDelta), NULL, &mouseDelta);

			if (evtTime == sMouseDeltaTime) return eventNotHandledErr;

			// Clamp any overflow
			if ((SInt32) sMouseDelta.h + (SInt32) mouseDelta.h > 32767)
				sMouseDelta.h = 32767;
			else if ((SInt32) sMouseDelta.h + (SInt32) mouseDelta.h < -32767)
				sMouseDelta.h = -32767;
			else
				sMouseDelta.h += mouseDelta.h;

			if ((SInt32) sMouseDelta.v + (SInt32) mouseDelta.v > 32767)
				sMouseDelta.h = 32767;
			else if ((SInt32) sMouseDelta.v + (SInt32) mouseDelta.v < -32767)
				sMouseDelta.v = -32767;
			else
				sMouseDelta.v += mouseDelta.v;

			sMouseDeltaTime = evtTime;
			
			// We handled the event, eat it
			result = noErr;
			break;
		}
		case kEventMouseDown:
		{
			(void)GetEventParameter(event, kEventParamMouseButton, typeMouseButton, NULL,
						  sizeof(theButton), NULL, &theButton);

			// the button # is 1-based, our array is 0-based
			theButton -= 1;
		
			if (theButton < MAX_BUTTONS)
			{
				EventTime evtTime = GetEventTime (event);

				if (evtTime != sMouseTime[theButton])
				{
					sMouseButtons[theButton] = 1;
					sMouseTime[theButton] = evtTime;
					
					// We handled the event, eat it
					result = noErr;
				}
			}
			break;
		}
		case kEventMouseUp:
		{
			(void)GetEventParameter(event, kEventParamMouseButton, typeMouseButton, NULL,
						  sizeof(theButton), NULL, &theButton);

			// the button # is 1-based, our array is 0-based
			theButton -= 1;
		
			if (theButton < MAX_BUTTONS)
			{
				EventTime evtTime = GetEventTime (event);

				if (evtTime != sMouseTime[theButton])
				{
					sMouseButtons[theButton] = 0;
					sMouseTime[theButton] = evtTime;
					
					// We handled the event, eat it
					result = noErr;
				}
			}
			break;
		}
	}

	return result;
}

//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
// ReadMouseDeltas
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
// Effectively polls the mouse delta values as set by the Carbon event handlers.
// It also resets them so that the delta states accurately represent the delta
// since the last time this routine was called.
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

static void ReadMouseDeltas (SInt32 *deltaX, SInt32 *deltaY)
{
	*deltaX = sMouseDelta.h;
	*deltaY = sMouseDelta.v;
		
	// Reset them for the next time through
	sMouseDelta.h = sMouseDelta.v = 0;
}

//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
// ReadMouseButton
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
// Effectively polls the mouse button values based on the current
// states as set by the Carbon event handlers. First button is 0.
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

static Boolean ReadMouseButton (int inButtonNum)
{
	if (inButtonNum >= MAX_BUTTONS) return false;
	
	return sMouseButtons[inButtonNum];
}

//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
// EnableMouse
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
// Controls whether our Carbon event handler will handle mouse events or
// ignore them totally. Used around dialogs.
// If mouse events are enabled, we eat all the events we can handle, which
// means that they don't get passed back to the system.
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

static void EnableMouse (Boolean inState)
{
	int i;

	// set the state
	sMouseEnabled = inState;

	// reset our internal states to default values
	sMouseDelta.h = sMouseDelta.v = 0;
	for (i = 0; i < MAX_BUTTONS; i ++)
		sMouseButtons[i] = 0;
}

static Boolean InitializeMouse (void)
{
	EventHandlerRef	ref;
	OSStatus status;
	EventTypeSpec list[] =
	{
		{ kEventClassMouse, kEventMouseDown },
		{ kEventClassMouse, kEventMouseUp },
		{ kEventClassMouse, kEventMouseMoved },			// deltas while mouse button is up
		{ kEventClassMouse, kEventMouseDragged },		// deltas while mouse button is down
		{ kEventClassMouse, kEventMouseWheelMoved }
	};

	// Disable the mouse initially so that any pre-game dialogs will work
	sMouseEnabled = false;

	// Install an application event handler
	sMouseEventHandlerUPP = NewEventHandlerUPP (appMouseEventHandler);
	status = InstallApplicationEventHandler(sMouseEventHandlerUPP, GetEventTypeCount(list), list, 0, &ref);

	if (status != noErr) return false;

	// indicate success
	return true;
}