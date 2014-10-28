#pragma once

#include <deque>
#include <set>

#include "CX_Clock.h"

#include "ofEvents.h"
#include "CX_Events.h"

namespace CX {

    class CX_InputManager;

	/*! This class is responsible for managing the keyboard. You should not need to create an instance of this class:
	use the instance of CX_Keyboard within CX::Instances::Input instead.
	\ingroup inputDevices */
	class CX_Keyboard {
	public:

		/*! The type of the keyboard event. */
		enum EventType {
			PRESSED, //!< A key has been pressed.
			RELEASED, //!< A key has been released.
			REPEAT /*!< \brief A key has been held for some time and automatic key repeat has kicked in, causing
				   multiple keypresses to be rapidly sent. This event is one of the many repeats. */
		};


		/*! This struct contains the results of a keyboard event, whether it be a key press or release, or key repeat. */
		struct Event {
			/*! The key involved in this event. The value of this can be compared with character literals for many of
			the standard keyboard keys. For example, you could use (myKeyEvent.key == 'e') to test if the key was the E key.
			Always check for lower case letters, because shift/capslock are ignored when setting the value for ::key.

			For special keys, `key` can be compared with the key constant values defined in ofConstants.h (e.g. `OF_KEY_ESC`).

			Note that the modifier keys (shift, ctrl, alt, and super) are treated a little unusually. For
			those keys, you can check for a specific key using, for example, the constants defined in "ofConstants.h" of
			`OF_KEY_RIGHT_CONTROL` or `OF_KEY_LEFT_CONTROL`.
			However, you can alternately check to see if `key` is either of the control keys by performing a bitwise AND (`&`) with
			`OF_KEY_CONTROL` and checking that the result of the AND is still `OF_KEY_CONTROL`. For example:
			\code{.cpp}
			if ((myKeyEvent.key & OF_KEY_CONTROL) == OF_KEY_CONTROL) {
			//...
			}
			\endcode
			This works the same way for all of the modifier keys. */
			int key;

			CX_Millis time; //!< The time at which the event was registered. Can be compared to the result of CX::CX_Clock::now().
			CX_Millis uncertainty; //!< The uncertainty in `time`. The event occured some time between `time` and `time` minus uncertainty.

			EventType type; //!< The type of the event: press, release, or key repeat.
		};

		~CX_Keyboard (void);

		void enable(bool enable);
		bool enabled(void);

		int availableEvents (void) const;
		CX_Keyboard::Event getNextEvent (void);
		void clearEvents (void);
		std::vector<CX_Keyboard::Event> copyEvents(void);

		bool isKeyHeld(int key) const;

		CX_Keyboard::Event waitForKeypress(int key, bool clear = true, bool eraseEvent = false);
		CX_Keyboard::Event waitForKeypress(std::vector<int> keys, bool clear = true, bool eraseEvent = false);

		void setExitChord(std::vector<int> chord);
		bool isChordHeld(const std::vector<int>& chord) const;

	private:
		friend class CX_InputManager;

		CX_Keyboard(CX_InputManager* owner);
		CX_InputManager *_owner;
		bool _enabled;
		CX_Millis _lastEventPollTime;

		std::deque<CX_Keyboard::Event> _keyEvents;
		std::set<int> _heldKeys;

		void _keyPressHandler (ofKeyEventArgs &a);
		void _keyReleaseHandler (ofKeyEventArgs &a);
		void _keyRepeatHandler (CX::Private::CX_KeyRepeatEventArgs_t &a);
		void _keyEventHandler(CX_Keyboard::Event &a);

		void _listenForEvents (bool listen);
		bool _listeningForEvents;

		std::vector<int> _exitChord;

	};

	std::ostream& operator<< (std::ostream& os, const CX_Keyboard::Event& ev);
	std::istream& operator>> (std::istream& is, CX_Keyboard::Event& ev);
}
