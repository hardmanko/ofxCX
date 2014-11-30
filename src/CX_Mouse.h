#pragma once

#include <deque>
#include <set>

#include "CX_Clock.h"
#include "CX_Events.h"
#include "CX_Utilities.h"

#include "ofEvents.h"

namespace CX {

    class CX_InputManager;

	/*! This class is responsible for managing the mouse. You should not need to create an instance of this class:
	use the instance of CX_Mouse within CX::Instances::Input instead.
	\ingroup inputDevices */
	class CX_Mouse {
	public:

		/*! Names of the mouse buttons corresponding to some of the integer button identifiers. */
		enum Buttons {
			LEFT = OF_MOUSE_BUTTON_LEFT,
			MIDDLE = OF_MOUSE_BUTTON_MIDDLE,
			RIGHT = OF_MOUSE_BUTTON_RIGHT
		};

		/*! The type event that caused the creation of a CX_Mouse::Event. */
		enum EventType {
			MOVED, //!< The mouse has been moved without a button being held. Event::button should be -1 (meaningless).
			PRESSED, //!< A mouse button has been pressed. Check Event::button for the button index and Event::x and Event::y for the location.
			RELEASED, //!< A mouse button has been released. Check Event::button for the button index and Event::x and Event::y for the location.
			DRAGGED, //!< The mouse has been moved while at least one button was held. Event::button may not be meaningful because the held button
			//!< can be changed during a drag, or multiple buttons may be held at once during a drag.
			SCROLLED //!< The mouse wheel has been scrolled. Check Event::y to get the change in the standard mouse wheel direction, or 
				//!< Event::x if your mouse has a wheel that can move horizontally.
		};

		/*! This struct contains the results of a mouse event, which is any type of interaction with the mouse, be it
		simply movement, a button press or release, a drag event (mouse button held while mouse is moved), or movement
		of the scroll wheel. */
		struct Event {
			/*! \brief The relevant mouse button if the event `type` is PRESSED, RELEASED, or DRAGGED.
			Can be compared with elements of enum CX_Mouse::Buttons to find out about the named buttons. */
			int button;

			/*! \brief The x position of the cursor at the time of the event, or the change in 
			the x-axis scroll if the `type` is EventType::SCROLLED. */
			float x;

			/*! \brief The y position of the cursor at the time of the event, or the change in 
			the y-axis scroll if the `type` is EventType::SCROLLED. */
			float y;

			CX_Millis time; //!< The time at which the event was registered. Can be compared to the result of CX::Clock::now().

			/*! \brief The uncertainty in `time`, which represents the difference between the time at which this
			event was timestamped by CX and the last time that events were checked for. */
			CX_Millis uncertainty;

			EventType type; //!< The type of the event.
		};

		~CX_Mouse (void);

		void enable(bool enable);
		bool enabled(void);

		int availableEvents(void);
		CX_Mouse::Event getNextEvent(void);
		std::vector<CX_Mouse::Event> copyEvents(void);
		void clearEvents(void);

		void showCursor(bool show);
		void setCursorPosition(ofPoint pos);
		ofPoint getCursorPosition(void);

	private:
		friend class CX_InputManager; //So that CX_InputManager can set _lastEventPollTime

		CX_Mouse(CX_InputManager* owner);
		CX_InputManager* _owner;
		bool _enabled;
		CX_Millis _lastEventPollTime;

		std::set<int> _heldMouseButtons;
		std::deque<CX_Mouse::Event> _mouseEvents;

		void _mouseButtonPressedEventHandler (ofMouseEventArgs &a);
		void _mouseButtonReleasedEventHandler (ofMouseEventArgs &a);
		void _mouseMovedEventHandler (ofMouseEventArgs &a);
		void _mouseDraggedEventHandler (ofMouseEventArgs &a);
		void _mouseWheelScrollHandler (Private::CX_MouseScrollEventArgs_t &a);

		void _mouseEventHandler(ofMouseEventArgs& ofEvent);

		bool _listeningForEvents;
		void _listenForEvents (bool listen);

		ofPoint _cursorPos;

	};

	std::ostream& operator<< (std::ostream& os, const CX_Mouse::Event& ev);
	std::istream& operator>> (std::istream& is, CX_Mouse::Event& ev);
}
