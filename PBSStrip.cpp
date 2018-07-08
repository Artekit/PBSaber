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

### PBSStrip.cpp

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

#include <Arduino.h>
#include <math.h>
#include "PBSStrip.h"
extern uint32_t getRandom(uint32_t min, uint32_t max);

uint32_t strip_update_time = 0;

PBSStrip::PBSStrip()
{
	update_rate_ms = 10;
	memset(&sections, 0, sizeof(sections));
}

PBSStrip::~PBSStrip()
{
}

bool PBSStrip::begin(uint32_t count, LedStripeType type)
{
	tickCounter.startTimeoutCounter(update_rate_ms);
	return LedStripDriver::begin(count, type);
}

bool PBSStrip::update(uint32_t index, bool async)
{
	// Do not allow updates while animating
	if (withAnimation())
		return false;

	return LedStripDriver::update(index, async);
}

void PBSStrip::set(uint32_t index, const COLOR& color)
{
	UNUSED(index);
	UNUSED(color);
}

void PBSStrip::set(uint32_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t w)
{
	UNUSED(index);
	UNUSED(r);
	UNUSED(g);
	UNUSED(b);
	UNUSED(w);
}

void PBSStrip::setRange(uint32_t start, uint32_t end, const COLOR& color)
{
	UNUSED(start);
	UNUSED(end);
	UNUSED(color);
}

void PBSStrip::setRange(uint32_t start, uint32_t end, uint8_t r, uint8_t g, uint8_t b, uint8_t w)
{
	UNUSED(start);
	UNUSED(end);
	UNUSED(r);
	UNUSED(g);
	UNUSED(b);
	UNUSED(w);
}

void PBSStrip::set(uint8_t section, uint32_t index, const COLOR& color)
{
	if (section > LED_STRIP_MAX_SECTIONS)
		return;

	set(section, index, color.r, color.g, color.b, color.w);
}

void PBSStrip::set(uint8_t section, uint32_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t w)
{
	if (section > LED_STRIP_MAX_SECTIONS)
		return;

	r *= sections[section].brightness;
	g *= sections[section].brightness;
	b *= sections[section].brightness;
	w *= sections[section].brightness;

	if (index == 0)
	{
		uint32_t start = sections[section].start;
		uint32_t end = sections[section].end;
		LedStripDriver::setRange(start, end, r, g, b, w);
	} else {
		LedStripDriver::set(sectionIndexToAbsoluteIndex(index, section), r, g, b, w);
	}
}

void PBSStrip::setRange(uint8_t section, uint32_t start, uint32_t end, const COLOR& color)
{
	setRange(section, start, end, color.r, color.g, color.b, color.w);
}

void PBSStrip::setRange(uint8_t section, uint32_t start, uint32_t end,
                        uint8_t r, uint8_t g, uint8_t b, uint8_t w)
{
	if (section > LED_STRIP_MAX_SECTIONS)
		return;

	r *= sections[section].brightness;
	g *= sections[section].brightness;
	b *= sections[section].brightness;
	w *= sections[section].brightness;

	start = sectionIndexToAbsoluteIndex(start, section);
	end = sectionIndexToAbsoluteIndex(end, section);

	LedStripDriver::setRange(start, end, r, g, b, w);
}

void PBSStrip::setWithMultp(uint8_t section, uint32_t start, uint32_t end,
                            COLOR& color, float multp)
{
	color *= multp;
	setRange(section, start, end, color);
}

// Layers
bool PBSStrip::setSection(uint8_t index, uint32_t start, uint32_t end)
{
	if (!initialized || start >= end || start < 1 || end > getLedCount() ||
		index > LED_STRIP_MAX_SECTIONS || sections[index].active)
		return false;

	sections[index].start = start;
	sections[index].end = end;
	sections[index].animating = false;
	sections[index].brightness = 1.0f;
	memset(&sections[index].anims, 0, sizeof(sectionAnimation));
	sections[index].active = true;
	return true;
}

void PBSStrip::resetSection(uint8_t section)
{
	if (!initialized || section > LED_STRIP_MAX_SECTIONS)
		return;

	sections[section].active = false;
}

