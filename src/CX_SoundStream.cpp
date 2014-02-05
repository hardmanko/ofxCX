#include "CX_SoundStream.h"

using namespace CX;
using namespace CX::Instances;

CX_SoundStream::CX_SoundStream (void) :
	_rtAudio(NULL),
	_lastSwapTime(0),
	_lastSampleNumber(0),
	_sampleNumberAtLastCheck(0)
{}

CX_SoundStream::~CX_SoundStream (void) {
	close();
	delete _rtAudio; //If a previous instance was allocated, delete it.
}

/*! Opens the sound stream with the specified configuration. If there was an error during configuration,
messages will be logged.
\return True if configuration appeared to be successful, false otherwise. */
bool CX_SoundStream::open (CX_SoundStreamConfiguration_t &config) {
	if (_rtAudio != NULL) {
		close();
		//delete _rtAudio; //If a previous instance was allocated, delete it. Done in close.
	}

	try {
		_rtAudio = new RtAudio( config.api );
	} catch (RtError err) {
		Log.error("CX_SoundStream") << err.getMessage();
		_rtAudio = NULL;
		return false;
	}

	_rtAudio->showWarnings(true);

	RtAudio::StreamParameters inputParameters;
	if (config.inputChannels > 0) {
		if (config.inputDeviceId < 0) {
			config.inputDeviceId = _rtAudio->getDefaultInputDevice();
		}
		inputParameters.deviceId = config.inputDeviceId;
		inputParameters.nChannels = config.inputChannels;		
	}

	RtAudio::StreamParameters outputParameters;
	if (config.outputChannels > 0) {
		if (config.outputDeviceId < 0) {
			config.outputDeviceId = _rtAudio->getDefaultOutputDevice();
		}
		outputParameters.deviceId = config.outputDeviceId;
		outputParameters.nChannels = config.outputChannels;
	}

	config.bufferSize = ofNextPow2(config.bufferSize);

	//Try to pick a sample rate >= the requested sample rate. If that is impossible, pick the next smallest sample rate.
	vector<RtAudio::DeviceInfo> devices = getDeviceList(config.api);
	unsigned int closestGreaterSampleRate = numeric_limits<unsigned int>::max();
	unsigned int closestLesserSampleRate = 0;

	for (int i = 0; i < devices.at( config.outputDeviceId ).sampleRates.size(); i++) {
		unsigned int thisSampleRate = devices.at( config.outputDeviceId ).sampleRates[i];
		if (thisSampleRate == config.sampleRate) {
			closestGreaterSampleRate = numeric_limits<unsigned int>::max();
			closestLesserSampleRate = 0;
			break;
		}
		if (thisSampleRate > config.sampleRate && thisSampleRate < closestGreaterSampleRate) {
			closestGreaterSampleRate = thisSampleRate;
		}
		if (thisSampleRate < config.sampleRate && thisSampleRate > closestLesserSampleRate) {
			closestLesserSampleRate = thisSampleRate;
		}
	}

	//If not at max, the desired sample rate must not have been possible and there was a greater sample rate available, so pick that sample rate.
	if (closestGreaterSampleRate != numeric_limits<unsigned int>::max()) {
		Log.warning("CX_SoundStream") << "Desired sample rate (" << config.sampleRate << ") not available. " << closestGreaterSampleRate << " chosen instead.";
		config.sampleRate = closestGreaterSampleRate;
	} else if (closestLesserSampleRate != 0) {
		Log.warning("CX_SoundStream") << "Desired sample rate (" << config.sampleRate << ") not available. " << closestLesserSampleRate << " chosen instead.";
		config.sampleRate = closestLesserSampleRate;
	}

	try {
		_rtAudio->openStream( 
			((config.outputChannels > 0) ? &outputParameters : NULL), 
			((config.inputChannels > 0) ? &inputParameters : NULL),
			RTAUDIO_FLOAT32,
			config.sampleRate,
			&config.bufferSize,
			CX_SoundStream::_rtAudioCallback,
			this,
			&config.streamOptions,
			NULL
		);

		config.sampleRate = _rtAudio->getStreamSampleRate(); //Check that the desired sample rate was used.

	} catch (RtError err) {
		Log.error("CX_SoundStream") << err.getMessage();
		return false;
	}

	_config = config; //Store the updated settings.
	return true;

}

