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

### PBSConfig.cpp

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

#include "PBSConfig.h"

PBSConfig::PBSConfig()
{
	hardware_offset = settings_offset = first_profile_offset = first_font_offset = 0;
	profile_count = 0;
}

bool PBSConfig::readSettings()
{
	debugMsg(DebugInfo, "Reading general settings");

	if (!config_file.setFileRWPointer(settings_offset))
		return false;

	if (!config_file.startSectionScan("settings"))
		return false;

	memset(&settings, 0, sizeof(settings));

	char* key_name;
	uint32_t key_len;
	uint32_t token;

	while (config_file.getNextKey(&token, &key_name, &key_len))
	{
		// Profile count
		if (strncasecmp("profile_count", key_name, key_len) == 0)
		{
			config_file.readValue(token, &settings.profile_count);
			continue;
		}

		// Initial profile
		if (strncasecmp("initial_profile", key_name, key_len) == 0)
		{
			config_file.readValue(token, &settings.initial_profile);
			continue;
		}

		// Update initial profile
		if (strncasecmp("update_initial_profile", key_name, key_len) == 0)
		{
			config_file.readValue(token, &settings.update_initial_profile);
			continue;
		}

		// Low-power timeout
		if (strncasecmp("low_power", key_name, key_len) == 0)
		{
			config_file.readValue(token, &settings.low_power);
			continue;
		}

		// Audio volume
		if (strncasecmp("master_volume", key_name, key_len) == 0)
		{
			config_file.readValue(token, &settings.master_volume);
			continue;
		}

		// Swing sensitivity
		if (strncasecmp("swing_sensitivity", key_name, key_len) == 0)
		{
			config_file.readValue(token, &settings.swing_sensitivity);
			continue;
		}

		// Swing limiter in milliseconds
		if (strncasecmp("swing_limiter", key_name, key_len) == 0)
		{
			config_file.readValue(token, &settings.swing_limiter);
			continue;
		}

		// Clash limiter in milliseconds
		if (strncasecmp("clash_limiter", key_name, key_len) == 0)
		{
			config_file.readValue(token, &settings.clash_limiter);
			continue;
		}

		// Spin limiter in milliseconds
		if (strncasecmp("spin_limiter", key_name, key_len) == 0)
		{
			config_file.readValue(token, &settings.spin_limiter);
			continue;
		}

		// Clash sensitivity
		if (strncasecmp("clash_sensitivity", key_name, key_len) == 0)
		{
			config_file.readValue(token, &settings.clash_sensitivity);
			continue;
		}

		// Button debounce counter
		if (strncasecmp("button_debounce", key_name, key_len) == 0)
		{
			config_file.readValue(token, &settings.button_debounce);
			continue;
		}

		//  On/Off button press debounce time to turn the thing off
		if (strncasecmp("off_button_time", key_name, key_len) == 0)
		{
			config_file.readValue(token, &settings.off_time);
			continue;
		}

		//  FX button press time for lock-up
		if (strncasecmp("lock_button_time", key_name, key_len) == 0)
		{
			config_file.readValue(token, &settings.lock_time);
			continue;
		}

		// Utility sounds folder
		if (strncasecmp("sound_utils", key_name, key_len) == 0)
		{
			uint32_t len = sizeof(settings.sound_utils);
			config_file.readValue(token, settings.sound_utils, &len);
			continue;
		}

		//  Dump profile info to debug port
		if (strncasecmp("dump_profile_info", key_name, key_len) == 0)
		{
			config_file.readValue(token, &settings.dump_profile_info);
			continue;
		}

		//  Dump profile info to debug port
		if (strncasecmp("dump_font_info", key_name, key_len) == 0)
		{
			config_file.readValue(token, &settings.dump_font_info);
			continue;
		}
	}

	// Check
	if (!settings.initial_profile)
	{
		debugMsg(DebugWarning, "initial_profile value = 0. Using profile1 as initial profile");
		settings.initial_profile = 1;
	}

	if (!settings.profile_count)
	{
		debugMsg(DebugWarning, "profile_count value = 0. Using autodetection");
		settings.profile_count = profile_count;
	}

	if (settings.master_volume < -100 || settings.master_volume > 12)
	{
		// TODO check configuration
		debugMsg(DebugWarning, "volume value: %i. Setting default to 0dB", settings.master_volume);
		settings.master_volume = 0;
	}

	if (settings.low_power < 1)
		debugMsg(DebugWarning, "low_power value: %i", settings.low_power);

	if (settings.swing_limiter == 0)
	{
		debugMsg(DebugWarning, "swing_limiter value 0. Defaulting to 250ms");
		settings.swing_limiter = 250;
	}

	if (settings.clash_limiter == 0)
	{
		debugMsg(DebugWarning, "clash_limiter value 0. Defaulting to 250ms");
		settings.clash_limiter = 250;
	}

	if (settings.spin_limiter == 0)
	{
		debugMsg(DebugWarning, "spin_limiter value 0. Defaulting to 250ms");
		settings.spin_limiter = 250;
	}

	if (!settings.clash_sensitivity)
	{
		debugMsg(DebugWarning, "clash_sensitivity value %i. Defaulting to 50",
								settings.clash_sensitivity);
		settings.clash_sensitivity = 50;
	}

	if (!settings.button_debounce)
	{
		debugMsg(DebugWarning, "button_debounce value %i. Defaulting to 25",
								settings.button_debounce);
		settings.button_debounce = 25;
	}

	if (settings.off_time == 0)
	{
		debugMsg(DebugWarning, "off_button_time value %i. Defaulting to 25",
								settings.off_time);
		settings.off_time = 25;
	}

	if (settings.lock_time < 50)
	{
		debugMsg(DebugWarning, "lock_button_time value %i. Defaulting to 50",
								settings.lock_time);
		settings.lock_time = 50;
	}

	debugMsg(DebugInfo, "Default profile is %lu", settings.initial_profile);

	config_file.endSectionScan();
	return true;
}

