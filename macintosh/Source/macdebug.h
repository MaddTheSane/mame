#ifndef MAC_DEBUG_H
#define MAC_DEBUG_H

#ifdef MAME_DEBUG

extern DisplayParameters		gDebugDisplay;	// current state of the debugger display
extern Boolean					gDebuggerFocus;	// true if debugger is active
extern GWorldPtr				gDebugGWorld;

void UpdateDebugger(void);
int DrawDebugger (mame_bitmap *debug_bitmap);
Boolean CreateDebugPalette16(void);
Boolean CreateDebugPalette32(void);

void update_debug_palette(const rgb_t *palette);

void DisposeDebugScreen (void);

void mac_set_debugger_focus (int hasfocus);


#else

#define gDebuggerFocus FALSE	// this avoids tons of conditional compile...

#endif

#endif /* MAC_DEBUG_H */