#pragma once

#include "ofEvents.h"
#include "CX_SoundStream.h"
#include "CX_SoundObject.h"

#include "CX_RandomNumberGenerator.h" //For white noise generator

/*! \namespace CX::Synth
This namespace contains a number of classes that can be combined together to form a modular
synth that can be used to generate sound stimuli.

\ingroup sound
*/

namespace CX {
namespace Synth {

	double sinc(double x);

	double relativeFrequency(double f, double semitoneDifference);

	struct ModuleControlData_t {
		ModuleControlData_t(void) :
			initialized(false),
			sampleRate(666)
		{}

		ModuleControlData_t(float sampleRate_) :
			initialized(true),
			sampleRate(sampleRate_)
		{}

		bool initialized;
		float sampleRate;

		bool operator==(const ModuleControlData_t& right) {
			return ((this->initialized == right.initialized) && (this->sampleRate == right.sampleRate));
		}

		bool operator!=(const ModuleControlData_t& right) {
			return !(this->operator==(right));
		}
	};

	class ModuleParameter;

	/*! All modules of the modular synth inherit from this class. */
	class ModuleBase {
	public:

		ModuleBase(void) {
			_data = new ModuleControlData_t;
		}

		~ModuleBase(void) {
			delete _data;
		}

		/*! This function should be overloaded for any derived class that produces values (outputs do not 
		produce values, they produce sound via sound hardware). */
		virtual double getNextSample(void) {
			return 0;
		}

		/*! This function sets the data needed by this module in order to function properly. Many modules need this data,
		specifically the sample rate that the synth using. If several modules are connected together, you will only need
		to set the data for one module and the change will propagate to the other connected modules automatically.

		This function does not usually need to be called driectly by the user. If an appropriate input or output is 
		connected, the data will be set from that module. 
		
		\param d The data to set.
		*/
		void setData(ModuleControlData_t d) {
			*_data = d;
			_data->initialized = true;
			this->_dataSet(nullptr);
		}

		ModuleControlData_t getData(void) {
			return *_data;
		}

		
		void disconnectInput(ModuleBase* in);
		void disconnectOutput(ModuleBase* out);

	protected:

		vector<ModuleBase*> _inputs;
		vector<ModuleBase*> _outputs;
		vector<ModuleParameter*> _parameters;
		//std::shared_ptr<ModuleControlData_t> _data;
		ModuleControlData_t *_data;

		/*! This operator is used to connect modules together. `l` is set as the input for `r`. 
		\code{.cpp}
		Oscillator osc;
		StreamOutput out;
		osc >> out; //Connect osc as the input for out.
		\endcode
		*/
		friend ModuleBase& operator>>(ModuleBase& l, ModuleBase& r);

		virtual void _dataSetEvent(void) { return; }

		void _dataSet(ModuleBase* caller);
		void _setDataIfNotSet(ModuleBase* target);
		void _registerParameter(ModuleParameter* p);


		virtual void _assignInput(ModuleBase* in);
		virtual void _assignOutput(ModuleBase* out);

		virtual int _maxInputs(void) { return 1; };
		virtual int _maxOutputs(void) { return 1; };

		virtual void _inputAssignedEvent(ModuleBase* in) {};
		virtual void _outputAssignedEvent(ModuleBase* out) {};
	};


	class ModuleParameter {
	public:

		ModuleParameter(void) :
			_value(0),
			_updated(true),
			_input(nullptr),
			_owner(nullptr)
		{}

		ModuleParameter(double d) :
			_value(d),
			_updated(true),
			_input(nullptr),
			_owner(nullptr)
		{}

		/*! Returns true if the value has been updated. */
		void updateValue(void) {
			if (_input != nullptr) { //If there is no input connected, just keep the same value.
				double temp = _input->getNextSample();
				if (temp != _value) {
					_value = temp;
					_updated = true;
				}
			}
		}

		bool valueUpdated(void) {
			if (_updated) {
				_updated = false;
				return true;
			}
			return false;
		}

		double& getValue(void) {
			return _value;
		}

		operator double(void) {
			return _value;
		}

		ModuleParameter& operator=(double d) {
			_value = d;
			_updated = true;
			_input = nullptr; //Disconnect the input
			return *this;
		}

