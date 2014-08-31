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

	/*! This class is responsible for managing three basic input devices: a keyboard, mouse, and joystick. 
	You access each of these devices with the corresponding member class: Keyboard, Mouse, and Joystick. 
	See \ref CX::CX_Keyboard, \ref CX::CX_Mouse, and \ref CX::CX_Joystick for more information about each
	specific device.

	By default, all three input devices are disabled. Call \ref setup() to enable specific devices.

	This class has a private constructor because you should never need more than one of them. If you really,
	really need more than one, you can use CX::Private::inputManagerFactory() to make one.
	
	\ingroup inputDevices
	*/
	class CX_InputManager {
	public:

		bool setup (bool useKeyboard, bool useMouse, int joystickIndex = -1);
		bool usingKeyboard(void);
		bool usingMouse(void);
		int joystickIndex(void);

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
		extern CX_InputManager Input;
	}
}
