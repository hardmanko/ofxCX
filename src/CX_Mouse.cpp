#include "CX_Mouse.h"

#include "ofAppRunner.h" //ofShowCursor()/ofHideCursor()
#include "GLFW\glfw3.h"

using namespace CX;

CX_Mouse::CX_Mouse (void) :
	_listeningForEvents(false)
{
}

CX_Mouse::~CX_Mouse (void) {
	_listenForEvents(false);
}

/*! Get the number of new events available for this input device. */
int CX_Mouse::availableEvents (void) {
	return _mouseEvents.size();
}

/*! Get the next event available for this input device. This is a destructive operation: the returned event is deleted
from the input device. */
CX_MouseEvent_t CX_Mouse::getNextEvent (void) {
	CX_MouseEvent_t front = _mouseEvents.front();
	_mouseEvents.pop();
	return front;
}

/*! Clear (delete) all events from this input device. */
void CX_Mouse::clearEvents (void) {
	while (!_mouseEvents.empty()) {
		_mouseEvents.pop();
	}
}

/*! 
Sets the position of the cursor, relative to the program the window. The window must be focused.
\param pos The location within the window to set the cursor.
*/
void CX_Mouse::setCursorPosition(ofPoint pos) {
	glfwSetCursorPos(CX::Private::glfwContext, pos.x, pos.y);
}

/*! Get the cursor position within the program window. If the mouse has left the window, 
this will return the last known position of the cursor within the window.
\return An ofPoint with the last cursor position. */
ofPoint CX_Mouse::getCursorPosition(void) {
	return ofPoint(ofGetMouseX(), ofGetMouseY());
}

/*! Show or hide the mouse cursor within the program window. If in windowed mode, the cursor will be visible outside of the window.
\param show If true, the cursor will be shown, if false it will not be shown. */
void CX_Mouse::showCursor (bool show) {
	if (show) {
		ofShowCursor();
	} else {
		ofHideCursor();
	}
}



void CX_Mouse::_mouseButtonPressedEventHandler (ofMouseEventArgs &a) {
	a.type = ofMouseEventArgs::Pressed; //To be clear, the only reason for this function and for mouseReleasedEventHandler are that OF does not
	//already mark the type properly. Maybe in the next version...
	_mouseEventHandler(a);
}

void CX_Mouse::_mouseButtonReleasedEventHandler (ofMouseEventArgs &a) {
	a.type = ofMouseEventArgs::Released;
	_mouseEventHandler(a);
}

void CX_Mouse::_mouseMovedEventHandler (ofMouseEventArgs &a) {
	a.type = ofMouseEventArgs::Moved;
	_mouseEventHandler(a);
}

void CX_Mouse::_mouseDraggedEventHandler (ofMouseEventArgs &a) {
	a.type = ofMouseEventArgs::Dragged;
	_mouseEventHandler(a);
}

void CX_Mouse::_mouseWheelScrollHandler(CX_MouseScrollEventArgs_t &a) {
	CX_MouseEvent_t ev;
	ev.eventType = CX_MouseEvent_t::SCROLLED;

	ev.eventTime = CX::Instances::Clock.getTime();
	ev.uncertainty = ev.eventTime - _lastEventPollTime;

	ev.button = -1;
	ev.x = (int)a.x;
	ev.y = (int)a.y;

	_mouseEvents.push(ev);
}

void CX_Mouse::_mouseEventHandler (ofMouseEventArgs &a) {
	CX_MouseEvent_t ev;
	ev.eventTime = CX::Instances::Clock.getTime();
	ev.uncertainty = ev.eventTime - _lastEventPollTime;

	ev.button = a.button;
	ev.x = (int)a.x;
	ev.y = (int)a.y;

	
	if (a.type == ofMouseEventArgs::Pressed) {
		ev.eventType = CX_MouseEvent_t::PRESSED;
		_heldMouseButtons.insert( a.button );

	} else if (a.type == ofMouseEventArgs::Released) {
		ev.eventType = CX_MouseEvent_t::RELEASED;
		_heldMouseButtons.erase( a.button );

	} else if (a.type == ofMouseEventArgs::Moved) {
		ev.eventType = CX_MouseEvent_t::MOVED;
		ev.button = -1; //To be obvious that the button data is garbage.

	} else if (a.type == ofMouseEventArgs::Dragged) {
		ev.eventType = CX_MouseEvent_t::DRAGGED;
		//It isn't clear what the button data should be set to in this case. The last mouse button pressed?
		//The last mouse button pressed before the drag started? User code just needs to check which mouse buttons are held, I guess.
		//GLFW sets it to something called "buttonInUse", which is the last mouse button pressed. This means that drags can start with one
		//mouse button and then continue with another. Let's just say that the button is gargage and user code has to deal with it.
		ev.button = -1; //To be obvious that the button data is garbage.

	} else {
		//This function should not be getting this event.
		return;
	}

	_mouseEvents.push( ev );
}

void CX_Mouse::_listenForEvents (bool listen) {
	if (listen == _listeningForEvents) {
		return;
	}

	if (listen) {
		ofAddListener( ofEvents().mousePressed, this, &CX_Mouse::_mouseButtonPressedEventHandler );
		ofAddListener( ofEvents().mouseReleased, this, &CX_Mouse::_mouseButtonReleasedEventHandler );
		ofAddListener( ofEvents().mouseMoved, this, &CX_Mouse::_mouseMovedEventHandler );
		ofAddListener( ofEvents().mouseDragged, this, &CX_Mouse::_mouseDraggedEventHandler );

		ofAddListener( CX::Events.scrollEvent, this, &CX_Mouse::_mouseWheelScrollHandler );
	} else {
		ofRemoveListener( ofEvents().mousePressed, this, &CX_Mouse::_mouseButtonPressedEventHandler );
		ofRemoveListener( ofEvents().mouseReleased, this, &CX_Mouse::_mouseButtonReleasedEventHandler );
		ofRemoveListener( ofEvents().mouseMoved, this, &CX_Mouse::_mouseMovedEventHandler );
		ofRemoveListener( ofEvents().mouseDragged, this, &CX_Mouse::_mouseDraggedEventHandler );

		ofRemoveListener(CX::Events.scrollEvent, this, &CX_Mouse::_mouseWheelScrollHandler);
	}
	_listeningForEvents = listen;
}

std::ostream& CX::operator<< (std::ostream& os, const CX_MouseEvent_t& ev) {
	string dlm = ", ";
	os << ev.button << dlm << ev.x << dlm << ev.y << dlm << ev.eventTime << dlm << ev.uncertainty << dlm << ev.eventType;
	return os;
}

std::istream& CX::operator>> (std::istream& is, CX_MouseEvent_t& ev) {
	is >> ev.button;
	is.ignore(2);
	is >> ev.x;
	is.ignore(2);
	is >> ev.y;
	is.ignore(2);
	is >> ev.eventTime;
	is.ignore(2);
	is >> ev.uncertainty;
	is.ignore(2);

	int eventType;
	is >> eventType;
	switch (eventType) {
	case CX_MouseEvent_t::MOVED: ev.eventType = CX_MouseEvent_t::MOVED; break;
	case CX_MouseEvent_t::DRAGGED: ev.eventType = CX_MouseEvent_t::DRAGGED; break;
	case CX_MouseEvent_t::PRESSED: ev.eventType = CX_MouseEvent_t::PRESSED; break;
	case CX_MouseEvent_t::RELEASED: ev.eventType = CX_MouseEvent_t::RELEASED; break;
	case CX_MouseEvent_t::SCROLLED: ev.eventType = CX_MouseEvent_t::SCROLLED; break;
	}

	return is;
}