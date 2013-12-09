#include "CX_SoundObject.h"

using namespace CX;

/*!
Loads a sound file with the given file name into the CX_SoundObject. Any pre-existing data in the sound object is deleted.
Some sound file types are supported. Others are not. In the limited testing, mp3 and wav files seem to work well.
If the file cannot be loaded, descriptive error messages will be logged.

@param fileName Name of the sound file to load.

@return True if the sound given in the fileName was loaded succesffuly, false otherwise.
*/
bool CX_SoundObject::loadFile (string fileName) {
	_successfullyLoaded = true;

	ofFmodSoundPlayer fmPlayer;
	bool loadSuccessful = fmPlayer.loadSound(fileName, false);
	if (!loadSuccessful) {
		ofLogError("CX_SoundObject") << "Error loading " << fileName;
		fmPlayer.unloadSound(); //Just in case.
		_successfullyLoaded = false;
		return _successfullyLoaded;
	}

	FMOD_SOUND *fmSound = fmPlayer.sound;

	FMOD_SOUND_TYPE soundType; //I'm not sure what to do with this, either. Check that it isn't a bizzare unreadable type, maybe?
	FMOD_SOUND_FORMAT soundFormat;
	int channels;
	int bits; //I'm not really sure what to do with this that soundFormat doesn't give me.

	FMOD_RESULT formatResult = FMOD_Sound_GetFormat( fmSound, &soundType, &soundFormat, &channels, &bits );
	if (formatResult != FMOD_OK) {
		ofLogError("CX_SoundObject") << "Error getting sound format of " << fileName;
		fmPlayer.unloadSound();
		_successfullyLoaded = false;
		return _successfullyLoaded;
	}

	_soundChannels = channels;
	_soundFormat = soundFormat;

	//Find out the sample rate of the sound. To be used if it needs to be resampled.
	//At least, I think this gets the sample rate of the sound. The documentation isn't clear.
	FMOD_Sound_GetDefaults( fmSound, &_soundSampleRate, NULL, NULL, NULL );

	void *ptr1;
	void *ptr2;
	unsigned int length1;
	unsigned int length2;

	switch (soundFormat) {
	case FMOD_SOUND_FORMAT_PCM8:
		ofLogError("CX_SoundOutput") << "File " << fileName << " is in an unsupported format (8-bit PCM). FMOD_SOUND_FORMAT_PCM8 not yet supported.";
		_successfullyLoaded = false;
		break;
	case FMOD_SOUND_FORMAT_PCM16:
		{
			unsigned int samplesToRead =  fmPlayer.length; // * channelCount ????

			FMOD_RESULT lockResult = FMOD_Sound_Lock( fmSound, 0, samplesToRead, &ptr1, &ptr2, &length1, &length2 );

			if (lockResult == FMOD_OK) {
				unsigned int totalSamples = length1 * channels; //The documentation says that length1 is in bytes, but it seems to be wrong. 
				//It is in samples for everything I've found (which have been entirely 16-bit PCM).
				_soundData.resize( totalSamples );
				
				for (unsigned int i = 0; i < totalSamples; i++) {
					_soundData[i] = ((float)(((int16_t*)ptr1)[i]))/32768;
				}
			} else {
				ofLogError("CX_SoundOutput") << "Error locking sound data";
				_successfullyLoaded = false;
			}

			FMOD_RESULT unlockResult = FMOD_Sound_Unlock( fmSound, ptr1, ptr2, length1, length2 );
		}
		break;
	case FMOD_SOUND_FORMAT_PCM24:
		//This is annoying because the sign must be extended. Maybe should also be processed as 64-bit float.
		ofLogError("CX_SoundOutput") << "File " << fileName << " is in an unsupported format (24-bit PCM). FMOD_SOUND_FORMAT_PCM24 not yet supported.";
		_successfullyLoaded = false;
		break;
	case FMOD_SOUND_FORMAT_PCM32:
		//This is annoying because it must be processed as 64-bit float before being converted back to 32-bit float.
		ofLogError("CX_SoundOutput") << "File " << fileName << " is in an unsupported format (32-bit PCM). FMOD_SOUND_FORMAT_PCM32 not yet supported.";
		_successfullyLoaded = false;
		break;
	case FMOD_SOUND_FORMAT_PCMFLOAT:
		//This is awesome because I don't have to do anything at all with it!
		{
			unsigned int samplesToRead = fmPlayer.length; // * channelCount ????

			FMOD_RESULT lockResult = FMOD_Sound_Lock( fmSound, 0, samplesToRead, &ptr1, &ptr2, &length1, &length2 );

			if (lockResult == FMOD_OK) {
				unsigned int totalSamples = length1 * channels;
				_soundData.resize( totalSamples );
				memcpy(_soundData.data(), ptr1, totalSamples * sizeof(float));
			} else {
				ofLogError("CX_SoundOutput") << "Error locking sound data";
				_successfullyLoaded = false;
			}

			FMOD_RESULT unlockResult = FMOD_Sound_Unlock( fmSound, ptr1, ptr2, length1, length2 );
		}
		break;
	case FMOD_SOUND_FORMAT_NONE:
		ofLogError("CX_SoundOutput") << "File " << fileName << " of unknown format.";
		_successfullyLoaded = false;
		break;
	case FMOD_SOUND_FORMAT_GCADPCM:
	case FMOD_SOUND_FORMAT_IMAADPCM:
	case FMOD_SOUND_FORMAT_VAG:
	case FMOD_SOUND_FORMAT_XMA:
	case FMOD_SOUND_FORMAT_MPEG:
		ofLogError("CX_SoundOutput") << "File " << fileName << " is of unsupported file format (compressed/video game console). There as no plans to ever support these formats.";
		_successfullyLoaded = false;
		break;
	};

	//Clean up by unloading this sound.
	fmPlayer.unloadSound();

	if (_successfullyLoaded) {
		name = fileName;
	}

	return _successfullyLoaded;
}

