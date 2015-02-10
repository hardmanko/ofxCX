#pragma once

#include "ofShader.h"
#include "ofGraphics.h"

#include "CX_Display.h"

namespace CX {
namespace Draw {

/*! This class draws gabor patches using hardware acceleration to speed up the process.
Compared to the loose functions, like CX::Draw::gabor(), this class is preferable
from a speed perspective, but it is slightly harder to use and not as flexible.
You use it by calling the setup function to specify some basic information about the
gabor, setting a number of data members of the class to certain values, and calling
the draw function. For example:

\code{.cpp}
#include "CX.h"

void runExperiment(void) {
	Draw::Gabor gabor; //Make an instance of the Gabor class.

	//Do basic setup for the gabor by setting the wave and envelope functions.
	gabor.setup(Draw::Gabor::Wave::sine, Draw::Gabor::Envelope::gaussian);

	gabor.envelope.controlParameter = 50; //Set the control parameter for the envelope (in this case, standard deviation).

	gabor.wave.wavelength = 30; //Set the wavelength of the waves, in pixels.
	gabor.wave.angle = 30; //Set the angle of the waves.

	gabor.color1 = ofColor::green; //Choose the two colors to alternate between.
	gabor.color2 = ofColor::red;

	Disp.beginDrawingToBackBuffer();

	ofBackground(127);

	gabor.draw(Disp.getCenter());

	Disp.endDrawingToBackBuffer();
	Disp.swapBuffers();

	Input.Keyboard.waitForKeypress(-1);
	}
\endcode

Advanced users:
The Gabor class is meant to be somewhat extensible, so that you
can add your own wave and envelope functions. To do so, you will need
to write a function body that calculates wave amplitudes and envelope
amounts using the OpenGL Shading Language (GLSL). These functions will
be called for every pixel that is drawn and will be given various pieces
of data that will help them calculate the resulting value.

The waveform function has the following type signature:

	float waveformFunction(in float wp)

where `wp` is the current position, in the interval [0,1), for the
waveform that you are calculating the amplitide for. The return value
is the amplitude of the wave at `wp` and should be in the interval [0,1].
An example of a function body that you could use to generate sine waves is

	return (sin(wp * 6.283185307179586232) + 1) / 2;

where the returned value is scaled to be in the interval [0,1] instead of [-1,1].

The envelope function has the following type signature:

	float envelopeFunction(in float d, in float cp)

where `d` is the distance from the center of the gabor patch and `cp` is
the control parameter, which the user can set by modifying
Gabor::envelope::controlParameter. The function returns a value in
the interval [0,1] that is interpreted as the alpha for the color that is
set for the current pixel. For example, for a circular envelope, the alpha
is fully opaque for pixels within the radius and fully transparent for
pixels outside of the radius, so a function body might be:

	if (d <= cp) return 1;
	return 0;

Due to how GLSL works, these function bodies can be written as strings
in C++ source code and passed to the GLSL compiler as strings. In this
case, you just need to pass the function bodies to Gabor::setup().

*/
class Gabor {
public:

	/*! This struct contains several functions that are used for calculating
	the mixing between color1 and color2. */
	struct Wave {
		static std::string saw; //!< Produces a saw wave.
		static std::string sine; //!< Produces a sine wave.
		static std::string square; //!< Produces a square wave.
		static std::string triangle; //!< Produces a triangle wave.
	};

	/*! This struct contains several functions that used for calculating
	the envelope containing the gabor patch (e.g. a fall-off away from the center). */
	struct Envelope {
		static std::string none; //!< Does nothing to affect the wave pattern.
		static std::string circle; //!< Creates a circle, clipped at a radius set by the control parameter.
		static std::string linear; //!< Creates linearly decreasing values up to a radius set by the control parameter.
		/*! \brief Creates values that decrease with a cosine shape as distance increases,
		depending on the control parameter for a radius. */
		static std::string cosine;
		/*! \brief Creates values that decrease with a gaussian shape as distance increases,
		where the control parameter sets the standard deviation. */
		static std::string gaussian;
	};

	Gabor(void);
	Gabor(std::string waveFunction, std::string envelopeFunction);

	void setup(std::string waveFunction, std::string envelopeFunction);

	void draw(void);
	void draw(float newX, float newY);
	void draw(ofPoint newCenter);
	void draw(ofPoint newCenter, float fboHeight);

	ofShader& getShader(void);

	ofPoint center; //!< The center of the gabor.

	/*! The maximum radius of the gabor. The should generally be larger than the (visible) edge of the 
	envelope that is used. If you have an envelope that should have a smooth (or blended) edge but are 
	seeing a hard-clipped edge, you should try increasing the radius. */
	float radius;

	/*! \brief If drawing the gabor into a framebuffer that has a different height than the main window, 
	use this to set the	height of that framebuffer. If this is less than 0, the height of the current 
	framebuffer will be	inferred to be the height of the main window. */
	float fboHeight;

