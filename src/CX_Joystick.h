#pragma once

#include <deque>
#include <string>
#include <vector>

#include "CX_Clock.h"

namespace CX {

	/*! This class manages a joystick that is attached to the system (if any). If more than one joystick is needed
	for the experiment, you can create more instances of CX_Joystick other than the one in CX::Instances::Input. Unlike
	CX_Keyboard and CX_Mouse, CX_Joystick does not need to be in a CX_InputManager to work.
	\ingroup inputDevices */
	class CX_Joystick {
	public:

		/*! The type of the joystick event. */
		enum EventType {
			ButtonPress, //!< A button on the joystick has been pressed. See \ref Event::buttonIndex and \ref Event::buttonState for the event data.
			ButtonRelease, //!< A button on the joystick has been released. See \ref Event::buttonIndex and \ref Event::buttonState for the event data.
			AxisPositionChange //!< The joystick has been moved in one of its axes. See \ref Event::axisIndex and \ref Event::axisPosition for the event data.
		};

		/*! This struct contains information about joystick events. Joystick events are either a button
		press or release or a change in the axes of the joystick. */
		struct Event {
			int buttonIndex; //!< If `type` is ButtonPress or ButtonRelease, this contains the index of the button that was changed.
			unsigned char buttonState; //!< If `type` is ButtonPress or ButtonRelease, this contains the current state of the button.

			int axisIndex; //!< If `type` is AxisPositionChange, this contains the index of the axis which changed.
			float axisPosition; //!< If `type` is AxisPositionChange, this contains the amount by which the axis changed.

			CX_Millis time; //!< The time at which the event was registered. Can be compared to the result of CX::CX_Clock::now().

			/*! \brief The uncertainty in `time`, which represents the difference between the time at which this
			event was timestamped by CX and the last time that events were checked for. */
			CX_Millis uncertainty;

			EventType type; //!< The type of the event, from the CX_Joystick::EventType enum.
		};

		CX_Joystick(void);
		~CX_Joystick (void);

		bool setup(int joystickIndex);
		std::string getJoystickName(void);
		int getJoystickIndex(void);

		bool pollEvents(void);

		int availableEvents(void);
		CX_Joystick::Event getNextEvent(void);

		std::vector<CX_Joystick::Event> copyEvents(void);
		void clearEvents(void);

		std::vector<float> getAxisPositions(void);
		std::vector<unsigned char> getButtonStates(void);

		void appendEvent(CX_Joystick::Event ev);

	private:
		int _joystickIndex;
		std::string _joystickName;

		std::deque<CX_Joystick::Event> _joystickEvents;

		std::vector<float> _axisPositions;
		std::vector<unsigned char> _buttonStates;

		CX_Millis _lastEventPollTime;
	};

	std::ostream& operator<< (std::ostream& os, const CX_Joystick::Event& ev);
	std::istream& operator>> (std::istream& is, CX_Joystick::Event& ev);

}