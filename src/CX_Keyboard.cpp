#include "CX_Keyboard.h"

#include "CX_InputManager.h"

namespace CX {

CX_Keyboard::CX_Keyboard(CX_InputManager* owner) :
	_owner(owner),
	_enabled(false),
	_listeningForEvents(false)
{
}

CX_Keyboard::~CX_Keyboard(void) {
	_listenForEvents(false);
}

/*! Enable or disable the keyboard.
\param enable If `true`, the keyboard will be enabled; if `false` it will be disabled.
*/
void CX_Keyboard::enable(bool enable) {
	_listenForEvents(enable);

	_enabled = enable;
	if (!enable) {
		clearEvents();
	}
}

/*! \brief Returns `true` if the keyboard is enabled. */
bool CX_Keyboard::enabled(void) {
	return _enabled;
}

/*! Get the number of available events for this input device. 
Events can be accessed with CX_Keyboard::getNextEvent() or CX_Keyboard::copyEvents(). */
int CX_Keyboard::availableEvents(void) const {
	return _keyEvents.size();
}

/*! Get the next event available for this input device. This is a destructive operation in which the returned event is deleted
from the input device. */
CX_Keyboard::Event CX_Keyboard::getNextEvent(void) {
	CX_Keyboard::Event nextEvent = _keyEvents.front();
	_keyEvents.pop_front();
	return nextEvent;
}

/*! Clear (delete) all events from this input device.
\note This function only clears already existing events from the device, which means that
responses made between a call to CX_InputManager::pollEvents() and a subsequent call to
clearEvents() will not be removed by calling clearEvents(). */
void CX_Keyboard::clearEvents(void) {
	_keyEvents.clear();
}

/*! \brief Return a vector containing a copy of the currently stored events. The events stored by
the input device are unchanged. The first element of the vector is the oldest event. */
std::vector<CX_Keyboard::Event> CX_Keyboard::copyEvents(void) {
	std::vector<CX_Keyboard::Event> copy(_keyEvents.size());
	for (unsigned int i = 0; i < _keyEvents.size(); i++) {
		copy[i] = _keyEvents.at(i);
	}
	return copy;
}

/*! This function checks to see if the given key is held, which means a keypress has been received, but not a key release.
\param key The character literal for the key you are interested in or special key code from CX::Keycode. 
\return `true` if the given key is held, `false` otherwise. */
bool CX_Keyboard::isKeyHeld(int key) const {
	return _heldKeys.find(key) != _heldKeys.end();
}


/*! Identical to CX_Keyboard::waitForKeypress() that takes a vector of keys except with a length 1 vector. */
CX_Keyboard::Event CX_Keyboard::waitForKeypress(int key, bool clear, bool eraseEvent) {
	std::vector<int> keys;
	keys.push_back(key);
	return waitForKeypress(keys, clear, eraseEvent);
}


/*! Wait until the first of the given `keys` is pressed. This specifically checks that a key has been pressed: If it was
held at the time this function was called and then released, it will have to be pressed again before this
function will return. Returns a CX_Keyboard::Event for the key that was waited on, optionally removing that event from the
stored events if `eraseEvent` is `true`.
\param keys A vector of key codes for the keys that will be waited on. If any of the codes are -1, any keypress will
cause this function to return. Should be character literals or from CX::Keycode.
\param clear If `true`, all waiting events will be flushed with CX_InputManager::pollEvents() and then all keyboard events
will be cleared both before and after waiting for the keypress. If `false` and `this->availableEvents() > 0`, it
is possible that one of the available events will include a keypress for a given key, in which case this function
will return immediately.
\param eraseEvent If `true`, the event will be erased from the queue of captured events. The implication of this removal
is that the return value of this function is the only opportunity to gain access to the event that caused this function to return.
The advantage of this approach is that if, after some given key is pressed, all events in the queue are processed, you are
guaranteed to not hit the same event twice (once from the return value of this function, once from processing the queue).
\return A CX_Keyboard::Event with information about the keypress that caused this function to return.
\note If the keyboard is not enabled at the time this function is called, it will be enabled for the
duration of the function and then disabled at the end of the function.
*/
CX_Keyboard::Event CX_Keyboard::waitForKeypress(std::vector<int> keys, bool clear, bool eraseEvent) {
	if (clear) {
		_owner->pollEvents();
		this->clearEvents();
	}

	bool enabled = this->enabled();
	this->enable(true);

	CX_Keyboard::Event rval;
	bool waiting = true;
	while (waiting) {
		if (!_owner->pollEvents()) {
			continue;
		}

		for (auto it = _keyEvents.begin(); it != _keyEvents.end(); it++) {
			if (it->type == CX_Keyboard::PRESSED) {
				bool minus1Found = std::find(keys.begin(), keys.end(), -1) != keys.end();
				bool keyFound = std::find(keys.begin(), keys.end(), it->key) != keys.end();

				if (minus1Found || keyFound) {
					rval = *it;

					if (eraseEvent) {
						_keyEvents.erase(it);
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



/*! Change the set of keys that must be pressed at once for the program to close.
By default, pressing `right-alt + F4` will exit the program.
\param chord A vector of keys that, when held simulatenously, will cause the program to exit.
\note You must be exact about modifier keys: Using, for example, OF_KEY_SHIFT does nothing.
You must use OF_KEY_LEFT_SHIFT or OF_KEY_RIGHT_SHIFT.
*/
void CX_Keyboard::setExitChord(std::vector<int> chord) {
	ofSetEscapeQuitsApp(false);
	_exitChord = chord;
}

/*! Checks whether the given key chord is held, i.e. are all of the keys in `chord` held
right now.
\return `false` if `chord` is empty or if not all of the keys in `chord` are held. `true` if all of
the keys in `chord` are held.
*/
bool CX_Keyboard::isChordHeld(const std::vector<int>& chord) const {
	if (chord.empty()) {
		return false;
	}

	bool allHeld = true;
	for (int key : chord) {
		allHeld = allHeld && isKeyHeld(key);
	}
	return allHeld;
}

/*! Appends a keyboard event to the event queue without any modification
(e.g. the timestamp is not set to the current time, it is left as-is).
This can be useful if you want to have a simulated participant perform the
task for debugging purposes.
If the event type is CX_Keyboard::PRESSED or CX_Keyboard::RELEASED, the key of the
event will be added to or removed from the list of held keys, depending on event
type.
\param ev The event to append.
*/
void CX_Keyboard::appendEvent(CX_Keyboard::Event ev) {
	if (ev.type == CX_Keyboard::PRESSED) {
		_heldKeys.insert(ev.key);
	} else if (ev.type == CX_Keyboard::PRESSED) {
		_heldKeys.erase(ev.key);
	}

	_keyEvents.push_back(ev);
}

void CX_Keyboard::_listenForEvents(bool listen) {
	if (_listeningForEvents == listen) {
		return;
	}

	if (listen) {
		ofAddListener(ofEvents().keyPressed, this, &CX_Keyboard::_keyPressHandler);
		ofAddListener(ofEvents().keyReleased, this, &CX_Keyboard::_keyReleaseHandler);
		ofAddListener(CX::Private::getEvents().keyRepeatEvent, this, &CX_Keyboard::_keyRepeatHandler);
	} else {
		ofRemoveListener(ofEvents().keyPressed, this, &CX_Keyboard::_keyPressHandler);
		ofRemoveListener(ofEvents().keyReleased, this, &CX_Keyboard::_keyReleaseHandler);
		ofRemoveListener(CX::Private::getEvents().keyRepeatEvent, this, &CX_Keyboard::_keyRepeatHandler);
	}
	_listeningForEvents = listen;
}

void CX_Keyboard::_keyPressHandler(ofKeyEventArgs &a) {
	CX_Keyboard::Event ev;
	ev.type = CX_Keyboard::PRESSED;

#if OF_VERSION_MAJOR == 0 && OF_VERSION_MINOR == 8 && OF_VERSION_PATCH == 0
	ev.codes = Keycodes(a.key, -1, -1, -1);
#elif OF_VERSION_MAJOR == 0 && OF_VERSION_MINOR == 8 && OF_VERSION_PATCH == 4
	ev.codes = Keycodes(a.key, a.keycode, a.scancode, a.codepoint);
#endif
	
	_keyEventHandler(ev);
}

void CX_Keyboard::_keyReleaseHandler(ofKeyEventArgs &a) {
	CX_Keyboard::Event ev;
	ev.type = CX_Keyboard::RELEASED;

#if OF_VERSION_MAJOR == 0 && OF_VERSION_MINOR == 8 && OF_VERSION_PATCH == 0
	ev.codes = Keycodes(a.key, -1, -1, -1);
#elif OF_VERSION_MAJOR == 0 && OF_VERSION_MINOR == 8 && OF_VERSION_PATCH == 4
	ev.codes = Keycodes(a.key, a.keycode, a.scancode, a.codepoint);
#endif

	_keyEventHandler(ev);
}

void CX_Keyboard::_keyRepeatHandler(CX::Private::CX_KeyRepeatEventArgs_t &a) {
	CX_Keyboard::Event ev;
	ev.type = CX_Keyboard::REPEAT;

	ev.codes = Keycodes(a.key, a.keycode, a.scancode, a.codepoint);

	_keyEventHandler(ev);
}

void CX_Keyboard::_keyEventHandler(CX_Keyboard::Event &ev) {
	ev.time = CX::Instances::Clock.now();
	ev.uncertainty = ev.time - _lastEventPollTime;

	ev.key = ev.codes.glfw;

	switch (ev.codes.oF) {
	case OF_KEY_CONTROL:
	case OF_KEY_ALT:
	case OF_KEY_SHIFT:
	case OF_KEY_SUPER:
		return; //These keys are reported by oF twice: once as OF_KEY_X and again as 
			//OF_KEY_RIGHT_X or OF_KEY_LEFT_X. This ignores the generic version.
	}

	switch (ev.type) {
	case CX_Keyboard::PRESSED:
		_heldKeys.insert(ev.key);
		break;
	case CX_Keyboard::RELEASED:
		_heldKeys.erase(ev.key);
		break;
	case CX_Keyboard::REPEAT:
		break;
	}

	if (isChordHeld(_exitChord)) {
		std::exit(0);
	}

	_keyEvents.push_back(ev);
}



static const std::string dlm = ", ";

/*! \brief Stream insertion operator for the CX_Keyboard::Event struct. */
std::ostream& operator<< (std::ostream& os, const CX_Keyboard::Event& ev) {
	os << ev.key << dlm << ev.time << dlm << ev.uncertainty << dlm << ev.type;
	return os;
}

/*! \brief Stream extraction operator for the CX_Keyboard::Event struct. */
std::istream& operator>> (std::istream& is, CX_Keyboard::Event& ev) {
	is >> ev.key;
	is.ignore(dlm.size());
	is >> ev.time;
	is.ignore(dlm.size());
	is >> ev.uncertainty;
	is.ignore(dlm.size());

	int eventType;
	is >> eventType;

	switch (eventType) {
	case CX_Keyboard::PRESSED: ev.type = CX_Keyboard::PRESSED; break;
	case CX_Keyboard::RELEASED: ev.type = CX_Keyboard::RELEASED; break;
	case CX_Keyboard::REPEAT: ev.type = CX_Keyboard::REPEAT; break;
	}
	return is;
}



}