bool PBSConfig::read()
{
	timeCounter.startCounter();

	if (!readHardwareConfiguration())
	{
		debugMsg(DebugError, "Failed while reading hardware configuration");
		return false;
	}

	if (!readSettings())
	{
		debugMsg(DebugError, "Failed while reading general settings");
		return false;
	}

	debugMsg(DebugInfo, "Configuration read in %lu ms", timeCounter.elapsed());
	return true;
}

bool PBSConfig::readAudioSettings()
{
	settings.audio_fs = 0;

	// Point to [settings] offset
	if (!config_file.setFileRWPointer(settings_offset))
		return false;

	if (!config_file.readValue("settings", "audio_fs", &settings.audio_fs))
		return false;

	if (settings.audio_fs != 22050 && settings.audio_fs != 32000 && settings.audio_fs != 44000 &&
		settings.audio_fs != 48000 && settings.audio_fs != 96000)
	{
		debugMsg(DebugWarning, "audio_fs value %i. Defaulting to 22050", settings.audio_fs);
		settings.audio_fs = 22050;
	}

	return true;
}

bool PBSConfig::readHardwareConfiguration()
{
	bool ret = true;
	uint32_t len;
	char* key_name;
	uint32_t key_len;
	uint32_t token;
	uint8_t output_count = 3;
	bool onoff_button = false;
	bool onoff_button_pol = false;

	debugMsg(DebugInfo, "Reading hardware configuration");

	// Point to [hardware] offset
	if (!config_file.setFileRWPointer(hardware_offset))
		return false;

	if (!config_file.startSectionScan("hardware"))
		return false;

	hw.has_button_fx = false;
	hw.strip_count = 0;

	while (config_file.getNextKey(&token, &key_name, &key_len))
	{
		// Blade type
		if (strncasecmp("blade_type", key_name, key_len) == 0)
		{
			uint32_t len = sizeof(tmp);
			if (!config_file.readValue(token, tmp, &len))
			{
				debugMsg(DebugError, "Error reading blade_type value");
				ret = false;
				break;
			}

			if (strcasecmp(tmp, "pixel") == 0)
				hw.blade_type = bladeStrip;
			else if (strcasecmp(tmp, "hbled") == 0)
				hw.blade_type = bladeHBLED;
			else
			{
				debugMsg(DebugError, "blade_type invalid value");
				ret = false;
				break;
			}
			continue;
		}

		// Blade type = bladeStrip
		if (hw.blade_type == bladeStrip)
		{
			if (strncasecmp("pixel_type", key_name, key_len) == 0)
			{
				// Strip type
				len = sizeof(tmp);
				if (!config_file.readValue(token, tmp, &len))
				{
					debugMsg(DebugError, "Error reading pixel_type value");
					ret = false;
					break;
				}

				if (len)
				{
					if (strcasecmp(tmp, "apa102") == 0)
						hw.strip_type = APA102;
					else if (strcasecmp(tmp, "ws2812") == 0)
						hw.strip_type = WS2812B;
					else if (strcasecmp(tmp, "sk6812rgbw") == 0)
						hw.strip_type = SK6812RGBW;
				}

				if (hw.strip_type == 0)
				{
					debugMsg(DebugError, "pixel_type invalid value: %s", tmp);
					ret = false;
					break;
				}
				continue;
			}

			// Strip LED count
			if (strncasecmp("pixel_count", key_name, key_len) == 0)
			{
				if (!config_file.readValue(token, &hw.strip_count))
				{
					debugMsg(DebugError, "Error reading pixel_count value");
					ret = false;
					break;
				}

				if (hw.strip_count == 0)
				{
					debugMsg(DebugError, "Invalid pixel_count value");
					ret = false;
					break;
				}
				continue;
			}
		} else if (hw.blade_type == bladeHBLED)
		{
			if (strncasecmp("hbled_current", key_name, key_len) == 0)
			{
				if (!config_file.readArray(token, hw.hbled_current, &output_count))
				{
					debugMsg(DebugError, "Error reading hbled_current value");
					ret = false;
					break;
				}

				if (output_count == 0)
				{
					debugMsg(DebugError, "hbled_current invalid count");
					ret = false;
					break;
				}
				continue;
			}

			if (strncasecmp("hbled_is_rgb", key_name, key_len) == 0)
			{
				if (!config_file.readValue(token, &hw.hbled_is_rgb))
				{
					debugMsg(DebugError, "Error reading hbled_is_rgb value");
					ret = false;
					break;
				}

				if (hw.hbled_is_rgb)
					debugMsg(DebugInfo, "HBLED is RGB");
				else
					debugMsg(DebugInfo, "HBLED is not RGB");

				continue;
			}
		}

		// Read buttons configuration
		if (strncasecmp("onoff_button_pad", key_name, key_len) == 0)
		{
			if (!config_file.readValue(token, &hw.button_onoff.pin))
			{
				// We need at least an ON/OFF button
				debugMsg(DebugError, "Error reading onoff_button_pad value");
				ret = false;
				break;
			}

			onoff_button = true;
			continue;
		}

		if (strncasecmp("onoff_button_pol", key_name, key_len) == 0)
		{
			if (!config_file.readValue(token, &hw.button_onoff.active_high))
			{
				// We need at least an ON/OFF button
				debugMsg(DebugError, "Error reading onoff_button_pol value");
				return false;
			}

			onoff_button_pol = true;
			continue;
		}

		if (strncasecmp("fx_button_pad", key_name, key_len) == 0)
		{
			if (!config_file.readValue(token, &hw.button_fx.pin))
			{
				debugMsg(DebugWarning, "Configuration for fx_button_pad not found");
			} else {
				hw.has_button_fx = true;
			}
			continue;
		}

		if (hw.has_button_fx)
		{
			if (strncasecmp("fx_button_pol", key_name, key_len) == 0)
			{
				if (!config_file.readValue(token, &hw.button_fx.active_high))
				{
					debugMsg(DebugWarning, "Configuration for fx_button_pol not found");
					hw.has_button_fx = false;
				}
				continue;
			}
		}
	}

	// Verify
	if (ret)
	{
		if (hw.blade_type == bladeHBLED)
		{
			if (output_count == 0)
			{
				debugMsg(DebugError, "blade_type invalid value");
				ret = false;
			}
		} else if (hw.blade_type == bladeStrip)
		{
			if (!hw.strip_type)
			{
				debugMsg(DebugError, "strip_type invalid value");
				ret = false;
			}

			if (!hw.strip_count)
			{
				debugMsg(DebugError, "strip_count invalid value");
				ret = false;
			}

		} else {
			debugMsg(DebugError, "blade_type invalid value");
			ret = false;
		}

		if (onoff_button)
		{
			debugMsg(DebugInfo, "On/Off button on pad %lu", hw.button_onoff.pin);
		} else {
			debugMsg(DebugError, "Cannot found pad configuration for the On/Off button");
			ret = false;
		}

		if (onoff_button_pol)
		{
			debugMsg(DebugInfo, "On/Off logic is active %s",
								hw.button_onoff.active_high ? "high" : "low");
		} else {
			debugMsg(DebugError, "Cannot found polarity for the On/Off button");
		}

		if (hw.has_button_fx)
		{
			debugMsg(DebugInfo, "FX button on pad %lu", hw.button_fx.pin);
			debugMsg(DebugInfo, "FX logic is active %s",
					hw.button_fx.active_high ? "high" : "low");
		} else {
			debugMsg(DebugInfo, "FX button is not present");
		}

		// Check if buttons are conflicting with LedStrip
		if (hw.blade_type == bladeStrip)
		{
			// Pin 10 == MOSI
			if ((hw.strip_type == WS2812B || hw.strip_type == SK6812RGBW) &&
				(hw.button_onoff.pin == 10 || hw.button_fx.pin == 10))
			{
				debugMsg(DebugError, "Buttons pads conflicts with LED strip DATA");
				ret = false;
			}

			// Pin 12 = CLK
			if (hw.strip_type == APA102 &&
				(hw.button_onoff.pin == 12 || hw.button_fx.pin == 12))
			{
				debugMsg(DebugError, "Buttons pads conflicts with LED strip CLK");
				ret = false;
			}
		}
	}

	config_file.endSectionScan();
	return ret;
}

