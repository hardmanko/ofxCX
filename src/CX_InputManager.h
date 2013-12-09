#ifndef _CX_INPUT_MANAGER_H_
#define _CX_INPUT_MANAGER_H_

#include <set>
#include <queue>

#include "ofEvents.h"

#include "CX_Clock.h"

#include "CX_Keyboard.h"
#include "CX_Mouse.h"
#include "CX_Joystick.h"

namespace CX {

	class CX_InputManager {
	public:

		CX_InputManager (void);

		bool setup (bool useKeyboard, bool useMouse, int joystickIndex = -1);

		bool pollEvents (void);

		CX_Keyboard Keyboard;
		CX_Mouse Mouse;
		CX_Joystick Joystick;

	private:
		bool _usingKeyboard;
		bool _usingMouse;
		bool _usingJoystick;
	};

}

#endif //_CX_INPUT_MANAGER_H_