#pragma once

/*! \class CX::CX_SoundBuffer

This class is a container for a sound. It can load sound files, manipulate the contents
of the sound data, add other sounds to an existing sound at specified offsets.

In order to play a CX_SoundBuffer, you use a \ref CX::CX_SoundBufferPlayer. See the soundBuffer
example for an introduction on how to use this class along with a CX_SoundBufferPlayer.

To record from a microphone into a CX_SoundBuffer, you use a CX::CX_SoundBufferRecorder.

\note Nearly all functions of this class should be considered \ref blockingCode. Many of
the operations can take quite a while to complete because they are performed on
a potentially large vector of sound samples.

\ingroup sound
*/

#include <algorithm>

#include "ofFmodSoundPlayer.h"

#include "CX_Definitions.h"
#include "CX_Clock.h"
#include "CX_Logger.h"

namespace CX {

	class CX_SoundBuffer {
	public:

		//! Stores the name of the last file from which data was loaded with `loadFile()`. It can be set by the user with no side effects.
		std::string name;


		CX_SoundBuffer(void);
		CX_SoundBuffer(std::string fileName);

		bool loadFile(std::string fileName);
		bool writeToFile(std::string fileName);

		bool initialize(float sampleRate, unsigned int channels, SampleFrame sf, bool zero = true);
		bool setFromVector(float sampleRate, unsigned int channels, const std::vector<float>& data);

		
		bool isReadyToPlay(bool log = false) const;

		void clear(void);



		bool addSound(std::string fileName, CX_Millis timeOffset);
		bool addSound(CX_SoundBuffer sb, CX_Millis timeOffset);

		bool insertSound(CX_SoundBuffer sb, CX_Millis insertionTime);

		bool insertChannel(CX_SoundBuffer sb, unsigned int channel);
		bool setChannel(CX_SoundBuffer sb, unsigned int channel);
		void setChannelData(unsigned int channel, const std::vector<float>& data);

		CX_SoundBuffer copyChannel(unsigned int channel) const;
		CX_SoundBuffer copySection(CX_Millis start, CX_Millis end) const;

		



		// Amplitude
		bool applyGain(float gain, int channel = -1);
		bool multiplyAmplitudeBy (float amount, int channel = -1);
		void normalize(float amount = 1.0);

		float getPositivePeak(void);
		float getNegativePeak(void);

		// Length
		void setLength(CX_Millis length);
		CX_Millis getLength(void) const;

		void setLengthSF(SampleFrame sf);
		SampleFrame getLengthSF(void) const;

		size_t getLengthSamples(void) const;


		void stripLeadingSilence (float tolerance);
		void addSilence(CX_Millis duration, bool atBeginning);
		void addSilenceSF(SampleFrame sf, bool atBeginning);

		void deleteAmount(CX_Millis duration, bool fromBeginning);
		void deleteSection(CX_Millis start, CX_Millis end);
		bool deleteChannel(unsigned int channel);
		bool clearChannel(unsigned int channel);
		
		void reverse(void);

		void multiplySpeed(float speedMultiplier);
		void resample(float newSampleRate);
		float getSampleRate(void) const;

		bool setChannelCount(unsigned int channels, bool average = true);
		unsigned int getChannelCount(void) const;
		
		
		// Raw access
		std::vector<float>& getRawDataReference(void);

		float getSample(unsigned int channel, SampleFrame sf) const;
		void setSample(unsigned int channel, SampleFrame sf, float val);

		//void setMetadata(unsigned int channels, float sampleRate);


		SampleFrame getSampleFrameAt(CX_Millis time) const;
		// OR
		SampleFrame timeToSF(CX_Millis time) const;
		CX_Millis sfToTime(SampleFrame sf) const;

	private:

		unsigned int _channels;
		float _sampleRate;

		std::vector<float> _data;

		size_t _timeToSample(CX_Millis time) const;

		void _resize(unsigned int channelCount, SampleFrame sf, bool zero = true);

	};

}
