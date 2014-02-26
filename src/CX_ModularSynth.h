#pragma once

#include "ofEvents.h"
#include "CX_SoundStream.h"
#include "CX_SoundObject.h"

#include "CX_RandomNumberGenerator.h" //For white noise generator

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
		return !(this->operator==(right));
		//return ((this->initialized != right.initialized) || (this->sampleRate != right.sampleRate));
	}
};

class ModuleParameter;

class ModuleBase {
public:

	ModuleBase(void) {
		//_data = std::shared_ptr<ModuleControlData_t>(new ModuleControlData_t);
		_data = new ModuleControlData_t; //Keep as pointer for now, but you should change to non-pointer soon.
	}

	~ModuleBase(void) {
		delete _data;
	}

	//This function should be overloaded for any derived class that produces values (outputs do not produce values, they produce sound via sound hardware).
	virtual double getNextSample(void) {
		return 0;
	}

	//This function is not usually needed. If an appropriate input or output is connected, the data will be set from that module.
	void setData(ModuleControlData_t d) {
		*_data = d;
		_data->initialized = true;
		this->_dataSet(nullptr);
	}

	ModuleControlData_t getData(void) {
		return *_data;
	}



protected:

	//Used to connect modules together. l is set as the input for r.
	friend ModuleBase& operator>>(ModuleBase& l, ModuleBase& r);

	virtual void _dataSetEvent(void) { return; }

	virtual int _maxInputs(void) {
		return 1;
	}

	virtual int _maxOutputs(void) {
		return 1;
	}

	vector<ModuleBase*> _inputs;
	vector<ModuleBase*> _outputs;
	vector<ModuleParameter*> _parameters;
	//std::shared_ptr<ModuleControlData_t> _data;
	ModuleControlData_t *_data;

	virtual void _assignInput(ModuleBase* in);
	virtual void _assignOutput(ModuleBase* out);

	void _dataSet(ModuleBase* caller);
	void _setDataIfNotSet(ModuleBase* target);

	virtual void _inputAssignedEvent(ModuleBase* in) {
		return;
	}

	virtual void _outputAssignedEvent(ModuleBase* out) {
		return;
	}

	void _registerParameter(ModuleParameter* p);

};


class ModuleParameter {
public:

	ModuleParameter(void) :
		_data(0),
		_input(nullptr),
		_owner(nullptr)
	{}

	ModuleParameter(double d) :
		_data(d),
		_input(nullptr),
		_owner(nullptr)
	{}

	virtual void updateValue(void) {
		if (_input != nullptr) { //If there is no input connected, just keep the same value.
			_data = _input->getNextSample();
		}
	}

	double& getValue(void) {
		return _data;
	}

	operator double(void) {
		return _data;
	}

	double& operator=(double d) {
		_data = d;
		return _data;
	}



	friend void operator>>(ModuleBase& l, ModuleParameter& r) {
		r._input = &l;
		l.setData(r._owner->getData());
	}

private:
	friend class ModuleBase;

	ModuleBase* _owner;

	ModuleBase* _input;

	double _data;
};



//For testing purposes
class TrivialGenerator : public ModuleBase {
public:

	TrivialGenerator(void) :
		value(0),
		step(0)
	{
		this->_registerParameter(&value);
		this->_registerParameter(&step);
	}

	ModuleParameter step;
	ModuleParameter value;

	double getNextSample(void) override {
		value.updateValue();
		value.getValue() += step;
		return value.getValue() - step;
	}

};

class Mixer : public ModuleBase {
public:

	double getNextSample(void) override {
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

class Adder : public ModuleBase {
public:
	Adder(void) :
		amount(0)
	{
		this->_registerParameter(&amount);
	}

	ModuleParameter amount;

	double getNextSample(void) override {
		amount.updateValue();
		if (_inputs.size() > 0) {
			return amount.getValue() + _inputs.front()->getNextSample();
		}
		return amount.getValue();
	}
};

class Multiplier : public ModuleBase {
public:

	Multiplier(void) :
		amount(1)
	{
		this->_registerParameter(&amount);
	}

	ModuleParameter amount;

	double getNextSample(void) override {
		if (_inputs.front() == nullptr) {
			return 0;
		}
		amount.updateValue();
		return _inputs.front()->getNextSample() * amount.getValue();
	}
};

class Splitter : public ModuleBase {
public:

	Splitter(void) :
		_currentSample(0.0),
		_fedOutputs(0)
	{}


	double getNextSample(void) override {
		if (_fedOutputs >= _outputs.size()) {
			_currentSample = _inputs.front()->getNextSample();
			_fedOutputs = 0;
		}
		++_fedOutputs;
		return _currentSample;
	}

private:

	void _outputAssignedEvent(ModuleBase* out) override {
		_fedOutputs = _outputs.size();
	}

	int _maxOutputs(void) override {
		return 32;
	}

	double _currentSample;
	int _fedOutputs;
};

class SoundObjectInput : public ModuleBase {
public:

	SoundObjectInput(void);

	void setSoundObject(CX::CX_SoundObject *so, unsigned int channel = 0);

	void setTime(double t);

	double getNextSample(void) override;

	bool canPlay(void);

private:

	CX::CX_SoundObject *_so;
	unsigned int _channel;
	unsigned int _currentSample;

};



class Envelope : public ModuleBase {
public:

	Envelope(void) :
		stage(4)
	{}

	double getNextSample(void) override;

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

	double _timePerSample;
	double _timeSinceLastStage;

	void _dataSetEvent(void);

};

class Oscillator : public ModuleBase {
public:

	Oscillator(void);

	double getNextSample(void) override;

	void setGeneratorFunction(std::function<double(double)> f);

	ModuleParameter frequency;

	static double saw(double wp);
	static double sine(double wp);
	static double square(double wp);
	static double triangle(double wp);
	static double whiteNoise(double wp);

private:

	std::function<double(double)> _generatorFunction;

	float _sampleRate;
	double _waveformPos;

	void _dataSetEvent(void);

};

class StreamOutput : public ModuleBase {
public:
	void setOuputStream(CX::CX_SoundStream& stream);

private:
	void _callback(CX::CX_SSOutputCallback_t& d);

	int _maxOutputs(void) override {
		return 0;
	}
};

class SoundObjectOutput : public ModuleBase {
public:

	void setup(float sampleRate);

	void sampleData(double t);

	CX::CX_SoundObject so;

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
		//_cutoffFrequency(2000),
		breakpoint(2000)
	{
		this->_registerParameter(&breakpoint);
	}

	double getNextSample(void) override {
		if (_inputs.size() > 0) {
			return _update(_inputs.front()->getNextSample());
		}
		return 0;
	}

	ModuleParameter breakpoint;

	//void setBreakpoint(double freq) {
	//	_cutoffFrequency = freq;
	//	_multiplier = (2 * PI * _cutoffFrequency) / _data->sampleRate;
	//}

private:

	double _v0;

	//double _cutoffFrequency;

	//double _multiplier;

	//void _dataSetEvent(void) override {
	//	_multiplier = (2 * PI * _cutoffFrequency) / _data->sampleRate;
	//}

	double _update(double v1) {
		breakpoint.updateValue();
		_v0 += (v1 - _v0) * 2 * PI * breakpoint.getValue() / _data->sampleRate;
		return _v0;
	}
};