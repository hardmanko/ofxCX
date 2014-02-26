#include "CX_ModularSynth.h"

ModuleBase& operator>>(ModuleBase& l, ModuleBase& r) {
	r._assignInput(&l);
	l._assignOutput(&r);
	return r;
}

void ModuleBase::_assignInput(ModuleBase* in) {
	if (_maxInputs() == 0) {
		return;
	}

	if (std::find(_inputs.begin(), _inputs.end(), in) == _inputs.end()) { //If it is not in the vector, try to add it.

		if (_inputs.size() == _maxInputs()) { //If the vector is full, pop off an element before adding the new one.
			_inputs.pop_back();
		}

		_inputs.push_back(in);
		_setDataIfNotSet(in);
		_inputAssignedEvent(in);
	}
}

void ModuleBase::_assignOutput(ModuleBase* out) {
	if (_maxOutputs() == 0) {
		return;
	}

	if (std::find(_outputs.begin(), _outputs.end(), out) == _outputs.end()) {

		if (_outputs.size() == _maxOutputs()) {
			_outputs.pop_back();
		}

		_outputs.push_back(out);
		_setDataIfNotSet(out);
		_outputAssignedEvent(out);
	}
}

void ModuleBase::_dataSet(ModuleBase* caller) {
	this->_dataSetEvent();

	for (unsigned int i = 0; i < _inputs.size(); i++) {
		if (_inputs[i] != nullptr && _inputs[i] != caller) {
			_setDataIfNotSet(_inputs[i]);
		}
	}

	for (unsigned int i = 0; i < _outputs.size(); i++) {
		if (_outputs[i] != nullptr && _outputs[i] != caller) {
			_setDataIfNotSet(_outputs[i]);
		}
	}

	for (unsigned int i = 0; i < _parameters.size(); i++) {
		if (_parameters[i]->_input != nullptr) {
			_setDataIfNotSet(_parameters[i]->_input);
		}
	}
}

void ModuleBase::_setDataIfNotSet(ModuleBase* target) {
	//Compare both pointers and contents. The pointer comparison is currently useless because the pointers will always be different.
	if ((target->_data != this->_data) || (*target->_data != *this->_data)) {
		*target->_data = *this->_data;
		target->_dataSet(this);
	}

}

void ModuleBase::_registerParameter(ModuleParameter* p) {
	if (std::find(_parameters.begin(), _parameters.end(), p) == _parameters.end()) {
		_parameters.push_back(p);
		p->_owner = this;
	}
}



double Envelope::getNextSample(void) {
	if (stage > 3) {
		return 0;
	}

	double p;

	switch (stage) {
	case 0:
		if (_timeSinceLastStage < a && a != 0) {
			p = _timeSinceLastStage / a;
			break;
		} else {
			_timeSinceLastStage = 0;
			stage++;
		}
	case 1:
		if (_timeSinceLastStage < d && d != 0) {
			p = 1 - (_timeSinceLastStage / d) * (1 - s);
			break;
		} else {
			_timeSinceLastStage = 0;
			stage++;
		}
	case 2:
		p = s;
		break;
	case 3:
		if (_timeSinceLastStage < r && r != 0) {
			p = (1 - _timeSinceLastStage / r) * _levelAtRelease;
			break;
		} else {
			//_timeSinceLastStage = 0;
			stage++;
			p = 0;
		}
	}

	_lastP = p;

	_timeSinceLastStage += _timePerSample;

	double val;
	if (_inputs.size() > 0) {
		val = _inputs.front()->getNextSample();
	} else {
		val = 1;
	}

	return val * p;
}

void Envelope::attack(void) {
	stage = 0;
	_timeSinceLastStage = 0;
}

void Envelope::release(void) {
	stage = 3;
	_timeSinceLastStage = 0;
	_levelAtRelease = _lastP;
}

void Envelope::_dataSetEvent(void) {
	_timePerSample = 1 / _data->sampleRate;
}


////////////////
// Oscillator //
////////////////

Oscillator::Oscillator(void) :
	frequency(0),
	_waveformPos(0)
{
	this->_registerParameter(&frequency);
	setGeneratorFunction(Oscillator::sine);
}

double Oscillator::getNextSample(void) {
	frequency.updateValue();
	double addAmount = frequency.getValue() / _sampleRate;

	_waveformPos += addAmount;
	if (_waveformPos >= 1) {
		_waveformPos = fmod(_waveformPos, 1);
	}

	return _generatorFunction(_waveformPos);
}

void Oscillator::setGeneratorFunction(std::function<double(double)> f) {
	_generatorFunction = f;
}

void Oscillator::_dataSetEvent(void) {
	_sampleRate = _data->sampleRate;
}

double Oscillator::saw(double wp) {
	return (2 * wp) - 1;
}

double Oscillator::sine(double wp) {
	return sin(wp * 2 * PI);
}

double Oscillator::square(double wp) {
	if (wp < .5) {
		return 1;
	} else {
		return -1;
	}
}

