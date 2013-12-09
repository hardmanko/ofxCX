#ifndef _CX_SOUND_OBJECT_H_
#define _CX_SOUND_OBJECT_H_

#include <algorithm>

#include "ofLog.h"
#include "ofFmodSoundPlayer.h"

namespace CX {

	class CX_SoundObject {
	public:

		bool loadFile (string fileName);
		bool addSound (string fileName, uint64_t timeOffset); //I'm really not sure I want to have this.
		bool addSound (CX_SoundObject so, uint64_t timeOffset);

		bool ready (void) { return _successfullyLoaded; }; //This can possibly be expanded.

		bool applyGain (float gain, int channel = -1);
		bool multiplyAmplitudeBy (float amount, int channel = -1);

		//void normalize (void); //This would just be multiplyAmplitudeBy( 1/max( getPositivePeak(), abs(getNegativePeak) ) );
		float getPositivePeak (void);
		float getNegativePeak (void);

		void setLength (uint64_t lengthInMicroseconds);
		uint64_t getLength (void);

		void stripLeadingSilence (float tolerance);

		void addSilence (uint64_t durationUs, bool atBeginning);
		void deleteAmount (uint64_t durationUs, bool fromBeginning);

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

