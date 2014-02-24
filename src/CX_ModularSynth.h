#include "ofEvents.h"
#include "CX_SoundStream.h"



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
		_dataSet();
	}

	void setData(ModuleControlData_t d) {
		*data = d;
		data->initialized = true;
		_dataSet();
	}

	ModuleBase *input;

	std::shared_ptr<ModuleControlData_t> data;

private:

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

class Envelope : public ModuleBase {
public:

	Envelope(void) :
		stage(4)
	{}

	double getNextSample(void) {
		if (stage > 3) {
			return 0;
		}

		double val = input->getNextSample();

		double p;

		switch (stage) {
		case 0:
			if (_timeSinceLastStage < a) {
				p = _timeSinceLastStage / a;
				break;
			} else {
				_timeSinceLastStage = 0;
				stage++;
			}
		case 1:
			if (_timeSinceLastStage < d) {
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
			if (_timeSinceLastStage < r) {
				p = (1 - _timeSinceLastStage / r) * s;
				break;
			} else {
				_timeSinceLastStage = 0;
				stage++;
				p = 0;
			}
		}

		//cout << p << endl;

		val *= p;

		_timeSinceLastStage += _timePerSample;

		return val;
	}

	void gate(void) {
		stage = 0;
		_timeSinceLastStage = 0;
	}

	void release(void) {
		stage = 3;
		_timeSinceLastStage = 0;
	}

	int stage;

	double a; //Time
	double d; //Time
	double s; //Level
	double r; //Time

private:

	double _timePerSample;
	double _timeSinceLastStage;

	void _dataSetEvent(void) {
		_timePerSample = 1 / data->sampleRate;
	}

};

class Oscillator : public ModuleBase {
public:

	Oscillator(void) :
		frequency(0),
		_sampleRate(0),
		_waveformPos(0)
	{
		setGeneratorFunction(Oscillator::sine);
	}

	double getNextSample(void) override {
		double addAmount = frequency / _sampleRate;

		_waveformPos += addAmount;
		if (_waveformPos >= 1) {
			_waveformPos = fmod(_waveformPos, 1);
		}

		return _generatorFunction(_waveformPos);
	}

	void setGeneratorFunction(std::function<double(double)> f) {
		_generatorFunction = f;
	}

	double frequency;

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
		//x = pow(2.718282, -2 * PI * freq);

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

class SoundOut : public ModuleBase {
public:

	void setOuputStream(CX::CX_SoundStream& stream) {
		ofAddListener(stream.outputCallbackEvent, this, &SoundOut::_callback);
		ModuleControlData_t data;
		data.sampleRate = stream.getConfiguration().sampleRate;
		this->setData(data);
	}

private:
	void _callback(CX::CX_SSOutputCallback_t& d) {

		if (input == nullptr) {
			return;
		}

		for (unsigned int sample = 0; sample < d.bufferSize; sample++) {
			float value = ofClamp(input->getNextSample(), -1, 1);
			//cout << value << endl;
			for (int ch = 0; ch < d.outputChannels; ch++) {
				d.outputBuffer[(sample * d.outputChannels) + ch] = value;
			}
		}

	}
};






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