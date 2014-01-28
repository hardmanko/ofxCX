#include "CX_DrawingPrimitives.h"

using namespace CX::Draw;

/*!
This function draws an approximation of a squircle (http://en.wikipedia.org/wiki/Squircle) using Bezier curves.
\param radius The radius of the largest circle that can be enclosed in the squircle.
\param rotation Rotation of the squircle around its center.
\param amount The squicliness of the squircle. The default (0.9) seems like a pretty good amount for a good 
approximation of a squircle, but different amounts can give different sorts of shapes.
\return An ofPath containing the squircle.
*/
ofPath CX::Draw::squircleToPath(double radius, double rotation, double amount) {
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

void squircle(ofPoint center, double radius, double rotation, double amount) {
	ofPath sq = CX::Draw::squircleToPath(radius, rotation, amount);
	sq.draw(center.x, center.y);
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

void CX::Draw::centeredString(ofPoint center, string s, ofTrueTypeFont &font) {
	Draw::centeredString(center.x, center.y, s, font);
}


ofTexture CX::Draw::gaborToTexture (const CX_GaborProperties_t& properties) {
	//double theta = -properties.angle; //Degrees
	//double perp = (PI*(theta + 90) / 180);

	double theta = -properties.angle * PI / 180;

	double r = properties.radius;
	int diameter = 2 * ceil(r);

	double m = tan(theta);

	ofPixels pix;
	pix.allocate(diameter, diameter, ofImageType::OF_IMAGE_COLOR_ALPHA);
	pix.set(3, 0); //Set the alpha channel to 0 (transparent)

	//i indexes y values, j indexes x values
	for (int i = 0; i < pix.getHeight(); i++) {
		for (int j = 0; j < pix.getWidth(); j++) {

			ofPoint p(j - (diameter / 2), i - (diameter / 2)); //Center so that x and y are relative to the origin.

			if (p.distance(ofPoint(0,0)) <= r) {
				double xa = p.y / m; //It should be (p.y - b) / m, but b is 0 because we go through the origin

				double hyp = abs(xa - p.x);
				double distFromA = hyp * sin(theta);

				if (m == 0) { //Special case for flat lines.
					distFromA = p.y;
				}

				double intensity = (1.0 + cos((distFromA / properties.period) * 2 * PI)) / 2.0; //Scale to be between 0 and 1

				pix.setColor(j, i, ofColor(properties.color.r, properties.color.g, properties.color.b, ofClamp(255 * intensity, 0, 255)));
			}
		}
	}

	ofTexture tex;
	tex.allocate(pix);
	tex.loadData(pix);
	return tex;
}

void CX::Draw::gabor (int x, int y, const CX_GaborProperties_t& properties) {
	ofTexture tex = gaborToTexture(properties);
	ofSetColor(255);
	tex.draw(x - tex.getWidth()/2, y - tex.getHeight()/2);
}