void PBSStrip::resetAllSections()
{
	if (!initialized)
		return;

	for (uint8_t i = 0; i < LED_STRIP_MAX_SECTIONS; i++)
	{
		resetSection(i);
	}
}

// Animation control
bool PBSStrip::withAnimation()
{
	if (!initialized)
		return false;

	for (uint8_t i = 0; i < LED_STRIP_MAX_SECTIONS; i++)
	{
		if (withAnimation(i))
			return true;
	}

	return false;
}

bool PBSStrip::withAnimation(uint8_t section)
{
	if (!initialized || section > LED_STRIP_MAX_SECTIONS)
		return false;

	if (sections[section].active && sections[section].animating)
		return true;

	return false;
}

void PBSStrip::stopAnimation(uint8_t section)
{
	if (!initialized || section > LED_STRIP_MAX_SECTIONS)
		return;

	if (sections[section].active)
	{
		sections[section].animating = false;
		sections[section].anims.fade.active = false;
		sections[section].anims.shimmer_color.active = false;
		sections[section].anims.base_shimmer.active = false;
		sections[section].anims.shimmer.active = false;
		sections[section].anims.flash.active = false;
		sections[section].anims.sparks.active = false;
		sections[section].anims.scroll.active = false;
	}
}

void PBSStrip::stopAllAnimations()
{
	if (!initialized)
		return;

	animations_enabled = false;

	for (uint8_t i = 0; i < LED_STRIP_MAX_SECTIONS; i++)
	{
		stopAnimation(i);
	}
}

void PBSStrip::stopFlash(uint8_t section)
{
	if (!initialized || section > LED_STRIP_MAX_SECTIONS)
		return;

	sections[section].anims.flash.active = false;
}

void PBSStrip::stopShimmer(uint8_t section)
{
	if (!initialized || section > LED_STRIP_MAX_SECTIONS)
		return;

	sections[section].anims.shimmer.active = false;
}

void PBSStrip::playAnimations()
{
	animations_enabled = true;
}

void PBSStrip::pauseAnimations()
{
	animations_enabled = false;
}

COLOR PBSStrip::colorCorrect(const COLOR& color)
{
	return COLOR(cie_lut[color.r],
				 cie_lut[color.g],
				 cie_lut[color.b],
				 cie_lut[color.w]);
}

uint32_t PBSStrip::sectionIndexToAbsoluteIndex(uint32_t section_index, uint8_t section)
{
	return sectionIndexToAbsoluteIndex(section_index, &sections[section]);
}

uint32_t PBSStrip::sectionIndexToAbsoluteIndex(uint32_t section_index, ledStripSection* section)
{
	return section->start + section_index - 1;
}

void PBSStrip::shimmer(bladeEffect* shimmer, uint32_t duration, shimmerEffect* effect)
{
	uint32_t frequency = shimmer->freq;

	if (frequency > 25)
		frequency = 25;

	effect->active = false;
	effect->color = shimmer->base_color;
	effect->random = shimmer->randomize;
	effect->value = 1.0f;
	effect->blend = shimmer->blend;

	if (!effect->random)
	{
		uint32_t half_cycle = ((1000 / frequency) / 2) / update_rate_ms;
		float limit = (float) (100 - shimmer->depth) / 100;
		effect->step = (1.0f - limit) / half_cycle;
	} else {
		effect->value = 1.0f;
	}

	effect->lower_limit = 100 - shimmer->depth;

	if (duration)
	{
		effect->ticks = duration / update_rate_ms;
		if (!effect->ticks)
			effect->ticks = 1;
	} else {
		effect->ticks = 0;
	}
}

// Animations
void PBSStrip::baseShimmer(bladeEffect* shimmer_effect, uint8_t section)
{
	if (!shimmer_effect->freq || !shimmer_effect->depth || shimmer_effect->depth > 100 ||
		section > LED_STRIP_MAX_SECTIONS)
		return;

	shimmer(shimmer_effect, 0, &sections[section].anims.base_shimmer);
	sections[section].anims.base_shimmer.blend = 100;
	sections[section].anims.base_shimmer.active = true;
	sections[section].animating = true;
}

