#ifndef _CX_DRAWING_PRIMITIVES_H_
#define _CX_DRAWING_PRIMITIVES_H_

#include "ofPoint.h"
#include "ofPath.h"
#include "ofTrueTypeFont.h"

namespace CX {

	namespace Draw {

		ofPath squircleToPath(ofPoint center, double radius, double rotation, double amount = 0.9);

		ofPath starToPath(int numberOfPoints, double innerRadius, double outerRadius);
		void star(ofPoint center, int numberOfPoints, float innerRadius, float outerRadius,
			ofColor color, ofColor fillColor, float lineWidth = 1, float rotationRad = 0);

		void centeredString(int x, int y, std::string s, ofTrueTypeFont &font);
		void centeredString(ofPoint location, string s, ofTrueTypeFont &font);
	}
}

#endif //_CX_DRAWING_H_