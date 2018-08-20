//============================================================
//
//	macdebugwin.c - Mac OS X debug window handling
//                  A port from Windows
//
//============================================================

// standard Mac OS X headers
#include <Carbon/Carbon.h>

// MAME headers
#include "driver.h"
#include "debugvw.h"
#include "debugcon.h"
#include "debugcpu.h"
#include "mac.h"
#include "macdebugwin.h"
#include "macvideo.h"

#pragma mark -
#pragma mark ¥ Defines

#define kCustomDebugViewClassID CFSTR("org.macmess.debugview")
#define kDebugWindowStyle	kWindowStandardDocumentAttributes | kWindowStandardHandlerAttribute | kWindowCompositingAttribute
#define MAX_VIEWS				4
#define EDGE_WIDTH				3
#define MAX_EDIT_STRING			256
#define HISTORY_LENGTH			20
#define MAX_OTHER_WND			4
#define DEBUG_FONT_NAME			"Monaco"
#define DEBUG_FONT_SIZE			10

#define cm_NewMemWindow 'CNMW'
#define cm_NewDisWindow 'CNDW'
#define cm_Run 'CRUN'
#define cm_RunAndHide 'CRAH'
#define cm_RunToNextCPU 'CRNC'
#define cm_RunToNextInterrupt 'CRNI'
#define cm_RunToNextVBLANK 'CRNV'
#define cm_StepInto 'CStI'
#define cm_StepOver 'CStO'
#define cm_StepOut 'CSOu'

#pragma mark -
#pragma mark ¥ Types

struct debugview_info
{
	struct debugwin_info *	owner;
	struct debug_view *		view;
	HIObjectRef				wnd;
	ControlRef				hscroll;
	ControlRef				vscroll;
	UINT8					at_bottom;
} ;

static struct debugview_info *creating_view_instance;

struct debugwin_info
{
	struct debugwin_info *	next;
	WindowRef				wnd;
	EventHandlerRef			handler;

	UINT32					minwidth, maxwidth;
	UINT32					minheight, maxheight;
	void					(*recompute_children)(struct debugwin_info *);

	UINT16					ignore_char_lparam;

	struct debugview_info	view[MAX_VIEWS];

	ControlRef				editwnd;
	void					(*process_string)(struct debugwin_info *, const char *);
	char					history[HISTORY_LENGTH][MAX_EDIT_STRING];
	int						history_count;
	int						last_history;

	ControlRef				otherwnd[MAX_OTHER_WND];
};

typedef float COLORREF[4];

#pragma mark -
#pragma mark ¥ LOCAL VARIABLES

static struct debugwin_info *main_console;
static struct debugwin_info *window_list;

static UINT8 debugger_active_countdown;
static UINT8 waiting_for_debugger;

static SInt32 hscroll_height;
static SInt32 vscroll_width;
static UINT32 debug_font_height;
static UINT32 debug_font_width;
static UINT32 debug_font_ascent;

static MenuRef console_contectual_menu;

#pragma mark -
#pragma mark ¥ Function prototypes

void osd_wait_for_debugger(void);
static struct debugwin_info *debug_window_create( CFStringRef title );
void console_create_window(void);
static void console_set_cpunum(int cpunum);
static void smart_show_window(WindowRef wnd, Boolean show);
static void smart_show_all(Boolean show);
pascal OSStatus debug_view_proc(EventHandlerCallRef inCaller, EventRef inEvent, void* inRefcon);
static void debug_view_update(struct debug_view *view);
static struct debugview_info *debug_view_find(struct debug_view *view);
static void smart_set_control_bounds(ControlRef wnd, WindowRef parent_window, HIObjectRef parent_view, Rect *bounds);
static void smart_show_control(ControlRef wnd, Boolean show);
static int debug_view_create(struct debugwin_info *info, int which, int type);
static void console_recompute_children(struct debugwin_info *info);
static void console_process_string(struct debugwin_info *info, const char *string);
static void debug_view_set_bounds(struct debugview_info *info, WindowRef parent, const Rect *newbounds);
pascal OSStatus debug_window_proc(EventHandlerCallRef inCaller, EventRef inEvent, void* inRefcon);
static void debug_view_draw_contents(struct debugview_info *view, CGContextRef windc);
pascal ControlKeyFilterResult console_edit_key_filter_proc_ptr( ControlRef theControl, SInt16 * keyCode, SInt16 * charCode, EventModifiers * modifiers );
static pascal void debug_view_live_scroll_proc (ControlHandle control, SInt16 part);
static void SetRGBAColor( COLORREF color, float R, float G, float B, float A );

#pragma mark -
#pragma mark ¥ OSD Functions

//============================================================
//	osd_wait_for_debugger
//============================================================

void osd_wait_for_debugger(void)
{
	mame_pause( 1 );
	waiting_for_debugger = 1;
	
	// create a console window
	if (!main_console)
	{
		console_create_window();
		ShowWindow( main_console->wnd );
		console_recompute_children(main_console);
	}

	// update the views in the console to reflect the current CPU
	if (main_console)
		console_set_cpunum(cpu_getactivecpu());

	// make sure the debug windows are visible
	smart_show_all(TRUE);

	// get and process messages
	ProcessEvents(true);

	// mark the debugger as active
	debugger_active_countdown = 2;
	waiting_for_debugger = 0;
}

//============================================================
//	debugwin_init_windows
//============================================================

