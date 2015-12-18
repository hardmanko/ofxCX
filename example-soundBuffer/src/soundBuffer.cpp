#include "CX.h"

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

//These sound-related objects will be used in the example.
CX_SoundStream soundStream;
CX_SoundBufferPlayer player;
CX_SoundBuffer cow;
CX_SoundBuffer duck;
CX_SoundBuffer compoundSound;

void runExperiment (void) {

	//In order to play sounds, we need to configure a CX_SoundStream first. CX_SoundStream uses
	//RtAudio (http://www.music.mcgill.ca/~gary/rtaudio/) for playing sounds. Because of this, 
	//configuring the CX_SoundStream is more or less directly configuring RtAudio. Most of the 
	//configuration settings in the CX_SoundStream::Configuration structure can be left at default 
	//values and things will generally work out. However, it is usually a good idea to set a 
	//number of the values. In this example, most of the major ones are manually set with comments 
	//describing a little bit about them. See the documentation for CX_SoundStream::Configuration
	//for more information.
	CX_SoundStream::Configuration config;

	config.api = RtAudio::Api::WINDOWS_DS; //Use Windows Direct Sound (more likely to work at all than ASIO).
		//However, ASIO is preferred due to lower latency. If your sound card supports ASIO, use it.
		//Of course, if you are not on Windows, use one of the APIs for your OS. You can see
		//which APIs are available for your OS by using:
		//cout << CX_SoundStream::convertApisToString( CX_SoundStream::getCompiledApis() ) << endl;
	
	config.outputDeviceId = -1; //Using -1 means to use the default output device. If you would like to
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

	if (!soundStream.setup(config)) {
		cout << "There was an error setting up the sound stream." << endl;
	}

	config = soundStream.getConfiguration(); //By doing this, we can check to see what sample rate was actually used.
	cout << "Actual sample rate: " << config.sampleRate << endl;

	//Now that the sound stream is set up, we give a pointer to it to the sound player.
	player.setup(&soundStream);

	//At this point, assuming that all went well, the sound player is all set up and we just need to
	//give it something to play, so we will load up some sound files. These files should be present in 
	//./bin/data (relative to the project directory). They should come with this example.
	cow.loadFile("Cow.wav"); //cow and duck are CX_SoundBuffers that were created at the start of this example.
	duck.loadFile("Duck.wav");

	//Given the way CX_SoundBufferPlayer works, the CX_SoundBuffers given to it must be at the same
	//sample rate that the hardware is currently using. If you don't resample the sounds before giving 
	//them to the player, it will do it for you, but with a warning. By doing it here, we avoid the warning.
	cow.resample( config.sampleRate );
	duck.resample( config.sampleRate );


	//Now we can use our CX_SoundBufferPlayer to play the sounds we loaded. To do this,
	//we need to set the sound we want to play as the active sound for the player.
	player.setSoundBuffer( &duck );

	//Now we press play.
	cout << "Playing the duck." << endl;
	player.play();
	while (player.isPlaying()) //Wait for the duck to stop quacking.
		;


	//We can do some things to the sounds in CX_SoundBuffers, like change their speed:
	duck.multiplySpeed(2);

	//This does not need to be called again here because the player already has a pointer to the duck sound.
	//player.setSoundBuffer( &duck ); 

	cout << "Playing a fast duck (2x speed; not pitch corrected)" << endl;
	player.play();
	while (player.isPlaying())
		;


	//Slow the duck down again
	duck.multiplySpeed(.5);
	
	//Here we make a compound sound composed of a cow followed by a duck (after 6 seconds).
	//If you want to present several auditory stimuli one after the other
	//with known offsets, this is the way to do so. By combining the sounds
	//into a single audio stream, sounds are essentially guaranteed to come
	//at the right offset following an earlier sound.
	compoundSound.addSound(cow, 0); //Add the cow at an offset of 0 milliseconds from the start.
	compoundSound.addSound(duck, CX_Seconds(6)); //Add the duck 6 seconds after the start.

	player.setSoundBuffer( &compoundSound );

	cout << "Playing compound sound: cow then duck." << endl;
	player.play();
	while (player.isPlaying())
		;


	//A more complex example:
	//The cow and duck files are monophonic. Here, setChannelCount is used to
	//extend the sounds to 2 channels (i.e. stereo), then multiplyAmplitudeBy() is
	//used to mute one of the channels, which has the effect of panning the sounds. 
	//Finally, the compound sound has the panned duck sound added in twice.

	CX_SoundBuffer rightCow = cow; //Copy the cow, because it will be modified.
	rightCow.setChannelCount(2); //Make the cow stereo from mono. This copies the data from one channel to both new channels.
	rightCow.multiplyAmplitudeBy(0, 0); //Mute channel 0

	CX_SoundBuffer leftDuck = duck;
	leftDuck.setChannelCount(2);
	leftDuck.multiplyAmplitudeBy(0, 1); //Mute channel 1

	compoundSound = rightCow; //Set the compound sound equal to the rightCow sound (a copy operation).
	compoundSound.addSound(leftDuck, 0); //Bring on the ducks!
	compoundSound.addSound(leftDuck, CX_Seconds(4)); //Because addSound() takes a copy of a CX_SoundBuffer,
		//you can add the same sound to another sound buffer multiple times (you can even add a sound
		//to itself!).
	
	cout << "Playing cow panned right and duck panned left (duck played twice)." << endl;
	player.play();
	while (player.isPlaying())
		;

}