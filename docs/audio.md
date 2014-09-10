Audio Input and Output {#audioIO}
======================

Audio input and output in CX is based on a number of classes. The two most important are CX_SoundStream and CX_SoundBuffer. 
Additionally, CX_SoundBufferPlayer and CX_SoundBufferRecorder combine together a CX_SoundStream and a CX_SoundBuffer to play back or record audio, respectively.
Finally, for people wanting to synthesize audio in real time (or ahead of time), the CX::Synth namespace provides a multitude of ways to synthesize audio.
We will go through these components in a practical order.


Setting up the CX_SoundStream for Playback
------------------------------------------

Because the CX_SoundStream is what actually does audio input and output, in order to get any sounds out of a CX program, you must configure a CX_SoundStream
for use. This requires that a CX_SoundStream::Configuration struct be filled out with the desired settings and given to CX_SoundStream::setup() as the argument.
There are several configuration options for the CX_SoundStream, but, if the gods smile on you today, you will only need one, which is the number of output channels. If you try this and the gods seem to be frowning, check out the Troubleshooting Audio Problems section below.
We will use stereo output, so 2 output channels.

\code{.cpp}
CX_SoundStream::Configuration ssConfig;
ssConfig.outputChannels = 2; //Stereo output

//Create the CX_SoundStream and set it up with the configuration.
CX_SoundStream soundStream;
soundStream.setup(ssConfig);

//Check for any error messages.
Log.flush();
\endcode

If there were any errors during setup of the sound stream, they will be logged. Check the console for any messages. You can also check if the return value
of CX_SoundStream::setup() or call CX_SoundStream::isStreamRunning() to see if setup was successful.

Playback
--------

Now that we have a CX_SoundStream set up, next next thing we need to do in order to play the contents of the sound file is to load the file
into CX. This is done by creating a CX_SoundBuffer and then loading a sound file into the sound buffer, as follows.

\code{.cpp}
CX_SoundBuffer soundBuffer;
soundBuffer.loadFile("sound_file.wav");
\endcode

If there wasn't an error loading the file, `soundBuffer` now contains the contents of the sound file in a format that can be played by CX. Once you have a sound file loaded
into a CX_SoundBuffer, there are a number of things you can do with it. 
You can remove leading silence with CX_SoundBuffer::stripLeadingSilence() or add silence to the beginning or end with CX_SoundBuffer::addSilence().
You can delete part of the sound, starting from the beginning or end, with CX_SoundBuffer::deleteAmount().
You can reverse the order of the samples, so as to be able to play the sound backwards with CX_SoundBuffer::reverse().
These are just some examples. See the documentation for CX_SoundBuffer and the soundBuffer example for more things you can do with it.

Now that you have CX_SoundBuffer with sound data loaded into it, you can play it back using a CX_SoundBufferPlayer. Before you can use a CX_SoundBufferPlayer,
you have to configure it with CX_SoundBufferPlayer::setup(). `setup()` takes either a structure holding configuration options for the CX_SoundStream that will
be used by the CX_SoundBufferPlayer or a pointer to a CX_SoundStream that has already been set up. We will use the CX_SoundStream called `soundStream` that 
we configured in the previous section.

\code{.cpp}
CX_SoundBufferPlayer player;
player.setup(&soundStream);
\endcode

Now that we have a configured CX_SoundBufferPlayer, we just need to give it a CX_SoundBuffer to play with CX_SoundBufferPlayer::setSoundBuffer() and play that sound.

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

A CX_SoundBufferPlayer can have a single CX_SoundBuffer assigned to it as the active sound buffer that the CX_SoundBufferPlayer will play back when 
CX_SoundBufferPlayer::play() is called. This means that you cannot play more than one sound at once with a CX_SoundBufferPlayer. 
This limitation is by design, but also by design there are ways to play multiple sounds at once. The preferred way involves merging together multiple CX_SoundBuffers using CX_SoundBuffer::addSound(). What this does is take a CX_SoundBuffer and add it to an existing CX_SoundBuffer at a given offset. This guarantees that the two sounds will be played at the correct time relative to one another, because there is no additional statup latency when the second sound starts playing. For example:

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

You can also put multiple CX_SoundBuffers and CX_SoundBufferPlayers into C++ standard library containers, like std::vector.


Recording Audio
---------------

To record audio, you can use a CX_SoundBufferRecorder. You set it up with a CX_SoundStream, just like CX_SoundBufferPlayer. The only difference is that for recording, we need input channels instead of output channels. We will set up the CX_SoundBufferRecorder, create a new CX_SoundBuffer for it to record into, and set that buffer to be recorded to.

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
recorder.start();
Clock.sleep(CX_Seconds(5));
recorder.stop();
\endcode

We sleep the main thread for 5 seconds while the recording takes place in a secondary thread. The implication of the use of secondary threads for recording is that you can start a recording, do whatever you feel like in the main thread -- draw visual stimuli, collect responses, etc. -- all while the recording keeps happening in a secondary thread.

Once our recording time is complete, we will set a CX_SoundBufferPlayer to play the recorded sound in the normal way.

\code{.cpp}
player.setSoundBuffer(&recordedSound);
player.play();
while (player.isPlaying())
	;
\endcode

Be careful that you are not recording to a sound buffer at the same time you are playing it back, because who knows what might happen! To be careful, you can "detach" a CX_SoundBuffer from either a player or a recorder by calling, e.g., CX_SoundBufferPlayer::setSoundBuffer() with `nullptr` as the argument.

\code{.cpp}
recorder.setSoundBuffer(nullptr); //Make it so that no buffers are associated with the recorder.
\endcode



Synthesizing Audio
------------------

TODO


Troubleshooting Audio Problems 
------------------------------

TODO


Direct Control of Audio IO
--------------------------

TODO?


The whole example:
\code{.cpp}
#include "CX_EntryPoint.h"

void runExperiment(void) {
	//Sound stream configuration
	CX_SoundStream::Configuration ssConfig;
	ssConfig.outputChannels = 2; //Stereo output

	CX_SoundStream soundStream;
	soundStream.setup(ssConfig);

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

	soundBuffer.addSound(otherBuffer, 1000);

	Log.flush();


}
\endcode