int debugwin_init_windows(void)
{
	static Boolean class_registered = false;
	OSStatus myErr;
	
	// register the window classes
	if (class_registered == false)
	{
		static HIObjectClassRef theHIClass;
		static EventHandlerUPP debug_view_proc_upp = NULL;
		
		if( debug_view_proc_upp == NULL )
			debug_view_proc_upp = NewEventHandlerUPP( debug_view_proc );
		
		EventTypeSpec kFactoryEvents[] = { { kEventClassHIObject, kEventHIObjectConstruct },
										   { kEventClassHIObject, kEventHIObjectInitialize },
										   { kEventClassHIObject, kEventHIObjectDestruct },
//										   { kEventClassControl, kEventControlHitTest },
//										   { kEventClassControl, kEventControlTrack },
										   { kEventClassControl, kEventControlDraw },
										   { kEventClassControl, kEventControlContextualMenuClick },
										   };

		myErr = HIObjectRegisterSubclass(kCustomDebugViewClassID, kHIViewClassID, 0, debug_view_proc_upp,
								GetEventTypeCount(kFactoryEvents),kFactoryEvents, &creating_view_instance, &theHIClass);
		
		class_registered = true;
	}
	
	// create the font
	
	ATSFontMetrics  horizontalMetrics;
	ATSFontFamilyRef myFontRef;
	
	myFontRef = ATSFontFindFromName( CFSTR(DEBUG_FONT_NAME), kATSOptionFlagsDefault );

	myErr = ATSFontGetHorizontalMetrics( myFontRef, 0, &horizontalMetrics); 

	debug_font_width = horizontalMetrics.maxAdvanceWidth * DEBUG_FONT_SIZE;
	debug_font_height = (horizontalMetrics.ascent + horizontalMetrics.leading + (-1 * horizontalMetrics.descent)) * DEBUG_FONT_SIZE;
	debug_font_ascent = horizontalMetrics.ascent * DEBUG_FONT_SIZE;

	// get other metrics
	myErr = GetThemeMetric( kThemeMetricScrollBarWidth, &vscroll_width );
	myErr = GetThemeMetric( kThemeMetricScrollBarWidth, &hscroll_height );

	return 0;
}

#pragma mark -
#pragma mark ¥ Console Window Functions

//============================================================
//	console_create_window
//============================================================

