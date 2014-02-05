#include "CX_SoundObjectPlayer.h"

using namespace CX;
using namespace CX::Instances;

CX_SoundObjectPlayer::CX_SoundObjectPlayer (void) :
	_startTime(numeric_limits<uint64_t>::max())
{
	ofAddListener( ofEvents().exit, this, &CX_SoundObjectPlayer::_exitHandler );
	ofAddListener( _soundStream.outputCallbackEvent, this, &CX_SoundObjectPlayer::_outputEventHandler );

}

CX_SoundObjectPlayer::~CX_SoundObjectPlayer (void) {
	ofRemoveListener( ofEvents().exit, this, &CX_SoundObjectPlayer::_exitHandler );
	ofRemoveListener( _soundStream.outputCallbackEvent, this, &CX_SoundObjectPlayer::_outputEventHandler );
}

/*! Configures the CX_SoundObjectPlayer with the given configuration. */
bool CX_SoundObjectPlayer::setup (CX_SoundObjectPlayerConfiguration_t config) {
	bool openedSuccessfully = _soundStream.open( (CX_SoundStreamConfiguration_t)config );
	
	bool startedSuccessfully = _soundStream.start();

	return startedSuccessfully && openedSuccessfully;
}

void CX_SoundObjectPlayer::_exitHandler (ofEventArgs &a) {
	this->stop();
	_soundStream.close();
}

/*!
Attempts to start playing the current CX_SoundObject associated with the player.

\return True if the sound object associated with the player isReadyToPlay(), false otherwise.
*/
bool CX_SoundObjectPlayer::play (void) {
	if (_activeSoundObject != NULL && _activeSoundObject->isReadyToPlay()) {
		_playing = true;
		_currentConcurrentSample = 0;
		return true;
	}
	return false;
}

/*! Stop the currently playing sound object, or, if a playback start was cued, cancel the cued playback.
\return Always returns true currently.
*/
bool CX_SoundObjectPlayer::stop (void) {
	_playing = false;
	_startTime = std::numeric_limits<CX_Micros>::max();
	return true;
}

bool CX_SoundObjectPlayer::startPlayingAt (CX_Micros experimentTime) {
	//_startTime = time - (_sampleOffset * 1000000) / _soundStream.getConfiguration().sampleRate;
	_startTime = experimentTime - _startTimeOffset; //_startTimeOffset is always negative.

	CX_Micros lastSwap = _soundStream.getLastSwapTime();
	CX_Micros timeFromLastSwap = _startTime - lastSwap;
	uint64_t samplesFromLastSwap = (timeFromLastSwap * _soundStream.getConfiguration().sampleRate)/1000000;
	
	uint64_t lastSampleNumber = _soundStream.getLastSampleNumber(); //This is the next sample that will be sent.

	lastSampleNumber = lastSampleNumber + samplesFromLastSwap - _soundStream.getConfiguration().bufferSize; //lastSampleNumber is at the end of the last buffer.

	//This is incomplete.
	return false;
}




/*!
This function is blocking because the sample rate and number of channels of sound are changed to those of the currently open stream.
\param sound A pointer to a CX_SoundObject that will be set as the current sound for the CX_SoundObjectPlayer. There are a variety of
reasons why the sound could fail to be set as the current sound for the player. If sound was not loaded successfully, this function
call fails and an error is logged. If it is not possible to convert the number of channels of sound to the number of channels that
the CX_SoundObjectPlayer is configured to use, this function call fails and an error is logged.
\return True if sound was successfully set to be the current sound, false otherwise.
*/
bool CX_SoundObjectPlayer::BLOCKING_setSound (CX_SoundObject *sound) {
	if (sound == NULL) {
		return false;
	}

	//I'm not entirely sure what this check is for. What exactly am I wanting to know about the status of the sound object?
	if (!sound->isLoadedSuccessfully()) {
		Log.error("CX_SoundObjectPlayer") << "Sound is not loaded successfully. It will not be set as the active sound.";
		return false;
	}


	const CX_SoundStreamConfiguration_t &streamConfig = this->_soundStream.getConfiguration();

	if (streamConfig.outputChannels != sound->getChannelCount()) {
		if (!sound->setChannelCount(streamConfig.outputChannels)) {
			Log.error("CX_SoundObjectPlayer") << "It was not possible to change the number of channels of the sound to the number used by the sound player.";
			return false;
		}
		Log.warning("CX_SoundObjectPlayer") << "Channel count changed: Sound fidelity may have been lost.";
	}

	if (streamConfig.sampleRate != sound->getSampleRate()) {
		Log.warning("CX_SoundObjectPlayer") << "Sound resampled: Sound fidelity may have been lost.";
		sound->resample( (float)streamConfig.sampleRate );
	}

	_activeSoundObject = sound;
	return true;

}


//void CX_SoundObjectPlayer::update (void) {
	//What goes in this function?

	/*
	static uint64_t lastKnownSwap = 0;
	if (cxPlayer._soundStream.hasSwappedSinceLastCheck()) {
		uint64_t thisSwapTime = cxPlayer._soundStream.getLastSwapTime();
		cout << thisSwapTime - lastKnownSwap << endl;
		lastKnownSwap = thisSwapTime;
	}
	*/
//}



bool CX_SoundObjectPlayer::_outputEventHandler (CX_SSOutputCallback_t &outputData) {
	if (!_playing || (_playbackStartConcurrentSample == numeric_limits<uint64_t>::max()) || (_activeSoundObject == NULL)) {
		return false;
	}

	//if (outputData.bufferUnderflow) {
	//	cout << "Underflow!" << endl;
	//}

	//cout << "Playing sample " << _currentSample << endl;

	vector<float> &soundData = _activeSoundObject->getRawDataReference();
	int soundChannelCount = _activeSoundObject->getChannelCount();

	const CX_SoundStreamConfiguration_t &config = _soundStream.getConfiguration();

	uint64_t concurrentSamplesToOutput = outputData.bufferSize;

	if (soundData.size() < ((_currentConcurrentSample + outputData.bufferSize) * config.outputChannels)) {
		concurrentSamplesToOutput = (soundData.size()/config.outputChannels) - _currentConcurrentSample;
		_playing = false;
	}

	//Elsewhere, we force the number of sound channels and output channels to be the same, so we can just memcpy here (POFL: Principle Of FrontLoading).
	memcpy( outputData.outputBuffer, 
		soundData.data() + (_currentConcurrentSample * config.outputChannels), 
		(size_t)(concurrentSamplesToOutput * config.outputChannels * sizeof(float)) );
	
	_currentConcurrentSample += concurrentSamplesToOutput;

	return true;

	/*
	if (config.outputChannels == soundChannelCount) {
		//If the sound and output channel counts are the same, then just copy out the data.
		memcpy( outputData.outputBuffer, soundData.data() + _currentSample, totalSampleCount * sizeof(float) );
		_currentSample += totalSampleCount;
		//We're assuming that output has been zeroed before entering this function, so we don't need to fill out the non-written samples with zeroes.

	} else if (soundChannelCount == 1) {
		//Mono to anything else: Just copy the data to each output channel.
		for (unsigned int i = 0; i < concurrentSamples; i++) {
			for (unsigned int j = 0; j < config.outputChannels; j++) {
				outputData.outputBuffer[(i * config.outputChannels) + j] = soundData[i];
			}
		}
	}
	*/
	//Other cases are more complicated and should be rejected outright. They should not be processed in this function.
	//When the sound is assigned, check for those cases that should be rejected. How? The number of output channels must
	//be known in order to do the comparison.
}