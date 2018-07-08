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

### PBSaber.cpp

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

#include "PBSaber.h"

static const float pulse_force_preset[5] = { 6, 4.5f, 4, 2.5f, 1 };
static const uint32_t pulse_time_preset[5] = { 500, 100, 50, 40, 20 };
static const uint32_t pulse_latency_preset[5] = { 200, 150, 100, 50, 25 };

static const float transient_force_preset[5] = { 0.9f, 0.7f, 0.6f, 0.5f, 0.3f };
static const uint32_t transient_time_preset[5] = { 80, 70, 50, 20, 20 };

#define STAB_REQUIRES_SWING		1
#define STAB_REQUIRES_CLASH		2

#define STAB_REQUIRES (STAB_REQUIRES_CLASH)

PBSaber::PBSaber()
{
	newStateCallback = NULL;
	onEffectCallback = NULL;
	initialized = false;
	background_changed = false;
	new_font = false;
	spin_count = 0;
	spinning = false;
	possible_stab = false;
}

bool PBSaber::begin(const char* config_file)
{
	if (initialized)
		return true;

	// Initialize debug and print version
	initDebug();
	debugMsg(DebugInfo, "- PBSaber v%i.%i.%i -", PBSABER_VER_MAJOR,
												 PBSABER_VER_MINOR,
	         	 	 	 	 	 	 	 	 	 PBSABER_VER_FIX);

	// Initialize SD debug (TODO remove)
	enableSdDebug(&Serial);

	// The user can provide its own configuration file path. If none is provided then use
	// the default one.
	if (config_file == NULL)
		config_file = PBSABER_CONFIG_FILE;

	// Open the configuration file and map it
	if (!config.open(config_file))
		return false;

	// Load and initialize Audio while the rest is loading. For this we will need to load
	// the audio_fs parameter from the 'Settings' section.
	debugMsg(DebugInfo, "Initializing audio");
	if (!config.readAudioSettings())
	{
		debugMsg(DebugError, "Failed while reading audio settings");
		return false;
	}

	uint32_t audio_init = GetTickCount();

	// Initialize audio
	if (!Audio.begin(config.settings.audio_fs, 16, true))
	{
		debugMsg(DebugError, "Audio initialization failed");
		return false;
	}

	// Read the rest of the global configuration (hardware & settings)
	if (!config.read())
		return false;

	// Get how many profiles there are in the configuration file. User can declare the quantity
	// directly on the configuration file. If not, the mapping function of the configuration
	// file will retrieve the (estimated) quantity.
	first_profile = 1;
	last_profile = config.settings.profile_count;

	save_initial_profile = false;

	// Check if the initial_profile is between the minimum and maximum profile number
	if (config.settings.initial_profile < first_profile ||
		config.settings.initial_profile > last_profile)
	{
		debugMsg(DebugWarning, "start_profile value (%i) is out of range (%i-%i). Using profile1",
								config.settings.initial_profile, first_profile, last_profile);

		save_initial_profile = true;
		config.settings.initial_profile = 1;
	}

	// Try to load the profile
	if (!config.loadProfile(config.settings.initial_profile, &current_profile))
	{
		debugMsg(DebugWarning, "Failed while reading profile%i", config.settings.initial_profile);
		return false;
	}

	// We can initialize the rest of the hardware
	if (!initializeHardware())
	{
		debugMsg(DebugError, "Failed while initializing hardware");
		return false;
	}

	// Initiate audio objects pointers
	hum = &hum1;
	monoFont = &monoFont1;
	music = &music1;

	// Wait until audio initializes (it takes 900ms approximately).
	uint32_t ticks = GetTickCount();
	if (ticks - audio_init < 900)
		delay(900 - (ticks - audio_init));

	// Debug output timing
	debug_ticks = GetTickCount();
	debug_interval = 1000; // ms

	// We're ready to go. Play boot sound and set stateOff.
	initialized = true;
	play(fontBoot);
	enterState(stateOff);
	return true;
}

bool PBSaber::initializeHardware()
{
	debugMsg(DebugInfo, "Initializing hardware");

	// Set volume
	Audio.setVolume(config.settings.master_volume);

	// Unmute
	Audio.unmute();

	// Initialize accelerometer
	Motion.begin(8, 400, false);

    config.settings.clash_sensitivity = 3;
    config.settings.swing_sensitivity = 3;
    
	// Configure motion pulse for clashes
	Motion.configPulse(AxisAll, pulse_force_preset[config.settings.clash_sensitivity-1],
								pulse_time_preset[config.settings.clash_sensitivity-1],
								pulse_latency_preset[config.settings.clash_sensitivity-1],
								MotionInterrupt1);

	// Configure motion transient for swings
	Motion.configTransient(AxisAll, transient_force_preset[config.settings.swing_sensitivity-1],
									transient_time_preset[config.settings.swing_sensitivity-1],
									MotionInterrupt2);

	// Attach both interrupts
	Motion.attachInterruptWithParam(MotionInterrupt1, motionPulsesStub, this);
	Motion.attachInterruptWithParam(MotionInterrupt2, motionTransientsStub, this);

	// Enable transient latch (we want to query MMA8452_TRANSIENT_SRC)
	uint8_t transient_cfg;
	if (!Motion.readRegister(MMA8452_TRANSIENT_CFG, &transient_cfg))
		return false;

	transient_cfg |= (1 << 4);

	if (!Motion.writeRegister(MMA8452_TRANSIENT_CFG, transient_cfg))
		return false;

#if (STAB_REQUIRES & STAB_REQUIRES_CLASH)
	// Enable pulse latch (we want to query MMA8452_PULSE_SRC)
	uint8_t pulse_cfg;
	if (!Motion.readRegister(MMA8452_PULSE_CFG, &pulse_cfg))
		return false;

	pulse_cfg |= (1 << 6);

	if (!Motion.writeRegister(MMA8452_PULSE_CFG, pulse_cfg))
		return false;
#endif

	Motion.enable();

	// Initialize buttons. On/Off button is mandatory.
	config.hw.button_onoff.button.begin(config.hw.button_onoff.pin,
								 	 	config.hw.button_onoff.active_high ?
								 	 			ButtonActiveHigh : ButtonActiveLow,
										config.settings.button_debounce);

	config.hw.button_onoff.button.setLongPressTime(config.settings.off_time);

	// The FX button is optional
	if (config.hw.has_button_fx)
	{
		config.hw.button_fx.button.begin(config.hw.button_fx.pin,
										 config.hw.button_fx.active_high ?
												 ButtonActiveHigh : ButtonActiveLow,
										 config.settings.button_debounce);

		config.hw.button_fx.button.setLongPressTime(config.settings.lock_time);
	}

	// Initialize blade
	bool success = false;
	if (config.hw.blade_type == bladeHBLED)
	{
		blade = new RgbHbLedBlade(config.hw.hbled_current[0],
								  config.hw.hbled_current[1],
								  config.hw.hbled_current[2]);

	} else {
 		blade = new LedStripBlade(config.hw.strip_type, config.hw.strip_count);
 	}

	if (blade)
		success = blade->initialize();

	if (!success)
	{
		debugMsg(DebugError, "Blade initialization failed");
		return false;
	}

	return true;
}