bool PBSConfig::loadProfile(uint32_t id, saberProfile* dst)
{
	char profile[32];
	uint8_t array_count;
	uint32_t as_profile = 0;
	static uint8_t recursion = 0;

	if (!id)
		return false;

	sprintf(profile, "profile%lu", id);
	debugMsg(DebugInfo, "Reading profile %s", profile);

	if (!recursion)
	{
		memset(dst, 0, sizeof(saberProfile));
		timeCounter.startCounter();
	}

	// Put the configuration file cursor at the first found profile
	if (!config_file.setFileRWPointer(first_profile_offset))
		return false;

	// Check if the profile is like another one that already exists
	if (config_file.readValue(profile, "as_profile", &as_profile))
	{
		if (++recursion == 3)
		{
			recursion = 0;
			debugMsg(DebugError, "Cannot use as_profile recursively");
			return false;
		}

		debugMsg(DebugInfo, "Inheriting from profile%lu", as_profile);

		if (!loadProfile(as_profile, dst))
		{
			recursion = 0;
			debugMsg(DebugError, "Error inheriting profile%lu", as_profile);
			return false;
		}

		recursion--;
	}

	if (as_profile)
	{
		// We have to set pointer back again at the first found profile since we don't know
		// if the as_profile profile comes before or after the profile we want to read
		if (!config_file.setFileRWPointer(first_profile_offset))
			return false;
	}

	dst->id = id;

	if (!config_file.startSectionScan(profile))
	{
		debugMsg(DebugError, "Profile %s does not exist", profile);
		return false;
	}

	char* key_name;
	uint32_t key_len;
	uint32_t token;
	uint8_t color_array[4];
	array_count = 4;
	uint32_t strsize;

	while (config_file.getNextKey(&token, &key_name, &key_len))
	{
		// Font
		if (strncasecmp("font", key_name, key_len) == 0)
		{
			config_file.readValue(token, &dst->font_num);
			continue;
		}

		// Ignition
		if (strncasecmp("ignition_mode", key_name, key_len) == 0)
		{
			strsize = sizeof(tmp);
			config_file.readValue(token, tmp, &strsize);

			if (strsize)
			{
				if (strcasecmp(tmp, "ramp") == 0)
					dst->ignition_mode = ignitionRamp;
				else if (strcasecmp(tmp, "full") == 0)
					dst->ignition_mode = ignitionFull;
				else if (strcasecmp(tmp, "scroll") == 0)
					dst->ignition_mode = ignitionScroll;
			}
			continue;
		}

		if (dst->ignition_mode != ignitionFull)
		{
			if (strncasecmp("ignition_duration", key_name, key_len) == 0)
			{
				config_file.readValue(token, &dst->ignition_duration);
				continue;
			}
		}

		if (strncasecmp("ignition_on_stab", key_name, key_len) == 0)
		{
			config_file.readValue(token, &dst->ignition_on_stab);
			continue;
		}

		// Retraction
		if (strncasecmp("retraction_mode", key_name, key_len) == 0)
		{
			strsize = sizeof(tmp);
			config_file.readValue(token, tmp, &strsize);

			if (strsize)
			{
				if (strcasecmp(tmp, "ramp") == 0)
					dst->retraction_mode = retractionRamp;
				else if (strcasecmp(tmp, "full") == 0)
					dst->retraction_mode = retractionFull;
				else if (strcasecmp(tmp, "scroll") == 0)
					dst->retraction_mode = retractionScroll;
			}
			continue;
		}

		if (dst->retraction_mode != retractionFull)
		{
			if (strncasecmp("retraction_mode", key_name, key_len) == 0)
			{
				config_file.readValue(token, &dst->retraction_duration);
				continue;
			}
		}

		// Shimmer color
		if (strncasecmp("shimmer_color", key_name, key_len) == 0)
		{
			memset(color_array, 0, sizeof(color_array));
			config_file.readArray(token, color_array, &array_count);

			if (array_count)
				dst->shimmer.base_color = RGBW(color_array[0], color_array[1],
										  	   color_array[2], color_array[3]);

			continue;
		}

		// Shimmer depth
		if (strncasecmp("shimmer_depth", key_name, key_len) == 0)
		{
			config_file.readValue(token, &dst->shimmer.depth);
			continue;
		}

		// Shimmer mode
		if (strncasecmp("shimmer_mode", key_name, key_len) == 0)
		{
			strsize = sizeof(tmp);
			config_file.readValue(token, tmp, &strsize);

			if (strsize)
			{
				if (strcasecmp(tmp, "random") == 0)
					dst->shimmer.randomize = true;
				else
					dst->shimmer.randomize = false;
			}
			continue;
		}

		// Shimmer frequency
		if (strncasecmp("shimmer_freq", key_name, key_len) == 0)
		{
			config_file.readValue(token, &dst->shimmer.freq);
			continue;
		}
		char key[32];
		char key_type[8];
		bladeEffect *effect;

		if (strncasecmp(key_name, "clash", 5) == 0)
		{
			strcpy(key_type, "clash");
			effect = &dst->clash;
		} else if (strncasecmp(key_name, "blaster", 7) == 0)
		{
			strcpy(key_type, "blaster");
			effect = &dst->blaster;
		} else if (strncasecmp(key_name, "stab", 4) == 0)
		{
			strcpy(key_type, "stab");
			effect = &dst->stab;
		} else if (strncasecmp(key_name, "lockup", 6) == 0)
		{
			strcpy(key_type, "lockup");
			effect = &dst->lockup;
		} else continue;

		// Mode
		sprintf(key, "%s_mode", key_type);
		if (strncasecmp(key, key_name, key_len) == 0)
		{
			strsize = sizeof(tmp);
			config_file.readValue(token, tmp, &strsize);

			if (strsize)
			{
				if (strcasecmp(tmp, "static") == 0)
					effect->type = effectTypeStatic;
				else if (strcasecmp(tmp, "shimmer") == 0)
					effect->type = effectTypeShimmer;
				else if (strcasecmp(tmp, "flash") == 0)
					effect->type = effectTypeFlash;
				else if (strcasecmp(tmp, "spark") == 0)
					effect->type = effectTypeSpark;
				else if (strcasecmp(tmp, "flash_spark") == 0)
					effect->type = effectTypeFlashSpark;
				else if (strcasecmp(tmp, "big_spark") == 0)
					effect->type = effectTypeBigSpark;
				else if (strcasecmp(tmp, "tip_spark") == 0)
					effect->type = effectTypeTipSpark;
				else if (strcasecmp(tmp, "flash_flicker") == 0)
					effect->type = effectTypeFlashFlicker;
			}
			continue;
		}

		// Color
		sprintf(key, "%s_color", key_type);
		if (strncasecmp(key, key_name, key_len) == 0)
		{
			array_count = 4;
			memset(color_array, 0, sizeof(color_array));
			config_file.readArray(token, color_array, &array_count);

			if (array_count)
			{
				effect->base_color = RGBW(color_array[0], color_array[1],
										  color_array[2], color_array[3]);
			}
			continue;
		}

		// Random
		sprintf(key, "%s_random", key_type);
		if (strncasecmp(key, key_name, key_len) == 0)
		{
			config_file.readValue(token, &effect->randomize);
			continue;
		}

		// Blend
		sprintf(key, "%s_blend", key_type);
		if (strncasecmp(key, key_name, key_len) == 0)
		{
			config_file.readValue(token, &effect->blend);
			continue;
		}

		// Frequency
		sprintf(key, "%s_freq", key_type);
		if (strncasecmp(key, key_name, key_len) == 0)
		{
			config_file.readValue(token, &effect->freq);
			continue;
		}

		// Depth
		sprintf(key, "%s_depth", key_type);
		if (strncasecmp(key, key_name, key_len) == 0)
		{
			config_file.readValue(token, &effect->depth);
			continue;
		}

		sprintf(key, "%s_duration", key_type);
		if (strncasecmp(key, key_name, key_len) == 0)
		{
			config_file.readValue(token, &effect->duration);
			continue;
		}
	}

	config_file.endSectionScan();

	// If not doing a recursion do dump, check and font loading
	if (!recursion)
	{
		if (settings.dump_profile_info)
			dumpProfileInfo(dst);

		if (!checkProfile(dst))
			return false;

		if (loadFontInfo(dst->font_num, &dst->font))
		{
			if (settings.dump_font_info)
				dumpFontInfo(&dst->font);

			if (strlen(dst->font.title))
			{
				debugMsg(DebugInfo, "Using font %s", dst->font.title);
			} else {
				debugMsg(DebugInfo, "Using font %i", dst->font.id);
			}

			debugMsg(DebugInfo, "profile%lu successfully read in %lu ms", id, timeCounter.elapsed());
		} else return false;
	}

	return true;
}

