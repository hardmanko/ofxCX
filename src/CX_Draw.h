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

		struct CX_PathParams_t {

			CX_PathParams_t(void) :
				strokeColor(255),
				fillColor(127),
				filled(true),
				strokeWidth(1),
				rotationAmount(0),
				rotationAxes(0,0,1)
			{}

			void applyTo(ofPath& p) {
				p.setFillColor(fillColor);
				p.setFilled(filled);
				p.setStrokeColor(strokeColor);
				p.setStrokeWidth(strokeWidth);
				p.rotate(rotationAmount, rotationAxes);
			}

			float strokeWidth;
			ofColor strokeColor;
			bool filled;
			ofColor fillColor;

			float rotationAmount;
			ofVec3f rotationAxes;
		};
		
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
				angle(0),
				width(100),
				height(100),
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

		template <typename ofColorType>	std::vector<ofColorType> getRGBSpectrum(unsigned int colorCount);

		ofVbo colorWheelToVbo(ofPoint center, std::vector<ofFloatColor> colors, float radius, float width, float angle);
		ofVbo colorArcToVbo(ofPoint center, std::vector<ofFloatColor> colors, float radiusX, float radiusY, float width, float angleBegin, float angleEnd);
		void colorWheel(ofPoint center, std::vector<ofFloatColor> colors, float radius, float width, float angle);
		void colorArc(ofPoint center, std::vector<ofFloatColor> colors, float radiusX, float radiusY, float width, float angleBegin, float angleEnd);

		std::vector<double> convertColors(std::string conversionFormula, double S1, double S2, double S3);
		ofFloatColor convertToRGB(std::string inputColorSpace, double S1, double S2, double S3);

		ofFbo patternMaskToFbo(unsigned int width, unsigned int height, float squareSize = 1.0, std::vector<ofColor> colors = std::vector<ofColor>(0));
	}
}

/*! Sample colors from the RGB spectrum with variable precision. Colors will be sampled
beginning with red, continue through yellow, green, cyan, blue, violet, and almost, but not quite, back to red.
\tparam ofColorType An oF color type. One of: ofColor, ofFloatColor, or ofShortColor.
\param colorCount The number of colors to draw from the RGB spectrum, which will be rounded
up to the next multiple of 6.
\return A vector containing the sampled colors with a number of colors equal to colorCount
rounded up to the next multiple of 6. */
template <typename ofColorType>
std::vector<ofColorType> CX::Draw::getRGBSpectrum(unsigned int colorCount) {

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