#include "CX_Mouse.h"

#include "ofAppRunner.h" //ofShowCursor()/ofHideCursor()

using namespace CX;

CX_Mouse::CX_Mouse (void) :
	_listeningForEvents(false)
{
}

CX_Mouse::~CX_Mouse (void) {
	_listenForEvents(false);
}


int CX_Mouse::availableEvents (void) {
	return _mouseEvents.size();
}

CX_MouseEvent_t CX_Mouse::getNextEvent (void) {
	CX_MouseEvent_t front = _mouseEvents.front();
	_mouseEvents.pop();
	return front;
}

void CX_Mouse::clearEvents (void) {
	while (!_mouseEvents.empty()) {
		_mouseEvents.pop();
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
	} else {
		ofRemoveListener( ofEvents().mousePressed, this, &CX_Mouse::_mouseButtonPressedEventHandler );
		ofRemoveListener( ofEvents().mouseReleased, this, &CX_Mouse::_mouseButtonReleasedEventHandler );
		ofRemoveListener( ofEvents().mouseMoved, this, &CX_Mouse::_mouseMovedEventHandler );
		ofRemoveListener( ofEvents().mouseDragged, this, &CX_Mouse::_mouseDraggedEventHandler );
	}
	_listeningForEvents = listen;
}

void CX_Mouse::showCursor (bool show) {
	if (show) {
		ofShowCursor();
	} else {
		ofHideCursor();
	}
}