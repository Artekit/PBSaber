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

### PBSDebug.cpp

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

#include "PBSDebug.h"
#include <stdarg.h>

#if PBS_DEBUG

static char debug_string[255];

void debugOut(char* str, bool crlf)
{
	if (crlf)
		PBS_DEBUG_SERIAL.println(str);
	else
		PBS_DEBUG_SERIAL.print(str);
}

void debugPrint(uint32_t level, const char* fmt, ...)
{
	uint32_t timestamp = GetTickCount();

	sprintf(debug_string, "[ %08lu ] ", timestamp);
	debugOut(debug_string, false);

	if (level & DebugError)
		sprintf(debug_string, "(%s) ", "ERROR");
	else if (level & DebugWarning)
		sprintf(debug_string, "(%s) ", "WARNING");
	else
		sprintf(debug_string, "(%s) ", "INFO");

	debugOut(debug_string, false);

	va_list argptr;
	va_start(argptr, fmt);
	vsprintf(debug_string, fmt, argptr);
	va_end(argptr);

	debugOut(debug_string, true);
}

void initDebug()
{
	PBS_DEBUG_SERIAL.begin(115200);
}

#endif // PBS_DEBUG