bool PBSConfig::checkProfile(saberProfile* profile)
{
	bool ret = true;

	// Check
	if (profile->font_num == 0)
	{
		debugMsg(DebugError, "font value is missing");
		ret = false;
	}

	if (profile->ignition_mode == 0)
	{
		debugMsg(DebugError, "ignition_mode value is missing");
		ret = false;
	}

	if (profile->retraction_mode == 0)
	{
		debugMsg(DebugError, "retraction_mode value is missing");
		ret = false;
	}

	if (!profile->shimmer.base_color)
	{
		debugMsg(DebugError, "shimmer_color value is missing");
		ret = false;
	}

	if (!profile->shimmer.depth || profile->shimmer.depth > 100)
	{
		debugMsg(DebugError, "shimmer_depth value is invalid. Setting it to 20");
		profile->shimmer.depth = 20;
	}

	if (profile->shimmer.depth > 100)
		profile->shimmer.depth = 100;

	if (!profile->shimmer.freq || profile->shimmer.freq > 25)
	{
		debugMsg(DebugError, "shimmer_freq value is invalid. Setting it to 15");
		profile->shimmer.freq = 15;
	}

	if (profile->shimmer.freq > 50)
		profile->shimmer.freq = 50;

	if (!profile->blaster.type)
		debugMsg(DebugError, "blaster_mode value is invalid");

	if (!profile->stab.type)
		debugMsg(DebugError, "stab_mode value is invalid");

	if (!profile->lockup.type)
		debugMsg(DebugError, "lockup_mode value is invalid");

	if (profile->clash.freq > 25)
		profile->clash.freq = 25;

	if (profile->blaster.freq > 25)
		profile->blaster.freq = 25;

	if (profile->stab.freq > 25)
		profile->stab.freq = 25;

	if (profile->lockup.freq > 25)
		profile->lockup.freq = 25;

	if (!profile->clash.blend)
	{
		debugMsg(DebugError, "clash_blend value is invalid. Setting it to 100");
		profile->clash.blend = 100;
	}

	if (!profile->blaster.blend)
	{
		debugMsg(DebugError, "blaster_blend value is invalid. Setting it to 100");
		profile->blaster.blend = 100;
	}

	if (!profile->stab.blend)
	{
		debugMsg(DebugError, "stab_blend value is invalid. Setting it to 100");
		profile->stab.blend = 100;
	}

	if (!profile->lockup.blend)
	{
		debugMsg(DebugError, "lockup_blend value is invalid. Setting it to 100");
		profile->lockup.blend = 100;
	}

	if (profile->clash.type != effectTypeStatic &&
		profile->clash.type != effectTypeShimmer &&
		profile->clash.type != effectTypeFlash &&
		profile->clash.type != effectTypeFlashFlicker &&
		profile->clash.type != effectTypeBigSpark)
	{
		profile->clash.type = effectTypeInvalid;
		debugMsg(DebugError, "clash_mode value is invalid");
	}

	if (profile->blaster.type != effectTypeStatic &&
		profile->blaster.type != effectTypeShimmer &&
		profile->blaster.type != effectTypeFlash &&
		profile->blaster.type != effectTypeFlashFlicker &&
		profile->blaster.type != effectTypeSpark &&
		profile->blaster.type != effectTypeFlashSpark)
	{
		profile->blaster.type = effectTypeInvalid;
		debugMsg(DebugError, "blaster_mode value is invalid");
	}

	if (profile->stab.type != effectTypeStatic &&
		profile->stab.type != effectTypeShimmer &&
		profile->stab.type != effectTypeFlash &&
		profile->stab.type != effectTypeFlashFlicker &&
		profile->stab.type != effectTypeTipSpark)
	{
		profile->stab.type = effectTypeInvalid;
		debugMsg(DebugError, "stab_mode value is invalid");
	}

	if (profile->lockup.type != effectTypeShimmer &&
		profile->lockup.type != effectTypeFlash &&
		profile->lockup.type != effectTypeFlashFlicker)
	{
		profile->lockup.type = effectTypeInvalid;
		debugMsg(DebugError, "lockup_mode value is invalid");
	}

	return ret;
}

