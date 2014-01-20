#ifndef _CX_SOUND_OBJECT_H_
#define _CX_SOUND_OBJECT_H_

/* \class CX_SoundObject

Note: Nearly all functions of this class should be considered blocking. Many of
the operations take quite a while to complete because they are performed on
a fairly large vector of sound samples.

*/

#include <algorithm>

#include "ofFmodSoundPlayer.h"

#include "CX_Clock.h"
#include "CX_Logger.h"

namespace CX {

	class CX_SoundObject {
	public:

		bool loadFile (string fileName);
		bool addSound (string fileName, CX_Micros_t timeOffset); //I'm really not sure I want to have this.
		bool addSound (CX_SoundObject so, CX_Micros_t timeOffset);

		bool isReadyToPlay (void);
		bool isLoadedSuccessfully (void) { return _successfullyLoaded; };

		bool applyGain (float gain, int channel = -1);
		bool multiplyAmplitudeBy (float amount, int channel = -1);

		//void normalize (void); //This would just be multiplyAmplitudeBy( 1/max( getPositivePeak(), abs(getNegativePeak) ) );
		float getPositivePeak (void);
		float getNegativePeak (void);

		void setLength (CX_Micros_t length);
		CX_Micros_t getLength (void);

		void stripLeadingSilence (float tolerance);

		void addSilence (CX_Micros_t duration, bool atBeginning);
		void deleteAmount (CX_Micros_t duration, bool fromBeginning);

		//setSpeed?
		void multiplySpeed (float speedMultiplier);
		void resample (float newSampleRate);
		float getSampleRate (void) { return _soundSampleRate; };

		bool setChannelCount (int channels);
		int getChannelCount (void) { return _soundChannels; };
		
		uint64_t getTotalSampleCount (void) { return _soundData.size(); };
		uint64_t getConcurrentSampleCount (void) { return _soundData.size()/_soundChannels; };

		//vector<float> getRawData (void) { return _soundData; };
		vector<float>& getRawDataReference (void) { return _soundData; };

		string name;

	private:
		//Should there be a name string for the sound? Defaulting to the file that was loaded, maybe.

		bool _successfullyLoaded;

		int _soundChannels;
		FMOD_SOUND_FORMAT _soundFormat;
		float _soundSampleRate;

		vector<float> _soundData;

		//float _readSample (int channel, unsigned int sample);
		//vector<float> _getChannelData (int channel);
	};

}

#endif //_CX_SOUND_OBJECT_H_