/*!
Uses loadFile(string) and addSound(CX_SoundObject, uint64_t) to add the given file to the current CX_SoundObject at the given time offset (in microseconds).
See those functions for more information.

@param fileName Name of the sound file to load.
@param timeOffset Time at which to add the new sound.

@return Returns true if the new sound was added sucessfully, false otherwise.
*/
bool CX_SoundObject::addSound (string fileName, uint64_t timeOffset) {
	if (_soundData.size() == 0 || !this->_successfullyLoaded) {
		bool loadSuccess = this->loadFile(fileName);
		if (loadSuccess) {
			this->addSilence(timeOffset, true);
		}
		return loadSuccess;
	} else {
		CX_SoundObject temp;
		if (!temp.loadFile(fileName)) {
			return false;
		}
		addSound(temp, timeOffset);
		this->_successfullyLoaded = true; //What is this doing here?
		return true;
	}
}

/*!
Adds the sound data in nso at the time offset. If the sample rates of the sounds differ, nso will be resampled to the sample rate of this CX_SoundObject.
If the number of channels of nso does not equal the number of channels of this, an attempt will be made to set the number of channels of
nso equal to the number of channels of this CX_SoundObject.
The data from nso and this CX_SoundObject are merged by adding the amplitudes of the sounds. The result of the addition is clamped between -1 and 1.

@param so A sound object. Must be ready().
@param timeOffset Time at which to add the new sound data in microseconds. Dependent on sample rate.

@return True if nso was successfully added to this CX_SoundObject, false otherwise.
*/
bool CX_SoundObject::addSound (CX_SoundObject nso, uint64_t timeOffset) {
	if (!nso.ready()) {
		return false;
	}

	if (nso.getSampleRate() != this->getSampleRate()) {
		nso.resample( this->getSampleRate() );
	}

	if (nso.getChannelCount() != this->getChannelCount()) {
		if (!nso.setChannelCount( this->getChannelCount() )) {
			return false;
		}
	}

	if (this->name == "") {
		this->name = nso.name;
	}

	//Time is in microseconds, so do samples/second * seconds * channels to get the (absolute) sample at which the new sound starts.
	unsigned int insertionSample = (unsigned int)(this->getSampleRate() * ((double)timeOffset / 1000000) * nso.getChannelCount());

	//Get the new data that will be merged.
	vector<float> &newData = nso.getRawDataReference();
	
	//If this sound isn't long enough to hold the new data, resize it to fit.
	if (insertionSample + newData.size() > this->_soundData.size()) {
		_soundData.resize( insertionSample + newData.size(), 0 ); //When resizing, set any new elements to silence (i.e. 0).
	}

	//Copy over the new data, clamping as needed.
	for (unsigned int i = 0; i < newData.size(); i++) {
		float newVal = _soundData[insertionSample + i] + newData[i];
		newVal = std::max(newVal, -1.0f);
		newVal = std::min(newVal, 1.0f);
		_soundData[insertionSample + i] = newVal;
	}

	return true;
}

