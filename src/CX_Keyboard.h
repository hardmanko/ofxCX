#ifndef _CX_KEYBOARD_H_
#define _CX_KEYBOARD_H_

#include <queue>
#include <set>

#include "CX_Clock.h"

#include "ofEvents.h"

namespace CX {

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

	struct CX_KeyEvent_t {
		int key;

		uint64_t eventTime;
		uint64_t uncertainty;

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
		CX_KeyEvent_t popEvent (void);
		void clearEvents (void);

	private:
		friend class CX_InputManager; //So that CX_InputManager can set _lastEventPollTime
		uint64_t _lastEventPollTime;

		queue<CX_KeyEvent_t> _keyEvents;
		set<int> _heldKeys;

		void _keyPressHandler (ofKeyEventArgs &a);
		void _keyReleaseHandler (ofKeyEventArgs &a);
		void _keyEventHandler (ofKeyEventArgs &a);

		//CX_KeyboardModifiers_t _heldModifiers;

		void _listenForEvents (bool listen);
		bool _listeningForEvents;

	};

}

#endif //_CX_KEYBOARD_H_