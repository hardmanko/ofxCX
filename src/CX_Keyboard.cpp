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

/*! Enable or disable the keyboard. When enabled or disabled, all stored events will
be cleared.
\param enable If `true`, the keyboard will be enabled; if `false` it will be disabled.
*/
void CX_Keyboard::enable(bool enable) {
	if (_enabled == enable) {
		return;
	}

	_listenForEvents(enable);

	_enabled = enable;
	
	clearEvents();
	_heldKeys.clear();
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
\note Unpolled events are not cleared by this function, which means that
responses made after a call to CX_InputManager::pollEvents() but before a call to
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


/*! \brief Identical to CX_Keyboard::waitForKeypress(std::vector<int>, bool, bool), except 
that this only takes a length 1 vector. */
CX_Keyboard::Event CX_Keyboard::waitForKeypress(int key, bool clear, bool eraseEvent) {
	std::vector<int> keys;
	keys.push_back(key);
	return waitForKeypress(keys, clear, eraseEvent);
}


/*! Wait until the first of the given `keys` is pressed. This specifically checks that a key has been pressed: If it was
already held at the time this function was called and then released, it will have to be pressed again before this
function will return. Returns a CX_Keyboard::Event for the key that was waited on, optionally removing that event 
that caused this function to return from the queue of stored events if `eraseEvent` is `true`.

\param keys A vector of key codes for the keys that will be waited on. If any of the codes are -1, any keypress will
cause this function to return. Should be character literals (e.g. 'A', with the single quotes, for the A key) or from CX::Keycode.
\param clear If `true`, all waiting events will be flushed with CX_InputManager::pollEvents() and then all keyboard events
will be cleared both before and after waiting for the keypress. If `false` and `this->availableEvents() > 0`, it
is possible that one of the available events will include a keypress for one of the keys to be waited on, in which 
case this function will return immediately.
\param eraseEvent If `true`, the event that caused this function to return will be erased from the queue of stored events. 
The implication of this removal is that the return value of this function is the only opportunity to gain access to the event 
that caused this function to return. The advantage of this approach is that if, after some given key is pressed, all events 
in the queue are processed, you are guaranteed to not hit the same event twice (once from the return value of this function, 
once from processing the queue).

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

	bool minus1Found = std::find(keys.begin(), keys.end(), -1) != keys.end();

	CX_Keyboard::Event rval;
	bool waiting = true;
	while (waiting) {
		if (!_owner->pollEvents()) {
			continue;
		}

		for (auto it = _keyEvents.begin(); it != _keyEvents.end(); it++) {
			if (it->type == CX_Keyboard::PRESSED) {
				
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


/*! Checks whether the given key chord is held, i.e. all of the keys in `chord` are held simultaneously.
This is an exact test: No extraneous keys may be held.
\return Retruns `false` if `chord` is empty or if not all of the keys in `chord` are held. 
Returns `true` if all of the keys in `chord` are held and no additional keys are held.
*/
bool CX_Keyboard::isChordHeld(const std::vector<int>& chord) const {
	if (chord.empty()) {
		return false;
	}

	std::set<int> chordSet(chord.begin(), chord.end());

	return chordSet == _heldKeys;
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
	} else if (ev.type == CX_Keyboard::RELEASED) {
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
	
	if (this->isKeyHeld(a.keycode)) {
		ev.type = CX_Keyboard::REPEAT;
	} else {
		ev.type = CX_Keyboard::PRESSED;
	}

#if OF_VERSION_MAJOR == 0 && OF_VERSION_MINOR == 8 && OF_VERSION_PATCH == 0
	ev.codes = Keycodes(a.key, -1, -1, -1);
#elif (OF_VERSION_MAJOR == 0 && OF_VERSION_MINOR == 8 && OF_VERSION_PATCH == 4) || \
		(OF_VERSION_MAJOR == 0 && OF_VERSION_MINOR == 9 && OF_VERSION_PATCH >= 0)
	ev.codes = Keycodes(a.key, a.keycode, a.scancode, a.codepoint);
#endif
	
	_keyEventHandler(ev);
}

void CX_Keyboard::_keyReleaseHandler(ofKeyEventArgs &a) {
	CX_Keyboard::Event ev;
	ev.type = CX_Keyboard::RELEASED;

#if OF_VERSION_MAJOR == 0 && OF_VERSION_MINOR == 8 && OF_VERSION_PATCH == 0
	ev.codes = Keycodes(a.key, -1, -1, -1);
#elif (OF_VERSION_MAJOR == 0 && OF_VERSION_MINOR == 8 && OF_VERSION_PATCH == 4) || \
		(OF_VERSION_MAJOR == 0 && OF_VERSION_MINOR == 9 && OF_VERSION_PATCH >= 0)
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
	default:
		break;
	}

	_checkForShortcuts();

	_keyEvents.push_back(ev);
}