/*!
Set the length of the sound to the specified length in microseconds. If the new length is longer than the old length,
the new data is zeroed (i.e. set to silence).
*/
void CX_SoundObject::setLength (uint64_t lengthInMicroseconds) {
	double endOfDurationSample = getSampleRate() * ((double)lengthInMicroseconds / 1000000) * getChannelCount();
	_soundData.resize( (unsigned int)endOfDurationSample, 0 );
}

/*!
Gets the length of the current sound in microseconds.
*/
uint64_t CX_SoundObject::getLength (void) {
	return ((uint64_t)_soundData.size() * 1000000)/(getChannelCount() * (uint64_t)getSampleRate());
}



float CX_SoundObject::getPositivePeak (void) {
	float peak = numeric_limits<float>::min();
	for (unsigned int i = 0; i < _soundData.size(); i++) {
		if (_soundData[i] > peak) {
			peak = _soundData[i];
		}
	}
	return peak;
}

float CX_SoundObject::getNegativePeak (void) {
	float peak = numeric_limits<float>::max();
	for (unsigned int i = 0; i < _soundData.size(); i++) {
		if (_soundData[i] < peak) {
			peak = _soundData[i];
		}
	}
	return peak;
}

/*!
Removes leading "silence" from the sound, where silence is defined by the given tolerance. It is unlikely that
the beginning of a sound, even if perceived as silent relative to the rest of the sound, has an amplitude of 0.
Therefore, a tolerance of 0 is unlikely to prove useful. Using getPositivePeak() and/or getNegativePeak() can
help to give a reference amplitude of which some small fraction is perceived as "silent".

@param tolerance All sound data up to and including the first instance of a sample with an amplitude
with an absolute value greater than tolerance is removed from the sound.
*/
void CX_SoundObject::stripLeadingSilence (float tolerance) {
	for (unsigned int concurrentSample = 0; concurrentSample < _soundData.size()/_soundChannels; concurrentSample++) {
		for (int channel = 0; channel < _soundChannels; channel++) {
			unsigned int index = (concurrentSample * _soundChannels) + channel;

			if (abs(_soundData.at(index)) > tolerance) {
				_soundData.erase(_soundData.begin(), _soundData.begin() + (concurrentSample * _soundChannels));
				break;
			}

		}
	}
}

/*!
Adds the specified amount of silence to the CX_SoundObject at either the beginning or end.

@param durationUs Duration of added silence in microseconds. Dependent on the sample rate of the sound. If the sample rate changes,
so does the duration of silence.
@param atBeginning If true, silence is added at the beginning of the CX_SoundObject. If false, the silence is added at the end.
*/
void CX_SoundObject::addSilence (uint64_t durationUs, bool atBeginning) {
	//Time is in microseconds, so do samples/second * channels * seconds to get the absolute sample count for the new silence.
	unsigned int absoluteSampleCount = (unsigned int)(getSampleRate() * getChannelCount() * ((double)durationUs / 1000000) );

	if (atBeginning) {
		_soundData.insert( _soundData.begin(), absoluteSampleCount, 0 );
	} else {
		_soundData.insert( _soundData.end(), absoluteSampleCount, 0 );
	}
}

/*!
Deletes the specified amount of sound from the CX_SoundObject from either the beginning or end.

@param durationUs Duration of removed sound in microseconds. Dependent on the sample rate of the sound. If the sample rate changes,
so does the duration of removed sound.
@param fromBeginning If true, sound is deleted from the beginning of the CX_SoundObject's buffer. 
If false, the sound is deleted from the end, toward the beginning.
*/
void CX_SoundObject::deleteAmount (uint64_t durationUs, bool fromBeginning) {
	//Time is in microseconds, so do samples/second * channels * seconds to get the absolute sample count to delete.
	unsigned int absoluteSampleCount = (unsigned int)(getSampleRate() * getChannelCount() * ((double)durationUs / 1000000) );

	if (absoluteSampleCount >= _soundData.size()) {
		_soundData.clear();
	} else {
		if (fromBeginning) {
			_soundData.erase( _soundData.begin(), _soundData.begin() + absoluteSampleCount );
		} else {
			_soundData.erase( _soundData.end() - absoluteSampleCount, _soundData.end() );
		}
	}
}

