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


	/*! This struct contains the results of a keyboard event, whether it be a key press or release, or key repeat.
	\ingroup inputDevices
	*/
	struct CX_KeyEvent_t {
		int key; /*!< The key involved in this event. The value of this can be compared with chars for many keys
				 (e.g. (myKeyEvent.key == 'e') to test if the key was the E key. For special keys, this can be compared
				 with values defined in ofConstants.h (e.g. OF_KEY_ESC). */

		CX_Micros eventTime; //!< The time at which the event was registered. Can be compared to the result of CX::Clock::getTime().
		CX_Micros uncertainty; //!< The uncertainty in eventTime. The event occured some time between eventTime and eventTime minus uncertainty.

		enum KeyboardEventType {
			PRESSED, //!< A key has been pressed.
			RELEASED, //!< A key has been released.
			REPEAT //!< A key has been held for some time and automatic key repeat has kicked in, causing multiple keypresses to be rapidly sent.
		} eventType; //!< The type of the event.

		//Held modifiers?
	};

	std::ostream& operator<< (std::ostream& os, const CX_KeyEvent_t& ev);
	std::istream& operator>> (std::istream& is, CX_KeyEvent_t& ev);

	/*!  
	//This class is responsible for managing the mouse.
	\ingroup inputDevices
	*/
	class CX_Keyboard {
	public:
	
		CX_Keyboard (void);
		~CX_Keyboard (void);

		int availableEvents (void);
		CX_KeyEvent_t getNextEvent (void);
		void clearEvents (void);

	private:
		friend class CX_InputManager; //So that CX_InputManager can set _lastEventPollTime
		CX_Micros _lastEventPollTime;

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