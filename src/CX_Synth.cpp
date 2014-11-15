#include "CX_Synth.h"

namespace CX {
namespace Synth {

/*! The sinc function, defined as `sin(x)/x`. */
double sinc(double x) {
	return sin(x) / x;
}

/*! This function returns the frequency that is `semitoneDifference` semitones from `f`.
\param f The starting frequency.
\param semitoneDifference The difference (positive or negative) from `f` to the desired output frequency.
\return The final frequency. */
double relativeFrequency(double f, double semitoneDifference) {
	return f * pow(2.0, semitoneDifference / 12);
}

/*! This operator is used to connect modules together. `l` is set as the input for `r`.
\code{.cpp}
Oscillator osc;
StreamOutput out;
osc >> out; //Connect osc as the input for out.
\endcode
*/
ModuleBase& operator>> (ModuleBase& l, ModuleBase& r) {
	r._assignInput(&l);
	l._assignOutput(&r);
	return r;
}

/*! This operator connects a module to the module parameter. It is not possible to connect a module
parameter as an input for anything: They are dead ends.

\code{.cpp}
using namespace CX::Synth;
Oscillator osc;
Envelope fenv;
Adder add;
add.amount = 500;
fenv >> add >> osc.frequency; //Connect the envelope as the input for the frequency of the oscillator with an offset of 500 Hz.
\endcode
*/
void operator>>(ModuleBase& l, ModuleParameter& r) {
	r._input = &l;
	r._owner->_setDataIfNotSet(&l);
}

////////////////
// ModuleBase //
////////////////

ModuleBase::ModuleBase(void) {
	_data = new ModuleControlData_t;
}

ModuleBase::~ModuleBase(void) {
	delete _data;
}

/*! This function should be overloaded for any derived class that can be used as the input for another module. 
\return The value of the next sample from the module. */
double ModuleBase::getNextSample(void) {
	return 0;
}

/*! This function sets the data needed by this module in order to function properly. Many modules need this data,
specifically the sample rate that the synth using. If several modules are connected together, you will only need
to set the data for one module and the change will propagate to the other connected modules automatically.

This function does not usually need to be called driectly by the user. If an appropriate input or output is
connected, the data will be set from that module. However, there are some cases where a pattern of reconnecting
previously used modules may result in inappropriate sample rates being set. For that reason, if you are having
a problem with seeing the correct sample rate after reconnecting some modules, try manually calling setData().

\param d The data to set.
*/
void ModuleBase::setData(ModuleControlData_t d) {
	*_data = d;
	_data->initialized = true;
	this->_dataSet(nullptr);
}

/*! Gets the data used by the module. */
ModuleControlData_t ModuleBase::getData(void) {
	return *_data;
}

/*! Disconnect a module that is an input to this module. This is a reciprocal operation: 
This module's input is disconnected and `in`'s output to this module is disconnected. */
void ModuleBase::disconnectInput(ModuleBase* in) {
	auto input = std::find(_inputs.begin(), _inputs.end(), in);
	if (input != _inputs.end()) {
		ModuleBase* inputModule = *input;
		_inputs.erase(input);
		inputModule->disconnectOutput(this);
	}
}

/*! Disconnect a module that this module outputs to. This is a reciprocal operation: 
This module's output is disconnected and `out`'s input from this module is disconnected. */
void ModuleBase::disconnectOutput(ModuleBase* out) {
	auto output = std::find(_outputs.begin(), _outputs.end(), out);
	if (output != _outputs.end()) {
		ModuleBase* outputModule = *output;
		_outputs.erase(output);
		outputModule->disconnectInput(this);
	}
}

/*! \brief Fully disconnect a module from all inputs and outputs. */
void ModuleBase::disconnect(void) {
	while (_inputs.size() > 0) {
		this->disconnectInput(_inputs[0]);
	}

	while (_outputs.size() > 0) {
		this->disconnectOutput(_outputs[0]);
	}
}

/*! Assigns a module as an input to this module. This is not a reciprocal operation. 
\param in The module to assign as an input. */
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

/*! Assigns a module as an output from this module. This is not a reciprocal operation. 
\param out The module to asssign as an output. */
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

/*! This function is called on a module after the data for that module has been set.
\param caller The module that set the data for this module. */
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

/*! This function is a sort of callback that is called whenever _dataSet is called. Within this 
function, you should do things for your module that depend on the new data values. You should 
not attempt to propagate the data values to inputs, outputs, or parameters: that is all done 
for you. */
void ModuleBase::_dataSetEvent(void) {
	return;
}

/*! This function sets the data for a target module if the data for that module has not been set.
\param target The target module to set the data for. */
void ModuleBase::_setDataIfNotSet(ModuleBase* target) {

	//If this is not initialized, don't set data for the target.
	if (!this->_data->initialized) {
		return;
	}

	if (*target->_data != *this->_data) {
		*target->_data = *this->_data;
		target->_dataSet(this);
	}

}

/*! If you are using a CX::Synth::ModuleParameter in your module, you must register that ModuleParameter
during construction (or setup) of the module using this function.

\code{.cpp}
class MyModule : public ModuleBase {
public:

	MyModule(void) {
		this->_registerParameter(&myParam);
		//...
	}

	ModuleParameter myParam;
	//...
};
\endcode
*/
void ModuleBase::_registerParameter(ModuleParameter* p) {
	if (std::find(_parameters.begin(), _parameters.end(), p) == _parameters.end()) {
		_parameters.push_back(p);
		p->_owner = this;
	}
}

/*! Returns the maximum number of inputs to this module. */
unsigned int ModuleBase::_maxInputs(void) { 
	return 1; 
}

/*! Returns the maximum numer of outputs from this module. */
unsigned int ModuleBase::_maxOutputs(void) { 
	return 1; 
}

/*! Does nothing by default, but can be overridden by inheriting classes. */
void ModuleBase::_inputAssignedEvent(ModuleBase* in) { 
	return; 
} 

/*! Does nothing by default, but can be overridden by inheriting classes. */
void ModuleBase::_outputAssignedEvent(ModuleBase* out) { 
	return; 
}

/////////////////////
// ModuleParameter //
/////////////////////

/*! \brief Construct a ModuleParameter with no value. */
ModuleParameter::ModuleParameter(void) :
_owner(nullptr),
_input(nullptr),
_updated(true),
_value(0)
{}

/*! \brief Construct a ModuleParameter with the given start value. */
ModuleParameter::ModuleParameter(double d) :
_owner(nullptr),
_input(nullptr),
_updated(true),
_value(d)
{}

/*! Update the value of the module parameter. This gets the next sample from the module
that is the input for the ModuleParameter, if any. */
void ModuleParameter::updateValue(void) {
	if (_input != nullptr) { //If there is no input connected, just keep the same value.
		double temp = _input->getNextSample();
		if (temp != _value) {
			_value = temp;
			_updated = true;
		}
	}
}

/*! Returns `true` if the value of the ModuleParameter has been updated since the last time
this function was called. This should be called right after updateValue() or with `checkForUpdates = true`.
Updates to the value resulting from assignment of a new value with `operator=()` count as updates
to the value.

If you don't care whether the value has been updated before using it, don't call this function.
Instead, just use updateValue() and getValue().

\param checkForUpdates Check for updates before determining whether the value has been updated.
\return `true` if the value has been updated since the last check.
*/
bool ModuleParameter::valueUpdated(bool checkForUpdates) {
	if (checkForUpdates) {
		updateValue();
	}

	if (_updated) {
		_updated = false;
		return true;
	}
	return false;
}

/*! Gets the current value of the parameter. */
double& ModuleParameter::getValue(void) {
	return _value;
}

/*! \brief Implicitly converts the parameter to `double`. */
ModuleParameter::operator double(void) {
	return _value;
}

/*! \brief Assign a value to the parameter. */
ModuleParameter& ModuleParameter::operator=(double d) {
	_value = d;
	_updated = true;
	_input = nullptr; //Disconnect the input
	return *this;
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

AdditiveSynth::AdditiveSynth(void) :
	fundamental(1)
{
	this->_registerParameter(&fundamental);
}

double AdditiveSynth::getNextSample(void) {
	double rval = 0;

	if (fundamental.valueUpdated(true)) {
		_recalculateWaveformPositions();
	}

	for (unsigned int i = 0; i < _harmonics.size(); i++) {
		double waveformPos = _harmonics[i].waveformPosition + _harmonics[i].positionChangePerSample;

		waveformPos = fmod(waveformPos, 1);

		rval += Oscillator::sine(waveformPos) * _harmonics[i].amplitude;
		_harmonics[i].waveformPosition = waveformPos;
	}

	return rval;
}

/*! This function sets the amplitudes of the harmonics based on the chosen type. The resulting waveform
will only be correct if the harmonic series is the standard harmonic series (see setStandardHarmonicSeries()).
\param a The type of wave calculate amplitudes for.
*/
void AdditiveSynth::setAmplitudes(AmplitudePresets a) {
	vector<AdditiveSynth::amplitude_t> amps = calculateAmplitudes(a, _harmonics.size());

	for (unsigned int i = 0; i < _harmonics.size(); i++) {
		_harmonics[i].amplitude = amps[i];
	}
}

/*! This function sets the amplitudes of the harmonics based on a mixture of the chosen types. The resulting waveform
will only be correct if the harmonic series is the standard harmonic series (see setStandardHarmonicSeries()).
This is a convenient way to morph between waveforms.
\param a1 The first preset.
\param a2 The second present.
\param mixture Should be in the interval [0,1]. The proportion of `a1` that will be used, with the remainder (`1 - mixture`) used from `a2`.
*/
void AdditiveSynth::setAmplitudes(AmplitudePresets a1, AmplitudePresets a2, double mixture) {
	vector<AdditiveSynth::amplitude_t> amps1 = calculateAmplitudes(a1, _harmonics.size());
	vector<AdditiveSynth::amplitude_t> amps2 = calculateAmplitudes(a2, _harmonics.size());

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

/*! This is a specialty function that only works when the standard harmonic series is being used. If so,
it calculates the amplitudes needed for the hamonics so as to produce the specified waveform type.
\param a The type of waveform that should be output from the additive synth.
\param count The number of harmonics.
\return A vector of amplitudes.
*/
std::vector<AdditiveSynth::amplitude_t> AdditiveSynth::calculateAmplitudes(AmplitudePresets a, unsigned int count) {
	std::vector<amplitude_t> rval(count, 0.0);

	if (a == AmplitudePresets::SAW) {
		for (unsigned int i = 0; i < count; i++) {
			rval[i] = 2 / (PI * (i + 1));
			if ((i % 2) == 1) { //Is even-numbered harmonic
				rval[i] *= -1;
			}
		}
	} else if (a == AmplitudePresets::SQUARE) {
		for (unsigned int i = 0; i < count; i++) {
			if ((i % 2) == 0) { //Is odd-numbered harmonic
				//rval[i] = 2 / (PI * (i + 1));
				rval[i] = 4 / (PI * (i + 1));
			}
			//Do nothing for even harmonics: they remain at 0.
		}
	} else if (a == AmplitudePresets::TRIANGLE) {
		for (unsigned int i = 0; i < count; i++) {
			if ((i % 2) == 0) { //Is odd-numbered harmonic
				rval[i] = 8 / ((PI * PI) * pow(i + 1, 2));
				if (((i / 2) % 2) == 1) {
					rval[i] *= -1;
				}
			}
			//Do nothing for even harmonics: they remain at 0.
		}
	} else if (a == AmplitudePresets::SINE) {
		rval[0] = 1;
	}

	return rval;
}

/*! This function removes all harmonics that have an amplitude that is less than or equal to a tolerance
times the amplitude of the harmonic with the greatest absolute amplitude. The result of this pruning is that 
the synthesizer will be more computationally efficient but provide a less precise approximation of the desired 
waveform.

\param tol `tol` is interpreted differently depending on its value. If `tol` is greater than or equal to 0, it is treated
as a proportion of the amplitude of the frequency with the greatest amplitude. If `tol` is less than 0, it is treated as
the difference in decibels between the frequency with the greatest amplitude and the tolerance cutoff point.

\note Because only harmonics with an amplitude less than or equal to the tolerance times an amplitude are pruned,
setting `tol` to 0 will remove harmonics with 0 amplitude, but no others.
*/
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

/*! The standard harmonic series begins with the fundamental frequency f1 and each seccuessive
harmonic has a frequency equal to f1 * n, where n is the harmonic number for the harmonic.
This is the natural harmonic series, one that occurs, e.g., in a vibrating string.
*/
void AdditiveSynth::setStandardHarmonicSeries(unsigned int harmonicCount) {
	this->setHarmonicSeries(harmonicCount, HarmonicSeriesType::MULTIPLE, 1.0);
}

/*! Set the harmonic series for the AdditiveSynth.
\param harmonicCount The number of harmonics to use.
\param type The type of harmonic series to generate. Can be either HS_MULTIPLE or HS_SEMITONE. For HS_MULTIPLE, each
harmonic's frequency will be some multiple of the fundamental frequency, depending on the harmonic number and
controlParameter. For HS_SEMITONE, each harmonic's frequency will be some number of semitones above the previous frequency,
based on controlParameter (specifying the number of semitones).
\param controlParameter If `type == HS_MULTIPLE`, the frequency for harmonic `i` will be `i * controlParameter`, where the
fundamental gives the value 1 for `i`. If `type == HS_SEMITONE`, the frequency for harmonic `i` will be
`pow(2, (i - 1) * controlParameter/12)`, where the fundamental gives the value 1 for `i`.

\note If `type == HS_MULTIPLE` and `controlParameter == 1`, then the standard harmonic series will be generated.
\note If `type == HS_SEMITONE`, `controlParameter` does not need to be an integer.
*/
void AdditiveSynth::setHarmonicSeries(unsigned int harmonicCount, HarmonicSeriesType type, double controlParameter) {
	_harmonics.resize(harmonicCount);

	if (type == HarmonicSeriesType::MULTIPLE) {
		for (unsigned int i = 0; i < _harmonics.size(); i++) {
			_harmonics[i].relativeFrequency = (i + 1) * controlParameter;
		}
	} else if (type == HarmonicSeriesType::SEMITONE) {
		for (unsigned int i = 0; i < _harmonics.size(); i++) {
			_harmonics[i].relativeFrequency = pow(2.0, i * controlParameter / 12);
		}
	}

	_recalculateWaveformPositions();
}

/*! This function applies the harmonic series from a vector of harmonics supplied by the user.

\param harmonicSeries A vector frequencies that create a harmonic series. These values
will be multiplied by the fundamental frequency in order to obtain the final frequency
of each harmonic. The multiplier for the first harmonic is at index 0, so by convention 
you might want to set harmonicSeries[0] equal to 1, so that when the fundamental frequency 
is set with setFundamentalFrequency(), the first harmonic is actually the fundamental 
frequency, but this is not enforced.

\note If `harmonicSeries.size()` is greater than the current number of harmonics, the 
new harmonics will have an amplitude of 0. If `harmonicSeries.size()` is less than the
current number of harmonics, the number of harmonics will be reduced to the size of
`harmonicSeries`.
*/
void AdditiveSynth::setHarmonicSeries(std::vector<frequency_t> harmonicSeries) {
	_harmonics.resize(harmonicSeries.size());

	for (unsigned int i = 0; i < _harmonics.size(); i++) {
		_harmonics[i].relativeFrequency = harmonicSeries[i];
	}

	_recalculateWaveformPositions();
}


void AdditiveSynth::_recalculateWaveformPositions(void) {
	double firstHarmonicPos = _harmonics[0].waveformPosition;

	double normalizedFrequency = fundamental.getValue() / _data->sampleRate;

	for (unsigned int i = 0; i < _harmonics.size(); ++i) {
		double relativeFrequency = _harmonics[i].relativeFrequency;

		_harmonics[i].positionChangePerSample = normalizedFrequency * relativeFrequency;

		_harmonics[i].waveformPosition = firstHarmonicPos * relativeFrequency; //This keeps the harmonics in phase
	}
}

void AdditiveSynth::_dataSetEvent(void) {
	_recalculateWaveformPositions();
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


Envelope::Envelope(void) :
gateInput(0.5),
_stage(4)
{
	this->_registerParameter(&gateInput);
	this->_registerParameter(&a);
	this->_registerParameter(&d);
	this->_registerParameter(&s);
	this->_registerParameter(&r);
}

double Envelope::getNextSample(void) {

	if (gateInput.valueUpdated(true)) {
		if (gateInput.getValue() == 1.0) {
			this->attack();
		} else if (gateInput.getValue() == 0.0) {
			this->release();
		}
	}

	if (_stage > 3) {
		return 0;
	}

	if (a.valueUpdated(true)) {
		_a = a.getValue();
	}
	if (d.valueUpdated(true)) {
		_d = d.getValue();
	}
	if (s.valueUpdated(true)) {
		_s = s.getValue();
	}
	if (r.valueUpdated(true)) {
		_r = r.getValue();
	}

	double p;

	switch (_stage) {
	case 0:
		if ((_timeSinceLastStage < _a) && (_a != 0)) {
			p = _timeSinceLastStage / _a;
			break;
		} else {
			_timeSinceLastStage = 0;
			_stage++;
		}
	case 1:
		if ((_timeSinceLastStage < _d) && (_d != 0)) {
			p = 1 - (_timeSinceLastStage / _d) * (1 - _s);
			break;
		} else {
			_timeSinceLastStage = 0;
			_stage++;
		}
	case 2:
		p = _s;
		break;
	case 3:
		if ((_timeSinceLastStage < _r) && (_r != 0)) {
			p = (1 - _timeSinceLastStage / _r) * _levelAtRelease;
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

/*! \brief Trigger the attack of the Envelope. */
void Envelope::attack(void) {
	_stage = 0;
	_timeSinceLastStage = 0;
}

/*! \brief Trigger the release of the Envelope. */
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

Filter::Filter(void) :
cutoff(1000),
bandwidth(50),
_filterType(FilterType::LOW_PASS),
x1(0),
x2(0),
y1(0),
y2(0)
{
	this->_registerParameter(&cutoff);
	this->_registerParameter(&bandwidth);
}

/*! \brief Set the type of filter to use, from the Filter::FilterType enum. */
void Filter::setType(FilterType type) {
	_filterType = type;
	_recalculateCoefficients();
}

double Filter::getNextSample(void) {
	if (_inputs.size() == 0) {
		return 0;
	}

	if (cutoff.valueUpdated(true) || bandwidth.valueUpdated(true)) {
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
	for (unsigned int i = 0; i < _inputs.size(); i++) {
		d += _inputs[i]->getNextSample();
	}
	return d;
}

unsigned int Mixer::_maxInputs(void) {
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

/*! Convenience constructor.
\param amount_ The amount to multiply the input by.
*/
Multiplier::Multiplier(double amount_) :
amount(amount_)
{
	this->_registerParameter(&amount);
}

double Multiplier::getNextSample(void) {
	if (_inputs.size() == 0) {
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

	_waveformPos = fmod(_waveformPos + addAmount, 1);

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

/*! Produces a sawtooth wave.
\param wp The waveform position to sample, in the interval [0, 1), where 0 is the 
start of the waveform and 1 is the end of the waveform. 
\return A value normalized to the interval [-1, 1] containing the value of the waveform
function at the given waveform position.
*/
double Oscillator::saw(double wp) {
	return (2 * wp) - 1;
}

/*! Produces a sine wave.
\param wp The waveform position to sample, in the interval [0, 1), where 0 is the
start of the waveform and 1 is the end of the waveform. 
\return A value normalized to the interval [-1, 1] containing the value of the waveform
function at the given waveform position.
*/
double Oscillator::sine(double wp) {
	return sin(wp * 2 * PI);
}

/*! Produces a square wave.
\param wp The waveform position to sample, in the interval [0, 1), where 0 is the
start of the waveform and 1 is the end of the waveform. 
\return A value normalized to the interval [-1, 1] containing the value of the waveform
function at the given waveform position.
*/
double Oscillator::square(double wp) {
	if (wp < .5) {
		return 1;
	} else {
		return -1;
	}
}

/*! Produces a triangle wave.
\param wp The waveform position to sample, in the interval [0, 1), where 0 is the
start of the waveform and 1 is the end of the waveform. 
\return A value normalized to the interval [-1, 1] containing the value of the waveform
function at the given waveform position.
*/
double Oscillator::triangle(double wp) {
	if (wp < .5) {
		return ((4 * wp) - 1);
	} else {
		return (3 - (4 * wp));
	}
}

/*! Produces white noise.
\param wp This argument is ignored. 
\return A random value in the interval [-1, 1].
*/
double Oscillator::whiteNoise(double wp) {
	return CX::Instances::RNG.randomDouble(-1, 1);
}

///////////////////
// RingModulator //
///////////////////

double RingModulator::getNextSample(void) {
	if (_inputs.size() == 2) {
		double i1 = _inputs[0]->getNextSample();
		double i2 = _inputs[1]->getNextSample();
		return i1 * i2;
	} else if (_inputs.size() == 1) {
		return _inputs.front()->getNextSample();
	}
	return 0;
}

unsigned int RingModulator::_maxInputs(void) {
	return 2;
}

//////////////
// Splitter //
//////////////

Splitter::Splitter(void) :
_currentSample(0.0),
_fedOutputs(0)
{}

//The way this works is that each output gets the same value from a single
//input, so only once all of the outputs have been fed do we update the
//sample value. If the splitter is feeding, e.g., two different stream outputs, 
//there is the potential for substantial desynchronization. Basically, samples
//from all outputs of the splitter must happen at adjacent times.
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
// SoundBufferInput //
//////////////////////
SoundBufferInput::SoundBufferInput(void) :
    _sb(nullptr),
	_currentSample(0)
{}

/*! This function sets the CX_SoundBuffer from which data will be drawn. Because the SoundBufferInput is monophonic,
you must pick one channel of the CX_SoundBuffer to use.
\param sb The CX_SoundBuffer to use. Because this CX_SoundBuffer is taken as a pointer and is not copied,
you should make sure that `sb` remains in existence and unmodified while the SoundBufferInput is in use.
\param channel The channel of the CX_SoundBuffer to use.
*/
void SoundBufferInput::setSoundBuffer(CX::CX_SoundBuffer *sb, unsigned int channel) {
	_sb = sb;
	_channel = channel;

	_data->sampleRate = _sb->getSampleRate();
	_data->initialized = true;
	_dataSet(nullptr);
}

/*! Set the playback time of the current CX_SoundBuffer. When playback starts, it will start from this time.
If playback is in progress, playback will skip to the selected time. */
void SoundBufferInput::setTime(CX::CX_Millis t) {
	if (_sb != nullptr) {
		unsigned int startSample = _sb->getChannelCount() * (unsigned int)(_sb->getSampleRate() * t.seconds());
		_currentSample = startSample + _channel;
	} else {
		_currentSample = _channel;
	}

}

double SoundBufferInput::getNextSample(void) {
	if (!this->canPlay()) {
		return 0;
	}
	double value = _sb->getRawDataReference().at(_currentSample);
	_currentSample += _sb->getChannelCount();
	return value;
}

/*! Checks to see if the CX_SoundBuffer that is associated with this SoundBufferInput is able to play.
It is unable to play if CX_SoundBuffer::isReadyToPlay() is false or if the whole sound has been played.*/
bool SoundBufferInput::canPlay(void) {
	return (_sb != nullptr) && (_sb->isReadyToPlay()) && (_currentSample < _sb->getTotalSampleCount());
}


///////////////////////
// SoundBufferOutput //
///////////////////////

/*! Configure the output to use a particular sample rate. If this function is not called, the sample rate
of the modular synth may be undefined.
\param sampleRate The sample rate in Hz. */
void SoundBufferOutput::setup(float sampleRate) {
	_data->sampleRate = sampleRate;
	_data->initialized = true;
	_dataSet(nullptr);
}

/*! This function samples `t` milliseconds of data at the sample rate given in setup().
The result is stored in the `sb` member of this class. If `sb` is not empty when this function
is called, the data is appended to `sb`. */
void SoundBufferOutput::sampleData(CX::CX_Millis t) {

	if (_inputs.size() == 0) {
		return; //Warn? Is it better to sample zeros than it is to sample nothing?
	}

	unsigned int samplesToTake = ceil(_data->sampleRate * t.seconds());

	vector<float> tempData(samplesToTake);

	ModuleBase* input = _inputs.front();

	for (unsigned int i = 0; i < samplesToTake; i++) {
		tempData[i] = CX::Util::clamp<float>((float)input->getNextSample(), -1, 1);
	}

	if (sb.getTotalSampleCount() == 0) {
		sb.setFromVector(tempData, 1, _data->sampleRate);
	} else {
		for (unsigned int i = 0; i < tempData.size(); i++) {
			sb.getRawDataReference().push_back(tempData[i]);
		}
	}
}

/////////////////////////////
// StereoSoundBufferOutput //
/////////////////////////////
/*! Configure the output to use a particular sample rate. If this function is not called, the sample rate
of the modular synth may be undefined.
\param sampleRate The sample rate in Hz. */
void StereoSoundBufferOutput::setup(float sampleRate) {
	ModuleControlData_t data;
	data.sampleRate = sampleRate;
	data.initialized = true;
	left.setData(data);
	right.setData(data);
}

/*! This function samples `t` milliseconds of data at the sample rate given in setup().
The result is stored in the `sb` member of this class. If `sb` is not empty when this function
is called, the data is appended to `sb`. */
void StereoSoundBufferOutput::sampleData(CX::CX_Millis t) {
	unsigned int samplesToTake = ceil(left.getData().sampleRate * t.seconds());

	unsigned int channels = 2; //Stereo

	vector<float> tempData(samplesToTake * channels);

	for (unsigned int i = 0; i < samplesToTake; i++) {
		tempData[(i * channels) + 0] = CX::Util::clamp<float>((float)left.getNextSample(), -1, 1);
		tempData[(i * channels) + 1] = CX::Util::clamp<float>((float)right.getNextSample(), -1, 1);
	}

	if (sb.getTotalSampleCount() == 0) {
		sb.setFromVector(tempData, channels, left.getData().sampleRate);
	} else {
		for (unsigned int i = 0; i < tempData.size(); i++) {
			sb.getRawDataReference().push_back(tempData[i]);
		}
	}
}

////////////////////////
// StereoStreamOutput //
////////////////////////
StereoStreamOutput::~StereoStreamOutput(void) {
	_listenForEvents(false);
}

/*! Set up the StereoStreamOutput with the given CX_SoundStream.
\param stream A CX_SoundStream that is configured for stereo output. */
void StereoStreamOutput::setup(CX::CX_SoundStream* stream) {
	_soundStream = stream;
	_listenForEvents(true);

	ModuleControlData_t data;
	data.sampleRate = _soundStream->getConfiguration().sampleRate;
	left.setData(data);
	right.setData(data);
}

void StereoStreamOutput::_callback(CX::CX_SoundStream::OutputEventArgs& d) {
	for (unsigned int sample = 0; sample < d.bufferSize; sample++) {
		unsigned int index = sample * d.outputChannels;
		d.outputBuffer[index + 0] = CX::Util::clamp<float>(right.getNextSample(), -1, 1); //The buffers only use float, so clamp with float.
		d.outputBuffer[index + 1] = CX::Util::clamp<float>(left.getNextSample(), -1, 1);
	}
}

void StereoStreamOutput::_listenForEvents(bool listen) {
	if ((listen == _listeningForEvents) || (_soundStream == nullptr)) {
		return;
	}

	if (listen) {
		ofAddListener(_soundStream->outputEvent, this, &StereoStreamOutput::_callback);
	} else {
		ofRemoveListener(_soundStream->outputEvent, this, &StereoStreamOutput::_callback);
	}
	_listeningForEvents = listen;
}

/////////////////
// StreamInput //
/////////////////
StreamInput::StreamInput(void) :
_maxBufferSize(4096),
_soundStream(nullptr),
_listeningForEvents(false)
{}

StreamInput::~StreamInput(void) {
	_listenForEvents(false);
}

/*! Set up the StreamInput with a CX_SoundStream configured for input. 
\param stream A pointer to the sound stream. */
void StreamInput::setup(CX::CX_SoundStream* stream) {
	if (stream->getConfiguration().inputChannels != 1) {
		CX::Instances::Log.error("StreamInput") << "setInputStream(): The provided stream must be"
			"configured with a single input channel, but it is not.";
	}
	_soundStream = stream;

	_listenForEvents(true);
}

double StreamInput::getNextSample(void) {
	double rval = 0;
	if (_maxBufferSize != 0) {
		while (_buffer.size() > _maxBufferSize) {
			_buffer.pop_front();
		}
	}

	if (_buffer.size() > 0) {
		rval = _buffer.front();
		_buffer.pop_front();
	}
	return rval;
}

/*! \brief Clear the contents of the input buffer. */
void StreamInput::clear(void) {
	_buffer.clear();
}

/*! Set the maximum number of samples that the input buffer can contain. 
\param size The size of the input buffer, in samples. */
void StreamInput::setMaximumBufferSize(unsigned int size) {
	_maxBufferSize = size;
}

void StreamInput::_callback(CX::CX_SoundStream::InputEventArgs& in) {
	for (unsigned int sampleFrame = 0; sampleFrame < in.bufferSize; sampleFrame++) {
		_buffer.push_back(in.inputBuffer[sampleFrame]);
	}
}

void StreamInput::_listenForEvents(bool listen) {
	if ((listen == _listeningForEvents) || (_soundStream == nullptr)) {
		return;
	}

	if (listen) {
		ofAddListener(_soundStream->inputEvent, this, &StreamInput::_callback);
	} else {
		ofRemoveListener(_soundStream->inputEvent, this, &StreamInput::_callback);
	}
	_listeningForEvents = listen;
}

unsigned int StreamInput::_maxInputs(void) {
	return 0;
}


//////////////////
// StreamOutput //
//////////////////

StreamOutput::~StreamOutput(void) {
	_listenForEvents(false);
}

/*! Set up the StereoStreamOutput with the given CX_SoundStream.
\param stream A CX_SoundStream that is configured for output to any number of channels. */
void StreamOutput::setup(CX::CX_SoundStream* stream) {
	_soundStream = stream;
	_listenForEvents(true);

	ModuleControlData_t data;
	data.sampleRate = _soundStream->getConfiguration().sampleRate;
	this->setData(data);
}

void StreamOutput::_callback(CX::CX_SoundStream::OutputEventArgs& d) {
	if (_inputs.size() == 0) {
		return;
	}

	ModuleBase* input = _inputs.front();

	for (unsigned int sample = 0; sample < d.bufferSize; sample++) {
		float value = CX::Util::clamp<float>(input->getNextSample(), -1, 1);
		for (int ch = 0; ch < d.outputChannels; ch++) {
			d.outputBuffer[(sample * d.outputChannels) + ch] = value;
		}
	}
}

void StreamOutput::_listenForEvents(bool listen) {
	if ((listen == _listeningForEvents) || (_soundStream == nullptr)) {
		return;
	}

	if (listen) {
		ofAddListener(_soundStream->outputEvent, this, &StreamOutput::_callback);
	} else {
		ofRemoveListener(_soundStream->outputEvent, this, &StreamOutput::_callback);
	}
	_listeningForEvents = listen;
}

//////////////////////
// TrivialGenerator //
//////////////////////

TrivialGenerator::TrivialGenerator(void) :
value(0),
step(0)
{
	this->_registerParameter(&value);
	this->_registerParameter(&step);
}


double TrivialGenerator::getNextSample(void) {
	value.updateValue();
	value.getValue() += step;
	return value.getValue() - step;
}




///////////////
// FIRFilter //
///////////////

FIRFilter::FIRFilter(void) :
_filterType(FilterType::LOW_PASS),
_coefCount(-1)
{}

/*! Set up the FIRFilter with the given filter type and number of coefficients to use.
\param filterType Should be a type of filter other than FIRFilter::FilterType::FIR_USER_DEFINED. If you want
to define your own filter type, use FIRFilter::setup(std::vector<double>) instead.
\param coefficientCount The number of coefficients sets the length of time, in samples, that the filter will
produce a non-zero output following an impulse. In other words, the filter operates on `coefficientCount`
samples at a time to produce each output sample.
*/
void FIRFilter::setup(FilterType filterType, unsigned int coefficientCount) {
	if (filterType == FIRFilter::FilterType::USER_DEFINED) {
		CX::Instances::Log.error("FIRFilter") << "setup(): FilterType::FIR_USER_DEFINED should not be used explicity."
			"Use FIRFilter::setup(std::vector<double>) if you want to define your own filter type.";
	}
	_filterType = filterType;

	if ((coefficientCount % 2) == 0) {
		coefficientCount++; //Must be odd in this implementation
	}

	_coefCount = coefficientCount;
	_inputSamples.assign(_coefCount, 0); //Fill with zeroes so that we never have to worry about not having enough input data.
}

/*! You can use this function to supply your own filter coefficients, which allows a great
deal of flexibility in the use of the FIRFilter.  See the fir1 and fir2 functions from the
//"signal" package for R for a way to design your own filter. 
\param coefficients The filter coefficients to use.
*/
void FIRFilter::setup(std::vector<double> coefficients) {
	_filterType = FilterType::USER_DEFINED;

	_coefficients = coefficients;

	_coefCount = coefficients.size();
	_inputSamples.assign(_coefCount, 0); //Fill with zeroes so that we never have to worry about not having enough input data.
}

/*! If using either FilterType::LOW_PASS or FilterType::HIGH_PASS, this function allows you to
change the cutoff frequency for the filter. This causes the filter coefficients to be recalculated.
\param cutoff The cutoff frequency, in Hz. */
void FIRFilter::setCutoff(double cutoff) {
	if (_filterType == FilterType::USER_DEFINED) {
		CX::Instances::Log.error("FIRFilter") << "setCutoff() should not be used when the filter type is USER_DEFINED.";
		return;
	}

	double omega = PI * cutoff / (_data->sampleRate / 2); //This is pi * normalized frequency, where normalization is based on the nyquist frequency.

	_coefficients.clear();

	//Set up LPF coefficients
	for (int i = -_coefCount / 2; i <= _coefCount / 2; i++) {
		_coefficients.push_back(_calcH(i, omega));
	}

	if (_filterType == FilterType::HIGH_PASS) {
		for (int i = -_coefCount / 2; i <= _coefCount / 2; i++) {
			_coefficients[i] *= pow(-1, i);
		}
	}


	//Do nothing for rectangular window
	if (_windowType == WindowType::HANNING) {
		for (int i = 0; i < _coefCount; i++) {
			_coefficients[i] *= 0.5*(1 - cos(2 * PI*i / (_coefCount - 1)));
		}
	} else if (_windowType == WindowType::BLACKMAN) {
		for (int i = 0; i < _coefCount; i++) {
			double a0 = 7938 / 18608;
			double a1 = 9240 / 18608;
			double a2 = 1430 / 18608;

			_coefficients[i] *= a0 - a1 * cos(2 * PI*i / (_coefCount - 1)) + a2*cos(4 * PI*i / (_coefCount - 1));
		}
	}
}

double FIRFilter::getNextSample(void) {
	//Because _inputSamples is set up to have _coefCount elements, you just always pop off an element to start.
	_inputSamples.pop_front();
	_inputSamples.push_back(_inputs.front()->getNextSample());

	double y_n = 0;

	for (int i = 0; i < _coefCount; i++) {
		y_n += _inputSamples[i] * _coefficients[i];
	}

	return y_n;
}

double FIRFilter::_calcH(int n, double omega) {
	if (n == 0) {
		return omega / PI;
	}
	return omega / PI * sinc(n*omega);
}

} //namespace Synth
} //namespace CX
