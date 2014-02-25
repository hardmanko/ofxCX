#pragma once

#include "ofEvents.h"
#include "CX_SoundStream.h"
#include "CX_SoundObject.h"


struct ModuleControlData_t {
	ModuleControlData_t(void) :
		initialized(false)
	{}

	bool initialized;
	float sampleRate;

	bool operator==(const ModuleControlData_t& right) {
		return ((this->initialized == right.initialized) && (this->sampleRate == right.sampleRate));
	}

	bool operator!=(const ModuleControlData_t& right) {
		return ((this->initialized != right.initialized) || (this->sampleRate != right.sampleRate));
	}
};

class ModuleBase {
public:

	ModuleBase(void)
	{
		_data = std::shared_ptr<ModuleControlData_t>(new ModuleControlData_t);
	}

	virtual double getNextSample(void) {
		return 0;
	}

	void setData(ModuleControlData_t d) {
		*_data = d;
		_data->initialized = true;
		this->_dataSet(nullptr);
	}

protected:

	virtual int _maxInputs(void) {
		return 1;
	}

	virtual int _maxOutputs(void) {
		return 1;
	}

	friend ModuleBase& operator>>(ModuleBase& l, ModuleBase& r);

	//virtual void _dataSetEvent(void) { return; }

	vector<ModuleBase*> _inputs;
	vector<ModuleBase*> _outputs;
	std::shared_ptr<ModuleControlData_t> _data;

	virtual void _assignInput(ModuleBase* in) {
		if (std::find(_inputs.begin(), _inputs.end(), in) == _inputs.end()) { //If it is not in the vector, try to add it.
			
			if (_inputs.size() == _maxInputs()) { //If the vector is full, pop off an element before adding the new one.
				_inputs.pop_back();
			}

			_inputs.push_back(in);
			_setDataIfNotSet(in);
			_inputAssigned(in);
		}
	}

	virtual void _assignOutput(ModuleBase* out) {
		if (std::find(_outputs.begin(), _outputs.end(), out) == _outputs.end()) {

			if (_outputs.size() == _maxOutputs()) {
				_outputs.pop_back();
			}

			_outputs.push_back(out);
			_setDataIfNotSet(out);
			_outputAssigned(out);
		}
	}

	virtual void _dataSet(ModuleBase* caller) {
		//this->_dataSetEvent();

		for (int i = 0; i < _inputs.size(); i++) {
			if (_inputs[i] != nullptr && _inputs[i] != caller) {
				_setDataIfNotSet(_inputs[i]);
			}
		}

		for (int i = 0; i < _outputs.size(); i++) {
			if (_outputs[i] != nullptr && _outputs[i] != caller) {
				_setDataIfNotSet(_outputs[i]);
			}
		}
	}

	void _setDataIfNotSet(ModuleBase* target) {
		if (target->_data != this->_data) {
			target->_data = this->_data;
			target->_dataSet(this);
		}
		//target->_dataSetEvent();
	}

	virtual void _inputAssigned(ModuleBase* in) {
		return;
	}

	virtual void _outputAssigned(ModuleBase* out) {
		return;
	}

};

//For testing purposes
class TrivialGenerator : public ModuleBase {
public:

	TrivialGenerator(void) :
		value(0),
		step(0)
	{}

	double value;
	double step;

	double getNextSample(void) {
		value += step;
		return value - step;
	}

};

class Adder : public ModuleBase {
public:

	double getNextSample(void) {
		double d = 0;
		for (int i = 0; i < _inputs.size(); i++) {
			d += _inputs[i]->getNextSample();
		}
		return d;
	}

private:

	int _maxInputs(void) override {
		return 32;
	}
};

class Splitter : public ModuleBase {
public:

	Splitter(void) :
		_currentSample(0.0),
		_fedOutputs(0)
	{}

	/*
	void addOutput(ModuleBase* m) {
		if (std::find(outputs.begin(), outputs.end(), m) == outputs.end()) {
			outputs.push_back(m);
			m->setInput(this);
			_fedOutputs = outputs.size();
		}
	}
	*/

	double getNextSample(void) override {
		if (_fedOutputs >= _outputs.size()) {
			_currentSample = _inputs.front()->getNextSample();
			_fedOutputs = 0;
		}
		++_fedOutputs;
		return _currentSample;
	}

private:

	void _outputAssigned(ModuleBase* out) override {
		_fedOutputs = _outputs.size();
	}

	int _maxOutputs(void) override {
		return 32;
	}

	double _currentSample;
	int _fedOutputs;

	//vector<ModuleBase*> outputs;
};

class SoundObjectInput : public ModuleBase {
public:

	SoundObjectInput(void) :
		_currentSample(0),
		_so(nullptr)
	{}

	//Set the sound object to use and also the channel to use (you can only use one channel: strictly monophonic).
	void setSoundObject(CX::CX_SoundObject *so, unsigned int channel = 0) {
		_so = so;
		_channel = channel;

		_data->sampleRate = _so->getSampleRate();
		_data->initialized = true;
		_dataSet(nullptr);
	}