/*! Starts the sound stream. The stream must already be open().
\return False if the stream was not started, true if the stream was started or if it was already running. */
bool CX_SoundStream::start (void) {

	if(_rtAudio == NULL) {
		Log.error("CX_SoundStream") << "start: Stream not started because instance pointer was NULL. Have you remembered to call open()?";
		return false;
	}

	if (!_rtAudio->isStreamOpen()) {
		Log.error("CX_SoundStream") << "start: Stream not started because the stream was not open. Have you remembered to call open()?";
		return false;
	}

	if (_rtAudio->isStreamRunning()) {
		Log.warning("CX_SoundStream") << "start: Stream was already running.";
		return true;
	}

	try {
		_rtAudio->startStream();
	} catch (RtError &err) {
		Log.error("CX_SoundStream") << err.getMessage();
		return false;
	}

	_lastSampleNumber = 0;
	_lastSwapTime = Clock.getTime();
	_sampleNumberAtLastCheck = 0;

	return true;
}


bool CX_SoundStream::stop (void) {
	if(_rtAudio == NULL) {
		return false;
	}
	
	try {
		if (_rtAudio->isStreamRunning()) {
    		_rtAudio->stopStream();
		}
  	} catch (RtError &err) {
   		Log.error("CX_SoundStream") << err.getMessage();
		return false;
 	}
	return true;
}


bool CX_SoundStream::close (void) {
	if(_rtAudio == NULL) {
		return false;
	}

	bool rval = true;
	
	try {
		if(_rtAudio->isStreamOpen()) {
    		_rtAudio->closeStream();
		}
  	} catch (RtError &err) {
   		Log.error("CX_SoundStream") << err.getMessage();
		rval = false;
 	}

	delete _rtAudio;
	_rtAudio = NULL;
	return rval;
}

CX_Micros CX_SoundStream::getStreamLatency (void) {
	long latencySamples = _rtAudio->getStreamLatency();
	CX_Micros latency = (CX_Micros)(((CX_Micros)latencySamples * 1000000.0) / _rtAudio->getStreamSampleRate());
	return latency;
}

/*! This function checks to see if the audio buffers have been swapped since the last time
this function was called.
\return True if at least one audio buffer has been swapped out, false if no buffers have been swapped. */
bool CX_SoundStream::hasSwappedSinceLastCheck (void) {
	if (_sampleNumberAtLastCheck != _lastSampleNumber) {
		_sampleNumberAtLastCheck = _lastSampleNumber;
		return true;
	}
	return false;
}

/*! Estimate the time at which the next buffer swap will occur.
\return The estimated time of next swap. This value can be compared with the result of CX::Instances::Clock.getTime(). */
CX_Micros CX_SoundStream::estimateNextSwapTime (void) {
	CX_Micros bufferSwapInterval = (_config.bufferSize * 1000000)/_config.sampleRate;
	return _lastSwapTime + bufferSwapInterval;
}

std::vector<RtAudio::Api> CX_SoundStream::getCompiledApis (void) {
	vector<RtAudio::Api> rval;
	RtAudio::getCompiledApi( rval );
	return rval;
}

std::vector<std::string> CX_SoundStream::convertApisToStrings (vector<RtAudio::Api> apis) {
	//vector<RtAudio::Api> apis;
	//RtAudio::getCompiledApi( apis );

	vector<string> rval;

	for (int i = 0; i < apis.size(); i++) {
		rval.push_back( convertApiToString(apis.at(i)) );
	}

	return rval;
}

std::string CX_SoundStream::convertApiToString (RtAudio::Api api) {
	switch (api) {
	case RtAudio::Api::UNSPECIFIED:
		return "UNSPECIFIED";
	case RtAudio::Api::LINUX_ALSA:
		return "LINUX_ALSA";
	case RtAudio::Api::LINUX_PULSE:
		return "LINUX_PULSE";
	case RtAudio::Api::LINUX_OSS:
		return "LINUX_OSS";
	case RtAudio::Api::UNIX_JACK:
		return "UNIX_JACK";
	case RtAudio::Api::MACOSX_CORE:
		return "MACOSX_CORE";
	case RtAudio::Api::WINDOWS_ASIO:
		return "WINDOWS_ASIO";
	case RtAudio::Api::WINDOWS_DS:
		return "WINDOWS_DS";
	case RtAudio::Api::RTAUDIO_DUMMY:
		return "RTAUDIO_DUMMY";
	};
	return "NULL";
}

std::string CX_SoundStream::convertApisToString (vector<RtAudio::Api> apis) {
	vector<string> sApis = convertApisToStrings(apis);
	string rval = "Available APIs:";
	for (string s : sApis) {
		rval.append( "\r\n" );
		rval.append( s );
	}
	return rval;
}

std::vector<std::string> CX_SoundStream::supportedFormatsToString (RtAudioFormat formats) {
	vector<string> rval;

	for (int i = 0; i < sizeof(RtAudioFormat) * 8; i++) {
		switch (formats & (1 << i)) {
		case RTAUDIO_SINT8:
			rval.push_back( "SINT8" ); break;
		case RTAUDIO_SINT16:
			rval.push_back( "SINT16" ); break;
		case RTAUDIO_SINT24:
			rval.push_back( "SINT24" ); break;
		case RTAUDIO_SINT32:
			rval.push_back( "SINT32" ); break;
		case RTAUDIO_FLOAT32:
			rval.push_back( "FLOAT32" ); break;
		case RTAUDIO_FLOAT64:
			rval.push_back( "FLOAT64" ); break;
		//default:
		};
	}
	return rval;
}

