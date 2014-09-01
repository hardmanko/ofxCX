#pragma once

#include <set>
#include <queue>

#include "ofEvents.h"

#include "CX_Clock.h"

#include "CX_Keyboard.h"
#include "CX_Mouse.h"
#include "CX_Joystick.h"

/*! \defgroup inputDevices Input Devices
There are a number of different classes that together perform the input handling functions of CX. Start by looking at CX::CX_InputManager
and the instance of that class that is created for you: CX::Instances::Input.

For interfacing with serial ports, use ofSerial 
(http://www.openframeworks.cc/documentation/communication/ofSerial.html).

\sa CX::CX_InputManager for the primary interface to input devices.
\sa CX::CX_Keyboard for keyboard specific information.
\sa CX::CX_Mouse for mouse specific information.
\sa CX::CX_Joystick for joystick specific information.
*/

namespace CX {

	namespace Private {
		CX_InputManager inputManagerFactory(void);
	}

	/*! This class is responsible for managing three basic input devices: the keyboard, mouse, and, if available, joystick. 
	You access each of these devices with the corresponding class member: Keyboard, Mouse, and Joystick. 
	See \ref CX::CX_Keyboard, \ref CX::CX_Mouse, and \ref CX::CX_Joystick for more information about each
	specific device.

	By default, all three input devices are disabled. Call \ref setup() to enable specific devices.
	Alternately, you can call CX_Mouse::enable() or CX_Keyboard::enable(), if that makes more sense to you.

	The overall structure of input in CX revolves around polling for new input, with CX_InputManager::pollEvents().
	This is the only way to get new input events for the keyboard and mouse. When pollEvents() is called, CX
	checks to see if any keyboard or mouse input has been given since the last time pollEvents() was called.
	If there are new events, they are put into input device specific queues. You can find out how many input
	events are available in, for example, the keyboard queue by calling `CX_Keyboard::availableEvents()`. If
	there are any available events, you can the first one with CX_Keyboard::getNextEvent(). 
	CX_Keyboard::getNextEvent() returns a CX_Keyboard::Event struct that contains information about the event.
	This all works the same way for the mouse.

	\code{.cpp}
	//Basic sequence of events:
	if (Input.pollEvents()) { //Returns true if there are any events available on any input devices.
		while (Input.Keyboard.availableEvents()) { //For each new event
			CX_Keyboard::Event keyEvent = Input.Keyboard.getNextEvent(); //Pop that event out of the queue
			if (keyEvent.type == CX_Keyboard::PRESSED) { //If a key has been pressed
				//Process the keypress...
				if (keyEvent.key == 'a') {
					//do something...
				}
			}
		}
	}
	\endcode

	If the timing of input is critical for you application, you should poll for input regularly, because 
	the quality of input timestamps is based on the regularity of polling.

	This class has a private constructor because you should never need more than one of them. If you really,
	really need more than one, you can use CX::Private::inputManagerFactory() to make one.
	
	\ingroup inputDevices
	*/
	class CX_InputManager {
	public:

		bool setup (bool useKeyboard, bool useMouse, int joystickIndex = -1);

		bool pollEvents (void);
		void clearAllEvents(bool poll = false);

		CX_Keyboard Keyboard; //!< An instance of CX::CX_Keyboard. Enabled or disabled with CX::CX_InputManager::setup().
		CX_Mouse Mouse; //!< An instance of CX::CX_Mouse. Enabled or disabled with CX::CX_InputManager::setup().
		CX_Joystick Joystick; //!< An instance of CX::CX_Joystick. Enabled or disabled with CX::CX_InputManager::setup().

	private:
		friend CX_InputManager Private::inputManagerFactory(void);
		CX_InputManager(void);

		bool _usingJoystick;
	};

	namespace Instances {
		/*! An instance of CX_InputManager that is exceedingly lightly hooked into the CX backend. The only way in which
		this is used that is not in user code, is that input events are polled for once during setup, which helps operating
		systems know that the program is still responding.
		\ingroup entryPoint */
		extern CX_InputManager Input;
	}
}
