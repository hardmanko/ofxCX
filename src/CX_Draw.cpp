#include "CX_Draw.h"

//using namespace CX::Draw;

/*!
This function draws an approximation of a squircle (http://en.wikipedia.org/wiki/Squircle) using Bezier curves.
The squircle will be centered on (0,0) in the ofPath.
\param radius The radius of the largest circle that can be enclosed in the squircle.
\param amount The "squircliness" of the squircle. The default (0.9) seems like a pretty good amount for a good 
approximation of a squircle, but different amounts can give different sorts of shapes.
\return An ofPath containing the squircle.
*/
ofPath CX::Draw::squircleToPath(double radius, double amount) {
	ofPath sq;
	sq.setFilled(false);

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
	}

	return sq;
}

/*
void squircle(ofPoint center, double radius, double rotationDeg, double amount) {
	ofPath sq = CX::Draw::squircleToPath(radius, amount);
	sq.rotate(-rotationDeg, ofVec3f(0, 0, 1));
	sq.draw(center.x, center.y);
}
*/

/*! Draws an arrow to an ofPath. The outline of the arrow is drawn with strokes, so you can
have the path be filled to have a solid arrow, or you can use non-zero width strokes in order
to have the outline of an arrow. The arrow points up by default but you can rotate it with
ofPath::rotate().
\param length The length of the arrow in pixels.
\param headOffsets The angle between the main arrow body and the two legs of the tip, in degrees.
\param headSize The length of the legs of the head in pixels.
\param lineWidth The width of the lines used to draw the arrow (i.e. the distance between parallel strokes).
\return An ofPath containing the arrow. The center of the arrow is at (0,0) in the ofPath.
*/
ofPath CX::Draw::arrowToPath(float length, float headOffsets, float headSize, float lineWidth) {

	headOffsets = (90 - headOffsets) * PI / 180;

	ofPath p;

	ofPoint outerPoint(headSize * cos(headOffsets), headSize * sin(headOffsets) - length / 2);
	ofPoint innerPoint = outerPoint + ofPoint(lineWidth * cos(headOffsets + PI / 2), lineWidth*sin(headOffsets + PI / 2));
	ofPoint innerAngle(lineWidth / 2, tan(headOffsets)*((lineWidth / 2) - innerPoint.x) + innerPoint.y);

	p.moveTo(0, -length / 2);
	
	p.lineTo(outerPoint);
	p.lineTo(innerPoint);
	p.lineTo(innerAngle);

	p.lineTo(lineWidth / 2, length / 2);
	p.lineTo(-lineWidth / 2, length / 2);

	p.lineTo(-innerAngle.x, innerAngle.y);
	p.lineTo(-innerPoint.x, innerPoint.y);
	p.lineTo(-outerPoint.x, outerPoint.y);
	p.lineTo(0, -length / 2);

	return p;
}

/*! This function obtains the vertices needed to draw an N pointed star.
\param numberOfPoints The number of points in the star.
\param innerRadius The distance from the center of the star at which the inner points of
the star hit.
\param outerRadius The distance from the center of the star to the outer points of the star.
\param rotationDeg The number of degrees to rotate the star. 0 degrees has one point of the star pointing up.
Positive values rotate the star counter-clockwise.
\return A vector of points defining the vertices needed to draw the star. There will be 2 * numberOfPoints + 1
vertices with the last vertex equal to the first vertex. The vertices are centered on (0, 0). */
std::vector<ofPoint> CX::Draw::getStarVertices(unsigned int numberOfPoints, float innerRadius, float outerRadius, float rotationDeg) {
	std::vector<ofPoint> vertices((2 * numberOfPoints) + 1);
	bool inside = true;

	float rotationRad = (rotationDeg - 90) * PI / 180;
	if (ofIsVFlipped()) {
		rotationRad = (rotationDeg + 90) * PI / 180;
	}

	for (unsigned int i = 0; i < vertices.size(); i++) {
		float xUnit = cos(rotationRad + (i * PI / numberOfPoints));
		float yUnit = sin(rotationRad + (i * PI / numberOfPoints));

		if (inside) {
			vertices[i] = ofPoint(xUnit * innerRadius, yUnit * innerRadius);
		} else {
			vertices[i] = ofPoint(xUnit * outerRadius, yUnit * outerRadius);
		}
		inside = !inside;
	}
	return vertices;
}

