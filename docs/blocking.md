Blocking Code {#blockingCode}
=============

Blocking code is code that either takes a long time to complete or that waits until some event occurs before
allowing code execution to continue. An example of blocking code that waits is

\code{.cpp}
do {
	Input.pollEvents();
} while (Input.Keyboard.availbleEvents() == 0);
\endcode

This code waits until the keyboard has been used in some way. No code past it can be executed until the keyboard
is used, which could take a long time. Any code that blocks while waiting for a human to do something is blocking.

An example of blocking code that takes a long time (or at least could take a long time) is

\code{.cpp}
vector<double> d = CX::Util::sequence<double>(0, 1000000, .033);
\endcode

which requires the allocation of about 300 MB of RAM. This code doesn't wait for anything to happen, it
just takes a long time to execute.

Blocking code is potentially harmful because it prevents some parts of CX from working in some situations. 
It is not a cardinal sin and there are times when using blocking code is acceptable. However, blocking code should
not be used when trying to present stimuli or when responses are being made. The reason for this is that CX expects
to be able to repeatedly check information related to stimulus presentation and input at very short intervals (at
least every millisecond), but that cannot happen if a piece of code is blocking. There is of course an exception to
the "no blocking while waiting for responses" rule, which is when your blocking code is doing nothing but waiting 
for a response and constantly polling for user input. For example, the following code waits until any response is made:

\code{.cpp}
while(!Input.pollEvents())
	;
\endcode