void PBSaber::debugOutput()
{

}

void PBSaber::loop()
{
	if (!initialized)
		return;

	// Update the blade
	blade->update();

	// Check the current state.
	// TODO we are OK right now, but we should pull out a fancy FSM pattern.
	switch (curr_state)
	{
		case stateIdleOn:			pollStateIdleOn();			break;
		case stateOff:				pollStateOff();				break;
		case stateIdleOff:			pollStateIdleOff();			break;
		case stateMusic:			pollStateMusic();			break;
		case stateCycleProfiles:	pollStateCycleProfiles();	break;
		case stateForce:			pollStateForce();			break;
		case stateLockUp:			pollStateLock();			break;
		case stateIgnition:			pollStateIgnition();		break;
		case stateRetraction:		pollStateRetraction();		break;
		case stateBlaster:			pollStateBlaster();			break;
		case stateClash:			pollStateClash();			break;
		case stateSwing:			pollStateSwing();			break;
		case stateSpin:				pollStateSpin();			break;
		case stateStab:				pollStateStab();			break;
		case stateNextProfile:		pollStateNextProfile();		break;
		case statePrevProfile:		pollStatePrevProfile();		break;
		default: break;
	}

	if (GetTickCount() - debug_ticks >= debug_interval)
	{
		debug_ticks = GetTickCount();
		debugOutput();
	}
}

void PBSaber::enterState(saberStateId state)
{
	debugMsg(DebugInfo, "Entering state: %s", getStateName(state));

	prev_state = curr_state;
	curr_state = state;

	// Call user callback
	if (newStateCallback)
		(newStateCallback)(state);

	// Enter the new state.
	switch (state)
	{
		case stateIdleOn:			enterStateIdleOn();			break;
		case stateIdleOff:			enterStateIdleOff();		break;
		case stateOff:				enterStateOff();			break;
		case stateMusic:			enterStateMusic();			break;
		case stateCycleProfiles:	enterStateCycleProfiles();	break;
		case stateForce:			enterStateForce();			break;
		case stateClash:			enterStateClash();			break;
		case stateSwing:			enterStateSwing();			break;
		case stateSpin:				enterStateSpin();			break;
		case stateBlaster:			enterStateBlaster();		break;
		case stateLockUp:			enterStateLock();			break;
		case stateStab:				enterStateStab();			break;
		case stateIgnition:			enterStateIgnition();		break;
		case stateRetraction:		enterStateRetraction();		break;
		case stateNextProfile:		enterStateNextProfile();	break;
		case statePrevProfile:		enterStatePrevProfile();	break;
		default: break;
	}
}

bool PBSaber::loadProfile(uint32_t id, saberProfile* profile)
{
	// Trick to update the LED strip while loading the profile (can take some extra milliseconds
	// that may be visible on the strip, and gets worst if running on an very slow SD). We go
	// asynchronous by using the ServiceTimer object to momentarily update the strip on a timer
	// interrupt. It has effect only if the blade type we are using is an LED strip.
	blade->asyncUpdate();

	bool ret = config.loadProfile(id, profile);

	blade->syncUpdate();
	return ret;
}

bool PBSaber::loadNextProfile(saberProfile* profile)
{
	// Load the next profile taking care of rolling back to the first profile
	// if the maximum has been reached.
	uint32_t id = current_profile.id + 1;

	if (id > last_profile)
		id = 1;

	return loadProfile(id, profile);
}

bool PBSaber::loadPrevProfile(saberProfile* profile)
{
	// Load the previous profile taking care of loading the last profile in the list
	// if the minimum has been reached.
	uint32_t id = current_profile.id - 1;
	if (id == 0)
		id = last_profile;

	return loadProfile(id, profile);
}

void PBSaber::getSoundFileName(char* dst, fontInfo* font, fontSoundType type)
{
	// Assemble the full path for the given fontSoundType
	if (strlen(font->folder))
		sprintf(dst, "%s\\%s", font->folder, font->files[type].filename);
	else
		sprintf(dst, "%s", font->files[type].filename);

	// Random files do not need the extension. TODO support other extensions.
	if (!font->files[type].random)
		strcat(dst, ".wav");
}

void PBSaber::getSpinFileName(char* dst, fontInfo* font, uint32_t num)
{
	char file_num[32];
	getSoundFileName(dst, font, fontSpin);

	if (font->files[fontSpin].random)
	{
		sprintf(file_num, "%lu", num);
		strcat(dst, file_num);
	}

	strcat(dst, ".wav");
}