void console_create_window(void)
{
	OSStatus myErr;
	struct debugwin_info *info;
	int bestwidth, bestheight;
	Rect bounds, work_bounds, portRect;
	UINT32 cpunum;
	ControlKeyFilterUPP keyFilter;
	
	// create the window
	info = debug_window_create( CFSTR("Debug") );
	if (!info)
		return;
	main_console = info;
	console_set_cpunum(0);

	// create the views
	if (!debug_view_create(info, 0, DVT_CONSOLE))
		goto cleanup;
	if (!debug_view_create(info, 1, DVT_DISASSEMBLY))
		goto cleanup;
	if (!debug_view_create(info, 2, DVT_REGISTERS))
		goto cleanup;

	// lock us to the bottom of the console by default
	info->view[0].at_bottom = 1;

	// set up the disassembly view to track the current pc
	debug_view_begin_update(info->view[1].view);
	debug_view_set_property_string(info->view[1].view, DVP_EXPRESSION, "pc");
	debug_view_set_property_UINT32(info->view[1].view, DVP_TRACK_LIVE, 1);
	debug_view_end_update(info->view[1].view);

	// create an edit box and override its key handling
	SetRect( &bounds, 0, 0, 100, 100 );
	myErr = CreateEditTextControl( info->wnd, &bounds, CFSTR(""), false, true, NULL, &(info->editwnd) );
	SetControlReference( info->editwnd, (SInt32) info );
	keyFilter = NewControlKeyFilterUPP( console_edit_key_filter_proc_ptr );
	SetControlData( info->editwnd, kControlEntireControl, kControlEditTextKeyFilterTag, sizeof( keyFilter ), &keyFilter );
	DisposeControlKeyFilterUPP( keyFilter );

	// set the child functions
	info->recompute_children = console_recompute_children;
	info->process_string = console_process_string;

	// loop over all CPUs and compute the sizes
	info->minwidth = 0;
	info->maxwidth = 0;
	for (cpunum = MAX_CPU - 1; (INT32)cpunum >= 0; cpunum--)
		if (Machine->drv->cpu[cpunum].cpu_type != CPU_DUMMY)
		{
			UINT32 regchars, dischars, conchars;
			UINT32 minwidth, maxwidth;

			// point all views to the new CPU number
			debug_view_set_property_UINT32(info->view[0].view, DVP_CPUNUM, cpunum);
			debug_view_set_property_UINT32(info->view[1].view, DVP_CPUNUM, cpunum);
			debug_view_set_property_UINT32(info->view[2].view, DVP_CPUNUM, cpunum);

			// get the total width of all three children
			conchars = debug_view_get_property_UINT32(info->view[0].view, DVP_TOTAL_COLS);
			dischars = debug_view_get_property_UINT32(info->view[1].view, DVP_TOTAL_COLS);
			regchars = debug_view_get_property_UINT32(info->view[2].view, DVP_TOTAL_COLS);

			// compute the preferred width
			minwidth = EDGE_WIDTH + regchars * debug_font_width + vscroll_width + 2 * EDGE_WIDTH + 100 + EDGE_WIDTH;
			maxwidth = EDGE_WIDTH + regchars * debug_font_width + vscroll_width + 2 * EDGE_WIDTH + ((dischars > conchars) ? dischars : conchars) * debug_font_width + vscroll_width + EDGE_WIDTH;
			if (minwidth > info->minwidth)
				info->minwidth = minwidth;
			if (maxwidth > info->maxwidth)
				info->maxwidth = maxwidth;
		}

	// get the work bounds
//	SystemParametersInfo(SPI_GETWORKAREA, 0, &work_bounds, 0);

	GDHandle theMainDevice;
	theMainDevice = GetMainDevice();
	work_bounds = (*theMainDevice)->gdRect;
	GetAvailableWindowPositioningBounds( theMainDevice, &work_bounds );

	// adjust the min/max sizes for the window style
	bounds.top = bounds.left = 0;
	bounds.right = bounds.bottom = info->minwidth;
	info->minwidth = bounds.right - bounds.left;

	bounds.top = bounds.left = 0;
	bounds.right = bounds.bottom = info->maxwidth;
	info->maxwidth = bounds.right - bounds.left;

	bestwidth = (info->maxwidth < (work_bounds.right - work_bounds.left)) ? info->maxwidth : (work_bounds.right - work_bounds.left);
	bestheight = (500 < (work_bounds.bottom - work_bounds.top)) ? 500 : (work_bounds.bottom - work_bounds.top);
	
	MoveWindow(info->wnd, work_bounds.right - bestwidth, work_bounds.bottom - bestheight, true);
	SizeWindow(info->wnd, bestwidth, bestheight, true);
	ClipRect(GetWindowPortBounds(info->wnd, &portRect));

//	// recompute the children
	console_recompute_children(info);

//	// mark the edit box as the default focus and set it
//	info->focuswnd = info->editwnd;

	SetKeyboardFocus( info->wnd, info->editwnd, kControlFocusNextPart );

	myErr = CreateNewMenu( 235, 0, &console_contectual_menu );
	require_noerr( myErr, cleanup );

	MenuItemIndex theIndex;

	myErr = AppendMenuItemTextWithCFString( console_contectual_menu, CFSTR("New Memory Window"), 0, cm_NewMemWindow, &theIndex );	
	myErr = SetMenuItemCommandKey( console_contectual_menu, theIndex, false, 'M' );
	myErr = AppendMenuItemTextWithCFString( console_contectual_menu, CFSTR("New Disassembly Window"), 0, cm_NewDisWindow, &theIndex );	
	myErr = SetMenuItemCommandKey( console_contectual_menu, theIndex, false, 'D' );
	myErr = AppendMenuItemTextWithCFString( console_contectual_menu, CFSTR("-"), kMenuItemAttrSeparator, 'SEPR', &theIndex );	
	myErr = AppendMenuItemTextWithCFString( console_contectual_menu, CFSTR("Run"), 0, cm_Run, &theIndex );	
	myErr = SetMenuItemKeyGlyph( console_contectual_menu, theIndex, kMenuF5Glyph );
	myErr = AppendMenuItemTextWithCFString( console_contectual_menu, CFSTR("Run and Hide Debugger"), 0, cm_RunAndHide, &theIndex );	
	myErr = SetMenuItemKeyGlyph( console_contectual_menu, theIndex, kMenuF12Glyph );
	myErr = AppendMenuItemTextWithCFString( console_contectual_menu, CFSTR("Run to Next CPU"), 0, cm_RunToNextCPU, &theIndex );	
	myErr = SetMenuItemKeyGlyph( console_contectual_menu, theIndex, kMenuF6Glyph );
	myErr = AppendMenuItemTextWithCFString( console_contectual_menu, CFSTR("Run until Next Interrupt on This CPU"), 0, cm_RunToNextInterrupt, &theIndex );	
	myErr = SetMenuItemKeyGlyph( console_contectual_menu, theIndex, kMenuF7Glyph );
	myErr = AppendMenuItemTextWithCFString( console_contectual_menu, CFSTR("Run until Next VBLANK"), 0, cm_RunToNextVBLANK, &theIndex );	
	myErr = SetMenuItemKeyGlyph( console_contectual_menu, theIndex, kMenuF8Glyph );
	myErr = AppendMenuItemTextWithCFString( console_contectual_menu, CFSTR("-"), kMenuItemAttrSeparator, 'SEPR', &theIndex );	
	myErr = AppendMenuItemTextWithCFString( console_contectual_menu, CFSTR("Step Into"), 0, cm_StepInto, &theIndex );	
	myErr = SetMenuItemKeyGlyph( console_contectual_menu, theIndex, kMenuF11Glyph );
	myErr = AppendMenuItemTextWithCFString( console_contectual_menu, CFSTR("Step Over"), 0, cm_StepOver, &theIndex );	
	myErr = SetMenuItemKeyGlyph( console_contectual_menu, theIndex, kMenuF10Glyph );
	myErr = AppendMenuItemTextWithCFString( console_contectual_menu, CFSTR("Step Out"), 0, cm_StepOut, &theIndex );	
	myErr = SetMenuItemKeyGlyph( console_contectual_menu, theIndex, kMenuF10Glyph );
	myErr = SetMenuItemModifiers( console_contectual_menu, theIndex, kMenuShiftModifier );

	return;

cleanup:
	if (info->view[2].view)
		debug_view_free(info->view[2].view);
	if (info->view[1].view)
		debug_view_free(info->view[1].view);
	if (info->view[0].view)
		debug_view_free(info->view[0].view);
}

//============================================================
//	console_process_string
//============================================================

static void console_process_string(struct debugwin_info *info, const char *string)
{
	char *empty = "";
	
	// an empty string is a single step
	if (string[0] == 0)
		debug_cpu_single_step(1);

	// otherwise, just process the command
	else
		debug_console_execute_command(string, 1);

	// clear the edit text box
	SetControlData( info->editwnd, kControlEditTextPart, kControlEditTextTextTag, 0, empty );
	HIViewSetNeedsDisplay( info->editwnd, true );	 
}

//============================================================
//	console_recompute_children
//============================================================

