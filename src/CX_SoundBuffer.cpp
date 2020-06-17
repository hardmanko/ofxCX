#include "CX_SoundBuffer.h"

#include "ofFmodSoundPlayer.h"

namespace CX {

/*! Default constructor. */
CX_SoundBuffer::CX_SoundBuffer(void) :
	_channels(0),
	_sampleRate(0)
{}

/*! Constructs the CX_SoundBuffer, calling `loadFile(fileName)`.

\param fileName Name of the sound file to load.

*/
CX_SoundBuffer::CX_SoundBuffer(std::string fileName) {
	loadFile(fileName);
}

/*!
Loads a sound file with the given file name into the `CX_SoundBuffer`. 
Any pre-existing data in the `CX_SoundBuffer` is deleted.
Some sound file types are supported. Others are not. In the limited testing, mp3 and wav files seem to work well.
If the file cannot be loaded, descriptive error messages will be logged.

\param fileName Name of the sound file to load.

\return `true` if the sound given in the `fileName` was loaded succesffuly, `false` otherwise.
*/
bool CX_SoundBuffer::loadFile(std::string fileName) {
	

	ofFmodSoundPlayer fmPlayer;
#if OF_VERSION_MAJOR >= 0 && OF_VERSION_MINOR >= 9 && OF_VERSION_PATCH >= 0
	bool loadSuccessful = fmPlayer.load(fileName, false);
#else
	bool loadSuccessful = fmPlayer.loadSound(fileName, false);
#endif
	if (!loadSuccessful) {
		CX::Instances::Log.error("CX_SoundBuffer") << "Error loading " << fileName;
		fmPlayer.unload(); //Just in case. This safety is probably unneccessary.
		return false;
	}

	bool copySuccess = false;

	FMOD_SOUND *fmSound = fmPlayer.sound;

	FMOD_SOUND_TYPE soundType; //I'm not sure what to do with this, either. Check that it isn't a bizzare unreadable type, maybe?
	FMOD_SOUND_FORMAT soundFormat;
	int channels;
	int bits; //I'm not really sure what to do with this that soundFormat doesn't give me.

	FMOD_RESULT formatResult = FMOD_Sound_GetFormat( fmSound, &soundType, &soundFormat, &channels, &bits );
	if (formatResult != FMOD_OK) {
		CX::Instances::Log.error("CX_SoundBuffer") << "Error getting sound format of " << fileName;
		fmPlayer.unload();
		return false;
	}

	_channels = channels;
	//_soundFormat = soundFormat;

	//Find out the sample rate of the sound. To be used if it needs to be resampled.
	//At least, I think this gets the sample rate of the sound. The documentation isn't clear.
	FMOD_Sound_GetDefaults( fmSound, &_sampleRate, NULL, NULL, NULL );

	void *ptr1;
	void *ptr2;
	unsigned int length1;
	unsigned int length2;

	switch (soundFormat) {
	case FMOD_SOUND_FORMAT_PCM8:
		CX::Instances::Log.error("CX_SoundOutput") << "File " << fileName << " is in an unsupported format (8-bit PCM). FMOD_SOUND_FORMAT_PCM8 not yet supported.";
		break;
	case FMOD_SOUND_FORMAT_PCM16:
		{
			unsigned int samplesToRead =  fmPlayer.length; // * channelCount ????

			FMOD_RESULT lockResult = FMOD_Sound_Lock( fmSound, 0, samplesToRead, &ptr1, &ptr2, &length1, &length2 );

			if (lockResult == FMOD_OK) {
				unsigned int totalSamples = length1 * channels; //The documentation says that length1 is in bytes, but it seems to be wrong.
					//It is in samples for everything I've found (which have been entirely 16-bit PCM).
				_data.resize( totalSamples );

				for (unsigned int i = 0; i < totalSamples; i++) {
					_data[i] = ((float)(((int16_t*)ptr1)[i]))/32768;
				}
				copySuccess = true;
			} else {
				CX::Instances::Log.error("CX_SoundOutput") << "Error locking sound data";
			}

			FMOD_RESULT unlockResult = FMOD_Sound_Unlock( fmSound, ptr1, ptr2, length1, length2 );
		}
		break;
	case FMOD_SOUND_FORMAT_PCM24:
		//This is annoying because the sign must be extended. Maybe should also be processed as 64-bit float.
		CX::Instances::Log.error("CX_SoundOutput") << "File " << fileName << " is in an unsupported format (24-bit PCM). FMOD_SOUND_FORMAT_PCM24 not yet supported.";
		break;
	case FMOD_SOUND_FORMAT_PCM32:
		//This is annoying because it must be processed as 64-bit float before being converted back to 32-bit float.
		CX::Instances::Log.error("CX_SoundOutput") << "File " << fileName << " is in an unsupported format (32-bit PCM). FMOD_SOUND_FORMAT_PCM32 not yet supported.";
		break;
	case FMOD_SOUND_FORMAT_PCMFLOAT:
		//This is awesome because I don't have to do anything at all with it!
		{
			unsigned int samplesToRead = fmPlayer.length; // * channelCount ????

			FMOD_RESULT lockResult = FMOD_Sound_Lock( fmSound, 0, samplesToRead, &ptr1, &ptr2, &length1, &length2 );

			if (lockResult == FMOD_OK) {
				unsigned int totalSamples = length1 * channels;
				_data.resize( totalSamples );
				memcpy(_data.data(), ptr1, totalSamples * sizeof(float));
				copySuccess = true;
			} else {
				CX::Instances::Log.error("CX_SoundOutput") << "Error locking sound data";
			}

			FMOD_RESULT unlockResult = FMOD_Sound_Unlock( fmSound, ptr1, ptr2, length1, length2 );
		}
		break;
	case FMOD_SOUND_FORMAT_NONE:
		CX::Instances::Log.error("CX_SoundOutput") << "File " << fileName << " of unknown format.";
		break;
	case FMOD_SOUND_FORMAT_GCADPCM:
	case FMOD_SOUND_FORMAT_IMAADPCM:
	case FMOD_SOUND_FORMAT_VAG:
	case FMOD_SOUND_FORMAT_XMA:
	case FMOD_SOUND_FORMAT_MPEG:
	case FMOD_SOUND_FORMAT_MAX:
	case FMOD_SOUND_FORMAT_FORCEINT:
		CX::Instances::Log.error("CX_SoundOutput") << "File " << fileName << " is of unsupported file format (compressed/video game console). There are no plans to ever support these formats.";
		break;
	};

	//Clean up by unloading this sound. Again, probably unneccessary. ofFmodPlayer unloads itself during destruction.
	fmPlayer.unload();

	if (copySuccess) {
		name = fileName;
	} else {
		this->clear();
	}

	return copySuccess;
}

/*!
Uses loadFile(string) and addSound(CX_SoundBuffer, CX_Millis) to add the 
given file to the current CX_SoundBuffer at the given time offset.
See those functions for more information.

\param fileName Name of the sound file to load.
\param timeOffset Time at which to add the new sound.

\return Returns `true` if the new sound was added sucessfully, `false` otherwise.
*/
bool CX_SoundBuffer::addSound(std::string fileName, CX_Millis timeOffset) {
	if (!this->isReadyToPlay()) {
		bool loadSuccess = this->loadFile(fileName);
		if (loadSuccess) {
			this->addSilence(timeOffset, true);
		}
		return loadSuccess;
	} else {
		CX_SoundBuffer temp;
		if (!temp.loadFile(fileName)) {
			return false;
		}
		this->addSound(temp, timeOffset);
		return true;
	}
}

/*!
Adds the sound data in `sb` at the time offset. 

If the sample rates of the sounds differ, `sb` will be resampled to the sample rate of this `CX_SoundBuffer`.

If the number of channels of `sb` does not equal the number of channels of this `CX_SoundBuffer`, an attempt will be made to set the number of channels of
`sb` equal to the number of channels of this.

The data from `sb` and this `CX_SoundBuffer` are merged by adding the amplitudes of the sounds. The result of the addition is clamped between -1 and 1.

\param sb A `CX_SoundBuffer`. Must be successfully loaded.
\param timeOffset Time at which to add the new sound data.

\return `true` if `sb` was successfully added to this `CX_SoundBuffer`, `false` otherwise.
*/
bool CX_SoundBuffer::addSound(CX_SoundBuffer sb, CX_Millis timeOffset) {
	if (!sb.isReadyToPlay()) {
		CX::Instances::Log.error("CX_SoundBuffer") << "addSound(): Added sound buffer not ready to play. It will not be added.";
		return false;
	}

	//This condition really should have a warning/notice message associated with it.
	if (!this->isReadyToPlay()) {
		*this = sb;
		this->addSilence(timeOffset, true);
		return true;
	}

	if (sb.getSampleRate() != this->getSampleRate()) {
		sb.resample( this->getSampleRate() );
	}

	if (sb.getChannelCount() != this->getChannelCount()) {
		if (!sb.setChannelCount( this->getChannelCount() )) {
			CX::Instances::Log.error("CX_SoundBuffer") << "addSound: Failed to match the number of channels of added sound to existing sound. The new sound will not be added.";
			return false;
		}
	}

	size_t insertionSample = _timeToSample(timeOffset);

	//Get the new data that will be merged.
	std::vector<float> &newData = sb.getRawDataReference();

	//If this sound isn't long enough to hold the new data, resize it to fit.
	if (insertionSample + newData.size() > this->_data.size()) {
		_data.resize( insertionSample + newData.size(), 0 ); //When resizing, set any new elements to silence (i.e. 0).
	}

	//Copy over the new data, clamping as needed.
	for (size_t i = 0; i < newData.size(); i++) {
		float newVal = _data[insertionSample + i] + newData[i];

		_data[insertionSample + i] = Util::clamp<float>(newVal, -1.0f, 1.0f);
	}

	return true;
}

/*! Inserts a sound into the sound buffer. Any sound data past the insertion point
given by `timeOffset` will be moved to be after the content of `sb`.

The new length of the sound buffer will be its previous length plus the
length of `sb`.

\param sb A `CX_SoundBuffer`. Must be ready to play (see `isReadyToPlay()`).
\param insertionTime Time at which to add the new sound data.

\return `true` if `sb` was successfully inserted to this `CX_SoundBuffer`, `false` otherwise.
*/
bool CX_SoundBuffer::insertSound(CX_SoundBuffer sb, CX_Millis insertionTime) {

	if (!sb.isReadyToPlay()) {
		CX::Instances::Log.error("CX_SoundBuffer") << "insertSound(): Inserted sound buffer was not ready to play. It was not inserted.";
		return false;
	}

	if (this->isReadyToPlay()) {
		// If this is ready to play, sb gets its sample rate.
		if (sb.getSampleRate() != this->getSampleRate()) {
			sb.resample(this->getSampleRate());
		}
	} else {
		// If this is not ready to play, copy the new sound.
		// This condition really should have a warning/notice message associated with it.
		*this = sb;
		this->addSilence(insertionTime, true);
		return true;
	}


	if (sb.getChannelCount() != this->getChannelCount()) {
		if (!sb.setChannelCount(this->getChannelCount())) {
			CX::Instances::Log.error("CX_SoundBuffer") << "insertSound(): Failed to match the number of channels of added sound to existing sound. The new sound was not inserted.";
			return false;
		}
	}

	size_t insertionSample = _timeToSample(insertionTime);
	size_t newLengthSamples = sb.getLengthSamples() + insertionSample;

	// If the new end is past the end of this sound, pad this with silence.
	if (newLengthSamples > this->getLengthSamples()) {
		size_t sampleDifference = newLengthSamples - this->getLengthSamples();
		_data.insert(_data.end(), sampleDifference, 0);
	}

	//Insert the data
	_data.insert(_data.begin() + insertionSample, sb._data.begin(), sb._data.end());

	return true;
}


/*! Insert a channel of sound data into the existing channels of the CX_SoundBuffer.

You may insert a channel into an empty CX_SoundBuffer.

\param sb A sound buffer with exactly 1 channel that `isReadyToPlay()`.
\param channel The 0-indexed channel index for the channel being inserted.
If greater than the highest existing channel, additional channels (set to silence)
will be added between the existing channels and the new channel if needed.
*/
bool CX_SoundBuffer::insertChannel(CX_SoundBuffer sb, unsigned int channel) {

	if (!sb.isReadyToPlay()) {
		CX::Instances::Log.error("CX_SoundBuffer") << "insertChannel(): To-be-inserted sound buffer was not ready to play. It was not inserted.";
		return false;
	}

	if (sb.getChannelCount() != 1) {
		CX::Instances::Log.error("CX_SoundBuffer") << "insertChannel(): To-be-inserted sound buffer did not have exactly 1 channel. It was not inserted.";
		return false;
	}

	// You need to make sure that this keeps the same sample rate, so resample
	// before testing if ready to play.
	if (sb.getSampleRate() != this->getSampleRate()) {
		sb.resample(this->getSampleRate());
	}

	// If this is ready to play, use it to resample the new channel
	if (this->isReadyToPlay()) {
		sb.resample(this->getSampleRate());
	} else {
		//  But if this is not ready, clear and set sample rate from sb
		this->clear();

		this->_sampleRate = sb._sampleRate;
	}

	// If new sound buffer is too short, lengthen it with silence.
	//if (sb.getLengthSF() < this->getLengthSF()) {
	//	sb.setLengthSF(this->getLengthSF());
	//}
	
	unsigned int newChannels = _channels + 1; // Default insert: Increase current channel count by 1.

	bool atEnd = channel >= _channels; // Don't need to move existing channels if at end.

	if (atEnd) {
		// If at end, the new number of channels is the channel index plus 1.
		newChannels = channel + 1;
	}

	SampleFrame newSF = std::max(sb.getLengthSF(), this->getLengthSF());

	// Past this point, _channels is updated
	this->_resize(newChannels, newSF, true);

	// Move old channels above the new channel up 1 channel, which only needs to 
	// be done if the inserted channel is not at the end.
	if (!atEnd) {
		for (SampleFrame sf = 0; sf < newSF; sf++) {

			size_t indexBase = sf * newChannels;

			for (unsigned int newCh = newChannels - 1; newCh > channel; newCh--) {
				_data[indexBase + newCh] = _data[indexBase + newCh - 1];
			}

		}
	}
	

	this->setChannelData(channel, sb.getRawDataReference());

	return true;
}

// This function seems incomplete
bool CX_SoundBuffer::setChannel(CX_SoundBuffer sb, unsigned int channel) {

	if (!sb.isReadyToPlay()) {
		CX::Instances::Log.error("CX_SoundBuffer") << "setChannel(): Sound buffer was not ready to play. It was not set.";
		return false;
	}

	if (sb.getChannelCount() != 1) {
		CX::Instances::Log.error("CX_SoundBuffer") << "setChannel(): Sound buffer did not have exactly 1 channel. It was not set.";
		return false;
	}

	// If this is ready to play, use it to resample the new channel
	if (this->isReadyToPlay()) {
		sb.resample(this->getSampleRate());
	}
	else {
		//  But if this is not ready, clear and set sample rate from sb
		this->clear();

		this->_sampleRate = sb._sampleRate;
	}
	

	setChannelData(channel, sb.getRawDataReference());

	return true;
}

/*! Set the contents of a single channel from a vector of float data.

\param channel The channel to set the data for. If greater than any existing channel, new channels will be created
so that the number of stored channels is equal to `channel + 1`. If you don't want a bunch of new empty channels, make sure
you don't use a large channel number.

\param data A vector of sound samples. These values must be in the interval [-1, 1], which is not checked for.
See CX::Util::clamp() for one method of making sure your data are in the correct range.
If the other channels in the CX_SoundBuffer are longer than `data`, `data` will be extended with zeroes.
If the other channels in the CX_SoundBuffer are shorter than `data`, those channels will be extended with zeroes.

*/
void CX_SoundBuffer::setChannelData(unsigned int channel, const std::vector<float>& data) {

	bool moreChannels = channel >= _channels;
	bool newLonger = data.size() > this->getLengthSF();

	if (moreChannels || newLonger) {

		unsigned int newChannels = moreChannels ? channel + 1 : _channels;
		SampleFrame newSF = newLonger ? data.size() : this->getLengthSF();

		this->_resize(newChannels, newSF, true);
	}

	// Copy over the data
	for (size_t sf = 0; sf < data.size(); sf++) {
		size_t index = (sf * _channels) + channel;
		_data[index] = data[sf];
	}

	// If this is longer than the new data, zero end of channel
	if (data.size() < this->getLengthSF()) {
		for (size_t sf = data.size(); sf < getLengthSF(); sf++) {
			size_t index = (sf * _channels) + channel;
			_data[index] = 0;
		}
	}

}



CX_SoundBuffer CX_SoundBuffer::copyChannel(unsigned int channel) const {

	std::vector<float> channelCopy(getLengthSF());

	for (size_t sf = 0; sf < channelCopy.size(); sf++) {
		size_t ind = (sf * _channels) + channel;
		channelCopy[sf] = _data[ind];
	}

	CX_SoundBuffer rval;
	rval.setFromVector(this->getSampleRate(), 1, channelCopy);

	return rval;
}

bool CX_SoundBuffer::initialize(float sampleRate, unsigned int channels, SampleFrame sf, bool zero) {

	if (sampleRate <= 0.0f) {
		CX::Instances::Log.error("CX_SoundBuffer") << "initialize(): Cannot initialize a buffer with sampleRate <= 0." <<
			" The provided sample rate was " << sampleRate << ".";
		return false;
	}

	if (channels == 0 && sf > 0) {
		CX::Instances::Log.notice("CX_SoundBuffer") << "initialize(): 0 channels were requested but more than 0 sample frames were requested. No sample frames can be stored unless there is at least 1 channel.";
	}

	_channels = channels;
	_sampleRate = sampleRate;

	if (zero) {
		_data.resize(sf * channels, 0);
	} else {
		_data.resize(sf *channels);
	}

	return true;
}


/*! Set the contents of the sound buffer from a vector of float data.

\param sampleRate The sample rate of the samples. If `data` contains, for example, a sine wave, that wave was sampled
at some rate (e.g. 48000 samples per second of waveform). `sampleRate` should be that rate.
\param channels The number of channels worth of data that is stored in `data`.
\param data A vector of sound samples. These values should go from -1 to 1. This requirement is not checked for.
If there is more than once channel of data, the data must be interleaved. This means that if, for example,
there are two channels, the ordering of the samples is 12121212... where 1 represents a sample for channel
1 and 2 represents a sample for channel 2. This requirement is not checked for.
The number of samples in this vector must be evenly divisible by the number of channels set with the `channels` argument,
which is checked for and an error logged if needed.

return True in all cases. No checking is done on any of the arguments.
*/
bool CX_SoundBuffer::setFromVector(float sampleRate, unsigned int channels, const std::vector<float>& data) {

	if (channels > 0 && (data.size() % channels) != 0) {
		CX::Instances::Log.error("CX_SoundBuffer") << "setFromVector(): The size of the sample data was not evenly divisible by the number of channels.";
		return false;
	}

	_data = data;
	_channels = channels;
	_sampleRate = sampleRate;
	return true;
}


/*! Clears all data stored in the sound buffer and returns it to an uninitialized state. */
void CX_SoundBuffer::clear(void) {
	_data.clear();
	_channels = 0;
	_sampleRate = 0;
	name = "";
}

/*! Checks to see if the CX_SoundBuffer is ready to play. It basically just checks if there is sound data
available and that the number of channels is set to a sane value. */
bool CX_SoundBuffer::isReadyToPlay(bool log) const {

	bool hasSoundData = _data.size() > 0;
	bool hasSoundChannels = _channels > 0;
	bool acceptableSampleCount = false;
	if (_channels > 0) {
		acceptableSampleCount = (_data.size() % _channels) == 0;
	}
	bool validSampleRate = _sampleRate > 0;

	if (log) {
		if (!hasSoundData) {
			CX::Instances::Log.warning("CX_SoundBuffer") << "isReadyToPlay(): Buffer not ready because it contains no sound data (0 samples of data).";
		}

		if (!hasSoundChannels) {
			CX::Instances::Log.warning("CX_SoundBuffer") << "isReadyToPlay(): Buffer not ready because it has 0 channels of sound data.";
		}

		if (!acceptableSampleCount) {
			CX::Instances::Log.warning("CX_SoundBuffer") << "isReadyToPlay(): Buffer not ready because the total number of samples (not sample frames) is not divisible by the number of channels.";
		}

		if (!validSampleRate) {
			CX::Instances::Log.warning("CX_SoundBuffer") << "isReadyToPlay(): Buffer not ready because it does not have a valid sample rate.";
		}
	}

	return hasSoundData && hasSoundChannels && acceptableSampleCount && validSampleRate;
}

/*! Set the length of the sound to the specified length. If the new length is longer than the old length,
the new data is zeroed (i.e. set to silence). */
void CX_SoundBuffer::setLength(CX_Millis length) {
	setLengthSF(this->timeToSF(length));
}

/*! Gets the length, in time, of the data stored in the sound buffer. This depends on the sample rate of the sound.
\return The length. */
CX_Millis CX_SoundBuffer::getLength(void) const {

	if (!this->isReadyToPlay()) {
		return CX_Millis(0);
	}

	return CX_Seconds(this->getLengthSF() / getSampleRate());
}


void CX_SoundBuffer::setLengthSF(SampleFrame sf) {
	_data.resize(sf * _channels, 0);
}

/*! Get the number of sample frames of the sound data held by the CX_SoundBuffer,
which is equal to the total number of samples divided by the number of channels.
*/
SampleFrame CX_SoundBuffer::getLengthSF(void) const {
	if (!this->isReadyToPlay()) {
		return 0;
	}
	return _data.size() / _channels;
}

/*! Get the total number of samples in the sound data held by the CX_SoundBuffer,
which is equal to the number of sample frames times the number of channels.
*/
size_t CX_SoundBuffer::getLengthSamples(void) const {
	return _data.size();
}


template <> 
void CX_SoundBuffer::setLength(CX_Millis length) {
	this->setLength<SampleFrame>(this->timeToSF(length));
}

template <> 
void CX_SoundBuffer::setLength(SampleFrame length) {
	_data.resize(length * _channels, 0);
}

template <>
CX_Millis CX_SoundBuffer::getLength(void) const {
	if (!this->isReadyToPlay()) {
		return CX_Millis(0);
	}

	return CX_Seconds(this->getLength<SampleFrame>() / getSampleRate());
}


template <>
SampleFrame CX_SoundBuffer::getLength(void) const {
	if (!this->isReadyToPlay()) {
		return 0;
	}

	return _data.size() / _channels;
}







/*! Finds the maximum amplitude in the sound buffer.
\return The maximum amplitude.
\note Amplitudes are between -1 and 1, inclusive. */
float CX_SoundBuffer::getPositivePeak(void) {
	return Util::max(_data);
}

/*! Finds the minimum amplitude in the sound buffer.
\return The minimum amplitude.
\note Amplitudes are between -1 and 1, inclusive. */
float CX_SoundBuffer::getNegativePeak(void) {
	return Util::min(_data);
}

/*! Normalizes the contents of the sound buffer.
\param amount The peak with the greatest absolute amplitude will be set to `amount` and all other 
samples will be scaled proportionally so as to retain their relationship with the greatest absolute peak.
Should be in the interval [0,1], unless clipping is desired. Should be positive unless you want to invert
the waveform.
*/
void CX_SoundBuffer::normalize(float amount) {
	float peak = std::max(std::abs(getPositivePeak()), std::abs(getNegativePeak()));
	float multiplier = amount / peak;

	for (unsigned int i = 0; i < _data.size(); i++) {
		_data[i] *= multiplier;
	}
}

/*!
Removes leading "silence" from the sound, where silence is defined by the given tolerance. It is unlikely that
the beginning of a sound, even if perceived as silent relative to the rest of the sound, has an amplitude of 0.
Therefore, a tolerance of 0 is unlikely to prove useful. 

Values from `getPositivePeak()` and/or `getNegativePeak()` can help to give a reference amplitude of which some 
small fraction is perceived as "silent".

\param tolerance All sound data up to and including the first instance of a sample with an amplitude
with an absolute value greater than or equal to tolerance is removed from the sound.
*/
void CX_SoundBuffer::stripLeadingSilence(float tolerance) {

	if (!isReadyToPlay()) {
		return;
	}

	size_t i = 0;
	for (; i < _data.size(); i++) {
		if (std::abs(_data[0]) >= tolerance) {
			break;
		}
	}

	size_t firstKept = i + (_channels - (i % _channels));

	if (firstKept > 0) {
		_data.erase(_data.begin(), _data.begin() + firstKept);
	}

}

/*! Adds the specified amount of silence to the CX_SoundBuffer at either the beginning or end.

\param duration Duration of added silence. Dependent on the sample rate of the sound. If the sample rate changes,
so does the duration of silence.
\param atBeginning If true, silence is added at the beginning of the CX_SoundBuffer. If false, the silence is added at the end.
*/
void CX_SoundBuffer::addSilence(CX_Millis duration, bool atBeginning) {
	addSilenceSF(timeToSF(duration), atBeginning);
}

void CX_SoundBuffer::addSilenceSF(SampleFrame sf, bool atBeginning) {
	auto insertIt = atBeginning ? _data.begin() : _data.end();

	_data.insert(insertIt, sf * _channels, 0.0f);
}

/*! Silence a section of audio. 
\param begin Beginning of the sample frames to silence (inclusive).
\param end End of the sample frames to silence (first non-silenced sample frame).
*/
void CX_SoundBuffer::silenceSection(SampleFrame begin, SampleFrame end) {

	size_t beginSample = begin * _channels;
	size_t endSample = end * _channels;

	if (beginSample > _data.size() || endSample > _data.size()) {
		CX::Instances::Log.warning("CX_SoundBuffer") << "silenceSection(): Begin or end sample frames were out of rance. Nothing was silenced.";
		return; // false
	}

	for (size_t i = beginSample; i < endSample; i++) {
		_data[i] = 0.0f;
	}

}

/*! Deletes the specified amount of sound from the CX_SoundBuffer from either the beginning or end.

\param duration Duration of removed sound. If this is greater than the duration of the sound, the whole sound is deleted.
\param fromBeginning If true, sound is deleted from the beginning of the CX_SoundBuffer's buffer.
If false, the sound is deleted from the end, toward the beginning.
*/
void CX_SoundBuffer::deleteAmount(CX_Millis duration, bool fromBeginning) {

	size_t absoluteSampleCount = _timeToSample(duration);

	if (absoluteSampleCount >= _data.size()) {
		_data.clear();
	} else {
		if (fromBeginning) {
			_data.erase( _data.begin(), _data.begin() + absoluteSampleCount );
		} else {
			_data.erase( _data.end() - absoluteSampleCount, _data.end() );
		}
	}
}

void CX_SoundBuffer::deleteSection(CX_Millis start, CX_Millis end) {

	auto startIt = _data.begin() + _timeToSample(start);
	auto endIt = _data.begin() + _timeToSample(end);

	_data.erase(startIt, endIt);
}

CX_SoundBuffer CX_SoundBuffer::copySection(CX_Millis start, CX_Millis end) const {

	auto startIt = _data.begin() + _timeToSample(start);
	auto endIt = _data.begin() + _timeToSample(end);
	
	CX_SoundBuffer copy = *this;
	copy._data.assign(startIt, endIt);

	return copy;
}

bool CX_SoundBuffer::clearChannel(unsigned int channel) {

	if (channel >= _channels) {
		CX::Instances::Log.error("CX_SoundBuffer") << "clearChannel(): Specified channel does not exist.";
		return false;
	}

	for (size_t samp = 0; samp < _data.size(); samp++) {
		if (samp % _channels == channel) {
			_data[samp] = 0.0f;
		}
	}

	return true;
}

/*! Delete the specified channel from the data.
\param channel A 0-indexed index of the channel to delete.
\return `true` if there were no errors.
*/
bool CX_SoundBuffer::deleteChannel(unsigned int channel) {

	if (channel >= _channels) {
		CX::Instances::Log.error("CX_SoundBuffer") << "deleteChannel(): Specified channel does not exist.";
		return false;
	}

	std::vector<float> dataCopy;
	dataCopy.resize(_data.size() - getLengthSF()); // sampleFrames samples in 1 channel

	size_t copyIndex = 0;
	for (size_t samp = 0; samp < _data.size(); samp++) {
		if (samp % _channels != channel) {
			dataCopy[copyIndex++] = _data[samp];
		}
	}

	_channels--;
	_data = dataCopy;
	return true;
}

/*!
Sets the number of channels of the sound. Depending on the old number of channels (`O`) and the new number of channels (`N`),
the conversion is performed in different ways. The cases in this list are evaluated in order and only case is executed, so a
later case cannot be reached if an earlier case has already evaluated to true. When a case says anything about the average
of existing data, it means the average on a sample-by-sample basis, not the average of all the samples.

+ If `O == N`, nothing happens.
+ If `O == 0`, the number of channels is just set to N. However, `O == 0` usually means that there is no sound data 
available, so changing the number of channels is kind of meaningless.
+ If `N == 0`, the CX_SoundBuffer is cleared: all data is deleted. If you have no channels, you cannot have data in those channels.
+ If `O == 1`, each of the `N` new channels is set equal to the value of the single old channel.
+ If `N == 1`, and `average == true` the new channel is set equal to the average of the `O` old channels. If `average == false`, 
all but the first channel are removed.
+ If `N > O`, the first `O` channels are preserved unchanged. If `average == true`, the `N - O` new channels are set to the average of the `O` old channels.
If `average == false`, the `N - O` new channels are set to 0.
+ If `N < O`, and `average == false`, the data from the `O - N` to-be-removed channels is discarded.
If `average == true` the data from the `O - N` to-be-removed channels are averaged and added on to the `N` remaining channels.
The averaging is done in an unusual way, so that the average intensitity of the kept channels is equal to the average intensity of the removed channels.
An example to show why this is done:
Assume that you have 3 channels -- a, b, and c -- and are switching to 2 channels, removing c. 
The average of c is just c, so when c is added to a and b, you now have c in 2 channels,
whereas it was just in 1 channel originally: (a + c) + (b + c) = a + b + 2c. Thus, the final intensity of c is double what it should be.
What we want to do is scale c down by the number of channels it is being added to so that the total amount of c is equal
both before and after changing the number of channels, so you divide c by the number of channels it is being added to (2).
Now, (a + c/2) + (b + c/2) = a + b + c.
However, there is another problem, which is that abs(a + c/2) can be greater than 1 even if the absolute value of both is not greater than 1.
Now we need to scale each sample so that it is constrained to the proper range.
We do that by taking the ratio of the new and old channels and multiplying the samples by that ratio. `N/O = 2/3` in the example.
Now we have 2/3 * (a + c/2) = 2/3 * a + 1/3 * c, which is bounded between -1 and 1, as long as a and c are both bounded.
Also, 2/3 * [(a + c/2) + (b + c/2)] = 2a/3 + 2b/3 + 2c/3, so the ratios of the components of the original sound are constant in the final sound.

\param N The number of channels the CX_SoundBuffer will have after the conversion.
\param average If `true` and case `N < O` is reached, then the `O - N` old channels that are being removed will be averaged and this
average will be added back into the `N` remaining channels. If `false` (the default), the channels that are being removed will actually be removed.

\return `true` if the conversion was successful, `false` if the attempted conversion is unsupported.
*/
bool CX_SoundBuffer::setChannelCount(unsigned int N, bool average) {

	unsigned int O = _channels; //Old number of channels

	if (O == N) {
		return true;
	}

	if (O == 0) {
		//If there are no old channels, just set the channel count to the new value. There can be no data to copy to new channels,
		//because there were no channels to contain the data.
		_channels = N;
		return true;
	}

	if (N == 0) {
		//If there are no new channels, just clear the data
		this->clear();
		return true;
	}

	if (O == 1) {
		//Mono to anything is easy: Just copy the data to new channels.

		unsigned int originalSize = _data.size();
		_data.resize( _data.size() * N);

		for (unsigned int samp = 0; samp < originalSize; samp++) {
			for (unsigned int ch = 0; ch < N; ch++) {
				unsigned int destIndex = _data.size() - 1 - (samp * N) - ch;
				unsigned int sourceIndex = originalSize - samp - 1;
				_data[destIndex]  = _data[sourceIndex];
			}
		}

		_channels = N;
		return true;
	}

	if (N == 1) {
		std::vector<float> newSoundData(this->getLengthSF());

		//Anything to mono is easy: just average all sample frames.
		if (average) {
			for (unsigned int outputSamp = 0; outputSamp < newSoundData.size(); outputSamp++) {
				float avg = 0;
				for (unsigned int ch = 0; ch < _channels; ch++) {
					avg += _data[(outputSamp * _channels) + ch];
				}
				avg /= (float)_channels;

				newSoundData[outputSamp] = avg;
			}
		} else {
			//Remove all but the first channel
			for (unsigned int outputSamp = 0; outputSamp < newSoundData.size(); outputSamp++) {
				newSoundData[outputSamp] = _data[(outputSamp * _channels) + 0];
			}

		}

		_channels = N;
		_data = newSoundData;
		return true;
	}

	if (N > O) {

		std::vector<float> newSoundData( this->getLengthSF() * N);

		if (average) {
			//New channels set to average of existing channels
			for (unsigned int sample = 0; sample < getLengthSF(); sample++) {
				float average = 0;
				for (unsigned int oldChannel = 0; oldChannel < _channels; oldChannel++) {
					float samp = _data[ (sample * _channels) + oldChannel ];
					average += samp;
					newSoundData[ (sample * N) + oldChannel ] = samp;
				}
				average /= _channels;

				for (unsigned int newChannel = _channels; newChannel < N; newChannel++) {
					newSoundData[ (sample * N) + newChannel ] = average;
				}
			}
		} else {
			//Silence new channels
			std::vector<float> newSoundData( this->getLengthSF() * N);
			for (unsigned int sample = 0; sample < getLengthSF(); sample++) {
				for (unsigned int oldChannel = 0; oldChannel < _channels; oldChannel++) {
					newSoundData[ (sample * N) + oldChannel ] = _data[ (sample * _channels) + oldChannel ];
				}
				for (unsigned int newChannel = _channels; newChannel < N; newChannel++) {
					newSoundData[ (sample * N) + newChannel ] = 0;
				}
			}
		}

		_channels = N;
		_data = newSoundData;

		return true;
	}

	if (N < O) {
		std::vector<float> newSoundData( this->getLengthSF() * N);

		if (average) {
			//the data from the `O - N` to-be-removed channels are averaged and added on to the `N` remaining channels

			//Scaling factors
			float sigma = (float)N / (float)O;
			float gamma = 1.0 / (float)N;

			for (unsigned int sampleFrame = 0; sampleFrame < this->getLengthSF(); sampleFrame++) {

				//Get the sum based on the to be removed channels
				float sum = 0;
				for (unsigned int oldChannel = O - 1; oldChannel >= N; oldChannel--) {
					sum += _data[ (sampleFrame * _channels) + oldChannel ];
				}

				//Add the average of the old data to the remaining channels, maintaining equal ratios
				for (unsigned int keptChannel = 0; keptChannel < N; keptChannel++) {
					float samp = _data[ (sampleFrame * _channels) + keptChannel ];
					newSoundData[ (sampleFrame * N) + keptChannel ] = (samp + (sum * gamma)) * sigma;
				}
			}

		} else {
			//the data from the `O - N` to-be-removed channels is discarded.
			for (unsigned int sampleFrame = 0; sampleFrame < this->getLengthSF(); sampleFrame++) {
				for (unsigned int retainedChannel = 0; retainedChannel < N; retainedChannel++) {
					newSoundData[ (sampleFrame * N) + retainedChannel ] = _data[ (sampleFrame * _channels) + retainedChannel ];
				}
			}
		}

		_channels = N;
		_data = newSoundData;

		return true;
	}
	/*
	CX::Instances::Log.error("CX_SoundBuffer") << "Sound cannot be set to the given number of channels. There is no known conversion from " <<
						_channels << " channels to " << N <<
						" channels. You will have to do it manually. Use getRawDataReference() to access the sound data." << endl;

	return false;
	*/
	return false;
}

/*! \brief Returns the number of channels in the sound data stored in this CX_SoundBuffer. */
unsigned int CX_SoundBuffer::getChannelCount(void) const {
	return _channels; 
}

/*!
Resamples the audio data stored in the CX_SoundBuffer by linear interpolation. Linear interpolation is not the ideal
way to resample audio data; some audio fidelity is lost, more so than with other resampling techinques. It is, however,
very fast compared to higher-quality methods both in terms of run time and programming time. It has acceptable results,
at least when the new sample rate is similar to the old sample rate.

\param newSampleRate The requested sample rate.
*/
void CX_SoundBuffer::resample(float newSampleRate) {
	if (newSampleRate == _sampleRate) {
		return;
	}

	if (_sampleRate == 0) {
		_sampleRate = newSampleRate;
		Instances::Log.notice("CX_SoundBuffer") << "resample(): The previous sample rate was 0, which is invalid. No resampling was performed, but the new sample rate was set.";
		return;
	}

	SampleFrame oldSFCount = getLengthSF();
	SampleFrame newSFCount = SampleFrame(oldSFCount * ((double)newSampleRate / _sampleRate));
	double sfMult = 1.0 / (double)newSFCount;

	std::vector<float> completeNewData((size_t)newSFCount * _channels);

	for (unsigned int channel = 0; channel < _channels; channel++) {

		for (SampleFrame sf = 0; sf < newSFCount; sf++) {
			double time = sf * sfMult;

			double oldIndex = time * oldSFCount;
			double oldIndexOffset = fmod( oldIndex, 1.0 );

			size_t i1 = (size_t)floor(oldIndex);
			size_t i2 = i1 + 1;

			//There is a little fudge at the end. If the last sample would be past the end of the old data, reuse the
			//previous sample. This is technically an error, but at the last sample it can't possibly have a meaningful effect.
			if (i2 >= oldSFCount) {
				i2 = i1;
			}

			float s1 = _data[ (i1 * _channels) + channel ];
			float s2 = _data[ (i2 * _channels) + channel ];
			float linInt = s1 + (s2 - s1)*(float)oldIndexOffset;

			completeNewData[(sf * _channels) + channel] = linInt;
		}
	}

	_data = completeNewData;

	_sampleRate = newSampleRate;

}


/*! Get the sample rate of the sound data stored in this CX_SoundBuffer.
*/
float CX_SoundBuffer::getSampleRate(void) const { 
	return _sampleRate; 
}

/*! Get a reference to the raw data underlying the CX_SoundBuffer. The raw data are a `vector<float>`.

\return A reference to the data. Modify at your own risk! */
std::vector<float>& CX_SoundBuffer::getRawDataReference(void) { 
	return _data; 
}

float CX_SoundBuffer::getSample(unsigned int channel, SampleFrame sf) const {
	size_t index = (sf * _channels) + channel;
	return _data.at(index);
}

void CX_SoundBuffer::setSample(unsigned int channel, SampleFrame sf, float val) {
	size_t index = (sf * _channels) + channel;
	_data.at(index) = val;
}

/*! Reverses the sound data stored in the CX_SoundBuffer to play in reverse. */
void CX_SoundBuffer::reverse(void) {
	std::vector<float> copy = _data;
	unsigned int sampleFrameCount = getLengthSF();
	for (unsigned int sf = 0; sf < sampleFrameCount; sf++) {
		unsigned int targetSampleFrame = sf*_channels;
		unsigned int sourceSampleFrame = (sampleFrameCount - 1 - sf) * _channels;

		for (unsigned int ch = 0; ch < _channels; ch++) {
			_data[targetSampleFrame + ch] = copy[sourceSampleFrame + ch];
		}
	}
}

/*! Changes the speed of the sound by some multiple. 
Does not do pitch compensation (e.g. faster sounds will be higher pitch).

\param speedMultiplier Amount to multiply the speed by. Must be greater than 0.
\note If you would like to use a negative value to reverse the direction of playback, see reverse().
*/
void CX_SoundBuffer::multiplySpeed(float speedMultiplier) {
	if (speedMultiplier <= 0) {
		return;
	}

	float sampleRate = this->_sampleRate;
	this->resample( this->getSampleRate() / speedMultiplier );
	this->_sampleRate = sampleRate;
}

/*! Apply gain in terms of decibels. The resulting values are clamped to be within [-1, 1].

\param decibels Gain to apply. 0 does nothing. Positive values increase volume, negative values decrease volume. 
Negative infinity is essentially mute, although see `multiplyAmplitudeBy()` for a more obvious way to mute.
\param channel The channel that the gain should be applied to. If channel is less than 0, the gain is applied to all channels.
*/
bool CX_SoundBuffer::applyGain(float decibels, int channel) {
	float amplitudeMultiplier = sqrt( pow(10.0f, decibels/10.0f) );
	return multiplyAmplitudeBy( amplitudeMultiplier, channel );
}

/*! Apply gain in terms of amplitude. The original value is simply multiplied by `amount` 
and then clamped to be within [-1, 1].

\param amount The gain that should be applied. A value of 0 mutes the channel. 
1 does nothing. 2 doubles the amplitude. -1 inverts the waveform.
\param channel The channel that the given multiplier should be applied to. If channel 
is less than 0, the amplitude multiplier is applied to all channels.
*/
bool CX_SoundBuffer::multiplyAmplitudeBy(float amount, int channel) {

	if (channel >= (int)_channels) {
		return false;
	}

	if (channel < 0) {
		//Apply to all channels
		for (unsigned int i = 0; i < _data.size(); i++) {
			_data[i] = Util::clamp<float>(_data[i] * amount, -1, 1);
		}

	} else {
		//Apply gain to the given channel
		for (unsigned int sf = 0; sf < (unsigned int)getLengthSF(); sf++) {
			unsigned int index = (sf * _channels) + channel;
			_data[index] = Util::clamp<float>(_data[index] * amount, -1, 1);
		}

	}

	return true;
}


size_t CX_SoundBuffer::_timeToSample(CX_Millis time) const {
	return _channels * timeToSF(time);
}

/*! Get the sample frame at the given time offset in the buffer.
This function is just `SF = time * getSampleRate()`.
\param time The time to calculate the sample frame for.
\return The sample frame. This value may be greater than 
the total number of sample frames in the sound buffer.
*/
SampleFrame CX_SoundBuffer::timeToSF(CX_Millis time) const {
	return this->getSampleRate() * time.seconds();
}

/*! Get the time at the given sample frame.
\param sf The sample frame to calculate the time for.
\return The time. This value may be greater than
the total length of the sound buffer.
*/
CX_Millis CX_SoundBuffer::sfToTime(SampleFrame sf) const {
	double secPerSF = 1.0 / this->getSampleRate();
	return CX_Seconds(secPerSF * sf);
}

/*! \brief Return a copy of the contents of this `CX_SoundBuffer` copied into an `ofSoundBuffer`. */
ofSoundBuffer CX_SoundBuffer::toOfSoundBuffer(void) {
	ofSoundBuffer rval;
	rval.copyFrom(_data, _channels, _sampleRate);
	return rval;
}

/*! \brief Set the contents of this `CX_SoundBuffer` from an `ofSoundBuffer`. */
bool CX_SoundBuffer::fromOfSoundBuffer(const ofSoundBuffer& buf) {
	return this->setFromVector(buf.getSampleRate(), buf.getNumChannels(), buf.getBuffer());
}

/*
CX_SoundBuffer& CX_SoundBuffer::operator=(const ofSoundBuffer& buf) {
	this->fromOfSoundBuffer(buf);
	return *this;
}

CX_SoundBuffer::operator ofSoundBuffer(void) {
	return this->toOfSoundBuffer();
}
*/

// Rough resizing utility. Keeps existing data as possible.
// Unused data are discarded.
void CX_SoundBuffer::_resize(unsigned int newChannels, SampleFrame newSF, bool zero) {

	SampleFrame oldSF = getLengthSF();

	unsigned int oldChannels = _channels;
	_channels = newChannels;
	
	// ---- Can't call functions that depend on _channels being the old channel count below here ---

	// If no old data, just resize
	if (_data.size() == 0 || oldChannels == 0 || oldSF == 0) {
		if (zero) {
			_data.resize(newSF * newChannels, 0.0f);
		} else {
			_data.resize(newSF * newChannels);
		}
		return;
	}

	// If no new data, just shrink
	if (newChannels == 0 || newSF == 0) {
		_data.resize(0);
		return;
	}

	// If the number of channels is the same, channel data do not need to be moved.
	if (newChannels == oldChannels) {

		if (newSF != oldSF) {
			if (zero) {
				_data.resize(newSF * newChannels, 0.0f);
			} else {
				_data.resize(newSF * newChannels);
			}
		}

		return;
	}

	// If the channel counts don't match, then you have to move channel data
	std::vector<float> newData;
	if (zero) {
		newData = std::vector<float>(newChannels * newSF, 0.0f);
	} else {
		newData = std::vector<float>(newChannels * newSF);
	}

	// The number of samples frames and channels to iterate over 
	// is the smaller (shorter length) of the old and new.
	// Can't iterate over what isn't there.
	SampleFrame itSF = std::min(oldSF, newSF);
	unsigned int itChannels = std::min(oldChannels, newChannels);

	for (SampleFrame sf = 0; sf < itSF; sf++) {
		for (unsigned int ch = 0; ch < itChannels; ch++) {

			size_t newInd = (sf * newChannels) + ch;
			size_t oldInd = (sf * oldChannels) + ch;

			newData[newInd] = _data[oldInd];
		}
	}

	_data = newData;
}

/*! Writes the contents of the sound buffer to a WAV file with the given file name. The data will
be encoded as 16-bit PCM. The sample rate is determined by the sample rate of the sound buffer.

\param fileName The name of the file to save the sound data to. `fileName` should have a .wav extension. If it does not,
".wav" will be appended to the file name and a warning will be logged.
\return `true` for successfully saving the file. `false` if there was an error while opening the file. If so, an error will be logged.
*/
bool CX_SoundBuffer::writeToFile(std::string fileName) {
	//This function was taken from the ofSoundFile additions suggested here: https://github.com/openframeworks/openFrameworks/pull/2626
	//From this file: https://github.com/admsyn/openFrameworks/blob/feature-sound-objects/libs/openFrameworks/sound/ofSoundFile.cpp
	//There were some modifications to get it to work with the data structure of CX_SoundBuffer.

	// check that we're writing a wav and complain if the file extension is wrong.
	ofFile f(fileName);
	if (ofToLower(f.getExtension()) != "wav") {
		fileName += ".wav";
		CX::Instances::Log.warning("CX_SoundBuffer") << "writeToFile(): Can only write wav files - will save file as " << fileName;
	}

	std::fstream file(ofToDataPath(fileName).c_str(), std::ios::out | std::ios::binary);
	if (!file.is_open()) {
		CX::Instances::Log.error("CX_SoundBuffer") << "writeToFile(): Error opening sound file \"" << fileName << "\" for writing.";
		return false;
	}

	// write a wav header
	short myFormat = 1; // for pcm
	int mySubChunk1Size = 16;
	int bitsPerSample = 16; // assume 16 bit pcm

	int channels = this->getChannelCount();
	int samplerate = this->getSampleRate();
	unsigned int bufferSize = this->getLengthSamples();

	int myByteRate = samplerate * channels * bitsPerSample / 8;
	short myBlockAlign = channels * bitsPerSample / 8;
	int myChunkSize = 36 + bufferSize*bitsPerSample / 8;
	int myDataSize = bufferSize*bitsPerSample / 8;


	file.seekp(0, std::ios::beg);
	file.write("RIFF", 4);
	file.write((char*)&myChunkSize, 4);
	file.write("WAVE", 4);
	file.write("fmt ", 4);
	file.write((char*)&mySubChunk1Size, 4);
	file.write((char*)&myFormat, 2); // should be 1 for PCM
	file.write((char*)&channels, 2); // # channels (1 or 2)
	file.write((char*)&samplerate, 4); // 44100
	file.write((char*)&myByteRate, 4); //
	file.write((char*)&myBlockAlign, 2);
	file.write((char*)&bitsPerSample, 2); //16
	file.write("data", 4);
	file.write((char*)&myDataSize, 4);

	// write the wav file per the wav file format, 4096 bytes of data at a time.
#define WRITE_BUFF_SIZE 4096

	short writeBuff[WRITE_BUFF_SIZE];
	unsigned int pos = 0;
	while (pos < bufferSize) {
		int len = MIN(WRITE_BUFF_SIZE, bufferSize - pos);
		for (int i = 0; i < len; i++) {
			writeBuff[i] = (int)(this->_data[pos] * 32767.f);
			pos++;
		}
		file.write((char*)writeBuff, len*bitsPerSample / 8);
	}

	file.close();
	return true;
}

} //namespace CX