	ofFloatColor color1; //!< The first color used in the waveforms. There is no meaning to order to the colors.
	ofFloatColor color2; //!< The second color used in the waveforms. There is no meaning to order to the colors.

	struct {
		float angle; //!< The angle at which the waves are oriented, in degrees.
		float wavelength; //!< The distance, in pixels, between the center of each wave within the pattern.
		float phase; //!< The phase shift of the waves, in degrees.
	} wave; //!< Settings for the waveforms.

	struct {
		float controlParameter; //!< Control parameter for the envelope generating function.
	} envelope; //!< Settings for the envelope.

private:

	void _initVars(void);
	ofShader _shader;

	void _draw(ofPoint center, float renderSurfaceHeightPx);

	void _setUniforms(void);

};


/*! Controls the properties of a waveform drawn with CX::Draw::waveformToPixels(). */
struct WaveformProperties {
	WaveformProperties(void) :
		width(-1),
		height(-1),
		angle(0),
		wavelength(30),
		phase(0),
		waveFunction(WaveformProperties::sine)
	{}

	/*! The width of the pattern, in pixels. */
	float width;

	/*! The height of the pattern, in pixels. */
	float height;

	/*! The angle at which the waves are oriented, in degrees. */
	float angle;

	float wavelength; //!< The distance, in pixels, between the center of each wave within the pattern.
	float phase; //!< The phase shift of the waves, in degrees.

	/*! A function that calculates the height of the wave given a waveform position.
	It should take the current waveform position as a value in the interval [0,1) and
	return the relative height of the wave as a value in the interval [0,1].
	See the static functions in this struct, like sine(), square(), etc. for some options.
	*/
	std::function<float(float)> waveFunction;

	static float sine(float wp);
	static float square(float wp);
	static float triangle(float wp);
	static float saw(float wp);
};

/*! This struct controls the properties of an envelope created with CX::Draw::envelopeToPixels().
The type of the envelope is specified with the \ref envelopeFunction member and depending on the
function that is used, \ref controlParameter can quantitatively affect the envelope.
*/
struct EnvelopeProperties {
	EnvelopeProperties(void) :
		width(-1),
		height(-1),
		envelopeFunction(EnvelopeProperties::none),
		controlParameter(10)
	{}

	float width; //!< The width of the envelope, in pixels.
	float height; //!< The height of the envelope, in pixels.

	/*! A function used to generate the envelope. Can be one of the static functions of this struct
	or some user defined function. The first argument it takes is the distance in pixels from the
	center of the envelope (depend on the width and height). The second argument is the
	\ref controlParameter, which is set by the user. The function should return a value in the
	interval [0,1]. */
	std::function<float(float, float)> envelopeFunction;

	/*! A parameter that controls the envelope in different ways, depending on the envelope function.
	This is passed as the second argument to \ref envelopeFunction() each time it is called. */
	float controlParameter;

	static float none(float d, float cp);
	static float circle(float d, float cp);
	static float linear(float d, float cp);
	static float cosine(float d, float cp);
	static float gaussian(float d, float cp);
};

/*! Draws a gabor patch with two colors that are used for the peaks and troughs of the waves
plus an envelope that smooths the edges of the patch. The waves are specified with the \ref wave
argument and the envelope with the \ref envelope argument.

The width and height of the wave and envelope do not need to be directly specified as their
values are taken from the width and height members of this struct.
*/
struct GaborProperties {
	GaborProperties(void) :
		width(100),
		height(100),
		color1(255, 255, 255, 255),
		color2(0, 0, 0, 255)
	{}

	float width; //!< The width of the gabor patch.
	float height; //!< The height of the gabor patch.

	ofColor color1; //!< The first color.
	ofColor color2; //!< The second color.

	WaveformProperties wave; //!< Parameters controlling the waveform used to create the gabor patch.
	EnvelopeProperties envelope; //!< Parameters controlling the envelope used to contain the waves.
};

ofFloatPixels waveformToPixels(const WaveformProperties& properties);
ofFloatPixels envelopeToPixels(const EnvelopeProperties& properties);

ofFloatPixels gaborToPixels(const GaborProperties& properties);
ofFloatPixels gaborToPixels(ofColor color1, ofColor color2, const ofFloatPixels& wave, const ofFloatPixels& envelope);
ofTexture gaborToTexture(const GaborProperties& properties);
ofTexture gaborToTexture(ofColor color1, ofColor color2, const ofFloatPixels& wave, const ofFloatPixels& envelope);
void gabor(ofPoint center, const GaborProperties& properties);
void gabor(ofPoint center, ofColor color1, ofColor color2, const ofFloatPixels& wave, const ofFloatPixels& envelope);



} //namespace Draw
} //namespace CX
