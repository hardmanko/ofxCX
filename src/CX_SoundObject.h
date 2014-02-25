#ifndef _CX_SOUND_OBJECT_H_
#define _CX_SOUND_OBJECT_H_

/*! \class CX::CX_SoundObject

This class is a container for a sound. It can load sound files, manipulate the contents
of the sound data, add other sounds to an existing sound at specified offsets.

In order to play a CX_SoundObject, you use a \ref CX::CX_SoundObjectPlayer.

See the soundObject example for an introduction on how to use this class along with
a CX_SoundObjectPlayer.

\note Nearly all functions of this class should be considered \ref blocking. Many of
the operations take quite a while to complete because they are performed on
a fairly large vector of sound samples.

\ingroup sound
*/

#include <algorithm>

#include "ofFmodSoundPlayer.h"

#include "CX_Clock.h"
#include "CX_Logger.h"

namespace CX {

	class CX_SoundObject {
	public:

		bool loadFile (string fileName);
		bool addSound (string fileName, CX_Micros timeOffset); //I'm really not sure I want to have this.
		bool addSound (CX_SoundObject so, CX_Micros timeOffset);
		bool setFromVector(const std::vector<float>& data, int channels, float sampleRate);

		void clear(void);

		bool isReadyToPlay (void);
		bool isLoadedSuccessfully (void) { return _successfullyLoaded; };

		bool applyGain (float gain, int channel = -1);
		bool multiplyAmplitudeBy (float amount, int channel = -1);

		//void normalize (void); //This would just be multiplyAmplitudeBy( 1/max( getPositivePeak(), abs(getNegativePeak) ) );
		float getPositivePeak (void);
		float getNegativePeak (void);
		void normalize(float amount = 1.0);

		void setLength (CX_Micros length);
		CX_Micros getLength (void);

		void stripLeadingSilence (float tolerance);

		void addSilence (CX_Micros duration, bool atBeginning);
		void deleteAmount (CX_Micros duration, bool fromBeginning);

		//setSpeed?
		void multiplySpeed (float speedMultiplier);
		void resample (float newSampleRate);
		float getSampleRate (void) { return _soundSampleRate; };

		bool setChannelCount (int channels);
		int getChannelCount (void) { return _soundChannels; };
		
		uint64_t getTotalSampleCount (void) { return _soundData.size(); };
		uint64_t getConcurrentSampleCount (void) { return _soundData.size()/_soundChannels; };

		//vector<float> getRawData (void) { return _soundData; };

		/*! This function returns a reference to the raw data underlying the sound object.
		\return A reference to the data. Modify at your own risk! */
		std::vector<float>& getRawDataReference (void) { return _soundData; };

		bool writeToFile(std::string path);

		string name; //!< This stores the name of the file from which data was read, if any. It can be set by the user with no side effects.

	private:
		//Should there be a name string for the sound? Defaulting to the file that was loaded, maybe.

		bool _successfullyLoaded;

		int _soundChannels;
		FMOD_SOUND_FORMAT _soundFormat; //This doesn't need to be here: It can be a local in the FMOD read in function.
		float _soundSampleRate;

		std::vector<float> _soundData;

		//float _readSample (int channel, unsigned int sample);
		//vector<float> _getChannelData (int channel);
	};

}

#endif //_CX_SOUND_OBJECT_H_

