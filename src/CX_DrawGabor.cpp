#include "CX_DrawGabor.h"


#define STRINGIFY(x) #x

//----------
// Simple vertex shader that does very little.
static std::string plainVert = "#version 150\n" STRINGIFY(
uniform mat4 modelViewProjectionMatrix;
in vec4 position;

in vec2 texcoord;

out vec2 texCoordVarying;

void main(){
	texCoordVarying = texcoord;
	gl_Position = modelViewProjectionMatrix * position;
}
);

//----------
// First part of the gabor shader program. Basically naming variables.
static std::string gaborPrelude = "#version 150\n" STRINGIFY(

uniform float relativeYMultiple;

uniform vec2 gaborCenter;

uniform float lineA;
uniform float lineC;
uniform float lineMult;
uniform float inverseWavelength;

uniform float envelopeCP;

uniform vec4 color1;
uniform vec4 color2;

out vec4 outputColor;
);

//----------
// Main part of the gabor shader program.
static std::string gaborMain = STRINGIFY(
void main() {
	float px = gl_FragCoord.x - gaborCenter.x;
	float py = gl_FragCoord.y - gaborCenter.y;
	py = py * relativeYMultiple;


	float distFromLine = abs(lineA * px + py + lineC) * lineMult; // implicitly, B == 1.
	float waveformPosition = mod(distFromLine * inverseWavelength, 1);

	float colorProportion = waveformFunction(waveformPosition);

	float r = colorProportion * color1[0] + (1 - colorProportion) * color2[0];
	float g = colorProportion * color1[1] + (1 - colorProportion) * color2[1];
	float b = colorProportion * color1[2] + (1 - colorProportion) * color2[2];

	float distFromCenter = distance(gl_FragCoord.xy, gaborCenter.xy);

	float alpha = envelopeFunction(distFromCenter, envelopeCP);

	outputColor = vec4(r, g, b, alpha);
}
);


namespace CX {
namespace Draw {

//Function bodies for various wave and envelope functions.
//wp is the waveform position, from 0 to 1.
std::string Gabor::Wave::saw = "return wp;";
std::string Gabor::Wave::sine = "return (sin(wp * 6.283185307179586232) + 1) / 2;";
std::string Gabor::Wave::square = "if (wp < 0.5) return 1; \n return 0;";
std::string Gabor::Wave::triangle = "if (wp < .5) return (2 * wp); \n return 2 - (2 * wp);";

//d is the distance from the center and cp is the control parameter (provided by the user).
std::string Gabor::Envelope::none = "return 1;";
std::string Gabor::Envelope::circle = "if (d <= cp) return 1; \n return 0;";
std::string Gabor::Envelope::linear = "if (d > cp) return 0; \n return 1 - (d / cp);";
std::string Gabor::Envelope::cosine = "if (d >= cp) return 0;\n return (cos(d / cp * PI) + 1) / 2;";
std::string Gabor::Envelope::gaussian = "return exp(-(d * d) / (2 * (cp * cp)));";

Gabor::Gabor(void)
{
	_initVars();
}

/*! Convenience constructor which sets up the class while constructing it. */
Gabor::Gabor(std::string waveFunction, std::string envelopeFunction) {
	_initVars();
	setup(waveFunction, envelopeFunction);
}

/*! Set up the gabor to use certain wave and envelope functions. This is a
special setup step because changing the functions changes the source code
of the fragment shader used to draw the gabor, so it has to be recompiled.
This is a potentially blocking function.

\param waveFunction A function to use to calculate the mixing between color1
and color2. Most users should use a value from Gabor::Wave. Advanced users
can write their own function using GLSL.

\param envelopeFunction A function to use to calculate the envelope giving
the falloff of the gabor from the center of the pattern. Most users should
use a value from Gabor::Envelope. Advanced users can write their own function
using GLSL.
*/
void Gabor::setup(std::string waveFunction, std::string envelopeFunction) {
	std::string fullWaveFunction = "float waveformFunction(in float wp) {\n" +
		waveFunction +
		"\n}\n";

	std::string fullEnvelopeFunction = "float envelopeFunction(in float d, in float cp) {\n" +
		envelopeFunction +
		"\n}\n";

	std::string source = gaborPrelude + fullWaveFunction + fullEnvelopeFunction + gaborMain;

	_shader.setupShaderFromSource(GL_VERTEX_SHADER, plainVert);
	_shader.setupShaderFromSource(GL_FRAGMENT_SHADER, source);

	if (ofIsGLProgrammableRenderer()){
		_shader.bindDefaults();
	}
	_shader.linkProgram();
}

/*! Draw the gabor given the current settings */
void Gabor::draw(void) {
	_draw(center, fboHeight);
}

/*! Draw the gabor, setting a new location for it. 
\param newX The new x coordinate of the center of the gabor.
\param newY The new y coordinate of the center of the gabor. */
void Gabor::draw(float newX, float newY) {
	draw(ofPoint(newX, newY));
}

/*! Draw the gabor, setting a new location for it.
\param newCenter The new center of the gabor. */
void Gabor::draw(ofPoint newCenter) {
	center = newCenter;
	draw();
}

/*! Draw the gabor, setting a new location for it and new fboHeight.
\param newCenter The new center of the gabor.
\param newFboHeight The new \ref CX::Draw::Gabor::fboHeight. */
void Gabor::draw(ofPoint newCenter, float newFboHeight) {
	center = newCenter;
	fboHeight = newFboHeight;
	draw();
}

/*! \brief Get a reference to the ofShader used by this class. Use this only if
you want to do advanced things directly with the shader. */
ofShader& Gabor::getShader(void) {
	return _shader;
}

void Gabor::_setUniforms(void) {
	
	struct {
		float A; //!< Used for calculating distance from a line. The x coefficient.
		float C; //!< Used for calculating distance from a line. The negative intercept.
		float multiplier; //!< Used for calculating distance from a line. 1 over a scale factor.

		float inverseWavelength; //!< 1 over the wavelength (i.e. the frequency).
	} waveValues;

	

	GLint fb;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &fb);
	bool drawingToBackBuffer = (fb == 0);

