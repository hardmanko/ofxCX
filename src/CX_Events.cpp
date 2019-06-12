#include "CX_Events.h"

CX::Private::CX_Events& CX::Private::getEvents(void) {
	static std::shared_ptr<CX::Private::CX_Events> _events = std::make_shared<CX::Private::CX_Events>();
	return *_events;
}