#pragma once

#include "ofEvents.h"
#include "CX_SoundStream.h"
#include "CX_SoundObject.h"

#include "CX_RandomNumberGenerator.h" //For white noise generator

/*! \namespace Synth
This namespace contains a number of classes that can be combined together to form a modular
synth that can be used to generate sound stimuli.
*/

namespace CX {
namespace Synth {

	double sinc(double x);

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
		}
	};

	class ModuleParameter;

	class ModuleBase {
	public:

		ModuleBase(void) {
			_data = new ModuleControlData_t;
		}

		~ModuleBase(void) {
			delete _data;
		}

		//This function should be overloaded for any derived class that produces values (outputs do not 
		//produce values, they produce sound via sound hardware).
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

		vector<ModuleBase*> _inputs;
		vector<ModuleBase*> _outputs;
		vector<ModuleParameter*> _parameters;
		//std::shared_ptr<ModuleControlData_t> _data;
		ModuleControlData_t *_data;

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

		ModuleParameter& operator=(double d) {
			_data = d;
			_input = nullptr; //Disconnect the input?
			return *this;
		}

		friend void operator>>(ModuleBase& l, ModuleParameter& r) {
			r._input = &l;
			//l.setData(r._owner->getData());
		}

	private:
		friend class ModuleBase;

		ModuleBase* _owner;
		ModuleBase* _input; //Parameters have one input and no outputs.

		double _data;
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


	class Adder : public ModuleBase {
	public:
		Adder(void);
		double getNextSample(void) override;
		ModuleParameter amount;
	};


	class Mixer : public ModuleBase {
	public:
		double getNextSample(void) override;
	private:
		int _maxInputs(void) override;
	};


	class Multiplier : public ModuleBase {
	public:
		Multiplier(void);
		double getNextSample(void) override;
		ModuleParameter amount;
	};


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
		void _callback(CX::CX_SSOutputCallback_t& d);
		int _maxOutputs(void) override { return 0; };
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

	/*! This class is a start at implementing a Finite Impulse Response (http://en.wikipedia.org/wiki/Finite_impulse_response)
	filter. You can use it as a basic low-pass or high-pass	filter, or, if you supply your own coefficients, as whatever kind 
	of FIR filter you are able to design. See the "signal" package for R for a method of constructing your own coefficients.
	\ingroup modSynth
	*/
	class FIRFilter : public ModuleBase{
	public:

		enum FilterType {
			LOW_PASS,
			HIGH_PASS,
			USER_DEFINED
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
			y2(0)
		{}

		void setup(RecursiveFilter::FilterType type) {
			_filterType = type;
			_calcCoefs();
		}

		double getNextSample(void) override;

		void setBreakpoint(double freq) {
			_breakpoint = freq;
			_calcCoefs();
		}

		//Only used for BAND_PASS and NOTCH filters. Sets the width (in frequency domain) of the stop or pass band
		//at which the amplitude is equal to sin(PI/4) (i.e. .707). So, for example, if you wanted the frequencies
		//100 Hz above and below the breakpoint to be at .707 of the maximum amplitude, set bw to 100.
		//Of course, past those frequencies the attenuation continues.
		//Larger values result in a less pointy band.
		void setBandwidth(double bw) {
			_bandwidth = bw;
		}

	private:

		RecursiveFilter::FilterType _filterType;

		void _dataSetEvent(void) override {
			_calcCoefs();
		}

		void _calcCoefs(void);

		double _breakpoint;
		double _bandwidth;

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
