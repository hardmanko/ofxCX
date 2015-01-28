#pragma once

#include <deque>
#include <set>

#include "ofEvents.h"

#include "CX_Clock.h"
#include "CX_Events.h"

#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"

namespace CX {

	namespace Keycode {
		enum {
			UNKNOWN = GLFW_KEY_UNKNOWN,
			SPACE = GLFW_KEY_SPACE,
			APOSTROPHE = GLFW_KEY_APOSTROPHE, //AKA single quote
			COMMA = GLFW_KEY_COMMA,
			MINUS = GLFW_KEY_MINUS,
			PERIOD = GLFW_KEY_PERIOD,
			SLASH = GLFW_KEY_SLASH, //AKA forward slash AKA question mark
			NR_0 = GLFW_KEY_0, //Number row. Should be equal to character literals, e.g., '1'.
			NR_1 = GLFW_KEY_1,
			NR_2 = GLFW_KEY_2,
			NR_3 = GLFW_KEY_3,
			NR_4 = GLFW_KEY_4,
			NR_5 = GLFW_KEY_5,
			NR_6 = GLFW_KEY_6,
			NR_7 = GLFW_KEY_7,
			NR_8 = GLFW_KEY_8,
			NR_9 = GLFW_KEY_9,
			SEMICOLON = GLFW_KEY_SEMICOLON,
			EQUAL = GLFW_KEY_EQUAL,
			A = GLFW_KEY_A, //Standard letters. Should be equal to uppercase character literals, e.g. 'A'.
			B = GLFW_KEY_B,
			C = GLFW_KEY_C,
			D = GLFW_KEY_D,
			E = GLFW_KEY_E,
			F = GLFW_KEY_F,
			G = GLFW_KEY_G,
			H = GLFW_KEY_H,
			I = GLFW_KEY_I,
			J = GLFW_KEY_J,
			K = GLFW_KEY_K,
			L = GLFW_KEY_L,
			M = GLFW_KEY_M,
			N = GLFW_KEY_N,
			O = GLFW_KEY_O,
			P = GLFW_KEY_P,
			Q = GLFW_KEY_Q,
			R = GLFW_KEY_R,
			S = GLFW_KEY_S,
			T = GLFW_KEY_T,
			U = GLFW_KEY_U,
			V = GLFW_KEY_V,
			W = GLFW_KEY_W,
			X = GLFW_KEY_X,
			Y = GLFW_KEY_Y,
			Z = GLFW_KEY_Z,
			LEFT_BRACKET = GLFW_KEY_LEFT_BRACKET, //AKA opening square bracket
			RIGHT_BRACKET = GLFW_KEY_RIGHT_BRACKET, //AKA closing square bracket
			BACKSLASH = GLFW_KEY_BACKSLASH,
			GRAVE_ACCENT = GLFW_KEY_GRAVE_ACCENT, //AKA tilde
			WORLD_1 = GLFW_KEY_WORLD_1,
			WORLD_2 = GLFW_KEY_WORLD_2,
			ESCAPE = GLFW_KEY_ESCAPE,
			ENTER = GLFW_KEY_ENTER,
			TAB = GLFW_KEY_TAB,
			BACKSPACE = GLFW_KEY_BACKSPACE,
			INSERT = GLFW_KEY_INSERT,
			DELETE_ = GLFW_KEY_DELETE,
			RIGHT_ARROW = GLFW_KEY_RIGHT,
			LEFT_ARROW = GLFW_KEY_LEFT,
			DOWN_ARROW = GLFW_KEY_DOWN,
			UP_ARROW = GLFW_KEY_UP,
			PAGE_UP = GLFW_KEY_PAGE_UP,
			PAGE_DOWN = GLFW_KEY_PAGE_DOWN,
			HOME = GLFW_KEY_HOME,
			END = GLFW_KEY_END,
			CAPS_LOCK = GLFW_KEY_CAPS_LOCK,
			SCROLL_LOCK = GLFW_KEY_SCROLL_LOCK,
			NUM_LOCK = GLFW_KEY_NUM_LOCK,
			PRINT_SCREEN = GLFW_KEY_PRINT_SCREEN,
			PAUSE = GLFW_KEY_PAUSE, //AKA break
			F1 = GLFW_KEY_F1,
			F2 = GLFW_KEY_F2,
			F3 = GLFW_KEY_F3,
			F4 = GLFW_KEY_F4,
			F5 = GLFW_KEY_F5,
			F6 = GLFW_KEY_F6,
			F7 = GLFW_KEY_F7,
			F8 = GLFW_KEY_F8,
			F9 = GLFW_KEY_F9,
			F10 = GLFW_KEY_F10,
			F11 = GLFW_KEY_F11,
			F12 = GLFW_KEY_F12,
			F13 = GLFW_KEY_F13,
			F14 = GLFW_KEY_F14,
			F15 = GLFW_KEY_F15,
			F16 = GLFW_KEY_F16,
			F17 = GLFW_KEY_F17,
			F18 = GLFW_KEY_F18,
			F19 = GLFW_KEY_F19,
			F20 = GLFW_KEY_F20,
			F21 = GLFW_KEY_F21,
			F22 = GLFW_KEY_F22,
			F23 = GLFW_KEY_F23,
			F24 = GLFW_KEY_F24,
			F25 = GLFW_KEY_F25,
			KP_0 = GLFW_KEY_KP_0, //KP == keypad AKA numpad
			KP_1 = GLFW_KEY_KP_1,
			KP_2 = GLFW_KEY_KP_2,
			KP_3 = GLFW_KEY_KP_3,
			KP_4 = GLFW_KEY_KP_4,
			KP_5 = GLFW_KEY_KP_5,
			KP_6 = GLFW_KEY_KP_6,
			KP_7 = GLFW_KEY_KP_7,
			KP_8 = GLFW_KEY_KP_8,
			KP_9 = GLFW_KEY_KP_9,
			KP_PERIOD = GLFW_KEY_KP_DECIMAL,
			KP_DIVIDE = GLFW_KEY_KP_DIVIDE,
			KP_MULTIPLY = GLFW_KEY_KP_MULTIPLY,
			KP_SUBTRACT = GLFW_KEY_KP_SUBTRACT,
			KP_ADD = GLFW_KEY_KP_ADD,
			KP_ENTER = GLFW_KEY_KP_ENTER,
			KP_EQUAL = GLFW_KEY_KP_EQUAL,
			LEFT_SHIFT = GLFW_KEY_LEFT_SHIFT,
			LEFT_CONTROL = GLFW_KEY_LEFT_CONTROL,
			LEFT_ALT = GLFW_KEY_LEFT_ALT,
			LEFT_SUPER = GLFW_KEY_LEFT_SUPER,
			RIGHT_SHIFT = GLFW_KEY_RIGHT_SHIFT,
			RIGHT_CONTROL = GLFW_KEY_RIGHT_CONTROL,
			RIGHT_ALT = GLFW_KEY_RIGHT_ALT,
			RIGHT_SUPER = GLFW_KEY_RIGHT_SUPER,
			MENU = GLFW_KEY_MENU
		};
	}



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