		/*! This operator connects a module to the module parameter. It is not possible to connect a module
		parameter as an input for anything. They are dead ends.
		
		\code{.cpp}
		Oscillator osc;
		Envelope fenv;
		Adder add;
		add.amount = 500;
		fenv >> add >> osc.frequency; //Connect the envelope as the input for the frequency of the oscillator.
		//with an offset of 500 Hz.
		\endcode
		*/
		friend void operator>>(ModuleBase& l, ModuleParameter& r) {
			r._input = &l;
		}

	private:
		friend class ModuleBase;

		ModuleBase* _owner;
		ModuleBase* _input; //Parameters have one input and no outputs.

		bool _updated;
		double _value;
	};

	/*! This class is an implementation of an additive synthesizer. Additive synthesizers are essentially an
	inverse fourier transform. You specify at which frequencies you want to have a sine wave and the amplitudes 
	of those waves, and they are combined together into a single waveform.

	The frequencies are refered to as harmonics, due to the fact that typical audio applications of additive
	synths use the standard harmonic series (f(i) = f_fundamental * i). However, setting the harmonics to
	values not found in the standard harmonic series can result in really unusual and interesting sounds.

	The output of the additive synth is not easily bounded between -1 and 1 due to various oddities of
	additive synthesis. For example, although in the limit as the number of harmonics goes to infinity
	square and sawtooth waves made with additive synthesis are bounded between -1 and 1, with smaller
	numbers of harmonics the amplitudes actually overshoot these bounds slightly. Of course, if an unusual
	harmonic series is used with arbitrary amplitudes, it can be hard to know if the output of the synth
	will be within the bounds. A Synth::Multiplier can help deal with this.
	\ingroup modSynth
	*/
	class AdditiveSynth : public ModuleBase {
	public:

		typedef float wavePos_t;
		typedef float amplitude_t;

		enum HarmonicSeriesType {
			HS_MULTIPLE, //Includes the standard harmonic series
			HS_SEMITONE, //Includes all of the strange thirds, fourths, tritones, etc.
			HS_USER_FUNCTION
		};

		enum HarmonicAmplitudeType {
			SINE,
			SQUARE,
			SAW,
			TRIANGLE
		};

		struct Configuration {
			unsigned int desiredHarmonicCount;
			HarmonicSeriesType hsType;
			double harmonicPruningTol;
		};

		void configure(unsigned int harmonicCount, HarmonicSeriesType hs, HarmonicAmplitudeType aType);

		void setFundamentalFrequency(double f);

		void setStandardHarmonicSeries(unsigned int harmonicCount);
		void setHarmonicSeries (unsigned int harmonicCount, HarmonicSeriesType type, double controlParameter);
		void setHarmonicSeries(unsigned int harmonicCount, std::function<double(unsigned int)> userFunction);

		void setAmplitudes (HarmonicAmplitudeType type);
		void setAmplitudes(HarmonicAmplitudeType t1, HarmonicAmplitudeType t2, double mixture);
		void setAmplitudes(std::vector<amplitude_t> amps);
		std::vector<amplitude_t> calculateAmplitudes(HarmonicAmplitudeType type, unsigned int count);
		void pruneLowAmplitudeHarmonics(double tol);

		double getNextSample(void) override;

	private:

		void _configure (std::vector<double> frequencies, std::vector<double> amplitudes);

		double _fundamental;

		struct HarmonicInfo {
			wavePos_t waveformPosition;
			wavePos_t positionChangePerSample;
			amplitude_t amplitude;
		};

		std::vector<HarmonicInfo> _harmonics;

		void _recalculateWaveformPositions(void);

		//Harmonic series stuff
		HarmonicSeriesType _harmonicSeriesType;
		double _harmonicSeriesMultiple;
		double _harmonicSeriesControlParameter;
		std::function<double(unsigned int)> _harmonicSeriesUserFunction;
		vector<float> _relativeFrequenciesOfHarmonics;
		void _calculateRelativeFrequenciesOfHarmonics (void);

		

		void _dataSetEvent(void) override;

	};

	//For testing purposes mostly
	class TrivialGenerator : public ModuleBase {
	public:

