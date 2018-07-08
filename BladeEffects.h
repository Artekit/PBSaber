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

### BladeEffects.h

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

#ifndef __BLADEEFFECT_H__
#define __BLADEEFFECT_H__

#include <Arduino.h>

// Common effects for both LEDs strip and HBLEDs

typedef struct
{
	bool active;
	COLOR end;
	float rgbw_rate[4];
	float rgbw_value[4];
	uint32_t ticks;
} changeShimmerEffect;

typedef struct
{
	bool active;
	COLOR color;
	uint32_t ticks;
	float blend;
} staticEffect;

typedef struct
{
	bool active;
	COLOR color;
	float value;
	uint8_t lower_limit;
	float step;
	bool random;
	uint8_t blend;
	uint32_t ticks;
} shimmerEffect;

typedef struct
{
	bool active;
	bool flicker;
	COLOR color;
	float blend;
	COLOR current_color;
	uint32_t ticks;
	uint32_t tick_count;
	uint32_t duration;
	bool toggle;
} flashEffect;

typedef struct
{
	bool active;
	float rate;
	float value;
	float end;
} fadeEffect;

#endif /* __BLADEEFFECT_H__ */
