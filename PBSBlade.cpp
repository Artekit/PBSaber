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

### PBSBlade.cpp

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

#include "PBSBlade.h"

extern uint32_t getRandom(uint32_t min, uint32_t max);


LedStripBlade::LedStripBlade(LedStripeType type, uint32_t count) :
		_type(type),
		_count(count),
		led_strip(NULL)
{
}

bool LedStripBlade::initialize()
{
	led_strip = new PBSStrip();
	if (!led_strip)
		return false;

	if (!led_strip->begin(_count, _type))
	{
		delete led_strip;
		return false;
	}

	// Clear
	COLOR color(0,0,0,0);
	led_strip->setSection(0, 1, _count);
	led_strip->set(0, 0, color);
	led_strip->update(0, true);
	return true;
}

void LedStripBlade::onIgnition(ignitionMode mode, uint32_t duration)
{
	if (!duration)
	{
		debugMsg(DebugError, "Invalid ignition duration");
		return;
	}

	debugMsg(DebugInfo, "Doing ignition effect on LedStripBlade, duration: %i ms", duration);

	/* Reset all and create a new section */
	led_strip->stopAllAnimations();
	led_strip->resetAllSections();
	led_strip->setSection(0, 1, _count);

	switch (mode)
	{
		case ignitionFull:
			// Actually a very fast ramp
			duration = 50;
			/* no break */

		case ignitionRamp:
			// Start a fade-in effect
			led_strip->fadeIn(duration, 0);
			break;

		case ignitionScroll:
			// Start a fade-in effect
			led_strip->fadeIn(duration, 0);

			// Start a scrolling effect
			led_strip->scroll(1, _count, duration, 0);
			break;
	}

	// Do base shimmer
	doBaseShimmer();

	// Run animations
	led_strip->playAnimations();
	effectTimeCounter.startTimeoutCounter(duration);
	blade_state = bladeStateIgnition;
}

void LedStripBlade::onRetraction(retractionMode mode, uint32_t duration)
{
	if (!duration)
	{
		debugMsg(DebugError, "Invalid retraction duration");
		return;
	}

	debugMsg(DebugInfo, "Doing retraction effect on LedStrip, duration: %i ms", duration);

	switch (mode)
	{
		case retractionFull:
			// Actually a very fast ramp
			duration = 100;
			/* no break */

		case retractionRamp:
			// Simply fade-out
			led_strip->fadeOut(duration, 0);
			break;

		case retractionScroll:
			// Fade-out while doing a scroll effect
			led_strip->fadeOut(duration, 0);
			led_strip->scroll(_count, 1, duration, true, 0);
			break;
	}

	// We need to know when to turn the blade off
	effectTimeCounter.startTimeoutCounter(duration);
	blade_state = bladeStateRetraction;
}

void LedStripBlade::onClash(bladeEffect* clash, uint32_t duration)
{
	doEffect(clash, duration);
}

void LedStripBlade::onBlaster(bladeEffect* blaster, uint32_t duration)
{
	doEffect(blaster, duration);
}

void LedStripBlade::onStab(bladeEffect* stab, uint32_t duration)
{
	doEffect(stab, duration);
}

void LedStripBlade::startLockup(bladeEffect* lockup)
{
	doEffect(lockup, 0);
}

void LedStripBlade::stopLockup()
{
	// Lockup effect doesn't have a duration and, if flashing, it has to be stopped (flash or
	// shimmer with infinite duration)
	led_strip->stopFlash(0);
	led_strip->stopShimmer(0);
}

void LedStripBlade::setShimmerMode(bladeEffect* shimmer)
{
	COLOR old_color = default_shimmer.base_color;
	COLOR new_color = shimmer->base_color;

	default_shimmer = *shimmer;

	if (blade_state == bladeStateIdle)
	{
		// Check if colors have changed
		if (old_color != new_color)
		{
			led_strip->changeBaseShimmerColor(new_color, BLADE_SHIMMER_SWITCH_DURATION, 0);
			effectTimeCounter.startTimeoutCounter(BLADE_SHIMMER_SWITCH_DURATION);
			blade_state = bladeStateColorChange;
		} else {
			doBaseShimmer();
		}
	}
}