/*!
This draws an N-pointed star to an ofPath. The star will be centered on (0,0) in the ofPath.
\param numberOfPoints The number of points in the star.
\param innerRadius The distance from the center of the star at which the inner points of
the star hit.
\param outerRadius The distance from the center of the star to the outer points of the star.
\return An ofPath containing the star.
*/
ofPath CX::Draw::starToPath(unsigned int numberOfPoints, float innerRadius, float outerRadius) {
	ofPath star;

	std::vector<ofPoint> vertices = getStarVertices(numberOfPoints, innerRadius, outerRadius);

	star.moveTo(vertices.back());
	for (unsigned int i = 0; i < vertices.size(); i++) {
		star.lineTo(vertices[i]);
	}

	return star;
}

/*!
This draws an N-pointed star.
\param center The point at the center of the star.
\param numberOfPoints The number of points in the star.
\param innerRadius The distance from the center of the star to where the inner points of
the star hit.
\param outerRadius The distance from the center of the star to the outer points of the star.
\param fillColor The color used to fill in the center of the star.
\param rotationDeg The number of degrees to rotate the star. 0 degrees has one point of the star pointing up. 
Positive values rotate the star counter-clockwise.
*/
void CX::Draw::star(ofPoint center, unsigned int numberOfPoints, float innerRadius, float outerRadius,
					ofColor fillColor, float rotationDeg)
{

	ofSetColor(fillColor);
	std::vector<ofPoint> vertices = getStarVertices(numberOfPoints, innerRadius, outerRadius, rotationDeg);
	for (unsigned int i = 0; i < vertices.size(); i++) {
		vertices[i] += center;
	}

	glBegin(GL_TRIANGLE_FAN);
	glVertex2f(center.x, center.y);
	for (unsigned int i = 0; i < vertices.size(); i++) {
		glVertex2f(vertices[i].x, vertices[i].y);
	}
	glEnd();

}

/*! Equivalent to a call to CX::Draw::centeredString(ofPoint(x, y), s, font). */
void CX::Draw::centeredString(int x, int y, std::string s, ofTrueTypeFont &font) {
	ofRectangle bb = font.getStringBoundingBox(s, 0, 0);
	x -= bb.width / 2;
	y -= (bb.y + bb.height / 2);
	font.drawString(s, x, y);
}

/*! Draws a string centered on a given location using the given font. Strings are normally
drawn such that the x coordinate gives the left edge of the string and the y coordinate
gives the line above which the letters will be drawn, where some characters (like y or g) 
can descend below the line.
\param center The coordinates of the center of the string.
\param s The string to draw.
\param font A font that has already been prepared for use.
*/
void CX::Draw::centeredString(ofPoint center, std::string s, ofTrueTypeFont &font) {
	Draw::centeredString(center.x, center.y, s, font);
}


