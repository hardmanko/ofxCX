#pragma once

#include <thread>
#include <mutex>

#include "ofConstants.h" // eventually GLEW
#include "GLFW/glfw3.h"

#include "CX_Clock.h"
#include "CX_SynchronizationUtils.h"

namespace CX {

class CX_Display;

namespace Util {

std::vector<double> framePeriodToFrameRate(const std::vector<CX_Millis>& periods);

struct FrameRateEstimationConfig {

	FrameRateEstimationConfig(void);

	CX_Display* disp;

	CX_Millis estimationTime = 1000; //!< The length of time to spend estimating the frame period.

	//bool saveResults = true;

	double minFrameRate = 30; //!< Min allowed frame rate. If an observed duration is less than `1 / minFrameRate` seconds, it will be ignored for purposes of estimating the frame period.
	double maxFrameRate = 1000; //!< Max allowed frame rate.

	int minGoodIntervals = 4; //!< Min frame periods within range defined by `minFrameRate` and `maxFrameRate` for the results to be saved.
	int maxBadIntervalsPrinted = 20; //!< When there are excluded swap intervals, this sets the max number of bad time intervals printed to the console.


};

struct FrameEstimationResult {

	bool success = false;
	std::vector<std::string> messages;

	//void setSwapPeriods(const std::vector<CX_Millis>& framePeriods);

	//const std::vector<CX_Millis>& getSwapPeriods(void) const;
	void clear(void);

	std::vector<CX_Millis> allPeriods;

	void filterByFramePeriod(CX_Millis minPeriod, CX_Millis maxPeriod);
	void filterByFrameRate(double minRate, double maxRate);


	
	std::vector<CX_Millis> includedPeriods;
	std::vector<CX_Millis> excludedPeriods;

	// For includedPeriods.
	CX_Millis calcFramePeriodMean(void) const;
	CX_Millis calcFramePeriodSD(void) const;
	double calcFrameRateMean(void) const;
	// frame rate SD is an odd concept

};

FrameEstimationResult estimateFrameRate(const FrameRateEstimationConfig& estCfg);

struct FrameRateEstimator {

	using Configuration = FrameRateEstimationConfig;

	using Results = FrameEstimationResult;

	bool setup(const Configuration& cfg);

};


/*! Stores openGL version numbers and has a few helper functions. */
struct GLVersion {
	GLVersion(void) :
		major(0),
		minor(0),
		release(0)
	{}

	GLVersion(int maj, int min, int rel) :
		major(maj),
		minor(min),
		release(rel)
	{}

	int major;
	int minor;
	int release;

	int compare(int maj, int min, int rel) const;
	int compare(const GLVersion& that) const;

	bool supportsGLFenceSync(void) const;
	//bool supportsGLProgrammableRenderer(void) const; // This would be nice, but isn't really needed

	GLVersion getCorrespondingGLSLVersion(void) const;

};

// Maybe CX_RenderingContextManager
class GlfwContextManager {
public:

	void setup(GLFWwindow* context, std::thread::id mainThreadId); // Do not call: Only called in CX_EntryPoint

	bool trylock(void);
	void lock(void); // blocks until it can get a lock
	void unlock(void); // if isLockedByThisThread() == false, it is a programming error to call unlock()

	//bool isCurrentOnThisThread(void); // If the rendering context is current on the calling thread, returns true

	bool isUnlocked(void);

	bool isLockedByThisThread(void);
	bool isLockedByMainThread(void);
	bool isLockedByAnyThread(void);

	std::thread::id getLockingThreadId(void);

	GLFWwindow* get(void);

	bool isMainThread(void); // true if function is called in main thread. This doesn't really belong in this class

private:

	std::recursive_mutex _mutex; // mutex for accessing data of this class

	std::thread::id _lockingThreadId;
	std::thread::id _mainThreadId;

	GLFWwindow* _glfwContext;
};

/*! OpenGL fence sync helper object. Fence syncs are a feature of OpenGL
and are used to synchronize the CPU and GPU (video card).

CX uses this class internally. Users of CX are unlikely to need to use this class.

See https://www.khronos.org/opengl/wiki/Sync_Object for more about
fence syncs. You don't need to understand how to use the functions on
that page: This class wraps those functions.

Use `startSync()` to insert a fence into the OpenGL command queue.

As long as `isSyncing() == true`, call `updateSync()` regularly. Once `isSyncing() == false`,
the sync will be complete.

Alternately, you can call `updateSync()` then if `syncComplete() == true`, the sync is complete.

Either way, check the status of the completed sync with `syncSuccess()` or `getStatus()`.

Sync start and complete times can be accessed with `getStartTime()` and `getCompleteTime()`.
*/
class GLSyncHelper {
public:

	enum class SyncStatus : int {
		Idle,
		Syncing,
		SyncSuccess,
		SyncFailed,
		TimedOut
	};

	GLSyncHelper(void);

	void startSync(CX_Millis timeout = 0);
	bool isSyncing(void) const;
	void updateSync(void);

	void stopSyncing(void);
	void clear(void);

	SyncStatus getStatus(void) const;
	bool syncComplete(void) const;
	bool syncSuccess(void) const;

	CX_Millis getStartTime(void) const;
	CX_Millis getCompleteTime(void) const;

private:

	SyncStatus _status;

	GLsync _fenceSyncObject;

	CX_Millis _syncStart;
	CX_Millis _syncComplete;
	CX_Millis _timeout;

};

class DisplaySwapper {
public:
	enum class Mode {
		NominalPeriod,
		Prediction // with NominalPeriod as backup
	};

	struct Configuration {

		Configuration(void) :
			display(nullptr),
			client(nullptr),
			preSwapSafetyBuffer(2),
			mode(Mode::NominalPeriod)
		{}

		CX_Display* display;
		Sync::DataClient* client;

		CX_Millis preSwapSafetyBuffer;

		Mode mode;
	};

	bool setup(const Configuration& config);
	const Configuration& getConfiguration(void) const;

	bool shouldSwap(void) const;
	bool trySwap(void); // true if swap happened

private:
	Configuration _config;

	bool _NominalPeriod_shouldSwap(void) const;
	bool _Prediction_shouldSwap(void) const;

};

} // namespace Util
} // namespace CX