		TrivialGenerator(void) :
			value(0),
			step(0)
		{
			this->_registerParameter(&value);
			this->_registerParameter(&step);
		}

		ModuleParameter value;
		ModuleParameter step;

		double getNextSample(void) override {
			value.updateValue();
			value.getValue() += step;
			return value.getValue() - step;
		}

	};

	/*! This class simply takes an input and adds an amount to it. The amount can be negative, in which case this
	class is a subtracter. 
	\ingroup modSynth
	*/
	class Adder : public ModuleBase {
	public:
		Adder(void);
		double getNextSample(void) override;
		ModuleParameter amount;
	};

	/*! This class clamps inputs to be in the interval [`low`, `high`], where `low` and `high` are the members of this class. 
	\ingroup modSynth
	*/
	class Clamper : public ModuleBase {
	public:
		Clamper(void);

		double getNextSample(void) override;

		ModuleParameter low;
		ModuleParameter high;
	};

	/*! This class is an ADSR envelope: http://en.wikipedia.org/wiki/Synthesizer#ADSR_envelope
	Setting the a, d, s, and r parameters works in the standard way. s should be in the interval [0,1].
	a, d, and r are expressed in seconds. 
	Call attack() to start the envelope. Once the attack and decay are finished, the envelope will
	stay at the sustain level until release() is called.
	\ingroup modSynth
	*/
	class Envelope : public ModuleBase {
	public:

		Envelope(void) :
			_stage(4)
		{}

		double getNextSample(void) override;

		void attack(void);
		void release(void);

		double a; //Time
		double d; //Time
		double s; //Level
		double r; //Time

	private:
		int _stage;

		double _lastP;
		double _levelAtRelease;

		double _timePerSample;
		double _timeSinceLastStage;

		void _dataSetEvent(void);
	};

	/*! This class mixes together a number of inputs. It does no mixing in the usual sense of
	setting levels of the inputs. Use Multipliers on the inputs for that. This class simply
	adds together all of the inputs with no amplitude correction, so it is possible for the
	output of the mixer to have very large amplitudes.

	This class is special in that it can have more than one input.

	\ingroup modSynth
	*/
	class Mixer : public ModuleBase {
	public:
		double getNextSample(void) override;
	private:
		int _maxInputs(void) override;
	};

	/*! This class multiplies an input by an `amount`. You can set the amount in terms of decibels
	of gain by using the setGain() function. 
	\ingroup modSynth
	*/
	class Multiplier : public ModuleBase {
	public:
		Multiplier(void);
		double getNextSample(void) override;
		void setGain(double decibels);
		ModuleParameter amount;
	};

	/*! This class splits a signal and sends that signal to multiple outputs. This can be used
	for panning effects, for example.

	This class is special because it allows multiple outputs.

	\code{.cpp}
	Splitter sp;
	Oscillator osc;
	Multiplier m1;
	Multiplier m2;
	StereoStreamOutput out;

	osc >> sp;
	sp >> m1 >> out.left;
	sp >> m2 >> out.right;
	\endcode
	\ingroup modSynth
	*/
	class Splitter : public ModuleBase {
	public:
		Splitter(void);
		double getNextSample(void) override;

	private:
		void _outputAssignedEvent(ModuleBase* out) override;
		int _maxOutputs(void) override { return 32; };

		double _currentSample;
		int _fedOutputs;
	};

	/*! This class allows you to use a CX_SoundObject as the input for the modular synth.
	It is strictly monophonic, so when you associate a CX_SoundObject with this class,
	you must pick one channel of the sound to use.
	\ingroup modSynth
	*/
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

	/*! This class provides one of the simplest ways of generating waveforms.

	\code{.cpp}
	//Configure the oscillator to produce a square wave with a fundamental frequency of 200 Hz.
	Oscillator osc;
	osc.frequency = 200;
	osc.setGeneratorFunction(Oscillator::square);
	\endcode
	\ingroup modSynth
	*/
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
		float _sampleRate; //This is a slight optimization. This just needs to refer to a data member, rather than _data->sampleRate.
		double _waveformPos;

		void _dataSetEvent(void);
	};