static void console_recompute_children(struct debugwin_info *info)
{
	Rect parent, regrect, disrect, conrect, editrect;
	UINT32 regchars, dischars, conchars;

	// get the parent's dimensions
	GetWindowBounds( info->wnd, kWindowContentRgn, &parent );
	OffsetRect( &parent, -parent.left, -parent.top );

	// get the total width of all three children
	conchars = debug_view_get_property_UINT32(info->view[0].view, DVP_TOTAL_COLS);
	dischars = debug_view_get_property_UINT32(info->view[1].view, DVP_TOTAL_COLS);
	regchars = debug_view_get_property_UINT32(info->view[2].view, DVP_TOTAL_COLS);

	// registers always get their desired width, and span the entire height
	regrect.top = parent.top + EDGE_WIDTH;
	regrect.bottom = parent.bottom - EDGE_WIDTH;
	regrect.left = parent.left + EDGE_WIDTH;
	regrect.right = regrect.left + debug_font_width * regchars + vscroll_width;

	// edit box goes at the bottom of the remaining area
	editrect.bottom = parent.bottom - EDGE_WIDTH;
	editrect.top = editrect.bottom - debug_font_height - 4;
	editrect.left = regrect.right + EDGE_WIDTH * 2;
	editrect.right = parent.right - EDGE_WIDTH;

	// console and disassembly split the difference
	disrect.top = parent.top + EDGE_WIDTH;
	disrect.bottom = (editrect.top - parent.top) / 2 - EDGE_WIDTH;
	disrect.left = regrect.right + EDGE_WIDTH * 2;
	disrect.right = parent.right - EDGE_WIDTH;

	conrect.top = disrect.bottom + EDGE_WIDTH * 2;
	conrect.bottom = editrect.top - EDGE_WIDTH;
	conrect.left = regrect.right + EDGE_WIDTH * 2;
	conrect.right = parent.right - EDGE_WIDTH;

	// set the bounds of things
	debug_view_set_bounds(&info->view[0], info->wnd, &conrect);
	debug_view_set_bounds(&info->view[1], info->wnd, &disrect);
	debug_view_set_bounds(&info->view[2], info->wnd, &regrect);
	smart_set_control_bounds(info->editwnd, info->wnd, NULL, &editrect);
}

//============================================================
//	console_set_cpunum
//============================================================

static void console_set_cpunum(int cpunum)
{
	CFStringRef newTitle = NULL, origTitle = NULL;
	OSStatus myErr;
	int viewnum;

	// first set all the views to the new cpu number
	for (viewnum = 0; viewnum < MAX_VIEWS; viewnum++)
		if (main_console->view[viewnum].view)
			debug_view_set_property_UINT32(main_console->view[viewnum].view, DVP_CPUNUM, cpunum);

	// then update the caption
	newTitle = CFStringCreateWithFormat( kCFAllocatorDefault, NULL, CFSTR("Debug: %s - CPU %d (%s)"), Machine->gamedrv->name, cpu_getactivecpu(), activecpu_name() );
	myErr = CopyWindowTitleAsCFString( main_console->wnd, &origTitle );

	if( myErr != noErr )
		return;

	if( newTitle != NULL )
	{
		if( CFStringCompare( newTitle, origTitle, kCFCompareCaseInsensitive ) != kCFCompareEqualTo )
		{
			SetWindowTitleWithCFString( main_console->wnd, newTitle );
			CFRelease( newTitle );
		}
	}
	
	CFRelease( origTitle );
}

pascal ControlKeyFilterResult console_edit_key_filter_proc_ptr( ControlRef theControl, SInt16 * keyCode, SInt16 * charCode, EventModifiers * modifiers )
{
	OSErr myErr;
	ControlEditTextSelectionRec	selection_record = {0, 255};
	ControlKeyFilterResult result = kControlKeyFilterPassKey;
	struct debugwin_info *info = (struct debugwin_info *)GetControlReference( theControl );
	char	buffer[256];
	Size	the_size = 256;
	
	if( *modifiers & cmdKey )
	{
		switch( *charCode )
		{
			case 'm':	/* New Memory Window */
				SysBeep( 0 );
				result = kControlKeyFilterBlockKey;
				break;
			case 'd':	/* New Disassembly Window */
				SysBeep( 0 );
				result = kControlKeyFilterBlockKey;
				break;
		}
		
		switch( *keyCode )
		{
			case 111:	/* Run and Hide Debugger */
				smart_show_all(FALSE);
			case 96:	/* Run */
				debug_cpu_go(~0);
				result = kControlKeyFilterBlockKey;
				break;
			case 97:	/* Run to next CPU */
				debug_cpu_next_cpu();
				result = kControlKeyFilterBlockKey;
				break;
			case 98:	/* Run until Next Interrupt */
				debug_cpu_go_interrupt(-1);
				result = kControlKeyFilterBlockKey;
				break;
			case 100:	/* Run until next VBLANK */
				debug_cpu_go_vblank();
				result = kControlKeyFilterBlockKey;
				break;
			case 103:	/* Step Into */
				debug_cpu_single_step(1);
				result = kControlKeyFilterBlockKey;
				break;
			case 109:
				if( *modifiers & shiftKey )
					debug_cpu_single_step_out();	/* Step Out */
				else
					debug_cpu_single_step_over(1);	/* Step Over */
				result = kControlKeyFilterBlockKey;
				break;
		}
	}
	else
	{
		switch( *charCode )
		{
			case 13:
				
				myErr = GetControlData( theControl, kControlEditTextPart, kControlEditTextTextTag, the_size, buffer, &the_size );
				buffer[the_size] = '\0';
				
				// add to the history if it's not a repeat of the last one
				if (buffer[0] != 0 && strcmp(buffer, &info->history[0][0]))
				{
					memmove(&info->history[1][0], &info->history[0][0], (HISTORY_LENGTH - 1) * MAX_EDIT_STRING);
					strcpy(&info->history[0][0], buffer);
					if (info->history_count < HISTORY_LENGTH)
						info->history_count++;
				}

				info->last_history = info->history_count - 1;

				if (info->process_string)
					(*info->process_string)(info, buffer);

				result = kControlKeyFilterBlockKey;
				break;
			case 30: /* Up arrow */
				if (info->last_history < info->history_count - 1)
					info->last_history++;
				else
					info->last_history = 0;
				
				SetControlData( info->editwnd, kControlEditTextPart, kControlEditTextTextTag,
							strlen(&info->history[info->last_history][0]), &info->history[info->last_history][0] );
				SetControlData( info->editwnd, kControlEditTextPart, kControlEditTextSelectionTag,
							sizeof(selection_record), &selection_record );

				HIViewSetNeedsDisplay( info->editwnd, true );
				result = kControlKeyFilterBlockKey;
				break;
			
			case 31: /* Down arrow */
				if (info->last_history > 0)
					info->last_history--;
				else if (info->history_count > 0)
					info->last_history = info->history_count - 1;
				else
					info->last_history = 0;

				SetControlData( info->editwnd, kControlEditTextPart, kControlEditTextTextTag,
							strlen(&info->history[info->last_history][0]), &info->history[info->last_history][0] );
				SetControlData( info->editwnd, kControlEditTextPart, kControlEditTextSelectionTag,
							sizeof(selection_record), &selection_record );
				HIViewSetNeedsDisplay( info->editwnd, true );
				result = kControlKeyFilterBlockKey;
				break;
		}
	}
	return result;
}

