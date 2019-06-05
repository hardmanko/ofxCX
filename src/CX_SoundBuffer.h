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

#include "CX_Clock.h"
#include "CX_Logger.h"

namespace CX {

	class CX_SoundBuffer {
	public:

		CX_SoundBuffer(void);
		CX_SoundBuffer(std::string fileName);

		bool loadFile(std::string fileName);
		bool writeToFile(std::string path);

		//! Stores the name of the last file from which data was loaded with `loadFile()`. It can be set by the user with no side effects.
		std::string name;

		bool addSound(std::string fileName, CX_Millis timeOffset);
		bool addSound(CX_SoundBuffer sb, CX_Millis timeOffset);
		bool insertSound(CX_SoundBuffer sb, CX_Millis timeOffset);
		bool setFromVector(const std::vector<float>& data, int channels, float sampleRate);
		void setChannelData(unsigned int channel, const std::vector<float>& data);

		CX_SoundBuffer copySection(CX_Millis start, CX_Millis end) const;

		void clear(void);

		bool isReadyToPlay(void) const;

		// Amplitude
		bool applyGain(float gain, int channel = -1);
		bool multiplyAmplitudeBy (float amount, int channel = -1);
		void normalize(float amount = 1.0);

		float getPositivePeak(void);
		float getNegativePeak(void);

		// Duration
		void setLength(CX_Millis length);
		CX_Millis getLength(void);

		void stripLeadingSilence (float tolerance);
		void addSilence(CX_Millis duration, bool atBeginning);

		void deleteAmount(CX_Millis duration, bool fromBeginning);
		void deleteSection(CX_Millis start, CX_Millis end);
		bool deleteChannel(unsigned int channel);
		
		void reverse(void);

		void multiplySpeed(float speedMultiplier);
		void resample(float newSampleRate);
		float getSampleRate(void) const;

		bool setChannelCount(unsigned int channels, bool average = true);
		unsigned int getChannelCount(void) const;
		
		uint64_t getTotalSampleCount(void) const;
		uint64_t getSampleFrameCount(void) const;

		std::vector<float>& getRawDataReference(void);



		//void setMetadata(unsigned int channels, float sampleRate);

	private:

		unsigned int _channels;
		float _sampleRate;

		std::vector<float> _data;

		unsigned int _timeToSample(CX_Millis time, int channels = -1) const;

	};

}