	//Set where in the sound you are
	void setTime(double t) {
		if (_so != nullptr) {
			unsigned int startSample = _so->getChannelCount() * (unsigned int)(_so->getSampleRate() * t);
			_currentSample = startSample + _channel;
		} else {
			_currentSample = _channel;
		}
		
	}

	double getNextSample(void) override {
		if (_so == nullptr || _currentSample >= _so->getTotalSampleCount()) {
			return 0;
		}
		double value = _so->getRawDataReference().at(_currentSample);
		_currentSample += _so->getChannelCount();
		return value;
	}

	bool canPlay (void) {
		return (_so != nullptr) && (_so->isReadyToPlay()) && (_currentSample < _so->getTotalSampleCount());
	}

private:

	CX::CX_SoundObject *_so;
	unsigned int _channel;

	//void _dataSetEvent(void) {
		//_timePerSample = 1 / data->sampleRate;
	//}

	unsigned int _currentSample;

};

class SoundObjectOutput : public ModuleBase {
public:

	void setup(float sampleRate) {
		_data->sampleRate = sampleRate;
		_data->initialized = true;
	}

	//Sample t seconds of data at the given sample rate (see setup). The result is stored in so.
	//If so is not empty, the data is appended.
	void sampleData(double t) {

		unsigned int samplesToTake = ceil(_data->sampleRate * t);
		//_data->sampleRate = sampleRate;
		//_data->initialized = true;
		//this->_dataSet(nullptr);

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

	CX::CX_SoundObject so;

};

class Envelope : public ModuleBase {
public:

	Envelope(void) :
		stage(4)
	{}

	double getNextSample(void);

	void attack(void);
	void release(void);

	int stage;

	double a; //Time
	double d; //Time
	double s; //Level
	double r; //Time

private:

	double _lastP;
	double _levelAtRelease;

	//double _timePerSample;
	double _timeSinceLastStage;

	//void _dataSetEvent(void) {
	//	_timePerSample = 1 / _data->sampleRate;
	//}

};

class Oscillator : public ModuleBase {
public:

	Oscillator(void);

	double getNextSample(void) override;

	void setGeneratorFunction(std::function<double(double)> f);

	double frequency;

	static double saw(double wp);
	static double sine(double wp);
	static double square(double wp);
	static double triangle(double wp);

private:

	std::function<double(double)> _generatorFunction;

	//float _sampleRate;
	double _waveformPos;

	//void _dataSetEvent(void) {
	//	_sampleRate = _data->sampleRate;
	//}

};

class RecursiveFilter : public ModuleBase {
public:

	RecursiveFilter(void) :
		x1(0),
		y1(0)
	{}

	double getNextSample(void) override {
		if (_inputs.front() == nullptr) {
			return 0;
		}

		return _update(_inputs.front()->getNextSample());
	}

	void setBreakpoint(double freq) {
		x = exp(-2 * PI * freq);

		//x = 0.999;

		//Low pass coefficients
		a0 = 1 - x;
		a1 = 0;
		b1 = x;

		//High pass coefficients
		//a0 = (1 + x) / 2;
		//a1 = -(1 + x) / 2;
		//b1 = x;
	}

private:

	//void _dataSetEvent(void) override {
	//	dt = 1 / (float)data->sampleRate;
	//	multiplier = dt / rc;
	//}

	double _update(double x0) {
		double y0 = a0*x0 + a1*x1 + b1 * y1;
		y1 = y0;
		x1 = x0;
		return y0;
	}

	double a0;
	double a1;
	double b1;

	double x1;
	double y1;

	/*
	vector<float> aCoefs;
	vector<float> bCoefs;

	deque<float> oldData;
	deque<float> oldOutputs;
	*/

	double x;

};

class RCFilter : public ModuleBase {
public:

	RCFilter(void) :
		_v0(0),
		_cutoffFrequency(2000)
	{}

	double getNextSample(void) override {
		if (_inputs.front() == nullptr) {
			return 0;
		}

		return _update(_inputs.front()->getNextSample());
	}

	void setBreakpoint(double freq) {
		_cutoffFrequency = freq;
	}

private:

	double _v0;

	double _cutoffFrequency;

	double _update(double v1) {
		_v0 += (v1 - _v0) * (2 * PI * _cutoffFrequency) / _data->sampleRate;
		return _v0;
	}
};

class Multiplier : public ModuleBase {
public:

	Multiplier(void) :
		amount(1)
	{}

	double amount;

	double getNextSample(void) override {
		if (_inputs.front() == nullptr) {
			return 0;
		}
		return _inputs.front()->getNextSample() * amount;
	}
};

class StreamOutput : public ModuleBase {
public:
	void setOuputStream(CX::CX_SoundStream& stream);

private:
	void _callback(CX::CX_SSOutputCallback_t& d);
};