#pragma mark -
#pragma mark ¥ Debug View Functions

//============================================================
//	debug_view_create
//============================================================

static int debug_view_create(struct debugwin_info *info, int which, int type)
{
	struct debugview_info *view = &info->view[which];
	HIViewRef root;
	HIViewRef contentView;
	HIRect hiFrame = CGRectMake( 0, 0, 100, 100 );
	Rect frame;
	OSStatus myErr;
	static ControlActionUPP myLiveScrollerUPP = NULL;
	
	void *callback = (void *)debug_view_update;

	// set the owner
	view->owner = info;

	// create the child view

	creating_view_instance = view;
	myErr = HIObjectCreate( kCustomDebugViewClassID, NULL, &(view->wnd) );
	if( myErr != noErr )
		goto cleanup;
	
	// Place the view into the Window content view.
    root = HIViewGetRoot( info->wnd );
    HIViewFindByID( root, kHIViewWindowContentID, &contentView );
    HIViewAddSubview( contentView, ( HIViewRef )view->wnd );
    
    // Position the view.
    HIViewSetFrame( ( HIViewRef )view->wnd, &hiFrame );
    
    // Views are initially invisible, so make it visible.
    ShowControl( ( HIViewRef )view->wnd );

	if( myLiveScrollerUPP == NULL )
		myLiveScrollerUPP = NewControlActionUPP (debug_view_live_scroll_proc);

	// create the scroll bars
	SetRect( &frame, 0, 0, 100, 100 );
	myErr = CreateScrollBarControl( info->wnd, &frame, 50, 0, 100, 100, true, myLiveScrollerUPP, &(view->hscroll) );
	if( myErr != noErr )
		goto cleanup;
	myErr = CreateScrollBarControl( info->wnd, &frame, 50, 0, 100, 100, true, myLiveScrollerUPP, &(view->vscroll) );
	if( myErr != noErr )
		goto cleanup;

	SetControlReference( view->hscroll, (SInt32) view );
	SetControlReference( view->vscroll, (SInt32) view );

	// create the debug view
	view->view = debug_view_alloc(type);
	if (!view->view)
		goto cleanup;

	// set the update handler
	debug_view_set_property_fct(view->view, DVP_UPDATE_CALLBACK, callback);
	return 1;

cleanup:
	if (view->view)
		debug_view_free(view->view);
	if (view->hscroll)
		CFRelease(view->hscroll);
	if (view->vscroll)
		CFRelease(view->vscroll);
	if (view->wnd)
		CFRelease(view->wnd);
	return 0;
}

//============================================================
//	debug_view_proc
//============================================================

