#include "CX_Keyboard.h"

using namespace CX;

CX_Keyboard::CX_Keyboard (void) :
	_listeningForEvents(false)
{
}

CX_Keyboard::~CX_Keyboard (void) {
	_listenForEvents(false);
}

/*! Get the number of new events available for this input device. */
int CX_Keyboard::availableEvents (void) {
	return _keyEvents.size();
}

/*! Get the next event available for this input device. This is a destructive operation: the returned event is deleted
from the input device. */
CX_Keyboard::Event CX_Keyboard::getNextEvent (void) {
	CX_Keyboard::Event front = _keyEvents.front();
	_keyEvents.pop();
	return front;
}

/*! Clear (delete) all events from this input device. */
void CX_Keyboard::clearEvents (void) {
	while (!_keyEvents.empty()) {
		_keyEvents.pop();
	}
}

void CX_Keyboard::_listenForEvents(bool listen) {
	if (_listeningForEvents == listen) {
		return;
	}

	if (listen) {
		ofAddListener(ofEvents().keyPressed, this, &CX_Keyboard::_keyPressHandler);
		ofAddListener(ofEvents().keyReleased, this, &CX_Keyboard::_keyReleaseHandler);
		ofAddListener(CX::Private::keyRepeatEvent, this, &CX_Keyboard::_keyRepeatHandler);
	} else {
		ofRemoveListener(ofEvents().keyPressed, this, &CX_Keyboard::_keyPressHandler);
		ofRemoveListener(ofEvents().keyReleased, this, &CX_Keyboard::_keyReleaseHandler);
		ofRemoveListener(CX::Private::keyRepeatEvent, this, &CX_Keyboard::_keyRepeatHandler);
	}
	_listeningForEvents = listen;
}

void CX_Keyboard::_keyPressHandler (ofKeyEventArgs &a) {
	CX_Keyboard::Event ev;
	ev.eventType = CX_Keyboard::Event::PRESSED;
	ev.key = a.key;
	_keyEventHandler(ev);
}

void CX_Keyboard::_keyReleaseHandler (ofKeyEventArgs &a) {
	CX_Keyboard::Event ev;
	ev.eventType = CX_Keyboard::Event::RELEASED;
	ev.key = a.key;
	_keyEventHandler(ev);
}

void CX_Keyboard::_keyRepeatHandler(CX::Private::CX_KeyRepeatEventArgs_t &a) {
	CX_Keyboard::Event ev;
	ev.eventType = CX_Keyboard::Event::REPEAT;
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

	ev.eventTime = CX::Instances::Clock.now();
	ev.uncertainty = ev.eventTime - _lastEventPollTime;

	switch (ev.eventType) {
	case CX_Keyboard::Event::PRESSED:
		_heldKeys.insert(ev.key);
		break;
	case CX_Keyboard::Event::RELEASED:
		_heldKeys.erase(ev.key);
		break;
	case CX_Keyboard::Event::REPEAT:

		break;
	}

	_keyEvents.push( ev );
}


std::ostream& CX::operator<< (std::ostream& os, const CX_Keyboard::Event& ev) {
	string dlm = ", ";
	os << ev.key << dlm << ev.eventTime << dlm << ev.uncertainty << dlm << ev.eventType;
	return os;
}

std::istream& CX::operator>> (std::istream& is, CX_Keyboard::Event& ev) {
	is >> ev.key;
	is.ignore(2);
	is >> ev.eventTime;
	is.ignore(2);
	is >> ev.uncertainty;
	is.ignore(2);

	int eventType;
	is >> eventType;
	switch (eventType) {
	case CX_Keyboard::Event::PRESSED: ev.eventType = CX_Keyboard::Event::PRESSED; break;
	case CX_Keyboard::Event::RELEASED: ev.eventType = CX_Keyboard::Event::RELEASED; break;
	case CX_Keyboard::Event::REPEAT: ev.eventType = CX_Keyboard::Event::REPEAT; break;
	}
	return is;
}