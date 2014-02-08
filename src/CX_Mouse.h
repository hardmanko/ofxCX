#pragma once

#include <queue>
#include <set>

#include "CX_Clock.h"
#include "CX_Events.h"
#include "CX_Utilities.h"

#include "ofEvents.h"

namespace CX {

	enum class CX_MouseButtons : int {
		LEFT_MOUSE = OF_MOUSE_BUTTON_LEFT,
		MIDDLE_MOUSE = OF_MOUSE_BUTTON_MIDDLE,
		RIGHT_MOUSE = OF_MOUSE_BUTTON_RIGHT
	};

	/*! This struct contains the results of a mouse event, which is any type of interaction with the mouse, be it
	simply movement, a button press or release, a drag event (mouse button held while mouse is moved), or movement 
	of the scroll wheel. 
	\ingroup inputDevices */
	struct CX_MouseEvent_t {
		int button; /*!< \brief The relevant mouse button if the eventType is PRESSED, RELEASED, or DRAGGED. 
						Can be compared with elements of enum CX_MouseButtons to find out about the primary buttons. */

		int x; //!< The x position of the cursor at the time of the event, or the change in the x-axis scroll if the eventType is SCROLLED.
		int y; //!< The y position of the cursor at the time of the event, or the change in the y-axis scroll if the eventType is SCROLLED.

		//ofPoint cursor;

		//struct {
		//	double x;
		//	double y;
		//} scroll;

		CX_Micros eventTime; //!< The time at which the event was registered. Can be compared to the result of CX::Clock::getTime().
		CX_Micros uncertainty; //!< The uncertainty in eventTime. The event occured some time between eventTime and eventTime minus uncertainty.

		enum MouseEventType {
			MOVED, //!< The mouse has been moved without a button being held. \ref button should be -1 (meaningless).
			PRESSED, //!< A mouse button has been pressed. Check \ref button for the button index and \ref x and \ref y for the location.
			RELEASED, //!< A mouse button has been released. Check \ref button for the button index and \ref x and \ref y for the location.
			DRAGGED, //!< The mouse has been moved while at least one button was held. \ref button may not be meaningful because the held button 
					 //!< can be changed during a drag, or multiple buttons may be held at once during a drag.
			SCROLLED //!< The mouse wheel has been scrolled. Check \ref y to get the change in the standard mouse wheel, or \ref x if your 
					 //!< mouse has a wheel that can move horizontally.
		} eventType; //!< The type of the event.

	};

	std::ostream& operator<< (std::ostream& os, const CX_MouseEvent_t& ev);
	std::istream& operator>> (std::istream& is, CX_MouseEvent_t& ev);

	/*! This class is responsible for managing the mouse. 
	
	\ingroup inputDevices
	*/
	class CX_Mouse {
	public:
		CX_Mouse (void);
		~CX_Mouse (void);

		int availableEvents (void);
		CX_MouseEvent_t getNextEvent (void);
		void clearEvents (void);

		void showCursor (bool show);
		void setCursorPosition (ofPoint pos);
		ofPoint getCursorPosition (void);

	private:
		friend class CX_InputManager; //So that CX_InputManager can set _lastEventPollTime
		CX_Micros _lastEventPollTime;

		std::set<int> _heldMouseButtons;
		std::queue<CX_MouseEvent_t> _mouseEvents;

		void _mouseButtonPressedEventHandler (ofMouseEventArgs &a);
		void _mouseButtonReleasedEventHandler (ofMouseEventArgs &a);
		void _mouseMovedEventHandler (ofMouseEventArgs &a);
		void _mouseDraggedEventHandler (ofMouseEventArgs &a);
		void _mouseWheelScrollHandler (Private::CX_MouseScrollEventArgs_t &a);

		void _mouseEventHandler (ofMouseEventArgs &a);

		bool _listeningForEvents;
		void _listenForEvents (bool listen);

	};
}