	/*! This class provides a method of playing the output of a modular synth using a CX_SoundStream.
	\ingroup modSynth
	*/
	class StreamOutput : public ModuleBase {
	public:
		void setOuputStream(CX::CX_SoundStream& stream);

	private:
		void _callback(CX::CX_SoundStream::OutputEventArgs& d);
		int _maxOutputs(void) override { return 0; };
	};

	
	class GenericOutput : public ModuleBase {
	public:
		double getNextSample(void) override {
			if (_inputs.size() == 0) {
				return 0;
			}
			return _inputs.front()->getNextSample();
		}
	private:
		int _maxOutputs(void) override { return 0; };
	};

	/*! This class is much like StreamOutput except in stereo.
	\ingroup modSynth
	*/
	class StereoStreamOutput {
	public:
		void setOuputStream(CX::CX_SoundStream& stream);

		GenericOutput left;
		GenericOutput right;

	private:
		void _callback(CX::CX_SoundStream::OutputEventArgs& d);
	};

	/*! This class provides a method of capturing the output of a modular synth and storing it in a CX_SoundObject
	for later use.
	\ingroup modSynth
	*/
	class SoundObjectOutput : public ModuleBase {
	public:
		void setup(float sampleRate);
		void sampleData(double t);

		CX::CX_SoundObject so;
	};

	/*! This class provides a method of capturing the output of a modular synth and storing it in a CX_SoundObject
	for later use.

	This captures stereo audio by taking the output of different streams of data into either the `left` or `right`
	modules that this class has. See the example code.

	\code{.cpp}
	StereoSoundObjectOutput sout;
	sout.setup(44100);

	Splitter sp;
	Oscillator osc;
	Multiplier leftM;
	Multiplier rightM;

	leftM.amount = .1;
	rightM.amount = .01;

	osc >> sp;
	sp >> leftM >> sout.left;
	sp >> rightM >> sout.right;

	sout.sampleData(2); //Sample 2 seconds worth of data on both channels.
	\endcode

	\ingroup modSynth */
	class StereoSoundObjectOutput {
	public:
		void setup(float sampleRate);
		void sampleData(double t);

		GenericOutput left;
		GenericOutput right;
	
		CX::CX_SoundObject so;

	};

	/*! This class is a start at implementing a Finite Impulse Response (http://en.wikipedia.org/wiki/Finite_impulse_response)
	filter. You can use it as a basic low-pass or high-pass	filter, or, if you supply your own coefficients, which cause the
	filter to do filtering in whatever way you want. See the "signal" package for R for a method of constructing your own coefficients.
	\ingroup modSynth
	*/
	class FIRFilter : public ModuleBase{
	public:

		enum FilterType {
			LOW_PASS,
			HIGH_PASS,
			USER_DEFINED
		};

		enum WindowType {
			RECTANGULAR,
			HANNING,
			BLACKMAN
		};

		FIRFilter(void) :
			_filterType(LOW_PASS),
			_coefCount(-1)
		{}

		void setup(FilterType filterType, unsigned int coefficientCount) {
			if ((coefficientCount % 2) == 0) {
				coefficientCount++; //Must be odd in this implementation
			}

			_coefCount = coefficientCount;

			_inputSamples.assign(_coefCount, 0); //Fill with zeroes so that we never have to worry about not having enough input data.
		}

		//You can supply your own coefficients. See the fir1 and fir2 functions from the 
		//"signal" package for R for a good way to design your own filter.
		void setup(std::vector<double> coefficients) {
			_filterType = USER_DEFINED;

			_coefficients = coefficients;

			_coefCount = coefficients.size();
			_inputSamples.assign(_coefCount, 0);
		}

