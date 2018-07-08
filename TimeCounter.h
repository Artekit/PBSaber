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

### TimeCounter.h

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

#ifndef __TIMECOUNTER_H__
#define __TIMECOUNTER_H__

#include <Arduino.h>

class TimeCounter
{
public:
	TimeCounter() : counting(false), initial_ticks(0), mark(0) {}

	inline void startCounter()
	{
		initial_ticks = GetTickCount();
		counting = true;
	}

	inline void stopCounter()
	{
		counting = false;
	}

	inline bool active()
	{
		return counting;
	}

	inline void startTimeoutCounter(uint32_t timeout)
	{
		mark = timeout;
		startCounter();
	}

	inline uint32_t elapsed()
	{
		return (GetTickCount() - initial_ticks);
	}

	inline bool timeout()
	{
		return elapsed() >= mark;
	}

private:
	bool counting;
	uint32_t initial_ticks;
	uint32_t mark;
};

#endif /* __TIMECOUNTER_H__ */
