#include "CX_DisplaySwapper.h"

#include "CX_Display.h"

namespace CX {

bool CX_DisplaySwapper::setup(const Configuration& config) {
	if (!config.display) {
		return false;
	}

	_config = config;

	if (!_config.client) {
		_config.client = &_config.display->swapClient;
	}

	if (config.preSwapSafetyBuffer < CX_Millis(1)) {
		Instances::Log.warning("CX_DisplaySwapper") << "setup(): config.preSwapSafetyBuffer was less than 1 millisecond. It is recommended that preSwapSafetyBuffer be at least one millisecond.";
		if (_config.preSwapSafetyBuffer < CX_Millis(0)) {
			_config.preSwapSafetyBuffer = CX_Millis(0);
		}
	}

	return true;
}

const CX_DisplaySwapper::Configuration& CX_DisplaySwapper::getConfiguration(void) const {
	return _config;
}


bool CX_DisplaySwapper::shouldSwap(void) const {

	switch (_config.mode) {
	case Mode::NominalPeriod:
		return _NominalPeriod_shouldSwap();
	case Mode::Prediction:
		return _Prediction_shouldSwap();
	}

	return false;
}

// true if swap happened
bool CX_DisplaySwapper::trySwap(void) {

	if (!shouldSwap()) {
		return false;
	}

	// no, no, cache it somehow (but how do you know you have an up-to-date copy???) ofParameter?
	//Private::swapVideoBuffers(_config.display->usingSoftwareVSync());

	_config.display->swapBuffers(); // or do this

	return true;
}

bool CX_DisplaySwapper::_NominalPeriod_shouldSwap(void) const {

	// TODO: cache the value of getFramePeriod somehow?
	CX_Millis nextSwapEst = _config.display->getLastSwapTime() + _config.display->getFramePeriod();

	CX_Millis timeToSwap = nextSwapEst - Instances::Clock.now();

	return timeToSwap < _config.preSwapSafetyBuffer;

}

bool CX_DisplaySwapper::_Prediction_shouldSwap(void) const {
	Sync::TimePrediction tp = _config.client->predictNextSwapTime();

	if (tp.usable) {

		tp.pred -= Instances::Clock.now();

		CX_Millis minTimeToSwap = tp.lowerBound();

		return minTimeToSwap < _config.preSwapSafetyBuffer;

	}

	return _NominalPeriod_shouldSwap();
}


} // namespace CX