bool PBSaber::play(fontSoundType type, PlayMode mode)
{
	// In this function we check the best way to play a sound depending on its type
	// (swing, clash, etc.), the mono/poly capabilities of the font and if we need to play the
	// sound by picking a random file or not.
	bool ret = false;

	// We already know whether the requested fontSoundType is present
	if (!current_profile.font.files[type].present)
	{
		debugMsg(DebugError, "The requested font type is not present");
		return false;
	}

	current_sound_start = 0;
	current_sound_duration = 0;

	// 'Name' and 'boot' sounds are played with the "fx" player in any "poly" or "mono" case
	if (type == fontName || type == fontBoot)
	{
		// Get the full path
		getSoundFileName(tmp, &current_profile.font, type);

		// Random?
		if (current_profile.font.files[type].random)
		{
			ret = fx.playRandom(tmp,
								current_profile.font.files[type].min,
								current_profile.font.files[type].max,
								mode);
		} else {
			ret = fx.play(tmp, mode);
		}

		if (ret)
		{
			debugMsg(DebugInfo, "Playing %s", fx.getFileName());

			// Remember start time and duration
			current_sound_start = GetTickCount();
			current_sound_duration = fx.duration();
		} else {
			debugMsg(DebugError, "Error playing %s", fx.getFileName());
		}

		return ret;
	}

	// Special case for ignition sound on mono fonts. We use a chained player, and we need
	// to first initialize the main track (hum) and then chain the ignition sound.
	if (type == fontIgnition && !current_profile.font.poly)
	{
		// Get the full path for the hum sound
		getSoundFileName(tmp, &current_profile.font, fontHum);

		// Prepare the main track
		if (!monoFont->begin(tmp))
		{
			debugMsg(DebugError, "Error playing %s", tmp);
			return false;
		}

		debugMsg(DebugInfo, "Mono font: main track = %s", tmp);

		// Get the full path for the ignition sound
		getSoundFileName(tmp, &current_profile.font, type);

		// Chain the ignition sound
		if (current_profile.font.files[type].random)
			ret = monoFont->chainRandom(tmp,
										current_profile.font.files[type].min,
										current_profile.font.files[type].max, mode);
		else
			ret = monoFont->chain(tmp, mode);

		if (!ret)
		{
			debugMsg(DebugError, "Error playing %s", monoFont->getChainedFileName());
			monoFont->stop();
			return false;
		}

		// Start playing
		if (!monoFont->play())
		{
			debugMsg(DebugError, "Error playing %s", monoFont->getChainedFileName());
			monoFont->stop();
			return false;
		}

		debugMsg(DebugInfo, "Mono font: chained track = %s", monoFont->getChainedFileName());

		current_sound_start = GetTickCount();
		current_sound_duration = monoFont->getChainedDuration();
		return true;
	}

	// Special case for HUM sound on poly fonts, that is played on its dedicated player
	if (type == fontHum && current_profile.font.poly)
	{
		getSoundFileName(tmp, &current_profile.font, type);
		ret = hum->play(tmp, PlayModeLoop);
		if (ret)
		{
			debugMsg(DebugInfo, "Playing %s", tmp);
			current_sound_start = GetTickCount();
			current_sound_duration = hum->duration();
		} else {
			debugMsg(DebugError, "Error playing %s", tmp);
		}

		return ret;
	}

	// Special case for ignition sound on poly fonts:
	// Start playing ignition of the fx player, together with the hum on the hum player,
	// but with volume = 0. Increase hum volume in pollStateIgnition().
	if (type == fontIgnition && current_profile.font.poly)
	{
		// Get the full path for the hum sound
		getSoundFileName(tmp, &current_profile.font, fontHum);

		// Set the hum sound volume to zero
		hum->setVolume(0);

		// Start playing the hum
		ret = hum->play(tmp, PlayModeLoop);
		if (!ret)
			return ret;

		debugMsg(DebugInfo, "Playing %s", tmp);

		// Get the full path for the ignition sound
		getSoundFileName(tmp, &current_profile.font, type);

		// Play the ignition sound
		if (current_profile.font.files[type].random)
			ret = fx.playRandom(tmp, current_profile.font.files[type].min,
									 current_profile.font.files[type].max);
		else
			ret = fx.play(tmp);

		if (ret)
		{
			debugMsg(DebugInfo, "Playing %s", fx.getFileName());
			current_sound_start = GetTickCount();
			current_sound_duration = fx.duration();
			volumeCounter.startTimeoutCounter(40);
		} else {
			debugMsg(DebugError, "Error playing %s", fx.getFileName());
			hum->stop();
		}

		return ret;
	}

	// Special case for retraction sound on poly fonts:
	// Hum is already playing, so we start to play the retraction sound on the fx player and
	// volume for the hum sound get decreased until 0 in pollStateRetraction().
	if (type == fontRetraction && current_profile.font.poly)
	{
		// Get the full path for the retraction sound
		getSoundFileName(tmp, &current_profile.font, type);

		// Play the ignition sound
		if (current_profile.font.files[type].random)
			ret = fx.playRandom(tmp, current_profile.font.files[type].min,
									 current_profile.font.files[type].max);
		else
			ret = fx.play(tmp);

		if (ret)
		{
			debugMsg(DebugInfo, "Playing %s", fx.getFileName());
			volumeCounter.startTimeoutCounter(40);
			current_sound_start = GetTickCount();
			current_sound_duration = fx.duration();
		}

		return ret;
	} else if (type == fontBackground)
	{
		// Background is played with the music player
		getSoundFileName(tmp, &current_profile.font, type);

		ret = music->play(tmp, mode);
		if (ret)
		{
			debugMsg(DebugInfo, "Playing %s", tmp);
			current_sound_start = GetTickCount();
			current_sound_duration = fx.duration();
		} else {
			debugMsg(DebugError, "Error playing %s", tmp);
		}

		return ret;
	}

	// Spin sound number gets selected at the moment of spin-lock detection
	if (type == fontSpin)
	{
		getSpinFileName(tmp, &current_profile.font, spin_num);
		if (current_profile.font.poly)
		{
			ret = monoFont->chain(tmp);
			if (ret)
			{
				current_sound_duration = monoFont->getChainedDuration();
				debugMsg(DebugInfo, "Mono font: chained track = %s", monoFont->getChainedFileName());
			} else {
				debugMsg(DebugError, "Error playing %s", monoFont->getChainedFileName());
			}
		} else {
			ret = fx.play(tmp);
			if (ret)
			{
				current_sound_duration = fx.duration();
				debugMsg(DebugInfo, "Playing %s", fx.getFileName());
			} else {
				debugMsg(DebugError, "Error playing %s", fx.getFileName());
			}
		}

		if (ret)
			current_sound_start = GetTickCount();

		return ret;
	}

	// Any other sound is played in their respective players
	getSoundFileName(tmp, &current_profile.font, type);

	if (current_profile.font.poly)
	{
		if (current_profile.font.files[type].random)
			ret = fx.playRandom(tmp, current_profile.font.files[type].min,
									 current_profile.font.files[type].max,
									 mode);
		else
			ret = fx.play(tmp, mode);

		if (ret)
		{
			debugMsg(DebugInfo, "Playing %s", fx.getFileName());
			current_sound_duration = fx.duration();
		} else {
			debugMsg(DebugError, "Error playing %s", fx.getFileName());
		}
	} else {
		if (current_profile.font.files[type].random)
			ret = monoFont->chainRandom(tmp,
										current_profile.font.files[type].min,
										current_profile.font.files[type].max, mode);
		else
			ret = monoFont->chain(tmp, mode);

		if (ret)
		{
			debugMsg(DebugInfo, "Mono font: chained track = %s", monoFont->getChainedFileName());
			current_sound_duration = monoFont->getChainedDuration();
		} else {
			debugMsg(DebugError, "Error playing %s", monoFont->getChainedFileName());
		}
	}

	if (ret)
		current_sound_start = GetTickCount();

	return ret;
}

