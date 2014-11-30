Visual Stimuli   {#visualStimuli}
===============

This section will describe a number of important details about how CX handles drawing visual stimuli. It begins with a discussion of framebuffers, which are what visual stimuli are drawn into. Then some examples of how to draw stimuli are given. Then the preferred method of presenting time-locked stimuli in CX is explained. Finally, some more background on achieving accurate stimulus timing by using vertical synchronization is given.

Framebuffers and Buffer Swapping
--------------------------------

Somes pieces of terminology that come up a lot in the documentation for CX are framebuffer, front buffer, back buffer, and buffer swapping.

A framebuffer is fairly easy to explain in the rough by example. The contents of the screen of a computer are stored in a framebuffer. A framebuffer is essentially a rectangle of pixels where each pixel can be set to display any color. Framebuffers do not always have the same number of pixels as the screen: you can have framebuffers that are smaller or larger than the size of the screen. Framebuffers larger than the screen don't really do much for you as you cannot fit the whole thing on the screen. All drawing in OpenGL and CX is done into a framebuffer.

There are two special framebuffers: The front buffer and the back buffer. These are created by OpenGL automatically as part of starting OpenGL. The size of these special framebuffers is functionally the same as the size of the window (or the whole screen, if in full screen mode). The front buffer contains what is shown on the screen. The back buffer is not presented on the screen, so it can be rendered to at any time without affecting what is visible on the screen. Typically, when you render stuff in CX, you call CX::CX_Display::beginDrawingToBackBuffer() and CX::CX_Display::endDrawingToBackBuffer() around whatever you are rendering. This causes drawing that happens between the two function calls to be rendered to the back buffer. 

What you have rendered to the back buffer has no effect on what you see on screen until you swap the contents of the front and back buffers. This isn't always a true swap, in that that the back buffer does not end up with the contents of the front buffer in it. On many systems, the back buffer is copied to the front buffer and is itself unchanged. In CX, this swap can be done by using different functions of CX_Display: CX::CX_Display::swapBuffers(), CX::CX_Display::swapBuffersInThread(), or CX::CX_Display::setAutomaticSwapping(). These functions are not interchangable, so make sure you are using the right one for your application. If there is any doubt, start with swapBuffers(), because it will work 100% of the time, with the others serving as ways to optimize the presentation of stimuli later.

Other than the back buffer, you can also make other offscreen buffers. You do this with an [ofFbo](http://openframeworks.cc/documentation/gl/ofFbo.html), which is an openFrameworks class. These offscreen buffers can be drawn into just like the back buffer, so you can use them to store part of a scene or a whole scene in an ofFbo. The contents of an ofFbo can be drawn onto the back buffer at a later time.

Drawing Visual Stimuli
----------------------

Now that framebuffers have been explained, we can talk about how to draw stimuli into them. Assume for the sake of example that we want to draw a red circle and a green rectangle on a black background. We might write this little test program:

~~~~~~{.cpp}
#include "CX.h"

void runExperiment(void) {
	Disp.beginDrawingToBackBuffer();

	ofBackground(ofColor::black);

	ofSetColor(ofColor::red);
	ofCircle(200, 300, 100);

	ofSetColor(0, 255, 0);
	ofRect(400, 200, 200, 100);

	Disp.endDrawingToBackBuffer();

	Disp.swapBuffers();

	Input.Keyboard.waitForKeypress(-1);
}
~~~~~~

Let's break this down. As always with a CX program, we include CX.h in order to access the functionality of CX. We also define the `runExperiment` function, in which we use the global display object `Disp`, which is a CX::CX_Display. We start with
~~~~~~{.cpp}
Disp.beginDrawingToBackBuffer();
~~~~~~
to say that we want to draw our stimuli to the back buffer. After saying that we want to draw into the back buffer, we can start drawing things. We do all of the drawing in this example with openFrameworks drawing functions. Like all functions in openFrameworks, they are prefixed with the abbreviation "of". With the call
~~~~~~{.cpp}
ofBackground(ofColor::black);
~~~~~~
we set the background color to black. There is nothing fancy here like a special background color layer: this is just filling the entire back buffer with black. If you draw some stimuli and then call `ofBackground`, you will overwrite your stimuli. The way that we specify the color is by using a named constant color that is a static member of the `ofColor` class, by using double-colon to access the static member. We then draw the circle with
~~~~~~{.cpp}
ofSetColor(ofColor::red);
ofCircle(200, 300, 100); //x position, y position, radius
~~~~~~
The way that drawing is set up in openFrameworks is that for a lot of things that you draw, you first set the color that it will be drawn with, then you draw the thing itself. Here, we call `ofSetColor` to set the drawing color to red before drawing the circle. `ofCircle` draws a circle at the specified x and y coordinates with the given radius. All of the values are in pixels. By default, the coordinate system is set up so that the point (0,0) is in the upper-left corner of the screen. The x values increase to the right and the y values increase downwards. If you don't like the fact that y values increase downwards, you can call CX::CX_Display::setYIncreasesUpwards() with `true` as the argument at the beginning of the experiment. It is possible that not everything properly accounts for the change in the y-axis direction, so some graphical bugs are possible if the y-values increase upwards. However, the vast majority of things work just fine.

Now that the circle has been drawn, we draw the rectangle with
~~~{.cpp}
ofSetColor(0, 255, 0); //red, green, blue (amounts out of 255)
ofRect(400, 200, 200, 100); //x position, y position, width, height
~~~
As before, we set the color before drawing the object. However, in this case, the color is specified with RGB coordinates. Values for the RGB coordinates of colors (at least 24-bit colors) go from 0 to 255 and are given in order. Calling `ofSetColor(0, 255, 0)` sets the drawing color to have no red (0), maximum green (255), and no blue (0). With the color set, `ofRect` draws a rectangle at the given x and y coordinates with a width and height specified in the last two arguments.

Now that we have drawn everything we wanted to, we need to say that we are done drawing into the back buffer and ask for the back buffer to be swapped to the front buffer so that it is actually visible, which is done with
~~~{.cpp}
Disp.endDrawingToBackBuffer();

Disp.swapBuffers();
~~~
With the first line of code, we tell the display that we are done drawing to the back buffer. By calling `swapBuffers`, we tell the display to swap the front and back buffers. Just after `swapBuffers()` is called, the objects should appear on screen. The final line of code before `runExperiment` returns is
~~~{.cpp}
Input.Keyboard.waitForKeypress(-1);
~~~
which just says to wait until any key has been pressed. Once a key has been pressed, control flow will fall off the end of `runExperiment`, causing it to implicitly return, after which the program will exit.

This example shows a number of basic things about how to draw stimuli in CX: 1) Use of the `Disp` object to control the rendering and framebuffer environment and 2) The basics of drawing specific stimuli using openFrameworks' drawing functions. OpenFrameworks has many different kinds of drawing functions for a wide variety of stimuli. A lot of the common functions can be found in ofGraphics.h (http://www.openframeworks.cc/documentation/graphics/ofGraphics.html), but there are a lot of other ways to draw stimuli with openFrameworks: See the graphics and 3d sections of this page: http://www.openframeworks.cc/documentation/. In addition to the many drawing functions of openFrameworks, CX provides a number of drawing functions in the CX::Draw namespace. The renderingTest example contains samples of many of the different kinds of stimuli that can be drawn with CX and openFrameworks.


