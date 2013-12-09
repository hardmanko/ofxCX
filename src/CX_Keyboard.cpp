#include "CX_Keyboard.h"

using namespace CX;

CX_Keyboard::CX_Keyboard (void) :
	_listeningForEvents(false)
{
}

CX_Keyboard::~CX_Keyboard (void) {
	_listenForEvents(false);
}

void CX_Keyboard::_listenForEvents (bool listen) {
	if (_listeningForEvents == listen) {
		return;
	}

	if (listen) {
		ofAddListener( ofEvents().keyPressed, this, &CX_Keyboard::_keyPressHandler );
		ofAddListener( ofEvents().keyReleased, this, &CX_Keyboard::_keyReleaseHandler );
	} else {
		ofRemoveListener( ofEvents().keyPressed, this, &CX_Keyboard::_keyPressHandler );
		ofRemoveListener( ofEvents().keyReleased, this, &CX_Keyboard::_keyReleaseHandler );
	}
	_listeningForEvents = listen;
}

int CX_Keyboard::availableEvents (void) {
	return _keyEvents.size();
}

CX_KeyEvent_t CX_Keyboard::popEvent (void) {
	CX_KeyEvent_t front = _keyEvents.front();
	_keyEvents.pop();
	return front;
}

void CX_Keyboard::clearEvents (void) {
	while (!_keyEvents.empty()) {
		_keyEvents.pop();
	}
}

void CX_Keyboard::_keyPressHandler (ofKeyEventArgs &a) {
	a.type = ofKeyEventArgs::Pressed;
	_keyEventHandler(a);
}

void CX_Keyboard::_keyReleaseHandler (ofKeyEventArgs &a) {
	a.type = ofKeyEventArgs::Released;
	_keyEventHandler(a);
}

void CX_Keyboard::_keyEventHandler (ofKeyEventArgs &a) {

	CX_KeyEvent_t ev;
	ev.eventTime = CX::Instances::Clock.getTime();
	ev.uncertainty = ev.eventTime - _lastEventPollTime;

	ev.key = a.key;

	//int modifierKeyChange = 0;
	if (a.type == ofKeyEventArgs::Pressed) {
		bool keyAlreadyHeld = (_heldKeys.find(a.key) != _heldKeys.end());

		if (keyAlreadyHeld) {
			ev.eventType = CX_KeyEvent_t::REPEAT;
		} else {
			ev.eventType = CX_KeyEvent_t::PRESSED;
		}

		_heldKeys.insert(a.key);

		//modifierKeyChange = 1;
	} else if (a.type == ofKeyEventArgs::Released) {
		ev.eventType = CX_KeyEvent_t::RELEASED;
		_heldKeys.erase(a.key);

		//modifierKeyChange = -1;
	}

	_keyEvents.push( ev );

	//Do I even want to do this????!?!?!!?!?!???!?!??!?!?!!!!!!!?!?!?!!?!!?!?!?????!?! NO!!!!!!!!?!!!
	/*
	if ((a.key & OF_KEY_CONTROL) == OF_KEY_CONTROL) {
		_heldModifiers.ctrl += modifierKeyChange;
	} else if ((a.key & OF_KEY_ALT) == OF_KEY_ALT) {
		_heldModifiers.alt += modifierKeyChange;
	} else if ((a.key & OF_KEY_SHIFT) == OF_KEY_SHIFT) {
		_heldModifiers.shift += modifierKeyChange;
	} else if ((a.key & OF_KEY_SUPER) == OF_KEY_SUPER) {
		_heldModifiers.meta += modifierKeyChange;
	}
	*/

}