void PBSStrip::shimmer(bladeEffect* shimmer_effect, uint32_t duration, uint8_t section)
{
	if (!shimmer_effect->freq || !shimmer_effect->depth || shimmer_effect->depth > 100 ||
		section > LED_STRIP_MAX_SECTIONS)
		return;

	shimmer(shimmer_effect, duration, &sections[section].anims.shimmer);
	sections[section].anims.shimmer.active = true;
	sections[section].animating = true;
}

void PBSStrip::staticFlash(const COLOR& color, uint32_t duration, uint8_t blend, uint8_t section)
{
	if (!duration || section > LED_STRIP_MAX_SECTIONS)
		return;

	sections[section].anims.static_effect.blend = (float) blend / 100;
	sections[section].anims.static_effect.ticks = duration / update_rate_ms;
	sections[section].anims.static_effect.color = color;
	sections[section].anims.static_effect.active = true;
	sections[section].animating = true;
}


bool PBSStrip::spark(bladeEffect* effect, uint32_t duration, uint32_t loc,
                       uint32_t width, uint8_t energy, uint8_t section)
{
	uint8_t idx;

	if (!loc || section > LED_STRIP_MAX_SECTIONS || energy == 0 || !duration)
		return false;

	if (loc - 1 + sections[section].start > sections[section].end)
		return false;

	if (energy > 100)
		energy = 100;

	// Find a spark with duration 0
	for (idx = 0; idx < LED_STRIP_MAX_SPARKS; idx++)
	{
		if (sections[section].anims.sparks.spark[idx].ticks == 0)
			break;
	}

	if (idx == LED_STRIP_MAX_SPARKS)
		return false;

	sections[section].anims.sparks.spark[idx].color = colorCorrect(effect->base_color);
	sections[section].anims.sparks.spark[idx].location = loc;
	sections[section].anims.sparks.spark[idx].width = width;
	sections[section].anims.sparks.spark[idx].ticks = duration / update_rate_ms;
	sections[section].anims.sparks.spark[idx].value = (float) 100 / energy;
	sections[section].anims.sparks.spark[idx].decay =
				sections[section].anims.sparks.spark[idx].value	/ (duration / update_rate_ms);

	sections[section].anims.sparks.active = true;
	sections[section].animating = true;

	return true;
}

void PBSStrip::scroll(uint32_t start, uint32_t end, uint32_t duration, bool stop, uint8_t section)
{
	uint32_t count;

	if (!start || !end || start == end || !duration || section > LED_STRIP_MAX_SECTIONS)
		return;

	sections[section].anims.scroll.active = false;

	if (start > end)
	{
		if (end < sections[section].start)
			end = sections[section].start;

		if (start > sections[section].end)
			start = sections[section].end;

		count = start - end + 1;
		sections[section].anims.scroll.forward = false;

	} else {
		if (start < sections[section].start)
			start = sections[section].start;

		if (end > sections[section].end)
			end = sections[section].end;

		count = end - start + 1;
		sections[section].anims.scroll.forward = true;
	}

	sections[section].anims.scroll.start = sectionIndexToAbsoluteIndex(start, section);
	sections[section].anims.scroll.end = sectionIndexToAbsoluteIndex(end, section);
	sections[section].anims.scroll.rate = (float) count / (duration / update_rate_ms);
	sections[section].anims.scroll.position = sections[section].anims.scroll.start;
	sections[section].anims.scroll.stop = stop;

	sections[section].anims.scroll.active = true;
	sections[section].animating = true;
}

void PBSStrip::fade(bool in, uint32_t duration, uint8_t section)
{
	if (!duration || section > LED_STRIP_MAX_SECTIONS)
		return;

	sections[section].anims.fade.end = in ? getSectionBrightness(section) : 0;
	sections[section].anims.fade.rate = getBrightness() / (duration / update_rate_ms);
	sections[section].anims.fade.value = in ? 0 : getBrightness();
	sections[section].anims.fade.active = true;
	sections[section].animating = true;
}

