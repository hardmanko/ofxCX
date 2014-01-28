#ifndef _CX_DRAWING_PRIMITIVES_H_
#define _CX_DRAWING_PRIMITIVES_H_

#include "ofPoint.h"
#include "ofPath.h"
#include "ofTrueTypeFont.h"
#include "ofGraphics.h"

namespace CX {

	namespace Draw {

		ofPath squircleToPath(double radius, double rotation, double amount = 0.9);
		void squircle(ofPoint center, double radius, double rotation, double amount = 0.9);

		ofPath starToPath(int numberOfPoints, double innerRadius, double outerRadius);
		void star(ofPoint center, int numberOfPoints, float innerRadius, float outerRadius,
			ofColor color, ofColor fillColor, float lineWidth = 1, float rotationRad = 0);

		void centeredString(int x, int y, std::string s, ofTrueTypeFont &font);
		void centeredString(ofPoint center, string s, ofTrueTypeFont &font);

		struct CX_GaborProperties_t {
			CX_GaborProperties_t(void) :
				color(ofColor::white),
				angle(0),
				radius(50),
				period(30)
			{}

			ofColor color;
			double angle;
			double radius;
			double period;
		};

		ofTexture gaborToTexture (const CX_GaborProperties_t& properties);
		void gabor (int x, int y, const CX_GaborProperties_t& properties);
	}
}

#endif //_CX_DRAWING_H_