/*!
Sets the number of channels of the sound. Depending on the old number of channels (N) and the new number of channels (M),
the conversion is performed in different ways.
If N == M, nothing happens.
If N == 1, each of the M new channels is set equal to the value of the single old channel. 
If M == 1, the new channel is set equal to the arithmetic average of the N old channels. 
If (N != 1 && M != 1 && M > N), the first N channels are preserved unchanged and the M - N new channels are set to the arithmetic average of the N old channels. 
Any other combination of M and N is an error condition.

@param newChannelCount The number of channels the CX_SoundObject will have after the conversion.

@return True if the conversion was successful, false if the attempted conversion is unsupported.
*/
bool CX_SoundObject::setChannelCount (int newChannelCount) {
	if (newChannelCount == _soundChannels) {
		return true;
	} else if (_soundChannels == 1) {
		//Mono to anything is easy: Just multiply the data.

		unsigned int originalSize = _soundData.size();
		_soundData.resize( _soundData.size() * newChannelCount );
		
		for (unsigned int samp = 0; samp < originalSize; samp++) {
			for (unsigned int ch = 0; ch < newChannelCount; ch++) {
				unsigned int destIndex = _soundData.size() - 1 - (samp * newChannelCount) - ch;
				unsigned int sourceIndex = originalSize - samp - 1;
				_soundData[destIndex]  = _soundData[sourceIndex];
			}
		}

		_soundChannels = newChannelCount;
		return true;
	} else if (newChannelCount == 1) {
		//Anything to mono is easy (to do a bad job of): just average all concurrent samples.
		vector<float> newSoundData( _soundData.size()/_soundChannels );
		for (unsigned int outputSamp = 0; outputSamp < newSoundData.size(); outputSamp++) {
			float sampleSum = 0;

			for (int i = 0; i < _soundChannels; i++) {
				sampleSum += _soundData[ (outputSamp * _soundChannels) + i ];
			}

			newSoundData[ outputSamp ] = sampleSum/_soundChannels;
		}

		_soundChannels = newChannelCount;
		_soundData = newSoundData;
		return true;
	} else if (newChannelCount > _soundChannels) {
		//If the data needs to be extended to an arbitrary number of channels, one way to solve the problem is to
		//set the new channels to silence or to an average of the existing channels.

		//Silence
		/*
		vector<float> newSoundData( (_soundData.size()/_soundChannels) * newChannelCount );
		for (unsigned int sample = 0; sample < getConcurrentSampleCount(); sample++) {
			for (int oldChannel = 0; oldChannel < _soundChannels; oldChannel++) {
				newSoundData[ (sample * newChannelCount) + oldChannel ] = _soundData[ (sample * _soundChannels) + oldChannel ];
			}
			for (int newChannel = _soundChannels; newChannel < newChannelCount; newChannel++) {
				newSoundData[ (sample * newChannelCount) + newChannel ] = 0;
			}
		}
		_soundData = newSoundData;
		*/

		//Average
		vector<float> newSoundData( (_soundData.size()/_soundChannels) * newChannelCount );
		for (unsigned int sample = 0; sample < getConcurrentSampleCount(); sample++) {
			float average = 0;
			for (int oldChannel = 0; oldChannel < _soundChannels; oldChannel++) {
				float samp = _soundData[ (sample * _soundChannels) + oldChannel ];
				average += samp;
				newSoundData[ (sample * newChannelCount) + oldChannel ] = samp;
			}
			average /= _soundChannels;

			for (int newChannel = _soundChannels; newChannel < newChannelCount; newChannel++) {
				newSoundData[ (sample * newChannelCount) + newChannel ] = average;
			}
		}
		_soundData = newSoundData;
		return true;
	} else if (newChannelCount < _soundChannels) {
		//In this case, it is unclear how to proceed. One simple solution is to strip off the old channels that
		//won't be used any more. Another is to average the removed channels and add that data back into the channels
		//that will be staying.
	}

	ofLogError("CX_SoundObject") << "Sound cannot be set to the given number of channels. There is no known conversion from " <<
						_soundChannels << " channels to " << newChannelCount << 
						" channels. You will have to do it manually. Use getRawDataReference() to access the sound data." << endl;

	return false;
}