ofPixels CX::Draw::greyscalePattern(const CX_PatternProperties_t& properties) {
	double theta = properties.angle * PI / 180;
	double radius = properties.width / 2; //Use width for radius. Consider using the one not set to 0 (whatever default value is)
	double slope = tan(theta);

	ofPixels pix;
	if (properties.apertureType == CX_PatternProperties_t::AP_CIRCLE) {
		pix.allocate(properties.width, properties.width, ofImageType::OF_IMAGE_GRAYSCALE);
	} else {
		pix.allocate(properties.width, properties.height, ofImageType::OF_IMAGE_GRAYSCALE);
	}
	pix.set(0, properties.minValue); //Set the single channel to 0. Already done in allocate

	//Get point on line tangent to "radius" of rectangle and the interecept of the line passing through that point
	double tanRadius = sqrt(pow(pix.getWidth(), 2) + pow(pix.getHeight(), 2));
	//Make the tanRadius be the next greatest multiple of the period
	tanRadius = (ceil(tanRadius / properties.period) * properties.period) + (properties.period * fmod(properties.phase, 360.0) / 360.0);
	ofPoint tangentPoint(tanRadius * sin(PI - theta), tanRadius * cos(PI - theta));
	double b = tangentPoint.y - (slope * tangentPoint.x);

	//i indexes y values, j indexes x values
	for (int i = 0; i < pix.getHeight(); i++) {
		for (int j = 0; j < pix.getWidth(); j++) {

			ofPoint p(j - (pix.getWidth() / 2), i - (pix.getHeight() / 2)); //Center so that x and y are relative to the origin.

			if (properties.apertureType == CX_PatternProperties_t::AP_CIRCLE) {
				if (p.distance(ofPoint(0, 0)) > radius) { //Determine radius from the width
					continue; //Do not draw anything in this pixel (already transparent)
				}
			} else if (properties.apertureType == CX_PatternProperties_t::AP_RECTANGLE) {
				//Do nothing, allow pattern to be drawn in entire texture.
			}

			double distFromA;
			if (slope == 0) { //Special case for flat lines.
				distFromA = p.y + (properties.period * fmod(properties.phase, 360.0) / 360.0);
			} else {
				double xa = (p.y - b) / slope;

				double hyp = abs(xa - p.x);
				distFromA = hyp * sin(theta);
			}

			double intensity = 0;

			switch (properties.maskType) {
			case CX_PatternProperties_t::SINE_WAVE:
				intensity = (1.0 + sin((distFromA / properties.period) * 2 * PI)) / 2.0; //Scale to be between 0 and 1
				break;
			case CX_PatternProperties_t::SQUARE_WAVE:
				if (cos((distFromA / properties.period) * 2 * PI) > 0) {
					intensity = 1;
				} else {
					intensity = 0;
				}
				break;
			case CX_PatternProperties_t::TRIANGLE_WAVE:
				double modulo = abs(fmod(distFromA, properties.period));
				if (modulo >= properties.period / 2) {
					intensity = modulo / properties.period;
				} else {
					intensity = 1 - modulo / properties.period;
				}
				break;
			}

			if (properties.fallOffPower != std::numeric_limits<double>::min()) {
				intensity *= (1 - pow(p.distance(ofPoint(0, 0)) / radius, properties.fallOffPower));
			}

			intensity = CX::Util::clamp<double>(intensity, 0, 1);

			pix.setColor(j, i, ofColor((properties.maxValue - properties.minValue) * intensity + properties.minValue));

		}
	}

	return pix;
}

ofPixels CX::Draw::gaborToPixels (const CX_GaborProperties_t& properties) {
	ofPixels pattern = greyscalePattern(properties.pattern);
	
	ofPixels pix;
	pix.allocate(pattern.getWidth(), pattern.getHeight(), ofImageType::OF_IMAGE_COLOR_ALPHA);
	pix.setColor(properties.color);
	pix.setChannel(3, pattern); //Set alpha channel equal to pattern

	return pix;
}

ofTexture CX::Draw::gaborToTexture (const CX_GaborProperties_t& properties) {
	ofPixels pix = gaborToPixels(properties);
	ofTexture tex;
	tex.allocate(pix);
	tex.loadData(pix);
	return tex;
}

void CX::Draw::gabor (int x, int y, const CX_GaborProperties_t& properties) {
	ofTexture tex = gaborToTexture(properties);
	ofSetColor(255);
	tex.draw(x - tex.getWidth()/2, y - tex.getHeight()/2); //Draws centered
}


bool isPointInRegion(ofPoint p, ofPoint r1, ofPoint r2, float tolerance) {
	float lowerX = min(r1.x, r2.x) - tolerance;
	float upperX = max(r1.x, r2.x) + tolerance;
	float lowerY = min(r1.y, r2.y) - tolerance;
	float upperY = max(r1.y, r2.y) + tolerance;

	return (p.x >= lowerX) && (p.x <= upperX) && (p.y >= lowerY) && (p.y <= upperY);
}

bool arePointsInLine(ofPoint p1, ofPoint p2, ofPoint p3, float slopeTolerance = 1e-3) {
	float x12Dif = p1.x - p2.x;
	float x23Dif = p2.x - p3.x;
	if ((x12Dif == 0) && (x23Dif == 0)) {
		return true;
	} else if ((x12Dif == 0 || x23Dif == 0) && (x12Dif != x23Dif)) {
		return false;
	}

	float m1 = ((p1.y - p2.y) / x12Dif);
	float m2 = ((p2.y - p3.y) / x23Dif);

	return (m2 >= (m1 - slopeTolerance)) && (m2 <= (m1 + slopeTolerance));
}


struct LineSegment {
	LineSegment(void) :
		p1(-1, -1),
		p2(-1, -1)
	{}

	LineSegment(ofPoint p1_, ofPoint p2_) :
		p1(p1_),
		p2(p2_)
	{}

