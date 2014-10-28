#include "CX_Joystick.h"

#include "ofAppGLFWWindow.h"

namespace CX {

CX_Joystick::CX_Joystick (void) :
	_joystickName("NULL"),
	_joystickIndex(-1)
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
*/
bool CX_Joystick::setup (int joystickIndex) {
	if (glfwJoystickPresent(joystickIndex) == GL_TRUE) {
		_joystickIndex = joystickIndex;

		const char *name = glfwGetJoystickName(_joystickIndex);
		if (name != NULL) {
			string s(name);
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
	return false;
}

/*! Get the name of the joystick, presumably as set by the joystick driver. 
The name may not be very meaningful. */
std::string CX_Joystick::getJoystickName (void) {
	return _joystickName;
}

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

	if (axisCount == _axisPositions.size()) {
		for (unsigned int i = 0; i < axisCount; i++) {
			if (_axisPositions[i] != axes[i]) {
				CX_Joystick::Event ev;

				ev.type = CX_Joystick::EventType::AXIS_POSITION_CHANGE;
				ev.axisIndex = i;
				ev.axisPosition = axes[i];

				ev.time = pollTime;
				ev.uncertainty = ev.time - _lastEventPollTime;

				_joystickEvents.push_back(ev);

				_axisPositions[i] = axes[i];
			}
		}
	}
	
	if (buttonCount == _buttonStates.size()) {
		for (unsigned int i = 0; i < buttonCount; i++) {
			if (_buttonStates[i] != buttons[i]) {
				CX_Joystick::Event ev;

				//I'm just guessing about button state here. 1 might be PRESSED, but it could also be UNDEFINED_BUTTON.
				if (buttons[i] == 1) {
					ev.type = CX_Joystick::EventType::BUTTON_PRESS;
				} else {
					ev.type = CX_Joystick::EventType::BUTTON_RELEASE;
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

/*! Get the number of new events available for this input device. */
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
\note This function only clears already existing events from the device, which means that
responses made between a call to CX_InputManager::pollEvents() and a subsequent call to
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

/*! This function is to be used for direct access to the axis positions of the joystick. It does not 
generate events (i.e. CX_Joystick::Event), nor does it do any timestamping. If timestamps and 
uncertainies are desired, you MUST use pollEvents() and the associated event functions (e.g. getNextEvent()). */
vector<float> CX_Joystick::getAxisPositions (void) {
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
}

/*! This function is to be used for direct access to the button states of the joystick. It does not 
generate events (i.e. CX_Joystick::Event), nor does it do any timestamping. If timestamps and 
uncertainies are desired, you MUST use pollEvents() and the associated event functions (e.g. getNextEvent()). */
vector<unsigned char> CX_Joystick::getButtonStates (void) {
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
	case CX_Joystick::EventType::BUTTON_PRESS: ev.type = CX_Joystick::EventType::BUTTON_PRESS; break;
	case CX_Joystick::EventType::BUTTON_RELEASE: ev.type = CX_Joystick::EventType::BUTTON_RELEASE; break;
	case CX_Joystick::EventType::AXIS_POSITION_CHANGE: ev.type = CX_Joystick::EventType::AXIS_POSITION_CHANGE; break;
	}

	return is;
}

} //namespace CX