pascal OSStatus debug_view_proc(EventHandlerCallRef inCaller, EventRef inEvent, void* inRefcon)
{
	OSStatus result = eventNotHandledErr, myErr;
	struct debugview_info *info = (struct debugview_info *)inRefcon;
	UInt32 outUserSelectionType;
	SInt16 outMenuID;
	MenuItemIndex outMenuItem;
	Point mousePoint;
	Rect parent;
	WindowRef theWindow;
	
	switch( GetEventClass(inEvent) )
	{
		case kEventClassHIObject:
			switch( GetEventKind( inEvent ) )
			{
				case kEventHIObjectConstruct:
					info = *(struct debugview_info **)inRefcon;
					SetEventParameter( inEvent, kEventParamHIObjectInstance, typeVoidPtr, sizeof( struct debugview_info * ), &info );
					result = noErr;
					break;
				case kEventHIObjectInitialize:
					result = noErr;
					break;
				case kEventHIObjectDestruct:
					result = noErr;
					break;
			}
			break;

		case kEventClassControl:
			switch (GetEventKind(inEvent))
			{
				case kEventControlDraw:
					{
					CGContextRef    context;
					HIRect          bounds;

					result = GetEventParameter(inEvent, kEventParamCGContextRef, typeCGContextRef, NULL, sizeof(context), NULL, &context);
					HIViewGetBounds( (HIViewRef)info->wnd, &bounds); 
					CGContextTranslateCTM (context, 0, bounds.size.height); 
					CGContextScaleCTM (context, 1.0, -1.0);
					debug_view_draw_contents(info, context);
					result = noErr;
					break;
					}
				case kEventControlContextualMenuClick:
					myErr = GetEventParameter( inEvent, kEventParamMouseLocation, typeQDPoint, NULL, sizeof(mousePoint), NULL, &mousePoint);
					myErr = GetEventParameter( inEvent, kEventParamWindowRef, typeWindowRef, NULL, sizeof(theWindow), NULL, &theWindow);
					GetWindowBounds( theWindow, kWindowStructureRgn, &parent );
					mousePoint.h += parent.left;
					mousePoint.v += parent.top;
					myErr = ContextualMenuSelect ( console_contectual_menu, mousePoint,  0, kCMHelpItemNoHelp, "\p", NULL,
												   &outUserSelectionType, &outMenuID, &outMenuItem );
					if( myErr != userCanceledErr || myErr == noErr )
					{
						MenuCommand outCommandID;
						
						myErr = GetMenuItemCommandID( console_contectual_menu, outMenuItem, &outCommandID );
						switch( outCommandID )
						{
							case cm_NewMemWindow:
								SysBeep(0);
								break;
							case cm_NewDisWindow:
								SysBeep(0);
								break;
							case cm_RunAndHide:
								smart_show_all(FALSE);
							case cm_Run:
								debug_cpu_go(~0);
								break;
							case cm_RunToNextCPU:
								debug_cpu_next_cpu();
								break;
							case cm_RunToNextInterrupt:
								debug_cpu_go_interrupt(-1);
								break;
							case cm_RunToNextVBLANK:
								debug_cpu_go_vblank();
								break;
							case cm_StepInto:
								debug_cpu_single_step(1);
								break;
							case cm_StepOver:
								debug_cpu_single_step_over(1);
								break;
							case cm_StepOut:
								debug_cpu_single_step_out();
								break;
						}
						
						result = noErr;
					}
					break;
			}
			break;
	}

	return result;
}

//============================================================
//	debug_view_set_bounds
//============================================================

static void debug_view_set_bounds(struct debugview_info *info, WindowRef parent, const Rect *newbounds)
{
	Rect bounds = *newbounds;

	// account for the edges and set the bounds
	if (info->wnd)
		smart_set_control_bounds( (ControlRef)info->wnd, parent, NULL, &bounds);

	// update
	debug_view_update(info->view);
}

//============================================================
//	debug_view_update
//============================================================

static void debug_view_update(struct debug_view *view)
{
	struct debugview_info *info = debug_view_find(view);
	
	// if we have a view window, process it
	if (info && info->view)
	{
		Rect bounds, vscroll_bounds, hscroll_bounds;
		int show_vscroll, show_hscroll;
		UINT32 visible_rows, visible_cols;
		UINT32 total_rows, total_cols;
		UINT32 top_row, left_col;
		
//		if( IsControlVisible( ( HIViewRef )info->wnd ) == false )
//			return;
		
		// get the view window bounds
		GetControlBounds( ( HIViewRef )info->wnd, &bounds );
		if( waiting_for_debugger == 0 )
			fprintf( stderr, "debug_view_update Bounds: %d, %d, %d, %d\n", bounds.top, bounds.left, bounds.bottom, bounds.right );

		visible_rows = (bounds.bottom - bounds.top) / debug_font_height;
		visible_cols = (bounds.right - bounds.left) / debug_font_width;

		// get the updated total rows/cols and left row/col
		total_rows = debug_view_get_property_UINT32(view, DVP_TOTAL_ROWS);
		total_cols = debug_view_get_property_UINT32(view, DVP_TOTAL_COLS);
		top_row = debug_view_get_property_UINT32(view, DVP_TOP_ROW);
		left_col = debug_view_get_property_UINT32(view, DVP_LEFT_COL);

		// determine if we need to show the scrollbars
		show_vscroll = show_hscroll = 0;
		if (total_rows > visible_rows)
		{
			bounds.right -= vscroll_width;
			visible_cols = (bounds.right - bounds.left) / debug_font_width;
			show_vscroll = 1;
		}
		if (total_cols > visible_cols)
		{
			bounds.bottom -= hscroll_height;
			visible_rows = (bounds.bottom - bounds.top) / debug_font_height;
			show_hscroll = 1;
		}
		if (!show_vscroll && total_rows > visible_rows)
		{
			bounds.right -= vscroll_width;
			visible_cols = (bounds.right - bounds.left) / debug_font_width;
			show_vscroll = 1;
		}

		// compute the bounds of the scrollbars
		GetControlBounds( ( HIViewRef )info->wnd, &vscroll_bounds );
		vscroll_bounds.left = vscroll_bounds.right - vscroll_width;
		if (show_hscroll)
			vscroll_bounds.bottom -= hscroll_height;

		GetControlBounds( ( HIViewRef )info->wnd, &hscroll_bounds );
		hscroll_bounds.top = hscroll_bounds.bottom - hscroll_height;
		if (show_vscroll)
			hscroll_bounds.right -= vscroll_width;

		// if we hid the scrollbars, make sure we reset the top/left corners
		if (top_row + visible_rows > total_rows || info->at_bottom)
			top_row = (total_rows > visible_rows) ? (total_rows - visible_rows) : 0;
		if (left_col + visible_cols > total_cols)
			left_col = (total_cols > visible_cols) ? (total_cols - visible_cols) : 0;

		// fill out the scroll info struct for the vertical scrollbar
		SetControl32BitMaximum( info->vscroll, total_rows - visible_rows );
		SetControl32BitMinimum( info->vscroll, 0 );
		SetControl32BitValue( info->vscroll, top_row );
		SetControlViewSize( info->vscroll, visible_rows );
		
		// fill out the scroll info struct for the horizontal scrollbar
		SetControl32BitMaximum( info->hscroll, total_cols - visible_cols );
		SetControl32BitMinimum( info->hscroll, 0 );
		SetControl32BitValue( info->hscroll, left_col );
		SetControlViewSize( info->hscroll, visible_cols );

		// update window info
		visible_rows++;
		visible_cols++;
		debug_view_set_property_UINT32(view, DVP_VISIBLE_ROWS, visible_rows);
		debug_view_set_property_UINT32(view, DVP_VISIBLE_COLS, visible_cols);
		debug_view_set_property_UINT32(view, DVP_TOP_ROW, top_row);
		debug_view_set_property_UINT32(view, DVP_LEFT_COL, left_col);

		// invalidate the bounds
		HIViewSetNeedsDisplay( ( HIViewRef )info->wnd, true );

		// adjust the bounds of the scrollbars and show/hide them
		if (info->vscroll)
		{
			if (show_vscroll)
				smart_set_control_bounds(info->vscroll, NULL, info->wnd, &vscroll_bounds);
			smart_show_control(info->vscroll, show_vscroll);
		}
		if (info->hscroll)
		{
			if (show_hscroll)
				smart_set_control_bounds(info->hscroll, NULL, info->wnd, &hscroll_bounds);
			smart_show_control(info->hscroll, show_hscroll);
		}
	}
}

