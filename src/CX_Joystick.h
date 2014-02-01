#ifndef _CX_JOYSTICK_H_
#define _CX_JOYSTICK_H_

#include <queue>
#include <string>
#include <vector>

#include "CX_Clock.h"

namespace CX {

	struct CX_JoystickEvent_t {

		int buttonIndex;
		unsigned char buttonState;

		int axisIndex; //You should reconsider this. Maybe instead give all of the axes when any of them change.
		float axisPosition;

		CX_Micros eventTime;
		CX_Micros uncertainty;

		enum {
			BUTTON_PRESS,
			BUTTON_RELEASE,
			AXIS_POSITION_CHANGE
		} eventType;

	};

	std::ostream& operator<< (std::ostream& os, const CX_JoystickEvent_t& ev);
	std::istream& operator>> (std::istream& is, CX_JoystickEvent_t& ev);

	class CX_Joystick {
	public:
		CX_Joystick (void);
		~CX_Joystick (void);

		bool setup (int joystickIndex);
		std::string getJoystickName (void);

		//Preferred interface. This interface collects response time data.
		bool pollEvents (void);
		int availableEvents (void);
		CX_JoystickEvent_t getNextEvent (void);
		void clearEvents (void);

		//Direct access functions. The preferred interface is pollEvents() and getNextEvent().
		std::vector<float> getAxisPositions (void);
		std::vector<unsigned char> getButtonStates (void);

	private:
		int _joystickIndex;
		std::string _joystickName;

		std::queue<CX_JoystickEvent_t> _joystickEvents;

		std::vector<float> _axisPositions;
		std::vector<unsigned char> _buttonStates;

		CX_Micros _lastEventPollTime;
	};

}

#endif //_CX_JOYSTICK_H_