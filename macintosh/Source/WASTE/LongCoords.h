/*
 *	LongCoords.h
 *
 *	C/C++ interface to the WASTE text engine:
 *	Long Coordinates
 *
 *	version 1.3
 *
 *	Copyright (c) 1993-1997 Marco Piovanelli
 *	All Rights Reserved
 * 
 */

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

#endif
#endif
