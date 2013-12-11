#ifndef _CX_SOUND_OBJECT_PLAYER_H_
#define _CX_SOUND_OBJECT_PLAYER_H_

#include "CX_SoundObject.h"
#include "CX_SoundStream.h"

#include "ofEvents.h"

namespace CX  {

	typedef CX_SoundStreamConfiguration_t CX_SoundObjectPlayerConfiguration_t;

	/*!
	This class is used for playing CX_SoundObjects.
	*/
	class CX_SoundObjectPlayer {
	public:

		CX_SoundObjectPlayer (void);
		~CX_SoundObjectPlayer (void);

		bool setup (CX_SoundObjectPlayerConfiguration_t config);
		

		bool play (void);
		bool startPlayingAt (uint64_t time);
		bool stop (void);

		bool isPlaying (void) { return _playing; };

		CX_SoundObjectPlayerConfiguration_t getConfiguration (void) { return (CX_SoundObjectPlayerConfiguration_t)_soundStream.getConfiguration(); };
		//void update (void);

		bool BLOCKING_setSound (CX_SoundObject *sound);

	private:

		bool _outputEventHandler (CX_SSOutputCallback_t &outputData);

		CX_SoundStream _soundStream;
		CX_SoundObject *_activeSoundObject;

		bool _playing;
		uint64_t _playbackStartConcurrentSample;
		uint64_t _currentConcurrentSample;

		uint64_t _startTime;
		uint64_t _startTimeOffset;

		void _exitHandler (ofEventArgs &a);
	
	};

}

#endif //_CX_SOUND_OBJECT_PLAYER_H_