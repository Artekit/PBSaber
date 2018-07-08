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

### PBSBlade.h

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

#ifndef __PBSBLADE_H__
#define __PBSBLADE_H__

#include <Arduino.h>
#include "PBSConfig.h"
#include "PBSDebug.h"
#include "PBSStrip.h"
#include "TimeCounter.h"

#define BLADE_SHIMMER_SWITCH_DURATION 500

extern uint32_t getRandom(uint32_t min, uint32_t max);

typedef enum
{
	bladeStateOff,
	bladeStateIgnition,
	bladeStateIdle,
	bladeStateRetraction,
	bladeStateColorChange
} bladeState;

class PBSBladeBase
{
public:
	PBSBladeBase() : blade_state(bladeStateOff) {}
	virtual ~PBSBladeBase() {}
	virtual bool initialize() = 0;
	virtual void onIgnition(ignitionMode mode, uint32_t duration) = 0;
	virtual void onRetraction(retractionMode mode, uint32_t duration) = 0;
	virtual void onClash(bladeEffect* clash, uint32_t duration) = 0;
	virtual void onBlaster(bladeEffect* blaster, uint32_t duration) = 0;
	virtual void onStab(bladeEffect* blaster, uint32_t duration) = 0;
	virtual void startLockup(bladeEffect* lockup) = 0;
	virtual void stopLockup() = 0;
	virtual void update() = 0;
	virtual void asyncUpdate() = 0;
	virtual void syncUpdate() = 0;
	virtual void setShimmerMode(bladeEffect* shimmer) = 0;
	virtual void doBaseShimmer() = 0;
	virtual void off() = 0;

	bladeState getState() { return blade_state; }

protected:
	bladeState blade_state;
};

class LedStripBlade : public PBSBladeBase, public STObject
{
public:
	LedStripBlade(LedStripeType type, uint32_t count);
	bool initialize();
	void onIgnition(ignitionMode mode, uint32_t duration);
	void onRetraction(retractionMode mode, uint32_t duration);
	void onClash(bladeEffect* clash, uint32_t duration);
	void onBlaster(bladeEffect* blaster, uint32_t duration);
	void onStab(bladeEffect* stab, uint32_t duration);
	void startLockup(bladeEffect* lockup);
	void stopLockup();
	void update();
	void asyncUpdate();
	void syncUpdate();
	void setShimmerMode(bladeEffect* shimmer);
	void doBaseShimmer();
	void off();

private:

	void poll() { update(); }
	bool doEffect(bladeEffect* effect, uint32_t duration);

	LedStripeType _type;
	uint32_t _count;
	PBSStrip* led_strip;
	bladeEffect default_shimmer;
	TimeCounter effectTimeCounter;
};

class RgbHbLedBlade : public PBSBladeBase, STObject
{
public:
	RgbHbLedBlade(uint16_t current1, uint16_t current2, uint16_t current3);
	bool initialize();
	void onIgnition(ignitionMode mode, uint32_t duration);
	void onRetraction(retractionMode mode, uint32_t duration);
	void onClash(bladeEffect* clash, uint32_t duration);
	void onBlaster(bladeEffect* blaster, uint32_t duration);
	void onStab(bladeEffect* stab, uint32_t duration);
	void startLockup(bladeEffect* lockup);
	void stopLockup();
	void update() {}
	void asyncUpdate() {}
	void syncUpdate() {}
	void setShimmerMode(bladeEffect* shimmer);
	void doBaseShimmer();
	void off();

private:

	void poll();
	void doShimmer(bladeEffect* params, uint32_t duration, shimmerEffect* dst);
	void doFlashEffect(bladeEffect* effect, uint32_t duration);
	bool instanceLED(uint8_t num, uint16_t current);
	void deleteLED(uint8_t num);
	void setMultiplier(float val);
	float getMultiplier();
	void processShimmerEffect(shimmerEffect* effect);

	COLOR pollBaseShimmer();
	COLOR& pollShimmer(COLOR& in);
	COLOR& pollFlash(COLOR& in);
	COLOR& pollStatic(COLOR& in);
	void pollChangeShimmer();
	void pollFade();

	HBLED* leds[3];

	uint16_t currents[3];
	TimeCounter effectTimeCounter;

	bladeEffect default_shimmer;
	shimmerEffect base_shimmer;
	shimmerEffect shimmer;
	changeShimmerEffect change_shimmer;
	flashEffect flash;
	fadeEffect fade;
	staticEffect static_flash;
};

#endif /* __PBSBLADE_H__ */