	ofPoint pointAlong(float p) {
		p = CX::Util::clamp<float>(p, 0, 1);
		return (p2 - p1)*p + p1;
	}

	bool doesPointLieOnSegment(ofPoint p, float tolerance = 1e-3) {
		return isPointInRegion(p, p1, p2, tolerance) && arePointsInLine(p, p1, p2, tolerance);

	}

	ofPoint p1;
	ofPoint p2;
};

struct LineStandardCoefs {

	LineStandardCoefs(LineSegment ls) {
		findCoefs(ls.p1, ls.p2);
	}

	void findCoefs(ofPoint p1, ofPoint p2) {
		if (p1.x == p2.x) {
			A = 1 / p1.x;
			B = 0;
			C = 1;
			return;
		}

		float m = (p1.y - p2.y) / (p1.x - p2.x);
		float b = p1.y - (m * p1.x);

		findCoefs(m, b);
	}

	void findCoefs(float m, float b) {
		C = 1;
		B = C / b;
		A = -B * m;
	}

	float A;
	float B;
	float C;
};



//This does not find the intersection of the line segments, but the intersection of the lines defined by the two points
//in each line segment.
ofPoint findIntersectionOfLines(LineSegment ls1, LineSegment ls2) {
	LineStandardCoefs c1(ls1);
	LineStandardCoefs c2(ls2);

	float det = 1 / (c1.A*c2.B - c2.A*c1.B);
	return ofPoint(det*(c2.B - c1.B), det*(c1.A - c2.A));
}

std::vector<LineSegment> getParallelLineSegments(LineSegment ls, float distance) {
	float d = distance;

	std::vector<LineSegment> rval(2);

	float xOffset = 0;
	float yOffset = 0;

	if (ls.p1.x == ls.p2.x) {
		xOffset = d;
		yOffset = 0;
	} else if (ls.p1.y == ls.p2.y) {
		xOffset = 0;
		yOffset = d;
	} else {
		float origM = (ls.p1.y - ls.p2.y) / (ls.p1.x - ls.p2.x);
		float m = -1 / origM;
		xOffset = d / (sqrt(1 + pow(m, 2)));
		yOffset = m*xOffset;
	}

	rval[0].p1.x = ls.p1.x + xOffset;
	rval[0].p1.y = ls.p1.y + yOffset;
	rval[0].p2.x = ls.p2.x + xOffset;
	rval[0].p2.y = ls.p2.y + yOffset;

	rval[1].p1.x = ls.p1.x - xOffset;
	rval[1].p1.y = ls.p1.y - yOffset;
	rval[1].p2.x = ls.p2.x - xOffset;
	rval[1].p2.y = ls.p2.y - yOffset;

	return rval;
}

ofVec3f getCornerOuterVector(ofPoint p1, ofPoint p2, ofPoint p3, float vectorLength) {
	ofVec3f offset = (p2 - p1) + (p2 - p3);
	float d = sqrt(offset.x * offset.x + offset.y*offset.y);
	float s = vectorLength / d;
	ofVec3f result;
	result.x = sqrt(vectorLength*vectorLength - pow(s*offset.y, 2));
	result.y = sqrt(vectorLength*vectorLength - pow(s*offset.x, 2));
	return result;
}

struct CornerPoint {
	enum Type {
		INNER,
		OUTER,
		PERPENDICULAR
	};

	CornerPoint(void) :
		p(-1, -1)
	{}

	CornerPoint(ofPoint p_, CornerPoint::Type type_) :
		p(p_),
		type(type_)
	{}

	ofPoint p;
	Type type;
};

