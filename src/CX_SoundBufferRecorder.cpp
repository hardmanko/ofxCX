#include "CX_SoundBufferRecorder.h"

#include "CX_Private.h"

namespace CX {

CX_SoundBufferRecorder::CX_SoundBufferRecorder(void) :
	_soundStream(nullptr)
{
	_inData.recording = false;
	_inData.startingRecording = false;
	_inData.overflowCount = 0;
}

CX_SoundBufferRecorder::~CX_SoundBufferRecorder(void) {
	stop();

	getOverflowsSinceLastCheck(true);
}


/*! Set up the sound buffer recorder to use an existing `CX_SoundStream`, `ss`. 
`ss` is not started automatically, the user code must start it.
`ss` must exist for the lifetime of the `CX_SoundBufferRecorder`.
\param ss A pointer to a fully configured `CX_SoundStream`.
\return `true` if setup was successful, `false` otherwise.
*/
bool CX_SoundBufferRecorder::setup(CX_SoundStream* ss) {
	return setup(CX::Private::wrapPtr(ss));
}

bool CX_SoundBufferRecorder::setup(std::shared_ptr<CX_SoundStream> ss) {

	_cleanUpOldSoundStream();

	_soundStream = ss;

	if (!ss) {
		return false;
	}

	if (!ss->isStreamRunning()) {
		CX::Instances::Log.notice("CX_SoundBufferRecorder") << "setup(): Sound stream is not running.";
	}

	_inputEventHelper.setup<CX_SoundBufferRecorder>(&ss->inputEvent, this, &CX_SoundBufferRecorder::_inputEventHandler);
	_inputEventHelper.listenToStopEvent(&ss->destructEvent);

	resetSoundBuffer(true);

	return true;
}


/*! Reset the CX_SoundBuffer that is being used for recording.

\param createNewIfNeeded If the recorder does not have a sound buffer associated with it, create a new `CX_SoundBuffer` to record to. It can be accessed with `getSoundBuffer()`.
*/
void CX_SoundBufferRecorder::resetSoundBuffer(bool createNewIfNeeded) {

	stop();

	const CX_SoundStream::Configuration& ssc = _soundStream->getConfiguration();

	std::lock_guard<std::recursive_mutex> inputLock(_inData);

	if (createNewIfNeeded && !_inData.buffer) {
		_inData.buffer = std::make_shared<CX_SoundBuffer>();
	}

	if (_inData.buffer) {
		_inData.buffer->setFromVector(ssc.sampleRate, ssc.inputChannels, std::vector<float>());
	}

	_inData.recording = false;
	_inData.startingRecording = false;
	_inData.recordingQueued = false;

	_inData.queuedRecordingStartSampleFrame = std::numeric_limits<SampleFrame>::max();

	_inData.recordingStart = 0;
	_inData.recordingEnd = 0;

	_inData.overflowCount = 0;
}

/*! Associates a `CX_SoundBuffer` with the `CX_SoundBufferRecorder`.
The `buffer` will be recorded to when `record()` is called.

If `buffer` is not configured to use the same sample rate or number of
channels as the input stream, it will be cleared and configured as with 
settings from the stream when `record()` is called.

\param buffer The `CX_SoundBuffer` to associate with this `CX_SoundBufferRecorder`.

\return `true` on success, `false` otherwise.
*/
bool CX_SoundBufferRecorder::setSoundBuffer(std::shared_ptr<CX_SoundBuffer> buffer) {

	std::lock_guard<std::recursive_mutex> lock(_inData);

	if (buffer == nullptr) {
		//_inData.buffer = std::make_shared<CX_SoundBuffer>();
		resetSoundBuffer(true);
	} else {
		_inData.buffer = buffer;
	}

	return true;
}

/*! \copydoc CX::CX_SoundBufferRecorder::setSoundBuffer(std::shared_ptr<CX::CX_SoundBuffer>)  */
bool CX_SoundBufferRecorder::setSoundBuffer(CX_SoundBuffer* buffer) {
	std::shared_ptr<CX_SoundBuffer> buf = CX::Private::wrapPtr(buffer);
	return setSoundBuffer(buf);
}



/*! Returns a pointer to the `CX_SoundBuffer` that is currently in use by this `CX_SoundBufferRecorder`.
You should not access the sound buffer while recording is in progress. Use `isRecording()` to check recording state.
A warning will be logged if this function is called while recording is in progress, but a pointer to the buffer will
still be returned.
\return A `shared_ptr` to the sound buffer.
*/
std::shared_ptr<CX_SoundBuffer> CX_SoundBufferRecorder::getSoundBuffer(void) {
	if (isRecordingOrQueued()) {
		CX::Instances::Log.warning("CX_SoundBufferRecorder") << "getSoundBuffer(): Sound buffer pointer accessed while recording was queued or in progress.";
	}

	std::lock_guard<std::recursive_mutex> lock(_inData);
	return _inData.buffer;
}

/*! \brief Get the experiment time at which the recording started. This is not latency adjusted. 

Returns 0 if recording or queued to record.
*/
CX_Millis CX_SoundBufferRecorder::getRecordingStartTime(void) {
	if (isRecordingOrQueued()) {
		return CX_Millis(0);
	}

	std::lock_guard<std::recursive_mutex> lock(_inData);
	return _inData.recordingStart;
}

/*! \brief Get experiment the time at which the recording ended. This is not latency adjusted. */
CX_Millis CX_SoundBufferRecorder::getRecordingEndTime(void) {
	if (isRecordingOrQueued()) {
		return CX_Millis(0);
	}

	std::lock_guard<std::recursive_mutex> lock(_inData);
	return _inData.recordingEnd;
}

CX_Millis CX_SoundBufferRecorder::getRecordingLength(void) {
	std::lock_guard<std::recursive_mutex> lock(_inData);
	if (!_inData.buffer) {
		return CX_Millis(0);
	}

	return _inData.buffer->getLength();
}


/*! Begins recording data to the `CX_SoundBuffer` that is associated with this `CX_SoundBufferRecorder`.

See `setSoundBuffer()` for a way to associate a sound buffer of your choice with the recorder.

\param clear If `true`, any data in the `CX_SoundBuffer` will be deleted before recording starts.
\return `false` if recording could not start, `true` if recording started.
*/
bool CX_SoundBufferRecorder::record(bool clear) {

	std::lock_guard<std::recursive_mutex> inputLock(_inData);

	_prepareRecordBuffer(clear, "record");

	_inData.startingRecording = true;
	_inData.recording = true;

	return true;
}

/*! Stop recording sound data.

If recording is queued, cancels queued recording.

More sound data can be recorded at the end of the current 
recording by calling `record()` again before clearing the data.

*/
void CX_SoundBufferRecorder::stop(void) {
	std::lock_guard<std::recursive_mutex> inputLock(_inData);
	_inData.recording = false;
	_inData.recordingQueued = false;
	_inData.startingRecording = false;
}

bool CX_SoundBufferRecorder::isRecordingComplete(void) {

	bool success = !isRecordingOrQueued() && _inData.buffer && _inData.buffer->isReadyToPlay();

	// Other things to consider?

	return success;
}




/*! \brief Returns `true` if currently recording. */
bool CX_SoundBufferRecorder::isRecording(void) {
	std::lock_guard<std::recursive_mutex> inputLock(_inData);
	return _inData.recording;
}

bool CX_SoundBufferRecorder::queueRecording(SampleFrame sampleFrame, bool clear) {

	if (_soundStream == nullptr) {
		CX::Instances::Log.error("CX_SoundBufferRecorder") << "queueRecording(): Could not queue recording becuase the sound stream was nullptr. Have you forgotten to call setup()?";
		return false;
	}

	if (sampleFrame < _soundStream->swapData.getNextSwapUnit()) {
		CX::Instances::Log.warning("CX_SoundBufferPlayer") << "queueRecording(): Desired start sample frame has already passed. Starting immediately. "
			"Desired start SF: " << sampleFrame << ", next swap SF: " << _soundStream->swapData.getNextSwapUnit() << ".";
		record(clear);
		return false;
	}

	std::lock_guard<std::recursive_mutex> inputLock(_inData);

	_inData.queuedRecordingStartSampleFrame = sampleFrame;
	_inData.recordingQueued = true;

	_prepareRecordBuffer(clear, "queueRecording");

	return true;
}

bool CX_SoundBufferRecorder::queueRecording(CX_Millis startTime, CX_Millis timeout, bool clear) {
	Sync::DataClient& cl = _soundStream->swapClient;

	if (!cl.waitUntilAllReady(timeout)) {
		return false;
	}

	Sync::SwapUnitPrediction sp = cl.predictSwapUnitAtTime(startTime);

	if (sp.usable) {
		return queueRecording(sp.prediction(), clear);
	}
	return false;
}



bool CX_SoundBufferRecorder::isRecordingQueued(void) {
	std::lock_guard<std::recursive_mutex> inputLock(_inData);
	
	return _inData.recordingQueued;
}

bool CX_SoundBufferRecorder::isRecordingOrQueued(void) {
	return isRecording() || isRecordingQueued();
}

/*! Get the number of buffer overflows since the last check for overflows with this function.
The number of overflows is reset each time this function is called.
An overflow means that some unknown amount of sound data that should have been 
recorded was lost.
\param logOverflows If `true` and there have been any overflows since the last check, a message will be logged.
\return The number of buffer overflows since the last check.
*/
unsigned int CX_SoundBufferRecorder::getOverflowsSinceLastCheck(bool logOverflows) {
	std::lock_guard<std::recursive_mutex> inputLock(_inData);

	unsigned int ovf = _inData.overflowCount;
	_inData.overflowCount = 0;
	if (logOverflows && ovf > 0) {
		CX::Instances::Log.warning("CX_SoundBufferRecorder") << "There have been " << ovf << " buffer overflows since the last check.";
	}
	return ovf;
}


/*! Provides direct access to the CX_SoundStream used by the CX_SoundBufferRecorder. */
std::shared_ptr<CX_SoundStream> CX_SoundBufferRecorder::getSoundStream(void) {
	return _soundStream; 
}

void CX_SoundBufferRecorder::_prepareRecordBuffer(bool clear, std::string callingFunctionName) {

	// Set the characteristics of the sound to the configuration of the sound stream that is doing the recording.
	const CX_SoundStream::Configuration& ssc = _soundStream->getConfiguration();

	std::lock_guard<std::recursive_mutex> inputLock(_inData);

	if (_inData.buffer == nullptr) {
		//_inData.buffer = std::make_shared<CX_SoundBuffer>();
		clear = true;
	}

	if (!clear) {
		if (_inData.buffer->getChannelCount() != ssc.inputChannels || _inData.buffer->getSampleRate() != ssc.sampleRate) {
			CX::Instances::Log.warning("CX_SoundBufferRecorder") << callingFunctionName <<
				"(): The sample rate or number of channels don't match between the stored sound buffer and the input stream. The sound buffer will be cleared.";

			clear = true;
		}
	}

	if (clear) {
		resetSoundBuffer(true);
		//_inData.buffer->setFromVector(std::vector<float>(), ssc.inputChannels, ssc.sampleRate);
	}
}

void CX_SoundBufferRecorder::_inputEventHandler(const CX_SoundStream::InputEventArgs& inputData) {

	// Get timestamp immediately
	CX_Millis eventTime = Instances::Clock.now();

	std::lock_guard<std::recursive_mutex> inputLock(_inData);

	if (!_inData.recording && !_inData.recordingQueued) {
		return;
	}

	// Queued recording
	int64_t sampleFramesToRecord = inputData.bufferSize;
	int64_t inputBufferOffsetSF = 0;

	if (_inData.recordingQueued) {

		SampleFrame nextBufferStartSF = inputData.bufferStartSampleFrame + inputData.bufferSize;
		if (_inData.queuedRecordingStartSampleFrame >= nextBufferStartSF) {
			//Instances::Log.notice("CX_SoundBufferPlayer") << "Queued but not starting...";
			return;
		} else {
			//Instances::Log.notice("CX_SoundBufferPlayer") << "Queued and starting!!!";
			_inData.recording = true;
			_inData.startingRecording = true;
			_inData.recordingQueued = false;

			inputBufferOffsetSF = _inData.queuedRecordingStartSampleFrame - inputData.bufferStartSampleFrame;
			sampleFramesToRecord = inputData.bufferSize - inputBufferOffsetSF;
		}
	}

	//Instances::Log.notice("CX_SoundBufferPlayer") << "Recording!";
	
	// Timing
	if (_inData.startingRecording) {
		_inData.startingRecording = false;

		// When starting to record, the first buffer comes once it is full, which takes the amount of time per buffer.
		// Subtract buffer latency from the event time to get the time at which that buffer started being recorded.
		_inData.recordingStart = eventTime - _soundStream->getLatencyPerBuffer();
	}

	// The end of the recording is the current event time (minus unknown input latency)
	_inData.recordingEnd = eventTime;


	// Record to the SoundBuffer
	unsigned int totalNewSamples = sampleFramesToRecord * inputData.inputChannels;

	std::vector<float>& soundData = _inData.buffer->getRawDataReference();
	std::vector<float>::size_type currentBufferEnd = soundData.size();

	soundData.resize(soundData.size() + totalNewSamples);

	float* destination = soundData.data() + currentBufferEnd;
	float* source = inputData.inputBuffer + (inputBufferOffsetSF * inputData.inputChannels);

	memcpy(destination, source, (size_t)(totalNewSamples * sizeof(float)));

	if (inputData.bufferOverflow) {
		_inData.overflowCount++;
	}

}


void CX_SoundBufferRecorder::_cleanUpOldSoundStream(void) {
	stop();

	getOverflowsSinceLastCheck(true);

	_soundStream = nullptr;
}



} // namespace CX
