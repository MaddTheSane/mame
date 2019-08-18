
#include <stdlib.h>
#ifdef __GNUC__
#include <unistd.h>
#endif
#include <sys/mman.h>
#include <sys/types.h>
#include <signal.h>
#include <time.h>
#ifdef __GNUC__
#include <sys/time.h>
#endif

// MAME headers
#include "osdcore.h"
#include "osdlib.h"

//============================================================
//  osd_getenv
//============================================================

const char *osd_getenv(const char *name)
{
    return getenv(name);
}

//============================================================
//  osd_setenv
//============================================================

int osd_setenv(const char *name, const char *value, int overwrite)
{
   return setenv(name, value, overwrite);
}

//============================================================
//  osd_process_kill
//============================================================

void osd_process_kill()
{
    kill(getpid(), SIGKILL);
}


//============================================================
//  osd_alloc_executable
//
//  allocates "size" bytes of executable memory.  this must take
//  things like NX support into account.
//============================================================

void *osd_alloc_executable(size_t size)
{
	return (void *)mmap(0, size, PROT_EXEC|PROT_READ|PROT_WRITE, MAP_ANON|MAP_SHARED, -1, 0);
}

//============================================================
//  osd_free_executable
//
//  frees memory allocated with osd_alloc_executable
//============================================================

void osd_free_executable(void *ptr, size_t size)
{
	munmap(ptr, size);
}

//============================================================
//  osd_break_into_debugger
//============================================================

void osd_break_into_debugger(const char *message)
{
	#ifdef MAME_DEBUG
	printf("MAME exception: %s\n", message);
	printf("Attempting to fall into debugger\n");
	kill(getpid(), SIGTRAP);
	#else
	printf("Ignoring MAME exception: %s\n", message);
	#endif
}

//============================================================
//  osd_get_clipboard_text
//============================================================

char *osd_get_clipboard_text()
{
	return nullptr;
}


//============================================================
//  osd_getpid
//============================================================

int osd_getpid()
{
	return getpid();
}