ofPath CX::Draw::lines(std::vector<ofPoint> points, ofColor color, float width, LineCornerMode cornerMode) {
	bool isClosed = (points.front() == points.back());

	
	for (unsigned int i = 0; i < points.size() - 2; i++) {
		//Clean out points that are in a line with each other
		if (arePointsInLine(points[i], points[i + 1], points[i + 2], 0) && 
			isPointInRegion(points[i + 1], points[i], points[i + 2], width / 100)) 
		{
			points.erase(points.begin() + i + 1);
			i--;
			continue;
		}
		//Remove identical points
		if (points[i] == points[i + 1]) {
			points.erase(points.begin() + i + 1);
			i--;
			continue;
		}
	}

	std::vector< std::vector< LineSegment > > parallelSegments(points.size() - 1);
	std::vector< std::vector<int> > lineSegmentSide(parallelSegments.size());
	std::vector< std::vector< CornerPoint > > cornerPoints(2);
	std::vector< CornerPoint > allCornerPoints;

	for (unsigned int i = 0; i < parallelSegments.size(); i++) {
		parallelSegments[i] = getParallelLineSegments(LineSegment(points[i], points[i + 1]), width / 2);
		lineSegmentSide[i].resize(2);
	}

	lineSegmentSide[0][0] = 0;
	lineSegmentSide[0][1] = 1;

	unsigned int endIndex = isClosed ? parallelSegments.size() : parallelSegments.size() - 1;

	for (unsigned int i = 0; i < endIndex; i++) {

		unsigned int i2 = i + 1;
		if (isClosed && (i == (parallelSegments.size() - 1))) {
			i2 = 0;
		}

		for (unsigned int j = 0; j < 2; j++) {
			for (unsigned int k = 0; k < 2; k++) {

				LineSegment& ls1 = parallelSegments[i][j];
				LineSegment& ls2 = parallelSegments[i2][k];

				ofPoint intersection = findIntersectionOfLines(ls1, ls2);

				bool inLs1 = isPointInRegion(intersection, ls1.p1, ls1.p2, width / 100);
				bool inLs2 = isPointInRegion(intersection, ls2.p1, ls2.p2, width / 100);

				//cout << "i,j,k: " << i << "," << j << "," << k << ": " << inLs1 << ", " << inLs2 << endl;

				int side = lineSegmentSide[i][j];

				if (arePointsInLine(points[i], points[i2], points[i2 + 1], 0)) {
					if (isPointInRegion(points[i2], points[i], points[i2 + 1], width / 100)) {
						//Middle point is between others. This should have been removed earlier.
					} else {
						//Middle point is not between others, it is at the end of a line sticking out.


						cornerPoints[side].push_back(CornerPoint(ls1.p1, CornerPoint::PERPENDICULAR));
					}
				} else if (inLs1 && inLs2) {

					lineSegmentSide[i2][k] = side;

					if (!isClosed && (i == 0)) {
						cornerPoints[side].push_back(CornerPoint(ls1.p1, CornerPoint::PERPENDICULAR));
						allCornerPoints.push_back(CornerPoint(ls1.p1, CornerPoint::PERPENDICULAR));
					}
					cornerPoints[side].push_back(CornerPoint(intersection, CornerPoint::INNER));
					allCornerPoints.push_back(CornerPoint(intersection, CornerPoint::INNER));
					if (!isClosed && (i == (endIndex - 1))) {
						cornerPoints[side].push_back(CornerPoint(ls2.p2, CornerPoint::PERPENDICULAR));
						allCornerPoints.push_back(CornerPoint(ls2.p2, CornerPoint::PERPENDICULAR));
					}

					ls1.p2 = intersection;
					ls2.p1 = intersection;
				} else if (!inLs1 && !inLs2) {
					lineSegmentSide[i2][k] = side;

					if (!isClosed && (i == 0)) {
						cornerPoints[side].push_back(CornerPoint(ls1.p1, CornerPoint::PERPENDICULAR));
						allCornerPoints.push_back(CornerPoint(ls1.p1, CornerPoint::PERPENDICULAR));
					}
					cornerPoints[side].push_back(CornerPoint(ls1.p2, CornerPoint::PERPENDICULAR));
					cornerPoints[side].push_back(CornerPoint(intersection, CornerPoint::OUTER));
					cornerPoints[side].push_back(CornerPoint(ls2.p1, CornerPoint::PERPENDICULAR));

					allCornerPoints.push_back(CornerPoint(ls1.p2, CornerPoint::PERPENDICULAR));
					allCornerPoints.push_back(CornerPoint(intersection, CornerPoint::OUTER));
					allCornerPoints.push_back(CornerPoint(ls2.p1, CornerPoint::PERPENDICULAR));

					if (!isClosed && (i == (endIndex - 1))) {
						cornerPoints[side].push_back(CornerPoint(ls2.p2, CornerPoint::PERPENDICULAR));
						allCornerPoints.push_back(CornerPoint(ls2.p2, CornerPoint::PERPENDICULAR));
					}

					ls1.p2 = intersection;
					ls2.p1 = intersection;
				}
			}
		}
	}

	if (isClosed) {
		cornerPoints[0].push_back(cornerPoints[0][0]);
		cornerPoints[1].push_back(cornerPoints[1][0]);

		allCornerPoints.push_back(cornerPoints[0][0]);
		allCornerPoints.push_back(cornerPoints[1][0]);
	}


	
	ofPath path;
	path.setColor(color);
	path.setStrokeColor(ofColor::white);
	path.setFilled(true);
	path.setStrokeWidth(1);
	path.setPolyWindingMode(ofPolyWindingMode::OF_POLY_WINDING_NONZERO);

	if (cornerMode == LineCornerMode::OUTER_POINT) {

		path.moveTo(cornerPoints[0][0].p);
		for (unsigned int i = 1; i < cornerPoints[0].size(); i++) {
			path.lineTo(cornerPoints[0][i].p); //You could do a check to make sure that perpendicular points that are not at the ends are not drawn,
				//but why bother?
		}

		for (unsigned int i = cornerPoints[1].size() - 1; i < cornerPoints[1].size(); i--) {
			path.lineTo(cornerPoints[1][i].p);
		}
		path.lineTo(cornerPoints[0][0].p);

	} else if (cornerMode == LineCornerMode::STRAIGHT_LINE) {
		path.moveTo(cornerPoints[0][0].p);
		for (unsigned int i = 1; i < cornerPoints[0].size(); i++) {
			if (cornerPoints[0][i].type != CornerPoint::OUTER) {
				path.lineTo(cornerPoints[0][i].p);
			}
		}

		for (unsigned int i = cornerPoints[1].size() - 1; i < cornerPoints[1].size(); i--) {
			if (cornerPoints[1][i].type != CornerPoint::OUTER) {
				path.lineTo(cornerPoints[1][i].p);
			}
		}
		path.lineTo(cornerPoints[0][0].p);

	} else if (cornerMode == LineCornerMode::BEZIER_ARC) {
		path.moveTo(cornerPoints[0][0].p);
		for (unsigned int i = 1; i < cornerPoints[0].size(); i++) {
			if (cornerPoints[0][i].type == CornerPoint::OUTER) {
				path.bezierTo(cornerPoints[0][i - 1].p, cornerPoints[0][i].p, cornerPoints[0][i + 1].p);
				i++;
			} else {
				path.lineTo(cornerPoints[0][i].p);
			}
		}

		for (unsigned int i = cornerPoints[1].size() - 1; i < cornerPoints[1].size(); i--) {
			if (cornerPoints[1][i].type == CornerPoint::OUTER) {
				path.bezierTo(cornerPoints[1][i + 1].p, cornerPoints[1][i].p, cornerPoints[1][i - 1].p);
				i--;
			} else {
				path.lineTo(cornerPoints[1][i].p);
			}
		}
		path.lineTo(cornerPoints[0][0].p);
	}

	return path;
}




