Response Input {#responseInput}
==============

CX provides built-in support for collecting input from keyboards, mice, and joysticks. The design of the input subsystem of CX is outlined in this section, with a focus on collecting keyboard input. The structure of input collection from other devices parallels keyboard input collection, so the ideas from this section generalize to other input devices. We begin with a code example of how to collect and process keyboard input.

~~~{.cpp}
#include "CX.h"

void runExperiment(void) {
	Input.setup(true, false); 

	while (true) {
		Input.pollEvents();

		while (Input.Keyboard.availableEvents() > 0) {

			CX_Keyboard::Event ev = Input.Keyboard.getNextEvent();

			if (ev.key == 'P' && ev.type == CX_Keyboard::PRESSED) {
				cout << "P is for Press" << endl;
			}

			if (ev.key == 'R' && ev.type == CX_Keyboard::RELEASED) {
				cout << "R is for Release" << endl;
			}

			if (ev.key == Keycode::PAGE_UP && ev.type == CX_Keyboard::REPEAT) {
				cout << "Page up is for repeat" << endl;
			}

			cout << "Key:         " << ev.key << endl <<
				"Key char:  \"" << (char)ev.key << "\"" << endl <<
				"Type:        " << ev.type << endl <<
				"Time:        " << ev.time << endl <<
				"Uncertainty: " << ev.uncertainty << endl << endl;

		}
	}
}
~~~

Keyboard, mouse, and joystick input in CX is handled through an instance of the \ref CX::CX_InputManager "CX_InputManager" class called CX::Instances::Input. By default, no input devices are enabled, so the first step is usually to enable the input devices you plan on using. To do so, call the setup function
~~~{.cpp}
Input.setup(true, false); //enable keyboard, don't enable mouse
~~~
which enables the keyboard but not the mouse. You can optionally pass a third argument, which is the index of a joystick that you want to use. We want this example to run indefinitely, so we put most of the code into a while loop, with the conditional expression set to the constant value `true`, so that the loop goes indefinitely. Inside the while loop, we check for new input on all enabled input devices by calling
~~~{.cpp}
Input.pollEvents();
~~~
which checks for new input on all input devices. We are mostly interested in the keyboard, which we can access with the Keyboard member of Input. With
~~~{.cpp}
while (Input.Keyboard.availableEvents() > 0) {

	CX_Keyboard::Event ev = Input.Keyboard.getNextEvent();

	//...
}
~~~
we say that as long as there are any available events for the keyboard, we want to keep looping. Each time through the loop, the next event from the keyboard is accessed with \ref CX::CX_Keyboard::getNextEvent() "Input.Keyboard.getNextEvent()". getNextEvent() returns the oldest event stored in an input device, deleting it from the device's event queue. As the events are processed, eventually they will all be removed from the queue of available events and availableEvents() will return 0. At that point, the event processing loop will end and the code will go back to checking for new input. The value that is returned by Input.Keyboard.getNextEvent() is a CX::CX_Keyboard::Event, which is a `struct` that contains information about the event, like what key was used and how the key was used. With
~~~{.cpp}
if (ev.key == 'P' && ev.type == CX_Keyboard::PRESSED) {
	cout << "P is for Press" << endl;
}
~~~
we test to see if the key that was used was the P key and that it was pressed. Notice that for many keys, we can compare the value of `ev.key` directly to character literals, which in C++ are enclosed in single quotes (strings are enclosed in double quotes). Also notice that the character 'P' is uppercase, which is the standard for the letter keys. To check that the key was pressed, we compare the type of the event to CX_Keyboard::PRESSED. We do a very similar thing for the R key
~~~{.cpp}
if (ev.key == 'R' && ev.type == CX_Keyboard::RELEASED) {
	cout << "R is for Release" << endl;
}
~~~
but instead of checking for a key press, we check for a release. The third and final type of key event is a key repeat, which happens after a key has been held for some time and multiple keypresses are sent repeatedly.
~~~{.cpp}
if (ev.key == Keycode::PAGE_UP && ev.type == CX_Keyboard::REPEAT) {
	cout << "Page up is for repeat" << endl;
}
~~~
Here we check for a key repeat for the page up key. For special keys for which it is not possible to represent the key with a character literal, you can compare the value of `ev.key` to special values from the CX::Keycode namespace/enum.

To end the example, we print out information about all each key that is used, regardless of how it was used, with
~~~{.cpp}
cout << "Key:         " << ev.key << endl <<
	"Key char:  \"" << (char)ev.key << "\"" << endl <<
	"Type:        " << ev.type << endl <<
	"Time:        " << ev.time << endl <<
	"Uncertainty: " << ev.uncertainty << endl << endl;
~~~
The key and type members have been discussed, but the time and uncertainty members have not. The time is the time at which the input event was received by CX. This is not the time at which the response was made, because there is some amount of latency between a response being made and information about that response filtering its way through the computer to CX. The uncertainty member gives the difference in time between the last and second to last times at which input events were polled with Input.pollEvents(). Because events cannot be polled for literally constantly, there is always some amount of time between each polling of events. The uncertainty member captures this uncertainty about when the event actually made it to CX, because the event could have become available at any point between the last and second to last polls. Generally, if the uncertainty is less than 1 ms, there is little cause for concern. If the uncertainty is high, you should use that as an indication that Input.pollEvents() should be called more frequently in the code. Naturally, uncertainty is the lower bound on the true uncertainty. For all CX knows, for any input event, that event could have occurred at any time in the past, so the true uncertainty is effectively unbounded. Because of the uncertantly and latency involved in input, the only correct interpretation of the time member of the event is that it is the time at which the event was given a timestamp by CX. The CX timestamps are probably pretty well correlated with the actual event time.

This example has shown one way of working with keyboard input events. There are a few other important functions for getting input. Instead of using getNextEvent() repeatedly, you can use \ref CX::CX_Keyboard::copyEvents() "copyEvents()" to get a vector containing a copy of all of the currently stored events. Using copyEvents() does not delete the stored events the way getNextEvent() does. To delete all of the stored events, you can use \ref CX::CX_Keyboard::clearEvents() "clearEvents()".

For working with mouse events, there is very little that is different from working with keyboard events. CX::Instances::Input::Mouse can be worked with in much the same way as the keyboard. The functions to check for available events and to get events have the same names and behaviors (availableEvents() and getNextEvent()). The type of the events for the mouse is CX_Mouse::Event, which has the same time and uncertainty members with identical interpretation as the keyboard events. It also has a \ref CX::CX_Mouse::Event::type "type" member that you can use to determine if the mouse was moved, clicked, dragged, etc. Instead of a key member, mouse events have a \ref CX::CX_Mouse::Event::button "button" member, which gives the number of the button that was used. It also gives \ref CX::CX_Mouse::Event::x "x" and \ref CX::CX_Mouse::Event::y "y" coordinates of the mouse for the event. CX_Joystick provides a similar interface for joysticks.

For a discussion of some of the issues involved in timestamping responses, see the \ref ioTiming page.