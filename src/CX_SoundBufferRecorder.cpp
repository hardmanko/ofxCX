#include "CX_SoundBufferRecorder.h"

namespace CX {

CX_SoundBufferRecorder::CX_SoundBufferRecorder(void) :
	_recording(false),
	_buffer(nullptr),
	_soundStream(nullptr),
	_soundStreamSelfAllocated(false),
	_listeningForEvents(false)
{
}

CX_SoundBufferRecorder::~CX_SoundBufferRecorder(void) {
	this->stop();
	if (_soundStream != nullptr) {
		_soundStream->closeStream();
		_listenForEvents(false);
		if (_soundStreamSelfAllocated) {
			delete _soundStream;
			_soundStreamSelfAllocated = false;
		}
	}
}

/*! This function sets up the CX_SoundStream that CX_SoundBufferRecorder uses to record audio data.
\param config A reference to a CX_SoundBufferRecorder::Configuration struct that will be used to configure
an internally-stored CX_SoundStream.
\return `true` if configuration of the CX_SoundStream was successful, `false` otherwise.
*/
bool CX_SoundBufferRecorder::setup(CX_SoundBufferRecorder::Configuration& config) {
	_cleanUpOldSoundStream();

	_soundStream = new CX_SoundStream;
	_soundStreamSelfAllocated = true;
	_listenForEvents(true);

	bool setupSuccessfully = _soundStream->setup((CX_SoundStream::Configuration&)config);

	if (_buffer != nullptr) {
		setSoundBuffer(_buffer);
	}

	bool startedSuccessfully = _soundStream->start();

	return startedSuccessfully && setupSuccessfully;
}

/*! Set up the sound buffer recorder from an existing CX_SoundStream. The CX_SoundStream is not started automatically.
The CX_SoundStream must remain in scope for the lifetime of the CX_SoundBufferRecorder.
\param ss A pointer to a fully configured CX_SoundStream.
\return `true` in all cases. */
bool CX_SoundBufferRecorder::setup(CX_SoundStream* ss) {
	_cleanUpOldSoundStream();

	_soundStream = ss;
	_soundStreamSelfAllocated = false;
	_listenForEvents(true);

	if (_buffer != nullptr) {
		setSoundBuffer(_buffer);
	}

	return true;
}

/*! This function associates a CX_SoundBuffer with the CX_SoundBufferRecorder. The CX_SoundBuffer
will be recorded to when start() is called.
\param so The CX_SoundBuffer to associate with the CX_SoundBufferRecorder. The sound buffer will be cleared
and it will be configured to have the same number of channels and sample rate that the CX_SoundBufferRecorder
was configured to use.
*/
void CX_SoundBufferRecorder::setSoundBuffer(CX_SoundBuffer* so) {
	_buffer = so;
	so->clear();
	const CX_SoundBufferRecorder::Configuration& config = this->getConfiguration();
	so->setFromVector(vector<float>(), config.inputChannels, config.sampleRate);
}

/*! This function returns a pointer to the CX_SoundBuffer that is currently in use by the CX_SoundBufferRecorder. */
CX_SoundBuffer* CX_SoundBufferRecorder::getSoundBuffer(void) {
	if (_recording) {
		CX::Instances::Log.warning("CX_SoundBufferRecorder") << "getSoundBuffer(): Sound buffer pointer accessed while recording was in progress.";
	}
	return _buffer;
}

/*! Begins recording data to the CX_SoundBuffer that was associated with this CX_SoundBufferRecorder
with setSoundBuffer().
\param clearExistingData If true, any data in the CX_SoundBuffer will be deleted before recording starts.
*/
void CX_SoundBufferRecorder::start(bool clearExistingData) {
	if (_buffer == nullptr) {
		CX::Instances::Log.error("CX_SoundBufferRecorder") << "start(): Unable to start recording because no CX_SoundBuffer was set.";
		return;
	}
	if (clearExistingData) {
		_buffer->getRawDataReference().clear();
	}
	_recording = true;
}

/*! \brief Stop recording sound data. */
void CX_SoundBufferRecorder::stop(void) {
	_recording = false;
}

/*! \brief Returns `true` is currently recording. */
bool CX_SoundBufferRecorder::isRecording(void) const {
	return _recording;
}


/*! Returns the configuration used for this CX_SoundBufferRecorder. */
CX_SoundBufferRecorder::Configuration CX_SoundBufferRecorder::getConfiguration(void) {
	if (_soundStream == nullptr) {
		CX::Instances::Log.error("CX_SoundBufferPlayer") << "getConfiguration(): Could not get configuration, the sound stream was nonexistent. Have you forgotten to call setup()?";
		return CX_SoundBufferRecorder::Configuration();
	}

	return (CX_SoundBufferRecorder::Configuration)_soundStream->getConfiguration();
}


bool CX_SoundBufferRecorder::_inputEventHandler(CX_SoundStream::InputEventArgs& inputData) {
	if (!_recording) {
		return false;
	}

	unsigned int totalNewSamples = inputData.bufferSize * inputData.inputChannels;

	inputData.inputBuffer;
	vector<float>& soundData = _buffer->getRawDataReference();
	unsigned int currentBufferEnd = soundData.size();

	soundData.resize(soundData.size() + totalNewSamples);

	memcpy(soundData.data() + currentBufferEnd, inputData.inputBuffer, (size_t)(totalNewSamples * sizeof(float)));
	return true;
}

void CX_SoundBufferRecorder::_listenForEvents(bool listen) {
	if ((listen == _listeningForEvents) || (_soundStream == nullptr)) {
		return;
	}

	if (listen) {
		ofAddListener(_soundStream->inputEvent, this, &CX_SoundBufferRecorder::_inputEventHandler);
	} else {
		ofRemoveListener(_soundStream->inputEvent, this, &CX_SoundBufferRecorder::_inputEventHandler);
	}

	_listeningForEvents = listen;
}

void CX_SoundBufferRecorder::_cleanUpOldSoundStream(void) {
	//If another sound stream was connected, stop listening to it.
	if (_soundStream != nullptr) {
		_listenForEvents(false);

		if (_soundStreamSelfAllocated) {
			delete _soundStream;
			_soundStreamSelfAllocated = false;
		}
	}
}

}