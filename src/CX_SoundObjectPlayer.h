#ifndef _CX_SOUND_OBJECT_PLAYER_H_
#define _CX_SOUND_OBJECT_PLAYER_H_

/*! \defgroup sound Sound */

/*! \class CX::CX_SoundObjectPlayer
This class is used for playing CX_SoundObjects.

\ingroup sound
*/

#include "CX_SoundObject.h"
#include "CX_SoundStream.h"
#include "CX_Clock.h"
#include "CX_Logger.h"

#include "ofEvents.h"

namespace CX  {

	typedef CX_SoundStreamConfiguration_t CX_SoundObjectPlayerConfiguration_t;

	class CX_SoundObjectPlayer {
	public:

		CX_SoundObjectPlayer (void);
		~CX_SoundObjectPlayer (void);

		bool setup (CX_SoundObjectPlayerConfiguration_t config);
		//void update (void);

		bool play (void);
		bool startPlayingAt (CX_Micros experimentTime);
		bool stop (void);

		bool isPlaying (void) { return _playing; };

		CX_SoundObjectPlayerConfiguration_t getConfiguration (void) { return (CX_SoundObjectPlayerConfiguration_t)_soundStream.getConfiguration(); };
		

		bool BLOCKING_setSound (CX_SoundObject *sound);

	private:

		bool _outputEventHandler (CX_SSOutputCallback_t &outputData);

		CX_SoundStream _soundStream;
		CX_SoundObject *_activeSoundObject;

		bool _playing;
		uint64_t _playbackStartConcurrentSample;
		uint64_t _currentConcurrentSample;

		CX_Micros _startTime;
		CX_Micros _startTimeOffset;

		void _exitHandler (ofEventArgs &a);
	
	};

}

#endif //_CX_SOUND_OBJECT_PLAYER_H_