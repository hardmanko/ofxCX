#ifndef _CX_SOUND_OBJECT_PLAYER_H_
#define _CX_SOUND_OBJECT_PLAYER_H_

/*! \defgroup sound Sound */

/*! \class CX::CX_SoundObjectPlayer
This class is used for playing CX_SoundObjects. See the soundObject example for an example of how to use this class.

\ingroup sound
*/

#include "CX_SoundObject.h"
#include "CX_SoundStream.h"
#include "CX_Clock.h"
#include "CX_Logger.h"

#include "ofEvents.h"

namespace CX  {

	typedef CX_SoundStream::Configuration CX_SoundObjectPlayerConfiguration_t; //!< This is typedef'ed to \ref CX::CX_SoundStream::Configuration.

	class CX_SoundObjectPlayer {
	public:

		CX_SoundObjectPlayer (void);
		~CX_SoundObjectPlayer (void);

		bool setup (CX_SoundObjectPlayerConfiguration_t config);

		bool play (void);
		bool startPlayingAt (CX_Micros experimentTime, CX_Micros offset);
		bool stop (void);

		//! Check if the sound is currently playing.
		bool isPlaying (void) { return _playing; };

		//! Check if the sound is queued to play.
		bool isQueuedToStart(void) { return _playbackStartQueued; }; 

		//! Returns the configuration used for this CX_SoundObjectPlayer.
		CX_SoundObjectPlayerConfiguration_t getConfiguration (void) { return (CX_SoundObjectPlayerConfiguration_t)_soundStream.getConfiguration(); };
		
		bool BLOCKING_setSound (CX_SoundObject *sound);

		void setTime(CX_Micros time);

	private:

		bool _outputEventHandler (CX_SoundStream::OutputEventArgs &outputData);

		CX_SoundStream _soundStream;
		CX_SoundObject *_activeSoundObject;

		bool _playing;

		bool _playbackStartQueued;
		uint64_t _playbackStartSampleFrame;
		uint64_t _currentSampleFrame; //This is an absolute: It is never reset. At a sample rate of 48000 Hz, this will overflow every 12186300 years.
		uint64_t _soundPlaybackSampleFrame;	
	};

}

#endif //_CX_SOUND_OBJECT_PLAYER_H_