void CX::Draw::lines(std::vector<ofPoint> points, float lineWidth) {

	float d = lineWidth / 2;

	for (unsigned int i = 0; i < points.size() - 1; i++) {
		Draw::line(points[i], points[i + 1], lineWidth);
		ofCircle(points[i], d);
	}
	ofCircle(points[points.size() - 1], d);
}

/*
GLfloat vertices[] = {...}; // 36 of vertex coords
...
// activate and specify pointer to vertex array
glEnableClientState(GL_VERTEX_ARRAY);
glVertexPointer(3, GL_FLOAT, 0, vertices);

// draw a cube
glDrawArrays(GL_TRIANGLES, 0, 36);

// deactivate vertex arrays after drawing
glDisableClientState(GL_VERTEX_ARRAY);
*/

void CX::Draw::line(ofPoint p1, ofPoint p2, float width) {

	std::vector<LineSegment> ls = getParallelLineSegments(LineSegment(p1, p2), width/2);

	ofPoint points [4];
	points[0] = ls[0].p1;
	points[1] = ls[0].p2;
	points[2] = ls[1].p1;
	points[3] = ls[1].p2;

	/*
	GLfloat vertices[4 * 3];
	vertices[0] = ls[0].p1.x;
	vertices[1] = ls[0].p1.y;
	vertices[2] = ls[0].p1.z;
	vertices[3] = ls[0].p2.x;
	vertices[4] = ls[0].p2.y;
	vertices[5] = ls[0].p2.z;
	vertices[6] = ls[1].p1.x;
	vertices[7] = ls[1].p1.y;
	vertices[8] = ls[1].p1.z;
	vertices[9] = ls[1].p2.x;
	vertices[10] = ls[1].p2.y;
	vertices[11] = ls[1].p2.z;

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, &vertices);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glDisableClientState(GL_VERTEX_ARRAY);
	*/

	/*
	glBegin(GL_TRIANGLE_STRIP);
		glVertex2f(ls[0].p1.x, ls[0].p1.y);
		glVertex2f(ls[0].p2.x, ls[0].p2.y);
		glVertex2f(ls[1].p1.x, ls[1].p1.y);
		glVertex2f(ls[1].p2.x, ls[1].p2.y);
	glEnd();
	*/

	ofFloatColor cols [4];
	cols[0] = ofColor::red;
	cols[1] = ofColor::yellow;
	cols[2] = ofColor::green;
	cols[3] = ofColor::blue;

	ofVbo vbo;
	vbo.setVertexData(points, 4, GL_STATIC_DRAW);
	vbo.setColorData(cols, 4, GL_STATIC_DRAW);
	vbo.draw(GL_TRIANGLE_STRIP, 0, 4);
}


