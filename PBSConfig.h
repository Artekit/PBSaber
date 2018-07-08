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

### PBSConfig.h

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

#ifndef __PBSCONFIG_H__
#define __PBSCONFIG_H__

#include <Arduino.h>
#include <PropButton.h>
#include <PropConfig.h>
#include <LedStripDriver.h>
#include "PBSDebug.h"
#include "TimeCounter.h"

#define MAX_FONT_NAME_LEN		32

// Blade types
typedef enum _bladeType
{
	bladeHBLED = 1,
	bladeStrip
} bladeType;

// Base flash effect modes
typedef enum
{
	effectTypeInvalid = 0,
	effectTypeStatic,
	effectTypeShimmer,
	effectTypeFlash,
	effectTypeFlashFlicker,
	effectTypeSpark,
	effectTypeFlashSpark,
	effectTypeBigSpark,
	effectTypeTipSpark,
} effectType;

// Lock-up modes
typedef enum
{
	lockRandom = 1,
	lockRamp,
} lockMode;

// Base ignition modes
typedef enum
{
	ignitionRamp = 1,
	ignitionFull,
	ignitionScroll,
} ignitionMode;

// Base retraction modes
typedef enum
{
	retractionRamp = 1,
	retractionFull,
	retractionScroll,
} retractionMode;

typedef enum
{
	buttonOnOff,
	buttonFx
} saberButtonType;

typedef struct
{
	uint32_t pin;
	bool active_high;
	PropButton button;
} saberButton;

typedef struct
{
	bladeType blade_type;
	LedStripeType strip_type;
	uint32_t strip_count;

	uint16_t hbled_current[3];
	bool hbled_is_rgb;

	saberButton button_onoff;
	saberButton button_fx;
	bool has_button_fx;

} saberHardware;

typedef struct
{
	uint32_t profile_count;
	uint32_t initial_profile;
	bool update_initial_profile;
	float master_volume;
	uint32_t low_power;
	uint32_t audio_fs;
	uint32_t swing_sensitivity;
	uint32_t swing_limiter;
	uint32_t clash_limiter;
	uint32_t spin_limiter;
	uint32_t dynamic_swing_depth;
	uint32_t clash_sensitivity;
	uint32_t button_debounce;
	uint32_t off_time;
	uint32_t lock_time;
	char sound_utils[MAX_FONT_NAME_LEN];
	bool dump_profile_info;
	bool dump_font_info;

} saberSettings;

typedef struct
{
	bool present;
	bool random;
	char filename[MAX_FONT_NAME_LEN];
	uint32_t min, max;
} fontSoundFile;

typedef enum
{
	fontBoot,
	fontIgnition,
	fontRetraction,
	fontLowPower,
	fontHum,
	fontBlaster,
	fontLock,
	fontSwing,
	fontClash,
	fontSpin,
	fontStab,
	fontForce,
	fontBackground,
	fontName,
	fontMax
} fontSoundType;

typedef struct
{
	uint32_t id;
	bool poly;
	char title[MAX_FONT_NAME_LEN];
	char folder[MAX_FONT_NAME_LEN];

	fontSoundFile files[fontMax];

} fontInfo;

typedef struct
{
	effectType type;
	COLOR base_color;
	COLOR other_color;
	uint32_t freq;
	uint8_t depth;
	uint8_t blend;
	uint32_t duration;
	bool randomize;
} bladeEffect;

typedef struct
{
	uint32_t id;
	uint32_t font_num;
	fontInfo font;
	ignitionMode ignition_mode;
	uint32_t ignition_duration;
	bool ignition_on_stab;
	retractionMode retraction_mode;
	uint32_t retraction_duration;
	bladeEffect shimmer;
	bladeEffect clash;
	bladeEffect blaster;
	bladeEffect stab;
	bladeEffect lockup;
} saberProfile;

class PBSConfig
{
public:
	PBSConfig();
	bool open(const char* file);
	bool read();
	bool readAudioSettings();
	bool loadProfile(uint32_t id, saberProfile* profile);
	bool saveProfile(uint32_t id, saberProfile* profile);
	bool loadFontInfo(uint32_t id, fontInfo* fi);
	bool saveInitialProfile(uint32_t id);

	saberHardware hw;
	saberSettings settings;
	PropConfig config_file;

private:
	uint32_t hardware_offset;
	uint32_t settings_offset;
	uint32_t first_profile_offset;
	uint32_t first_font_offset;
	uint32_t profile_count;

	bool readSettings();
	bool readHardwareConfiguration();
	bool folderExists(char* folder);
	bool checkProfile(saberProfile* profile);
	void dumpProfileInfo(saberProfile* profile);
	void dumpFontInfo(fontInfo* font);
	void map();
	bool mapCallback(uint32_t section, uint32_t data, char* str);

	static bool mapCallbackStub(uint32_t section,
								uint32_t data, char* str, void* param)
	{
		PBSConfig* ptr = (PBSConfig*) param;
		return ptr->mapCallback(section, data, str);
	}

	char tmp[255];
	TimeCounter timeCounter;
};

#endif /* __PBSCONFIG_H__ */

