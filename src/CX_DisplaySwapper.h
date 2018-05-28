#pragma once

//#include "CX_Definitions.h"
#include "CX_SynchronizationUtils.h"

namespace CX {

class CX_Display;

class CX_DisplaySwapper {
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

} // namespace CX