//============================================================
//	debug_view_find
//============================================================

static struct debugview_info *debug_view_find(struct debug_view *view)
{
	struct debugwin_info *info;
	int curview;

	// loop over windows and find the view
	for (info = window_list; info; info = info->next)
		for (curview = 0; curview < MAX_VIEWS; curview++)
			if (info->view[curview].view == view)
				return &info->view[curview];
	return NULL;
}


static void debug_view_draw_contents(struct debugview_info *view, CGContextRef windc)
{
	struct debug_view_char *viewdata;
	UINT32 visrows, viscols;
	UINT32 col, row;
	HIRect client;
	CGColorSpaceRef cg_space;
	HIRect bounds;

	// get the client rect
	client = CGContextGetClipBoundingBox ( windc );

	// first get the visible size from the view and a pointer to the data
	visrows = debug_view_get_property_UINT32(view->view, DVP_VISIBLE_ROWS);
	viscols = debug_view_get_property_UINT32(view->view, DVP_VISIBLE_COLS);
	viewdata = debug_view_get_property_ptr(view->view, DVP_VIEW_DATA);

	// set the font
	CGContextSelectFont (windc, DEBUG_FONT_NAME, DEBUG_FONT_SIZE, kCGEncodingMacRoman); 
	CGContextSetTextDrawingMode (windc, kCGTextFill);

	cg_space = CGColorSpaceCreateDeviceRGB();
	CGContextSetFillColorSpace (windc, cg_space);
	
	bounds.size.height = debug_font_height;
	bounds.size.width = debug_font_width;

	for( row = 0; row <visrows; row++ )
	{
		float text_y_pos;
		int last_attrib;
		int	start, size;
		char buffer[256];
		
		bounds.origin.y = (client.size.height - debug_font_height) - (row * debug_font_height);
		text_y_pos = bounds.origin.y + (debug_font_height - debug_font_ascent );
		start = size = 0;
		last_attrib = viewdata[0].attrib;
		
		for (col = 0; col < viscols; col++)
		{
			COLORREF color, bgcolor;
			
			buffer[ size++ ] = viewdata[col].byte;
			
			if( last_attrib == viewdata[col].attrib )
				if( col+1 < viscols )
					continue;
				
			/* Flush out drawing */
			bounds.origin.x = start * debug_font_width;
			bounds.size.width = size * debug_font_width;

			SetRGBAColor( bgcolor, 1, 1, 1, 1 );
			if (last_attrib & DCA_ANCILLARY) SetRGBAColor( bgcolor, 0.87, 0.87, 0.87, 1 );
			if (last_attrib & DCA_SELECTED) SetRGBAColor( bgcolor, 1, .5, .5, 1 );
			if (last_attrib & DCA_CURRENT) SetRGBAColor( bgcolor, 1, 1, 0, 1 );

			CGContextSetFillColor( windc, bgcolor );
			CGContextFillRect(windc, bounds);
			
			SetRGBAColor( color, 0, 0, 0, 1 );
			if (last_attrib & DCA_CHANGED) SetRGBAColor( color, 1, 0, 0, 1 );
			if (last_attrib & DCA_INVALID) SetRGBAColor( color, 0, 0, 1, 1 );
			if (last_attrib & DCA_DISABLED) SetRGBAColor( color, (color[0] + bgcolor[0]) / 2.0, (color[1] + bgcolor[1]) / 2.0, (color[2] + bgcolor[2]) / 2.0, 1 );
			
			CGContextSetFillColor( windc, color );
			CGContextShowTextAtPoint(windc, bounds.origin.x, text_y_pos, buffer, size);
			
			/* Get ready for next segement */
			start = col;
			size = 0;

			last_attrib = viewdata[col].attrib;
		}

		viewdata += viscols;
		
	}
}

//============================================================
//	debug_view_draw_contents
//============================================================

