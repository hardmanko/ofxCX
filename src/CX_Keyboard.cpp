#include "CX_Keyboard.h"

#include "CX_InputManager.h"

namespace CX {

CX_Keyboard::CX_Keyboard(CX_InputManager* owner) :
	_owner(owner),
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

/*! Returns `true` if the keyboard is enabled. */
bool CX_Keyboard::enabled(void) {
	return _enabled;
}

/*! Get the number of new events available for this input device. */
int CX_Keyboard::availableEvents(void) {
	return _keyEvents.size();
}

/*! Get the next event available for this input device. This is a destructive operation: the returned event is deleted
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

/*! This function checks to see if the given key is held, which means a keypress has been received, but not a key release.
\param key The key code or character for the key you are interested in. See the
documentation for \ref CX_Keyboard::Event::key for more information about this value.
\return True iff the given key is held. */
bool CX_Keyboard::isKeyHeld(int key) {
	return (_heldKeys.find(key) != _heldKeys.end());
}

/*! Identical to waitForKeypress() that takes a vector of keys except with a length 1 vector. */
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
cause this function to return.
\param clear If `true`, all waiting events will be flushed with CX_InputManager::pollEvents() and then all keyboard events
will be cleared both before and after waiting for the keypress. If `false` and `this->availableEvents() > 0`, it
is possible that one of the available events will include a keypress for a given key, in which case this function
will return immediately.
\param eraseEvent If `true`, the event will be erased from the queue of captured events. The implication of this removal 
is that the return value of this function is the only opportunity to gain access to the event that caused this function to return.
The advantage of this approach is that if, after some given key is pressed, all events in the queue are processed, you are
guaranteed to not hit the same event twice (once form the return value of this function, once from processing the queue).
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
		if (_owner->pollEvents()) {
			for (auto it = _keyEvents.begin(); it != _keyEvents.end(); it++) {
				if (it->type == CX_Keyboard::PRESSED) {
					if ((std::find(keys.begin(), keys.end(), -1) != keys.end()) ||
						(std::find(keys.begin(), keys.end(), it->key) != keys.end())) {

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
	}

	if (clear) {
		this->clearEvents();
	}

	this->enable(enabled);

	return rval;
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
	ev.key = a.key;
	_keyEventHandler(ev);
}

void CX_Keyboard::_keyReleaseHandler(ofKeyEventArgs &a) {
	CX_Keyboard::Event ev;
	ev.type = CX_Keyboard::RELEASED;
	ev.key = a.key;
	_keyEventHandler(ev);
}

void CX_Keyboard::_keyRepeatHandler(CX::Private::CX_KeyRepeatEventArgs_t &a) {
	CX_Keyboard::Event ev;
	ev.type = CX_Keyboard::REPEAT;
	ev.key = a.key;
	_keyEventHandler(ev);
}

void CX_Keyboard::_keyEventHandler(CX_Keyboard::Event &ev) {
	switch (ev.key) {
	case OF_KEY_CONTROL:
	case OF_KEY_ALT:
	case OF_KEY_SHIFT:
	case OF_KEY_SUPER:
		return; //These keys are reported by oF twice: once as OF_KEY_X and again as OF_KEY_RIGHT_X or OF_KEY_LEFT_X. We ignore the generic version.
	}

	//Make all keys lower (counteract oF behavior of uppcasing letter keys if shift is held).
	if (ev.key <= std::numeric_limits<unsigned char>::max()) {
		ev.key = ::tolower(ev.key);
	}

	ev.time = CX::Instances::Clock.now();
	ev.uncertainty = ev.time - _lastEventPollTime;

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

	_keyEvents.push_back(ev);
}

static const std::string dlm = ", ";

std::ostream& CX::operator<< (std::ostream& os, const CX_Keyboard::Event& ev) {
	os << ev.key << dlm << ev.time << dlm << ev.uncertainty << dlm << ev.type;
	return os;
}

std::istream& CX::operator>> (std::istream& is, CX_Keyboard::Event& ev) {
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