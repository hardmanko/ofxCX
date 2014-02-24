#ifndef _CX_KEYBOARD_H_
#define _CX_KEYBOARD_H_

#include <queue>
#include <set>

#include "CX_Clock.h"

#include "ofEvents.h"
#include "CX_Events.h"

namespace CX {

	/*! This struct contains the results of a keyboard event, whether it be a key press or release, or key repeat.
	\ingroup inputDevices
	*/
	struct CX_KeyEvent_t {
		int key; /*!< The key involved in this event. The value of this can be compared with character literals for many of
				 the standard keyboard keys. For example, you could use `(myKeyEvent.key == 'e')` to test if the key was the E key. 
				 Note that if shift is held when the key is pressed, the letter will be uppercase, so you may want to check for both 'e' and 'E'.

				 For special keys, `key` can be compared with the key constant values defined in ofConstants.h (e.g. `OF_KEY_ESC`).

				 Note that for the modifier keys (shift, ctrl, alt, and super) are treated a little unusually. For
				 those keys, you can check for a specific key using, for example, `OF_KEY_RIGHT_CONTROL` or `OF_KEY_LEFT_CONTROL`.
				 However, you can alternately check to see if `key` is any of the control keys by performing a bitwise AND with
				 `OF_KEY_CONTROL` and checking that the result of the AND is still `OF_KEY_CONTROL`. For example:
				 \code{.cpp}
				 if ((myKeyEvent.key & OF_KEY_CONTROL) == OF_KEY_CONTROL) {
					//...
				 }
				 \endcode 
				 This works the same way for all of the modifier keys.
				 */

		CX_Micros eventTime; //!< The time at which the event was registered. Can be compared to the result of CX::Clock::getTime().
		CX_Micros uncertainty; //!< The uncertainty in eventTime. The event occured some time between eventTime and eventTime minus uncertainty.

		enum KeyboardEventType {
			PRESSED, //!< A key has been pressed.
			RELEASED, //!< A key has been released.
			REPEAT /*!< \brief A key has been held for some time and automatic key repeat has kicked in, causing 
				multiple keypresses to be rapidly sent. This event is one of the many repeats. */
		} eventType; //!< The type of the event.
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

		/*! This function checks to see if the given key is pressed.
		\param key The key code for key you are interested in. See the documentation for the `key` member of CX_KeyEvent_t 
		for more information about this value.
		\return True if the given key is held.
		*/
		bool isKeyPressed(int key) {
			return ofGetKeyPressed(key);
		}

	private:
		friend class CX_InputManager; //So that CX_InputManager can set _lastEventPollTime
		CX_Micros _lastEventPollTime;

		std::queue<CX_KeyEvent_t> _keyEvents;
		std::set<int> _heldKeys;

		void _keyPressHandler (ofKeyEventArgs &a);
		void _keyReleaseHandler (ofKeyEventArgs &a);
		void _keyRepeatHandler (CX::Private::CX_KeyRepeatEventArgs_t &a);
		void _keyEventHandler(CX_KeyEvent_t &a);

		//CX_KeyboardModifiers_t _heldModifiers;

		void _listenForEvents (bool listen);
		bool _listeningForEvents;

	};

}

#endif //_CX_KEYBOARD_H_