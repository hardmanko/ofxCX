Audio Input and Output {#audioIO}
======================

Audio input and output in CX is based on a number of classes. The two most important are CX_SoundStream and CX_SoundBuffer. 
Additionally, CX_SoundBufferPlayer and CX_SoundBufferRecorder combine together a CX_SoundStream and a CX_SoundBuffer to play back or record audio, respectively.
Finally, for people wanting to synthesize audio in real time (or ahead of time), the CX::Synth namespace provides a multitude of ways to synthesize audio.
We will go through these components in a practical order.


Setting up the Sound Stream for Playback
----------------------------------------

Because the CX_SoundStream is what actually does audio input and output, in order to get any sounds out of a CX program, you must configure a CX_SoundStream
for use. This requires that a CX_SoundStream::Configuration struct be filled out with the desired settings and given to CX_SoundStream::setup() as the argument.
There are several configuration options for the CX_SoundStream, but, if the Gods smile on you today, you will only need one, which is the number of output channels.
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

Playing multiple sounds at once
-------------------------------

A CX_SoundBufferPlayer can have a single CX_SoundBuffer assigned to it as the active sound buffer that the CX_SoundBufferPlayer will play back when 
CX_SoundBufferPlayer::play() is called. This means that you cannot play more than one sound at once with a CX_SoundBufferPlayer. 
This limitation is by design, but also by design there are ways to play multiple sounds at once. The preferred way involves merging together
multiple CX_SoundBuffers using CX_SoundBuffer::addSound().





Direct control of audio IO
--------------------------

The most important class for audio in CX is the CX_SoundStream, because it is the audio backend that interfaces with sound hardware. CX_SoundBufferPlayer and CX_SoundBufferRecorder both use
a CX_SoundStream internally to actually input and output audio. 
Although a CX_SoundStream is always used in CX whenever audio is sent or received, it is easy to not directly interact with a CX_SoundStream and still do complex audio operations.



The whole example:
\code{.cpp}
#include "CX_EntryPoint.h"

void runExperiment(void) {
	//Chunk 1: sound stream config
	CX_SoundStream::Configuration ssConfig;
	ssConfig.outputChannels = 2; //Stereo output

	CX_SoundStream soundStream;
	soundStream.setup(ssConfig);

	//Chunk 2: playback
	CX_SoundBuffer soundBuffer;
	soundBuffer.loadFile("sound_file.wav");

	//Chunnk 3: playback
	CX_SoundBufferPlayer player;
	player.setup(&soundStream);


	//Chunk 4: playback
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


	//Chunk 5: multiple sounds
	Log.notice() << "Merge together 2 CX_SoundBuffers:";
	CX_SoundBuffer otherBuffer;
	otherBuffer.loadFile("other_sound_file.wav");

	soundBuffer.addSound(otherBuffer, 1000);

	Log.flush();


}
\endcode