#include "CX_SoundObjectPlayer.h"

using namespace CX;
using namespace CX::Instances;

CX_SoundObjectPlayer::CX_SoundObjectPlayer (void) :
	_playing(false),
	_playbackStartQueued(false),
	_soundObject(nullptr),
	_playbackStartSampleFrame(std::numeric_limits<uint64_t>::max()),
	_currentSampleFrame(0)
{
	ofAddListener(_soundStream.outputEvent, this, &CX_SoundObjectPlayer::_outputEventHandler);
}

CX_SoundObjectPlayer::~CX_SoundObjectPlayer (void) {
	this->stop();
	_soundStream.closeStream();
	ofRemoveListener(_soundStream.outputEvent, this, &CX_SoundObjectPlayer::_outputEventHandler);
}

/*! Configures the CX_SoundObjectPlayer with the given configuration. */
bool CX_SoundObjectPlayer::setup (CX_SoundObjectPlayerConfiguration_t config) {
	bool openedSuccessfully = _soundStream.setup( (CX_SoundStream::Configuration&)config );
	
	bool startedSuccessfully = _soundStream.start();

	return startedSuccessfully && openedSuccessfully;
}

/*!
Attempts to start playing the current CX_SoundObject associated with the player.

\return True if the sound object associated with the player isReadyToPlay(), false otherwise.
*/
bool CX_SoundObjectPlayer::play (void) {
	if (_soundObject != nullptr && _soundObject->isReadyToPlay()) {
		_playing = true;
		_soundPlaybackSampleFrame = 0;
		return true;
	}
	Log.error("CX_SoundObjectPlayer") << "Could not start sound playback. There was a problem with the sound object associated with the player.";
	return false;
}

/*! Stop the currently playing sound object, or, if a playback start was cued, cancel the cued playback.
\return Always returns true currently.
*/
bool CX_SoundObjectPlayer::stop (void) {
	_playing = false;
	//_startTime = std::numeric_limits<CX_Micros>::max();
	return true;
}

/*! Queue the start time of the sound in experiment time with an offset to account for latency. 

\param experimentTime The desired experiment time at which the sound should start playing. This time
plus the offset should be in the future. If it is not, the sound will start playing immediately.
\param latencyOffset An offset that accounts for latency. If, for example, you called this function with
an offset of 0 and discovered that the sound played 200 ms later than you were expecting it to,
you would set offset to -200 * 1000 in order to queue the start time 200 ms earlier than the
desired experiment time.
\return False if the start time plus the offset is in the past. True otherwise.
\note See setTime() for a way to choose the current time point within the sound.
*/
bool CX_SoundObjectPlayer::startPlayingAt(CX_Micros experimentTime, CX_Micros latencyOffset) {
	CX_Micros adjustedStartTime = experimentTime + latencyOffset;

	if (adjustedStartTime <= Clock.getTime()) {
		Log.warning("CX_SoundObjectPlayer") << "startPlayingAt: Desired start time has already passed. Starting immediately.";
		play();
		return false;
	}

	CX_Micros lastSwapTime = _soundStream.getLastSwapTime(); //This is the time at which the last swap started (i.e. as soon as the fill buffer callback was called).
	uint64_t samplesFramesSinceLastSwap = ((adjustedStartTime - lastSwapTime) * _soundStream.getConfiguration().sampleRate) / 1000000;
	
	uint64_t lastSwapStartSFNumber = _soundStream.getSampleFrameNumber() - _soundStream.getConfiguration().bufferSize; //Go back to the previous buffer start SF

	_playbackStartSampleFrame = lastSwapStartSFNumber + samplesFramesSinceLastSwap;
	_playbackStartQueued = true;
	return true;
}

/*! Set the current time in the active sound. When playback starts, it will begin from that 
time in the sound. If the sound object is currently playing, this will jump to that point
in the sound.
\param time The time in the sound to seek to.
*/
void CX_SoundObjectPlayer::setTime(CX_Micros time) {
	_soundPlaybackSampleFrame = (time * _soundStream.getConfiguration().sampleRate) / 1000000;
}


/*!
This function is blocking because the sample rate and number of channels of sound are changed to those of the currently open stream.
\param sound A pointer to a CX_SoundObject that will be set as the current sound for the CX_SoundObjectPlayer. There are a variety of
reasons why the sound could fail to be set as the current sound for the player. If sound was not loaded successfully, this function
call fails and an error is logged. If it is not possible to convert the number of channels of sound to the number of channels that
the CX_SoundObjectPlayer is configured to use, this function call fails and an error is logged.

This function call is not blocking if the same rate and channel count of the CX_SoundObject are the same as those in use by
the CX_SoundObjectPlayer. See \ref blockingCode for more information.

\return True if sound was successfully set to be the current sound, false otherwise.
*/
bool CX_SoundObjectPlayer::BLOCKING_setSound (CX_SoundObject *sound) {
	if (sound == nullptr) { //This check is redundant, in that nullptr is checked for in play().
		return false;
	}

	//I'm not entirely sure what this check is for. What exactly am I wanting to know about the status of the sound object?
	if (!sound->isLoadedSuccessfully()) {
		Log.error("CX_SoundObjectPlayer") << "Sound is not loaded successfully. It will not be set as the active sound.";
		return false;
	}

	_playing = false; //Stop playback of the current sound.

	const CX_SoundStream::Configuration &streamConfig = this->_soundStream.getConfiguration();

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

	_soundObject = sound;

	//_currentSampleFrame = 0;
	return true;

}


bool CX_SoundObjectPlayer::_outputEventHandler (CX_SoundStream::OutputEventArgs &outputData) {
	//This check is a bit strange, but if !_playing and _playbackStartQueued, then we are checking for playback start and not returning false.
	if ((!_playing && !_playbackStartQueued) || (_soundObject == nullptr)) {
		return false;
	}

	//if (outputData.bufferUnderflow) {
	//	cout << "Underflow!" << endl;
	//}

	//cout << "Playing sample " << _currentSample << endl;

	uint64_t sampleFramesToOutput = outputData.bufferSize;
	uint64_t outputBufferOffset = 0;
	vector<float> &soundData = _soundObject->getRawDataReference();
	const CX_SoundStream::Configuration &config = _soundStream.getConfiguration();

	if (_playbackStartQueued) {
		if (_playbackStartSampleFrame < (_currentSampleFrame + outputData.bufferSize)) {
			_playing = true;
			_playbackStartQueued = false;

			outputBufferOffset = _playbackStartSampleFrame - _currentSampleFrame;
			sampleFramesToOutput = outputData.bufferSize - outputBufferOffset;
			_soundPlaybackSampleFrame = 0;
		} else {
			sampleFramesToOutput = 0;
		}
	}

	if (soundData.size() < ((_soundPlaybackSampleFrame + outputData.bufferSize - outputBufferOffset) * config.outputChannels)) {
		//If there is not enough data to completely fill the sound buffer, only use some of it.
		sampleFramesToOutput = (soundData.size() / config.outputChannels) - _soundPlaybackSampleFrame - outputBufferOffset;
		_playing = false;
	}

	//Elsewhere, we force the number of sound channels and output channels to be the same, so we can just memcpy here.
	memcpy(outputData.outputBuffer + outputBufferOffset,
		soundData.data() + (_soundPlaybackSampleFrame * config.outputChannels),
		(size_t)(sampleFramesToOutput * config.outputChannels * sizeof(float)));
	
	_currentSampleFrame += outputData.bufferSize;

	_soundPlaybackSampleFrame += sampleFramesToOutput;

	return true;
}