std::vector<RtAudio::DeviceInfo> CX_SoundStream::getDeviceList (RtAudio::Api api) {
	RtAudio *tempRt;
	vector<RtAudio::DeviceInfo> devices;

	try {
		tempRt = new RtAudio(api);
	} catch (RtError err) {
		Log.error("CX_SoundStream") << "Exception while getting device list: " << err.getMessage();
		return devices;
	}

	unsigned int deviceCount = tempRt->getDeviceCount();
	for (unsigned int i = 0; i < deviceCount; i++) {
		try {
			devices.push_back( tempRt->getDeviceInfo(i) );
		} catch (RtError err) {
			Log.error("CX_SoundStream") << "Exception while getting device " << i << ": " << err.getMessage();
			return devices;
		}
	}

	return devices;
}


std::string CX_SoundStream::listDevices (RtAudio::Api api) {
	vector<RtAudio::DeviceInfo> devices = getDeviceList(api);

	stringstream rval;
	rval << "Available devices for " << convertApiToString(api) << " API:" << endl;

	for (unsigned int i = 0; i < devices.size(); i++) {

		RtAudio::DeviceInfo &dev = devices.at(i);

		if (dev.probed == false) {
			rval << "Device " << i << " not successfully probed." << endl;
		} else {

			rval << endl << "---------------------------------------" << endl;

			rval << "Name: " << dev.name << endl;
			rval << "Supported sample rates: ";
			for (int i = 0; i < dev.sampleRates.size(); i++) {
				rval << dev.sampleRates.at(i) << "  ";
			}
			rval << endl;

			rval << "Is default input/output: " << (dev.isDefaultInput ? "True" : "False") << 
				"/" << (dev.isDefaultOutput ? "True" : "False") << endl;

			rval << "Input/output/duplex channels: " << dev.inputChannels << "/" << 
				dev.outputChannels << "/" << dev.duplexChannels << endl;

			vector<string> formats = supportedFormatsToString(dev.nativeFormats);
			rval << "Supported formats: ";
			for (int i = 0; i < formats.size(); i++) {
				rval << formats.at(i) << "  ";
			}
			rval << endl;
			rval << "---------------------------------------" << endl;

		}

	}
	return rval.str();
}

int CX_SoundStream::_rtAudioCallbackHandler (void *outputBuffer, void *inputBuffer, unsigned int bufferSize, double streamTime, RtAudioStreamStatus status) {

	_lastSwapTime = CX::Instances::Clock.getTime();

	if (status != 0) {
		Log.error("CX_SoundStream") << "Buffer underflow/overflow detected.";
	}

	//I don't think that I really need to check for this error. I will check for a while and if it never happens, I might remove the check.
	if (_config.bufferSize != bufferSize) {
		Log.error("CX_SoundStream") << "The configuration's buffer size does not agree with the callback's buffer size. The stream is broken.";	
	}

	if (_config.inputChannels > 0) {
		CX_SSInputCallback_t callbackData;
		callbackData.inputBuffer = (float*)inputBuffer;
		callbackData.bufferSize = bufferSize;

		//Does this data need to be passed on to the listener?
		if (status & RTAUDIO_INPUT_OVERFLOW) {
			callbackData.bufferOverflow = true;
		}

		ofNotifyEvent( this->inputCallbackEvent, callbackData );
	}

	if (_config.outputChannels > 0) {

		//Set the output to 0 so that if the event listener does nothing, this passes silence. This is really wasteful.
		memset(outputBuffer, 0, bufferSize * _config.outputChannels * sizeof(float));

		CX_SSOutputCallback_t callbackData;
		callbackData.outputBuffer = (float*)outputBuffer;
		callbackData.bufferSize = bufferSize;

		//Does this data need to be passed on to the listener?
		if (status & RTAUDIO_OUTPUT_UNDERFLOW) {
			callbackData.bufferUnderflow = true;
		}

		ofNotifyEvent(this->outputCallbackEvent, callbackData);
	}

	_lastSampleNumber += bufferSize;

	return 0; //Return 0 to keep the stream going.
}

int CX_SoundStream::_rtAudioCallback (void *outputBuffer, void *inputBuffer, unsigned int bufferSize, double streamTime, RtAudioStreamStatus status, void *data) {
	return ((CX_SoundStream*)data)->_rtAudioCallbackHandler(outputBuffer, inputBuffer, bufferSize, streamTime, status);
}