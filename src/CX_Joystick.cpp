#include "CX_Joystick.h"

#include "ofAppGLFWWindow.h"

namespace CX {

CX_Joystick::CX_Joystick (void) :
    _joystickIndex(-1),
	_joystickName("unnamed")
{
}

CX_Joystick::~CX_Joystick (void) {
}

/*!
Set up the joystick by attempting to initialize the joystick at the given index. If the
joystick is present on the system, it will be initialized and its name can be accessed
by calling getJoystickName().

If the set up is successful (i.e. if the selected joystick is present on the system), this
function will return true. If the joystick is not present, it will return false.

\param joystickIndex The index of the joystick to set up. 
If `joystickIndex >= 0`, an attempt will be made to set up the joystick at that index. 
If `joystickIndex < 0`, no attempt will be made to set up the joystick and the joystick will be disabled.

\return `true` if a joystick at the selected index is present, `false` otherwise.
*/
bool CX_Joystick::setup(int joystickIndex) {
	if (glfwJoystickPresent(joystickIndex) == GL_FALSE) {
		return false;
	}

	_joystickIndex = joystickIndex;

	const char *name = glfwGetJoystickName(_joystickIndex);
	if (name != NULL) {
		std::string s(name);
		_joystickName = s;
	}

	int axisCount = 0;
	glfwGetJoystickAxes(_joystickIndex, &axisCount);
	_axisPositions.resize(axisCount);

	int buttonCount = 0;
	glfwGetJoystickButtons(_joystickIndex, &buttonCount);
	_buttonStates.resize(buttonCount);

	return true;

}

/*! Get the name of the joystick, presumably as set by the joystick driver.
The name may not be very meaningful. */
std::string CX_Joystick::getJoystickName (void) {
	return _joystickName;
}

/*! Get the integer index of the currently selected joystick. */
int CX_Joystick::getJoystickIndex(void) {
	return _joystickIndex;
}

/*! Check to see if there are any new joystick events. If there are new events,
they can be accessed with availableEvents() and getNextEvent().
\return True if there are new events.*/
bool CX_Joystick::pollEvents (void) {
	if (_joystickIndex == -1) {
		return false;
	}

	int axisCount = 0;
	const float *axes = glfwGetJoystickAxes(_joystickIndex, &axisCount);

	int buttonCount = 0;
	const unsigned char *buttons = glfwGetJoystickButtons(_joystickIndex, &buttonCount);

	CX_Millis pollTime = CX::Instances::Clock.now();

	if ((unsigned int)axisCount == _axisPositions.size()) {
		for (unsigned int i = 0; i < (unsigned int)axisCount; i++) {
			if (_axisPositions[i] != axes[i]) {
				CX_Joystick::Event ev;

				ev.type = CX_Joystick::EventType::AxisPositionChange;
				ev.axisIndex = i;
				ev.axisPosition = axes[i];

				ev.time = pollTime;
				ev.uncertainty = ev.time - _lastEventPollTime;

				_joystickEvents.push_back(ev);

				_axisPositions[i] = axes[i];
			}
		}
	}

	if ((unsigned int)buttonCount == _buttonStates.size()) {
		for (unsigned int i = 0; i < (unsigned int)buttonCount; i++) {
			if (_buttonStates[i] != buttons[i]) {
				CX_Joystick::Event ev;

				//I'm just guessing about button state here. 1 might be Pressed, but it could also be UNDEFINED_BUTTON.
				if (buttons[i] == 1) {
					ev.type = CX_Joystick::EventType::ButtonPress;
				} else {
					ev.type = CX_Joystick::EventType::ButtonRelease;
				}

				ev.buttonIndex = i;
				ev.buttonState = buttons[i];

				ev.time = pollTime;
				ev.uncertainty = ev.time - _lastEventPollTime;

				_joystickEvents.push_back( ev );

				_buttonStates[i] = buttons[i];
			}
		}
	}

	_lastEventPollTime = pollTime;

	if (_joystickEvents.size() > 0) {
		return true;
	}
	return false;
}

/*! Get the number of available events for this input device. 
Events can be accessed with CX_Joystick::getNextEvent() or CX_Joystick::copyEvents(). */
int CX_Joystick::availableEvents (void) {
	return _joystickEvents.size();
}

/*! Get the next event available for this input device. This is a destructive operation in which the returned event is deleted
from the input device. */
CX_Joystick::Event CX_Joystick::getNextEvent (void) {
	CX_Joystick::Event front = _joystickEvents.front();
	_joystickEvents.pop_front();
	return front;
}

/*! Clear (delete) all events from this input device.
\note Unpolled events are not cleared by this function, which means that
responses made after a call to CX_InputManager::pollEvents() but before a call to
clearEvents() will not be removed by calling clearEvents(). */
void CX_Joystick::clearEvents (void) {
	_joystickEvents.clear();
}

/*! \brief Return a vector containing a copy of the currently stored events. The events stored by
the input device are unchanged. The first element of the vector is the oldest event. */
std::vector<CX_Joystick::Event> CX_Joystick::copyEvents(void) {
	std::vector<CX_Joystick::Event> copy(_joystickEvents.size());
	for (unsigned int i = 0; i < _joystickEvents.size(); i++) {
		copy[i] = _joystickEvents.at(i);
	}
	return copy;
}

/*! This function returns in the current positions of the joystick axes.
\return A vector of the current axis positions. */
vector<float> CX_Joystick::getAxisPositions (void) {
	return _axisPositions;

	/* This function is to be used for direct access to the axis positions of the joystick. It does not
	generate events (i.e. CX_Joystick::Event), nor does it do any timestamping. If timestamps and
	uncertainies are desired, you MUST use pollEvents() and the associated event functions (e.g. getNextEvent()). */
	/*
	vector<float> pos;
	if (_joystickIndex == -1) {
		return pos;
	}

	int axisCount = 0;
	const float *axes = glfwGetJoystickAxes(_joystickIndex, &axisCount);
	pos.resize(axisCount);
	for (int i = 0; i < axisCount; i++) {
		pos[i] = axes[i];
	}
	return pos;
	*/
}

/*! This function returns in the current states of the joystick buttons.
\return A vector of the current button states. */
vector<unsigned char> CX_Joystick::getButtonStates (void) {
	return _buttonStates;

	/* This function is to be used for direct access to the button states of the joystick. It does not
	generate events (i.e. CX_Joystick::Event), nor does it do any timestamping. If timestamps and
	uncertainies are desired, you MUST use pollEvents() and the associated event functions (e.g. getNextEvent()). */
	/*
	vector<unsigned char> but;
	if (_joystickIndex == -1) {
		return but;
	}

	int buttonCount = 0;
	const unsigned char *buttons = glfwGetJoystickButtons(_joystickIndex, &buttonCount);
	but.resize(buttonCount);
	for (int i = 0; i < buttonCount; i++) {
		but[i] = buttons[i];
	}
	return but;
	*/
}

/*! Appends a joystick event to the event queue without any modification
(e.g. the timestamp is not set to the current time, it is left as-is).
This can be useful if you want to have a simulated participant perform the
task for debugging purposes.
\param ev The event to append.
*/
void CX_Joystick::appendEvent(CX_Joystick::Event ev) {
	if (ev.type == CX_Joystick::AxisPositionChange) {
		_axisPositions[ev.axisIndex] = ev.axisPosition;
	} else if (ev.type == CX_Joystick::ButtonPress) {
		_buttonStates[ev.buttonIndex] = 1;
	} else if (ev.type == CX_Joystick::ButtonRelease) {
		_buttonStates[ev.buttonIndex] = 0;
	}

	_joystickEvents.push_back(ev);
}

static const std::string dlm = ", ";

/*! \brief Stream insertion operator for the CX_Joystick::Event struct. */
std::ostream& operator<< (std::ostream& os, const CX_Joystick::Event& ev) {
	os << ev.buttonIndex << dlm << ev.buttonState << dlm << ev.axisIndex << dlm << ev.axisPosition << dlm <<
		ev.time << dlm << ev.uncertainty << dlm << ev.type;
	return os;
}

/*! \brief Stream extraction operator for the CX_Joystick::Event struct. */
std::istream& operator>> (std::istream& is, CX_Joystick::Event& ev) {
	is >> ev.buttonIndex;
	is.ignore(dlm.size());
	is >> ev.buttonState;
	is.ignore(dlm.size());
	is >> ev.axisIndex;
	is.ignore(dlm.size());
	is >> ev.axisPosition;
	is.ignore(dlm.size());
	is >> ev.time;
	is.ignore(dlm.size());
	is >> ev.uncertainty;
	is.ignore(dlm.size());

	int eventType;
	is >> eventType;
	switch (eventType) {
	case CX_Joystick::EventType::ButtonPress: ev.type = CX_Joystick::EventType::ButtonPress; break;
	case CX_Joystick::EventType::ButtonRelease: ev.type = CX_Joystick::EventType::ButtonRelease; break;
	case CX_Joystick::EventType::AxisPositionChange: ev.type = CX_Joystick::EventType::AxisPositionChange; break;
	}

	return is;
}

} //namespace CX
