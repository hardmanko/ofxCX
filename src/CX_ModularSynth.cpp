#include "CX_ModularSynth.h"

using namespace CX::Synth;

double CX::Synth::sinc(double x) {
	return sin(x) / x;
}

double CX::Synth::relativeFrequency(double f, double semitoneDifference) {
	return f * pow(2.0, semitoneDifference / 12);
}

/*! This operator is used to connect modules together. l is set as the input for r. */
ModuleBase& CX::Synth::operator>> (ModuleBase& l, ModuleBase& r) {
	r._assignInput(&l);
	l._assignOutput(&r);
	return r;
}

////////////////
// ModuleBase //
////////////////

/*! This is a reciprocal operation: This module's input is removed and in's output to this module
is removed. */
void ModuleBase::disconnectInput(ModuleBase* in) {
	auto input = std::find(_inputs.begin(), _inputs.end(), in);
	if (input != _inputs.end()) {
		ModuleBase* inputModule = *input;
		_inputs.erase(input);
		inputModule->disconnectOutput(this);
	}
}

void ModuleBase::disconnectOutput(ModuleBase* out) {
	auto output = std::find(_outputs.begin(), _outputs.end(), out);
	if (output != _outputs.end()) {
		ModuleBase* outputModule = *output;
		_outputs.erase(output);
		outputModule->disconnectInput(this);
	}
}

