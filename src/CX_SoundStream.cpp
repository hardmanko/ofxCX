#include "CX_SoundStream.h"

using namespace CX;
using namespace CX::Instances;

CX_SoundStream::CX_SoundStream (void) :
	_rtAudio(nullptr),
	_lastSwapTime(0),
	_lastSampleNumber(0),
	_sampleNumberAtLastCheck(0)
{}

CX_SoundStream::~CX_SoundStream (void) {
	closeStream();
}

/*! Opens the sound stream with the specified configuration. If there was an error during configuration,
messages will be logged.
\param config The configuration settings that are desired. Some of the configuration options are only suggestions,
so some of the values that are used may differ from the values that are chosen. In those cases, config is updated
based on the actually used settings. You can check the configuration later using getConfiguration().
\return True if configuration appeared to be successful, false otherwise. 
\note Opening the stream does not start it. See \ref start(). */
bool CX_SoundStream::setup (CX_SoundStream::Configuration &config) {
	if (_rtAudio != nullptr) {
		closeStream();
	}

	try {
		_rtAudio = new RtAudio( config.api );
	} catch (RtError err) {
		Log.error("CX_SoundStream") << err.getMessage();
		_rtAudio = nullptr;
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

	int searchDeviceId = (config.outputDeviceId >= 0) ? config.outputDeviceId : config.inputDeviceId;

	for (int i = 0; i < devices.at(searchDeviceId).sampleRates.size(); i++) {
		unsigned int thisSampleRate = devices.at(searchDeviceId).sampleRates[i];
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

	if(_rtAudio == nullptr) {
		Log.error("CX_SoundStream") << "start: Stream not started because instance pointer was NULL. Have you remembered to call open()?";
		return false;
	}

	if (!_rtAudio->isStreamOpen()) {
		Log.error("CX_SoundStream") << "start: Stream not started because the stream was not open. Have you remembered to call open()?";
		return false;
	}

	if (_rtAudio->isStreamRunning()) {
		Log.notice("CX_SoundStream") << "start: Stream was already running.";
		return true;
	}

	try {
		_rtAudio->startStream();
	} catch (RtError &err) {
		Log.error("CX_SoundStream") << err.getMessage();
		return false;
	}

	_lastSampleNumber = 0;
	//_lastSwapTime = Clock.now();
	_sampleNumberAtLastCheck = 0;

	return true;
}

/*! Check whether the sound stream is running. 
\return false if the stream is not open or not running or if RtAudio has not been initialized. Returns true if the stream is running. */
bool CX_SoundStream::isStreamRunning(void) const {
	if (_rtAudio == nullptr) {
		return false;
	}
	return _rtAudio->isStreamRunning();
}

/*! Stop the stream, if is running. If there is an error, a message will be logged.
\return False if there was an error, true otherwise. */
bool CX_SoundStream::stop (void) {
	if(_rtAudio == nullptr) {
		Log.error("CX_SoundStream") << "stop: Stream not stopped because instance pointer was NULL. Have you remembered to call open()?";
		return false;
	}
	
	try {
		if (_rtAudio->isStreamRunning()) {
    		_rtAudio->stopStream();
		} else {
			Log.notice("CX_SoundStream") << "stop: Stream was already stopped.";
		}
  	} catch (RtError &err) {
   		Log.error("CX_SoundStream") << err.getMessage();
		return false;
 	}
	return true;
}

/*! Closes the sound stream.
\return False if an error was encountered while closing the stream, true otherwise. */
bool CX_SoundStream::closeStream(void) {
	if(_rtAudio == nullptr) {
		return false;
	}

	bool rval = true;
	
	try {
		if(_rtAudio->isStreamOpen()) {
    		_rtAudio->closeStream();
		} else {
			Log.notice("CX_SoundStream") << "closeStream: Stream was already closed.";
		}
  	} catch (RtError &err) {
   		Log.error("CX_SoundStream") << err.getMessage();
		rval = false;
 	}

	delete _rtAudio;
	_rtAudio = nullptr;
	return rval;
}

/*! This function gets an estimate of the stream latency. However, it should not be relied on as
it is based on what the sound card driver reports, which is often false. 
\return The stream latency in microseconds. */
CX_Millis CX_SoundStream::getStreamLatency(void) {
	long latencySamples = _rtAudio->getStreamLatency();
	CX_Millis latency = (((long long)latencySamples * 1000000.0) / _rtAudio->getStreamSampleRate());
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
\return The estimated time of next swap. This value can be compared with the result of CX::Instances::Clock.now(). */
CX_Millis CX_SoundStream::estimateNextSwapTime(void) {
	CX_Millis bufferSwapInterval = ((long long)_config.bufferSize * 1000000) / _config.sampleRate;
	return _lastSwapTime + bufferSwapInterval;
}

/*! This function returns a pointer to the RtAudio instance that this CX_SoundStream is using. 
This should not be needed most of the time, but there may be cases in which you need to directly 
access RtAudio. Here is the documentation for RtAudio: https://www.music.mcgill.ca/~gary/rtaudio/
*/
RtAudio* CX_SoundStream::getRtAudioInstance(void) {
	return _rtAudio;
}

/*! Get a vector containing a list of all of the APIs for which the RtAudio driver
has been compiled to use. If the API you want is not available, you might be able
to get it by using a different version of RtAudio. */
std::vector<RtAudio::Api> CX_SoundStream::getCompiledApis (void) {
	vector<RtAudio::Api> rval;
	RtAudio::getCompiledApi( rval );
	return rval;
}

/*! This helper function converts a vector of RtAudio::Api to a vector of strings, using 
convertApiToString() for the conversion.
\param apis A vector of apis to convert to strings.
\return A vector of string names of the apis. */
std::vector<std::string> CX_SoundStream::convertApisToStrings (vector<RtAudio::Api> apis) {
	//vector<RtAudio::Api> apis;
	//RtAudio::getCompiledApi( apis );

	vector<string> rval;

	for (int i = 0; i < apis.size(); i++) {
		rval.push_back( convertApiToString(apis.at(i)) );
	}

	return rval;
}

/*! This helper function converts an RtAudio::Api to a string.
\param api The api to get a string of.
\return A string of the api name. */
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

/*! Converts a string name of an RtAudio API to an RtAudio::Api enum constant. 
\param apiString The name of the API as a string. Should be one of the following, with no surrounding whitespace:
UNSPECIFIED, LINUX_ALSA, LINUX_PULSE, LINUX_OSS, UNIX_JACK, MACOSX_CORE, WINDOWS_ASIO, WINDOWS_DS, RTAUDIO_DUMMY
\return The RtAudio::Api corresponding to the provided string. If the string is not one of the above values, 
RtAudio::Api::UNSPECIFIED is returned.
*/
RtAudio::Api CX_SoundStream::convertStringToApi(std::string apiString) {
	if (apiString == "UNSPECIFIED") {
		return RtAudio::Api::UNSPECIFIED;
	} else if (apiString == "LINUX_ALSA") {
		return RtAudio::Api::LINUX_ALSA;
	} else if (apiString == "LINUX_PULSE") {
		return RtAudio::Api::LINUX_PULSE;
	} else if (apiString == "LINUX_OSS") {
		return RtAudio::Api::LINUX_OSS;
	} else if (apiString == "UNIX_JACK") {
		return RtAudio::Api::UNIX_JACK;
	} else if (apiString == "MACOSX_CORE") {
		return RtAudio::Api::MACOSX_CORE;
	} else if (apiString == "WINDOWS_ASIO") {
		return RtAudio::Api::WINDOWS_ASIO;
	} else if (apiString == "WINDOWS_DS") {
		return RtAudio::Api::WINDOWS_DS;
	} else if (apiString == "RTAUDIO_DUMMY") {
		return RtAudio::Api::RTAUDIO_DUMMY;
	}

	return RtAudio::Api::UNSPECIFIED; //This is a bit of an error code...
}

/*! This helper function converts a vector of RtAudio::Api to a string, with
the specified delimiter between API names.
\param apis The vector of RtAudio::Api to convert to string.
\param delim The delimiter between elements of the string.
\return A string containing the names of the APIs.
*/
std::string CX_SoundStream::convertApisToString (vector<RtAudio::Api> apis, std::string delim) {
	vector<string> sApis = convertApisToStrings(apis);
	string rval;

	for (int i = 0; i < sApis.size(); i++) {
		rval.append( sApis[i] );
		if (i < sApis.size() - 1) {
			rval.append( delim );
		}
	}

	return rval;
}

/*! Converts a bitmask of audio formats to a vector of strings. 
\param formats The bitmask of audio formats.
\return A vector of strings, one string for each bit set in formats for 
which there is a corresponding valid audio format that RtAudio supports.
*/
std::vector<std::string> CX_SoundStream::formatsToStrings (RtAudioFormat formats) {
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

/*! Converts a bitmask of audio formats to a string, with each format delimited by `delim`.
\param formats The bitmask of audio formats.
\param delim The delimiter.
\return A string containing string representations of the valid formats in `formats`.
*/
std::string CX_SoundStream::formatsToString (RtAudioFormat formats, std::string delim) {
	vector<string> sFormats = formatsToStrings(formats);
	string rval;
	for (int i = 0; i < sFormats.size(); i++) {
		rval.append( sFormats[i] );
		if (i < sFormats.size() - 1) {
			rval.append( delim );
		}
	}
	return rval;
}

/*! For the given api, lists all of the devices on the system that support that api.
\param api Devices that support this API are scanned.
\return A machine-readable list of information. See http://www.music.mcgill.ca/~gary/rtaudio/structRtAudio_1_1DeviceInfo.html
for information about the members of the RtAudio::DeviceInfo struct.
*/
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

/*! For the given api, lists all of the devices on the system that support that api.
Lots of information about each device is given, like supported sample rates, number of
input and output channels, etc.
\param api Devices that support this API are scanned.
\return A human-readable formatted string containing the scanned information. Can be printed directly to std::cout or elsewhere.
*/
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

			rval << "Index: " << i << endl;
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

			rval << "Supported formats: " << formatsToString(dev.nativeFormats, ", ");

			rval << endl;
			rval << "---------------------------------------" << endl;

		}

	}
	return rval.str();
}

/*! This function exists to serve a per-computer configuration function that is otherwise difficult to provide
due to the fact that C++ programs are compiled to binaries and cannot be easily edited on the computer on which
they are running. This function takes the file name of a specially constructed configuration file and reads the
key-value pairs in that file in order to fill a CX_SoundStream::Configuration struct. The format of the file is
provided in the example code below.

Sample configuration file:
\code
ss.api = WINDOWS_DS //See convertStringToApi() for valid API names.
ss.sampleRate = 44100
ss.bufferSize = 512
ss.inputChannels = 0
//ss.inputDeviceId not used in this example because no input channels are used. It would take a non-negative integer.
ss.outputChannels = 2
ss.outputDeviceId = 0
ss.streamOptions.numberOfBuffers = 4
ss.streamOptions.flags = RTAUDIO_SCHEDULE_REALTIME | RTAUDIO_MINIMIZE_LATENCY //The | is not needed,
//but it matches the way these flags are used in code. All flags are supported.
//ss.streamOptions.priority is not used in this example. It would take a positive integer.
\endcode
All of the configuration keys are used in this example.
Any values in the \ref CX_SoundStream::Configuration struct that do not have values provided in the configuration
file will be left at default values. Note that the "ss" prefix allows this configuration to be embedded in
a file that also performs other configuration functions. Note that the names of the data members match the names
used in the \ref CX_SoundStream::Configuration struct and have a 1-to-1 relationship with those values.

Because this function uses CX::Util::readKeyValueFile() internally, it has the same arguments.
\param filename The name of the file containing configuration data.
\param delimiter The string that separates the key from the value. In the example, it is "=", but can be other values.
\param trimWhitespace If true, whitespace characters surrounding both the key and value will be removed. This is a good idea to do.
\param commentString If commentString is not the empty string (i.e. ""), everything on a line 
following the first instance of commentString will be ignored.
*/
CX_SoundStream::Configuration CX_SoundStream::readConfigurationFromFile(std::string filename, std::string delimiter, 
																		bool trimWhitespace, std::string commentString) 
{
	std::map<std::string, std::string> kv = CX::Util::readKeyValueFile(filename, delimiter, trimWhitespace, commentString);
	
	CX_SoundStream::Configuration ssCon;

	if (kv.find("ss.api") != kv.end()) {
		ssCon.api = CX_SoundStream::convertStringToApi(kv["ss.api"]);
	}
	if (kv.find("ss.bufferSize") != kv.end()) {
		ssCon.bufferSize = ofFromString<unsigned int>(kv["ss.bufferSize"]);
	}
	if (kv.find("ss.inputChannels") != kv.end()) {
		ssCon.inputChannels = ofFromString<int>(kv["ss.inputChannels"]);
	}
	if (kv.find("ss.outputChannels") != kv.end()) {
		ssCon.outputChannels = ofFromString<int>(kv["ss.outputChannels"]);
	}
	if (kv.find("ss.inputDeviceId") != kv.end()) {
		ssCon.inputDeviceId = ofFromString<int>(kv["ss.inputDeviceId"]);
	}
	if (kv.find("ss.outputDeviceId") != kv.end()) {
		ssCon.outputDeviceId = ofFromString<int>(kv["ss.outputDeviceId"]);
	}
	if (kv.find("ss.sampleRate") != kv.end()) {
		ssCon.sampleRate = ofFromString<int>(kv["ss.sampleRate"]);
	}
	if (kv.find("ss.streamOptions.numberOfBuffers") != kv.end()) {
		ssCon.streamOptions.numberOfBuffers = ofFromString<unsigned int>(kv["ss.streamOptions.numberOfBuffers"]);
	}
	if (kv.find("ss.streamOptions.priority") != kv.end()) {
		ssCon.streamOptions.priority = ofFromString<int>(kv["ss.streamOptions.priority"]);
	}

	if (kv.find("ss.streamOptions.flags") != kv.end()) {
		ssCon.streamOptions.flags = 0;
		string flags = kv["ss.streamOptions.flags"];

		if (flags.find("RTAUDIO_NONINTERLEAVED") != std::string::npos) {
			ssCon.streamOptions.flags |= RTAUDIO_NONINTERLEAVED; //emit warning. Don't use this flag!
		}
		if (flags.find("RTAUDIO_MINIMIZE_LATENCY") != std::string::npos) {
			ssCon.streamOptions.flags |= RTAUDIO_MINIMIZE_LATENCY;
		}
		if (flags.find("RTAUDIO_HOG_DEVICE") != std::string::npos) {
			ssCon.streamOptions.flags |= RTAUDIO_HOG_DEVICE;
		}
		if (flags.find("RTAUDIO_ALSA_USE_DEFAULT") != std::string::npos) {
			ssCon.streamOptions.flags |= RTAUDIO_ALSA_USE_DEFAULT;
		}
		if (flags.find("RTAUDIO_SCHEDULE_REALTIME") != std::string::npos) {
			ssCon.streamOptions.flags |= RTAUDIO_SCHEDULE_REALTIME;
		}
	}

	return ssCon;
}


int CX_SoundStream::_rtAudioCallbackHandler (void *outputBuffer, void *inputBuffer, unsigned int bufferSize, double streamTime, RtAudioStreamStatus status) {

	_lastSwapTime = CX::Instances::Clock.now();

	if (status != 0) {
		Log.error("CX_SoundStream") << "Buffer underflow/overflow detected.";
	}

	//I don't think that I really need to check for this error. I will check for a while and if it never happens, I might remove the check.
	if (_config.bufferSize != bufferSize) {
		Log.error("CX_SoundStream") << "The configuration's buffer size does not agree with the callback's buffer size. The stream is broken.";	
	}

	if (_config.inputChannels > 0) {
		CX_SoundStream::InputEventArgs callbackData;
		callbackData.inputBuffer = (float*)inputBuffer;
		callbackData.bufferSize = bufferSize;
		callbackData.inputChannels = _config.inputChannels;
		callbackData.instance = this;

		//Does this data need to be passed on to the listener?
		if (status & RTAUDIO_INPUT_OVERFLOW) {
			callbackData.bufferOverflow = true;
		}

		ofNotifyEvent( this->inputEvent, callbackData );
	}

	if (_config.outputChannels > 0) {

		//Set the output to 0 so that if the event listener(s) do(es) nothing, this passes silence. This is wasteful if the event listeners do stuff.
		memset(outputBuffer, 0, bufferSize * _config.outputChannels * sizeof(float));

		CX_SoundStream::OutputEventArgs callbackData;
		callbackData.outputBuffer = (float*)outputBuffer;
		callbackData.bufferSize = bufferSize;
		callbackData.outputChannels = _config.outputChannels;
		callbackData.instance = this;

		//Does this data need to be passed on to the listener?
		if (status & RTAUDIO_OUTPUT_UNDERFLOW) {
			callbackData.bufferUnderflow = true;
		}

		ofNotifyEvent(this->outputEvent, callbackData);
	}

	_lastSampleNumber += bufferSize;

	return 0; //Return 0 to keep the stream going.
}

int CX_SoundStream::_rtAudioCallback (void *outputBuffer, void *inputBuffer, unsigned int bufferSize, double streamTime, RtAudioStreamStatus status, void *data) {
	return ((CX_SoundStream*)data)->_rtAudioCallbackHandler(outputBuffer, inputBuffer, bufferSize, streamTime, status);
}