Time-Locked Visual Stimuli: CX_SlidePresenter
---------------------------------------------

Typically, stimuli should be presented at specific times. CX provides a helpful class that controls stimulus timing for you, called the CX_SlidePresenter. Examples of the use of a CX_SlidePresenter can be found in the nBack and changeDetection examples. In particular, the nBack example goes into some depth with advanced features of the CX_SlidePresenter. However, we will start with examples of basic use of the slide presenter. In the example, we will present the same circle and rectangle that we drew above, but this time, we will present them in a time-locked sequence. The full example:

~~~{.cpp}
#include "CX.h"

CX_SlidePresenter slidePresenter;

void runExperiment(void) {
	
	slidePresenter.setup(&Disp);

	slidePresenter.beginDrawingNextSlide(3000);
		ofBackground(ofColor::black);
		ofSetColor(ofColor::red);
		ofCircle(200, 300, 100);
	slidePresenter.endDrawingCurrentSlide();

	slidePresenter.beginDrawingNextSlide(1000);
		ofBackground(ofColor::black);
		ofSetColor(0, 255, 0);
		ofRect(400, 200, 200, 200);
	slidePresenter.endDrawingCurrentSlide();

	slidePresenter.beginDrawingNextSlide(1);
		ofBackground(ofColor::black);
		ofSetColor(ofColor::red);
		ofCircle(200, 300, 100);
		ofSetColor(0, 255, 0);
		ofRect(400, 200, 200, 200);
	slidePresenter.endDrawingCurrentSlide();

	slidePresenter.startSlidePresentation();
	while (slidePresenter.isPresentingSlides()) {
		slidePresenter.update();
	}

	Input.Keyboard.waitForKeypress(-1);
}
~~~

