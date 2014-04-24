#include "CX_EntryPoint.h"

/*
This example shows how to do a number of things with the CX_SoundBufferPlayer
and CX_SoundBuffers. CX_SoundBufferPlayer can only play a single CX_SoundBuffer
at a time, but a single CX_SoundBuffer can be procedurally generated out of
multiple different sound files, as will be shown.

A compound sound can be generated from several different sound buffers, with each
sound starting at a known offset. By combining the sounds into a single audio 
stream, sounds are essentially guaranteed to start at the correct time (relative
to the rest of the sounds).
*/

CX_SoundBufferPlayer player;
CX_SoundBuffer cow;
CX_SoundBuffer duck;
CX_SoundBuffer compoundSound;

void runExperiment (void) {

	//CX_SoundBufferPlayer uses RtAudio (http://www.music.mcgill.ca/~gary/rtaudio/) for playing sounds.
	//Because of this, configuring the CX_SoundBufferPlayer is more or less directly configuring
	//RtAudio. Most of the configuration settings in the CX_SoundBufferPlayer::Configuration structure
	//can be left at default values and things will generally work out. However, it is usually a good
	//idea to set a number of the values. In this example, most of the major ones are manually set with 
	//comments describing a little bit about them.
	CX_SoundBufferPlayer::Configuration config;

	config.api = RtAudio::Api::WINDOWS_DS; //Use Windows Direct Sound (more likely to do work at all than ASIO).
		//However, ASIO is preferred. If your sound card supports ASIO, use it.
		//Of course, if you are not on Windows, use one of the APIs for your OS. You can see
		//which APIs are available for your OS by using:
		//cout << CX_SoundStream::convertApisToString( CX_SoundStream::getCompiledApis() ) << endl;

	config.outputDeviceId = -1; //Using -1 means to use the default output device. You can also use
		//the constant CX_SOUND_STREAM_USE_DEFAULT_DEVICE, which evaluates to -1. If you would like to
		//see which output devices are available on your system, use
		//cout << CX_SoundStream::listDevices(RtAudio::Api::UNSPECIFIED) << endl;
		//where UNSPECIFIED is replaced with the API you are using.

	config.outputChannels = 2; //We want at least stereo output for this example. CX does not *gracefully*
		//support channel configurations past stereo, but they are supported.
	
	config.sampleRate = 48000; //Note that this sample rate is only requested: it may not be supported by your
		//audio hardware. In that case, the closest sample rate greater than the requested rate will be chosen,
		//if available. If not, the closest rate below will be chosen.

	config.bufferSize = 4096; //Bigger buffers mean fewer audio glitches and more latency.
	config.streamOptions.numberOfBuffers = 4; //More buffers means fewer audio glitches and more latency.
		//Not all APIs allow you to change the number of buffers, in which case this setting will have no effect.

	if (!player.setup(config)) { //Use the configuration settings to set up the CX_SoundBufferPlayer.
		cout << "There was an error setting up the sound player." << endl;
	}

	config = player.getConfiguration(); //By doing this, we can check to see what sample rate was actually chosen.
	cout << "Actual sample rate: " << config.sampleRate << endl;

	//Now we're going to load up a couple of sounds. These files should be present in ./bin/data
	//(relative to the visual studio project directory). They should come with this example.
	cow.loadFile("Cow.wav");
	duck.loadFile("Duck.wav");

	//Given the way CX_SoundBufferPlayer works, the CX_SoundBufferss given to it must be at the same
	//sample rate that the hardware is currently using. If you don't resample before giving the sound
	//to the player, it will do it for you, but with a warning. By doing it here, we avoid the warning.
	cow.resample( config.sampleRate );
	duck.resample( config.sampleRate );


	//You can use a CX_SoundBufferPlayer to play CX_SoundBuffers (duh).
	//If you want to just play single sounds (like this example), you are possibly better off
	//just using ofSoundPlayer. More interesting uses of CX_SoundBuffers can be found below.
	cout << "Playing the duck." << endl;
	player.setSoundBuffer( &duck );
	player.play();
	while (player.isPlaying())
		;

	cout << "Plying a fast duck (2x speed; not pitch corrected)" << endl;
	duck.multiplySpeed(2);
	//player.setSoundBuffer( &duck ); //This does not need to be called because the player already has a pointer to the duck sound.
	player.play();
	while (player.isPlaying())
		;

	//Slow the duck down again
	duck.multiplySpeed(.5);
	
	//Here a compound sound composed of a cow followed by a duck (after 6 seconds).
	//If you want to present several auditory stimuli one after the other
	//with known offsets, this is the way to do so. By combining the sounds
	//into a single audio stream, sounds are essentially guaranteed to come
	//at the right offset following an earlier sound.
	cout << "Playing compound sound: cow then duck." << endl;
	compoundSound.addSound(cow, 0);
	compoundSound.addSound(duck, CX_Seconds(6));

	player.setSoundBuffer( &compoundSound );

	player.play();
	while (player.isPlaying())
		;

	//A more complex example:
	//The cow and duck files are monophonic. Here, setChannelCount is used to
	//extend the sounds to 2 channels (i.e. stereo), then multiplyAmplitudeBy() is
	//used to mute one of the channels. Finally, the compound sound has the
	//panned duck sound added twice, right after each other.
	cout << "Playing cow panned right and duck panned left (duck played twice)." << endl;

	CX_SoundBuffer rightCow = cow;
	rightCow.setChannelCount(2);
	rightCow.multiplyAmplitudeBy(0, 0); //Mute channel 0

	CX_SoundBuffer leftDuck = duck;
	leftDuck.setChannelCount(2);
	leftDuck.multiplyAmplitudeBy(0, 1); //Mute channel 1

	compoundSound = rightCow; //Set the compound sound equal to the rightCow sound (a copy operation).
	compoundSound.addSound(leftDuck, 0);
	compoundSound.addSound(leftDuck, CX_Seconds(4)); //Because addSound() takes a copy of a CX_SoundBuffer,
		//you can add the same sound to another sound buffer multiple times (you can even add a sound
		//to itself).
	
	//Notice at no time is a local variable given to setSoundBuffer(). setSoundBuffer()
	//takes the address of a CX_SoundBuffer and does not copy that buffer, it only uses the
	//address of that buffer. Because of this, if you give it the address of a local variable 
	//that falls out of scope, you will get errors when you try to play that sound later.
	player.setSoundBuffer( &compoundSound );
	player.play();
	while (player.isPlaying())
		;

}