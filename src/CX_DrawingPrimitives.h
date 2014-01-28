#ifndef _CX_DRAWING_PRIMITIVES_H_
#define _CX_DRAWING_PRIMITIVES_H_

#include "ofPoint.h"
#include "ofPath.h"
#include "ofTrueTypeFont.h"
#include "ofGraphics.h"

namespace CX {

	namespace Draw {

		struct CX_PathParams_t {
			ofColor lineColor;
			ofColor fillColor;
			float lineWidth;

			float rotationAmount;
			ofVec3f rotationAxes;
		};

		ofPath squircleToPath(double radius, double amount = 0.9);
		//void squircle(ofPoint center, double radius, double rotationDeg = 0, double amount = 0.9);

		ofPath starToPath(int numberOfPoints, double innerRadius, double outerRadius);
		void star(ofPoint center, int numberOfPoints, float innerRadius, float outerRadius,
				  ofColor lineColor, ofColor fillColor, float lineWidth = 1, float rotationDeg = 0);

		void centeredString(int x, int y, std::string s, ofTrueTypeFont &font);
		void centeredString(ofPoint center, string s, ofTrueTypeFont &font);

		struct CX_GaborProperties_t {
			CX_GaborProperties_t(void) :
				color(ofColor::white),
				angle(0),
				width(100),
				height(100),
				period(30),
				maskType(CX_GaborProperties_t::COSINE_WAVE),
				apertureType(CX_GaborProperties_t::AP_CIRCLE)
			{}

			ofColor color; //The color to use for drawing the gabor. The alpha channel of this color will be the maximum alpha of the pattern.
			double angle;

			double width; //If AP_CIRCLE is used, the diameter of the circle is specified by width
			double height;

			double period;

			enum {
				COSINE_WAVE,
				SQUARE_WAVE,
				TRIANGLE_WAVE
			} maskType;

			enum {
				AP_CIRCLE, 
				AP_RECTANGLE
			} apertureType;
		};

		ofPixels gaborToPixels (const CX_GaborProperties_t& properties);
		ofTexture gaborToTexture (const CX_GaborProperties_t& properties);
		void gabor (int x, int y, const CX_GaborProperties_t& properties);
	}
}

#endif //_CX_DRAWING_H_