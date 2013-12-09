#ifndef _CX_MOUSE_H_
#define _CX_MOUSE_H_

#include <queue>
#include <set>

#include "CX_Clock.h"

#include "ofEvents.h"

namespace CX {

	enum class CX_NamedMouseButtons : int {
		LEFT_MOUSE = OF_MOUSE_BUTTON_LEFT,
		MIDDLE_MOUSE = OF_MOUSE_BUTTON_MIDDLE,
		RIGHT_MOUSE = OF_MOUSE_BUTTON_RIGHT
	};

	struct CX_MouseEvent_t {
		int button; //Can be compared with elements of enum CX_NamedMouseButtons to find out about the main buttons.

		int x;
		int y;

		uint64_t eventTime;
		uint64_t uncertainty;

		enum {
			MOVED,
			PRESSED,
			RELEASED,
			DRAGGED, //I'm not sure if this is something that this library should implement for itself or just use OF's version.
			SCROLLED //i.e. the scroll wheel, although it may be just treated as another button. OF will have to have the proper 
				//event added in order for this to work. GLFW has a specific callback for scroll that isn't implemented by OF.
		} eventType;
	};

	class CX_Mouse {
	public:
		CX_Mouse (void);
		~CX_Mouse (void);

		int availableEvents (void);
		CX_MouseEvent_t popEvent (void);
		void clearEvents (void);

		void showCursor (bool show);

	private:
		friend class CX_InputManager; //So that CX_InputManager can set _lastEventPollTime
		uint64_t _lastEventPollTime;

		set<int> _heldMouseButtons;
		queue<CX_MouseEvent_t> _mouseEvents;

		void _mouseButtonPressedEventHandler (ofMouseEventArgs &a);
		void _mouseButtonReleasedEventHandler (ofMouseEventArgs &a);
		void _mouseMovedEventHandler (ofMouseEventArgs &a);
		void _mouseDraggedEventHandler (ofMouseEventArgs &a);

		void _mouseEventHandler (ofMouseEventArgs &a);

		bool _listeningForEvents;
		void _listenForEvents (bool listen);
	};

}

#endif //_CX_MOUSE_H_