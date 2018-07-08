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

### PBSaber.h

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

#ifndef __PBSCLASS_H__
#define __PBSCLASS_H__

#include <Arduino.h>
#include "PBSBlade.h"
#include "PBSConfig.h"
#include "PBSDebug.h"
#include "PBSStrip.h"
#include "TimeCounter.h"
#include <PropButton.h>
#include <stdint.h>
#include <strings.h>
#include <stdio.h>
#include <stdarg.h>

#define PBSABER_VER_MAJOR		1
#define PBSABER_VER_MINOR		0
#define PBSABER_VER_FIX			0

#define PBSABER_CONFIG_FILE	"config.ini"

#define fontPresent(x) (current_profile.font.files[x].present)

typedef enum
{
	stateOff,
	stateIdleOff,
	stateMusic,
	stateCycleProfiles,
	stateIgnition,
	stateIdleOn,
	stateRetraction,
	stateBlaster,
	stateLockUp,
	stateClash,
	stateSwing,
	stateSpin,
	stateStab,
	stateForce,
	stateNextProfile,
	statePrevProfile,
	stateMAX,
} saberStateId;

typedef enum
{
	sndutilBeep
} saberUtilitySound;

#define DECLARE_STATE(X)		\
void enterState##X();			\
void pollState##X();

typedef void (onNewState)(saberStateId);
typedef void (onEffect)(uint32_t, saberStateId, bladeEffect&);

class PBSaber
{
public:
	PBSaber();
	bool begin(const char* config_file = NULL);
	void loop();

	void setNewStateCallback(onNewState* fnptr)
	{
		newStateCallback = fnptr;
	}

	void setOnEffectCallback(onEffect* fnptr)
	{
		onEffectCallback = fnptr;
	}


private:

	bool initializeHardware();
	void resetAllButtonsEvents();
	void resetAllMotionEvents();
	bool loadProfile(uint32_t id, saberProfile* profile);
	bool loadNextProfile(saberProfile* profile);
	bool loadPrevProfile(saberProfile* profile);
	void changeProfile(saberProfile* new_profile);
	bool play(fontSoundType type, PlayMode mode = PlayModeNormal);
	void setNextProfile();
	void setPrevProfile();
	bool switchProfile();
	void enterState(saberStateId state);
	void getSoundFileName(char* dst, fontInfo* font, fontSoundType type);
	void getSpinFileName(char* dst, fontInfo* font, uint32_t num);
	void debugOutput();
	void playUtility(saberUtilitySound snd, PlayMode mode = PlayModeNormal);
	void motionPulses();
	void motionTransients();
	const char* getStateName(saberStateId state);
	bool ignitionOnStab();
	void handleButtonsEvents();
	void handleMotionEvents();
	void readAccelerometer();

	void notifyEffectToUser(bladeEffect& effect)
	{
		(onEffectCallback)(current_profile.id, curr_state, effect);
	}

	DECLARE_STATE(Off);
	DECLARE_STATE(IdleOff);
	DECLARE_STATE(IdleOn);
	DECLARE_STATE(Ignition);
	DECLARE_STATE(Retraction);
	DECLARE_STATE(Lock);
	DECLARE_STATE(Swing);
	DECLARE_STATE(Blaster);
	DECLARE_STATE(Clash);
	DECLARE_STATE(Spin);
	DECLARE_STATE(Stab);
	DECLARE_STATE(NextProfile);
	DECLARE_STATE(PrevProfile);
	DECLARE_STATE(Force);
	DECLARE_STATE(Music);
	DECLARE_STATE(CycleProfiles);

	static void motionPulsesStub(void* param)
	{
		PBSaber* ptr = (PBSaber*) param;
		ptr->motionPulses();
	}

	static void motionTransientsStub(void* param)
	{
		PBSaber* ptr = (PBSaber*) param;
		ptr->motionTransients();
	}

	inline PropButton* getButton(saberButtonType type)
	{
		if (type == buttonOnOff)
			return &config.hw.button_onoff.button;
		else if (type == buttonFx && config.hw.has_button_fx)
			return &config.hw.button_fx.button;
		return NULL;
	}

	bool initialized;
	PBSConfig config;
	PBSBladeBase* blade;

	saberProfile current_profile;
	saberProfile tmp_profile;
	saberStateId prev_state;
	saberStateId curr_state;

	WavPlayer hum1;
	WavPlayer hum2;
	WavPlayer* hum;
	WavChainPlayer monoFont1;
	WavChainPlayer monoFont2;
	WavChainPlayer* monoFont;
	WavPlayer fx;
	WavPlayer music1;
	WavPlayer music2;
	WavPlayer* music;

	bool new_font;
	bool background_changed;
	RawPlayer* new_font_player;
	RawPlayer* prev_font_player;
	WavPlayer* new_bkg_player;
	WavPlayer* prev_bkg_player;
	float new_font_step;
	float new_font_target;
	uint32_t new_font_cycles;

	uint32_t off_start_time;
	uint32_t current_sound_duration;
	uint32_t current_sound_start;

	TimeCounter swing_counter;
	TimeCounter spin_limit_counter;
	TimeCounter spin_counter;
	TimeCounter clash_counter;
	TimeCounter stab_counter;

	bool possible_stab;

	uint8_t spin_count;
	uint32_t spin_num;
	bool spinning;

	volatile bool event_clash;
	volatile bool event_swing;

	uint32_t first_profile;
	uint32_t last_profile;

	bool save_initial_profile;
	uint32_t profile_at_ignition;

	uint32_t debug_interval;
	uint32_t debug_ticks;

	TimeCounter volumeCounter;

	onNewState* newStateCallback;
	onEffect* onEffectCallback;

	char tmp[255];
};

#endif /* __PBSCLASS_H__ */
