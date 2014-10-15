Audio Input and Output {#audioIO}
======================

Audio input and output in CX is based on a number of classes. The two most important are \ref CX::CX_SoundStream "CX_SoundStream" and \ref CX::CX_SoundBuffer "CX_SoundBuffer". 
Additionally, \ref CX::CX_SoundBufferPlayer "CX_SoundBufferPlayer" and \ref CX::CX_SoundBufferRecorder "CX_SoundBufferRecorder" combine together a CX_SoundStream and a CX_SoundBuffer to play back or record audio, respectively.
Finally, for people wanting to synthesize audio in real time (or ahead of time), the CX::Synth namespace provides a multitude of ways to synthesize audio.
We will go through these components in a practical order.


Setting up the CX_SoundStream for Playback
------------------------------------------

Because the CX_SoundStream is what actually does audio input and output, in order to get any sounds out of a CX program, you must configure a CX_SoundStream for use. This requires that a \ref CX::CX_SoundStream::Configuration "CX_SoundStream::Configuration" struct be filled out with the desired settings and given to \ref CX::CX_SoundStream::setup() "CX_SoundStream::setup()" as the argument.
There are several configuration options for the CX_SoundStream, but, if the gods smile on you today, you will only need one, which is the number of output channels. If you try this and the gods seem to be frowning, check out the Troubleshooting Audio Problems section below.
We will use stereo output, so 2 output channels.

\code{.cpp}
CX_SoundStream::Configuration ssConfig;
ssConfig.outputChannels = 2; //Stereo output
//ssConfig.api = RtAudio::Api::WINDOWS_DS; //The most likely thing you will need to change is the low-level audio API.

//Create the CX_SoundStream and set it up with the configuration.
CX_SoundStream soundStream;
soundStream.setup(ssConfig);

//Check for any error messages.
Log.flush();
\endcode

If there were any errors during setup of the sound stream, they will be logged. Check the console for any messages. You can also check if the return value of `setup()` or call \ref CX::CX_SoundStream::isStreamRunning() "CX_SoundStream::isStreamRunning()" to see if setup was successful.

Playback
--------

Now that we have a CX_SoundStream set up, next next thing we need to do in order to play the contents of the sound file is to load the file
into CX. This is done by creating a CX_SoundBuffer and then loading a sound file into the sound buffer, as follows.

\code{.cpp}
CX_SoundBuffer soundBuffer;
soundBuffer.loadFile("sound_file.wav");
\endcode

If there wasn't an error loading the file, `soundBuffer` now contains the contents of the sound file in a format that can be played by CX. Once you have a sound file loaded into a CX_SoundBuffer, there are a number of things you can do with it. 
You can remove leading silence with CX_SoundBuffer::stripLeadingSilence() or add silence to the beginning or end with CX_SoundBuffer::addSilence().
You can delete part of the sound, starting from the beginning or end, with CX_SoundBuffer::deleteAmount().
You can reverse the order of the samples, so as to be able to play the sound backwards with CX_SoundBuffer::reverse().
These are just some examples. See the documentation for \ref CX::CX_SoundBuffer "CX_SoundBuffer" and the `soundBuffer` example for more things you can do with it.

Now that you have CX_SoundBuffer with sound data loaded into it, you can play it back using a \ref CX::CX_SoundBufferPlayer "CX_SoundBufferPlayer". Before you can use a CX_SoundBufferPlayer, you have to configure it with \ref CX::CX_SoundBufferPlayer::setup() "CX_SoundBufferPlayer::setup()". `setup()` takes either a structure holding configuration options for the CX_SoundStream that will be used by the CX_SoundBufferPlayer or a pointer to a CX_SoundStream that has already been set up. We will use the CX_SoundStream called `soundStream` that we configured in the previous section.

\code{.cpp}
CX_SoundBufferPlayer player;
player.setup(&soundStream);
\endcode

Now that we have a configured CX_SoundBufferPlayer, we just need to give it a CX_SoundBuffer to play by using \ref CX::CX_SoundBufferPlayer::setSoundBuffer() "CX_SoundBufferPlayer::setSoundBuffer()" and play the sound.

\code{.cpp}
player.setSoundBuffer(&soundBuffer);

player.play();

//Wait for it to finish playing.
while (player.isPlaying())
	;
\endcode

Because playback does not happen in the main thread, we wait in the main thread until playback is complete before going on.

Playing Multiple Sounds Simultaneously
--------------------------------------

A CX_SoundBufferPlayer can have a single CX_SoundBuffer assigned to it as the active sound buffer. This means that you cannot play more than one sound at once with a CX_SoundBufferPlayer. 
This limitation is by design, but also by design there are ways to play multiple sounds at once. The preferred way involves merging together multiple CX_SoundBuffers using \ref CX::CX_SoundBuffer::addSound() "CX_SoundBuffer::addSound()". What this does is take a CX_SoundBuffer and add it to an another CX_SoundBuffer at a given offset. This guarantees that the two sounds will be played at the correct time relative to one another, because there is no additional statup latency when the second sound starts playing. For example:

\code{.cpp}
CX_SoundBuffer otherBuffer;
otherBuffer.loadFile("other_sound_file.wav");

CX_SoundBuffer combinedBuffer = soundBuffer;

combinedBuffer.addSound(otherBuffer, 500); //Add the second sound to the first, 
	//with the second starting 500 ms after the first.
	
player.setSoundBuffer(&combinedBuffer);
player.play();
while(player.isPlaying())
	;
\endcode

Another way to play multiple sounds at once is to create multiple CX_SoundBufferPlayers, all of which use the same CX_SoundStream. Then you can assign different CX_SoundBuffers to each player and call CX_SoundBufferPlayer::play() whenever you want to play the specific sound.

\code{.cpp}
CX_SoundBufferPlayer player2;
player2.setup(&soundStream);
player2.setSoundBuffer(&otherBuffer);

player.play();
Clock.sleep(500);
player2.play();
while (player.isPlaying() || player2.isPlaying())
	;
\endcode

You can also put multiple CX_SoundBuffers and CX_SoundBufferPlayers into C++ standard library containers, like `std::vector`. However, I must again stress that using \ref CX::CX_SoundBuffer::addSound() "CX_SoundBuffer::addSound()" is a better way to do things because it provides 100% predictable relative onset times of sounds (unless there are glitches in audio playback, but that's a different serious problem).

Recording Audio
---------------

To record audio, you can use a \ref CX::CX_SoundBufferRecorder "CX_SoundBufferRecorder". You set it up with a CX_SoundStream, just like CX_SoundBufferPlayer. The only difference is that for recording, we need input channels instead of output channels. We will stop the currenly
running CX_SoundStream and reconfigure it to also have 1 input channel. We then set up the CX_SoundBufferRecorder using `soundStream`, create a new CX_SoundBuffer for it to record into, and set that buffer to be recorded to.

\code{.cpp}
soundStream.stop();
ssConfig.inputChannels = 1; //Most microphones are mono.
soundStream.setup(ssConfig);

CX_SoundBufferRecorder recorder;
recorder.setup(&soundStream);

CX_SoundBuffer recordedSound;
recorder.setSoundBuffer(&recordedSound);

Log.flush(); //As usual, let's check for errors during setup.
\endcode

Now that we have set up the recorder, we will record for 5 seconds, then play back what we have recorded.

\code{.cpp}
cout << "Starting to record." << endl;
recorder.start();
Clock.sleep(CX_Seconds(5));
recorder.stop();
cout << "Done recording." << endl;
\endcode

We sleep the main thread for 5 seconds while the recording takes place in a secondary thread. The implication of the use of secondary threads for recording is that you can start a recording, do whatever you feel like in the main thread -- draw visual stimuli, collect responses, etc. -- all while the recording keeps happening in a secondary thread.

Once our recording time is complete, we will set a CX_SoundBufferPlayer to play the recorded sound in the normal way.

\code{.cpp}
player.setSoundBuffer(&recordedSound);
player.play();
while (player.isPlaying())
	;
\endcode

Be careful that you are not recording to a sound buffer at the same time you are playing it back, because who knows what might happen (it would probably be fine, actually). To be careful, you can "detach" a CX_SoundBuffer from either a player or a recorder by calling, e.g., CX_SoundBufferPlayer::setSoundBuffer() with `nullptr` as the argument.

\code{.cpp}
recorder.setSoundBuffer(nullptr); //Make it so that no buffers are associated with the recorder.
\endcode


Synthesizing Audio
------------------

You can synthesize audio in real time, or ahead of time, using the classes in the CX::Synth namespace. See also the modularSynth example.

Direct Control of Audio IO
--------------------------

If you want to be really fancy, you can directly read and modify the audio data that a CX_SoundStream is sending or receiving. This is a relatively advanced operation and is unlikely to be needed in very many cases, but it's there if need be.

In order to directly access the data that a CX_SoundStream is transmitting, you need to create a class containing a function that will be called every time the CX_SoundStream needs to send more data to the sound card. For example, you could have a class like this that creates a sine wave.

\code{.cpp}
class ExampleOutputClass {
public:
	void callbackFunction(CX_SoundStream::OutputEventArgs& args) {
		static float wavePosition = 0;

		float sampleRate = args.instance->getConfiguration().sampleRate;
		float frequency = 524;
		float positionChangePerSampleFrame = 2 * PI * frequency / sampleRate;

		for (unsigned int sampleFrame = 0; sampleFrame < args.bufferSize; sampleFrame++) {
			for (unsigned int channel = 0; channel < args.outputChannels; channel++) {
				args.outputBuffer[(sampleFrame * args.outputChannels) + channel] = sin(wavePosition);
			}
			
			wavePosition += positionChangePerSampleFrame;
			if (wavePosition >= 2 * PI) {
				wavePosition = 0;
			}
		}
	}
};
\endcode

Of course, if you really wanted to create sine waves in real time, use CX::Synth::Oscillator and CX::Synth::StreamOutput, but for the sake of exaple, lets use this class.
Then create an instance of your class and add it as a listener to the `outputEvent` of a CX_SoundStream.

\code{.cpp}
	CX_SoundStream soundStream; //Assume this has been or will be set up elsewhere.
	ExampleOutputClass sineOut;
	
	//For event soundStream.outputEvent, targeting class instance sineOut, call callbackFunction of that class instance.
	ofAddListener(soundStream.outputEvent, &sineOut, &ExampleOutputClass::callbackFunction);
\endcode

From now on, whenever `soundStream` needs more output data, `sineOut.callbackFunction` will be called automatically. The data that you put into the output buffer must be `float` and bounded between -1 and 1, inclusive. You can remove a listener to an event with ofRemoveListener. More information about the events used by openFrameworks can be found here: http://www.openframeworks.cc/documentation/events/ofEvent.html.

Directly accessing input data works in a very similar way. You need a class with a function that takes a reference to a CX_SoundStream::inputEventArgs struct and returns void. Instead of putting data into the output buffer, you would read data out of the input buffer.


Troubleshooting Audio Problems 
------------------------------

It is often the case that audio playback problems arise due to the wrong input or output device being used. For this reason, CX_SoundStream has a utility function that lists the available devices on your system so that you can select the correct one. You do this with CX::CX_SoundStream::listDevices() like so:
\code{.cpp}
cout << CX_SoundStream::listDevices() << endl;
\endcode
Note that `listDevices()` is a static function, so you use the name of the CX_SoundStream class and `::` to access it.

CX_SoundStream uses RtAudio (http://www.music.mcgill.ca/~gary/rtaudio/) internally. It is possible that some problems could be solved with help from the RtAudio documentation. For example, one of the configuration options for CX_SoundStream is the low level audio API to use (see \ref CX::CX_SoundStream::Configuration::api), about which the RtAudio documentation provides some help (http://www.music.mcgill.ca/~gary/rtaudio/classRtAudio.html#ac9b6f625da88249d08a8409a9db0d849). You can get a pointer to the RtAudio instance being used by the CX_SoundStream by calling CX::CX_SoundStream::getRtAudioInstance(), which should allow you to do just about anything with RtAudio.




The whole example:
\code{.cpp}
#include "CX_EntryPoint.h"

void runExperiment(void) {
	//Sound stream configuration
	CX_SoundStream::Configuration ssConfig;
	ssConfig.outputChannels = 2; //Stereo output
	//ssConfig.api = RtAudio::Api::WINDOWS_DS; //The most likely thing you will need to change is the low-level audio API.
	
	CX_SoundStream soundStream;
	soundStream.setup(ssConfig);
	
	//If things aren't working, try uncommenting this line to learn about the devices available on your system for the given api.
	//cout << CX_SoundStream::listDevices(RtAudio::Api::WINDOWS_DS) << endl;

	//Playback
	CX_SoundBuffer soundBuffer;
	soundBuffer.loadFile("sound_file.wav");

	CX_SoundBufferPlayer player;
	player.setup(&soundStream);

	player.setSoundBuffer(&soundBuffer);

	player.play();

	//Wait for it to finish playing.
	while (player.isPlaying())
		;

	Log.flush();

	soundBuffer.deleteChannel(1);
	player.setSoundBuffer(&soundBuffer);

	player.play();

	//Wait for it to finish playing.
	while (player.isPlaying())
		;

	Log.flush();


	//Multiple sounds at once
	Log.notice() << "Merge together 2 CX_SoundBuffers:";
	
	CX_SoundBuffer otherBuffer;
	otherBuffer.loadFile("other_sound_file.wav");

	CX_SoundBuffer combinedBuffer = soundBuffer;

	combinedBuffer.addSound(otherBuffer, 500); //Add the second sound to the first, 
		//with the second starting 500 ms after the first.
		
	player.setSoundBuffer(&combinedBuffer);
	player.play();
	while(player.isPlaying())
		;

	CX_SoundBufferPlayer player2;
	player2.setup(&soundStream);
	player2.setSoundBuffer(&otherBuffer);

	player.play();
	Clock.sleep(500);
	player2.play();
	while (player.isPlaying() || player2.isPlaying())
		;

	//Recording
	soundStream.stop();
	ssConfig.inputChannels = 1; //Most microphones are mono.
	soundStream.setup(ssConfig);

	CX_SoundBufferRecorder recorder;
	recorder.setup(&soundStream);

	CX_SoundBuffer recordedSound;
	recorder.setSoundBuffer(&recordedSound);

	Log.flush(); //As usual, let's check for errors during setup.

	cout << "Starting to record." << endl;
	recorder.start();
	Clock.sleep(CX_Seconds(5));
	recorder.stop();
	cout << "Done recording." << endl;

	player.setSoundBuffer(&recordedSound);
	player.play();
	while (player.isPlaying())
		;
	
	recorder.setSoundBuffer(nullptr); //Make it so that no buffers are associated with the recorder.

}
\endcode