		/*! This struct contains four alternative representations of the pressed key.
		
		+ \ref oF The openFrameworks key representation. This depends on modifier keys.
		+ \ref glfw The GLFW keycode. This does not depend on modifier keys.
		+ \ref scancode System-specific scancode. This is not very easy to use, but does not depend on modifier keys.
		+ \ref codepoint The locale-specific unicode code point for the key. This depends on modifier keys.
		*/
		struct Keycodes {
			Keycodes(void) :
				oF(-1),
				glfw(-1),
				scancode(-1),
				codepoint(-1)
			{}

			/*! Fancy constructor for the struct. */
			Keycodes(int oF_, int glfw_, int scancode_, unsigned int codepoint_) :
				oF(oF_),
				glfw(glfw_),
				scancode(scancode_),
				codepoint(codepoint_)
			{}

			/*! The openFrameworks keycode.
			The value of this can be compared with character literals for many of the standard keyboard keys.
			The value depends on the modifier keys.

			For special keys, this can be compared with the key constant values defined in ofConstants.h (e.g. `OF_KEY_ESC`).

			For modifier keys, you can check for a specific key using, for example, the constants 
			`OF_KEY_RIGHT_CONTROL` or `OF_KEY_LEFT_CONTROL`.
			You can alternately check to see if this is either of the control keys by performing a bitwise AND (`&`) with
			`OF_KEY_CONTROL` and checking that the result of the AND is still `OF_KEY_CONTROL`. For example:
			\code{.cpp}
			bool ctrlHeld = (myKeyEvent.key & OF_KEY_CONTROL) == OF_KEY_CONTROL;
			\endcode
			This works the same way for all of the modifier keys. */
			int oF;

			/*! The GLFW keycode. These can be compared to the constants defined here:
			http://www.glfw.org/docs/latest/group__keys.html. This value does not depend on modifier keys.
			Like \ref oF, the value of this can be compared	with character literals for	a lot of the standard 
			keys (letters are uppercase). */
			int glfw;

			int scancode; //!< System-specific scancode. These are not very easy to use, but do not depend on modifier keys.

			/*! The locale-specific unicode codepoint for the key. This is the most like the natural 
			language value of the key, so it naturally depends on modifier keys. */
			unsigned int codepoint;
		};

		/*! This struct contains the results of a keyboard event, whether it be a key press or release, or key repeat.

		The primary representation of the key that was pressed is given by \ref key. Four alternative representations
		are given in the \ref codes struct.
		*/
		struct Event {

			/*! The key that was pressed. This can be compared with character literals for most standard
			keys. For example, you could use (myKeyEvent.key == 'E') to test if the key was the E key.
			This does not depend on modifier keys: You always check for uppercase letters. For the
			number row keys, you check for the number, not the special character that is produced when
			shift is held, etc.

			For special keys, this value can be compared to the values in the CX::Keycode enum.
			*/
			int key;

			CX_Millis time; //!< The time at which the event was registered. Can be compared to the result of CX::CX_Clock::now().

			/*! \brief The uncertainty in `time`, which represents the difference between the time at which this
			event was timestamped by CX and the last time that events were checked for. */
			CX_Millis uncertainty;

			EventType type; //!< The type of the event: press, release, or key repeat.

			Keycodes codes; //!< Alternative representations of the pressed key.
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
