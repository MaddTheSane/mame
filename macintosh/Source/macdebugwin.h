#ifndef MAC_DEBUG_H
#define MAC_DEBUG_H

#ifdef MAME_DEBUG

int debugwin_init_windows(void);
void osd_wait_for_debugger(void);

#endif /* MAME_DEBUG */

#define gDebuggerFocus false

#endif /* MAC_DEBUG_H */