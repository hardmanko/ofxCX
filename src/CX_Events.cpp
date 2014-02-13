#include "CX_Events.h"

#include "CX_Private.h"

using namespace CX::Private;

//This function should not be called before CX::Private::glfwContext has been set.
CX_Events& CX::Private::getEvents (void) {
	static CX_Events* _events = new CX_Events;
	return *_events;
}

void _scrollCallback(GLFWwindow* window, double x, double y); //Declaration

void CX_Events::setup(void) {
	if (CX::Private::glfwContext) {
		glfwSetScrollCallback(CX::Private::glfwContext, &_scrollCallback);
	}
	//Log an error?
}

CX_Events::CX_Events(void) {
	this->setup();
}

CX_Events::~CX_Events (void) {
	glfwSetScrollCallback(CX::Private::glfwContext, NULL);
}


void _scrollCallback(GLFWwindow* window, double x, double y) {
	CX_MouseScrollEventArgs_t a(x, y);
	ofNotifyEvent(CX::Private::getEvents().scrollEvent, a);
}