/*! Add a keyboard shortcut chord (1 or more keys held at once) and the function that will be called
when the shortcut is held. The shortcuts require that exactly the desired keys are held. No other keys
may be held.

Keyboard shortcuts are checked for every time `CX_InputManager::pollEvents()` is called. This means
that you can set up keyboard shortcuts that work the same way throughout the whole experiment once,
and because the shortcuts are set up, you won't have to check for the shortcuts in each section of code
in which input is awaited on. You just need to regularly call `Input.pollEvents()` in your code.
Given that most experiment code spends a lot of time waiting on input, using keyboard shortcuts should
be easy.

By default, CX is set up to use the shortcut `LEFT_ALT + F1` to toggle the fullscreen state of the display.
Windows 10 appears to intercept alt-tab so that it never reaches the program, otherwise that would be
the best choice. The shortcut is named "Toggle fullscreen: LEFT_ALT + F1".

\param name The name of the shortcut. Each shortcut must have a unique name.
\param chord A vector of keys that must be simultaneously held (and no other keys may be held) to trigger the shortcut.
\param callback A function that takes and returns `void`.

\note The keyboard is automatically enabled.

\code{.cpp}

void helloWorld(void) {
	Log.notice() << "Hello, world!";
	Log.flush();
}

void toggleFullscreen(void) {
	Disp.setFullscreen(!Disp.isFullscreen());
}

void runExperiment(void) {
	Input.Keyboard.enable(true);

	Input.Keyboard.addShortcut("helloWorld", { Keycode::LEFT_CTRL, 'H' }, helloWorld);

	Input.Keyboard.addShortcut("toggleFullscreen", { Keycode::LEFT_CTRL, 'T' }, toggleFullscreen);

	
	bool looping = true;
	auto endLoop = [&]() {
		looping = false;
	};

	Input.Keyboard.addShortcut("endLoop", { Keycode::LEFT_CTRL, 'Q' }, endLoop);

	while (looping) {
		Input.pollEvents();
	}

}

\endcode
*/
void CX_Keyboard::addShortcut(std::string name, const std::vector<int>& chord, std::function<void(void)> callback) {
	this->enable(true); // Automatically enable keyboard

	KeyboardShortcut ks;
	ks.chord.insert(chord.begin(), chord.end());
	ks.callback = callback;

	_shortcuts[name] = ks;
}

/*! Removes a shortcut by name.
\param name The name of the shortcut.
*/
void CX_Keyboard::removeShortcut(std::string name) {
	_shortcuts.erase(name);
}

/*! Clears all stored keyboard shortcuts. */
void CX_Keyboard::clearShortcuts(void) {
	_shortcuts.clear();
}

/*! Get a vector of the names of shortcuts.
\return A vector of shortcut names.
*/
std::vector<std::string> CX_Keyboard::getShortcutNames(void) const {
	std::vector<std::string> names;
	for (auto& it : _shortcuts) {
		names.push_back(it.first);
	}
	return names;
}

void CX_Keyboard::_checkForShortcuts(void) {
	for (const std::pair<std::string, KeyboardShortcut>& ks : _shortcuts) {
		if (ks.second.chord == _heldKeys) {
			ks.second.callback();
		}
	}
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

} // namespace CX
