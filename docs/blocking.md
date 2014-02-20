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

Blocking code is bad because it prevents some parts of CX from working in some situations. It is not a cardinal
sin and there are times when using blocking code is acceptable. However, blocking code should
not be used when trying to present stimuli or when responses are being made. There is of course an exception to
the responses rule, which is when your blocking code is explicitly polling for user input, e.g.:

\code{.cpp}
while(!Input.pollEvents());
\endcode
