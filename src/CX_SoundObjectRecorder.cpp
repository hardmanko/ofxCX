#include "CX_SoundObjectRecorder.h"

using namespace CX;

CX_SoundObjectRecorder::CX_SoundObjectRecorder(void) :
	_recording(false),
	_soundObject(nullptr)
{
	ofAddListener(_soundStream.inputEvent, this, &CX_SoundObjectRecorder::_inputEventHandler);
}

CX_SoundObjectRecorder::~CX_SoundObjectRecorder(void) {
	this->stopRecording();
	_soundStream.closeStream();
	ofRemoveListener(_soundStream.inputEvent, this, &CX_SoundObjectRecorder::_inputEventHandler);
}

/*! This function sets up the CX_SoundStream that CX_SoundObjectRecorder uses to record audio data.
\return True if configuration of the CX_SoundStream was successful, false otherwise.
*/
bool CX_SoundObjectRecorder::setup(CX_SoundObjectRecorder::Configuration& config) {
	ofAddListener(_soundStream.inputEvent, this, &CX_SoundObjectRecorder::_inputEventHandler);

	bool success = _soundStream.setup(config);

	if (_soundObject != nullptr) {
		setSoundObject(_soundObject);
	}

	success = success && _soundStream.start();
	return success;
}

/*! This function associates a CX_SoundObject with the CX_SoundObjectRecorder. The CX_SoundObject
will be recorded to when startRecording() is called.
\param so The CX_SoundObject to associate with the CX_SoundObjectRecorder. The sound object will be cleared
and it will be configured to have the same number of channels and sample rate that the CX_SoundObjectRecorder
was configured to use.
*/
void CX_SoundObjectRecorder::setSoundObject(CX_SoundObject* so) {
	_soundObject = so;
	so->clear();
	so->setFromVector(vector<float>(), _soundStream.getConfiguration().inputChannels, _soundStream.getConfiguration().sampleRate);
}

/*! This function returns a pointer to the CX_SoundObject that is currently associated with this CX_SoundObjectRecorder. */
CX_SoundObject* CX_SoundObjectRecorder::getSoundObject(void) {
	if (_recording) {
		CX::Instances::Log.warning("CX_SoundObjectRecorder") << "getSoundObject: Sound object pointer accessed while recording was in progress.";
	}
	return _soundObject;
}

/*! Begins recording data to the CX_SoundObject that was associated with this CX_SoundObjectRecorder
with setSoundObject().
\param clearExistingData If true, any data in the CX_SoundObject will be deleted before recording starts.
*/
void CX_SoundObjectRecorder::startRecording(bool clearExistingData) {
	if (_soundObject == nullptr) {
		CX::Instances::Log.error("CX_SoundObjectRecorder") << "Unable to start recording because no CX_SoundObject was set.";
		return;
	}
	if (clearExistingData) {
		_soundObject->getRawDataReference().clear();
	}
	_recording = true;
}

/*! Stop recording sound data. */
void CX_SoundObjectRecorder::stopRecording(void) {
	_recording = false;
}


bool CX_SoundObjectRecorder::_inputEventHandler(CX_SoundStream::InputEventArgs& inputData) {
	if (!_recording) {
		return false;
	}

	unsigned int totalNewSamples = inputData.bufferSize * inputData.inputChannels;

	inputData.inputBuffer;
	vector<float>& soundData = _soundObject->getRawDataReference();
	unsigned int currentBufferEnd = soundData.size();

	soundData.resize(soundData.size() + totalNewSamples);

	memcpy(soundData.data() + currentBufferEnd, inputData.inputBuffer, (size_t)(totalNewSamples * sizeof(float)));
	return true;
}