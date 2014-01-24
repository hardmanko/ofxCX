#include "CX_DrawingPrimitives.h"

using namespace CX::Draw;

ofPath CX::Draw::squircleToPath(ofPoint center, double radius, double rotation, double amount) {
	ofPath sq;

	ofPoint start, p1, p2, end;

	int s1[] = { 1, 1, -1, -1 };
	int s2[] = { 1, -1, -1, 1 };

	for (int i = 0; i < 4; i++) {
		start = ofPoint(s1[i] * radius, 0);
		p1 = ofPoint(s1[i] * radius, s2[i] * amount * radius);
		p2 = ofPoint(s1[i] * amount * radius, s2[i] * radius);
		end = ofPoint(0, s2[i] * radius);

		sq.moveTo(start);
		sq.bezierTo(p1, p2, end);
		sq.setFilled(false);
	}

	sq.rotate(rotation, ofVec3f(0, 0, 1));

	return sq;
}

//The star will be centered on 0,0 in the ofPath.
ofPath CX::Draw::starToPath(int numberOfPoints, double innerRadius, double outerRadius) {
	ofPath star;

	bool inside = true;

	double rotationRad = -PI / 2;

	for (int i = 0; i < (2 * numberOfPoints) + 1; i++) {

		float xUnit = cos(rotationRad + (i * PI / numberOfPoints));
		float yUnit = sin(rotationRad + (i * PI / numberOfPoints));

		if (inside) {
			star.lineTo(xUnit * outerRadius, yUnit * outerRadius);
		} else {
			star.lineTo(xUnit * innerRadius, yUnit * innerRadius);
		}

		inside = !inside;

	}

	return star;
}

void CX::Draw::star(ofPoint center, int numberOfPoints, float innerRadius, float outerRadius,
	ofColor color, ofColor fillColor, float lineWidth, float rotationRad) 
{
	ofPath star = CX::Draw::starToPath(numberOfPoints, innerRadius, outerRadius);
	star.setColor(color);
	star.setFillColor(fillColor);
	star.setStrokeWidth(lineWidth);
	
	star.rotate(rotationRad, ofVec3f(0, 0, 1));

	star.draw(center.x, center.y);
}

void CX::Draw::centeredString(int x, int y, string s, ofTrueTypeFont &font) {
	ofRectangle bb = font.getStringBoundingBox(s, 0, 0);
	x -= bb.width / 2;
	y -= (bb.y + bb.height / 2);
	font.drawString(s, x, y);
}

void CX::Draw::centeredString(ofPoint location, string s, ofTrueTypeFont &font) {
	Draw::centeredString(location.x, location.y, s, font);
}
