#include "CX_ThreadUtils.h"

namespace CX {
namespace Util {

		
////////////////////////
// MessageQueue<void> //
////////////////////////

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


} // namespace Util
} // namespace CX