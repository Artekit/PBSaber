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

### PBSStrip.h

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

#ifndef __PBSSTRIP_H__
#define __PBSSTRIP_H__

#include <stm32f4xx.h>
#include <ServiceTimer.h>
#include <bitmap.h>
#include <LedStripDriver.h>
#include "BladeEffects.h"
#include "PBSConfig.h"
#include "TimeCounter.h"

#define LED_STRIP_MAX_SECTIONS	3
#define LED_STRIP_MAX_SPARKS	10
#define LED_STRIP_SPARK_DECAY(x)	(x/2)

// These effects here are only for LED strips. The rest is included from BladeEffects.h
typedef struct
{
	COLOR color;
	uint32_t location;
	uint32_t width;
	uint32_t ticks;
	float value;
	float decay;
} sparkInfo;

typedef struct
{
	bool active;
	sparkInfo spark[LED_STRIP_MAX_SPARKS];
} sparkEffect;

typedef struct
{
	bool active;
	uint32_t start;
	uint32_t end;
	float rate;
	float position;
	bool forward;
	bool stop;
} scrollEffect;

typedef struct
{
	shimmerEffect base_shimmer;
	shimmerEffect shimmer;
	flashEffect flash;
	fadeEffect fade;
	scrollEffect scroll;
	changeShimmerEffect shimmer_color;
	sparkEffect sparks;
	staticEffect static_effect;
} sectionAnimation;

typedef struct
{
	bool active;
	bool animating;
	uint32_t start;
	uint32_t end;
	float brightness;
	sectionAnimation anims;
} ledStripSection;

class PBSStrip : public LedStripDriver, STObject
{
public:
	PBSStrip();
	virtual ~PBSStrip();

	// Overrides
	bool begin(uint32_t count, LedStripeType type = WS2812B);
	bool update(uint32_t index = 0, bool async = false);
	void set(uint32_t index, const COLOR& color);
	void set(uint32_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t w = 0);
	void setRange(uint32_t start, uint32_t end, const COLOR& color);
	void setRange(uint32_t start, uint32_t end, uint8_t r, uint8_t g, uint8_t b, uint8_t w = 0);

	// Set and Set Range in section
	void set(uint8_t section, uint32_t index, const COLOR& color);
	void set(uint8_t section, uint32_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t w = 0);
	void setRange(uint8_t section, uint32_t start, uint32_t end, const COLOR& color);
	void setRange(uint8_t section, uint32_t start, uint32_t end,
	              uint8_t r, uint8_t g, uint8_t b, uint8_t w = 0);

	// Section
	bool setSection(uint8_t index, uint32_t start, uint32_t end);
	void resetSection(uint8_t section);
	void resetAllSections();

	// Animations
	void baseShimmer(bladeEffect* shimmer_effect, uint8_t section = 0);
	void shimmer(bladeEffect* shimmer_effect, uint32_t duration, uint8_t section = 0);
	void staticFlash(const COLOR& color, uint32_t duration, uint8_t blend, uint8_t section = 0);
	void flash(bladeEffect* flash, uint32_t duration, uint8_t section = 0);
	void fadeIn(uint32_t duration, uint8_t section = 0);
	void fadeOut(uint32_t duration, uint8_t section = 0);
	void scroll(uint32_t start, uint32_t end, uint32_t duration, bool stop, uint8_t section = 0);
	bool spark(bladeEffect* effect, uint32_t duration, uint32_t loc,
	             uint32_t width, uint8_t energy, uint8_t section = 0);
	void changeBaseShimmerColor(const COLOR& into, uint32_t duration, uint8_t section);

	// Animation control
	bool withAnimation();
	bool withAnimation(uint8_t section);
	void playAnimations();
	void pauseAnimations();
	void stopAnimation(uint8_t section);
	void stopAllAnimations();
	void stopFlash(uint8_t section);
	void stopShimmer(uint8_t section);
	void poll();

	// Sync/Async
	void startAsync() { add(); }
	void stopAsync() { remove(); }

	inline LedStripData* ledData() { return led_data; }
	inline uint32_t getUpdateRate() { return update_rate_ms; }

private:
	void shimmer(bladeEffect* shimmer, uint32_t duration, shimmerEffect* effect);
	void fade(bool in, uint32_t duration, uint8_t section);
	void setWithMultp(uint8_t section, uint32_t start, uint32_t end, COLOR& color, float multp);
	COLOR colorCorrect(const COLOR& color);
	uint32_t sectionIndexToAbsoluteIndex(uint32_t section_index, uint8_t section);
	uint32_t sectionIndexToAbsoluteIndex(uint32_t section_index, ledStripSection* section);
	float getSectionBrightness(uint8_t section)
	{
		return sections[section].brightness;
	}

	void setSectionBrightness(uint8_t section, float value)
	{
		sections[section].brightness = value;
	}

	bool pollShimmer(shimmerEffect* effect, uint8_t index);
	bool pollSpark(uint8_t index);
	bool pollScroll(uint8_t index);
	bool pollFade(uint8_t index);
	bool pollFlash(uint8_t index);
	bool pollStatic(uint8_t index);
	bool pollChangeShimmerColor(uint8_t index);

	// LedStripData* anim_buffer;
	uint32_t update_rate_ms;
	TimeCounter tickCounter;
	ledStripSection sections[LED_STRIP_MAX_SECTIONS];
	volatile bool animations_enabled;
};

#endif /* __PBSSTRIP_H__ */
