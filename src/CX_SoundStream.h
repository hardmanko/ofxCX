#pragma once

#include "RtAudio.h"

#include "ofConstants.h"
#include "ofBaseSoundStream.h"
#include "ofTypes.h"
#include "ofEvents.h"

#include "CX_Clock.h"
#include "CX_Logger.h"

namespace CX {

/*! \class CX::CX_SoundStream
This class provides a method for directly accessing and manipulating sound data that is sent/received from
sound hardware. To use this class, you should set up the stream (see setup()), set a user function that will
be called when either the outputEvent or inputEvent is triggered, and start the stream with start().

If the stream in configured for output, the output event will be triggered whenever the sound card needs 
more sound data. If the stream is configured for input, the input event will be triggered whenever some
amount of sound data has been recorded.

CX_SoundStream uses RtAudio internally, so you are having problems, you might be able to figure out what is
going wrong by checking out the page for RtAudio: http://www.music.mcgill.ca/~gary/rtaudio/index.html
\ingroup sound
*/
class CX_SoundStream {
public:

	/*! This struct controls the configuration of the CX_SoundStream. */
	struct Configuration {

		Configuration(void) :
			inputChannels(0),
			outputChannels(0),
			sampleRate(44100),
			bufferSize(4096),

			api(RtAudio::Api::UNSPECIFIED),

			inputDeviceId(-1),
			outputDeviceId(-1)
		{
			//streamOptions.streamName = "CX_SoundStream";
			streamOptions.numberOfBuffers = 2; //More buffers means higher latency but fewer glitches. Same applies to bufferSize.
			streamOptions.flags = RTAUDIO_SCHEDULE_REALTIME; // | RTAUDIO_HOG_DEVICE | RTAUDIO_MINIMIZE_LATENCY;
			streamOptions.priority = 1;
		}

		int inputChannels; //!< The number of input (e.g. microphone) channels to use. If 0, no input will be used.
		int outputChannels; //!< The number of output channels to use. Currently only stereo and mono are well-supported. If 0, no output will be used.
		int sampleRate; /*!< The requested sample rate for the input and output channels. If, for the selected device(s), this sample
						cannot be used, the nearest greater sample rate will be chosen. If there is no greater sample rate, the next
						lower sample rate will be used. */

		/*! The size of the audio data buffer to use, in sample frames. A larger buffer size means more latency 
		but also a greater potential for audio glitches (clicks and pops). Buffer size is per channel (i.e. 
		if there are two channels and buffer size is set to 256, the actual buffer size will be 512 samples). */
		unsigned int bufferSize;

		/*! This argument depends on your operating system. Using RtAudio::Api::UNSPECIFIED will pick an available API for 
		your system (if any; see the links below). The API means the type of software interface to use. For example,
		on Windows, you can choose from Windows Direct Sound (DS) and ASIO. ASIO is commonly used with audio recording equipment
		because	it has lower latency whereas DS is more of a consumer-grade interface. The choice of API does not affect
		how you use this class, but it may affect the performance of sound playback.

		See http://www.music.mcgill.ca/~gary/rtaudio/classRtAudio.html#ac9b6f625da88249d08a8409a9db0d849 for a listing of
		the APIs. See http://www.music.mcgill.ca/~gary/rtaudio/classRtAudio.html#afd0bfa26deae9804e18faff59d0273d9 for the
		default ordering of the APIs if RtAudio::Api::UNSPECIFIED is used. */
		RtAudio::Api api;

		/*! See http://www.music.mcgill.ca/~gary/rtaudio/structRtAudio_1_1StreamOptions.html for more information.

		`flags` must not include RTAUDIO_NONINTERLEAVED: The audio data used by CX is interleaved.
		*/
		RtAudio::StreamOptions streamOptions;

		int inputDeviceId; //!< The ID of the desired input device. A value less than 0 will cause the system default input device to be used.
		int outputDeviceId; //!< The ID of the desired output device. A value less than 0 will cause the system default output device to be used.

		bool setFromFile(std::string filename, std::string delimiter = "=", bool trimWhitespace = true, std::string commentStr = "//", std::string keyPrefix = "ss.");

	};

	/*! The audio output event of the CX_SoundStream sends a copy of this structure with
	the fields filled out when the event is called. */
	struct OutputEventArgs {
		OutputEventArgs(void) :
			bufferUnderflow(false)
		{};