void PBSStrip::fadeIn(uint32_t duration, uint8_t layer)
{
	fade(true, duration, layer);
}

void PBSStrip::fadeOut(uint32_t duration, uint8_t layer)
{
	fade(false, duration, layer);
}

void PBSStrip::flash(bladeEffect* flash, uint32_t duration, uint8_t section)
{
	if (!flash->freq || section > LED_STRIP_MAX_SECTIONS)
		return;

	uint32_t frequency = flash->freq;

	if (frequency > (1000 / update_rate_ms))
		frequency = 1000 / update_rate_ms;

	sections[section].anims.flash.flicker = (flash->type == effectTypeFlashFlicker);
	sections[section].anims.flash.color = flash->base_color;
	sections[section].anims.flash.blend = (float) flash->blend / 100;
	sections[section].anims.flash.current_color = flash->base_color;
	sections[section].anims.flash.ticks = 1000 / (frequency * 2) / update_rate_ms;
	sections[section].anims.flash.toggle = true;

	if (!sections[section].anims.flash.ticks)
		sections[section].anims.flash.ticks = 1;

	if (duration && duration < sections[section].anims.flash.ticks * update_rate_ms * 2)
		duration = sections[section].anims.flash.ticks * update_rate_ms * 2;

	sections[section].anims.flash.tick_count = sections[section].anims.flash.ticks;
	sections[section].anims.flash.duration = duration;
	sections[section].anims.flash.active = true;
	sections[section].animating = true;
}

void PBSStrip::changeBaseShimmerColor(const COLOR& into, uint32_t duration, uint8_t section)
{
	if (!duration || section > LED_STRIP_MAX_SECTIONS)
		return;

	if (!sections[section].anims.base_shimmer.active)
		return;

	changeShimmerEffect* sc = &sections[section].anims.shimmer_color;

	sc->rgbw_value[0] = sections[section].anims.base_shimmer.color.r;
	sc->rgbw_value[1] = sections[section].anims.base_shimmer.color.g;
	sc->rgbw_value[2] = sections[section].anims.base_shimmer.color.b;
	sc->rgbw_value[3] = sections[section].anims.base_shimmer.color.w;

	sc->ticks = duration / update_rate_ms;

	sc->rgbw_rate[0] = (into.r - sc->rgbw_value[0]) / sc->ticks;
	sc->rgbw_rate[1] = (into.g - sc->rgbw_value[1]) / sc->ticks;
	sc->rgbw_rate[2] = (into.b - sc->rgbw_value[2]) / sc->ticks;
	sc->rgbw_rate[3] = (into.w - sc->rgbw_value[3]) / sc->ticks;

	sc->end = into;
	sc->active = true;

	sections[section].animating = true;
}

void PBSStrip::poll()
{
	bool animating = false;

	// Limit updates/second
	if (!tickCounter.timeout())
		return;

	if (busy())
		return;

	tickCounter.startTimeoutCounter(update_rate_ms);

	if (!animations_enabled)
		return;

	strip_update_time = micros();

	// Go through all sections
	for (int8_t i = 0; i < LED_STRIP_MAX_SECTIONS; i++)
	{
		if (sections[i].active && sections[i].animating)
		{
			// Start with brightness fade
			if (sections[i].anims.fade.active)
				animating |= pollFade(i);

			// Change shimmer color
			if (sections[i].anims.shimmer_color.active)
				animating |= pollChangeShimmerColor(i);

			// Base shimmer effect
			if (sections[i].anims.base_shimmer.active)
				animating |= pollShimmer(&sections[i].anims.base_shimmer, i);

			if (sections[i].anims.shimmer.active)
				animating |= pollShimmer(&sections[i].anims.shimmer, i);

			// Flash effect
			if (sections[i].anims.flash.active)
				animating |= pollFlash(i);

			// Static effect
			if (sections[i].anims.static_effect.active)
				animating |= pollStatic(i);

			// Sparks
			if (sections[i].anims.sparks.active)
				animating |= pollSpark(i);

			// Scroll
			if (sections[i].anims.scroll.active)
				animating |= pollScroll(i);

			if (!animating)
				sections[i].animating = false;
		}
	}

	updateInternal(0, NULL, true);
	strip_update_time = micros() - strip_update_time;
}