On the third line, we instantiate a CX_SlidePresenter and call it `slidePresenter`. Within `runExperiment` we set up `slidePresenter` by giving it a pointer to `Disp` by calling
~~~{.cpp}
slidePresenter.setup(&Disp);
~~~
Using `&Disp` means to get a pointer to `Disp`. There is another setup function for CX_SlidePresenter that takes a CX_SlidePresenter::Configuration `struct`, which allows you to configure the CX_SlidePresenter more thoroughly. For now, we will just use the basic setup function.

With the following code, we will create a new slide in `slidePresenter` and draw stimuli into the slide.
~~~{.cpp}
slidePresenter.beginDrawingNextSlide(3000, "circle"); //slide duration, name
	ofBackground(ofColor::black);
	ofSetColor(ofColor::red);
	ofCircle(200, 300, 100);
slidePresenter.endDrawingCurrentSlide();
~~~
You should recogize the stimulus drawing functions from before. By calling `slidePresenter.beginDrawingNextSlide(1000, "circle")`, we are saying to create a new slide with a duration of 1000 ms and name "circle". The name of a slide is optional and purely to make it easy for you to identify the slide later. Everything that is drawn before `slidePresenter.endDrawingCurrentSlide()` is called will be drawn into the slide. It works this way because for each new slide, the slide presenter makes an offscreen framebuffer (an ofFbo) and sets it up so that everything will be drawn into that framebuffer until `endDrawingCurrentSlide()` is called. Calling `endDrawingCurrentSlide()` is optional in that if you forget to do it, it will be done for you.

We do the same thing to draw the rectangle.
~~~{.cpp}
slidePresenter.beginDrawingNextSlide(1000, "rectangle");
	ofBackground(ofColor::black);
	ofSetColor(0, 255, 0);
	ofRect(400, 200, 200, 200);
slidePresenter.endDrawingCurrentSlide();
~~~
Notice that we need to call `ofBackground()` for every slide, because there is no default background color for newly created slides. Also, we made the duration of this slide to be 2000 ms.

Finally, we make the final slide, which has both objects in it.
~~~{.cpp}
slidePresenter.beginDrawingNextSlide(1);
	ofBackground(ofColor::black);
	ofSetColor(ofColor::red);
	ofCircle(200, 300, 100);
	ofSetColor(0, 255, 0);
	ofRect(400, 200, 200, 200);
slidePresenter.endDrawingCurrentSlide();
~~~
Notice that the duration of the final slide is set to 1 ms, yet when the example is run, the final slide stays on sreen indefinitely. This is correct behavior: The final slide that is created is the finishing point for the slide presenter. As soon as the last slide is presented, the slide presentation is done. The logic of it is that it is not possible for the slide presenter to know what should be presented after the last slide. Should the screen turn black? Should a test pattern be presented? There is no obvious default. For this reason, after the last slide is presented, the slide presenter can't sensibly replace that slide with anything else, so it remains on screen until other code draws something else. Thus, the duration of the last slide is ignored and as soon as the last slide is presented, the slide presenter is done.

Once all of the slides have been drawn, to present the slides, the following code is used
~~~{.cpp}
slidePresenter.startSlidePresentation();
while (slidePresenter.isPresentingSlides()) {
	slidePresenter.update();
}
~~~
On the first line, the slide presentation is started. While a slide presentation is in progress, the slide presenter needs to be updated regularly. The reason for this is that every time the slide presenter is updated, it checks to see if the next stimulus should be drawn to the screen. If it is not updated regularly, it could miss a stimulus start time. For this reason, in the while loop, all we do is update the slide presenter with `slidePresenter.update();`. The condition in the loop is just checking to see if the slide presentation is in progress. If so, it keeps looping. At some point, all of the slides will have been presented and the loop will end.

While the slide presentation is in progress, we may want to do other things as well. We can do so by adding these other tasks to the `while` loop. During the slide presentation loop, you can check to see what slide was the last slide to be presented with CX_SlidePresenter::getLastPresentedSlideName(). Using that function, you can do some kinds of synchronization, such as synchronizing sound stimuli with the visual stimuli. Additionally, we could insert
~~~{.cpp}
Input.pollEvents();
~~~
into the loop to check for new input regularly. If you call CX_SlidePresenter::presentSlides(), it does a standard slide presentation, including polling for input regularly. 

At the end of the example, we wait for any key press before exiting.
~~~{.cpp}
Input.Keyboard.waitForKeypress(-1);
~~~

