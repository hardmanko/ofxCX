#include "CX_ThreadUtils.h"

namespace CX {
namespace Util {

	MessageQueue<void>::MessageQueue(void) :
		_available(0)
	{}

	void MessageQueue<void>::push(void) {
		_available++;
	}

	size_t MessageQueue<void>::available(void) {
		return _available;
	}

	void MessageQueue<void>::pop(void) {
		_available--;
	}

	void MessageQueue<void>::clear(void) {
		_available = 0;
	}

		
PolledEventListener<void>::PolledEventListener(void) :
	_prevEv(nullptr),
	_available(0)
{}

PolledEventListener<void>::PolledEventListener(ofEvent<void>* ev) :
	_available(0)
{
	listenTo(ev);
}

void PolledEventListener<void>::listenTo(ofEvent<void>* ev) {
	_listenTo(ev);
}

size_t PolledEventListener<void>::available(void) {
	return _available;
}

void PolledEventListener<void>::pop(void) {
	_available--;
}

void PolledEventListener<void>::clearEvents(void) {
	_available = 0;
}

void PolledEventListener<void>::_listenTo(ofEvent<void>* ev) {
	if (_prevEv) {
		ofRemoveListener(*_prevEv, this, &PolledEventListener::_listenFun);
		_prevEv = nullptr;
		this->clearEvents();
	}

	if (ev) {
		ofAddListener(*ev, this, &PolledEventListener::_listenFun);
		_prevEv = ev;
	}
}

void PolledEventListener<void>::_listenFun(void) {
	_available++;
}

}
}