bool PBSStrip::pollShimmer(shimmerEffect* effect, uint8_t index)
{
	ledStripSection* section = &sections[index];
	COLOR color;

	if (!effect->random)
	{
		float limit = (float) effect->lower_limit / 100;
		effect->value -= effect->step;
		if (effect->value >= 1.0f || effect->value <= limit)
			effect->step *= -1;
	} else {
		effect->value = (float) getRandom(effect->lower_limit, 100) / 100;
	}

	color = effect->color;
	color *= effect->value;

	uint32_t end;
	if (section->anims.scroll.active)
		end = section->anims.scroll.position;
	else
		end = section->end;

	if (effect->blend == 100)
	{
		setRange(index, section->start, end, color);
	} else {
		// For now assume the background is 'shimmer' with a fixed color
		COLOR shimmer = led_data->get(1);
		float blend = (float) effect->blend / 100;
		shimmer.blend(color, blend);
		setRange(index, section->start, end, shimmer);
	}

	if (effect->ticks)
	{
		effect->ticks--;
		if (!effect->ticks)
		{
			effect->active = false;
		}
	}

	return true;
}

bool PBSStrip::pollSpark(uint8_t index)
{
	bool ret = false;
	bool active = false;
	ledStripSection* section = &sections[index];

	for (uint8_t idx = 0; idx < LED_STRIP_MAX_SPARKS; idx++)
	{
		sparkInfo* sp = &section->anims.sparks.spark[idx];
		if (sp->ticks)
		{
			active = true;
			uint32_t led_index = sectionIndexToAbsoluteIndex(sp->location, index);

			uint32_t decay1_len = LED_STRIP_SPARK_DECAY(sp->width);
			uint32_t main_len = sp->width;
			uint32_t decay2_len = decay1_len;

			uint32_t decay1_ends = led_index + decay1_len - 1;
			uint32_t main_ends = decay1_ends + main_len;
			uint32_t decay2_ends = main_ends + decay2_len;

			uint32_t total_len = decay2_ends;
			float mult = 1.0f / (float) (decay1_len * 2);

			for (uint32_t i = led_index; i <= total_len; i++)
			{
				if (i > section->end)
					break;

				float value;
				COLOR color = led_data->get(i);

				if (i <= decay1_ends)
				{
					// First (linear) decay area
					mult += 1.0f / (float) decay1_len;
					value = sp->value * mult;
					color.blend(sp->color, value);
				} else if (i <= main_ends)
				{
					// Effective area
					if (sp->value != 1)
						color.blend(sp->color, sp->value);
					else
						color = sp->color;
				} else {
					// Second (linear) decay area. Use the same value calculated for decay1.
					value = sp->value * mult;
					mult -= 1.0f / (float) decay1_len;
					color.blend(sp->color, value);
				}

				// Sparks write directly into the LED strip driver buffer
				led_data->set(i, color);
			}

			sp->value -= sp->decay;
			sp->ticks--;
			if (sp->ticks)
				ret = true;
		}
	}

	if (!active)
		section->anims.sparks.active = false;

	return ret;
}

bool PBSStrip::pollScroll(uint8_t index)
{
	ledStripSection* section = &sections[index];
	uint32_t end;
	bool ret = true;

	if (section->anims.scroll.forward)
	{
		// Scroll going out
		section->anims.scroll.position += section->anims.scroll.rate;
		end = section->anims.scroll.end;

		if (section->anims.scroll.position > end)
		{
			section->anims.scroll.position = end;
			section->anims.scroll.active = false;
			ret = false;
		}
	} else {
		// Scroll coming in
		section->anims.scroll.position -= section->anims.scroll.rate;
		end = section->anims.scroll.start;

		if (section->anims.scroll.position < section->anims.scroll.end)
		{
			section->anims.scroll.position = section->anims.scroll.end;
			section->anims.scroll.active = false;
			ret = false;
		}
	}

	COLOR color(0,0,0,0);
	setRange(index, section->anims.scroll.position, end, color);
	return ret;
}

