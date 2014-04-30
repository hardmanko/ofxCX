#include "CX_InputManager.h"

#include "CX_AppWindow.h" //glfwPollEvents()

using namespace CX;

CX_InputManager::CX_InputManager (void) :
	_usingKeyboard(false),
	_usingMouse(false),
	_usingJoystick(false)
{
}

/*!
Setup the input manager to use the requested devices. You may call this function multiple times if you want to
change the configuration over the course of the experiment. Every time this function is called, all input device
events are cleared.
\param useKeyboard Enable or disable the keyboard.
\param useMouse Enable or disable the mouse.
\param joystickIndex Optional. If >= 0, an attempt will be made to set up the joystick at that index. If < 0, no attempt will 
be made to set up the joystick.
\return False if the requested joystick could not be set up correctly, true otherwise.
*/
bool CX_InputManager::setup (bool useKeyboard, bool useMouse, int joystickIndex) {

	pollEvents(); //Flush out all waiting events during setup.

	Keyboard.clearEvents();
	_usingKeyboard = useKeyboard;
	Keyboard._listenForEvents(useKeyboard);
	
	Mouse.clearEvents();
	_usingMouse = useMouse;
	Mouse._listenForEvents(useMouse);

	bool success = true;
	if (joystickIndex >= 0) {
		Joystick.clearEvents();
		success = success && Joystick.setup(joystickIndex);
		if (success) {
			_usingJoystick = true;
		}
	}
	return success;
}

/*!
It is not typically neccessary for the user to call this function directly, although there is no harm in doing so.
This function polls for new events on all of the configured input devices (see setup()). After a call to this function,
new events for the input devices can be found by checking the availableEvents() function for each device.
\return True if there are any events available for enabled devices, false otherwise. The events do not neccessarily 
need to be new events. If there are events that were already stored in Mouse, Keyboard, or Joystick but had not been 
processed by user code, this function will return true.
*/
bool CX_InputManager::pollEvents (void) {

	glfwPollEvents();
	CX_Millis pollCompleteTime = CX::Instances::Clock.now();

	if (_usingJoystick) {
		Joystick.pollEvents();
	}

	if (_usingMouse) {
		Mouse._lastEventPollTime = pollCompleteTime;
	} else {
		Mouse.clearEvents();
	}

	if (_usingKeyboard) {
		Keyboard._lastEventPollTime = pollCompleteTime;
	} else {
		Keyboard.clearEvents();
	}

	if (Mouse.availableEvents() > 0 || Keyboard.availableEvents() > 0 || Joystick.availableEvents() > 0) {
		return true;
	}
	return false;
}