void PBSaber::resetAllButtonsEvents()
{
	PropButton* onButton = getButton(buttonOnOff);
	PropButton* fxButton = getButton(buttonFx);

	// Forget all preceding button events
	onButton->resetEvents();
	if (fxButton)
		fxButton->resetEvents();
}

void PBSaber::resetAllMotionEvents()
{
	// Reset both swing and clash motion flags
	__disable_irq();
	event_clash = event_swing = false;
	__enable_irq();

	// Clear any latched values
	Motion.getTransientSource();
	Motion.getPulseSource();
}

void PBSaber::enterStateOff()
{
	// Remember when we've gone OFF
	off_start_time = GetTickCount();

	// Update the start_profile value, if configured to do so
	if (save_initial_profile)
	{
		if (config.settings.update_initial_profile)
		{
			if (config.saveInitialProfile(current_profile.id))
				debugMsg(DebugInfo, "Updated initial_profile = %lu", current_profile.id);
			else
				debugMsg(DebugError, "Error updating initial_profile");
		}

		save_initial_profile = false;
	}
}

void PBSaber::pollStateOff()
{
	// Wait for the user to release all buttons
	PropButton* onButton = getButton(buttonOnOff);
	PropButton* fxButton = getButton(buttonFx);

	if (!onButton->released())
		return;

	if (fxButton && !fxButton->released())
		return;

	enterState(stateIdleOff);
}

void PBSaber::enterStateIdleOff()
{
}

void PBSaber::pollStateIdleOff()
{
	// Monitor ignition button sequence
	PropButton* onButton = getButton(buttonOnOff);
	PropButton* fxButton = getButton(buttonFx);

	ButtonEvent event = onButton->getEvent();

	// Short press-and-release is common for both single or dual button configurations
	if (event == ButtonShortPressAndRelease)
	{
		// Check if the profile has background music
		if (current_profile.font.files[fontBackground].present)
			// Then jump to the Music state
			enterState(stateMusic);
		else
			// Do normal ignition
			enterState(stateIgnition);
		return;
	}

	// Monitor for ignition-on-stab
	if (ignitionOnStab())
		enterState(stateIgnition);

	if (fxButton)
	{
		// Saber has an FX button
		event = fxButton->getEvent();

		// Monitor Next Profile sequence
		if (event == ButtonShortPressAndRelease)
		{
			enterState(stateNextProfile);
			return;
		}

		if (event == ButtonLongPressed)
		{
			// Configuration mode?
			// For now, set the previous profile
			enterState(statePrevProfile);
			return;
		}
	} else {
		// Saber has only On/Off button. Use long-press on the On/Off button to enter
		// the "profile cycle mode"
		if (event == ButtonLongPressed)
		{
			enterState(stateCycleProfiles);
			return;
		}
	}

	// Check if we have to go into low power
	if (config.settings.low_power)
	{
		if (GetTickCount() - off_start_time > (config.settings.low_power * 1000))
		{
			debugMsg(DebugInfo, "Entering low power mode after %i seconds",
								(GetTickCount() - off_start_time) / 1000);

			delay(100);

			// Power down sound if any
			play(fontLowPower, PlayModeBlocking);

			// Low power
			enterLowPowerMode(config.hw.button_onoff.pin,
							  config.hw.button_onoff.active_high ? RISING : FALLING,
							  false);
		}
	}
}

void PBSaber::enterStateIgnition()
{
	// Start playing ignition sound
	if (!play(fontIgnition))
	{
		// Ignition sound not found, not going anywhere
		enterState(stateOff);
		return;
	}

	// Update shimmer
	blade->setShimmerMode(&current_profile.shimmer);

	// Start ignition LED effect
	uint32_t duration = current_profile.ignition_duration;
	if (!duration)
		duration = current_sound_duration;
	blade->onIgnition(current_profile.ignition_mode, duration);

	// Remember the current profile
	profile_at_ignition = current_profile.id;

	// If it is a poly font, set a counter to increment the hum volume every 40ms up to 1
	if (current_profile.font.poly)
		volumeCounter.startTimeoutCounter(40);
}

void PBSaber::pollStateIgnition()
{
	bool ready = true;

	// Wait for user to release the ON button
	PropButton* onButton = getButton(buttonOnOff);
	ready = onButton->released();

	// Wait for all "ignition" audio to end
	if (current_profile.font.poly)
	{
		volatile float volume = hum->getVolume();
		if (volume == 1)
		{
			ready &= true;
		} else {
			ready = false;

			// Increment the hum volume up to 1, each 40ms
			if (volumeCounter.timeout())
			{
				volumeCounter.startTimeoutCounter(40);
				float step = 1.0f / (current_sound_duration / 40);
				volume += step;
				if (volume > 1)
					volume = 1;
				hum->setVolume(volume);
			}
		}

		if (fx.playing())
			ready = false;
	} else {
		if (monoFont->playingChained())
			ready = false;
	}

	// Wait for the ignition effect on blade is over
	if (blade->getState() != bladeStateIdle)
		ready = false;

	// If ready, move to IDLE ON state
	if (ready)
		enterState(stateIdleOn);
}