bool PBSStrip::pollFade(uint8_t index)
{
	ledStripSection* section = &sections[index];
	bool ret = true;

	if (section->anims.fade.end == 0)
	{
		// Fade out
		section->anims.fade.value -= section->anims.fade.rate;
		if (section->anims.fade.value <= 0)
		{
			section->anims.fade.value = 0;
			section->anims.fade.active = false;
			ret = false;
		}
	} else {
		// Fade in
		section->anims.fade.value += section->anims.fade.rate;
		if (section->anims.fade.value >= section->anims.fade.end)
		{
			section->anims.fade.value = section->anims.fade.end;
			section->anims.fade.active = false;
			ret = false;
		}
	}

	// Modify section brightness
	setSectionBrightness(index, section->anims.fade.value);
	return ret;
}

bool PBSStrip::pollStatic(uint8_t index)
{
	ledStripSection* section = &sections[index];

	// For now assume the background is 'shimmer' with a fixed color
	if (section->anims.static_effect.blend != 1)
	{
		COLOR shimmer = led_data->get(1);
		shimmer.blend(section->anims.static_effect.color, section->anims.static_effect.blend);
		setRange(index, section->start, section->end, shimmer);
	} else {
		setRange(index, section->start, section->end, section->anims.static_effect.color);
	}

	if (section->anims.static_effect.ticks)
	{
		section->anims.static_effect.ticks--;
		if (section->anims.static_effect.ticks == 0)
			section->anims.static_effect.active = false;
	}

	return true;
}

bool PBSStrip::pollFlash(uint8_t index)
{
	ledStripSection* section = &sections[index];
	bool ret = true;
	COLOR color;

	section->anims.flash.tick_count++;
	if (section->anims.flash.tick_count >= section->anims.flash.ticks)
	{
		section->anims.flash.tick_count = 0;
		section->anims.flash.toggle = !section->anims.flash.toggle;

		if (section->anims.flash.toggle)
		{
			section->anims.flash.current_color = 0;
		} else {
			color = section->anims.flash.color;

			if (section->anims.flash.flicker)
			{
				float mult = (float) getRandom(50, 100) / 100;
				color *= mult;
			}

			section->anims.flash.current_color = color;
		}
	}

	if (section->anims.flash.current_color != 0)
	{
		color = section->anims.flash.current_color;
		// For now assume the background is 'shimmer' with a fixed color
		if (section->anims.flash.blend != 1)
		{
			COLOR shimmer = led_data->get(1);
			shimmer.blend(color, section->anims.flash.blend);
			setRange(index, section->start, section->end, shimmer);
		} else {
			setRange(index, section->start, section->end, color);
		}
	}

	if (section->anims.flash.duration)
	{
		if (section->anims.flash.duration > update_rate_ms)
			section->anims.flash.duration -= update_rate_ms;
		else
			section->anims.flash.duration = 0;

		if (!section->anims.flash.duration)
		{
			section->anims.flash.active = false;
			ret = false;
		}
	}

	return ret;
}

bool PBSStrip::pollChangeShimmerColor(uint8_t index)
{
	ledStripSection* section = &sections[index];
	changeShimmerEffect* sc = &section->anims.shimmer_color;

	sc->ticks--;
	if (!sc->ticks)
	{
		section->anims.base_shimmer.color = sc->end;
		section->anims.shimmer_color.active = false;
		return false;
	} else {
		sc->rgbw_value[0] += sc->rgbw_rate[0];
		sc->rgbw_value[1] += sc->rgbw_rate[1];
		sc->rgbw_value[2] += sc->rgbw_rate[2];
		sc->rgbw_value[3] += sc->rgbw_rate[3];
		section->anims.base_shimmer.color = RGBW((uint8_t) sc->rgbw_value[0],
											(uint8_t) sc->rgbw_value[1],
											(uint8_t) sc->rgbw_value[2],
											(uint8_t) sc->rgbw_value[3]);
	}
	return true;
}