		void setCutoff(double cutoff) {
			if (_filterType == USER_DEFINED) {
				return;
			}

			double omega = PI * cutoff / (_data->sampleRate / 2); //This is pi * normalized frequency, where normalization is based on the nyquist frequency.

			_coefficients.clear();

			for (int i = -_coefCount / 2; i <= _coefCount / 2; i++) {
				_coefficients.push_back(_calcH(i, omega));
			}

			if (_filterType == FilterType::HIGH_PASS) {
				for (int i = -_coefCount / 2; i <= _coefCount / 2; i++) {
					_coefficients[i] *= pow(-1, i);
				}
			}

			if (_windowType == WindowType::HANNING) {
				for (int i = 0; i < _coefCount; i++) {
					_coefficients[i] *= 0.5*(1 - cos(2*PI*i / (_coefCount - 1)));
				}
			} else if (_windowType == WindowType::BLACKMAN) {
				for (int i = 0; i < _coefCount; i++) {
					double a0 = 7938 / 18608;
					double a1 = 9240 / 18608;
					double a2 = 1430 / 18608;

					_coefficients[i] *= a0 - a1 * cos(2*PI*i / (_coefCount - 1)) + a2*cos(4*PI*i / (_coefCount - 1));
				}
			}

			//else if (_filterType == FilterType::BAND_PASS) {
			//	for (int i = -_coefCount / 2; i <= _coefCount / 2; i++) {
			//		_convolutionCoefficients[i] *= 2 * cos(i * PI / 2);
			//	}
			//}
		}

		double getNextSample(void) {
			//Because _inputSamples is set up to have _coefCount elements, you just always pop off an element to start.
			_inputSamples.pop_front();
			_inputSamples.push_back(_inputs.front()->getNextSample());

			double y_n = 0;

			for (int i = 0; i < _coefCount; i++) {
				y_n += _inputSamples[i] * _coefficients[i];
			}

			return y_n;
		}

	private:

		FilterType _filterType;
		WindowType _windowType;

		int _coefCount;

		vector<double> _coefficients;
		deque<double> _inputSamples;

		double _calcH(int n, double omega) {
			if (n == 0) {
				return omega / PI;
			}
			return omega / PI * sinc(n*omega);
		}
	};

	/*!	This class implements some simple IIR filters. They may not be stable at all frequencies. 
	They are computationally very efficient. They are not highly configurable. They may be chained
	for sharper frequency response.
	This class is based on http://www.dspguide.com/ch19.htm. 
	\ingroup modSynth */
	class RecursiveFilter : public ModuleBase {
	public:

		/*! The type of filter to use. */
		enum FilterType {
			LOW_PASS,
			HIGH_PASS,
			BAND_PASS,
			NOTCH
		};

		RecursiveFilter(void) :
			_filterType(LOW_PASS),
			x1(0),
			x2(0),
			y1(0),
			y2(0),
			frequency(1000),
			bandwidth(50)
		{
			this->_registerParameter(&frequency);
			this->_registerParameter(&bandwidth);
		}

		void setup(RecursiveFilter::FilterType type) {
			_filterType = type;
			_recalculateCoefficients();
		}

		double getNextSample(void) override;

		//void setBreakpoint(double freq) {
		//	_breakpoint = freq;
		//	_calcCoefs();
		//}

		//void setBandwidth(double bw);

		/*! The cutoff frequency of the filter. */
		ModuleParameter frequency;

		/*! Only used for BAND_PASS and NOTCH filters. Sets the width (in frequency domain) of the stop or pass band
		at which the amplitude is equal to sin(PI/4) (i.e. .707). So, for example, if you wanted the frequencies
		100 Hz above and below the breakpoint to be at .707 of the maximum amplitude, set bw to 100.
		Of course, past those frequencies the attenuation continues.
		Larger values result in a less pointy band.	*/
		ModuleParameter bandwidth;

	private:

		RecursiveFilter::FilterType _filterType;

		void _dataSetEvent(void) override {
			_recalculateCoefficients();
		}

		void _recalculateCoefficients(void);

		double a0;
		double a1;
		double a2;
		double b1;
		double b2;

		double x1;
		double x2;
		double y1;
		double y2;
	};

	/*! This class emulates an analog RC low-pass filter (http://en.wikipedia.org/wiki/Low-pass_filter#Electronic_low-pass_filters).
	Setting the breakpoint affects the frequency at which the filter starts to have an effect.
	\ingroup modSynth */
	class RCFilter : public ModuleBase {
	public:

		RCFilter(void) :
			_v0(0),
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

	private:

		double _v0;

		double _update(double v1) {
			breakpoint.updateValue();
			_v0 += (v1 - _v0) * 2 * PI * breakpoint.getValue() / _data->sampleRate;
			return _v0;
		}
	};

} //namespace Synth
} //namespace CX