static pascal void debug_view_live_scroll_proc (ControlHandle control, SInt16 part)
{
    SInt16 currentValue, min, max;
	struct debugview_info *view = (struct debugview_info *)GetControlReference( control );
	
	if( view == NULL )
		return;
	
    currentValue = GetControl32BitValue (control); 
    min = GetControl32BitMinimum (control);
    max = GetControl32BitMaximum (control);

	switch( part )
	{	
		case kControlUpButtonPart:
			currentValue += -1;
			break;
		case kControlDownButtonPart:
			currentValue += 1;
			break;
		case kControlPageUpPart:
			currentValue += -10;
			break;
		case kControlPageDownPart:
			currentValue += 10;
		break;
	}
	
	if( currentValue < min )
		currentValue = min;
	
	if( currentValue > max )
		currentValue = max;

	if( control == view->hscroll )
		debug_view_set_property_UINT32(view->view, DVP_LEFT_COL, currentValue);
	else if( control == view->vscroll )
	{
		view->at_bottom = (currentValue == max);
		debug_view_set_property_UINT32(view->view, DVP_TOP_ROW, currentValue);
	}	
}

#pragma mark -
#pragma mark ¥ Utility functions

//============================================================
//	debug_window_create
//============================================================

static struct debugwin_info *debug_window_create( CFStringRef title )
{
	OSStatus myErr;
	struct debugwin_info *info = NULL;
	Rect work_bounds = {100, 100, 200, 200};
	static EventHandlerUPP debugwin_event_handler_upp = NULL;
	
	// allocate memory
	info = malloc(sizeof(*info));
	if (!info)
		return NULL;
	memset(info, 0, sizeof(*info));

	// create the window
	myErr =  CreateNewWindow( kDocumentWindowClass, kDebugWindowStyle, &work_bounds, &(info->wnd) );
	if (myErr != noErr)
		goto cleanup;
	
	myErr =  SetWindowTitleWithCFString( info->wnd, title );
	if (myErr != noErr)
		goto cleanup;
	
	EventTypeSpec DebugWindowEventList[] =
	{
//		{ kEventClassWindow, kEventWindowHandleContentClick },
		{ kEventClassWindow, kEventWindowClose },
//		{ kEventClassWindow, kEventWindowActivated },
//		{ kEventClassWindow, kEventWindowDeactivated },
//		{ kEventClassWindow, kEventWindowDrawContent },
//		{ kEventClassWindow, kEventWindowGetMaximumSize },
		{ kEventClassWindow, kEventWindowResizeCompleted },
//		{ kEventClassKeyboard, kEventRawKeyDown },
//		{ kEventClassKeyboard, kEventRawKeyRepeat },
	};
	
	if( debugwin_event_handler_upp == NULL )
		debugwin_event_handler_upp = NewEventHandlerUPP(debug_window_proc);
		
	InstallWindowEventHandler(info->wnd, debugwin_event_handler_upp, GetEventTypeCount(DebugWindowEventList), DebugWindowEventList, (void *)info, &(info->handler));

	// fill in some defaults
//	SystemParametersInfo(SPI_GETWORKAREA, 0, &work_bounds, 0);

	GDHandle theMainDevice;
	theMainDevice = GetMainDevice();
	work_bounds = (*theMainDevice)->gdRect;
	GetAvailableWindowPositioningBounds( theMainDevice, &work_bounds );
	
	info->minwidth = 200;
	info->minheight = 200;
	info->maxwidth = work_bounds.right - work_bounds.left;
	info->maxheight = work_bounds.bottom - work_bounds.top;

	// set the default handlers
//	info->handle_command = global_handle_command;
//	info->handle_key = global_handle_key;

	// hook us in
	info->next = window_list;
	window_list = info;
	return info;

cleanup:
	if (info->wnd)
		DisposeWindow( info->wnd );
	free(info);
	return NULL;
}

//============================================================
//	debug_window_proc
//============================================================

pascal OSStatus debug_window_proc(EventHandlerCallRef inCaller, EventRef inEvent, void* inRefcon)
{
	OSStatus result = eventNotHandledErr;
	struct debugwin_info *info = inRefcon;
	
	switch( GetEventClass(inEvent) )
	{
		case kEventClassWindow:
			switch( GetEventKind( inEvent ) )
			{
				case kEventWindowClose:
					if (main_console && main_console == info)
					{
						smart_show_all(FALSE);
						debug_cpu_go(~0);
						result = noErr;
					}
					else
					{
						result = noErr; //	DestroyWindow(wnd);
					}
					break;
				case kEventWindowResizeCompleted:
					if (info->recompute_children)
						(*info->recompute_children)(info);
					result = noErr;
					break;
			}
			break;
	}

	return result;
}

//============================================================
//	smart_set_control_bounds
//============================================================

static void smart_set_control_bounds(ControlRef wnd, WindowRef parent_window, HIObjectRef parent_view, Rect *bounds)
{
	SetControlBounds( wnd, bounds );
}

//============================================================
//	smart_show_window
//============================================================

static void smart_show_window(WindowRef wnd, Boolean show)
{
	Boolean visible = IsWindowVisible( wnd );
	if ((visible && !show) || (!visible && show))
	{
		if( show == true )
			ShowWindow(wnd);
		else
			HideWindow(wnd);
	}
}

//============================================================
//	smart_show_control
//============================================================

static void smart_show_control(ControlRef wnd, Boolean show)
{
	Boolean visible = IsControlVisible( wnd );
	if ((visible && !show) || (!visible && show))
	{
		if( show == true )
			ShowControl(wnd);
		else
			HideControl(wnd);
	}
}


//============================================================
//	smart_show_all
//============================================================

static void smart_show_all(Boolean show)
{
	struct debugwin_info *info;
	if (!show)
	{
		BringToFront( gDisplayState->window );
		mame_pause( 0 );
	}

	for (info = window_list; info; info = info->next)
	{
		smart_show_window(info->wnd, show);
	}
	
	if( show )
	{
		SelectWindow( main_console->wnd );
	}
}

static void SetRGBAColor( COLORREF color, float R, float G, float B, float A )
{
	color[0] = R;
	color[1] = G;
	color[2] = B;
	color[3] = A;
}


