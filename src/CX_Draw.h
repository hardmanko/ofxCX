#pragma once

#include "ofPoint.h"
#include "ofPath.h"
#include "ofTrueTypeFont.h"
#include "ofGraphics.h"
#include "ofShader.h"

#include "CX_Utilities.h"
#include "CX_RandomNumberGenerator.h"
#include "CX_Algorithm.h"

namespace CX {

/*! This namespace contains functions for drawing certain complex stimuli.
These functions are provided "as-is": If what they draw looks nice to you, great;
however, there are no strong guarantees about what the output of the functions will look like.
\ingroup video */
namespace Draw {

	std::vector<double> convertColors(std::string conversionFormula, double S1, double S2, double S3);
	ofFloatColor convertToRGB(std::string inputColorSpace, double S1, double S2, double S3);

	enum class LineCornerMode {
		OUTER_POINT,
		BEZIER_ARC,
		STRAIGHT_LINE,
		ARC //unimplemented
	};
	ofPath lines(std::vector<ofPoint> points, float width, LineCornerMode cornerMode);
	void lines(std::vector<ofPoint> points, float lineWidth);
	void line(ofPoint p1, ofPoint p2, float width);

	void ring(ofPoint center, float radius, float width, unsigned int resolution);
	void arc(ofPoint center, float radiusX, float radiusY, float width, float angleBegin, float angleEnd, unsigned int resolution);

	std::vector<ofPoint> getBezierVertices(std::vector<ofPoint> controlPoints, unsigned int resolution);
	void bezier(std::vector<ofPoint> controlPoints, float width, unsigned int resolution);

	ofPath arrowToPath(float length, float headOffsets, float headSize, float lineWidth);

	ofPath squircleToPath(double radius, double amount = 0.9);
	//void squircle(ofPoint center, double radius, double rotationDeg = 0, double amount = 0.9);

	ofPath starToPath(unsigned int numberOfPoints, float innerRadius, float outerRadius);
	void star(ofPoint center, unsigned int numberOfPoints, float innerRadius, float outerRadius, float rotationDeg = 0);
	std::vector<ofPoint> getStarVertices(unsigned int numberOfPoints, float innerRadius, float outerRadius, float rotationDeg = 0);

	std::vector<ofPoint> getFixationCrossVertices(float armLength, float armWidth);
	ofPath fixationCrossToPath(float armLength, float armWidth);
	void fixationCross(ofPoint location, float armLength, float armWidth);

	void centeredString(int x, int y, std::string s, ofTrueTypeFont &font);
	void centeredString(ofPoint center, std::string s, ofTrueTypeFont &font);

	/*! \class CX_PatternProperties_t
	This structure contains settings controlling the creation of greyscale patterns using CX::Draw::greyscalePatternToPixels().
	The pattern that is created looks like a simple gabor pattern.
	*/
	struct CX_PatternProperties_t {
		CX_PatternProperties_t(void) :
			minValue(0),
			maxValue(255),
			width(100),
			height(100),
			angle(0),
			period(30),
			phase(0),
			maskType(CX_PatternProperties_t::SINE_WAVE),
			apertureType(CX_PatternProperties_t::AP_CIRCLE),
			fallOffPower(std::numeric_limits<float>::min())
		{}

		unsigned char minValue; //!< The minimum value that will be used in the pattern.
		unsigned char maxValue; //!< The maximum value that will be used in the pattern.

		/*! The width of the pattern, or if `apertureType` is `AP_CIRCLE`, the diameter of the circle enclosing the pattern. */
		float width;

		/*! The height of the pattern. ignored if `apertureType` is `AP_CIRCLE`. */
		float height;

		/*! The angle at which the waves are oriented. */
		float angle;

		float period; //!< The distance, in pixels, between the center of each wave within the pattern.
		float phase; //!< The offset, in degrees, of the waves.

		/*! The type of waves that will be used in the pattern. */
		enum {
			SINE_WAVE,
			SQUARE_WAVE,
			TRIANGLE_WAVE
		} maskType;

		/*! Because the pattern created with these settings extends to infinity in every direction,
		an aperture through which it is to be viewed must be specified. The aperture can either be a circle or a rectangle. */
		enum {
			AP_CIRCLE,
			AP_RECTANGLE
		} apertureType;

		/*! The intensity of each pixel is decreased slightly based on how far from the center of the pattern
		that pixel is, depending on the value of `fallOffPower`. By default, there is no falloff. A value of 1
		produces a standard cosine falloff. The falloff is computed as `(cos((d/r)^falloffPower * PI) + 1)/2`,
		where `d` is the distance of the current pixel from the center of the pattern and `r` is the radius of
		the pattern. */
		float fallOffPower;
	};

	struct CX_GaborProperties_t {
		CX_GaborProperties_t(void) :
		color(255, 255, 255, 255)
		{}

		ofColor color;
		CX_PatternProperties_t pattern;
	};

	ofPixels greyscalePatternToPixels(const CX_PatternProperties_t& patternProperties);

