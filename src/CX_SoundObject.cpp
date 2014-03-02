#include "CX_SoundObject.h"

using namespace CX;
using namespace CX::Instances;

/*!
Loads a sound file with the given file name into the CX_SoundObject. Any pre-existing data in the sound object is deleted.
Some sound file types are supported. Others are not. In the limited testing, mp3 and wav files seem to work well.
If the file cannot be loaded, descriptive error messages will be logged.

\param fileName Name of the sound file to load.

\return True if the sound given in the fileName was loaded succesffuly, false otherwise.
*/
bool CX_SoundObject::loadFile (string fileName) {
	_successfullyLoaded = true;

	ofFmodSoundPlayer fmPlayer;
	bool loadSuccessful = fmPlayer.loadSound(fileName, false);
	if (!loadSuccessful) {
		Log.error("CX_SoundObject") << "Error loading " << fileName;
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
		Log.error("CX_SoundObject") << "Error getting sound format of " << fileName;
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
		Log.error("CX_SoundOutput") << "File " << fileName << " is in an unsupported format (8-bit PCM). FMOD_SOUND_FORMAT_PCM8 not yet supported.";
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
				Log.error("CX_SoundOutput") << "Error locking sound data";
				_successfullyLoaded = false;
			}

			FMOD_RESULT unlockResult = FMOD_Sound_Unlock( fmSound, ptr1, ptr2, length1, length2 );
		}
		break;
	case FMOD_SOUND_FORMAT_PCM24:
		//This is annoying because the sign must be extended. Maybe should also be processed as 64-bit float.
		Log.error("CX_SoundOutput") << "File " << fileName << " is in an unsupported format (24-bit PCM). FMOD_SOUND_FORMAT_PCM24 not yet supported.";
		_successfullyLoaded = false;
		break;
	case FMOD_SOUND_FORMAT_PCM32:
		//This is annoying because it must be processed as 64-bit float before being converted back to 32-bit float.
		Log.error("CX_SoundOutput") << "File " << fileName << " is in an unsupported format (32-bit PCM). FMOD_SOUND_FORMAT_PCM32 not yet supported.";
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
				Log.error("CX_SoundOutput") << "Error locking sound data";
				_successfullyLoaded = false;
			}

			FMOD_RESULT unlockResult = FMOD_Sound_Unlock( fmSound, ptr1, ptr2, length1, length2 );
		}
		break;
	case FMOD_SOUND_FORMAT_NONE:
		Log.error("CX_SoundOutput") << "File " << fileName << " of unknown format.";
		_successfullyLoaded = false;
		break;
	case FMOD_SOUND_FORMAT_GCADPCM:
	case FMOD_SOUND_FORMAT_IMAADPCM:
	case FMOD_SOUND_FORMAT_VAG:
	case FMOD_SOUND_FORMAT_XMA:
	case FMOD_SOUND_FORMAT_MPEG:
	case FMOD_SOUND_FORMAT_MAX:
	case FMOD_SOUND_FORMAT_FORCEINT:
		Log.error("CX_SoundOutput") << "File " << fileName << " is of unsupported file format (compressed/video game console). There are no plans to ever support these formats.";
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

\param fileName Name of the sound file to load.
\param timeOffset Time at which to add the new sound.