void PBSaber::enterStateIdleOn()
{
	// Reset any previous button events
	resetAllButtonsEvents();

	// Reset any previous motion events
	resetAllMotionEvents();
}

void PBSaber::handleButtonsEvents()
{
	PropButton* onButton = getButton(buttonOnOff);
	PropButton* fxButton = getButton(buttonFx);
	ButtonEvent event;
	event = onButton->getEvent();

	// On/Off button long-pressed. This event is valid for both with and without FX button sabers
	if (event == ButtonLongPressed)
	{
		enterState(stateRetraction);
		return;
	}

	if (fxButton)
	{
		// Saber has FX button

		// Check NEXT PROFILE sequence
		// (On/Off button pressed event + an already pressed FX button)
		if (event == ButtonPressed && fxButton->pressed() && config.settings.profile_count > 1)
		{
			enterState(stateNextProfile);
			return;
		}

		// Check events for the FX button
		event = fxButton->getEvent();

		// Blaster effect
		if (event == ButtonShortPressAndRelease && fontPresent(fontBlaster))
		{
			enterState(stateBlaster);
			return;
		}

		// Lock-up effect. First check if there is a lock-up sound in the current font.
		if (event == ButtonLongPressed && fontPresent(fontLock))
		{
			enterState(stateLockUp);
			return;
		}

		// Check PREV PROFILE sequence
		// (FX button pressed with an already pressed On/Off button)
		if (event == ButtonPressed && onButton->pressed() && config.settings.profile_count > 1)
		{
			enterState(statePrevProfile);
			return;
		}
	} else {
		// Saber has only On/Off button

		// Blaster is done through the On/Off button
		if (event == ButtonShortPressAndRelease && fontPresent(fontBlaster))
		{
			enterState(stateBlaster);
			return;
		}
	}
}

void PBSaber::handleMotionEvents()
{
	PropButton* onButton = getButton(buttonOnOff);
	PropButton* fxButton = getButton(buttonFx);

#if (STAB_REQUIRES == (STAB_REQUIRES_SWING | STAB_REQUIRES_CLASH))
	// Time between a X-axis swing and a clash to be considered a stab
	if (stab_counter.active() && stab_counter.timeout())
	{
		stab_counter.stopCounter();
		possible_stab = false;
	}
#endif

	// Motion events
	if (event_clash)
	{
		event_clash = false;

		// If got here it means we aren't spinning anymore
		spin_count = 0;
		spinning = false;
		spin_counter.stopCounter();
		spin_limit_counter.stopCounter();

		// Check if we have an only an On/Off button
		if (!fxButton)
		{
			// Clash + On/Off button = lock-up
			if (onButton->pressed() && fontPresent(fontLock))
			{
				enterState(stateLockUp);
				return;
			}
		}

		// Check the axis of the pulse event
		uint8_t pulse_src = Motion.getPulseSource();
		debugMsg(DebugInfo, "Pulse source: X:%i Y:%i Z:%i",
					pulse_src & MotionPulseOnX ? (pulse_src & MotionPulseNegativeX ? -1 : 1) : 0,
					pulse_src & MotionPulseOnY ? (pulse_src & MotionPulseNegativeY ? -1 : 1) : 0,
					pulse_src & MotionPulseOnZ ? (pulse_src & MotionPulseNegativeZ ? -1 : 1) : 0);

		if (pulse_src == (MotionPulseOnX | MotionPulseNegativeX))
		{
			#if (STAB_REQUIRES == (STAB_REQUIRES_SWING | STAB_REQUIRES_CLASH))
			// Check for possible stab after swing
			// (swing in X direction + clash in X direction, in less than 500ms)
			if (possible_stab)
			{
				stab_counter.stopCounter();
				possible_stab = false;
				if (fontPresent(fontStab))
				{
					clash_counter.startTimeoutCounter(config.settings.clash_limiter);
					enterState(stateStab);
					return;
				}
			}
			#endif

			#if (STAB_REQUIRES == STAB_REQUIRES_CLASH)
			if (fontPresent(fontStab))
			{
				clash_counter.startTimeoutCounter(config.settings.clash_limiter);
				enterState(stateStab);
				return;
			}
			#endif
		}

		// Do normal clash
		if (fontPresent(fontClash))
		{
			if (clash_counter.active() && !clash_counter.timeout())
			{
				debugMsg(DebugInfo, "Clash limiter hit");
			} else {
				clash_counter.startTimeoutCounter(config.settings.clash_limiter);
				enterState(stateClash);
			}
			return;
		}
	}

	if (event_swing)
	{
		event_swing = false;

		// If the event happened only on negative X axis, then it may be a stab
		uint8_t transient_src = Motion.getTransientSource();
		debugMsg(DebugInfo, "Transient source: X:%i Y:%i Z:%i",
			transient_src & MotionTransientOnX ? (transient_src & MotionTransientNegativeX ? -1 : 1) : 0,
			transient_src & MotionTransientOnY ? (transient_src & MotionTransientNegativeY ? -1 : 1) : 0,
			transient_src & MotionTransientOnZ ? (transient_src & MotionTransientNegativeZ ? -1 : 1) : 0);

		if (transient_src == (MotionOnX | MotionNegativeX))
		{
			#if (STAB_REQUIRES == STAB_REQUIRES_SWING)
			if (fontPresent(fontStab))
			{
				clash_counter.startTimeoutCounter(config.settings.clash_limiter);
				enterState(stateStab);
				return;
			}
			#endif

			#if (STAB_REQUIRES == (STAB_REQUIRES_SWING | STAB_REQUIRES_CLASH))
			possible_stab = true;
			stab_counter.startTimeoutCounter(500);
			#endif
		}

		// Limit swings per second
		bool swing = true;
		if (swing_counter.active() && !swing_counter.timeout())
		{
			swing = false;
			debugMsg(DebugInfo, "Swing limiter hit");
		}

		if (clash_counter.active() && !clash_counter.timeout())
		{
			swing = false;
			debugMsg(DebugInfo, "Clash (swing) limiter hit");
		}

		// Check spin
		if (fontPresent(fontSpin) && swing)
		{
			if (spin_counter.active() && spin_counter.timeout())
			{
				spin_count = 0;
				spinning = false;
				spin_counter.stopCounter();
				spin_limit_counter.stopCounter();
				debugMsg(DebugInfo, "Spin condition timed out");
			}

			if (transient_src == (MotionTransientOnX | MotionTransientNegativeX))
			{
				bool spin = false;
				if (!spinning)
				{
					spin_count++;
					spin_counter.startTimeoutCounter(config.settings.spin_limiter * 4);
					debugMsg(DebugInfo, "spin_count = %lu", spin_count);

					if (spin_count == 3)
					{
						spinning = true;
						spin_count = 0;

						// Pick a font file number and stick with it
						if (current_profile.font.files[fontSpin].random)
							spin_num = getRandom(current_profile.font.files[fontSpin].min,
										 current_profile.font.files[fontSpin].max);

						spin = true;
						debugMsg(DebugInfo, "Spinning");
					}
				} else {
					if (spin_limit_counter.active())
					{
						 if (spin_limit_counter.timeout())
							spin = true;
						 else
							 debugMsg(DebugInfo, "Spin (swing) limiter hit");
					} else {
						spin = true;
					}
				}

				if (spin)
				{
					spin_limit_counter.startTimeoutCounter(config.settings.spin_limiter);
					spin_counter.startTimeoutCounter(config.settings.spin_limiter * 4);
					enterState(stateSpin);
				}
			}
		}

		// Check for swings
		if (swing && !spinning)
		{
			// Check if we have only the On/Off button
			if (!fxButton)
			{
				// Swing + On/Off button = 'force'
				if (onButton->pressed() && fontPresent(fontForce))
				{
					enterState(stateForce);
					return;
				}
			} else {
				// Swing + FX button = 'force' effect
				if (fxButton->pressed() && fontPresent(fontForce))
				{
					enterState(stateForce);
					return;
				}
			}

			// Normal swings
			if (fontPresent(fontSwing))
			{
				swing_counter.startTimeoutCounter(config.settings.swing_limiter);
				enterState(stateSwing);
			}
		}
	}
}