void CX::Draw::ring(ofPoint center, float radius, float width, unsigned int resolution) {
	/*
	std::vector<ofPoint> vertices;

	float innerRadius = std::max(radius - width, 0.0f);
	float m = 2 * PI / resolution;
	for (unsigned int i = 0; i < resolution; i++) {
		float angle = i * m;

		vertices.push_back(center + ofPoint(innerRadius*cos(angle), innerRadius*sin(angle)));
		vertices.push_back(center + ofPoint(radius*cos(angle), radius*sin(angle)));
	}

	glBegin(GL_TRIANGLE_STRIP);
		for (unsigned int i = 0; i < vertices.size(); i++) {
			glVertex2f(vertices[i].x, vertices[i].y);
		}
		glVertex2f(vertices[0].x, vertices[0].y);
		glVertex2f(vertices[1].x, vertices[1].y);
	glEnd();
	*/

	ofPath path;
	path.moveTo(center + ofPoint(radius, 0));
	path.circle(center, radius);
	path.moveTo(center + ofPoint(radius - width, 0));
	path.circle(center, radius - width);
	
	ofMesh tess = path.getTessellation();
	tess.draw(ofPolyRenderMode::OF_MESH_FILL);
}



/*! Draws a bezier curve with an arbitrary number of control points. May become slow with a large number
of control points. Uses de Casteljau's algorithm to calculate the curve points. See this awesome guide: 
http://pomax.github.io/bezierinfo/
\param controlPoints Control points for the bezier.
\param width The width of the lines to be drawn. Uses CX::Draw::lines internally to draw the connecting lines.
\param resolution Controls the approximation of the bezier curve. There will be `resolution` line segments drawn to 
complete the curve.
*/
void CX::Draw::bezier(std::vector<ofPoint> controlPoints, float width, unsigned int resolution) {

	vector<vector<LineSegment>> segs(controlPoints.size() - 1);
	for (unsigned int i = 0; i < segs.size(); i++) {
		segs[i].resize(controlPoints.size() - i - 1);
	}

	//Initialize layer 0
	for (unsigned int i = 0; i < controlPoints.size() - 1; i++) {
		segs[0][i].p1 = controlPoints[i];
		segs[0][i].p2 = controlPoints[i + 1];
	}

	vector<ofPoint> outputPoints(resolution + 1);

	vector<ofPoint> nextLayerCP(segs.size());
	for (unsigned int res = 0; res < resolution + 1; res++) {
		float t = (float)res / resolution;

		for (unsigned int layer = 0; layer < segs.size(); layer++) {
			for (unsigned int segment = 0; segment < segs[layer].size(); segment++) {
				ofPoint p = segs[layer][segment].pointAlong(t);
				nextLayerCP[segment] = p;
				if (layer == segs.size() - 1) {
					outputPoints[res] = p;
				}
			}
			//You've finished layer i, prepare layer i + 1
			for (unsigned int i = 0; i < segs[layer].size() - 1; i++) {
				segs[layer + 1][i].p1 = nextLayerCP[i];
				segs[layer + 1][i].p2 = nextLayerCP[i + 1];
			}
		}
	}

	Draw::lines(outputPoints, width);
}