void ModuleBase::_assignInput(ModuleBase* in) {
	if (_maxInputs() == 0) {
		return;
	}

	if (std::find(_inputs.begin(), _inputs.end(), in) == _inputs.end()) { //If it is not in the vector, try to add it.

		if (_inputs.size() == _maxInputs()) { //If the vector is full, pop off an element before adding the new one.
			disconnectInput(_inputs.back());
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
			disconnectOutput(_outputs.back());
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


///////////////////
// AdditiveSynth //
///////////////////

double AdditiveSynth::getNextSample(void) {
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

void AdditiveSynth::setFundamentalFrequency(double f) {
	_fundamental = f;

	_recalculateWaveformPositions();
}

/*! This function sets the amplitudes of the harmonics based on the chosen type. The resulting waveform
will only be correct if the harmonic series is the standard harmonic series (see setStandardHarmonicSeries()).
\param type The type of wave calculate amplitudes for.
*/
void AdditiveSynth::setAmplitudes(HarmonicAmplitudeType type) {
	vector<AdditiveSynth::amplitude_t> amps = calculateAmplitudes(type, _harmonics.size());

	for (unsigned int i = 0; i < _harmonics.size(); i++) {
		_harmonics[i].amplitude = amps[i];
	}
}

/*! This function sets the amplitudes of the harmonics based on a mixture of the chosen types. The resulting waveform
will only be correct if the harmonic series is the standard harmonic series (see setStandardHarmonicSeries()).
This is a convenient way to morph between waveforms.
*/
void AdditiveSynth::setAmplitudes(HarmonicAmplitudeType t1, HarmonicAmplitudeType t2, double mixture) {
	vector<AdditiveSynth::amplitude_t> amps1 = calculateAmplitudes(t1, _harmonics.size());
	vector<AdditiveSynth::amplitude_t> amps2 = calculateAmplitudes(t2, _harmonics.size());

	mixture = CX::Util::clamp<double>(mixture, 0, 1);

	for (unsigned int i = 0; i < _harmonics.size(); i++) {
		_harmonics[i].amplitude = (amps1[i] * mixture) + (amps2[i] * (1 - mixture));
	}
}

/*! This function sets the amplitudes of the harmonics to arbitrary values as specified in `amps`.
\param amps The amplitudes of the harmonics. If this vector does not contain as many values as
there are harmonics, the unspecified amplitudes will be set to 0.
*/
void AdditiveSynth::setAmplitudes(std::vector<amplitude_t> amps) {
	while (amps.size() < _harmonics.size()) {
		amps.push_back(0.0);
	}

	for (unsigned int i = 0; i < _harmonics.size(); i++) {
		_harmonics[i].amplitude = amps[i];
	}
}

std::vector<AdditiveSynth::amplitude_t> AdditiveSynth::calculateAmplitudes(HarmonicAmplitudeType type, unsigned int count) {
	std::vector<amplitude_t> rval(count, 0.0);

	if (type == AdditiveSynth::HarmonicAmplitudeType::SAW) {
		for (unsigned int i = 0; i < count; i++) {
			rval[i] = 2 / (PI * (i + 1));
			if ((i % 2) == 1) { //Is even-numbered harmonic
				rval[i] *= -1;
			}
		}
	} else if (type == AdditiveSynth::HarmonicAmplitudeType::SQUARE) {
		for (unsigned int i = 0; i < count; i++) {
			if ((i % 2) == 0) { //Is odd-numbered harmonic
				//rval[i] = 2 / (PI * (i + 1));
				rval[i] = 4 / (PI * (i + 1));
			}
			//Do nothing for even harmonics: they remain at 0.
		}
	} else if (type == AdditiveSynth::HarmonicAmplitudeType::TRIANGLE) {
		for (unsigned int i = 0; i < count; i++) {
			if ((i % 2) == 0) { //Is odd-numbered harmonic
				rval[i] = 8 / ((PI * PI) * pow(i + 1, 2));
				if (((i / 2) % 2) == 1) {
					rval[i] *= -1;
				}
			}
			//Do nothing for even harmonics: they remain at 0.
		}
	} else if (type == AdditiveSynth::HarmonicAmplitudeType::SINE) {
		rval[0] = 1;
	}

	return rval;
}

/*! This function removes all harmonics that have an amplitude that is less than or equal to a tolerance 
times the amplitude of the frequency with the greatest absolute amplitude. 

The result of this pruning is that the synthesizer will be more computationally efficient but provide a possibly worse 
approximation of the desired waveform.

\param tol `tol` is interpreted differently depending on its value. If `tol` is greater than or equal to 0, it is treated
as a proportion of the amplitude of the frequency with the greatest amplitude. If `tol` is less than 0, it is treated as
the difference in decibels between the frequency with the greatest amplitude and the tolerance.

\note Because harmonics with an amplitude equal to the tolerance times an amplitude, setting `tol` to 0 will remove 
harmonics with 0 amplitude, but no others. */
void AdditiveSynth::pruneLowAmplitudeHarmonics(double tol) {

	double maxAmplitude = 0;
	for (unsigned int i = 0; i < _harmonics.size(); i++) {
		if (abs(_harmonics[i].amplitude) > maxAmplitude) {
			maxAmplitude = abs(_harmonics[i].amplitude);
		}
	}

	if (tol < 0) {
		tol = sqrt(pow(10.0f, tol / 10.0f));
	}

	double cutoffAmplitude = maxAmplitude * tol;

	for (unsigned int i = 0; i < _harmonics.size(); i++) {
		if (abs(_harmonics[i].amplitude) < cutoffAmplitude) {
			_harmonics.erase(_harmonics.begin() + i);
			i--;
		}
	}
}


void AdditiveSynth::_recalculateWaveformPositions(void) {

	//generalWaveData.baseFrequency = fundamental;

	wavePos_t firstHarmonicPos = _harmonics[0].waveformPosition;

	double normalizedFrequency = _fundamental / _data->sampleRate;

	for (unsigned int i = 0; i < _harmonics.size(); ++i) {
		_harmonics[i].positionChangePerSample = normalizedFrequency * _relativeFrequenciesOfHarmonics[i];

		_harmonics[i].waveformPosition = firstHarmonicPos * _relativeFrequenciesOfHarmonics[i];
		//_harmonics[i].waveformPosition = 0;
	}
}

void AdditiveSynth::_dataSetEvent(void) {
	_recalculateWaveformPositions();
}

/*! The standard harmonic series begins with the fundamental frequency f1 and each seccuessive
harmonic has a frequency equal to f1 * n, where n is the harmonic number for the harmonic.
This is the natural harmonic series, one that occurs, e.g., in a vibrating string.
*/
void AdditiveSynth::setStandardHarmonicSeries(unsigned int harmonicCount) {
	this->setHarmonicSeries(harmonicCount, HarmonicSeriesType::HS_MULTIPLE, 1.0);
}

/*!
\param type The type of harmonic series to generate. Can be either HS_MULTIPLE or HS_SEMITONE. For HS_MULTIPLE, each 
harmonic's frequency will be some multiple of the fundamental frequency, depending on the harmonic number and 
controlParameter. For HS_SEMITONE, each harmonic's frequency will be some number of semitones above the previous frequency,
based on controlParameter (specifying the number of semitones).
\param controlParameter If type == HS_MULTIPLE, the frequency for harmonic i will be i * controlParameter, where the 
fundamental gives the value 1 for i. If type == HS_SEMITONE, the frequency for harmonic i will be 
`pow(2, (i - 1) * controlParameter/12)`, where the fundamental gives the value 1 for i.

\note If `type == HS_MULTIPLE` and `controlParameter == 1`, then the standard harmonic series will be generated.
\note If `type == HS_SEMITONE`, `controlParameter` does not need to be an integer.
*/
void AdditiveSynth::setHarmonicSeries(unsigned int harmonicCount, HarmonicSeriesType type, double controlParameter) {
	_harmonics.resize(harmonicCount);
	_harmonicSeriesControlParameter = controlParameter;
	_harmonicSeriesType = type;
	_calculateRelativeFrequenciesOfHarmonics();

	_recalculateWaveformPositions();
}

/*! This function calculates the harmonic series from a function supplied by the user.
\param harmonicCount The number of harmonics to generate.
\param userFunction The user function takes an integer representing the harmonic number, where the fundamental has the value 1, and returns
the frequency that should be used for that harmonic.
*/
void AdditiveSynth::setHarmonicSeries(unsigned int harmonicCount, std::function<double(unsigned int)> userFunction) {
	_harmonics.resize(harmonicCount);
	_harmonicSeriesType = HS_USER_FUNCTION;
	_harmonicSeriesUserFunction = userFunction;
	_calculateRelativeFrequenciesOfHarmonics();

	_recalculateWaveformPositions();
}

void AdditiveSynth::_calculateRelativeFrequenciesOfHarmonics(void) {
	_relativeFrequenciesOfHarmonics.resize(_harmonics.size());

	if (_harmonicSeriesType == HS_MULTIPLE) {
		for (unsigned int i = 0; i < _harmonics.size(); i++) {
			_relativeFrequenciesOfHarmonics[i] = (i + 1) * _harmonicSeriesControlParameter;
		}
	} else if (_harmonicSeriesType == HS_SEMITONE) {
		for (unsigned int i = 0; i < _harmonics.size(); i++) {
			_relativeFrequenciesOfHarmonics[i] = pow(2.0, i * _harmonicSeriesControlParameter / 12);
		}
	} else if (_harmonicSeriesType == HS_USER_FUNCTION) {
		for (unsigned int i = 0; i < _harmonics.size(); i++) {
			_relativeFrequenciesOfHarmonics[i] = _harmonicSeriesUserFunction(i + 1);
		}
	}
}


/////////////
// Clamper //
/////////////

Clamper::Clamper(void) :
low(-1),
high(1)
{
	this->_registerParameter(&low);
	this->_registerParameter(&high);
}

double Clamper::getNextSample(void) {
	if (_inputs.size() == 0) {
		return 0;
	}

	double temp = this->_inputs.front()->getNextSample();

	high.updateValue();
	low.updateValue();

	temp = std::min(temp, high.getValue());
	temp = std::max(temp, low.getValue());
	return temp;
}


//////////////
// Envelope //
//////////////

double Envelope::getNextSample(void) {
	
	gateInput.updateValue();
	if (gateInput.valueUpdated()) {
		if (gateInput.getValue() == 1.0) {
			this->attack();
		} else if (gateInput.getValue() == 0.0) {
			this->release();
		}
	}
	


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



////////////
// Filter //
////////////

double Filter::getNextSample(void) {
	if (_inputs.size() == 0) {
		return 0;
	}

	cutoff.updateValue();
	bandwidth.updateValue();

	if (cutoff.valueUpdated() || bandwidth.valueUpdated()) {
		_recalculateCoefficients();
	}

	double x0 = _inputs.front()->getNextSample();
	double y0;

	if (_filterType == FilterType::LOW_PASS || _filterType == FilterType::HIGH_PASS) {
		//a2 and b2 are always 0 for LOW_PASS and HIGH_PASS, so they are omitted from the calculation.
		y0 = a0*x0 + a1*x1 + b1*y1;
		y1 = y0;
		x1 = x0;
	} else {
		y0 = a0*x0 + a1*x1 + a2*x2 + b1*y1 + b2*y2;
		y2 = y1;
		y1 = y0;
		x2 = x1;
		x1 = x0;
	}

	return y0;
}

void Filter::_recalculateCoefficients(void) {
	if (!_data->initialized) {
		return;
	}

	double f_angular = 2 * PI * cutoff.getValue() / _data->sampleRate; //Normalized angular frequency

	if (_filterType == FilterType::LOW_PASS || _filterType == FilterType::HIGH_PASS) {
		double x = exp(-f_angular);

		a2 = 0;
		b2 = 0;

		if (_filterType == FilterType::LOW_PASS) {
			a0 = 1 - x;
			a1 = 0;
			b1 = x;
		} else if (_filterType == FilterType::HIGH_PASS) {
			a0 = (1 + x) / 2;
			a1 = -(1 + x) / 2;
			b1 = x;
		}

	} else if (_filterType == FilterType::BAND_PASS || _filterType == FilterType::NOTCH) {
		double R = 1 - (3 * bandwidth.getValue() / _data->sampleRate); //Bandwidth is normalized
		double K = (1 - 2 * R*cos(f_angular) + (R*R)) / (2 - 2 * cos(f_angular));

		b1 = 2 * R * cos(f_angular);
		b2 = -(R*R);

		if (_filterType == FilterType::BAND_PASS) {
			a0 = 1 - K;
			a1 = 2 * (K - R) * cos(f_angular);
			a2 = (R*R) - K;
		} else if (_filterType == FilterType::NOTCH) {
			a0 = K;
			a1 = -2 * K * cos(f_angular);
			a2 = K;
		}
	}
}

///////////
// Mixer //
///////////

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

/*! Sets the `amount` of the multiplier based on gain in decibels.
\param decibels The gain to apply. If greater than 0, `amount` will be greater than 1. If less than 0, `amount` will be less than 1.
After calling this function, `amount` will never be negative.
*/
void Multiplier::setGain(double decibels) {
	amount = sqrt(pow(10.0, decibels / 10.0));
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

/*! It is very easy to make your own waveform generating functions to be used with an Oscillator.
A waveform generating function takes a value that represents the location in the waveform at
the current point in time. These values are in the interval [0,1).

The waveform generating function should return a double representing the amplitude of the
wave at the given waveform position.

To put this all together, a sine wave generator looks like this:
\code{.cpp}
double sineWaveGeneratorFunction(double waveformPosition) {
	return sin(2 * PI * waveformPosition); //The argument for sin() is in radians. 1 cycle is 2*PI radians.
}
\endcode
*/
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
	return CX::Instances::RNG.randomDouble(-1, 1);
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

/*! Set the playback time of the current CX_SoundObject. When playback starts, it will start from this time.
If playback is in progress, playback will skip to the selected time. */
void SoundObjectInput::setTime(CX::CX_Millis t) {
	if (_so != nullptr) {
		unsigned int startSample = _so->getChannelCount() * (unsigned int)(_so->getSampleRate() * t.seconds());
		_currentSample = startSample + _channel;
	} else {
		_currentSample = _channel;
	}

}

double SoundObjectInput::getNextSample(void) {
	if (!this->canPlay()) {
		return 0;
	}
	double value = _so->getRawDataReference().at(_currentSample);
	_currentSample += _so->getChannelCount();
	return value;
}

/*! Checks to see if the sound object that is associated with this SoundObjectInput is able to play. 
It is unable to play if CX_SoundObject::isReadyToPlay() is false or if the whole sound has been played.*/
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
void SoundObjectOutput::sampleData(CX::CX_Millis t) {

	if (_inputs.size() == 0) {
		return; //Warn? Is it better to sample zeros than it is to sample nothing?
	}

	unsigned int samplesToTake = ceil(_data->sampleRate * t.seconds());

	vector<float> tempData(samplesToTake);

	for (unsigned int i = 0; i < samplesToTake; i++) {
		tempData[i] = CX::Util::clamp<float>((float)_inputs.front()->getNextSample(), -1, 1);
	}

	if (so.getTotalSampleCount() == 0) {
		so.setFromVector(tempData, 1, _data->sampleRate);
	} else {
		for (unsigned int i = 0; i < tempData.size(); i++) {
			so.getRawDataReference().push_back(tempData[i]);
		}
	}
}

/////////////////////////////
// StereoSoundObjectOutput //
/////////////////////////////
void StereoSoundObjectOutput::setup(float sampleRate) {
	ModuleControlData_t data;
	data.sampleRate = sampleRate;
	data.initialized = true;
	left.setData(data);
	right.setData(data);
}

void StereoSoundObjectOutput::sampleData(CX::CX_Millis t) {
	unsigned int samplesToTake = ceil(left.getData().sampleRate * t.seconds());

	unsigned int channels = 2; //Stereo

	vector<float> tempData(samplesToTake * channels);

	for (unsigned int i = 0; i < samplesToTake; i++) {
		tempData[(i * channels) + 0] = CX::Util::clamp<float>((float)left.getNextSample(), -1, 1);
		tempData[(i * channels) + 1] = CX::Util::clamp<float>((float)right.getNextSample(), -1, 1);
	}

	if (so.getTotalSampleCount() == 0) {
		so.setFromVector(tempData, channels, left.getData().sampleRate);
	} else {
		for (unsigned int i = 0; i < tempData.size(); i++) {
			so.getRawDataReference().push_back(tempData[i]);
		}
	}
}

////////////////////////
// StereoStreamOutput //
////////////////////////
void StereoStreamOutput::setOuputStream(CX::CX_SoundStream& stream) {
	ofAddListener(stream.outputEvent, this, &StereoStreamOutput::_callback);
	ModuleControlData_t data;
	data.sampleRate = stream.getConfiguration().sampleRate;
	left.setData(data);
	right.setData(data);
}

void StereoStreamOutput::_callback(CX::CX_SoundStream::OutputEventArgs& d) {
	for (unsigned int sample = 0; sample < d.bufferSize; sample++) {

		d.outputBuffer[(sample * d.outputChannels) + 0] = CX::Util::clamp<float>(right.getNextSample(), -1, 1);

		d.outputBuffer[(sample * d.outputChannels) + 1] = CX::Util::clamp<float>(left.getNextSample(), -1, 1);

		//for (int ch = 2; ch < d.outputChannels; ch++) {
		//	d.outputBuffer[(sample * d.outputChannels) + ch] = value;
		//}
	}
}


//////////////////
// StreamOutput //
//////////////////
void StreamOutput::setOuputStream(CX::CX_SoundStream& stream) {
	ofAddListener(stream.outputEvent, this, &StreamOutput::_callback);
	ModuleControlData_t data;
	data.sampleRate = stream.getConfiguration().sampleRate;
	this->setData(data);
}

void StreamOutput::_callback(CX::CX_SoundStream::OutputEventArgs& d) {

	if (_inputs.size() == 0) {
		return;
	}

	for (unsigned int sample = 0; sample < d.bufferSize; sample++) {
		float value = CX::Util::clamp<float>(_inputs.front()->getNextSample(), -1, 1);
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
		ofAddListener(stream.outputEvent, this, &Noisemaker::_callback);
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

	void _callback(CX_SoundStream::OutputEventArgs& d) {

		float addAmount = frequency / d.instance->getConfiguration().sampleRate;

		for (unsigned int sample = 0; sample < d.bufferSize; sample++) {
			_waveformPos += addAmount;
			if (_waveformPos >= 1) {
				_waveformPos = fmod(_waveformPos, 1);
			}
			double value = CX::Util::clamp<float>(_generatorFunction(_waveformPos) * maxAmplitude, -1, 1);
			for (int ch = 0; ch < d.outputChannels; ch++) {
				d.outputBuffer[(sample * d.outputChannels) + ch] = value;
			}
		}

	}

};
*/