void PBSaber::pollStateIdleOn()
{
	handleButtonsEvents();
	handleMotionEvents();
}

void PBSaber::enterStateRetraction()
{
	// Stop music (if any). TODO maybe play a (configurable) closure music?
	music->stop();

	// Play retraction sound
	if (play(fontRetraction))
	{
		// Start blade retraction effect
		uint32_t duration = current_profile.retraction_duration;
		if (!duration)
			duration = current_sound_duration;

		blade->onRetraction(current_profile.retraction_mode, duration);

		if (current_profile.font.poly)
			volumeCounter.startTimeoutCounter(40);
	} else {
		enterState(stateOff);
	}
}

void PBSaber::pollStateRetraction()
{
	bool ready = true;

	// Wait for user to release the ON button
	PropButton* onButton = getButton(buttonOnOff);
	ready = onButton->released();

	// Wait for all "ignition" audio to end
	if (current_profile.font.poly)
	{
		float volume = hum->getVolume();
		if (volume == 0)
		{
			ready &= true;
			hum->stop();
		} else {
			ready = false;

			// Decrement the hum volume down to 0, each 40ms
			if (volumeCounter.timeout())
			{
				volumeCounter.startTimeoutCounter(40);
				float step = 1.0f / (current_sound_duration / 40);
				volume -= step;
				if (volume < 0)
					volume = 0;
				hum->setVolume(volume);
			}
		}

		if (fx.playing())
			ready = false;
	} else {
		if (monoFont->playingChained())
			ready = false;
		else
			/* Stop the chained player after the off sound has finished playing */
			monoFont->stop();
	}

	if (ready)
		enterState(stateOff);
}

void PBSaber::enterStateLock()
{
	if (play(fontLock, PlayModeLoop))
	{
		if (onEffectCallback)
		{
			bladeEffect userEffect = current_profile.lockup;
			notifyEffectToUser(userEffect);
			blade->startLockup(&userEffect);
		} else {
			blade->startLockup(&current_profile.lockup);
		}
	} else {
		enterState(stateIdleOn);
	}
}

void PBSaber::pollStateLock()
{
	PropButton* onoffButton = getButton(buttonOnOff);
	PropButton* fxButton = getButton(buttonFx);
	PropButton* button = fxButton ? fxButton : onoffButton;

	if (button->released())
	{
		if (current_profile.font.poly)
			fx.stop();
		else
			monoFont->restart();

		blade->stopLockup();
		blade->doBaseShimmer();
		enterState(stateIdleOn);
	}
}

void PBSaber::enterStateSwing()
{
	if (!play(fontSwing))
		enterState(stateIdleOn);
}

void PBSaber::pollStateSwing()
{
	enterState(stateIdleOn);
}

void PBSaber::enterStateBlaster()
{
	if (play(fontBlaster))
	{
		// Check if the blaster LED effect has to have the duration of the sound file
		uint32_t duration = current_profile.blaster.duration;
		if (!duration)
			// Modify the effect duration
			duration = current_sound_duration;

		if (onEffectCallback)
		{
			bladeEffect userEffect = current_profile.blaster;
			notifyEffectToUser(userEffect);
			blade->onBlaster(&userEffect, duration);
		} else {
			blade->onBlaster(&current_profile.blaster, duration);
		}
	} else {
		enterState(stateIdleOn);
	}
}

void PBSaber::pollStateBlaster()
{
	PropButton* onButton = getButton(buttonOnOff);
	PropButton* fxButton = getButton(buttonFx);
	ButtonEvent event;

	if (fxButton)
		event = fxButton->getEvent();
	else
		event = onButton->getEvent();

	if (event == ButtonShortPressAndRelease)
	{
		// again
		enterState(stateBlaster);
		return;
	}

	// Blasters don't get interrupted, so poll here
	if (current_profile.font.poly)
	{
		if (!fx.playing())
			enterState(stateIdleOn);
	} else {
		if (!monoFont->playingChained())
			enterState(stateIdleOn);
	}
}