/*!
Resamples the audio data stored in the CX_SoundObject by linear interpolation. Linear interpolation is not the ideal
way to resample audio data; some audio fidelity is lost, more so than with other resampling techinques. It is, however, 
very fast compared to higher-quality methods both in terms of run time and programming time. It has acceptable results, 
at least when the new sample rate is similar to the old sample rate.

@param newSampleRate The requested sample rate.
*/
void CX_SoundObject::resample (float newSampleRate) {

	//Using SRC	
	/*
	SRC_DATA data;
	data.src_ratio = (double)newSampleRate/_soundSampleRate;

	long requiredSamples = (long)(getConcurrentSampleCount() * ((double)newSampleRate/_soundSampleRate));
	data.output_frames = requiredSamples;
	data.data_out = new float(data.output_frames);
	
	data.input_frames = _soundData.size()/_soundChannels;
	data.data_in = _soundData.data();

	data.end_of_input = 1;
	
	int result = src_simple(&data, SRC_SINC_FASTEST, _soundChannels);

	//const char* src_strerror (int error) ;
	const char *errorString = src_strerror(result);


	vector<float> newSoundData(data.output_frames_gen);
	for (unsigned int i = 0; i < newSoundData.size(); i++) {
		newSoundData[i] = data.data_out[i];
	}

	_soundSampleRate = newSampleRate;
	*/


	
	
	uint64_t oldSampleCount = getConcurrentSampleCount();
	uint64_t newSampleCount = (uint64_t)(getConcurrentSampleCount() * ((double)newSampleRate/_soundSampleRate));

	vector<float> completeNewData((unsigned int)newSampleCount * _soundChannels);

	unsigned int lastI1 = 0;

	for (int channel = 0; channel < _soundChannels; channel++) {

		for (unsigned int sample = 0; sample < newSampleCount; sample++) {
			double time = ((double)sample)/newSampleCount;

			double oldIndex = time * oldSampleCount;
			double oldIndexOffset = fmod( oldIndex, 1.0 );

			unsigned int i1 = (unsigned int)floor(oldIndex);
			unsigned int i2 = i1 + 1;

			//There is a little fudge at the end. If the last sample would be past the end of the old data, reuse the
			//previous sample. This is technically an error, but at the last sample it can't possibly have a meaningful effect.
			if (i2 >= oldSampleCount) {
				i2 = i1;
			}

			float s1 = _soundData[ (i1 * _soundChannels) + channel ];
			float s2 = _soundData[ (i2 * _soundChannels) + channel ];
			float linInt = s1 + (s2 - s1)*(float)oldIndexOffset;

			completeNewData[(sample * _soundChannels) + channel] = linInt;
		}
	}

	_soundData = completeNewData;

	_soundSampleRate = newSampleRate;

}

/*!
Apply gain to the channel in terms of decibels.
@param decibels Gain to apply. 0 does nothing. Positive values increase volume, negative values decrease volume. Negative infinity is essentially mute,
although see multiplyAmplitudeBy() for a more obvious way to do that same operation.
@param channel The channel that the gain should be applied to. If channel is less than 0, the gain is applied to all channels.
*/
bool CX_SoundObject::applyGain (float decibels, int channel) {
	float amplitudeMultiplier = sqrt( pow(10.0f, decibels/10.0f) );
	return multiplyAmplitudeBy( amplitudeMultiplier, channel );
}

/*!
Apply gain to the sound. The original value is simply multiplied by the amount and then clamped to be within [-1, 1].
@param amount The gain that should be applied. A value of 0 mutes the channel. 1 does nothing. 2 doubles the amplitude. -1 inverts the waveform.
@param channel The channel that the given multiplier should be applied to. If channel is less than 0, the amplitude multiplier is applied to all channels.
*/
bool CX_SoundObject::multiplyAmplitudeBy (float amount, int channel) {

	if (channel >= _soundChannels) {
		return false;
	}

	if (channel < 0) {
		//Apply to all channels
		for (unsigned int i = 0; i < _soundData.size(); i++) {
			float newVal = _soundData[i] * amount;
			newVal = std::max(newVal, -1.0f);
			newVal = std::min(newVal, 1.0f);
			_soundData[i] = newVal;
		}

	} else {
		//Apply gain to the given channel
		for (unsigned int i = 0; i < (unsigned int)getConcurrentSampleCount(); i++) {
			unsigned int index = (i * _soundChannels) + channel;
			float newVal = _soundData[index] * amount;
			newVal = std::max(newVal, -1.0f);
			newVal = std::min(newVal, 1.0f);
			_soundData[index] = newVal;
		}

	}
	return true;
}

/*
float CX_SoundObject::_readSample (int channel, unsigned int sample) {
	unsigned int newIndex = (sample * _soundChannels) + channel;
	return _soundData.at( newIndex );
}

vector<float> CX_SoundObject::_getChannelData (int channel) {
	vector<float> rval( (unsigned int)getConcurrentSampleCount() );

	for (unsigned int sample = 0; sample < rval.size(); sample++) {
		rval[sample] = _soundData.at( (sample * _soundChannels) + channel );
	}

	return rval;
}
*/