double Oscillator::triangle(double wp) {
	if (wp < .5) {
		return ((4 * wp) - 1);
	} else {
		return (3 - (4 * wp));
	}
}




//////////////////
// StreamOutput //
//////////////////
void StreamOutput::setOuputStream(CX::CX_SoundStream& stream) {
	ofAddListener(stream.outputCallbackEvent, this, &StreamOutput::_callback);
	ModuleControlData_t data;
	data.sampleRate = stream.getConfiguration().sampleRate;
	this->setData(data);
}

void StreamOutput::_callback(CX::CX_SSOutputCallback_t& d) {

	if (_inputs.front() == nullptr) {
		return;
	}

	for (unsigned int sample = 0; sample < d.bufferSize; sample++) {
		float value = ofClamp(_inputs.front()->getNextSample(), -1, 1);
		//cout << value << endl;
		for (int ch = 0; ch < d.outputChannels; ch++) {
			d.outputBuffer[(sample * d.outputChannels) + ch] = value;
		}
	}
}

//////////////////////
// SoundObjectInput //
//////////////////////
SoundObjectInput::SoundObjectInput(void) :
	_currentSample(0),
	_so(nullptr)
{}

//Set the sound object to use and also the channel to use (you can only use one channel: strictly monophonic).
void SoundObjectInput::setSoundObject(CX::CX_SoundObject *so, unsigned int channel) {
	_so = so;
	_channel = channel;

	_data->sampleRate = _so->getSampleRate();
	_data->initialized = true;
	_dataSet(nullptr);
}

//Set where in the sound you are
void SoundObjectInput::setTime(double t) {
	if (_so != nullptr) {
		unsigned int startSample = _so->getChannelCount() * (unsigned int)(_so->getSampleRate() * t);
		_currentSample = startSample + _channel;
	} else {
		_currentSample = _channel;
	}

}

double SoundObjectInput::getNextSample(void) {
	if (_so == nullptr || _currentSample >= _so->getTotalSampleCount()) {
		return 0;
	}
	double value = _so->getRawDataReference().at(_currentSample);
	_currentSample += _so->getChannelCount();
	return value;
}

bool SoundObjectInput::canPlay(void) {
	return (_so != nullptr) && (_so->isReadyToPlay()) && (_currentSample < _so->getTotalSampleCount());
}


///////////////////////
// SoundObjectOutput //
///////////////////////
void SoundObjectOutput::setup(float sampleRate) {
	_data->sampleRate = sampleRate;
	_data->initialized = true;
	_dataSet(nullptr);
}

//Sample t seconds of data at the given sample rate (see setup). The result is stored in so.
//If so is not empty, the data is appended.
void SoundObjectOutput::sampleData(double t) {

	unsigned int samplesToTake = ceil(_data->sampleRate * t);

	vector<float> tempData(samplesToTake);

	for (unsigned int i = 0; i < samplesToTake; i++) {
		tempData[i] = ofClamp((float)_inputs.front()->getNextSample(), -1, 1);
	}

	if (so.getTotalSampleCount() == 0) {
		so.setFromVector(tempData, 1, _data->sampleRate);
	} else {
		for (unsigned int i = 0; i < tempData.size(); i++) {
			so.getRawDataReference().push_back(tempData[i]);
		}
	}
}





/*
class Noisemaker {
public:

	Noisemaker(void) :
		frequency(0),
		maxAmplitude(0),
		_waveformPos(0)
	{
		_generatorFunction = Noisemaker::sine;
	}

	void setOuputStream(CX_SoundStream& stream) {
		ofAddListener(stream.outputCallbackEvent, this, &Noisemaker::_callback);
	}

	void setGeneratorFunction(std::function<double(double)> f) {
		_generatorFunction = f;
	}

	double frequency;
	float maxAmplitude;

	static double sine(double wp) {
		return sin(wp * 2 * PI);
	}

	static double triangle(double wp) {
		if (wp < .5) {
			return ((4 * wp) - 1);
		} else {
			return (3 - (4 * wp));
		}
	}

	static double square(double wp) {
		if (wp < .5) {
			return 1;
		} else {
			return -1;
		}
	}

	static double saw(double wp) {
		return (2 * wp) - 1;
	}

private:

	std::function<double(double)> _generatorFunction;

	float _waveformPos;

	void _callback(CX_SSOutputCallback_t& d) {

		float addAmount = frequency / d.instance->getConfiguration().sampleRate;

		for (unsigned int sample = 0; sample < d.bufferSize; sample++) {
			_waveformPos += addAmount;
			if (_waveformPos >= 1) {
				_waveformPos = fmod(_waveformPos, 1);
			}
			double value = ofClamp(_generatorFunction(_waveformPos) * maxAmplitude, -1, 1);
			for (int ch = 0; ch < d.outputChannels; ch++) {
				d.outputBuffer[(sample * d.outputChannels) + ch] = value;
			}
		}

	}

};
*/