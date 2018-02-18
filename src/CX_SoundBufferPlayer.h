#pragma once

/*! \defgroup sound Sound
There are a few different ways to deal with sounds in CX. The thing that most people want to do is to
play sounds, which is done with the CX_SoundBufferPlayer. See the soundBuffer tutorial for information
on how to do that.

If you want to record sound, use the CX_SoundBufferRecorder.

If you want to generate sound stimuli through sound synthesis, see the CX::Synth namespace and modularSynth
tutorial.

Finally, if you want to have direct control of the data going to and from a sound device, see CX_SoundStream.
*/

#include "CX_SoundBuffer.h"
#include "CX_SoundStream.h"
#include "CX_Clock.h"
#include "CX_Logger.h"

#include "ofEvents.h"

namespace CX  {


	/*!
	This class is used for playing CX_SoundBuffers. See example-soundBuffer for an example of how to use this class.

	\ingroup sound
	*/
	class CX_SoundBufferPlayer {
	public:

		typedef CX_SoundStream::Configuration Configuration; //!< This is typedef'ed to \ref CX::CX_SoundStream::Configuration.

		CX_SoundBufferPlayer(void);
		~CX_SoundBufferPlayer(void);

		// 1. Set up the player (choose one)
		//bool setup(Configuration config);
		bool setup(CX_SoundStream *ss);
		bool setup(std::shared_ptr<CX_SoundStream> ss);

		// 2. Set a buffer to play from
		bool setSoundBuffer(std::shared_ptr<CX_SoundBuffer> buffer);
		bool setSoundBuffer(CX_SoundBuffer* buffer);
		bool assignSoundBuffer(CX_SoundBuffer buffer);
		
		// 3. Play or queue the sound
		bool play(bool restart = true);
		bool queuePlayback(CX_Millis startTime, CX_Millis latencyOffset = CX_Millis(0), bool restart = true);

		// 4. (Optional) Check playback status or stop the sound before it finishes
		bool isPlaying(void);
		bool isPlaybackQueued(void);
		bool isPlayingOrQueued(void);
		void stop(void);
		
		
		// Other functions
		void seek(CX_Millis time);
		CX_Millis getPlaybackTime(void);

		std::shared_ptr<CX_SoundBuffer> getSoundBuffer(void);

		unsigned int getUnderflowsSinceLastCheck(bool logUnderflows = true);

		std::shared_ptr<CX_SoundStream> getSoundStream(void);
		//const Configuration& getConfiguration(void) const;

	private:
		
		struct OuputEventData : public std::recursive_mutex {

			OuputEventData(void) :
				playing(false),
				playbackQueued(false),
				playbackStartSampleFrame(std::numeric_limits<int64_t>::max()),
				soundPlaybackSampleFrame(0),
				underflowCount(0),
				soundBuffer(nullptr)
			{}

			bool playing;
			bool playbackQueued;

			int64_t playbackStartSampleFrame; // For queued playback, the sample frame at which playback should start.
			int64_t soundPlaybackSampleFrame; // This is relative to the current playback of the current sound buffer.

			unsigned int underflowCount;

			std::shared_ptr<CX_SoundBuffer> soundBuffer;

		} _outData;

		void _outputEventHandler(CX_SoundStream::OutputEventArgs &outputData);

		std::shared_ptr<CX_SoundStream> _soundStream;
		void _cleanUpOldSoundStream(void);
		
		bool _listeningForEvents;
		void _listenForEvents(bool listen);

		//Configuration _defaultConfigReference;
	};

	namespace Instances {
		/*! During CX initialization, `SoundPlayer` is configured to use
		`CX::Instances::SoundStream` as its `CX_SoundStream`.
		This means that only `SoundStream` needs to be set up to use `SoundPlayer`.
		`SoundPlayer` does not need to have `setup()` called.
		
		\ingroup sound */
		CX_SoundBufferPlayer SoundPlayer;
	}

}