void PBSaber::enterStateClash()
{
	if (play(fontClash))
	{
		// Check if the clash LED effect has to have the duration of the sound file
		uint32_t duration = current_profile.clash.duration;
		if (!duration)
			// Modify the effect duration
			duration = current_sound_duration;

		if (onEffectCallback)
		{
			bladeEffect userEffect = current_profile.clash;
			notifyEffectToUser(userEffect);
			blade->onClash(&userEffect, duration);
		} else {
			blade->onClash(&current_profile.clash, duration);
		}
	} else {
		enterState(stateIdleOn);
	}
}

void PBSaber::pollStateClash()
{
	enterState(stateIdleOn);
}

void PBSaber::enterStateSpin()
{
	if (!play(fontSpin))
		enterState(stateIdleOn);
}

void PBSaber::pollStateSpin()
{
	enterState(stateIdleOn);
}

void PBSaber::enterStateStab()
{
	if (play(fontStab))
	{
		// Check if the stab LED effect has to have the duration of the sound file
		uint32_t duration = current_profile.stab.duration;
		if (!duration)
			// Modify the effect duration
			duration = current_sound_duration;

		if (onEffectCallback)
		{
			bladeEffect userEffect = current_profile.stab;
			notifyEffectToUser(userEffect);
			blade->onStab(&userEffect, duration);
		} else {
			blade->onStab(&current_profile.stab, duration);
		}

	} else {
		enterState(stateIdleOn);
	}
}

void PBSaber::pollStateStab()
{
	enterState(stateIdleOn);
}

void PBSaber::enterStateCycleProfiles()
{
	playUtility(sndutilBeep, PlayModeBlocking);
	resetAllButtonsEvents();
}

void PBSaber::pollStateCycleProfiles()
{
	PropButton* onButton = getButton(buttonOnOff);

	ButtonEvent event = onButton->getEvent();

	if (event == ButtonShortPressAndRelease)
	{
		playUtility(sndutilBeep);

		if (loadNextProfile(&tmp_profile))
			changeProfile(&tmp_profile);
	} else if (event == ButtonLongPressed)
	{
		// Set a flag indicating to update the initial profile (in the configuration file)
		// at saber retraction.
		save_initial_profile = profile_at_ignition != tmp_profile.id;

		// Play 'confirmation' sound
		playUtility(sndutilBeep, PlayModeBlocking);
		enterState(stateOff);
	}
}

void PBSaber::enterStateMusic()
{
	if (!play(fontBackground, PlayModeLoop))
		enterState(prev_state);
}

void PBSaber::pollStateMusic()
{
	// Wait for a second On/Off button press to start ignition
	PropButton* onButton = getButton(buttonOnOff);
	ButtonEvent event = onButton->getEvent();

	if (event == ButtonShortPressAndRelease)
	{
		enterState(stateIgnition);
		return;
	} else if (event == ButtonLongPressed)
	{
		// Stop the music and go back to Off
		music->stop();
		enterState(stateOff);
		return;
	}

	// Monitor ignition on stab
	if (ignitionOnStab())
		enterState(stateIgnition);
}

void PBSaber::enterStateForce()
{
	if (!play(fontForce))
		enterState(prev_state);
}

void PBSaber::pollStateForce()
{
	// Wait for the user to release the button
	PropButton* onButton = getButton(buttonOnOff);
	PropButton* fxButton = getButton(buttonFx);

	// Using FX button?
	if (fxButton)
	{
		if (!fxButton->pressed())
			enterState(stateIdleOn);
	} else {
		// Using On/Off button
		if (!onButton->pressed())
			enterState(stateIdleOn);
	}
}

void PBSaber::changeProfile(saberProfile* new_profile)
{
	bool font_changed = false;

	WavPlayer* new_poly_player = NULL;
	WavChainPlayer* new_mono_player = NULL;

	if (new_profile->font_num != current_profile.font_num)
		font_changed = true;

	// If previous state was stateIdleOff, then we don't change anything,
	// just load the profile and play a sound.
	if (prev_state != stateIdleOff)
	{
		// We have LED shimmering and hum sound playing,
		// the idea is to progressively switch to colors and audio
		// to those of the new profile.

		// Change shimmer
		blade->setShimmerMode(&new_profile->shimmer);

		if (font_changed)
		{
			// Font has changed. Prepare the other player and start playing the hum
			getSoundFileName(tmp, &new_profile->font, fontHum);

			if (new_profile->font.poly)
			{
				if (hum == &hum1)
					new_poly_player = &hum2;
				else
					new_poly_player = &hum1;

				new_font_player = new_poly_player;
				new_poly_player->setVolume(0);
				font_changed = new_poly_player->play(tmp, PlayModeLoop);
			} else {
				if (monoFont == &monoFont1)
					new_mono_player = &monoFont2;
				else
					new_mono_player = &monoFont1;

				new_font_player = new_mono_player;
				new_mono_player->setVolume(0);
				new_mono_player->stop();
				new_mono_player->begin(tmp);
				font_changed = new_mono_player->play();
			}

			if (current_profile.font.poly)
				prev_font_player = hum;
			else
				prev_font_player = monoFont;

			if (new_profile->font.files[fontBackground].present)
			{
				// New font has background audio
				new_bkg_player = (music == &music1) ? &music2 : &music1;
				getSoundFileName(tmp, &new_profile->font, fontBackground);
				new_bkg_player->setVolume(0);
				if (new_bkg_player->play(tmp, PlayModeLoop))
				{
					background_changed = true;

					// Are we playing background for the current font?
					if (fontPresent(fontBackground))
						// We need to crossover the background tracks
						prev_bkg_player = music;
					else
						prev_bkg_player = NULL;
				}
			} else {
				// Check if there is background playing
				if (fontPresent(fontBackground))
				{
					background_changed = true;
					new_bkg_player = NULL;
					prev_bkg_player = music;
				}
			}

			// Do the audio change in about 500ms, updating every 40ms.
			if (font_changed)
			{
				new_font_target = prev_font_player->getVolume();
				new_font_cycles =  BLADE_SHIMMER_SWITCH_DURATION / 40;
				new_font_step = new_font_target / (float) new_font_cycles;
				volumeCounter.startTimeoutCounter(40);
			}

			new_font = font_changed;
		}
	}

	// Update the current profile
	memcpy(&current_profile, new_profile, sizeof(saberProfile));

	if (prev_state == stateIdleOff)
	{
		// Make sure the volume of the players we want to use is set to 1
		if (current_profile.font.poly)
			hum->setVolume(1.0f);
		else
			monoFont->setVolume(1.0f);

		music->setVolume(1.0f);

		// Play the profile name (if any)
		if (font_changed)
			play(fontName);
	}
}

