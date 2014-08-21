#pragma once

#include <queue>
#include <string>
#include <vector>

#include "CX_Clock.h"

namespace CX {

	/*! This class manages a joystick that is attached to the system (if any). If more than one joystick is needed
	for the experiment, you can create more instances of CX_Joystick other than the one in CX::Instances::Input.
	\ingroup inputDevices */
	class CX_Joystick {
	public:

		/*! This struct contains information about joystick events. Joystick events are either a button
		press or release or a change in the axes of the joystick. */
		struct Event {
			int buttonIndex; //!< If eventType is BUTTON_PRESS or BUTTON_RELEASE, this contains the index of the button that was changed.
			unsigned char buttonState; //!< If eventType is BUTTON_PRESS or BUTTON_RELEASE, this contains the current state of the button.

			int axisIndex; //!< If eventType is AXIS_POSITION_CHANGE, this contains the index of the axis which changed.
			float axisPosition; //!< If eventType is AXIS_POSITION_CHANGE, this contains the amount by which the axis changed.

			CX_Millis eventTime; //!< The time at which the event was registered. Can be compared to the result of CX::CX_Clock::now().
			CX_Millis uncertainty; //!< The uncertainty in eventTime. The event occured some time between eventTime and eventTime minus uncertainty.

			/*! The type of the joystick event. */
			enum JoystickEventType {
				BUTTON_PRESS, //!< A button on the joystick has been pressed. See \ref buttonIndex and \ref buttonState for the event data.
				BUTTON_RELEASE, //!< A button on the joystick has been released. See \ref buttonIndex and \ref buttonState for the event data.
				AXIS_POSITION_CHANGE //!< The joystick has been moved in one of its axes. See \ref axisIndex and \ref axisPosition for the event data.
			} eventType; //!< The type of the event.
		};

		CX_Joystick (void);
		~CX_Joystick (void);

		bool setup (int joystickIndex);
		std::string getJoystickName (void);

		//Preferred interface. This interface collects response time data.
		bool pollEvents (void);
		int availableEvents (void);
		CX_Joystick::Event getNextEvent (void);
		void clearEvents (void);

		//Direct access functions. The preferred interface is pollEvents() and getNextEvent().
		std::vector<float> getAxisPositions (void);
		std::vector<unsigned char> getButtonStates (void);

	private:
		int _joystickIndex;
		std::string _joystickName;

		std::queue<CX_Joystick::Event> _joystickEvents;

		std::vector<float> _axisPositions;
		std::vector<unsigned char> _buttonStates;

		CX_Millis _lastEventPollTime;
	};

	std::ostream& operator<< (std::ostream& os, const CX_Joystick::Event& ev);
	std::istream& operator>> (std::istream& is, CX_Joystick::Event& ev);

}