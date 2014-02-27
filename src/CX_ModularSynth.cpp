#include "CX_ModularSynth.h"

using namespace CX::Synth;

double CX::Synth::sinc(double x) {
	return sin(x) / x;
}

//Used to connect modules together. l is set as the input for r.
ModuleBase& CX::Synth::operator>> (ModuleBase& l, ModuleBase& r) {
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
	if (!this->_data->initialized) {
		return;
	}

	//Compare both pointers and contents. The pointer comparison is currently useless because the pointers will always be different.
	if ((target->_data != this->_data) || (*target->_data != *this->_data)) {
		//if (!target->_data->initialized) {
		//}

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

///////////
// Adder //
///////////

Adder::Adder(void) :
amount(0)
{
	this->_registerParameter(&amount);
}

double Adder::getNextSample(void) {
	amount.updateValue();
	if (_inputs.size() > 0) {
		return amount.getValue() + _inputs.front()->getNextSample();
	}
	return amount.getValue();
}

//////////////
// Envelope //
//////////////

double Envelope::getNextSample(void) {
	if (_stage > 3) {
		return 0;
	}

	double p;

	switch (_stage) {
	case 0:
		if (_timeSinceLastStage < a && a != 0) {
			p = _timeSinceLastStage / a;
			break;
		} else {
			_timeSinceLastStage = 0;
			_stage++;
		}
	case 1:
		if (_timeSinceLastStage < d && d != 0) {
			p = 1 - (_timeSinceLastStage / d) * (1 - s);
			break;
		} else {
			_timeSinceLastStage = 0;
			_stage++;
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
			_stage++;
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
	_stage = 0;
	_timeSinceLastStage = 0;
}

void Envelope::release(void) {
	_stage = 3;
	_timeSinceLastStage = 0;
	_levelAtRelease = _lastP;
}

void Envelope::_dataSetEvent(void) {
	_timePerSample = 1 / _data->sampleRate;
}

double Mixer::getNextSample(void) {
	double d = 0;
	for (int i = 0; i < _inputs.size(); i++) {
		d += _inputs[i]->getNextSample();
	}
	return d;
}

int Mixer::_maxInputs(void) {
	return 32;
}

////////////////
// Multiplier //
////////////////

Multiplier::Multiplier(void) :
amount(1)
{
	this->_registerParameter(&amount);
}

double Multiplier::getNextSample(void) {
	if (_inputs.front() == nullptr) {
		return 0;
	}
	amount.updateValue();
	return _inputs.front()->getNextSample() * amount.getValue();
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



double Oscillator::whiteNoise(double wp) {
	return CX::Instances::RNG.uniformDeviate(-1, 1);
}


/////////////////////
// RecursiveFilter //
/////////////////////

double RecursiveFilter::getNextSample(void) {
	if (_inputs.size() == 0) {
		return 0;
	}

	double x0 = _inputs.front()->getNextSample();

	double y0 = a0*x0 + a1*x1 + a2*x2 + b1*y1 + b2*y2;

	y2 = y1;
	y1 = y0;
	x2 = x1;
	x1 = x0;
	return y0;
}

void RecursiveFilter::_calcCoefs(void) {
	if (!_data->initialized) {
		return;
	}

	double f_angular = 2 * PI * _breakpoint / _data->sampleRate; //Normalized angular frequency

	if (_filterType == LOW_PASS || _filterType == HIGH_PASS) {
		double x = exp(-f_angular);

		a2 = 0;
		b2 = 0;

		if (_filterType == LOW_PASS) {
			a0 = 1 - x;
			a1 = 0;
			b1 = x;
		} else if (_filterType == HIGH_PASS) {
			a0 = (1 + x) / 2;
			a1 = -(1 + x) / 2;
			b1 = x;
		}

	} else if (_filterType == BAND_PASS || _filterType == NOTCH) {
		double R = 1 - (3 * _bandwidth / _data->sampleRate); //Bandwidth is normalized
		double K = (1 - 2 * R*cos(f_angular) + (R*R)) / (2 - 2 * cos(f_angular));

		b1 = 2 * R * cos(f_angular);
		b2 = -(R*R);

		if (_filterType == BAND_PASS) {
			a0 = 1 - K;
			a1 = 2 * (K - R) * cos(f_angular);
			a2 = (R*R) - K;
		} else if (_filterType == NOTCH) {
			a0 = K;
			a1 = -2 * K * cos(f_angular);
			a2 = K;
		}
	}
}

//////////////
// Splitter //
//////////////

Splitter::Splitter(void) :
_currentSample(0.0),
_fedOutputs(0)
{}


double Splitter::getNextSample(void) {
	if (_fedOutputs >= _outputs.size()) {
		_currentSample = _inputs.front()->getNextSample();
		_fedOutputs = 0;
	}
	++_fedOutputs;
	return _currentSample;
}

void Splitter::_outputAssignedEvent(ModuleBase* out) {
	_fedOutputs = _outputs.size();
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

///////////////////
// AdditiveSynth //
///////////////////
void AdditiveSynth::_initializeAmplitudes (void) {

	_squareWaveAmplitudes.resize(_harmonics.size());
	_sawWaveAmplitudes.resize(_harmonics.size());
	_triangleWaveAmplitudes.resize(_harmonics.size());

	for (unsigned int i = 0; i < _harmonics.size(); i++) {
		int currentHarmonic = i + 1; //The 1th (first) harmonic is at index 0.
		
		//For odd harmonics...
		if ((currentHarmonic % 2) == 1) {
			_squareWaveAmplitudes[i] = 0.5 / currentHarmonic;
			_sawWaveAmplitudes[i] = 0.5 / currentHarmonic;
			
			//For every other wave, triangle is negative. 1+, 3-, 5+, etc.
			if ( (((currentHarmonic - 1)/2) % 2) == 1 ) {
				_triangleWaveAmplitudes[i] = -0.5/(currentHarmonic*currentHarmonic);
			} else {
				_triangleWaveAmplitudes[i] = 0.5/(currentHarmonic*currentHarmonic);
			}								
			
			//I don't appear to have used these anywhere.
			//evenHarmonicSteadySlopeAmplitudes[i] = 0; //The fundamental amplitude as specified by this is ignored later.
		} else { //For even waves...
			_squareWaveAmplitudes[i] = 0;
			//sawWaveAmplitudes[i] = -1L * (MAXIMUM_AMPLITUDE / currentHarmonic) * TWO_OVER_PI;
			_sawWaveAmplitudes[i] = -0.5 / currentHarmonic;
			_triangleWaveAmplitudes[i] = 0;
			
			//evenHarmonicSteadySlopeAmplitudes[i] = MAXIMUM_AMPLITUDE/(2*currentHarmonic);
		}
	}
}

void AdditiveSynth::_setFundamentalFrequency (double fundamental) {
		
	//generalWaveData.baseFrequency = fundamental;

	wavePos_t firstHarmonicPos = _harmonics[0].waveformPosition;
	
	/*
	if (_harmonicSeries == HS_STANDARD) {
		//When using the normal harmonic series you can do just add the same distance to the previous.
		wavePos_t fundamentalPositionChangePerSample = fundamental / _data->sampleRate;
		wavePos_t nextValue = fundamentalPositionChangePerSample;
		for (unsigned int i = 0; i < _harmonics.size(); ++i) {
			_harmonics[i].positionChangePerSample = nextValue;
			nextValue += fundamentalPositionChangePerSample;
					
			//Restart all waves at 0 so they are in phase. It would be smarter to make all harmonic positions a 
			//function of the fundamental harmonic position at time of function call.
			_harmonics[i].waveformPosition = 0;
		}
	} else */
	{
		double normalizedFrequency = fundamental / _data->sampleRate;
	
		for (unsigned int i = 0; i < _harmonics.size(); ++i) {
			_harmonics[i].positionChangePerSample = normalizedFrequency * _relativeFrequenciesOfHarmonics[i];

			//_harmonics[i].waveformPosition = firstHarmonicPos * _relativeFrequenciesOfHarmonics[i];
			_harmonics[i].waveformPosition = 0;
		}
	}
}

double AdditiveSynth::_update(void) {
	double rval = 0;

	for (unsigned int i = 0; i < _harmonics.size(); i++) {
		_harmonics[i].waveformPosition += _harmonics[i].positionChangePerSample;
		if (_harmonics[i].waveformPosition >= 1) {
			_harmonics[i].waveformPosition = fmod(_harmonics[i].waveformPosition, 1);
		}

		rval += Oscillator::sine(_harmonics[i].waveformPosition) * _harmonics[i].amplitude;
	}
	return rval;
}


/*
\param type The type of harmonic series to generate. Can be either HS_MULTIPLE or HS_SEMITONE.
\param controlParameter If type == HS_MULTIPLE, the frequency for harmonic i will be i * controlParameter, where the fundamental gives the value 1 for i.
If type == HS_SEMITONE, the frequency for harmonic i will be pow(2, (i - 1) * controlParameter/12), where the fundamental gives the value 1 for i.

\note If type == HS_MULTIPLE and controlParameter == 1, then the standard harmonic series will be generated.
*/
void AdditiveSynth::setHarmonicSeries(HarmonicSeriesType type, double controlParameter) {
	_harmonicSeriesControlParameter = controlParameter;
	_harmonicSeriesType = type;
	_calculateRelativeFrequenciesOfHarmonics();
}

//The user function takes an integer representing the harmonic number, where the fundamental has the value 1 and returns
//the frequency that should be used for that harmonic.
void AdditiveSynth::setHarmonicSeries (std::function<double(unsigned int)> userFunction) {
	_harmonicSeriesType = HS_USER_FUNCTION;
	_harmonicSeriesUserFunction = userFunction;
	_calculateRelativeFrequenciesOfHarmonics();
}

void AdditiveSynth::_calculateRelativeFrequenciesOfHarmonics (void) {
	_relativeFrequenciesOfHarmonics.resize(_harmonics.size());

	if (_harmonicSeriesType == HS_MULTIPLE) {
		for (unsigned int i = 0; i < _harmonics.size(); i++) {
			_relativeFrequenciesOfHarmonics[i] = (i + 1) * _harmonicSeriesControlParameter;
		}

	} else if (_harmonicSeriesType == HS_SEMITONE) {
		for (unsigned int i = 0; i < _harmonics.size(); i++) {
			_relativeFrequenciesOfHarmonics[i] = pow(2.0, i * _harmonicSeriesControlParameter/12);
		}
	} else if (_harmonicSeriesType == HS_USER_FUNCTION) {
		for (unsigned int i = 0; i < _harmonics.size(); i++) {
			_relativeFrequenciesOfHarmonics[i] = _harmonicSeriesUserFunction(i + 1);
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