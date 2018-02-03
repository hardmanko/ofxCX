#include "CX_Mouse.h"

#include "CX_InputManager.h"

#include "GLFW/glfw3.h"
#include "CX_Private.h"
#include "CX_Display.h"

namespace CX {

CX_Mouse::CX_Mouse(CX_InputManager* owner) :
	_owner(owner),
	_enabled(false),
	_listeningForEvents(false),
	_cursorPos(ofPoint(0,0))
{
}

CX_Mouse::~CX_Mouse(void) {
	_listenForEvents(false);
}

/*! Enable or disable the mouse.
\param enable If `true`, the mouse will be enabled; if `false` it will be disabled.
*/
void CX_Mouse::enable(bool enable) {
	_listenForEvents(enable);

	_enabled = enable;
	if (!enable) {
		clearEvents();
	}
}

/*! \brief Returns `true` if the mouse is enabled. */
bool CX_Mouse::enabled(void) {
	return _enabled;
}

/*! Get the number of available events for this input device. 
Events can be accessed with CX_Mouse::getNextEvent() or CX_Mouse::copyEvents(). */
int CX_Mouse::availableEvents(void) {
	return _mouseEvents.size();
}

/*! Get the next event available for this input device. This is a destructive operation in which the returned event is deleted
from the input device. */
CX_Mouse::Event CX_Mouse::getNextEvent(void) {
	CX_Mouse::Event front = _mouseEvents.front();
	_mouseEvents.pop_front();
	return front;
}

/*! Clear (delete) all events from this input device.
\note Unpolled events are not cleared by this function, which means that
responses made after a call to CX_InputManager::pollEvents() but before a call to
clearEvents() will not be removed by calling clearEvents(). */
void CX_Mouse::clearEvents(void) {
	_mouseEvents.clear();
}

/*! \brief Return a vector containing a copy of the currently stored events. The events stored by
the input device are unchanged. The first element of the vector is the oldest event. */
std::vector<CX_Mouse::Event> CX_Mouse::copyEvents(void) {
	std::vector<CX_Mouse::Event> copy(_mouseEvents.size());
	for (unsigned int i = 0; i < _mouseEvents.size(); i++) {
		copy[i] = _mouseEvents.at(i);
	}
	return copy;
}

/*!
Sets the position of the cursor, relative to the program the window. The window must be focused.
\param pos The location within the window to set the cursor.
*/
void CX_Mouse::setCursorPosition(ofPoint pos) {
	_cursorPos = pos;
	glfwSetCursorPos(CX::Private::glfwContext, pos.x, pos.y);
}

/*! Get the cursor position within the program window. If the mouse has left the window,
this will return the last known position of the cursor within the window.
\return An ofPoint with the last cursor position. */
ofPoint CX_Mouse::getCursorPosition(void) {
	return _cursorPos;
}

/*! Show or hide the mouse cursor within the program window. If in windowed mode, the cursor will be visible outside of the window.
\param show If true, the cursor will be shown, if false it will not be shown. */
void CX_Mouse::showCursor(bool show) {
	if (show) {
		ofShowCursor();
	} else {
		ofHideCursor();
	}
}

/*! This function checks to see if the button key is held, which means a button press has been received, but not a button release.
\param button The index of a button to check for. For the most common named buttons, see the CX_Mouse::Buttons enum.
\return `true` if the given key is held, `false` otherwise. */
bool CX_Mouse::isButtonHeld(int button) const {
	return _heldMouseButtons.find(button) != _heldMouseButtons.end();
}


/*! \brief Identical to CX_Mouse::waitForButtonPress(std::vector<int>, bool, bool), except
that this only takes a length 1 vector. */
CX_Mouse::Event CX_Mouse::waitForButtonPress(int button, bool clear, bool eraseEvent) {
	std::vector<int> buttons;
	buttons.push_back(button);
	return waitForButtonPress(buttons, clear, eraseEvent);
}


/*! Wait until the first of the given `buttons` is pressed. This specifically checks that a button has been pressed: If it was
already held at the time this function was called and then released, it will have to be pressed again before this
function will return. Returns a CX_Mouse::Event for the buttons that were waited on, optionally removing that event 
that caused this function to return from the queue of stored events if `eraseEvent` is `true`.
\param buttons A vector of button indices for the buttons that will be waited on. If any of the values are -1, any button 
press will cause this function to return. The button indices may be from CX_Mouse::Buttons or just raw integers.
\param clear If `true`, all waiting events will be flushed with CX_InputManager::pollEvents() and then all mouse events
will be cleared both before and after waiting for the keypress. If `false` and `this->availableEvents() > 0`, it
is possible that one of the available events will include a press for one of the buttons to be waited on, in which 
case this function will return immediately.
\param eraseEvent If `true`, the event that caused this function to return will be erased from the queue of stored events.
The implication of this removal is that the return value of this function is the only opportunity to gain access to the 
event that caused this function to return. The advantage of this approach is that if, after some given key is pressed, 
all events in the queue are processed, you are guaranteed to not hit the same event twice (once from the return value 
of this function, once from processing the queue).
\return A CX_Mouse::Event with information about the button press that caused this function to return.
\note If the mouse is not enabled at the time this function is called, it will be enabled for the
duration of the function and then disabled at the end of the function.
*/
CX_Mouse::Event CX_Mouse::waitForButtonPress(std::vector<int> buttons, bool clear, bool eraseEvent) {
	if (clear) {
		_owner->pollEvents();
		this->clearEvents();
	}

	bool enabled = this->enabled();
	this->enable(true);

	bool minus1Found = std::find(buttons.begin(), buttons.end(), -1) != buttons.end();

	CX_Mouse::Event rval;
	bool waiting = true;
	while (waiting) {
		if (!_owner->pollEvents()) {
			continue;
		}

		for (auto it = _mouseEvents.begin(); it != _mouseEvents.end(); it++) {
			if (it->type == CX_Mouse::PRESSED) {

				bool buttonFound = std::find(buttons.begin(), buttons.end(), it->button) != buttons.end();

				if (minus1Found || buttonFound) {
					rval = *it;

					if (eraseEvent) {
						_mouseEvents.erase(it);
					}

					waiting = false;
					break;
				}
			}
		}

	}

	if (clear) {
		this->clearEvents();
	}

	this->enable(enabled);

	return rval;
}

/*! Appends a mouse event to the event queue without any modification
(e.g. the timestamp is not set to the current time, it is left as-is).
This can be useful if you want to have a simulated participant perform the
task for debugging purposes.
If the event type is CX_Mouse::PRESSED or CX_Mouse::RELEASED, the button of the
event will be added to or removed from the list of held buttons, depending on event
type.
\param ev The event to append.
*/
void CX_Mouse::appendEvent(CX_Mouse::Event ev) {
	if (ev.type == CX_Mouse::PRESSED) {
		_heldMouseButtons.insert(ev.button);
	} else if (ev.type == CX_Mouse::RELEASED) {
		_heldMouseButtons.erase(ev.button);
	}

	_mouseEvents.push_back(ev);
}

//As of oF 084 (at least), the type of the event is properly marked by oF, so these functions are depreciated once oF 080 support is dropped.
void CX_Mouse::_mouseButtonPressedEventHandler(ofMouseEventArgs &a) {
	a.type = ofMouseEventArgs::Pressed;
	_mouseEventHandler(a);
}

void CX_Mouse::_mouseButtonReleasedEventHandler(ofMouseEventArgs &a) {
	a.type = ofMouseEventArgs::Released;
	_mouseEventHandler(a);
}

void CX_Mouse::_mouseMovedEventHandler(ofMouseEventArgs &a) {
	a.type = ofMouseEventArgs::Moved;
	_mouseEventHandler(a);
}

void CX_Mouse::_mouseDraggedEventHandler(ofMouseEventArgs &a) {
	a.type = ofMouseEventArgs::Dragged;
	_mouseEventHandler(a);
}

#if OF_VERSION_MAJOR == 0 && OF_VERSION_MINOR == 9 && OF_VERSION_PATCH >= 0
void CX_Mouse::_mouseWheelScrollHandler(ofMouseEventArgs &a) {
	CX_Mouse::Event ev;

	ev.time = CX::Instances::Clock.now();
	ev.uncertainty = ev.time - _lastEventPollTime;

	ev.type = CX_Mouse::SCROLLED;

	ev.button = -1;
	ev.x = a.scrollX;
	ev.y = a.scrollY;

	_mouseEvents.push_back(ev);
}
#else
void CX_Mouse::_mouseWheelScrollHandler(Private::CX_MouseScrollEventArgs_t &a) {
	CX_Mouse::Event ev;
	
	ev.time = CX::Instances::Clock.now();
	ev.uncertainty = ev.time - _lastEventPollTime;

	ev.type = CX_Mouse::SCROLLED;

	ev.button = -1;
	ev.x = a.x;
	ev.y = a.y;

	_mouseEvents.push_back(ev);
}
#endif

void CX_Mouse::_mouseEventHandler(ofMouseEventArgs &ofEvent) {
	CX_Mouse::Event ev;
	ev.time = CX::Instances::Clock.now();
	ev.uncertainty = ev.time - _lastEventPollTime;

	ev.button = ofEvent.button;
	ev.x = ofEvent.x;
	ev.y = ofEvent.y;
	if (CX::Instances::Disp.getYIncreasesUpwards()) {
		ev.y = CX::Instances::Disp.getResolution().y - ev.y; //Not good if multiple displays are possible
	}

	_cursorPos = ofPoint(ev.x, ev.y);

	switch (ofEvent.type) {
	case ofMouseEventArgs::Pressed:
		ev.type = CX_Mouse::PRESSED;
		_heldMouseButtons.insert(ofEvent.button);
		break;
	case ofMouseEventArgs::Released:
		ev.type = CX_Mouse::RELEASED;
		_heldMouseButtons.erase(ofEvent.button);
		break;
	case ofMouseEventArgs::Moved:
		ev.type = CX_Mouse::MOVED;
		ev.button = -1; //To be obvious that the button data is garbage.
		break;
	case ofMouseEventArgs::Dragged:
		ev.type = CX_Mouse::DRAGGED;
		//It isn't clear what the button data should be set to in this case. The last mouse button pressed?
		//The last mouse button pressed before the drag started? User code just needs to check which mouse buttons are held, I guess.
		//GLFW sets it to something called "buttonInUse", which is the last mouse button pressed. This means that drags can start with one
		//mouse button and then continue with another. Let's just say that the button is garbage and user code has to deal with it.
		ev.button = -1; //To be obvious that the button data is garbage.
		break;
	default:
		return; //This function should not be getting this event.
	}

	_mouseEvents.push_back(ev);
}

void CX_Mouse::_listenForEvents(bool listen) {
	if (listen == _listeningForEvents) {
		return;
	}

	if (listen) {
		ofAddListener(ofEvents().mousePressed, this, &CX_Mouse::_mouseButtonPressedEventHandler);
		ofAddListener(ofEvents().mouseReleased, this, &CX_Mouse::_mouseButtonReleasedEventHandler);
		ofAddListener(ofEvents().mouseMoved, this, &CX_Mouse::_mouseMovedEventHandler);
		ofAddListener(ofEvents().mouseDragged, this, &CX_Mouse::_mouseDraggedEventHandler);

#if OF_VERSION_MAJOR == 0 && OF_VERSION_MINOR == 9 && OF_VERSION_PATCH >= 0
		ofAddListener(ofEvents().mouseScrolled, this, &CX_Mouse::_mouseWheelScrollHandler);
#else
		ofAddListener(CX::Private::getEvents().scrollEvent, this, &CX_Mouse::_mouseWheelScrollHandler);
#endif

	} else {
		ofRemoveListener(ofEvents().mousePressed, this, &CX_Mouse::_mouseButtonPressedEventHandler);
		ofRemoveListener(ofEvents().mouseReleased, this, &CX_Mouse::_mouseButtonReleasedEventHandler);
		ofRemoveListener(ofEvents().mouseMoved, this, &CX_Mouse::_mouseMovedEventHandler);
		ofRemoveListener(ofEvents().mouseDragged, this, &CX_Mouse::_mouseDraggedEventHandler);

#if OF_VERSION_MAJOR == 0 && OF_VERSION_MINOR == 9 && OF_VERSION_PATCH >= 0
		ofRemoveListener(ofEvents().mouseScrolled, this, &CX_Mouse::_mouseWheelScrollHandler);
#else
		ofRemoveListener(CX::Private::getEvents().scrollEvent, this, &CX_Mouse::_mouseWheelScrollHandler);
#endif
	}
	_listeningForEvents = listen;
}

static const std::string dlm = ", ";

/*! \brief Stream insertion operator for the CX_Mouse::Event struct. */
std::ostream& operator<< (std::ostream& os, const CX_Mouse::Event& ev) {
	os << ev.button << dlm << ev.x << dlm << ev.y << dlm << ev.time << dlm << ev.uncertainty << dlm << ev.type;
	return os;
}

/*! \brief Stream extraction operator for the CX_Mouse::Event struct. */
std::istream& operator>> (std::istream& is, CX_Mouse::Event& ev) {
	is >> ev.button;
	is.ignore(dlm.size());
	is >> ev.x;
	is.ignore(dlm.size());
	is >> ev.y;
	is.ignore(dlm.size());
	is >> ev.time;
	is.ignore(dlm.size());
	is >> ev.uncertainty;
	is.ignore(dlm.size());

	int eventType;
	is >> eventType;
	switch (eventType) {
	case CX_Mouse::MOVED: ev.type = CX_Mouse::MOVED; break;
	case CX_Mouse::DRAGGED: ev.type = CX_Mouse::DRAGGED; break;
	case CX_Mouse::PRESSED: ev.type = CX_Mouse::PRESSED; break;
	case CX_Mouse::RELEASED: ev.type = CX_Mouse::RELEASED; break;
	case CX_Mouse::SCROLLED: ev.type = CX_Mouse::SCROLLED; break;
	}

	return is;
}

}