\return Returns true if the new sound was added sucessfully, false otherwise.
*/
bool CX_SoundObject::addSound (string fileName, CX_Micros timeOffset) {
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
Adds the sound data in `nso` at the time offset. If the sample rates of the sounds differ, nso will be resampled to the sample rate of this CX_SoundObject.
If the number of channels of nso does not equal the number of channels of this, an attempt will be made to set the number of channels of
nso equal to the number of channels of this CX_SoundObject.
The data from `nso` and this CX_SoundObject are merged by adding the amplitudes of the sounds. The result of the addition is clamped between -1 and 1.

\param nso A sound object. Must be successfully loaded.
\param timeOffset Time at which to add the new sound data in microseconds. Dependent on sample rate.

\return True if nso was successfully added to this CX_SoundObject, false otherwise.
*/
bool CX_SoundObject::addSound (CX_SoundObject nso, CX_Micros timeOffset) {
	if (!nso._successfullyLoaded) {
		Log.error("CX_SoundObject") << "addSound: Added sound object not successfully loaded. It will not be added.";
		return false;
	}

	//This condition really should have a warning message associated with it.
	if (!this->_successfullyLoaded) {
		*this = nso;
		this->addSilence(timeOffset, true);
		return true;
	}

	if (nso.getSampleRate() != this->getSampleRate()) {
		nso.resample( this->getSampleRate() );
	}

	if (nso.getChannelCount() != this->getChannelCount()) {
		if (!nso.setChannelCount( this->getChannelCount() )) {
			Log.error("CX_SoundObject") << "addSound: Failed to match the number of channels of added sound to existing sound. The new sound will not be added.";
			return false;
		}
	}

	//Time is in microseconds, so do samples/second * seconds * channels to get the (absolute) sample at which the new sound starts.
	unsigned int insertionSample = _soundChannels * (unsigned int)(this->getSampleRate() * ((double)timeOffset / 1000000));

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

/*! Set the contents of the sound object from a vector of float data. 
\param data A vector of sound samples. These values should go from -1 to 1. This requirement is not checked for. 
If there is more than once channel of data, the data must be interleaved. This means that if, for example, 
there are two channels, the ordering of the samples is 12121212... where 1 represents a sample for channel 
1 and 2 represents a sample for channel 2. This requirement is not checked for. The number of samples in this
vector must be evenly divisible by the number of channels set with the `channels` argument.
\param channels The number of channels worth of data that is stored in `data`.
\param sampleRate The sample rate of the samples. If `data` contains, for example, a sine wave, that wave was sampled
at some rate (e.g. 48000 samples per second of waveform). `sampleRate` should be that rate.
return True in all cases. No checking is done on any of the arguments.
*/
bool CX_SoundObject::setFromVector(const std::vector<float>& data, int channels, float sampleRate) {
	if ((data.size() % channels) != 0) {
		Log.error("CX_SoundObject") << "setFromVector: The size of the sample data was not evenly divisible by the number of channels.";
		return false;
	}


	_soundData = data;
	_soundChannels = channels;
	_soundSampleRate = sampleRate;
	_successfullyLoaded = true; //Do no checking of the values.
	return true;
}

/*! Checks to see if the CX_SoundObject is ready to play. It basically just checks if there is sound data
available and that the number of channels is set to a sane value. */
bool CX_SoundObject::isReadyToPlay (void) {
	return ((_soundChannels > 0) && (_soundData.size() != 0)); //_successfullyLoaded? _soundSampleRate? Remove _soundChannels?
}

/*! Set the length of the sound to the specified length in microseconds. If the new length is longer than the old length,
the new data is zeroed (i.e. set to silence). */
void CX_SoundObject::setLength (CX_Micros length) {
	unsigned int endOfDurationSample = _soundChannels * (unsigned int)(getSampleRate() * ((double)length / 1000000));

	_soundData.resize( endOfDurationSample, 0 );
}

/*! Gets the length, in time, of the sound object.
\return The length. */
CX_Micros CX_SoundObject::getLength (void) {
	return ((uint64_t)_soundData.size() * 1000000)/(getChannelCount() * (uint64_t)getSampleRate());
}


/*! Finds the maximum amplitude in the sound object.
\return The maximum amplitude.
\note Amplitudes are between -1 and 1, inclusive. */
float CX_SoundObject::getPositivePeak (void) {
	float peak = numeric_limits<float>::min();
	for (unsigned int i = 0; i < _soundData.size(); i++) {
		if (_soundData[i] > peak) {
			peak = _soundData[i];
		}
	}
	return peak;
}

/*! Finds the minimum amplitude in the sound object.
\return The minimum amplitude.
\note Amplitudes are between -1 and 1, inclusive. */
float CX_SoundObject::getNegativePeak (void) {
	float peak = numeric_limits<float>::max();
	for (unsigned int i = 0; i < _soundData.size(); i++) {
		if (_soundData[i] < peak) {
			peak = _soundData[i];
		}
	}
	return peak;
}

/*! Normalizes the contents of the sound object.
\param amount Must be in the interval [0,1]. If 1, normalize will normalize in the standard way:
The peak with the greatest magnitude will be set to +/-1 and everything else will be scaled relative to the peak.
If amount is less than 1, the greatest peak will be set to that value.
*/
void CX_SoundObject::normalize(float amount) {
	float peak = std::max(abs(getPositivePeak()), abs(getNegativePeak()));
	float multiplier = amount / peak;

	for (unsigned int i = 0; i < _soundData.size(); i++) {
		_soundData[i] *= multiplier;
	}
}

/*!
Removes leading "silence" from the sound, where silence is defined by the given tolerance. It is unlikely that
the beginning of a sound, even if perceived as silent relative to the rest of the sound, has an amplitude of 0.
Therefore, a tolerance of 0 is unlikely to prove useful. Using getPositivePeak() and/or getNegativePeak() can
help to give a reference amplitude of which some small fraction is perceived as "silent".

\param tolerance All sound data up to and including the first instance of a sample with an amplitude
with an absolute value greater than or equal to tolerance is removed from the sound.
*/
void CX_SoundObject::stripLeadingSilence (float tolerance) {
	for (unsigned int sampleFrame = 0; sampleFrame < this->getSampleFrameCount(); sampleFrame++) {
		for (int channel = 0; channel < _soundChannels; channel++) {
			unsigned int index = (sampleFrame * _soundChannels) + channel;

			if (abs(_soundData.at(index)) >= tolerance) {
				_soundData.erase(_soundData.begin(), _soundData.begin() + (sampleFrame * _soundChannels));
				return;
			}
		}
	}
}

/*! Adds the specified amount of silence to the CX_SoundObject at either the beginning or end.

\param duration Duration of added silence in microseconds. Dependent on the sample rate of the sound. If the sample rate changes,
so does the duration of silence.
\param atBeginning If true, silence is added at the beginning of the CX_SoundObject. If false, the silence is added at the end.
*/
void CX_SoundObject::addSilence (CX_Micros duration, bool atBeginning) {
	//Time is in microseconds, so do samples/second * seconds * channels to get the absolute sample count for the new silence.
	unsigned int absoluteSampleCount = _soundChannels * (unsigned int)(getSampleRate() * ((double)duration / 1000000));

	if (atBeginning) {
		_soundData.insert( _soundData.begin(), absoluteSampleCount, 0 );
	} else {
		_soundData.insert( _soundData.end(), absoluteSampleCount, 0 );
	}
}

/*! Deletes the specified amount of sound from the CX_SoundObject from either the beginning or end.

\param duration Duration of removed sound in microseconds. If this is greater than the duration of the sound, the whole sound is deleted.
\param fromBeginning If true, sound is deleted from the beginning of the CX_SoundObject's buffer.
If false, the sound is deleted from the end, toward the beginning.
*/
void CX_SoundObject::deleteAmount (CX_Micros duration, bool fromBeginning) {
	//Time is in microseconds, so do samples/second * seconds * channels to get the absolute sample count to delete.
	unsigned int absoluteSampleCount = _soundChannels * (unsigned int)(getSampleRate() * ((double)duration / 1000000));

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

\param newChannelCount The number of channels the CX_SoundObject will have after the conversion.

\return True if the conversion was successful, false if the attempted conversion is unsupported.
*/
bool CX_SoundObject::setChannelCount (int newChannelCount) {

	if (newChannelCount == _soundChannels) {
		return true;
	}

	if (_soundChannels == 0) {
		//0 to anything is easy
		_soundChannels = newChannelCount;
		return true;
	}

	if (_soundChannels == 1) {
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
		for (unsigned int sample = 0; sample < getSampleFrameCount(); sample++) {
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
		for (unsigned int sample = 0; sample < getSampleFrameCount(); sample++) {
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
	
	Log.error("CX_SoundObject") << "Sound cannot be set to the given number of channels. There is no known conversion from " <<
						_soundChannels << " channels to " << newChannelCount << 
						" channels. You will have to do it manually. Use getRawDataReference() to access the sound data." << endl;

	return false;
}

/*!
Resamples the audio data stored in the CX_SoundObject by linear interpolation. Linear interpolation is not the ideal
way to resample audio data; some audio fidelity is lost, more so than with other resampling techinques. It is, however, 
very fast compared to higher-quality methods both in terms of run time and programming time. It has acceptable results, 
at least when the new sample rate is similar to the old sample rate.

\param newSampleRate The requested sample rate.
*/
void CX_SoundObject::resample (float newSampleRate) {

	//Using SRC	
	/*
	SRC_DATA data;
	data.src_ratio = (double)newSampleRate/_soundSampleRate;

	long requiredSamples = (long)(getSampleFrameCount() * ((double)newSampleRate/_soundSampleRate));
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


	
	
	uint64_t oldSampleCount = getSampleFrameCount();
	uint64_t newSampleCount = (uint64_t)(getSampleFrameCount() * ((double)newSampleRate / _soundSampleRate));

	vector<float> completeNewData((unsigned int)newSampleCount * _soundChannels);

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

/*! This function reverses the sound data stored in the CX_SoundObject so that if it is played, it will
play in reverse. */
void CX_SoundObject::reverse(void) {
	vector<float> copy = _soundData;
	unsigned int sampleFrameCount = getSampleFrameCount();
	for (unsigned int sf = 0; sf < sampleFrameCount; sf++) {
		unsigned int targetSampleFrame = sf*_soundChannels;
		unsigned int sourceSampleFrame = (sampleFrameCount - 1 - sf) * _soundChannels;

		for (unsigned int ch = 0; ch < _soundChannels; ch++) {
			_soundData[targetSampleFrame + ch] = copy[sourceSampleFrame + ch];
		}
	}
}

/*! This function changes the speed of the sound by some multiple.
\param speedMultiplier Amount to multiply the speed by. Must be greater than 0.
\note If you would like to use a negative value to reverse the direction of playback, see reverse().
*/
void CX_SoundObject::multiplySpeed (float speedMultiplier) {
	if (speedMultiplier <= 0) {
		return;
	}

	float sampleRate = this->_soundSampleRate;
	this->resample( this->getSampleRate() / speedMultiplier );
	this->_soundSampleRate = sampleRate;
}

/*!
Apply gain to the channel in terms of decibels.
\param decibels Gain to apply. 0 does nothing. Positive values increase volume, negative values decrease volume. Negative infinity is essentially mute,
although see multiplyAmplitudeBy() for a more obvious way to do that same operation.
\param channel The channel that the gain should be applied to. If channel is less than 0, the gain is applied to all channels.
*/
bool CX_SoundObject::applyGain (float decibels, int channel) {
	float amplitudeMultiplier = sqrt( pow(10.0f, decibels/10.0f) );
	return multiplyAmplitudeBy( amplitudeMultiplier, channel );
}

/*!
Apply gain to the sound. The original value is simply multiplied by the amount and then clamped to be within [-1, 1].
\param amount The gain that should be applied. A value of 0 mutes the channel. 1 does nothing. 2 doubles the amplitude. -1 inverts the waveform.
\param channel The channel that the given multiplier should be applied to. If channel is less than 0, the amplitude multiplier is applied to all channels.
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
		for (unsigned int sf = 0; sf < (unsigned int)getSampleFrameCount(); sf++) {
			unsigned int index = (sf * _soundChannels) + channel;
			float newVal = _soundData[index] * amount;
			newVal = std::max(newVal, -1.0f);
			newVal = std::min(newVal, 1.0f);
			_soundData[index] = newVal;
		}

	}
	return true;
}

/*! Clears all data stored in the sound object and returns it to an uninitialized state. */
void CX_SoundObject::clear(void) {
	_soundData.clear();
	_successfullyLoaded = false;
	_soundChannels = 0;
	_soundSampleRate = 0;
}


/*! Writes the contents of the sound object to a file with the given file name. The data will
be encoded as 16-bit PCM. The sample rate is determined by the sample rate of the sound object.
\param filename The name of the file to save the sound data to. `filename` should have a .wav extension. If it does not,
".wav" will be appended to the file name and a warning will be logged.
\return False if there was an error while opening the file. If so, an error will be logged.
*/
bool CX_SoundObject::writeToFile(std::string filename) {
	//Taken from the ofSoundFile additions suggested here: https://github.com/openframeworks/openFrameworks/pull/2626
	//From this file: https://github.com/admsyn/openFrameworks/blob/feature-sound-objects/libs/openFrameworks/sound/ofSoundFile.cpp
	//There were some modifications to get it to work with the data structure of CX_SoundObject.

	// check that we're writing a wav and complain if the file extension is wrong.
	ofFile f(filename);
	if (ofToLower(f.getExtension()) != "wav") {
		filename += ".wav";
		CX::Instances::Log.warning("CX_SoundObject") << "writeToFile: Can only write wav files - will save file as " << filename;
	}

	fstream file(ofToDataPath(filename).c_str(), ios::out | ios::binary);
	if (!file.is_open()) {
		CX::Instances::Log.error("CX_SoundObject") << "writeToFile: Error opening sound file '" << filename << "' for writing.";
		return false;
	}

	// write a wav header
	short myFormat = 1; // for pcm
	int mySubChunk1Size = 16;
	int bitsPerSample = 16; // assume 16 bit pcm

	int channels = this->getChannelCount();
	int samplerate = this->getSampleRate();
	unsigned int bufferSize = this->getTotalSampleCount();

	int myByteRate = samplerate * channels * bitsPerSample / 8;
	short myBlockAlign = channels * bitsPerSample / 8;
	int myChunkSize = 36 + bufferSize*bitsPerSample / 8;
	int myDataSize = bufferSize*bitsPerSample / 8;


	file.seekp(0, ios::beg);
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
	int pos = 0;
	while (pos < bufferSize) {
		int len = MIN(WRITE_BUFF_SIZE, bufferSize - pos);
		for (int i = 0; i < len; i++) {
			writeBuff[i] = (int)(this->_soundData[pos] * 32767.f);
			pos++;
		}
		file.write((char*)writeBuff, len*bitsPerSample / 8);
	}

	file.close();
	return true;
}



/*
float CX_SoundObject::_readSample (int channel, unsigned int sample) {
	unsigned int newIndex = (sample * _soundChannels) + channel;
	return _soundData.at( newIndex );
}

vector<float> CX_SoundObject::_getChannelData (int channel) {
	vector<float> rval( (unsigned int)getSampleFrameCount() );

	for (unsigned int sample = 0; sample < rval.size(); sample++) {
		rval[sample] = _soundData.at( (sample * _soundChannels) + channel );
	}

	return rval;
}
*/