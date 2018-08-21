#pragma once

#ifndef MacAppleEvents_h
#define MacAppleEvents_h

//
// Constants
//
#define kMAMEEventClass FOUR_CHAR_CODE( 'MAME' )

#define kMAMEGetDriverList FOUR_CHAR_CODE( 'drvr' )

typedef struct
{
	char name[16];
	char source_file[16];
	char clone_of[16];
	char year[8];
	UInt32 flags; 

	char description[256];
	char manufacturer[256];
} MacMAMEGameDriver;

//
// Prototypes
//
void InitializeAppleEvents(void);

#endif