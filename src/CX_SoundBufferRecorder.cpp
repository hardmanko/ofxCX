#include "CX_SoundBufferRecorder.h"

using namespace CX;

CX_SoundBufferRecorder::CX_SoundBufferRecorder(void) :
	_recording(false),
	_buffer(nullptr)
{
	ofAddListener(_soundStream.inputEvent, this, &CX_SoundBufferRecorder::_inputEventHandler);
}

CX_SoundBufferRecorder::~CX_SoundBufferRecorder(void) {
	this->stopRecording();
	_soundStream.closeStream();
	ofRemoveListener(_soundStream.inputEvent, this, &CX_SoundBufferRecorder::_inputEventHandler);
}

/*! This function sets up the CX_SoundStream that CX_SoundBufferRecorder uses to record audio data.
\return True if configuration of the CX_SoundStream was successful, false otherwise.
*/
bool CX_SoundBufferRecorder::setup(CX_SoundBufferRecorder::Configuration& config) {
	ofAddListener(_soundStream.inputEvent, this, &CX_SoundBufferRecorder::_inputEventHandler);

	bool success = _soundStream.setup(config);

	if (_buffer != nullptr) {
		setSoundBuffer(_buffer);
	}

	success = success && _soundStream.start();
	return success;
}

/*! This function associates a CX_SoundBuffer with the CX_SoundBufferRecorder. The CX_SoundBuffer
will be recorded to when startRecording() is called.
\param so The CX_SoundBuffer to associate with the CX_SoundBufferRecorder. The sound buffer will be cleared
and it will be configured to have the same number of channels and sample rate that the CX_SoundBufferRecorder
was configured to use.
*/
void CX_SoundBufferRecorder::setSoundBuffer(CX_SoundBuffer* so) {
	_buffer = so;
	so->clear();
	so->setFromVector(vector<float>(), _soundStream.getConfiguration().inputChannels, _soundStream.getConfiguration().sampleRate);
}

/*! This function returns a pointer to the CX_SoundBuffer that is currently associated with this CX_SoundBufferRecorder. */
CX_SoundBuffer* CX_SoundBufferRecorder::getSoundBuffer(void) {
	if (_recording) {
		CX::Instances::Log.warning("CX_SoundBufferRecorder") << "getSoundBuffer: Sound buffer pointer accessed while recording was in progress.";
	}
	return _buffer;
}

/*! Begins recording data to the CX_SoundBuffer that was associated with this CX_SoundBufferRecorder
with setSoundBuffer().
\param clearExistingData If true, any data in the CX_SoundBuffer will be deleted before recording starts.
*/
void CX_SoundBufferRecorder::startRecording(bool clearExistingData) {
	if (_buffer == nullptr) {
		CX::Instances::Log.error("CX_SoundBufferRecorder") << "Unable to start recording because no CX_SoundBuffer was set.";
		return;
	}
	if (clearExistingData) {
		_buffer->getRawDataReference().clear();
	}
	_recording = true;
}

/*! Stop recording sound data. */
void CX_SoundBufferRecorder::stopRecording(void) {
	_recording = false;
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