bool PBSConfig::folderExists(char* folder)
{
	DIR dir;
	FRESULT res;

	res = f_opendir(&dir, folder);
	if (res != FR_OK)
		return false;

	f_closedir(&dir);
	return true;
}

bool PBSConfig::loadFontInfo(uint32_t id, fontInfo* fi)
{
	uint32_t len;
	const char* name;
	const char* min_max;
	char section[32];
	uint32_t as_font = 0;
	static uint8_t recursion = 0;
	bool ret = true;
	char* key_name;
	uint32_t key_len;
	uint32_t token;
	bool poly_found = false;

	sprintf(section, "font%lu", id);
	debugMsg(DebugInfo, "Reading %s", section);

	if (!recursion)
		memset(fi, 0, sizeof(fontInfo));

	// Put the configuration file pointer to the first found font
	if (!config_file.setFileRWPointer(first_font_offset))
		return false;

	// Check if the font info is like another one that already exists
	if (config_file.readValue(section, "as_font", &as_font))
	{
		if (++recursion == 3)
		{
			recursion = 0;
			debugMsg(DebugError, "Cannot use as_font recursively");
			return false;
		}

		if (!loadFontInfo(as_font, fi))
		{
			debugMsg(DebugError, "Error locating font%lu", as_font);
			return false;
		}

		recursion = 0;
	}

	if (as_font)
	{
		// We have to set pointer back again at the first found font since we don't know
		// if the as_font font comes before or after the font we want to read
		if (!config_file.setFileRWPointer(first_font_offset))
			return false;
	}

	if (!config_file.startSectionScan(section))
	{
		debugMsg(DebugError, "Error locating section '%s'", section);
		return false;
	}

	fi->id = id;

	while (config_file.getNextKey(&token, &key_name, &key_len))
	{
		// Font title is not mandatory
		if (strncasecmp("title", key_name, key_len) == 0)
		{
			len = MAX_FONT_NAME_LEN;
			config_file.readValue(token, tmp, &len);

			if (len)
				strcpy(fi->title, tmp);
			continue;
		}

		// Font folder is mandatory and must exists
		if (strncasecmp("folder", key_name, key_len) == 0)
		{
			len = MAX_FONT_NAME_LEN;
			config_file.readValue(token, fi->folder, &len);
			continue;
		}

		// Polyphony info is mandatory
		if (strncasecmp("poly", key_name, key_len) == 0)
		{
			config_file.readValue(token, &fi->poly);
			poly_found = true;
			continue;
		}

		fontSoundFile* sound_file = NULL;

		if (strncasecmp(key_name, "boot", 4) == 0)
		{
			name = "boot";
			min_max = "boot_min_max";
			sound_file = &fi->files[fontBoot];
		} else if (strncasecmp(key_name, "ignition", 8) == 0)
		{
			name = "ignition";
			min_max = "ignition_min_max";
			sound_file = &fi->files[fontIgnition];
		} else if (strncasecmp(key_name, "retraction", 10) == 0)
		{
			name = "retraction";
			min_max = "retraction_min_max";
			sound_file = &fi->files[fontRetraction];
		} else if (strncasecmp(key_name, "low_power", 9) == 0)
		{
			name = "low_power";
			min_max = "low_power_min_max";
			sound_file = &fi->files[fontLowPower];
		} else if (strncasecmp(key_name, "hum", 3) == 0)
		{
			name = "hum";
			min_max = "hum_min_max";
			sound_file = &fi->files[fontHum];
		} else if (strncasecmp(key_name, "blaster", 7) == 0)
		{
			name = "blaster";
			min_max = "blaster_min_max";
			sound_file = &fi->files[fontBlaster];
		} else if (strncasecmp(key_name, "lock", 4) == 0)
		{
			name = "lock";
			min_max = "lock_min_max";
			sound_file = &fi->files[fontLock];
		} else if (strncasecmp(key_name, "swing", 5) == 0)
		{
			name = "swing";
			min_max = "swing_min_max";
			sound_file = &fi->files[fontSwing];
		} else if (strncasecmp(key_name, "clash", 5) == 0)
		{
			name = "clash";
			min_max = "clash_min_max";
			sound_file = &fi->files[fontClash];
		} else if (strncasecmp(key_name, "spin", 4) == 0)
		{
			name = "spin";
			min_max = "spin_min_max";
			sound_file = &fi->files[fontSpin];
		} else if (strncasecmp(key_name, "stab", 4) == 0)
		{
			name = "stab";
			min_max = "stab_min_max";
			sound_file = &fi->files[fontStab];
		} else if (strncasecmp(key_name, "force", 5) == 0)
		{
			name = "force";
			min_max = "force_min_max";
			sound_file = &fi->files[fontForce];
		} else if (strncasecmp(key_name, "background", 10) == 0)
		{
			name = "background";
			min_max = "background_min_max";
			sound_file = &fi->files[fontBackground];
		} else if (strncasecmp(key_name, "name", 4) == 0)
		{
			name = "name";
			min_max = NULL;
			sound_file = &fi->files[fontName];
		} else continue;

		if (strncasecmp(name, key_name, key_len) == 0)
		{
			len = MAX_FONT_NAME_LEN;
			config_file.readValue(token, sound_file->filename, &len);

			sound_file->present = (len > 0);
			continue;
		}

		if (min_max)
		{
			if (strncasecmp(min_max, key_name, key_len) == 0)
			{
				uint8_t min_max_array[2];
				uint8_t min_max_array_len = 2;
				config_file.readArray(token, min_max_array, &min_max_array_len);

				if (min_max_array_len == 2)
				{
					sound_file->min = min_max_array[0];
					sound_file->max = min_max_array[1];

					if (sound_file->min != sound_file->max)
						sound_file->random = true;
				} else {
					sound_file->min = 0;
					sound_file->max = 0;
					sound_file->random = false;
				}
				continue;
			}
		} else {
			sound_file->random = false;
		}
	}

	// Check
	if (!folderExists(fi->folder))
	{
		debugMsg(DebugError, "Error locating folder %s", fi->folder);
		ret = false;
	}

	if (!poly_found)
	{
		debugMsg(DebugError, "poly value not found");
		ret = false;
	}

	config_file.endSectionScan();
	return ret;
}

