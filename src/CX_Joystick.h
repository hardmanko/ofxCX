#ifndef _CX_JOYSTICK_H_
#define _CX_JOYSTICK_H_

#include "CX_Clock.h"

#include <queue>
#include <string>
#include <vector>

using namespace std;

namespace CX {

	struct CX_JoystickEvent_t {

		int buttonIndex;
		unsigned char buttonState;

		int axisIndex;
		float axisPosition;

		CX_Micros_t eventTime;
		CX_Micros_t uncertainty;

		enum {
			BUTTON_PRESS,
			BUTTON_RELEASE,
			AXIS_POSITION_CHANGE
		} eventType;

	};

	class CX_Joystick {
	public:
		CX_Joystick (void);
		~CX_Joystick (void);

		bool setup (int joystickIndex);
		string getJoystickName (void);

		//Preferred interface. This interface collects response time data.
		bool pollEvents (void);
		int availableEvents (void);
		CX_JoystickEvent_t getNextEvent (void);
		void clearEvents (void);

		//Direct access functions. The preferred interface is pollEvents() and getNextEvent().
		vector<float> getAxisPositions (void);
		vector<unsigned char> getButtonStates (void);

	private:
		int _joystickIndex;
		string _joystickName;

		queue<CX_JoystickEvent_t> _joystickEvents;

		vector<float> _axisPositions;
		vector<unsigned char> _buttonStates;

		CX_Micros_t _lastEventPollTime;
	};

}

#endif //_CX_JOYSTICK_H_