	ofPixels gaborToPixels (const CX_GaborProperties_t& properties);
	ofTexture gaborToTexture (const CX_GaborProperties_t& properties);
	void gabor (ofPoint center, const CX_GaborProperties_t& properties);

	void saveFboToFile(ofFbo& fbo, std::string filename);

	/*! Sample colors from the RGB spectrum with variable precision. Colors will be sampled
	beginning with red, continue through yellow, green, cyan, blue, violet, and almost, but not quite, back to red.
	\tparam ofColorType An oF color type. One of: ofColor, ofFloatColor, or ofShortColor, or ofColor_<someOtherType>.
	\param colorCount The number of colors to draw from the RGB spectrum, which will be rounded
	up to the next multiple of 6.
	\return A vector containing the sampled colors with a number of colors equal to colorCount
	rounded up to the next multiple of 6. */
	template <typename ofColorType>
	std::vector<ofColorType> getRGBSpectrum(unsigned int colorCount) {

		unsigned int precision = 1 + (colorCount - (colorCount % 6)) / 6;
		float maxValue = ofFloatColor::limit();

		vector<float> increasingComponents = CX::Util::sequenceAlong<float>(0, maxValue, precision + 1);
		increasingComponents.pop_back();
		vector<float> decreasingComponents = CX::Util::sequenceAlong<float>(maxValue, 0, precision + 1);
		decreasingComponents.pop_back();

		vector<float> redComponents(precision, maxValue);
		redComponents.insert(redComponents.end(), decreasingComponents.begin(), decreasingComponents.end());
		redComponents.insert(redComponents.end(), 2 * precision, 0);
		redComponents.insert(redComponents.end(), increasingComponents.begin(), increasingComponents.end());
		redComponents.insert(redComponents.end(), precision, maxValue);

		vector<float> greenComponents = increasingComponents;
		greenComponents.insert(greenComponents.end(), 2 * precision, maxValue);
		greenComponents.insert(greenComponents.end(), decreasingComponents.begin(), decreasingComponents.end());
		greenComponents.insert(greenComponents.end(), 2 * precision, 0);

		vector<float> blueComponents = Util::repeat<float>(0, precision * 2);
		blueComponents.insert(blueComponents.end(), increasingComponents.begin(), increasingComponents.end());
		blueComponents.insert(blueComponents.end(), 2 * precision, maxValue);
		blueComponents.insert(blueComponents.end(), decreasingComponents.begin(), decreasingComponents.end());

		vector<ofColorType> colors(redComponents.size());
		for (unsigned int i = 0; i < colors.size(); i++) {
			colors[i] = ofFloatColor(redComponents[i], greenComponents[i], blueComponents[i]); //Implicit conversion in assignment
		}

		return colors;
	}



	/*! See CX::Draw::colorArc(ofPoint, std::vector<ofColor_<T>>, float, float, float, float, float) for documentation. 
	The only difference is that this function returns an ofVbo,
	which a complicated thing you can learn about here: http://www.openframeworks.cc/documentation/gl/ofVbo.html 
	The ofFbo is ready to be drawn without any further processing. */
	template <typename T>
	ofVbo colorArcToVbo(ofPoint center, std::vector<ofColor_<T>> colors, float radiusX, float radiusY, float width, float angleBegin, float angleEnd) {
		float d = width / 2;

		angleBegin *= -PI / 180;
		angleEnd *= -PI / 180;

		vector<ofFloatColor> convertedColors(colors.size());
		for (int i = 0; i < convertedColors.size(); i++) {
			convertedColors[i] = colors[i];
		}

		convertedColors = CX::Util::repeat(convertedColors, 1, 2);

		vector<ofPoint> vertices(convertedColors.size(), center);

		unsigned int resolution = vertices.size() / 2;

		for (unsigned int i = 0; i < resolution; i++) {

			float p = (float)i / (resolution - 1);

			float rad = (angleEnd - angleBegin) * p + angleBegin;

			vertices[2 * i] += ofPoint((radiusX + d) * cos(rad), (radiusY + d) * sin(rad));
			vertices[2 * i + 1] += ofPoint((radiusX - d) * cos(rad), (radiusY - d) * sin(rad));
		}

		ofVbo vbo;
		vbo.setVertexData(vertices.data(), vertices.size(), GL_STATIC_DRAW);
		vbo.setColorData(convertedColors.data(), convertedColors.size(), GL_STATIC_DRAW);
		return vbo;
	}

	/*! Draws an arc with specified colors. The precision of the arc is controlled by how many colors are supplied.
	\param center The center of the color wheel.
	\param colors The colors to use in the color arc.
	\param radiusX The radius of the color wheel in the X-axis.
	\param radiusY The radius of the color wheel in the Y-axis.
	\param width The width of the arc. The arc will extend half of the width
	in either direction from the radii.
	\param angleBegin The angle at which to begin the arc, in degrees.
	\param angleEnd The angle at which to end the arc, in degrees. If the arc goes in the "wrong" direction, try giving a negative value for `angleEnd`.
	*/
	template <typename T>
	void colorArc(ofPoint center, std::vector<ofColor_<T>> colors, float radiusX, float radiusY, float width, float angleBegin, float angleEnd) {
		ofVbo vbo = colorArcToVbo(center, colors, radiusX, radiusY, width, angleBegin, angleEnd);
		vbo.draw(GL_TRIANGLE_STRIP, 0, vbo.getNumVertices());
	}

