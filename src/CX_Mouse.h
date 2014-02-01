#ifndef _CX_MOUSE_H_
#define _CX_MOUSE_H_

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

	struct CX_MouseEvent_t {
		int button; //Can be compared with elements of enum CX_MouseButtons to find out about the primary buttons.

		int x;
		int y;

		CX_Micros eventTime;
		CX_Micros uncertainty;

		enum {
			MOVED,
			PRESSED,
			RELEASED,
			DRAGGED, //I'm not sure if this is something that this library should implement for itself or just use OF's version.
			SCROLLED //i.e. the scroll wheel, although it may be just treated as another button. oF will have to have the proper 
				//event added in order for this to work. GLFW has a specific callback for scroll that isn't implemented by OF.
		} eventType;
	};



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
		void _mouseWheelScrollHandler (CX_MouseScrollEventArgs_t &a);

		void _mouseEventHandler (ofMouseEventArgs &a);

		bool _listeningForEvents;
		void _listenForEvents (bool listen);


	};

}

#endif //_CX_MOUSE_H_