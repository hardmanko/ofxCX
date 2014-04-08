#include "CX_Events.h"

CX::Private::CX_Events& CX::Private::getEvents(void) {
	CX::Private::CX_Events* ev = new CX::Private::CX_Events;
	return *ev;
}