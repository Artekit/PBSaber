# PBSaber

PBSaber turns your [PropBoard](https://www.artekit.eu/products/devboards/propboard) into a full-featured lightsaber sound/light/motion board. It allows you to use the PropBoard as a lightsaber soundboard without writing a single line of code, giving you access to the *full power* of the PropBoard even if you don't know how to program. It requires just minimum soldering skills and editing a configuration file.

Click in the following picture for a demo video.

<a href="http://www.youtube.com/watch?feature=player_embedded&v=J88ACAQPcTY
" target="_blank"><img src="https://www.artekit.eu/resources/doc/propboard-pbsaber/youtube_thumb.png" 
alt="PropBoard: PBSaber" width="640" height="360" border="10" /></a>

## Features (v1.0.0)

* No coding skills required: you can create and customize your lightsaber even if you don't know how to program the PropBoard.
* You can upgrade the software, anytime, with the latest version.
* Very easy to use and install.
* Fully configurable through a user-friendly, human-readable configuration file in the SD.
* Profile-based:
	* the entire lightsaber behavior is controlled by user-defined profiles.
	* in each profile you can customize the colors and modes for the shimmering, clash, blaster, stab and lock-up effects, ignition and retraction type and duration, and the sound font to use.
	* you can select profiles through a button combination.
	* it remembers the last used profile.
	* there are no limits on how many profiles you can define.
* Audio:
	* supports WAV files, both monophonic and polyphonic fonts.
	* real-time mixing.
	* latency from event detection to audio output: typical ~4.5ms (tested at 22050fs), on both mono and polyphonic fonts, with or without background music.
	* supported sampling frequencies (@ 16 bits per sample): 22050, 32000, 44100, 48000, 96000.
	* mono and stereo.
	* gapless, clickless playback on both mono and poly fonts.
	* it switches fonts on-the-go, from mono to poly and back based on the font the profile it's using.
	* unlimited font banks.
	* unlimited font slots per sound type (for example unlimited swings slots).
	* supports background music.
* Hardware support:
	* supports 3 high-brightness LEDs up to 1A each.
	* supports addressable LEDs (a.k.a Neopixels): APA102 or APA102C, WS2812 or WS2812B and SK6812RGBW RGBW LED.
	* single or dual momentary button support.
	* Low-power mode.
* Feedback-driven project: have a feature request? you want something modified? Start a discussion in our [forum](https://forum.artekit.eu/c/propboard) or open a *Github issue*.
* 100% open source.

## Firmware installation and updates

1. Install the Arduino IDE and the PropBoard support by following [these instructions](https://www.artekit.eu/doc/guides/propboard-manual#arduino-ide-installation).
2. Grab the latest PBSaber release by clicking on this link.
3. Unzip the file anywhere in your computer and double-click on the *PBSaber.ino* file.
4. [Connect your PropBoard](https://www.artekit.eu/doc/guides/propboard-manual#connecting-the-usb-cable) to your PC/MAC using an USB cable.
5. Select the serial port for the PropBoard in the Arduino IDE and click the Upload button.
2. For software updates, repeat from step 2.

## Documentation

* [PBSaber manual](https://www.artekit.eu/doc/propboard-pbsaber)
* [PropBoard manual](https://www.artekit.eu/doc/propboard-manual)
* [PropBoard product page](https://www.artekit.eu/products/devboards/propboard)