    /*! See CX::Draw::colorWheel(ofPoint, std::vector<ofColor_<T>>, float, float, float) for documentation. 
	The only difference is that this function returns an ofVbo,
	which a complicated thing you can learn about here: http://www.openframeworks.cc/documentation/gl/ofVbo.html 
	The ofFbo is ready to be drawn without any further processing. */
	template <typename T>
	ofVbo colorWheelToVbo(ofPoint center, std::vector<ofColor_<T>> colors, float radius, float width, float angle) {
		colors.push_back(colors.front());

		return colorArcToVbo(center, colors, radius, radius, width, angle, angle - 360);
	}

	/*! Draws a color wheel (really, a ring) with specified colors. It doesn't look quite right 
	if there isn't any empty space in the middle of the ring.
	\param center The center of the color wheel.
	\param colors The colors to use in the color wheel.
	\param radius The radius of the color wheel.
	\param width The width of the color wheel. The color wheel will extend half of the width
	in either direction from the radius.
	\param angle The amount to rotate the color wheel.

	\code{.cpp}
	//This code snippet draws an isoluminant color wheel to the screen using color conversion from LAB to RGB.
	//Move the mouse and turn the scroll wheel to see different slices of the LAB space.
	#include "CX_EntryPoint.h"

	void runExperiment(void) {

	Input.setup(false, true);

	float L = 50;
	float aOff = 40;
	float bOff = 40;

	while (true) {
		if (Input.pollEvents()) {
			while (Input.Mouse.availableEvents() > 0) {
				CX_Mouse::Event mev = Input.Mouse.getNextEvent();

				if (mev.eventType == CX_Mouse::Event::SCROLLED) {
					L += mev.y;
				}

				if (mev.eventType == CX_Mouse::Event::MOVED) {
					aOff = mev.x - Display.getCenter().x;
					bOff = mev.y - Display.getCenter().y;
				}
			}

			//Only if input has been received, redraw the color wheel
			vector<ofFloatColor> wheelColors(100);

			for (int i = 0; i < wheelColors.size(); i++) {
				float angle = (float)i / wheelColors.size() * 2 * PI;
				float A = sin(angle) * aOff;
				float B = cos(angle) * bOff;

				wheelColors[i] = Draw::convertToRGB("LAB", L, A, B); //Convert the L, A, and B components to the RGB color space.
			}

			Display.beginDrawingToBackBuffer();
			ofBackground(0);
			Draw::colorWheel(Display.getCenter(), wheelColors, 200, 70, 0);
			Display.endDrawingToBackBuffer();
			Display.swapBuffers();
			}
		}
	}
	\endcode
	*/
	template <typename T>
	void colorWheel(ofPoint center, std::vector<ofColor_<T>> colors, float radius, float width, float angle) {

		ofVbo vbo = colorWheelToVbo(center, colors, radius, width, angle);
		vbo.draw(GL_TRIANGLE_STRIP, 0, vbo.getNumVertices());
	}



	/*! This function draws a pattern mask created with a large number of small squares.
	\param width The width of the area to draw to, in pixels.
	\param height The height of the are ato draw to, in pixels.
	\param pixelSize The size of each small square making up the shape, in pixels.
	\param colors Optional. If a vector of colors is provided, colors will be sampled in blocks
	using an Algo::BlockSampler from the provided colors. If no colors are provided, each color will
	be chosen randomly by sampling a hue value in the HSB color space, with the S and B held constant
	at maximum values (i.e. each color will be a bright, fully saturated color). */
	template <typename T>
	void patternMask(ofPoint center, float width, float height, float squareSize, std::vector<ofColor_<T>> colors = std::vector<ofColor_<T>>(0)) {
		center.x -= width / 2;
		center.y -= height / 2;

		squareSize = abs(squareSize);

		ofRectangle pos(0, 0, squareSize, squareSize);

		CX::Algo::BlockSampler<ofColor_<T>> bs(&CX::Instances::RNG, colors);

		while (pos.x < width) {
			while (pos.y < height) {

				ofColor_<T> col;
				if (colors.size() == 0) {
					col = ofColor_<T>::fromHsb(CX::Instances::RNG.randomDouble(0, ofColor_<T>::limit()), ofColor_<T>::limit(), ofColor_<T>::limit());
				} else {
					col = bs.getNextValue();
				}

				ofSetColor(col);
				ofRect(pos.x + center.x, pos.y + center.y, pos.width, pos.height);
				pos.y += squareSize;
			}
			pos.x += squareSize;
			pos.y = 0;
		}
	}

}
}
