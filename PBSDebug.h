/***************************************************************************
 * PBSaber
 * https://www.artekit.eu/doc/guides/propboard-pbsaber
 *
 * for Artekit PropBoard
 * https://www.artekit.eu/products/devboards/propboard
 *
 * Written by Ivan Meleca
 * Copyright (c) 2018 Artekit Labs
 * https://www.artekit.eu

### PBSDebug.h

#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 3 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.

***************************************************************************/

#ifndef __PBSDEBUG_H__
#define __PBSDEBUG_H__

#include <Arduino.h>

/* Debug macros */
#ifndef PBS_DEBUG
#define PBS_DEBUG 1
#endif

#if PBS_DEBUG

#define DEBUG_CONTEXT(x)	x

#ifndef PBS_DEBUG_SERIAL
#define PBS_DEBUG_SERIAL	Serial
#endif

#ifndef PBS_DEBUG_LEVEL_MASK
#define PBS_DEBUG_LEVEL_MASK	DebugInfoAll
#endif

void initDebug();
void debugPrint(uint32_t level, const char* fmt, ...);
void debugOut(char* str, bool crlf);

#define debugMsg(level, fmt, ...)						\
do {													\
	if (level && (level & PBS_DEBUG_LEVEL_MASK))	\
	{													\
		debugPrint(level, fmt, ##__VA_ARGS__);			\
	}													\
} while(0)

#else
#define DEBUG_CONTEXT(x)
#define initDebug()
#define debugPrint(x, y, ...) (void)(0)
#define debugOut(x, y) (void)(0)
#define debugMsg(level, fmt, ...) (void)(0)
#endif /* PBS_DEBUG */

#define DebugError		0x01
#define DebugWarning	0x02
#define DebugInfo		0x04
#define DebugInfoAll	(DebugError | DebugWarning | DebugInfo)

#endif /* __PBSDEBUG_H__ */
