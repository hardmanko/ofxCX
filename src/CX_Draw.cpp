#include "CX_Draw.h"

#include "CX_Display.h"

#include "../libs/colorspace/colorspace.h"

namespace CX {
namespace Draw {



//These are some local functions and classes that are not part of the CX api.
// \cond INTERNAL_DOCS
struct LineSegment;

bool isPointInRegion(ofPoint p, ofPoint r1, ofPoint r2, float tolerance);
bool arePointsInLine(ofPoint p1, ofPoint p2, ofPoint p3, float angleTolerance);
ofPoint findIntersectionOfLines(LineSegment ls1, LineSegment ls2);
std::vector<LineSegment> getParallelLineSegments(LineSegment ls, float distance);

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

	bool doesPointLieOnSegment(ofPoint p, float locationTolerance = 1e-3, float angleTolerance = 0.5) {
		return isPointInRegion(p, p1, p2, locationTolerance) && arePointsInLine(p, p1, p2, angleTolerance);

	}

	ofPoint p1;
	ofPoint p2;
};

// \endcond


/*! This function draws an approximation of a squircle (http://en.wikipedia.org/wiki/Squircle) using Bezier curves
to an ofPath. The squircle will be centered on (0,0) in the ofPath.
\param radius The radius of the largest circle that can be enclosed in the squircle.
\param amount The "squircliness" of the squircle. The default (0.9) seems like a pretty good amount for a good 
approximation of a squircle, but different amounts can give different sorts of shapes.
\return An ofPath containing the squircle.
*/
ofPath squircleToPath(double radius, double amount) {
	ofPath sq;
	sq.setFilled(false);

	ofPoint start, p1, p2, end;

	int s1[] = { 1, 1, -1, -1 };
	int s2[] = { 1, -1, -1, 1 };

	sq.moveTo(ofPoint(s1[0] * radius, 0));

	for (int i = 0; i < 4; i++) {
		start = ofPoint(s1[i] * radius, 0);
		p1 = ofPoint(s1[i] * radius, s2[i] * amount * radius);
		p2 = ofPoint(s1[i] * amount * radius, s2[i] * radius);
		end = ofPoint(0, s2[i] * radius);

		sq.lineTo(start);
		sq.bezierTo(p1, p2, end);
	}

	return sq;
}

/*! This function draws an approximation of a squircle (http://en.wikipedia.org/wiki/Squircle) using Bezier curves.
\param center The squircle will be drawn centered at `center`.
\param radius The radius of the largest circle that can be enclosed in the squircle.
\param amount The "squircliness" of the squircle. The default (0.9) seems like a pretty good amount for a good
approximation of a squircle, but different amounts can give different sorts of shapes.
\param rotationDeg The amount to rotate the squircle, in degrees.
\note If more control over the drawing of the squircle is desired, use squircleToPath() and then modify the ofPath.
*/
void squircle(ofPoint center, double radius, double amount, double rotationDeg) {
	ofPath sq = squircleToPath(radius, amount);

	sq.setColor(ofGetStyle().color);
	sq.setFilled(true);

	sq.rotate(rotationDeg, ofVec3f(0, 0, 1));

	sq.draw(center.x, center.y);
}

/*! Draws an arrow to an ofPath. The outline of the arrow is drawn with strokes, so you can
have the path be filled to have a solid arrow, or you can use non-zero width strokes in order
to have the outline of an arrow. The arrow points in the positive y-direction by default but 
you can rotate it with ofPath::rotate().
\param length The length of the arrow in pixels.
\param headOffsets The angle between the main arrow body and the two legs of the tip, in degrees.
\param headSize The length of the legs of the head in pixels.
\param lineWidth The width of the lines used to draw the arrow (i.e. the distance between parallel strokes).
\return An ofPath containing the arrow. The center of the arrow is at (0,0) in the ofPath.
*/
ofPath arrowToPath(float length, float headOffsets, float headSize, float lineWidth) {

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

	p.rotate(180, ofVec3f(0, 0, 1));

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
std::vector<ofPoint> getStarVertices(unsigned int numberOfPoints, float innerRadius, float outerRadius, float rotationDeg) {
	std::vector<ofPoint> vertices((2 * numberOfPoints) + 1);
	bool inside = true;

	float rotationRad = (rotationDeg + 90) * PI / 180;
	//if (CX::Instances::Disp.getYIncreasesUpwards()) {
	//	rotationRad = (rotationDeg - 90) * PI / 180;
	//}

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
\see CX::Draw::star()
*/
ofPath starToPath(unsigned int numberOfPoints, float innerRadius, float outerRadius) {
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
\param rotationDeg The number of degrees to rotate the star. 0 degrees has one point of the star pointing up. 
Positive values rotate the star counter-clockwise.
*/
void star(ofPoint center, unsigned int numberOfPoints, float innerRadius, float outerRadius, float rotationDeg) {

	std::vector<ofPoint> vertices = getStarVertices(numberOfPoints, innerRadius, outerRadius, rotationDeg);

	for (unsigned int i = 0; i < vertices.size(); i++) {
		vertices[i] += center;
	}

	vertices.insert(vertices.begin(), center);

	ofVbo vbo;
	vbo.setVertexData(vertices.data(), vertices.size(), GL_STATIC_DRAW);

	vbo.draw(GL_TRIANGLE_FAN, 0, vertices.size());
}

/*! Equivalent to a call to CX::Draw::centeredString(ofPoint, std::string, ofTrueTypeFont&) 
with the x and y values in the point. */
void centeredString(int x, int y, std::string s, ofTrueTypeFont &font) {
	ofRectangle bb = font.getStringBoundingBox(s, 0, 0);
	x -= bb.width / 2;
	//if (CX::Instances::Disp.getYIncreasesUpwards()) {
	//	y = y + bb.y + bb.height / 2;
	//} else {
		y = y - bb.y - bb.height / 2;
	//}
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
void centeredString(ofPoint center, std::string s, ofTrueTypeFont &font) {
	Draw::centeredString(center.x, center.y, s, font);
}


/*! This function draws a series of line segments to connect the given points.
At each point, if `circleJoins` is `true` the line segments are joined with a circle, which results in overdraw.
Overdraw means that some areas are drawn twice, which means that if transparency is
used, it results in differing colors at the overdrawn areas. A (very inefficient) workaround is to
draw with max alpha into an fbo and then draw the fbo with transparency. A more advanced version 
of this function that attempts to prevent overdraw is
\ref Draw::lines(std::vector<ofPoint> points, float width, LineCornerMode cornerMode),
but that function can break in various ways.
\param points The points to connect with lines.
\param lineWidth The width of the line.
\param circleJoins Whether each junction of two lines should have a circle drawn over it.
*/
void lines(std::vector<ofPoint> points, float lineWidth, bool circleJoins) {
	if (points.size() < 2) {
		return;
	}

	float d = lineWidth / 2;
	Draw::line(points[0], points[1], lineWidth);
	for (unsigned int i = 1; i < points.size() - 1; i++) {
		if (circleJoins) {
			ofCircle(points[i], d);
		}
		Draw::line(points[i], points[i + 1], lineWidth);
	}

	if (circleJoins && (points.back() == points.front())) {
		ofCircle(points.front(), d);
	}
}

/*! This function draws a line from p1 to p2 with the given width. Note that this function is purely 2D:
The Z coordinate is basically ignored and should be 0 for best performance.
\param p1 One end of the line.
\param p2 The other end of the line.
\param width The width of the line.
*/
void line(ofPoint p1, ofPoint p2, float width) {
	std::vector<LineSegment> ls = getParallelLineSegments(LineSegment(p1, p2), width/2);

	ofPoint points [4];
	points[0] = ls[0].p1;
	points[1] = ls[0].p2;
	points[2] = ls[1].p1;
	points[3] = ls[1].p2;

	ofTriangle(points[0], points[1], points[2]);
	ofTriangle(points[1], points[2], points[3]);
}

/*! This function draws a ring, i.e. an unfilled circle. The filled area of the ring is between `radius + width/2` and `radius - width/2`.
\param center The center of the ring.
\param radius The radius of the ring.
\param width The radial width of the ring.
\param resolution The ring will be approximated with a number of line segments, which is controlled with `resolution`.

\note This function supersedes drawing rings with ofCircle with fill set to off because the line width of 
the unfilled circle cannot be set to a value greater than 1 with ofCircle.
*/
void ring(ofPoint center, float radius, float width, unsigned int resolution) {
	float halfWidth = width / 2;

	ofPath path;
	path.setCircleResolution(resolution);
	path.moveTo(center + ofPoint(radius + halfWidth, 0));
	path.circle(center, radius + halfWidth);
	path.moveTo(center + ofPoint(radius - halfWidth, 0));
	path.circle(center, radius - halfWidth);
	
	ofMesh tess = path.getTessellation();
	tess.draw(ofPolyRenderMode::OF_MESH_FILL);
}

/*! Draw an arc around a central point. If radiusX and radiusY are equal, the arc will be like a section of a circle. If they
are unequal, the arc will be a section of an ellipse.
\param center The point around which the arc will be drawn.
\param radiusX The radius of the arc in the X-axis.
\param radiusY The radius of the arc in the Y-axis.
\param width The width of the arc, radially from the center.
\param angleBegin The angle at which to begin the arc, in degrees.
\param angleEnd The angle at which to end the arc, in degrees. If the arc goes in the "wrong" direction, try giving a negative value for `angleEnd`.
\param resolution The resolution of the arc. The arc will be composed of `resolution` line segments.
\note This uses an ofVbo internally. If VBOs are not supported by your video card, this may not work at all.
*/
void arc(ofPoint center, float radiusX, float radiusY, float width, float angleBegin, float angleEnd, unsigned int resolution) {

	float d = width / 2;
	unsigned int vertexCount = resolution + 1;

	std::vector<ofPoint> vertices(2 * vertexCount);

	for (unsigned int i = 0; i < vertexCount; i++) {
		float angle = (angleEnd - angleBegin) * i / (vertexCount - 1) + angleBegin;
		angle = angle * PI / 180;

		vertices[(2 * i)] = center + ofPoint((radiusX - d) * cos(angle), (radiusY - d) * sin(angle));
		vertices[(2 * i) + 1] = center + ofPoint((radiusX + d) * cos(angle), (radiusY + d) * sin(angle));
	}

	ofVbo vbo;
	vbo.setVertexData(vertices.data(), vertices.size(), GL_STATIC_DRAW);
	vbo.draw(GL_TRIANGLE_STRIP, 0, vertices.size());
}




/*! Gets the vertices needed to draw a bezier curve.
\param controlPoints Control points for the bezier.
\param times A vector of "times" in the interval [0,1] giving the times at which 
to evaluate the bezier curve. Values outside of the interval [0,1] are clamped to be in the interval.
\return A vector of points along the bezier curve.
*/
std::vector<ofPoint> getBezierVertices(std::vector<ofPoint> controlPoints, std::vector<float> times) {
	std::vector< std::vector<LineSegment> > segs(controlPoints.size() - 1);
	for (unsigned int i = 0; i < segs.size(); i++) {
		segs[i].resize(controlPoints.size() - i - 1);
	}

	//Initialize layer 0
	for (unsigned int i = 0; i < controlPoints.size() - 1; i++) {
		segs[0][i].p1 = controlPoints[i];
		segs[0][i].p2 = controlPoints[i + 1];
	}

	std::vector<ofPoint> outputPoints(times.size());

	std::vector<ofPoint> nextLayerCP(segs.size());
	for (unsigned int ti = 0; ti < times.size(); ti++) {
		float t = Util::clamp<float>(times[ti], 0, 1);

		for (unsigned int layer = 0; layer < segs.size(); layer++) {
			for (unsigned int segment = 0; segment < segs[layer].size(); segment++) {
				ofPoint p = segs[layer][segment].pointAlong(t);
				nextLayerCP[segment] = p;
				if (layer == segs.size() - 1) {
					outputPoints[ti] = p;
				}
			}
			//You've finished layer i, prepare layer i + 1
			for (unsigned int i = 0; i < segs[layer].size() - 1; i++) {
				segs[layer + 1][i].p1 = nextLayerCP[i];
				segs[layer + 1][i].p2 = nextLayerCP[i + 1];
			}
		}
	}
	return outputPoints;
}

/*! Gets the vertices needed to draw a bezier curve.
\param controlPoints Control points for the bezier.
\param resolution Controls the approximation of the bezier curve. There will be `resolution` line segments drawn to
complete the curve (`resolution + 1` points).
\return A vector of points created based on the `controlPoints`. */
std::vector<ofPoint> getBezierVertices(std::vector<ofPoint> controlPoints, unsigned int resolution) {
	std::vector<float> times = Util::sequenceAlong<float>(0, 1, resolution + 1);

	return getBezierVertices(controlPoints, times);
}

/*! Draws a bezier curve with an arbitrary number of control points. May become slow with a large number
of control points. Uses de Casteljau's algorithm to calculate the curve points. See this awesome guide: 
http://pomax.github.io/bezierinfo/
\param controlPoints Control points for the bezier.
\param width The width of the lines to be drawn. Uses CX::Draw::lines(std::vector<ofPoint>, float) internally to draw the connecting lines.
\param resolution Controls the approximation of the bezier curve. There will be `resolution` line segments drawn to 
complete the curve (`resolution + 1` points).
*/
void bezier(std::vector<ofPoint> controlPoints, float width, unsigned int resolution) {
	Draw::lines(getBezierVertices(controlPoints, resolution), width);
}



/*! Convert between two color spaces. This conversion uses this library internally: http://www.getreuer.info/home/colorspace
\param conversionFormula A formula of the format "SRC -> DEST", where SRC and DEST are valid color spaces.
For example, if you wanted to convert from HSL to RGB, you would use "HSL -> RGB" as the formula. The whitespace
is immaterial, but the arrow must exist (the arrow can point either direction). See this page for options for 
the color space: http://www.getreuer.info/home/colorspace#TOC-MATLAB-Usage.

Ranges for the values for some common color spaces:
+ HSV/HSB/HSL/HSI: For any of these color spaces, H is in the range [0,360) and the other components are in the range [0,1].
+ RGB: All in [0,1].
+ LAB: L is in the range [0,100]. A and B have vague ranges, because at certain values, the color that results
cannot exist (an "imaginary color"). However, in general, A and B should be in the approximate range [-128,128],
although the edges are likely to be imaginary.

\param S1 Source coordinate 1. Corresponds to, e.g., the R in RGB.
\param S2 Source coordinate 2. Corresponds to, e.g., the G in RGB.
\param S3 Source coordinate 3. Corresponds to, e.g., the B in RGB.
\return An vector of length 3 containing the converted coordinates in the destination color space.
The value at index 0 corresponds to the first letter in the resulting color space and the next two
indices proceed as expected.

\code{.cpp}
vector<double> hslValues = Draw::convertColors("XYZ -> HSL", .7, .4, .6); //Convert x=.7, y=.4, z=.6 to HSL values.
hslValues[0]; //Access the hue value.
hslValues[2]; //Access the lightness value.
\endcode

\note The values returned by this function may not be in the allowed range for the destination color space. 
Make sure they are clamped to reasonable values if they are to be used directly.

\see CX::Draw::convertToRGB() is a convenience function for the most common conversion that will typically be done
(something to RGB).
*/
std::vector<double> convertColors(std::string conversionFormula, double S1, double S2, double S3) {
	std::vector<num> dest(3, 0);
	colortransform cTrans;

	if (((conversionFormula.find("->") == std::string::npos) && (conversionFormula.find("<-") == std::string::npos)) || 
		!GetColorTransform(&cTrans, conversionFormula.c_str()))
	{
		CX::Instances::Log.error() << "CX::Draw::convertColors: Invalid syntax or unknown color space. The provided conversion formula was \"" << 
			conversionFormula << "\"" << std::endl;
		return dest;
	}

	ApplyColorTransform(cTrans, &dest[0], &dest[1], &dest[2], S1, S2, S3);

	return dest;
}

/*! This function converts from an arbitrary color space to the RGB color space. This is convenient, 
because in order to draw stimuli with a color, you need to have the color in the RGB space. This
uses CX::Draw::convertColors(std::string, double, double, double), which provides more options.

\param inputColorSpace The color space to convert from. For example, if you wanted to convert from LAB
coordinates, you would provde the string "LAB". See this page for more options for the color space:
http://www.getreuer.info/home/colorspace#TOC-MATLAB-Usage (ignore the MATLAB title on that page; it's 
the same interface in both the MATLAB and C versions).
\param S1 Source coordinate 1. Corresponds to, e.g., the R in RGB.
\param S2 Source coordinate 2. Corresponds to, e.g., the G in RGB.
\param S3 Source coordinate 3. Corresponds to, e.g., the B in RGB.
\return An `ofFloatColor` contaning the RGB coordinates. Instances of `ofFloatColor` can be implicitly 
converted in assignment to other `ofColor` types.

\see Example code in the documentation for CX::Draw::colorWheel() uses this function.
*/
ofFloatColor convertToRGB(std::string inputColorSpace, double S1, double S2, double S3) {
	std::string conversionFormula = inputColorSpace + " -> RGB";

	std::vector<double> result = convertColors(conversionFormula, S1, S2, S3);

	return ofFloatColor(result[0], result[1], result[2]);
}


/*! Gets teh vertices defining the perimeter of a standard fixation cross (plus sign).
\param armLength The length of the arms of the cross (end to end, not from the center).
\param armWidth The width of the arms.
\return A vector with the 12 needed vertices. */
std::vector<ofPoint> getFixationCrossVertices(float armLength, float armWidth) {
	float w = armWidth / 2;
	float l = armLength / 2;

	std::vector<ofPoint> points(12);

	points[0] = ofPoint(w, l);
	points[1] = ofPoint(-w, l);
	points[2] = ofPoint(-w, w);

	points[3] = ofPoint(-l, w);
	points[4] = ofPoint(-l, -w);
	points[5] = ofPoint(-w, -w);

	points[6] = ofPoint(-w, -l);
	points[7] = ofPoint(w, -l);
	points[8] = ofPoint(w, -w);

	points[9] = ofPoint(l, -w);
	points[10] = ofPoint(l, w);
	points[11] = ofPoint(w, w);

	return points;
}

/*! Draws a standard fixation cross (plus sign) to an ofPath. The fixation cross will be centered on (0,0) in the ofPath.
\param armLength The length of the arms of the cross (end to end, not from the center).
\param armWidth The width of the arms.
\return An ofPath containing the fixation cross. */
ofPath fixationCrossToPath(float armLength, float armWidth) {
	std::vector<ofPoint> points = getFixationCrossVertices(armLength, armWidth);

	ofPath path;
	path.moveTo(points.back());
	for (auto p : points) {
		path.lineTo(p);
	}

	path.setFilled(true);
	path.setStrokeWidth(0);
	return path;
}

/*! Draws a standard fixation cross (plus sign).
\param location Where to draw the fixation cross.
\param armLength The length of the arms of the cross (end to end, not from the center).
\param armWidth The width of the arms. */
void fixationCross(ofPoint location, float armLength, float armWidth) {
	ofPath path = fixationCrossToPath(armLength, armWidth);
	path.setColor(ofGetStyle().color);
	path.draw(location.x, location.y);
}

/*! Saves the contents of an ofFbo to an image file. The file type is hinted by the file extension you provide
as part of the file name.
\param fbo The framebuffer to save.
\param filename The path of the file to save. The file extension determines the type of file that is saved.
If no file extention is given, nothing gets saved.
Many standard file types are supported: png, bmp, jpg, gif, etc. However, if the fbo has an alpha channel,
only png works properly (at least of those I have tested).
*/
void saveFboToFile(ofFbo& fbo, std::string filename) {
	ofPixels pix;
	fbo.readToPixels(pix);
	ofSaveImage(pix, filename, OF_IMAGE_QUALITY_BEST);
}


// \cond INTERNAL_DOCS

bool isPointInRegion(ofPoint p, ofPoint r1, ofPoint r2, float tolerance) {
	float lowerX = std::min(r1.x, r2.x) - tolerance;
	float upperX = std::max(r1.x, r2.x) + tolerance;
	float lowerY = std::min(r1.y, r2.y) - tolerance;
	float upperY = std::max(r1.y, r2.y) + tolerance;

	return (p.x >= lowerX) && (p.x <= upperX) && (p.y >= lowerY) && (p.y <= upperY);
}

bool arePointsInLine(ofPoint p1, ofPoint p2, ofPoint p3, float angleTolerance) {
	float a1 = Util::getAngleBetweenPoints(p1, p2);
	float a2 = Util::getAngleBetweenPoints(p2, p3);

	float dif = a1 - a2;
	if (abs(dif) > 90.0) {
		dif = abs(dif) - 180.0;
	}

	if (abs(dif) < angleTolerance) {
		return true;
	}
	return false;
}




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
		if (b == 0) {
			B = 1;
			C = b * B;
			A = -B * m;
		} else {
			C = 1;
			B = C / b;
			A = -B * m;
		}
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

// \endcond

/*! This function is an experimental attempt to draw a collection of lines in an idealized way. */
ofPath lines(std::vector<ofPoint> points, float width, LineCornerMode cornerMode) {
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
						//Middle point is between others. This should have been removed earlier, so do nothing now.
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
	path.setFilled(true);
	path.setStrokeWidth(0);
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



} //namespace Draw
} //namespace CX