	//bool yIncreasesUpwards = CX::Instances::Disp.getYIncreasesUpwards();
	bool yIncreasesUpwards = false;

	ofPoint modCenter = center;
	float relativeYMultiple = 1;

	float framebufferHeight = this->fboHeight;
	if (framebufferHeight < 0) {
		framebufferHeight = CX::Instances::Disp.getResolution().y;
	}

	if (drawingToBackBuffer) {
		if (!yIncreasesUpwards) {
			modCenter.y = framebufferHeight - center.y;
			relativeYMultiple = -1;
		}
	} else {
		if (yIncreasesUpwards) {
			modCenter.y = framebufferHeight - center.y;
			relativeYMultiple = -1;
		}
	}

	float angle = ofDegToRad(wave.angle);

	float slope = tan(angle);

	float waveformPosition = wave.wavelength * fmod(wave.phase, 360.0) / 360.0;

	//Get point on line tangent to "radius" of rectangle and the intercept of the line passing through that point
	float tanRadius = radius;
	//Make the tanRadius be the next greatest multiple of the period
	tanRadius = (ceil(tanRadius / wave.wavelength) * wave.wavelength) + waveformPosition;
	ofPoint tangentPoint(tanRadius * sin(PI - angle), tanRadius * cos(PI - angle));
	float intercept = tangentPoint.y - (slope * tangentPoint.x);

	waveValues.A = -slope;
	waveValues.C = -intercept;
	waveValues.multiplier = 1 / sqrt(pow(waveValues.A, 2) + 1);

	waveValues.inverseWavelength = 1 / wave.wavelength;


	_shader.setUniform1f("relativeYMultiple", relativeYMultiple);

	_shader.setUniform2f("gaborCenter", modCenter.x, modCenter.y);

	_shader.setUniform1f("lineA", waveValues.A);
	_shader.setUniform1f("lineC", waveValues.C);
	_shader.setUniform1f("lineMult", waveValues.multiplier);
	_shader.setUniform1f("inverseWavelength", waveValues.inverseWavelength);

	_shader.setUniform1f("envelopeCP", envelope.controlParameter);