bool PBSConfig::open(const char* file)
{
	debugMsg(DebugInfo, "Configuration file is %s", file);

	if (!config_file.begin(file))
	{
		debugMsg(DebugError, "Error opening configuration file");
		return false;
	}

	map();
	return true;
}

bool PBSConfig::mapCallback(uint32_t section, uint32_t data, char* str)
{
	UNUSED(data);

	if (hardware_offset == 0 && strcmp(str, "hardware") == 0)
		hardware_offset = section;

	if (settings_offset == 0 && strcmp(str, "settings") == 0)
		settings_offset = section;

	if (strncasecmp(str, "profile", 7) == 0)
	{
		profile_count++;

		if (first_profile_offset == 0)
			first_profile_offset = section;
	}

	if (first_font_offset == 0 && strncasecmp(str, "font", 4) == 0)
		first_font_offset = section;

	return true;
}

void PBSConfig::map()
{
	hardware_offset = settings_offset = first_profile_offset = first_font_offset = 0;
	profile_count = 0;

	debugMsg(DebugInfo, "Mapping configuration file");

	timeCounter.startCounter();

	config_file.mapSections(mapCallbackStub, this);

	debugMsg(DebugInfo, "Mapping done in %lums", timeCounter.elapsed());
}

void PBSConfig::dumpProfileInfo(saberProfile* profile)
{
#if PBS_DEBUG
	const char* str = NULL;

	debugMsg(DebugInfo, "-- Start profile info dump for profile%i", profile->id);
	debugMsg(DebugInfo, "font_num = %i", profile->font_num);
	debugMsg(DebugInfo, "ignition_duration = %i", profile->ignition_duration);
	switch (profile->ignition_mode)
	{
		case ignitionRamp: str = "ramp"; break;
		case ignitionFull: str = "full"; break;
		case ignitionScroll: str = "scroll"; break;
		default: str = "unknown"; break;
	}
	debugMsg(DebugInfo, "ignition_mode = %s", str);

	debugMsg(DebugInfo, "retraction_duration = %i", profile->retraction_duration);
	switch (profile->retraction_mode)
	{
		case retractionRamp: str = "ramp"; break;
		case retractionFull: str = "full"; break;
		case retractionScroll: str = "scroll"; break;
		default: str = "unknown"; break;
	}
	debugMsg(DebugInfo, "retraction_mode = %s", str);

	if (profile->shimmer.randomize)
		str = "random";
	else
		str = "pulse";

	debugMsg(DebugInfo, "shimmer_mode = %s", str);
	debugMsg(DebugInfo, "shimmer_color = %i,%i,%i,%i", profile->shimmer.base_color.r,
													   profile->shimmer.base_color.g,
													   profile->shimmer.base_color.b,
													   profile->shimmer.base_color.w);
	debugMsg(DebugInfo, "shimmer_freq = %lu", profile->shimmer.freq);
	debugMsg(DebugInfo, "shimmer_depth = %lu", profile->shimmer.depth);

	for (uint8_t i = 0; i < 4; i++)
	{
		bladeEffect* effect;
		const char* type;
		const char* mode;
		switch (i)
		{
			case 0: type = "clash"; effect = &profile->clash; break;
			case 1: type = "blaster"; effect = &profile->blaster; break;
			case 2: type = "stab"; effect = &profile->stab; break;
			case 3: type = "lockup"; effect = &profile->lockup; break;
		}

		switch (effect->type)
		{
			case effectTypeStatic: mode = "static"; break;
			case effectTypeShimmer: mode = "shimmer"; break;
			case effectTypeFlash: mode = "flash"; break;
			case effectTypeSpark: mode = "spark"; break;
			case effectTypeFlashSpark: mode = "flash_spark"; break;
			case effectTypeBigSpark: mode = "big_spark"; break;
			case effectTypeTipSpark: mode = "tip_spark"; break;
			case effectTypeFlashFlicker: mode = "flash_flicker"; break;
			default: mode = "unknown";
		}

		debugMsg(DebugInfo, "%s_mode = %s", type, mode);
		debugMsg(DebugInfo, "%s_color = %i,%i,%i,%i", type, effect->base_color.r,
		         	 	 	 	 	 	 	 	 	 	 	 effect->base_color.g,
		         	 	 	 	 	 	 	 	 	 	 	 effect->base_color.b,
		         	 	 	 	 	 	 	 	 	 	 	 effect->base_color.w);
		debugMsg(DebugInfo, "%s_blend = %lu", type, effect->blend);
		debugMsg(DebugInfo, "%s_freq = %lu", type, effect->freq);
		debugMsg(DebugInfo, "%s_depth = %lu", type, effect->depth);
		debugMsg(DebugInfo, "%s_random = %s", type, effect->randomize ? "yes" : "no");
		debugMsg(DebugInfo, "%s_duration = %lu", type, effect->duration);
	}

	debugMsg(DebugInfo, "-- End profile info dump for profile%i", profile->id);
#endif
}

