#include "CX_InputManager.h"

#include "ofAppGLFWWindow.h" //glfwPollEvents()

using namespace CX;

CX_InputManager::CX_InputManager (void) :
	_usingKeyboard(false),
	_usingMouse(false),
	_usingJoystick(false)
{
}

bool CX_InputManager::setup (bool useKeyboard, bool useMouse, int joystickIndex) {
	Keyboard.clearEvents();
	_usingKeyboard = useKeyboard;
	Keyboard._listenForEvents(useKeyboard);
	
	Mouse.clearEvents();
	_usingMouse = useMouse;
	Mouse._listenForEvents(useMouse);

	bool success = true;
	if (joystickIndex != -1) {
		Joystick.clearEvents();
		success = success && Joystick.setup(joystickIndex);
		if (success) {
			_usingJoystick = true;
		}
	}
	return success;
}


bool CX_InputManager::pollEvents (void) {
	if (_usingKeyboard || _usingMouse) {
		glfwPollEvents();

		uint64_t pollCompleteTime = CX::Instances::Clock.getTime();

		Keyboard._lastEventPollTime = pollCompleteTime;
		Mouse._lastEventPollTime = pollCompleteTime;
	}

	if (_usingJoystick) {
		Joystick.pollEvents();
	}

	if (Mouse.availableEvents() > 0 || Keyboard.availableEvents() > 0 || Joystick.availableEvents() > 0) {
		return true;
	}
	return false;
}