void LedStripBlade::update()
{
	if (blade_state == bladeStateOff)
		return;

	led_strip->poll();

	if (blade_state == bladeStateRetraction)
	{
		if (effectTimeCounter.timeout())
		{
			effectTimeCounter.stopCounter();
			off();
		}
		return;
	} else if (blade_state == bladeStateIgnition)
	{
		if (effectTimeCounter.timeout())
		{
			effectTimeCounter.stopCounter();
			blade_state = bladeStateIdle;
		}
		return;
	}

	// Check end of effect and go back to shimmering
	if (effectTimeCounter.active() && effectTimeCounter.timeout())
	{
		effectTimeCounter.stopCounter();
		doBaseShimmer();
	}
}

void LedStripBlade::asyncUpdate()
{
	led_strip->startAsync();
}

void LedStripBlade::syncUpdate()
{
	led_strip->stopAsync();
}

void LedStripBlade::doBaseShimmer()
{
	led_strip->baseShimmer(&default_shimmer, 0);
	blade_state = bladeStateIdle;
}

bool LedStripBlade::doEffect(bladeEffect* effect, uint32_t duration)
{
	switch (effect->type)
	{
		case effectTypeFlash:
		case effectTypeFlashFlicker:
			led_strip->flash(effect, duration);
			break;

		case effectTypeShimmer:
			led_strip->shimmer(effect, duration, (uint8_t) 0);
			break;

		case effectTypeFlashSpark:
			led_strip->staticFlash(effect->base_color, 40, 100);
			// no break

		case effectTypeSpark:
		{
			uint32_t width;
			if (effect->randomize)
				effect->base_color = randomColor();

			if (effect->depth)
				width = effect->depth;
			else
				width = getRandom(10, 20);

			led_strip->spark(effect, duration, getRandom(0, _count - 20), width, 100, 0);
			break;
		}

		case effectTypeBigSpark:
		{
			uint32_t width;
			if (effect->randomize)
				effect->base_color = randomColor();

			led_strip->staticFlash(effect->base_color, 20, 100);

			if (effect->depth)
				width = effect->depth;
			else
				width = _count / 3;

			int32_t location = (_count / 2) - width;
			if (location < 1)
				location = 1;

			led_strip->spark(effect, duration, location, width, 100, 0);
			break;
		}

		case effectTypeTipSpark:
		{
			uint32_t width;
			if (effect->randomize)
				effect->base_color = randomColor();

			if (effect->depth)
				width = effect->depth;
			else
				width = getRandom(10, 20);

			uint32_t location = _count - width - LED_STRIP_SPARK_DECAY(width);
			led_strip->spark(effect, duration, location, width, 100, 0);
			break;
		}

		case effectTypeInvalid:
			return false;

		default:
			debugPrint(DebugWarning, "Invalid blade effect mode for LED strip");
			debugPrint(DebugWarning, "Doing static flash with random color");
			effect->base_color = randomColor();
			duration = 50;
			effect->blend = 100;
			/* no break */

		case effectTypeStatic:
			led_strip->staticFlash(effect->base_color, duration, effect->blend);
			break;
	}

	return true;
}

void LedStripBlade::off()
{
	COLOR color(0,0,0,0);
	led_strip->set(0, 0, color);
	led_strip->update(0, true);
	blade_state = bladeStateOff;
}

RgbHbLedBlade::RgbHbLedBlade(uint16_t current1, uint16_t current2, uint16_t current3)
{
	leds[0] = leds[1] = leds[2] = NULL;
	currents[0] = current1;
	currents[1] = current2;
	currents[2] = current3;

	change_shimmer.active = false;
	base_shimmer.active = false;
	shimmer.active = false;
	flash.active = false;
	static_flash.active = false;
	fade.active = false;
}

bool RgbHbLedBlade::instanceLED(uint8_t num, uint16_t current)
{
	// num is zero-based
	if (leds[num])
		delete leds[num];

	leds[num] = new HBLED(num+1);
	if (!leds[num])
		return false;

	return leds[num]->begin(current);
}

void RgbHbLedBlade::deleteLED(uint8_t num)
{
	if (leds[num])
		delete leds[num];

	leds[num] = NULL;
}