void PBSConfig::dumpFontInfo(fontInfo* font)
{
#if PBS_DEBUG
	debugMsg(DebugInfo, "-- Start info dump for font%i", font->id);
	debugMsg(DebugInfo, "title = %s", font->title);
	debugMsg(DebugInfo, "folder = %s", font->folder);
	debugMsg(DebugInfo, "poly = %s", font->poly ? "yes" : "no");

	for (int32_t i = 0; i < fontMax; i++)
	{
		const char* type;

		switch (i)
		{
			case fontBoot: type = "boot"; break;
			case fontIgnition: type = "ignition"; break;
			case fontRetraction: type = "retraction"; break;
			case fontLowPower: type = "low_power"; break;
			case fontHum: type = "hum"; break;
			case fontBlaster: type = "blaster"; break;
			case fontLock: type = "lock"; break;
			case fontSwing: type = "swing"; break;
			case fontClash: type = "clash"; break;
			case fontSpin: type = "spin"; break;
			case fontStab: type = "stab"; break;
			case fontBackground: type = "background"; break;
			case fontForce: type = "force"; break;
			case fontName: type = "name"; break;
			default:
			case fontMax: type = "unknown"; break;
		}

		if (!font->files[i].present)
		{
			debugMsg(DebugInfo, "%s = not present", type);
		} else {
			if (font->files[i].random) // It means they have min and max values
			{
				debugMsg(DebugInfo, "%s = %s\\%s(%i-%i).wav", type, font->folder,
																	font->files[i].filename,
																	font->files[i].min,
																	font->files[i].max);
			} else {
				debugMsg(DebugInfo, "%s = %s\\%s.wav", type, font->folder,
															 font->files[i].filename);
			}
		}
	}

	debugMsg(DebugInfo, "-- End info dump for font%i", font->id);
#endif
}

bool PBSConfig::saveInitialProfile(uint32_t id)
{
	if (config_file.setFileRWPointer(settings_offset))
	{
		if (config_file.writeValue("settings", "initial_profile", id))
		{
			// Remap
			map();
			return true;
		}
	}

	return false;
}