		bool bufferUnderflow; //!< This is set to true if there was a buffer underflow, which means that the sound hardware ran out of data to output.
		float *outputBuffer; //!< A pointer to an array that should be filled with sound data.
		unsigned int bufferSize; //!< The number of sample frames that are in `outputBuffer`. The total number of samples is `bufferSize * outputChannels`.
		int outputChannels; //!< The number of channels worth of data in `outputBuffer`.

		CX_SoundStream *instance; //!< A pointer to the CX_SoundStream instance that notified this output event.
	};

	/*! The audio input event of the CX_SoundStream sends a copy of this structure with
	the fields filled out when the event is called. */
	struct InputEventArgs {
		InputEventArgs(void) :
			bufferOverflow(false)
		{};

		bool bufferOverflow; //!< This is set to true if there was a buffer overflow, which means that the sound hardware recorded data that was not processed.
		float *inputBuffer; //!< A pointer to an array of sound data that should be processed by the event handler function.
		unsigned int bufferSize; //!< The number of sample frames that are in `inputBuffer`. The total number of samples is `bufferSize * inputChannels`.
		int inputChannels; //!< The number of channels worth of data in `inputBuffer`.

		CX_SoundStream *instance; //!< A pointer to the CX_SoundStream instance that notified this input event.
	};


	CX_SoundStream (void);
	~CX_SoundStream (void);

	bool setup(CX_SoundStream::Configuration &config);
	bool closeStream(void);

	bool start(void);
	bool stop(void);

	bool isStreamRunning(void) const;
	
	/*! Gets the configuration that was used on the last call to setup(). Because some of the configuration
	options are only suggestions, this function allows you to check what the actual used configuration was.
	\return A const reference to the configuration struct. */
	const CX_SoundStream::Configuration& getConfiguration (void) const { return _config; };
	
	/*! Returns the number of the sample frame that is about to be loaded into the stream buffer on the next buffer swap. */
	uint64_t getSampleFrameNumber (void) const { return _lastSampleNumber; };

	ofEvent<CX_SoundStream::OutputEventArgs> outputEvent; //!< This event is triggered every time the CX_SoundStream needs to feed more data to the output buffer of the sound card.
	ofEvent<CX_SoundStream::InputEventArgs> inputEvent; //!< This event is triggered every time the CX_SoundStream hsa gotten some data from the input buffer of the sound card.

	CX_Millis estimateTotalLatency(void) const;
	CX_Millis estimateLatencyPerBuffer(void) const;

	bool hasSwappedSinceLastCheck(void);
	void waitForBufferSwap(void);
	CX_Millis getLastSwapTime(void) const;
	CX_Millis estimateNextSwapTime(void) const;

	RtAudio* getRtAudioInstance(void) const;

	static std::vector<RtAudio::Api> getCompiledApis(void);
	static std::vector<std::string> convertApisToStrings(vector<RtAudio::Api> apis);
	static std::string convertApisToString(std::vector<RtAudio::Api> apis, std::string delim = "\r\n");
	static std::string convertApiToString(RtAudio::Api api);
	static RtAudio::Api convertStringToApi(std::string apiString);

	static std::vector<std::string> formatsToStrings(RtAudioFormat formats);
	static std::string formatsToString(RtAudioFormat formats, std::string delim = "\r\n");

	static std::vector<RtAudio::DeviceInfo> getDeviceList(RtAudio::Api api);
	static std::string listDevices(RtAudio::Api api);

private:

	static unsigned int _getBestSampleRate(unsigned int requestedSampleRate, RtAudio::Api api, int deviceIndex);

	static int _rtAudioCallback(void *outputBuffer, void *inputBuffer, unsigned int bufferSize, double streamTime, RtAudioStreamStatus status, void *data);

	int _rtAudioCallbackHandler (void *outputBuffer, void *inputBuffer, unsigned int bufferSize, double streamTime, RtAudioStreamStatus status);

	RtAudio *_rtAudio;
	CX_SoundStream::Configuration _config;

	CX_Millis _lastSwapTime;
	uint64_t _lastSampleNumber;
	uint64_t _sampleNumberAtLastCheck;
};

} //namespace CX