	_shader.setUniform4f("color1", color1.r, color1.g, color1.b, color1.a);
	_shader.setUniform4f("color2", color2.r, color2.g, color2.b, color2.a);
}

void Gabor::_draw(ofPoint center, float renderSurfaceHeightPx) {
	_shader.begin();

	_setUniforms();
	ofDrawCircle(center, radius);

	//ofRect(x, y, width, height); //Draw a rect containing the gabor

	_shader.end();
}

void Gabor::_initVars(void) {
	color1 = ofColor(255);
	color2 = ofColor(0);
	radius = 400;

	envelope.controlParameter = 100;
	wave.angle = 0;
	wave.phase = 0;
	wave.wavelength = 30;

	fboHeight = -1;
}





/*! This function draws a two-dimensional waveform pattern to an ofFloatPixels objects.
The results of this function are not intended to be used directly, but to be applied
to an image, for example. The pattern lacks color information, but can be used as an alpha mask,
used to control color mixing, or otherwise.
\param properties The properties that will be used to create the pattern.
\return An ofFloatPixels object containing the pattern.
*/
ofFloatPixels waveformToPixels(const WaveformProperties& properties) {

	ofFloatPixels pix;
	unsigned int width = ceil(properties.width);
	unsigned int height = ceil(properties.height);
	pix.allocate(width, height, ofImageType::OF_IMAGE_GRAYSCALE);


	float theta = properties.angle * PI / 180;
	float slope = tan(theta);

	float waveformPosition = properties.wavelength * fmod(properties.phase, 360.0) / 360.0;

	//Get point on line tangent to "radius" of rectangle and the interecept of the line passing through that point
	float tanRadius = sqrt(width * width + height * height);
	//Make the tanRadius be the next greatest multiple of the period
	tanRadius = (ceil(tanRadius / properties.wavelength) * properties.wavelength) + waveformPosition;
	ofPoint tangentPoint(tanRadius * sin(PI - theta), tanRadius * cos(PI - theta));
	float intercept = tangentPoint.y - (slope * tangentPoint.x);

	ofPoint center = ofPoint(properties.width / 2, properties.height / 2);

	float inverseWavelength = 1 / properties.wavelength;

	float A = -slope;
	//float B = 1;
	float C = -intercept;
	float mult = 1 / sqrt(A * A + 1 * 1);

	for (size_t yi = 0; yi < pix.getHeight(); yi++) {
		for (size_t xi = 0; xi < pix.getWidth(); xi++) {

			//Center so that x and y are relative to the origin.
			float px = xi - center.x;
			float py = yi - center.y;

			float distFromLine = (A*px + py + C) * mult; //B == 1

			float wavePos = fmod(distFromLine * inverseWavelength, 1);
			float intensity = properties.waveFunction(wavePos);

			intensity = CX::Util::clamp<float>(intensity, 0, 1);

			size_t index = xi + yi * pix.getWidth();
			pix[index] = intensity;
		}
	}

	return pix;
}


/*! Produces a sine wave.
\param wp The waveform position in the interval [0,1).
\return A sinusoidal value in the range [0,1], depending on the waveform position. */
float WaveformProperties::sine(float wp) {
	return (sin(wp * TWO_PI) + 1) / 2;
}

/*! Produces a square wave.
\param wp The waveform position in the interval [0,1).
\return A 0 or 1, depending on the waveform position. */
float WaveformProperties::square(float wp) {
	if (wp < .5) {
		return 1;
	}
	return 0;
}

/*! Produces a triangle wave.
\param wp The waveform position in the interval [0,1).
\return A value in the range [0,1], depending on the waveform position. */
float WaveformProperties::triangle(float wp) {
	if (wp < .5) {
		return (2 * wp);
	}
	return 2 - (2 * wp);
}

/*! Produces a saw wave.
\param wp The waveform position in the interval [0,1).
\return A value in the range [0,1], depending on the waveform position. */
float WaveformProperties::saw(float wp) {
	return wp;
}

/*! Draws a two-dimensional envelope to an ofFloatPixels. An example of how this can be used
is to create the alpha blending falloff effect seen in gabor patches as they fade out toward
their edges. There is only a single channel in the pixels, which can be used for alpha blending
or other kinds of blending effects. Because the type of the color that is used is ofFloatColor,
you can access the value of each pixel like this:
\code{.cpp}
ofFloatPixels result = Draw::envelopeToPixels(properties); //Get the pixels
float level = result.getColor(1,2).getBrightness(); //where 1 and 2 are some x and y coordinates
\endcode

\param properties The properties of the envelope.
\return An ofFloatPixels containing the envelope.
*/
ofFloatPixels envelopeToPixels(const EnvelopeProperties& properties) {
	ofFloatPixels pix;

	pix.allocate(ceil(properties.width), ceil(properties.height), ofImageType::OF_IMAGE_GRAYSCALE);

	ofPoint center = ofPoint(properties.width / 2, properties.height / 2);

	for (size_t y = 0; y < pix.getHeight(); y++) {
		for (size_t x = 0; x < pix.getWidth(); x++) {

			float d = center.distance(ofPoint(x, y));

			float amount = properties.envelopeFunction(d, properties.controlParameter);

			amount = CX::Util::clamp<float>(amount, 0, 1);

			size_t index = x + y * pix.getWidth();
			pix[index] = amount;
		}
	}

	return pix;
}

/*! Does nothing to affect the wave pattern.
\param d The distance.
\param cp The control parameter.
\return 1, regardless of the inputs. */
float EnvelopeProperties::none(float d, float cp) {
	return 1;
}

/*! Creates a hard clipped circle.
\param d The distance.
\param cp The control parameter, interpreted as a radius.
\return 1 if `d <= cp`, 0 otherwise. */
float EnvelopeProperties::circle(float d, float cp) {
	if (d <= cp) {
		return 1;
	}
	return 0;
}

/*! Creates linearly decreasing values up to a radius set by `cp`.
\param d The distance.
\param cp The control parameter, interpreted as a radius.
\return `1 - (d / cp)` if `d <= cp`, 0 otherwise. */
float EnvelopeProperties::linear(float d, float cp) {
	if (d <= cp) {
		return 1 - (d / cp);
	}
	return 0;
}

/*! Creates values that decrease with a cosine shape as `d` increases.
\param d The distance.
\param cp The control parameter, interpreted as a radius.
\return A value that drops off with a cosine shape as d increases up to cp, beyond which this returns 0. */
float EnvelopeProperties::cosine(float d, float cp) {
	if (d < cp) {
		return (cos(PI * d / cp) + 1) / 2;
	}
	return 0;
}

/*! Creates values that decrease with a gaussian shape as `d` increases.
\param d The distance.
\param cp The control parameter, interpreted as the standard deviation of a gaussian distribution.
\return A value from a gaussian kernel for deviate `d` with mean 0 and standard deviation `cp`. */
float EnvelopeProperties::gaussian(float d, float cp) {
	return exp(-(d * d) / (2 * (cp * cp)));
}

/*! Just like Draw::gabor(ofPoint, const GaborProperties&), except that instead of drawing the
pattern, it returns it in an ofFloatPixels object.
\param properties The settings to be used to generate the pattern.
\return An ofFloatPixels containing the gabor pattern. It cannot be drawn directly, but can
be put into an ofTexture and drawn from there, for example.
*/
ofFloatPixels gaborToPixels(const GaborProperties& properties) {

	WaveformProperties waveProp = properties.wave;
	waveProp.width = properties.width;
	waveProp.height = properties.height;
	ofFloatPixels wave = Draw::waveformToPixels(waveProp);

	EnvelopeProperties envProp = properties.envelope;
	envProp.width = properties.width;
	envProp.height = properties.height;
	ofFloatPixels envelope = Draw::envelopeToPixels(envProp);

	return gaborToPixels(properties.color1, properties.color2, wave, envelope);
}

/*! This version of gaborToPixels uses precalculated waves and envelopes. This can save time. However, if
speed is the primary concern, the class \ref CX::Draw::Gabor should be used instead.

\param color1 The first color of the waves.
\param color2 The second color of the waves.
\param wave A precalculated waveform pattern. Must only have a single channel of color data (i.e. is greyscale).
\param envelope A precalculated envelope. Must only have a single channel of color data (i.e. is greyscale).
\return An ofFloatPixels containing the gabor pattern. It cannot be drawn directly, but can
be put into an ofTexture and drawn from there, for example.
*/
ofFloatPixels gaborToPixels(ofColor color1, ofColor color2, const ofFloatPixels& wave, const ofFloatPixels& envelope) {

	ofFloatPixels pix;

	if (wave.getNumChannels() != 1 || envelope.getNumChannels() != 1) {
		CX::Instances::Log.error("Draw") << "gaborToPixels(): The wave and envelope must only have a single channel"
			" of color data each (i.e. they should be greyscale).";
		return pix;
	}

	if (wave.getWidth() != envelope.getWidth() || wave.getHeight() != envelope.getHeight()) {
		CX::Instances::Log.warning("Draw") << "gaborToPixels(): The wave and envelope are not the same dimensions."
			" The minimum of both will be used.";
	}

	int width = ceil(std::min(wave.getWidth(), envelope.getWidth()));
	int height = ceil(std::min(wave.getHeight(), envelope.getHeight()));

	
	pix.allocate(width, height, ofImageType::OF_IMAGE_COLOR_ALPHA);

	for (size_t x = 0; x < pix.getHeight(); x++) {
		for (size_t y = 0; y < pix.getWidth(); y++) {

			size_t index = x + y * pix.getWidth();

			float waveProportion = wave[index];
			ofFloatColor lerped = color1.getLerped(color2, waveProportion);

			float envelopeProportion = envelope[index];
			lerped.a = envelopeProportion;

			pix.setColor(x, y, lerped);
		}
	}

	return pix;
}

/*! Just like Draw::gabor(ofPoint, const GaborProperties&), except that instead of drawing the
pattern, it returns it in an ofTexture object. */
ofTexture gaborToTexture(const GaborProperties& properties) {
	ofPixels pix = gaborToPixels(properties);
	ofTexture tex;
	tex.allocate(pix);
	tex.loadData(pix);
	return tex;
}

/*! Just like Draw::gabor(ofPoint, ofColor, ofColor, const ofFloatPixels&, const ofFloatPixels&), 
except that instead of drawing the pattern, it returns it in an ofTexture object. */
ofTexture gaborToTexture(ofColor color1, ofColor color2, const ofFloatPixels& wave, const ofFloatPixels& envelope) {
	ofPixels pix = gaborToPixels(color1, color2, wave, envelope);
	ofTexture tex;
	tex.allocate(pix);
	tex.loadData(pix);
	return tex;
}

/*! Draws a gabor pattern with the specified properties. See the renderingTest example for
an example of the use of this function.
\param center The location of the center of the pattern.
\param properties The settings to be used to generate the pattern.
\see \ref CX::Draw::Gabor for a more computationally efficient way to draw gabors.
*/
void gabor(ofPoint center, const GaborProperties& properties) {
	ofTexture tex = gaborToTexture(properties);

	ofSetColor(255);
	tex.draw(center.x - tex.getWidth() / 2, center.y - tex.getHeight() / 2); //Draws centered
}

/*! This version of gabor() uses precalculated waves and envelopes. This can save time. However, if
speed is the primary concern, the class \ref CX::Draw::Gabor should be used instead.

\param center The location of the center of the pattern.
\param color1 The first color of the waves.
\param color2 The second color of the waves.
\param wave A precalculated waveform pattern. Must only have a single channel of color data(i.e.is greyscale).
\param envelope A precalculated envelope. Must only have a single channel of color data(i.e.is greyscale).
\return An ofFloatPixels containing the gabor pattern.It cannot be drawn directly, but can
be put into an ofTexture and drawn from there, for example.
*/
void gabor(ofPoint center, ofColor color1, ofColor color2, const ofFloatPixels& wave, const ofFloatPixels& envelope) {
	ofTexture tex = gaborToTexture(color1, color2, wave, envelope);

	ofSetColor(255);
	tex.draw(center.x - tex.getWidth() / 2, center.y - tex.getHeight() / 2); //Draws centered
}




} //namespace Draw
} //namespace CX