#ifndef _CX_KEYBOARD_H_
#define _CX_KEYBOARD_H_

#include <queue>
#include <set>

#include "CX_Clock.h"

#include "ofEvents.h"

namespace CX {

	/*
	struct CX_KeyboardModifiers_t {
		CX_KeyboardModifiers_t (void) :
			shift(0),
			ctrl(0),
			alt(0),
			meta(0)
		{}
		int shift;
		int ctrl;
		int alt;
		int meta;
	};
	*/

	struct CX_KeyEvent_t {
		int key;

		CX_Micros_t eventTime;
		CX_Micros_t uncertainty;

		enum {
			PRESSED,
			RELEASED,
			REPEAT
		} eventType;

		//Held modifiers?
	};

	class CX_Keyboard {
	public:
	
		CX_Keyboard (void);
		~CX_Keyboard (void);

		int availableEvents (void);
		CX_KeyEvent_t getNextEvent (void);
		void clearEvents (void);

	private:
		friend class CX_InputManager; //So that CX_InputManager can set _lastEventPollTime
		CX_Micros_t _lastEventPollTime;

		std::queue<CX_KeyEvent_t> _keyEvents;
		std::set<int> _heldKeys;

		void _keyPressHandler (ofKeyEventArgs &a);
		void _keyReleaseHandler (ofKeyEventArgs &a);
		void _keyEventHandler (ofKeyEventArgs &a);

		//CX_KeyboardModifiers_t _heldModifiers;

		void _listenForEvents (bool listen);
		bool _listeningForEvents;

	};

}

#endif //_CX_KEYBOARD_H_