#include "CX_SoundBufferPlayer.h"

#include "CX_Logger.h"

namespace CX {

CX_SoundBufferPlayer::CX_SoundBufferPlayer(void) :
	_soundStream(nullptr)
{}

CX_SoundBufferPlayer::~CX_SoundBufferPlayer(void) {
	stop();

	getUnderflowsSinceLastCheck(true);
}


/*! Set up the sound buffer player to use an existing `CX_SoundStream`, `ss`. 
`ss` is not set up or started automatically, the user code must set it up and start it.
`ss` must exist for the lifetime of the `CX_SoundBufferPlayer`.
\param ss A pointer to a fully configured and started `CX_SoundStream`.
\return `true` in all cases. */
bool CX_SoundBufferPlayer::setup(CX_SoundStream *ss) {
	return setup(CX::Util::wrapPtr(ss));
}

bool CX_SoundBufferPlayer::setup(std::shared_ptr<CX_SoundStream> ss) {
	
	_cleanUpOldSoundStream();

	_soundStream = ss;
	
	if (!ss) {
		return false;
	}

	if (!ss->isStreamRunning()) {
		CX::Instances::Log.notice("CX_SoundBufferPlayer") << "setup(): Sound stream is not running.";
	}

	_outputEventHelper.setup<CX_SoundBufferPlayer>(&ss->outputEvent, this, &CX_SoundBufferPlayer::_outputEventHandler);
	_outputEventHelper.listenToStopEvent(&ss->destructEvent);

	setSoundBuffer(_outData.soundBuffer); // Re-set current sound buffer as new sound buffer

	return true;
}

std::shared_ptr<CX_SoundStream> CX_SoundBufferPlayer::getSoundStream(void) {
	return _soundStream; 
}


/*! Attempts to start playing the current CX_SoundBuffer associated with the player.

\param restart If `true`, playback will be restarted from the beginning of the sound. 
If `false`, playback will continue from where it was last stopped.

\return `true` if the sound buffer associated with the player is ready to play (see `CX_SoundBuffer::isReadyToPlay()`), `false` otherwise. */
bool CX_SoundBufferPlayer::play(bool restart) {

	if (!_checkPlaybackRequirements("play")) {
		return false;
	}

	std::lock_guard<std::recursive_mutex> outputLock(_outData);

	_outData.playing = true;

	if (restart) {
		_outData.soundPlaybackSampleFrame = 0;
	}

	return true;
	
}

/*! Queue the start time of the sound in experiment time with an offset to account for latency.

The start time is adjusted by an estimate of the latency of the sound stream. This is calculated
as (N_b - 1) * S_b / SR, where N_b is the number of buffers, S_b is the size of the buffers (in sample
frames), and SR is the sample rate, in sample frames per second.

In order for this function to have any meaningful effect, the request start time, plus any latency
adjustments, must be in the future. If `startTime` minus the estimated stream
latency is not in the future, the sound will start playing immediately and a warning will be logged.

\param startTime The desired experiment time at which the sound should start playing.
\param timeout The longest amount of time to wait until a prediction for the future swap unit is ready.
\param restart If `true`, playback will be restarted from the beginning of the sound.
If `false`, playback will continue from where it was last stopped.

\return `false` if the start time plus the latency offset is in the past. `true` otherwise.

\note See CX_SoundBufferPlayer::seek() for a way to choose the current time point within the sound.
*/
bool CX_SoundBufferPlayer::queuePlayback(CX_Millis startTime, CX_Millis timeout, bool restart) {

	Sync::DataClient& cl = _soundStream->swapClient;

	if (!cl.waitUntilAllReady(timeout)) {
		return false;
	}

	Sync::SwapUnitPrediction sp = cl.predictSwapUnitAtTime(startTime);
	if (sp.usable) {
		return queuePlayback(sp.prediction(), restart);
	}
	return false;

	//CX_SoundStream::SampleFrame startSF = _soundStream->estimateSampleFrameAtTime(startTime, latencyOffset);
	//return queuePlayback(startSF, restart);
}

bool CX_SoundBufferPlayer::queuePlayback(SampleFrame sampleFrame, bool restart) {

	if (!_checkPlaybackRequirements("queuePlayback")) {
		return false;
	}

	if (sampleFrame < _soundStream->swapData.getNextSwapUnit()) {
		CX::Instances::Log.warning("CX_SoundBufferPlayer") << "queuePlayback(): Desired start sample frame has already passed. Starting immediately. "
			"Desired start SF: " <<	sampleFrame << ", next swap SF: " << _soundStream->swapData.getNextSwapUnit() << ".";
		play(restart);
		return false;
	}

	std::lock_guard<std::recursive_mutex> outputLock(_outData);

	_outData.playbackStartSampleFrame = sampleFrame;
	_outData.playbackQueued = true;

	if (restart) {
		_outData.soundPlaybackSampleFrame = 0;
	}

	return true;
}

bool CX_SoundBufferPlayer::_checkPlaybackRequirements(std::string callerName) {

	if (_soundStream == nullptr) {
		CX::Instances::Log.error("CX_SoundBufferPlayer") << callerName << "(): Could not start sound playback because the sound stream was nullptr. Have you forgotten to call setup()?";
		return false;
	}

	if (!_soundStream->isStreamRunning()) {
		CX::Instances::Log.error("CX_SoundBufferPlayer") << callerName << "(): Could not start sound playback. The sound stream was not running.";
		return false;
	}

	std::lock_guard<std::recursive_mutex> outputLock(_outData);

	if (_outData.soundBuffer == nullptr || !_outData.soundBuffer->isReadyToPlay()) {
		CX::Instances::Log.error("CX_SoundBufferPlayer") << callerName << "(): Could not start sound playback. There was a problem with the sound "
			"buffer associated with the player. Have you remembered to call setSoundBuffer()?";
		return false;
	}

	return true;
}

/*! Stop the currently playing sound buffer, or, if a playback start was cued, cancel the cued playback. */
void CX_SoundBufferPlayer::stop(void) {
	
	std::lock_guard<std::recursive_mutex> outputLock(_outData);

	_outData.playing = false;
	_outData.playbackQueued = false;
}

/*! Check if the sound is currently playing. 
\return `true` if the sound is currently playing, `false` otherwise.
*/
bool CX_SoundBufferPlayer::isPlaying(void) {
	std::lock_guard<std::recursive_mutex> outputLock(_outData);
	return _outData.playing;
}

/*! Check if the sound is queued to play (with `queuePlayback()`).
\return `true` if playback is queued, `false` otherwise.
*/
bool CX_SoundBufferPlayer::isPlaybackQueued(void) {
	std::lock_guard<std::recursive_mutex> outputLock(_outData);
	return _outData.playbackQueued;
}

bool CX_SoundBufferPlayer::isPlayingOrQueued(void) {
	return isPlaying() || isPlaybackQueued();
}



/*! Set the current time in the active sound. When playback starts, it will begin from that
time in the sound. If the sound buffer is currently playing, this will jump to that point
in the sound.

Use `getPlaybackTime()` to get the current playback time to do relative seeks.

\param time The time in the sound to seek to.
\note If used while the sound is playing, a warning will be logged but the function will still
have its normal effect.
*/
void CX_SoundBufferPlayer::seek(CX_Millis time) {

	std::lock_guard<std::recursive_mutex> outputLock(_outData);

	if (_outData.playing || _outData.playbackQueued) {
		CX::Instances::Log.warning("CX_SoundBufferPlayer") << "seek() used while sound was playing or queued.";
	}

	_outData.soundPlaybackSampleFrame = time.seconds() * _soundStream->getConfiguration().sampleRate;
	
}

/*! Get the playback time of the sound buffer that is associated with the player.

The return value of this function does not depend on whether the sound is playing.

\return A `CX_Millis` containing the current time of the sound.
*/
CX_Millis CX_SoundBufferPlayer::getPlaybackTime(void) {
	double seconds = getPlaybackSF() / (double)_soundStream->getConfiguration().sampleRate;

	return CX_Seconds(seconds);
}


/*! Get the playback position (in Sample Frames) of the sound buffer that is associated with the player.

The return value of this function does not depend on whether the sound is playing.
*/
SampleFrame CX_SoundBufferPlayer::getPlaybackSF(void) {
	std::lock_guard<std::recursive_mutex> outputLock(_outData);

	//if (!_outData.playing) { return 0; }

	return _outData.soundPlaybackSampleFrame;
}


CX_Millis CX_SoundBufferPlayer::getRemainingPlaybackTime(void) {
	SampleFrame remainingSF = getRemainingPlaybackSF();

	return CX_Seconds(remainingSF / (double)_soundStream->getConfiguration().sampleRate);
}

SampleFrame CX_SoundBufferPlayer::getRemainingPlaybackSF(void) {
	std::lock_guard<std::recursive_mutex> outputLock(_outData);

	//if (!_outData.playing) { return 0; }

	if (_outData.soundBuffer != nullptr && _outData.soundBuffer->isReadyToPlay()) {
		return _outData.soundBuffer->getLengthSF() - _outData.soundPlaybackSampleFrame;
	}

	return _outData.soundPlaybackSampleFrame;
}


bool CX_SoundBufferPlayer::isReadyToPlay(bool log) {

	if (!_soundStream) {
		if (log) {
			CX::Instances::Log.warning("CX_SoundBufferPlayer") << "isReadyToPlay(): Failed because there was no CX_SoundStream associated with the CX_SoundBufferPlayer. Use CX_SoundBufferPlayer::setup() to associate a sound stream with the player.";
		}
		return false;
	}

	if (!_soundStream->isStreamRunning()) {
		if (log) {
			CX::Instances::Log.warning("CX_SoundBufferPlayer") << "isReadyToPlay(): Failed because the sound stream was not running. If your CX_SoundBufferPlayer is named player, try: bool streamStarted = player.getSoundStream()->startStream();";
		}
		return false;
	}

	std::lock_guard<std::recursive_mutex> outputLock(_outData);

	if (!_outData.soundBuffer) {
		if (log) {
			CX::Instances::Log.warning("CX_SoundBufferPlayer") << "isReadyToPlay(): Failed because there was no CX_SoundBuffer associated with the CX_SoundBufferPlayer. Use CX_SoundBufferPlayer::setSoundBuffer() to associate a sound buffer with the player.";
		}
		return false;
	}

	if (!_outData.soundBuffer->isReadyToPlay(log)) {
		if (log) {
			CX::Instances::Log.warning("CX_SoundBufferPlayer") << "isReadyToPlay(): Failed because the CX_SoundBuffer associated with the CX_SoundBufferPlayer was not ready to play.";
		}
		return false;
	}

	return true;
}

/*! Get the number of buffer underflows since the last check for underflows with this function.
The number of underflows is reset each time this function is called.
An underflows means that some unknown amount of sound data that should have been
played was not played.
\param logUnderflows If `true` and there have been any underflows since the last check, a warning will be logged.
\return The number of buffer underflows since the last check.
*/
unsigned int CX_SoundBufferPlayer::getUnderflowsSinceLastCheck(bool logUnderflows) {

	_outData.lock();
	unsigned int ovf = _outData.underflowCount;
	_outData.underflowCount = 0;
	_outData.unlock();

	if (logUnderflows && ovf > 0) {
		CX::Instances::Log.warning("CX_SoundBufferPlayer") << "There have been " << ovf << " buffer underflows since the last check.";
	}
	return ovf;
}


/*! Sets the `CX_SoundBuffer` that is used by the `CX_SoundBufferPlayer`.

This function is potentially blocking because the sample rate and number of channels of `buffer`
are changed to those of the currently open stream if they do not already match.
This function is not blocking if the same rate and channel count of the CX_SoundBuffer are the same as those in use by
the `CX_SoundBufferPlayer` (see `CX_SoundBufferPlayer::getConfiguration()` for a way to get the current settings).
See \ref blockingCode for more information about blocking.

There are a variety of reasons why `buffer` could fail to be set as the current sound buffer for the player.
If the sound buffer is not ready to play, an error is logged and `false` is returned.
If it is not possible to convert the number of channels of sound to the number of channels that
the CX_SoundBufferPlayer is configured to use, this function call fails and an error is logged.

\param buffer A pointer to a CX_SoundBuffer that will be set as the current sound buffer for the CX_SoundBufferPlayer.
`buffer` may be modified by this function.

\return `true` if sound was successfully set to be the current sound, `false` otherwise.
*/
bool CX_SoundBufferPlayer::setSoundBuffer(std::shared_ptr<CX_SoundBuffer> buffer) {
	if (_soundStream == nullptr) {
		CX::Instances::Log.error("CX_SoundBufferPlayer") << "setSoundBuffer(): You cannot set the sound buffer until the CX_SoundBufferPlayer has been set up. Call setup() first.";
		return false;
	}

	if (buffer == nullptr) {
		stop();
		std::lock_guard<std::recursive_mutex> outputLock(_outData);
		_outData.soundBuffer = buffer;
		return false;
	}

	//I'm not entirely sure what this check is for. What exactly am I wanting to know about the status of the sound buffer?
	if (!buffer->isReadyToPlay()) {
		CX::Instances::Log.error("CX_SoundBufferPlayer") << "setSoundBuffer(): Sound is not ready to play. It will not be set as the active sound.";
		return false;
	}

	stop(); //Stop playback of the current sound.

	const CX_SoundStream::Configuration &ssc = _soundStream->getConfiguration();

	unsigned int oldChannelCount = buffer->getChannelCount();
	if (ssc.outputChannels != buffer->getChannelCount()) {
		if (!buffer->setChannelCount(ssc.outputChannels)) {
			CX::Instances::Log.error("CX_SoundBufferPlayer") << "setSoundBuffer(): It was not possible to change the number of channels of the sound to the number used by the sound player.";
			return false;
		}
		CX::Instances::Log.notice("CX_SoundBufferPlayer") << "setSoundBuffer(): Channel count changed from" <<
			oldChannelCount << " to " << ssc.outputChannels << ", which is the sound stream's channel count.";
	}

	if (ssc.sampleRate != buffer->getSampleRate()) {
		CX::Instances::Log.warning("CX_SoundBufferPlayer") << "setSoundBuffer(): Sound resampled. Sound fidelity may have been lost.";
		buffer->resample((float)ssc.sampleRate);
	}

	std::lock_guard<std::recursive_mutex> outputLock(_outData);
	_outData.soundBuffer = buffer;
	_outData.soundPlaybackSampleFrame = 0;

	return true;
}

bool CX_SoundBufferPlayer::setSoundBuffer(CX_SoundBuffer* buffer) {
	std::shared_ptr<CX_SoundBuffer> buf = CX::Util::wrapPtr(buffer);
	return setSoundBuffer(buf);
}

bool CX_SoundBufferPlayer::assignSoundBuffer(CX_SoundBuffer buffer) {
	std::shared_ptr<CX_SoundBuffer> buf = std::make_shared<CX_SoundBuffer>(std::move(buffer));
	return setSoundBuffer(buf);
}


/*! Provides access to the `CX_SoundBuffer` that is in use by this `CX_SoundBufferPlayer`.
If no `CX_SoundBuffer` is currently in use by this, a `CX_SoundBuffer` will be constructed and 
a pointer to it will be returned.

During playback, you should not modify the sound buffer pointed to by the returned pointer. Check `isPlaying()`.
If this function is called during playback of the sound buffer, a notice will be logged, but the
buffer pointer will still be returned.

You should not change the sound that is used by directly modifying the buffer pointed to by return value of this. 
If you want to change the sound that is used by this, you must use `setSoundBuffer()`.

\return A pointer to the CX_SoundBuffer that is used by the CX_SoundBufferPlayer. 

\code{.cpp}

CX_SoundBufferPlayer player;

// Within some function
void modifySetSound(void) {
	
	shared_ptr<CX_SoundBuffer> buffer = player.getSoundBuffer();

	// Change the buffer by adding a sound from file
	buffer->addSound("file.wav", 1000);

	// Don't forget to reassign the buffer with setSoundBuffer
	player.setSoundBuffer(buffer);

}

\endcode

*/
std::shared_ptr<CX_SoundBuffer> CX_SoundBufferPlayer::getSoundBuffer(void) {

	std::lock_guard<std::recursive_mutex> outputLock(_outData);

	if (_outData.playing) {
		CX::Instances::Log.notice("CX_SoundBufferPlayer") << "getSoundBuffer: Sound buffer pointer accessed while playback was in progress.";
	}

	if (_outData.soundBuffer == nullptr) {
		_outData.soundBuffer = std::make_shared<CX_SoundBuffer>();

		// Set to the correct output channels and sample rate.
		_outData.soundBuffer->setFromVector(_soundStream->getConfiguration().sampleRate, _soundStream->getConfiguration().outputChannels, std::vector<float>());
	}

	return _outData.soundBuffer;

}

void CX_SoundBufferPlayer::_outputEventHandler(const CX_SoundStream::OutputEventArgs& outputData) {
	
	std::lock_guard<std::recursive_mutex> outputLock(_outData);

	if ((!_outData.playing && !_outData.playbackQueued) || (_outData.soundBuffer == nullptr)) {
		//Instances::Log.notice("CX_SoundBufferPlayer") << "----";
		return;
	}

	int64_t sampleFramesToOutput = outputData.bufferSize;
	int64_t outputBufferOffsetSF = 0;
	
	if (_outData.playbackQueued) {

		int64_t nextBufferStartSF = outputData.bufferStartSampleFrame + outputData.bufferSize;
		if (_outData.playbackStartSampleFrame >= nextBufferStartSF) {
			//Instances::Log.notice("CX_SoundBufferPlayer") << "Queued but not starting";
			return;
		} else {
			//Instances::Log.notice("CX_SoundBufferPlayer") << "Queued and starting!!!";
			_outData.playing = true;
			_outData.playbackQueued = false;

			outputBufferOffsetSF = _outData.playbackStartSampleFrame - outputData.bufferStartSampleFrame;
			sampleFramesToOutput = outputData.bufferSize - outputBufferOffsetSF;
		}
	}

	//Instances::Log.notice("CX_SoundBufferPlayer") << "Playing";

	int64_t remainingSampleFramesInSoundBuffer = _outData.soundBuffer->getLengthSF() - _outData.soundPlaybackSampleFrame;

	if (sampleFramesToOutput > remainingSampleFramesInSoundBuffer) {
		//Instances::Log.notice("CX_SoundBufferPlayer") << "Done playing!";
		sampleFramesToOutput = remainingSampleFramesInSoundBuffer;
		_outData.playing = false;
	}

	std::vector<float> &soundData = _outData.soundBuffer->getRawDataReference();
	
	const CX_SoundStream::Configuration &ssc = _soundStream->getConfiguration();

	//Copy over the data, adding to the existing data. Addition allows multiple CX_SoundBufferPlayers to play into
	//the same sound stream at the same time.
	if (sampleFramesToOutput > 0) {
		int64_t rawSamplesToOutput = sampleFramesToOutput * ssc.outputChannels;
		float *sourceData = soundData.data() + (_outData.soundPlaybackSampleFrame * ssc.outputChannels);
		float *targetData = outputData.outputBuffer + (outputBufferOffsetSF * ssc.outputChannels);

		for (int64_t i = 0; i < rawSamplesToOutput; i++) {
			// Add, not assign, because there may be more than one listener to the sound stream event
			targetData[i] += sourceData[i];
		}
	}

	_outData.soundPlaybackSampleFrame += sampleFramesToOutput;

	if (outputData.bufferUnderflow) {
		_outData.underflowCount++;
	}

}

void CX_SoundBufferPlayer::_cleanUpOldSoundStream(void) {
	stop();

	getUnderflowsSinceLastCheck(true);

	_soundStream = nullptr; // Unlink from shared_ptr
}

} //namespace CX;