bool RgbHbLedBlade::initialize()
{
	bool ret = true;

	if (!currents[0] && !currents[1] && !currents[2])
		return false;

	for (uint8_t i = 0; i < 3; i++)
	{
		if (currents[i])
			ret &= instanceLED(i, currents[i]);
	}

	if (!ret)
	{
		deleteLED(0);
		deleteLED(1);
		deleteLED(2);
	} else {
		// Start timer
		add();
	}

	return ret;
}

void RgbHbLedBlade::setMultiplier(float val)
{
	for (uint8_t i = 0; i < 3; i++)
	{
		if (leds[i])
			leds[i]->setMultiplier(val);
	}
}

float RgbHbLedBlade::getMultiplier()
{
	for (uint8_t i = 0; i < 3; i++)
	{
		if (leds[i])
			return leds[i]->getMultiplier();
	}

	return 0;
}

void RgbHbLedBlade::onIgnition(ignitionMode mode, uint32_t duration)
{
	if (!duration)
	{
		debugMsg(DebugError, "Invalid ignition duration");
		return;
	}

	// Ignition on High Brightness LED
	debugMsg(DebugInfo, "Doing ignition effect on HBLED, duration: %i ms", duration);

	switch (mode)
	{
		default:
			// Not supported on HBLED
			debugMsg(DebugInfo, "Ignition effect type not supported on HBLED");
			debugMsg(DebugInfo, "Doing ramp with duration 50ms");
			// no break

		case ignitionFull:
			// Actually a very fast ramp
			duration = 50;
			// no break

		case ignitionRamp:
			setMultiplier(0);

			// Start shimmering
			doBaseShimmer();

			// Set fade parameters
			fade.end = 1.0f;
			fade.rate = 1.0f / (float) duration;
			fade.value = 0;
			fade.active = true;
			break;
	}

	// We need to know when this effect ends
	effectTimeCounter.startTimeoutCounter(duration);
	blade_state = bladeStateIgnition;
}

void RgbHbLedBlade::onRetraction(retractionMode mode, uint32_t duration)
{
	if (!duration)
	{
		debugMsg(DebugError, "Invalid retraction duration");
		return;
	}

	// Ignition on High Brightness LED
	debugMsg(DebugInfo, "Doing retraction effect on HBLED, duration: %i ms", duration);

	// Check ignition mode
	switch (mode)
	{
		default:
			// Not supported on HBLED
			debugMsg(DebugInfo, "Retraction effect type not supported on HBLED");
			debugMsg(DebugInfo, "Doing ramp with duration 50ms");
			// no break

		case retractionFull:
			// Actually a very fast ramp
			duration = 50;
			// no break

		case retractionRamp:
			// Set fade parameters
			fade.end = 0.0f;
			fade.value = getMultiplier();
			fade.rate = 1.0f / (float) duration;
			fade.active = true;
			break;
	}

	// We need to know when this effect ends
	effectTimeCounter.startTimeoutCounter(duration);
	blade_state = bladeStateRetraction;
}

void RgbHbLedBlade::onClash(bladeEffect* clash, uint32_t duration)
{
	doFlashEffect(clash, duration);
}

void RgbHbLedBlade::onBlaster(bladeEffect* blaster, uint32_t duration)
{
	doFlashEffect(blaster, duration);
}

void RgbHbLedBlade::onStab(bladeEffect* stab, uint32_t duration)
{
	doFlashEffect(stab, duration);
}

void RgbHbLedBlade::startLockup(bladeEffect* lockup)
{
	doFlashEffect(lockup, 0);
}

void RgbHbLedBlade::stopLockup()
{
	shimmer.active = false;
	flash.active = false;
}

