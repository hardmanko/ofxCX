#include "CX_ModularSynth.h"

ModuleBase& operator>>(ModuleBase& l, ModuleBase& r) {
	r._assignInput(&l);
	l._assignOutput(&r);
	return r;
}




double Envelope::getNextSample(void) {
	if (stage > 3) {
		return 0;
	}

	double val = _inputs.front()->getNextSample();

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

	//cout << p << endl;

	//val *= p;

	_timeSinceLastStage += (1 / _data->sampleRate);

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


////////////////
// Oscillator //
////////////////

Oscillator::Oscillator(void) :
	frequency(0),
	_waveformPos(0)
{
	setGeneratorFunction(Oscillator::sine);
}

double Oscillator::getNextSample(void) {
	double addAmount = frequency / _data->sampleRate;

	_waveformPos += addAmount;
	if (_waveformPos >= 1) {
		_waveformPos = fmod(_waveformPos, 1);
	}

	return _generatorFunction(_waveformPos);
}

void Oscillator::setGeneratorFunction(std::function<double(double)> f) {
	_generatorFunction = f;
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