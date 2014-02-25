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
};

class ModuleBase {
public:

	ModuleBase(void) :
		input(nullptr)
		//output(nullptr)
	{
		data = std::shared_ptr<ModuleControlData_t>(new ModuleControlData_t);
	}

	virtual double getNextSample(void) {
		return 0;
	}

	virtual void setInput(ModuleBase* newInput) {
		input = newInput;
		//if (this->data->initialized) {
		//	newInput->data = data;
		//} else {
		//	data = newInput->data;
		//}
		if (newInput->output != this) {
			newInput->setOutput(this);
		}
		_dataSet();
	}

	virtual void setOutput(ModuleBase* newOutput) {
		output = newOutput;
		//if (!output->data->initialized) {
		//	output->setData(*data);
		//}
		if (newOutput->input != this) {
			newOutput->setInput(this);
		}
		_dataSet();
	}

	void setData(ModuleControlData_t d) {
		*data = d;
		data->initialized = true;
		_dataSet();
	}

	ModuleBase *input;
	ModuleBase *output;

	std::shared_ptr<ModuleControlData_t> data;

protected:

	virtual void _dataSetEvent(void) {
		return;
	}

	void _dataSet(void) {
		this->_dataSetEvent();
		if (input != nullptr) {
			if (!input->data->initialized) {
				input->data = this->data;
			}
			input->_dataSet();
		}
	}

};

class Mixer : public ModuleBase {
public:

	double getNextSample(void) {
		double d = 0;
		for (int i = 0; i < inputs.size(); i++) {
			d += inputs[i]->getNextSample();
		}
		return d;
	}

	void addInput(ModuleBase *m) {
		if (std::find(inputs.begin(), inputs.end(), m) == inputs.end()) {
			inputs.push_back(m);
			m->setOutput(this);
		}
	}

	void setInput(ModuleBase* newInput) {
		input = newInput;
		if (newInput->output != this) {
			newInput->setOutput(this);
		}
		_dataSet();
	}

	void removeInput(ModuleBase *m) {
		auto pos = std::find(inputs.begin(), inputs.end(), m);
		if (pos != inputs.end()) {
			inputs.erase(pos);
		}
	}

private:

	vector<ModuleBase*> inputs;

};

class Splitter : public ModuleBase {
public:

	Splitter(void) :
		_allOutputsFed(false),
		_currentSample(0.0),
		_fedOutputs(0)
	{}

	void addOutput(ModuleBase* m) {
		if (std::find(outputs.begin(), outputs.end(), m) == outputs.end()) {
			outputs.push_back(m);
			m->setInput(this);
			_fedOutputs = outputs.size();
		}
	}

	double getNextSample(void) {
		if (_fedOutputs >= outputs.size()) {
			_currentSample = input->getNextSample();
			_fedOutputs = 0;
		}
		++_fedOutputs;
		return _currentSample;
	}

private:

	bool _allOutputsFed;
	double _currentSample;
	int _fedOutputs;

	vector<ModuleBase*> outputs;
};

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
	}

	void setTime(double t) {
		if (data->initialized) {
			unsigned int startSample = (unsigned int)(data->sampleRate * t);
			startSample -= startSample % _so->getChannelCount();
			_currentSample = startSample + _channel;
		}
		_currentSample = _channel;
	}

	double getNextSample(void) {
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

	void _dataSetEvent(void) {
		//_timePerSample = 1 / data->sampleRate;
	}

	unsigned int _currentSample;

};

class SoundObjectOutput : public ModuleBase {
public:

	//Sample t seconds of data at the given sample rate. The result is stored in so.
	void sampleData(double t, float sampleRate) {
		unsigned int samplesToTake = ceil(sampleRate * t);
		data->sampleRate = sampleRate;
		data->initialized = true;
		this->_dataSet();

		so.deleteAmount(so.getLength() + 1000000, true); //Delete all of the sound, plus an extra second to be sure

		vector<float> tempData(samplesToTake);

		for (unsigned int i = 0; i < samplesToTake; i++) {
			tempData[i] = ofClamp((float)input->getNextSample(), -1, 1);
		}

		so.setFromVector(tempData, 1, sampleRate);
	}

	CX::CX_SoundObject so;

private:


};

class Envelope : public ModuleBase {
public:

	Envelope(void) :
		stage(4)
	{}

	double getNextSample(void);

	void gate(void);
	void release(void);

	int stage;

	double a; //Time
	double d; //Time
	double s; //Level
	double r; //Time

private:

	double _lastP;
	double _levelAtRelease;

	double _timePerSample;
	double _timeSinceLastStage;

	void _dataSetEvent(void) {
		_timePerSample = 1 / data->sampleRate;
	}

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

	float _sampleRate;
	double _waveformPos;

	void _dataSetEvent(void) {
		_sampleRate = data->sampleRate;
	}

};

class RecursiveFilter : public ModuleBase {
public:

	RecursiveFilter(void) :
		x1(0),
		y1(0)
	{}

	double getNextSample(void) override {
		if (input == nullptr) {
			return 0;
		}

		return _update(input->getNextSample());
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
		v0(0)
	{}

	double getNextSample(void) override {
		if (input == nullptr) {
			return 0;
		}

		return _update(input->getNextSample());
	}

	void setBreakpoint(double freq) {
		rc = 1 / (2 * PI * freq);
		multiplier = dt / rc;
	}

	double v0;

	double rc; //Set based on cutoff frequency
	double dt; //Set by sample rate

	double multiplier; // = dt / rc;

private:

	void _dataSetEvent(void) override {
		dt = 1 / (double)data->sampleRate;
		multiplier = dt / rc;
	}

	double _update(double v1) {
		v0 += (v1 - v0) * multiplier;
		return v0;
	}
};

class Amplifier : public ModuleBase {
public:

	Amplifier(void) :
		amplitude(1)
	{}

	double amplitude;

	double getNextSample(void) override {
		if (input == nullptr) {
			return 0;
		}
		return input->getNextSample() * amplitude;
	}
};

class StreamOutput : public ModuleBase {
public:
	void setOuputStream(CX::CX_SoundStream& stream);

private:
	void _callback(CX::CX_SSOutputCallback_t& d);
};