The CX_SlidePresenter is a very useful class that does away with most of the difficult aspects of presenting time-locked visual stimuli. You should probably never present time-locked visual stimuli without using a CX_SlidePresenter or another similar mechanism. The one exception is if you are presenting a long animation sequence in which the scene changes on every frame, especially if the change occurs in response to user input. In that case, using a slide presenter is unweildly and you should probably just write a loop in which the animation is updated every frame. See the animation example for one way of doing an animation without blocking in the main thread.


Vertical Synchronization
------------------------

An aspect of visual stimulus presentation that is important is vertical synchronization. Vertical synchronization (Vsync) is the process by which the swaps of the front and back buffers are synchronized to the refreshes of the monitor in order to prevent vertical tearing. Vertical tearing happens when one part of a scene is being drawn onto the monitor while a different scene is copied into the front buffer, causing parts of both scenes to be drawn at once. The "tearing" happens on the monitor where one scene abruptly becomes the other. In order to use Vsync, there must be some control over when the front buffer is drawn to. The ideal process might be that when the user requests a buffer swap the video card waits until the next vertical blank to swap the buffers. Unfortunately, what actually happens is implementation dependent, which makes writing software that will always work properly difficult.

One problem that I have observed is that even with Vsync enabled if there have been no buffer swaps for some time (several screen refresh periods), buffer swaps can happen more quickly than expected. For example, if the buffers have not been swapped for 2.5 refresh periods and a buffer swap is requested, the buffer swap function can return immediately, not waiting until 3 refresh periods have passed to queue the swap. One process that could explain this is if when the user requests a buffer swap, if at least one vertical blank has passed since the last buffer swap, the buffers are swapped immediately. This can cause problems if the surrounding code is expecting the buffer swap to wait until the next refresh has occured to return. One possible solution to this is, after a buffer swap has been requested, to tell OpenGL to wait until all ongoing processes have completed before continuing. This can be done with CX::CX_Display::waitForOpenGL() and results in a kind of "software" Vsync, as opposed to the "hardware" Vsync that is done by OpenGL internally. Calling the buffer swap function and then CX::CX_Display::waitForOpenGL() works sometimes, but it isn't perfect. On some systems, this will result in a wait of two frame periods before continuing (don't ask me why). On other systems, it works just fine. Other times, it does nothing to fix the problem. You can turn on hardware or software Vsync with CX::CX_Display::useHardwareVSync() and CX::CX_Display::useSoftwareVSync().

How do you check to see if you are having problems with video presentation that are related to Vsync? One way to check is to use CX::CX_Display::testBufferSwapping(), which tests buffer swapping under few of conditions and provides a summary of results along with raw data from the test. A part of the test is a visual one, in which you should be able to visually discriminate between correct and erroneous behavior. Even though errors occur one a time scale that is not normally perceptable, the structure of the visual displays should allow most people to recognize errors if they know what to look for, which is explained in the documentation for the function.

Another way to check to see if you are having problems with Vsync is to use a feature of CX::CX_SlidePresenter to learn about the timing of your stimuli. CX::CX_SlidePresenter::printLastPresentationInformation() provides a lot of timing information related to slide presentation so that you can check for errors easily. The errors can take the form of incorrect slide durations or frame counts (depending on presentation mode). If slides are consistently not started at the intended start time but the copy to the back buffer is happening in time, the most likely culprit is that something strange is going on with Vsync. You can also try different buffer swapping modes of CX::CX_SlidePresenter (see \ref CX::CX_SlidePresenter::SwappingMode). One of the swapping modes (MULTI_CORE) swaps the buffers every frame in a secondary thread which avoids issues that arise from not swapping the buffers every frame. However, this mode can really only be used effectively with a 2+ core CPU, so if you are working with old computers, this may not be for you.

If you have tried CX_Display::useHardwareVSync() and it does not appear to do anything, one option to help deal with Vsync issues is to force Vsync on or off in your video card driver. Modern AMD and Nvidia drivers allow you to force Vsync on or off for specific applications or globally, which in my experience seems to be more reliable than turning Vsync on or off from within CX. If you force Vsync to a setting in the video card driver, CX::CX_Display::useHardwareVSync() will probably not do anything, but CX::CX_Display::useSoftwareVSync() possibly would still do something (although it is not clear that you would want to have both hardware and software Vsync enabled at the same time). CX_Display::testBufferSwapping() includes a test with both kinds of Vsync enabled simultaneously, so you can check to see if it is working correctly.

If you are experiencing problems with Vsync in windowed mode but not in full screen mode, you shouldn't worry. Vsync does not work properly in windowed mode in most modern operating systems due to the way in which they do window compositing. This is a good reason to always run experiments in full screen mode.