void PBSaber::enterStateNextProfile()
{
	if (!loadNextProfile(&tmp_profile))
	{
		enterState(prev_state);
	} else {
		if (prev_state == stateIdleOff)
			playUtility(sndutilBeep, PlayModeBlocking);
		else
			playUtility(sndutilBeep);

		changeProfile(&tmp_profile);
	}
}

void PBSaber::pollStateNextProfile()
{
	bool done = true;

	// Wait for the audio switching to end (if any)
	if (new_font && new_font_cycles)
	{
		if (!volumeCounter.timeout())
			return;

		volumeCounter.startTimeoutCounter(40);
		prev_font_player->setVolume(prev_font_player->getVolume() - new_font_step);
		new_font_player->setVolume(new_font_player->getVolume() + new_font_step);

		// Do the same for the background
		if (background_changed)
		{
			if (prev_bkg_player)
				prev_bkg_player->setVolume(prev_bkg_player->getVolume() - new_font_step);

			if (new_bkg_player)
				new_bkg_player->setVolume(new_bkg_player->getVolume() + new_font_step);
		}

		new_font_cycles--;
		if (!new_font_cycles)
		{
			// Stop old player
			prev_font_player->stop();

			// Replace hum player
			if (current_profile.font.poly)
			{
				hum = (WavPlayer*) new_font_player;
				hum->setVolume(new_font_target);
			} else {
				monoFont = (WavChainPlayer*) new_font_player;
				monoFont->setVolume(new_font_target);
			}

			// Replace background player
			if (background_changed)
			{
				if (prev_bkg_player)
				{
					prev_bkg_player->setVolume(0);
					prev_bkg_player->stop();
				}

				if (new_bkg_player)
				{
					new_bkg_player->setVolume(new_font_target);
					music = new_bkg_player;
				} else {
					music = prev_bkg_player;
				}
				background_changed = false;
			}

			// Play the font name (if any)
			play(fontName);
		} else done = false;
	}

	// Wait for the blade to go idle
	if (blade->getState() != bladeStateIdle && blade->getState() != bladeStateOff)
		done = false;

	// Wait for the user to release the button(s)
	PropButton* onButton = getButton(buttonOnOff);
	PropButton* fxButton = getButton(buttonFx);

	if (!onButton->released())
		done = false;

	if (fxButton && !fxButton->released())
		done = false;

	if (done)
	{
		// Set a flag indicating to update the initial profile (in the configuration file)
		// at saber retraction.
		if (prev_state != stateIdleOff)
			save_initial_profile = profile_at_ignition != tmp_profile.id;

		// Go back to the previous state
		enterState(prev_state);
	}
}

void PBSaber::enterStatePrevProfile()
{
	if (!loadPrevProfile(&tmp_profile))
	{
		enterState(prev_state);
	} else {
		if (prev_state == stateIdleOff)
			playUtility(sndutilBeep, PlayModeBlocking);
		else
			playUtility(sndutilBeep);

		changeProfile(&tmp_profile);
	}
}

void PBSaber::pollStatePrevProfile()
{
	// Same as...
	pollStateNextProfile();
}

bool PBSaber::ignitionOnStab()
{
	if (current_profile.ignition_on_stab)
	{
		if (event_swing)
		{
			// If event happened only on negative X axis, then it's a stab
			uint8_t transient_src = Motion.getTransientSource();
			if (transient_src == (MotionOnX | MotionNegativeX))
			{
				// If On/Off button is pressed
				if (getButton(buttonOnOff)->pressed())
					return true;
			}
		}
	}

	return false;
}

void PBSaber::playUtility(saberUtilitySound snd, PlayMode mode)
{
	if (!strlen(config.settings.sound_utils))
	{
		debugMsg(DebugWarning, "Utility sounds folder not configured");
		return;
	}

	sprintf(tmp, "%s\\", config.settings.sound_utils);

	switch (snd)
	{
		case sndutilBeep:
			strcat(tmp, "beep.wav");
			break;
	}

	// Use FX player
	if (fx.play(tmp, mode))
	{
		debugMsg(DebugInfo, "Playing utility sound %s", tmp);
	} else {
		debugMsg(DebugInfo, "Error playing %s", tmp);
	}
}

const char* PBSaber::getStateName(saberStateId state)
{
	switch (state)
	{
		case stateOff: 				return "OFF";
		case stateIdleOff: 			return "IDLE OFF";
		case stateIdleOn: 			return "IDLE ON";
		case stateIgnition: 		return "IGNITION";
		case stateRetraction: 		return "RETRACTION";
		case stateBlaster: 			return "BLASTER";
		case stateLockUp: 			return "LOCKUP";
		case stateClash: 			return "CLASH";
		case stateSwing: 			return "SWING";
		case stateSpin: 			return "SPIN";
		case stateStab: 			return "STAB";
		case stateMusic: 			return "MUSIC";
		case stateForce: 			return "FORCE";
		case stateCycleProfiles: 	return "CYCLE PROFILES";
		case stateNextProfile: 		return "NEXT PROFILE";
		case statePrevProfile: 		return "PREV PROFILE";
		default: break;
	}

	return "UNKNOWN";
}

void PBSaber::motionPulses()
{
	event_clash = true;
}

void PBSaber::motionTransients()
{
	event_swing = true;
}

#ifdef FROM_ECLIPSE
PBSaber saber;

void setup()
{
	saber.begin();
}

void loop()
{
	saber.loop();
}
#endif
