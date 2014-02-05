#ifndef _CX_INPUT_MANAGER_H_
#define _CX_INPUT_MANAGER_H_

#include <set>
#include <queue>

#include "ofEvents.h"

#include "CX_Clock.h"

#include "CX_Keyboard.h"
#include "CX_Mouse.h"
#include "CX_Joystick.h"

/*! \defgroup input Input
There are a number of different classes that together perform the input handling functions of CX. For interfacing with serial ports, use ofSerial 
(http://www.openframeworks.cc/documentation/communication/ofSerial.html).

\sa CX::CX_InputManager for the primary interface to input devices.
\sa CX::CX_Keyboard for keyboard specific information.
\sa CX::CX_Mouse for mouse specific information.
\sa CX::CX_Joystick for joystick specific information.
*/

namespace CX {

	/*! This class is responsible for managing three basic input devices: a keyboard, mouse, and joystick. 
	You access each of these devices with the corresponding member class: Keyboard, Mouse, and Joystick. 
	See \ref CX::CX_Keyboard, \ref CX::CX_Mouse, and \ref CX::CX_Joystick for more information about each
	specific device.

	By default, all three input devices are disabled. Call \ref setup() to enable specific devices.
	
	\ingroup input
	*/
	class CX_InputManager {
	public:

		CX_InputManager (void);

		bool setup (bool useKeyboard, bool useMouse, int joystickIndex = -1);

		bool pollEvents (void);

		CX_Keyboard Keyboard; //!< An instance of CX::CX_Keyboard. Configured using CX::CX_InputManager::setup().
		CX_Mouse Mouse; //!< An instance of CX::CX_Mouse. Configured using CX::CX_InputManager::setup().
		CX_Joystick Joystick; //!< An instance of CX::CX_Joystick. Configured using CX::CX_InputManager::setup().

	private:
		bool _usingKeyboard;
		bool _usingMouse;
		bool _usingJoystick;
	};

}

#endif //_CX_INPUT_MANAGER_H_