void RgbHbLedBlade::poll()
{
	COLOR color;

	if (effectTimeCounter.active())
	{
		if (effectTimeCounter.timeout())
		{
			effectTimeCounter.stopCounter();

			if (blade_state == bladeStateRetraction)
			{
				off();
			} else if (blade_state == bladeStateIgnition)
			{
				blade_state = bladeStateIdle;
			} else if (blade_state == bladeStateColorChange)
			{
				blade_state = bladeStateIdle;
				doBaseShimmer();
			}
		}
	}

	if (blade_state != bladeStateOff)
	{
		if (change_shimmer.active)
			pollChangeShimmer();

		if (base_shimmer.active)
			color = pollBaseShimmer();

		if (shimmer.active)
			color = pollShimmer(color);

		if (flash.active)
			color = pollFlash(color);

		if (static_flash.active)
			color = pollStatic(color);

		if (fade.active)
			pollFade();

		if (leds[0])
			leds[0]->setValue(color.r);

		if (leds[1])
			leds[1]->setValue(color.g);

		if (leds[2])
			leds[2]->setValue(color.b);
	}
}

void RgbHbLedBlade::setShimmerMode(bladeEffect* shimmer)
{
	COLOR old_color = default_shimmer.base_color;
	COLOR new_color = shimmer->base_color;

	default_shimmer = *shimmer;

	if (blade_state == bladeStateIdle)
	{
		if (old_color == new_color)
		{
			doBaseShimmer();
			return;
		}

		uint32_t ticks = BLADE_SHIMMER_SWITCH_DURATION;
		change_shimmer.rgbw_value[0] = base_shimmer.color.r;
		change_shimmer.rgbw_value[1] = base_shimmer.color.g;
		change_shimmer.rgbw_value[2] = base_shimmer.color.b;
		change_shimmer.rgbw_value[3] = base_shimmer.color.w;
		change_shimmer.rgbw_rate[0] = (float) (new_color.r - base_shimmer.color.r) / ticks;
		change_shimmer.rgbw_rate[1] = (float) (new_color.g - base_shimmer.color.g) / ticks;
		change_shimmer.rgbw_rate[2] = (float) (new_color.b - base_shimmer.color.b) / ticks;
		change_shimmer.rgbw_rate[3] = (float) (new_color.w - base_shimmer.color.w) / ticks;
		change_shimmer.ticks = ticks;
		change_shimmer.end = new_color;
		change_shimmer.active = true;

		effectTimeCounter.startTimeoutCounter(BLADE_SHIMMER_SWITCH_DURATION);
		blade_state = bladeStateColorChange;
	}
}

void RgbHbLedBlade::doShimmer(bladeEffect* params, uint32_t duration, shimmerEffect* dst)
{
	uint32_t frequency = params->freq;

	if (!frequency || frequency > 25)
		frequency = 25;

	dst->active = false;
	dst->color = params->base_color;
	dst->random = params->randomize;
	dst->value = 1.0f;
	dst->blend = params->blend;

	if (!dst->random)
	{
		uint32_t half_cycle = (1000 / frequency) / 2;
		float limit = (float) (100 - params->depth) / 100;
		dst->step = (1.0f - limit) / half_cycle;
	} else {
		dst->value = 1.0f;
	}

	dst->lower_limit = 100 - params->depth;

	if (duration)
	{
		dst->ticks = duration;
		if (!dst->ticks)
			dst->ticks = 1;
	} else {
		dst->ticks = 0;
	}
}

void RgbHbLedBlade::doBaseShimmer()
{
	doShimmer(&default_shimmer, 0, &base_shimmer);
	base_shimmer.blend = 100;
	base_shimmer.active = true;
	blade_state = bladeStateIdle;
}

void RgbHbLedBlade::doFlashEffect(bladeEffect* effect, uint32_t duration)
{
	switch(effect->type)
	{
		default:
			effect->blend = 100;
			effect->base_color = randomColor();
			duration = 50;
			// no break

		case effectTypeStatic:
			static_flash.active = false;
			static_flash.blend = (float) effect->blend / 100;
			static_flash.ticks = duration;
			static_flash.color = effect->base_color;
			static_flash.active = true;
			break;

		case effectTypeShimmer:
			doShimmer(effect, duration, &shimmer);
			shimmer.active = true;
			break;

		case effectTypeFlashFlicker:
		case effectTypeFlash:
			uint32_t frequency = effect->freq;

			if (!frequency || frequency > 25)
				frequency = 25;

			flash.flicker = (effect->type == effectTypeFlashFlicker);
			flash.color = effect->base_color;
			flash.blend = (float) effect->blend / 100;
			flash.current_color = effect->base_color;
			flash.ticks = 1000 / (frequency * 2);
			flash.toggle = false;

			if (!flash.ticks)
				flash.ticks = 1;

			if (duration && duration < flash.ticks * 2)
				duration = flash.ticks * 2;

			flash.duration = duration;
			flash.active = true;
			break;
	}
}

