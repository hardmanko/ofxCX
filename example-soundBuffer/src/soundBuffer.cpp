#include "CX.h"

/* This example shows how to do play back sound files with CX_SoundBufferPlayer.

Sound files are loaded and stored by CX_SoundBuffers.
Once the files are loaded, CX_SoundBufferPlayer can play the corresponding CX_SoundBuffers.

CX_SoundBufferPlayer can only play a single CX_SoundBuffer at a time, but any 
number of different CX_SoundBuffers can be combined together into a compound 
sound that is then played.
When sounds are combined into a single buffer that is played back as one continuous 
sound, the relative start times of the individual sounds are guaranteed.

When running this example, look at the console output to see which sounds are supposed to be playing.
*/

//These sound-related objects will be used in the example.
CX_SoundStream soundStream;
CX_SoundBufferPlayer player;
CX_SoundBuffer cow;
CX_SoundBuffer duck;
CX_SoundBuffer compoundSound;

void runExperiment (void) {

	// In order to play sounds, we need to configure a CX_SoundStream first. CX_SoundStream uses
	// RtAudio (http://www.music.mcgill.ca/~gary/rtaudio/) for playing sounds. Because of this, 
	// configuring the CX_SoundStream is more or less directly configuring RtAudio. Most of the 
	// configuration settings in the CX_SoundStream::Configuration structure can be left at default 
	// values and things will generally work out. However, it is usually a good idea to set a 
	// number of the values. In this example, most of the major ones are manually set with comments 
	// describing a little bit about them. See the documentation for CX_SoundStream::Configuration
	// for more information.
	CX_SoundStream::Configuration config;

	// The api affects how sound data is transferred between your program and the sound hardware.
	config.api = RtAudio::Api::WINDOWS_WASAPI; // Best choice for reasonably modern Windows: Low latency support for most hardware.
	//config.api = RtAudio::Api::WINDOWS_DS; // Fallback in case WASAPI is unsupported: Higher latency but broad hardware support.
	//config.api = RtAudio::Api::WINDOWS_ASIO; // If you have hardware with specialized ASIO drivers, ASIO is probably the best.
	//
	// If you are not on Windows, use one of the APIs for your OS. 
	// You can see which APIs are available on your system by using:
	//cout << CX_SoundStream::convertApisToString( CX_SoundStream::getCompiledApis() ) << endl;
	
	
	config.outputDeviceId = -1; // Using -1 means to use the default output device (which is the default). 
		// See which output devices are available on your system with:
		//cout << CX_SoundStream::listDevices(config.api) << endl;

	config.outputChannels = 2; // Choose stereo output. CX does not *gracefully*
		// support channel configurations past stereo, but they are supported.
	
	config.sampleRate = 48000; // Requested sample rate for the audio samples, which may not be supported. 
							   // If unsupported, a nearby sample rate will be chosen automatically.

	config.bufferSize = 4096; // Larger buffers increase latency, but a minimum buffer size is needed to prevent audio glitches.

	// Try to set up the sound stream with that configuration.
	if (!soundStream.setup(config)) {
		cout << "There was an error setting up the sound stream." << endl;
	}

	// Check to see what sample rate was actually chosen.
	config = soundStream.getConfiguration();
	cout << "Actual sample rate: " << config.sampleRate << endl;


	// Now that the sound stream is set up, we give a pointer to it to the sound player so that the
	// sound player knows to use that sound stream for output.
	if (!player.setup(&soundStream)) {
		cout << "There was an error setting up the sound player." << endl;
	}


	// At this point, if all went well, the sound player is all set up and we just need to
	// give it something to play, so we will load up some sound files. These files should be present in 
	// ./bin/data (relative to the project directory). Those sound files should come with this example.
	// cow and duck are CX_SoundBuffers that were created at the start of this example.
	cow.loadFile("Cow.wav"); 
	duck.loadFile("Duck.wav");

	// Given the way CX_SoundBufferPlayer works, the CX_SoundBuffers given to it must be at the same
	// sample rate that the sound stream is currently using. If you don't resample the sounds before giving 
	// them to the player, it will do it for you, but with a warning. By doing it here, we avoid the warning.
	cow.resample(config.sampleRate);
	duck.resample(config.sampleRate);


	// To play a loaded sound buffer, set it as the active sound buffer for the player
	// by passing a pointer to the buffer by putting "&" in front of the buffer name.
	player.setSoundBuffer( &duck );

	// Now press play.
	cout << "Playing the duck." << endl;
	player.play();
	while (player.isPlaying()) // Wait for the duck to stop quacking.
		;


	// We can do some things to the sounds in CX_SoundBuffers, like change their speed:
	CX_SoundBuffer fastDuck = duck; // Copy the duck before modifying it to preserve the original.
	fastDuck.multiplySpeed(2);

	player.setSoundBuffer( &fastDuck ); 

	cout << "Playing a fast duck (2x speed; not pitch corrected)" << endl;
	player.play();
	while (player.isPlaying())
		;

	

	// Here we make a compound sound composed of multiple sound buffers.
	// If you want to present several auditory stimuli one after the other
	// with known offsets, this is a good way to do so.

	compoundSound.addSound(cow, 0); // Add the cow at an offset of 0 milliseconds from the start.

	compoundSound.addSound(duck, CX_Seconds(6)); // Add the duck 6 seconds after the start.

	compoundSound.addSound(fastDuck, CX_Seconds(2)); // Also add the fast duck at 2 seconds.

	player.setSoundBuffer( &compoundSound );

	cout << "Playing compound sound: cow overlapped with fast duck, followed by normal duck." << endl;
	player.play();
	while (player.isPlaying())
		;


	// A more complex example:
	// The cow and duck files are monophonic. Here, setChannelCount is used to
	// extend the sounds to 2 channels (i.e. stereo), then multiplyAmplitudeBy() is
	// used to mute one of the channels, which has the effect of panning the sounds. 

	CX_SoundBuffer rightCow = cow;
	rightCow.setChannelCount(2); // Convert the cow from mono to stereo. This copies the data from one channel to both new channels.
	rightCow.multiplyAmplitudeBy(0, 0); // Mute channel 0 (left)

	CX_SoundBuffer leftDuck = duck;
	leftDuck.setChannelCount(2);
	leftDuck.multiplyAmplitudeBy(0, 1); // Mute channel 1 (right)

	compoundSound = rightCow; // Set the compound sound to be a copy of the rightCow sound.

	// Bring on the ducks!
	compoundSound.addSound(leftDuck, CX_Seconds(1)); 
	compoundSound.addSound(leftDuck, CX_Seconds(4));
	// addSound() takes a copy of a CX_SoundBuffer, which means that you can add the same 
	// sound to another sound buffer multiple times. You can even add a sound to itself!
	
	cout << "Playing cow panned right and duck panned left (duck played twice)." << endl;
	player.play();
	while (player.isPlaying())
		;

}