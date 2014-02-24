#ifndef _CX_SOUNDSTREAM_H_
#define _CX_SOUNDSTREAM_H_

/*! \class CX::CX_SoundStream
CX_SoundStream uses RtAudio internally, so you are having problems, you might be able to figure out what is
going wrong by checking out the page for it: http://www.music.mcgill.ca/~gary/rtaudio/index.html
\ingroup sound
*/

#include "RtAudio.h"

#include "ofConstants.h"
#include "ofBaseSoundStream.h"
#include "ofTypes.h"
#include "ofEvents.h"

#include "CX_Clock.h"
#include "CX_Logger.h"

#define CX_SOUND_STREAM_USE_DEFAULT_DEVICE -1

namespace CX {

	class CX_SoundStream;

	/*! This struct controls the configuration of the CX_SoundStream.
	\ingroup sound*/
	struct CX_SoundStreamConfiguration_t {

		CX_SoundStreamConfiguration_t (void) :
			inputChannels(0),
			outputChannels(0),
			sampleRate(48000),
			bufferSize(4096),

			api(RtAudio::UNSPECIFIED),

			inputDeviceId(CX_SOUND_STREAM_USE_DEFAULT_DEVICE),
			outputDeviceId(CX_SOUND_STREAM_USE_DEFAULT_DEVICE)
		{
			//streamOptions.streamName = "CX_SoundStream";
			streamOptions.numberOfBuffers = 2; //More buffers == higher latency but fewer glitches. Same applies to bufferSize.
			streamOptions.flags = RTAUDIO_SCHEDULE_REALTIME; // | RTAUDIO_HOG_DEVICE | RTAUDIO_MINIMIZE_LATENCY;
			streamOptions.priority = 1;
		}

		int inputChannels; //!< The number of input (e.g. microphone) channels to use. If 0, no input will be used.
		int outputChannels; //!< The number of output channels to use. Currently only stereo and mono are well-supported.
		int sampleRate; /*!< The requested sample rate for the input and output channels. If, for the selected device(s), this sample
						cannot be used, the nearest greater sample rate will be chosen. If there is no greater sample rate, the next
						lower sample rate will be used. */

		/*! The size of the audio data buffer to use. A larger buffer size means more latency but also a greater potential for audio glitches
		(clicks and pops). Buffer size is per channel (i.e. if there are two channels and buffer size is set to 256, the actual buffer size 
		will be 512 samples). Defaults to 4096 samples. */
		unsigned int bufferSize;

		/*! This argument depends on your operating system. Using RtAudio::Api::UNSPECIFIED will attempt to pick a working
		API from those that are available on your system. The API means the type of software interface to use. For example,
		on Windows, you can choose from Windows Direct Sound (DS) and ASIO. ASIO is commonly used with audio recording equipment 
		because	it has lower latency whereas DS is more of a consumer-grade interface. The choice of API does not affect
		how you use this class, but it may affect the performance of sound playback.
		
		See http://www.music.mcgill.ca/~gary/rtaudio/classRtAudio.html#ac9b6f625da88249d08a8409a9db0d849 for a listing of
		the APIs. See http://www.music.mcgill.ca/~gary/rtaudio/classRtAudio.html#afd0bfa26deae9804e18faff59d0273d9 for the 
		default ordering of the APIs if RtAudio::Api::UNSPECIFIED is used. */
		RtAudio::Api api;

		/*! See http://www.music.mcgill.ca/~gary/rtaudio/structRtAudio_1_1StreamOptions.html for more information.

		flags must not include RTAUDIO_NONINTERLEAVED: The audio data used by CX is interleaved.
		*/
		RtAudio::StreamOptions streamOptions;

		int inputDeviceId; //!< The ID of the desired input device. A value of -1 will cause the system default input device to be used.
		int outputDeviceId; //!< The ID of the desired output device. A value of -1 will cause the system default output device to be used.

	};

	struct CX_SSOutputCallback_t {
		CX_SSOutputCallback_t (void) :
			bufferUnderflow(false)
		{};

		bool bufferUnderflow;
		float *outputBuffer;
		unsigned int bufferSize; //This is really "neededSamples". (This * outputChannels) == the actual size of the buffer.
		int outputChannels;

		CX_SoundStream *instance;
	};

	struct CX_SSInputCallback_t {
		CX_SSInputCallback_t (void) :
			bufferOverflow(false)
		{};

		bool bufferOverflow;
		float *inputBuffer;
		unsigned int bufferSize;
		int inputChannels;

		CX_SoundStream *instance;
	};


	class CX_SoundStream {
	public:

		CX_SoundStream (void);
		~CX_SoundStream (void);

		bool setup(CX_SoundStreamConfiguration_t &config);
		bool closeStream (void);

		bool start (void);
		bool stop (void);
	
		/*! Gets the configuration that was used on the last call to open(). Because some of the configuration
		options are only suggestions, this function allows you to check what the actual configuration was.
		\return A const reference to the configuration struct. */
		const CX_SoundStreamConfiguration_t& getConfiguration (void) { return _config; };
	
		uint64_t getLastSampleNumber (void) { return _lastSampleNumber; };
		void setLastSampleNumber (uint64_t sampleNumber) { _lastSampleNumber = sampleNumber; };

		
		ofEvent<CX_SSOutputCallback_t> outputCallbackEvent;
		ofEvent<CX_SSInputCallback_t> inputCallbackEvent;

		CX_Micros getStreamLatency (void);

		bool hasSwappedSinceLastCheck (void);
		/*! Gets the time at which the last buffer swap occurred. \return This time value can be compared with the result of CX::Instances::Clock.getTime(). */
		CX_Micros getLastSwapTime (void) { return _lastSwapTime; };
		CX_Micros estimateNextSwapTime (void);

		static std::vector<RtAudio::Api> getCompiledApis (void);
		static std::vector<std::string> convertApisToStrings (vector<RtAudio::Api> apis);
		static std::string convertApisToString (vector<RtAudio::Api> apis, std::string delim = "\r\n");
		static std::string convertApiToString (RtAudio::Api api);

		static std::vector<std::string> formatsToStrings (RtAudioFormat formats);
		static std::string formatsToString (RtAudioFormat formats, std::string delim = "\r\n");

		static std::vector<RtAudio::DeviceInfo> getDeviceList (RtAudio::Api api);
		static std::string listDevices (RtAudio::Api api);

	private:

		static int _rtAudioCallback(void *outputBuffer, void *inputBuffer, unsigned int bufferSize, double streamTime, RtAudioStreamStatus status, void *data);

		int _rtAudioCallbackHandler (void *outputBuffer, void *inputBuffer, unsigned int bufferSize, double streamTime, RtAudioStreamStatus status);

		RtAudio *_rtAudio;
		CX_SoundStreamConfiguration_t _config;

		CX_Micros _lastSwapTime;
		uint64_t _lastSampleNumber;
		uint64_t _sampleNumberAtLastCheck;
	};

}

#endif //_CX_SOUNDSTREAM_H_