void RgbHbLedBlade::off()
{
	base_shimmer.active = shimmer.active = flash.active = fade.active = false;

	for (uint8_t i = 0; i < 3; i++)
	{
		if (leds[i])
			leds[i]->setValue(0);
	}

	blade_state = bladeStateOff;
}

void RgbHbLedBlade::processShimmerEffect(shimmerEffect* effect)
{
	if (!effect->random)
	{
		float limit = (float) effect->lower_limit / 100;
		effect->value -= effect->step;
		if (effect->value >= 1.0f || effect->value <= limit)
			effect->step *= -1;
	} else {
		effect->value = (float) getRandom(effect->lower_limit, 100) / 100;
	}
}

COLOR RgbHbLedBlade::pollBaseShimmer()
{
	processShimmerEffect(&base_shimmer);
	COLOR color = base_shimmer.color * base_shimmer.value;
	return color;
}

COLOR& RgbHbLedBlade::pollShimmer(COLOR& in)
{
	processShimmerEffect(&shimmer);
	COLOR color = shimmer.color * shimmer.value;

	if (shimmer.blend != 100)
	{
		float blend = (float) shimmer.blend / 100;
		in.blend(color, blend);
	} else {
		in = color;
	}

	if (shimmer.ticks)
	{
		shimmer.ticks--;
		if (!shimmer.ticks)
			shimmer.active = false;
	}

	return in;
}

COLOR& RgbHbLedBlade::pollFlash(COLOR& in)
{
	flash.tick_count++;
	if (flash.tick_count >= flash.ticks)
	{
		flash.tick_count = 0;
		flash.toggle = !flash.toggle;

		if (flash.toggle)
		{
			flash.current_color = 0;
		} else {
			flash.current_color = flash.color;

			if (flash.flicker)
			{
				float mult = (float) getRandom(50, 100) / 100;
				flash.current_color *= mult;
			}
		}
	}

	if (flash.current_color != 0)
	{
		COLOR color = flash.current_color;

		if (flash.blend != 1)
			in.blend(color, flash.blend);
		else
			in = color;
	}

	if (flash.duration)
	{
		flash.duration--;
		if (!flash.duration)
			flash.active = false;
	}

	return in;
}

COLOR& RgbHbLedBlade::pollStatic(COLOR& in)
{
	if (static_flash.blend != 1)
		in.blend(static_flash.color, static_flash.blend);
	else
		in = static_flash.color;

	if (static_flash.ticks)
	{
		static_flash.ticks--;
		if (static_flash.ticks == 0)
			static_flash.active = false;
	}

	return in;
}

void RgbHbLedBlade::pollFade()
{
	if (fade.end == 0)
	{
		// Fade out
		fade.value -= fade.rate;
		if (fade.value <= 0)
		{
			fade.value = 0;
			fade.active = false;
		}
	} else {
		// Fade in
		fade.value += fade.rate;
		if (fade.value >= fade.end)
		{
			fade.value = fade.end;
			fade.active = false;
		}
	}

	setMultiplier(fade.value);
}

void RgbHbLedBlade::pollChangeShimmer()
{
	change_shimmer.ticks--;
	if (!change_shimmer.ticks)
	{
		base_shimmer.color = change_shimmer.end;
		change_shimmer.active = false;
	} else {
		change_shimmer.rgbw_value[0] += change_shimmer.rgbw_rate[0];
		change_shimmer.rgbw_value[1] += change_shimmer.rgbw_rate[1];
		change_shimmer.rgbw_value[2] += change_shimmer.rgbw_rate[2];
		change_shimmer.rgbw_value[3] += change_shimmer.rgbw_rate[3];
		base_shimmer.color = RGBW((uint8_t) change_shimmer.rgbw_value[0],
								  (uint8_t) change_shimmer.rgbw_value[1],
								  (uint8_t) change_shimmer.rgbw_value[2],
								  (uint8_t) change_shimmer.rgbw_value[3]);
	}
}
