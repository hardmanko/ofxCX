#include "CX_InputManager.h"

namespace CX {

	namespace Private {
		CX_InputManager inputManagerFactory(void) {
			return CX_InputManager();
		}
	}

	CX::CX_InputManager CX::Instances::Input = CX::Private::inputManagerFactory();

	CX_InputManager::CX_InputManager(void) :
		Keyboard(this),
		Mouse(this),
		_usingJoystick(false)
	{
	}

	/*!
	Set up the input manager to use the requested devices. You may call this function multiple times if you want to
	change the configuration over the course of the experiment. Every time this function is called, all input device
	events are cleared.
	\param useKeyboard Enable or disable the keyboard.
	\param useMouse Enable or disable the mouse.
	\param joystickIndex Optional. If `joystickIndex >= 0`, an attempt will be made to set up the joystick at that index. If `joystickIndex < 0`, no attempt will
	be made to set up the joystick and the joystick will be disabled.
	\return `false` if the requested joystick could not be set up correctly, `true` otherwise.
	*/
	bool CX_InputManager::setup(bool useKeyboard, bool useMouse, int joystickIndex) {

		pollEvents(); //Flush out all waiting events during setup.

		Keyboard.clearEvents();
		Keyboard.enable(useKeyboard);

		Mouse.clearEvents();
		Mouse.enable(useMouse);

		bool success = true;
		if (joystickIndex >= 0) {
			Joystick.clearEvents();
			success = success && Joystick.setup(joystickIndex);
			_usingJoystick = success;
		} else {
			_usingJoystick = false;
		}
		return success;
	}

	/*!	This function polls for new events on all of the configured input devices (see CX_InputManager::setup()). 
	After a call to this function, new events for the input devices can be found by checking the availableEvents() 
	function for each device.
	\return `true` if there are any events available for enabled devices, `false` otherwise. Note that the events 
	do not neccessarily	need to be new events in order for this to return `true`. If there were events that were 
	already stored in Mouse, Keyboard, or Joystick that had not been processed by user code at the time this 
	function was called, this function will return `true`. */
	bool CX_InputManager::pollEvents(void) {

		// Notice what happens here: It is the main reason for the InputManager class.
		// Events are polled with glfwPollEvents(), which polls both keyboard and mouse events.
		// Once polling is complete, a timestamp is taken immediately.
		// That timestamp is then used to set the _lastEventPollTime private member of the Mouse
		// and Keyboard members so that they can have the correct timestamp for the poll time, which
		// they wouldn't have if they each took a poll time one after the other.
		// The joystick works differently: the GLFW helper functions simply reads
		// off the current axis and button values rather than creating events.
		glfwPollEvents();
		CX_Millis pollCompleteTime = CX::Instances::Clock.now();

		if (_usingJoystick) {
			Joystick.pollEvents();
		}

		if (Mouse.enabled()) {
			Mouse._lastEventPollTime = pollCompleteTime;
		} else {
			Mouse.clearEvents();
		}

		if (Keyboard.enabled()) {
			Keyboard._lastEventPollTime = pollCompleteTime;
		} else {
			Keyboard.clearEvents();
		}

		if (Mouse.availableEvents() > 0 || Keyboard.availableEvents() > 0 || Joystick.availableEvents() > 0) {
			return true;
		}
		return false;
	}

	/*! This function clears all events on all input devices.
	\param pollFirst If `true`, events are polled before they are cleared, so that events that hadn't yet
	made it into the device specific queues (e.g. the Keyboard queue) are cleared as well. */
	void CX_InputManager::clearAllEvents(bool pollFirst) {
		if (pollFirst) {
			pollEvents();
		}
		Keyboard.clearEvents();
		Mouse.clearEvents();
		Joystick.clearEvents();
	}

}
