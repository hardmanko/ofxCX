#ifndef _CX_SOUNDSTREAM_H_
#define _CX_SOUNDSTREAM_H_

#include "RtAudio.h"

#include "ofConstants.h"
#include "ofBaseSoundStream.h"
#include "ofTypes.h"
#include "ofEvents.h"

#include "CX_Clock.h"
#include "CX_DeferredLogger.h"

#define CX_SOUND_STREAM_USE_DEFAULT_DEVICE -1

namespace CX {

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

		int inputChannels;
		int outputChannels;
		int sampleRate;
		unsigned int bufferSize; //Buffer size is per channel (i.e. if there are two channels and buffer size is set to 256, the actual buffer size will be 512 samples).

		RtAudio::Api api;
		RtAudio::StreamOptions streamOptions;

		int inputDeviceId;
		int outputDeviceId;

	};

	struct CX_SSOutputCallback_t {
		CX_SSOutputCallback_t (void) :
			bufferUnderflow(false)
		{};

		bool bufferUnderflow;
		float *outputBuffer;
		unsigned int bufferSize; //Isn't this a bit redundant?
	};

	struct CX_SSInputCallback_t {
		CX_SSInputCallback_t (void) :
			bufferOverflow(false)
		{};

		bool bufferOverflow;
		float *inputBuffer;
		unsigned int bufferSize;
	};


	class CX_SoundStream {
	public:

		CX_SoundStream (void);
		~CX_SoundStream (void);

		//Unlike the openFrameworks version, open does not start the stream automatically.
		bool open (CX_SoundStreamConfiguration_t &config);
		bool close (void);

		bool start (void);
		bool stop (void);
	
		CX_SoundStreamConfiguration_t& getConfiguration (void) { return _config; };
	
		uint64_t getLastSampleNumber (void) { return _lastSampleNumber; };
		void setLastSampleNumber (uint64_t sampleNumber) { _lastSampleNumber = sampleNumber; };

		int rtAudioCallbackHandler (void *outputBuffer, void *inputBuffer, unsigned int bufferSize, double streamTime, RtAudioStreamStatus status);
		ofEvent<CX_SSOutputCallback_t> outputCallbackEvent;
		ofEvent<CX_SSInputCallback_t> inputCallbackEvent;

		CX_Micros_t getStreamLatency (void);

		bool hasSwappedSinceLastCheck (void);
		CX_Micros_t getLastSwapTime (void) { return _lastSwapTime; };
		CX_Micros_t estimateNextSwapTime (void);

		static vector<RtAudio::Api> getCompiledApis (void);
		static vector<string> convertApisToStrings (vector<RtAudio::Api> apis);
		static string convertApisToString (vector<RtAudio::Api> apis);
		static string convertApiToString (RtAudio::Api api);

		static vector<string> supportedFormatsToString (RtAudioFormat formats);

		static vector<RtAudio::DeviceInfo> getDeviceList (RtAudio::Api api);
		static string listDevices (RtAudio::Api api);

	private:

		static int _rtAudioCallback(void *outputBuffer, void *inputBuffer, unsigned int bufferSize, double streamTime, RtAudioStreamStatus status, void *data);

		RtAudio *_rtAudio;
		CX_SoundStreamConfiguration_t _config;

		CX_Micros_t _lastSwapTime;
		uint64_t _lastSampleNumber;
		uint64_t _sampleNumberAtLastCheck;
	};

}

#endif //_CX_SOUNDSTREAM_H_