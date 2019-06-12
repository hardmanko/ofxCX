#include "CX_UnitConversion.h"

#include "CX_Private.h"

namespace CX {
namespace Util {

	/*! Given point `ap` in rectangle `a`, find the corresponding point in rectangle `b`. */
	ofPoint mapPointBetweenRectangles(const ofPoint& ap, const ofRectangle& a, const ofRectangle& b) {

		float apxp = (ap.x - a.x) / a.getWidth();
		float apyp = (ap.y - a.y) / a.getHeight();

		ofPoint bp;
		bp.x = apxp * b.getWidth() + b.x;
		bp.y = apyp * b.getHeight() + b.y;

		return bp;
	}

	/*! Returns the number of pixels needed to subtend deg degrees of visual angle. You might want to round this
	if you want to align to pixel boundaries. However, if you are antialiasing your stimuli you
	might want to use floating point values to get precise subpixel rendering.
	\param degrees Number of degrees.
	\param pixelsPerUnit The number of pixels per distance unit on the target monitor. You can pick any unit of
	distance, as long as `viewingDistance` has the same unit.
	\param viewingDistance The distance of the viewer from the monitor, with the same distance unit as `pixelsPerUnit`.
	\return The number of pixels needed. */
	float degreesToPixels(float degrees, float pixelsPerUnit, float viewingDistance) {
		float rad = (degrees / 2) * PI / 180;
		float length = 2 * sin(rad) * viewingDistance;
		return length * pixelsPerUnit;
	}

	/*! The inverse of CX::Util::degreesToPixels(). */
	float pixelsToDegrees(float pixels, float pixelsPerUnit, float viewingDistance) {
		float length = pixels / pixelsPerUnit;
		float rad = asin(length / (2 * viewingDistance));
		return 2 * rad * 180 / PI;
	}

	//////////////////////////
	// BaseUnitConverter //
	//////////////////////////

	/*! Applies the unit conversion to a whole vector. */
	std::vector<float> BaseUnitConverter::operator() (const std::vector<float>& vx) {
		std::vector<float> rval(vx.size());
		for (unsigned int i = 0; i < rval.size(); i++) {
			rval[i] = this->operator()(vx[i]);
		}
		return rval;
	}

	/*! Applies the inverse unit conversion to a whole vector. */
	std::vector<float> BaseUnitConverter::inverse(const std::vector<float>& vy) {
		std::vector<float> rval(vy.size());
		for (unsigned int i = 0; i < rval.size(); i++) {
			rval[i] = this->inverse(vy[i]);
		}
		return rval;
	}


	///////////////////////////////
	// DegreeToPixelConverter //
	///////////////////////////////

	DegreeToPixelConverter::DegreeToPixelConverter(void) :
		_pixelsPerUnit(0),
		_viewingDistance(0),
		_roundResult(false)
	{}

	/*! Constructs an instance of a DegreeToPixelConverter with the given configuration. See setup() for the
	meaning of the parameters. */
	DegreeToPixelConverter::DegreeToPixelConverter(float pixelsPerUnit, float viewingDistance, bool roundResult) {
		setup(pixelsPerUnit, viewingDistance, roundResult);
	}

	/*! Sets up a DegreeToPixelConverter with the given configuration.
	\param pixelsPerUnit The number of pixels within one length unit (e.g. inches, centimeters). This can
	be measured by drawing an object with a known size on the screen and measuring the length of a side and dividing
	the number of pixels by the total length measured.
	\param viewingDistance The distance from the monitor that the participant will be viewing the screen from.
	\param roundResult If true, the result of conversions will be rounded to the nearest integer (i.e. pixel).
	For drawing certain kinds of stimuli (especially text) it can be helpful to draw on pixel boundaries. */
	void DegreeToPixelConverter::setup(float pixelsPerUnit, float viewingDistance, bool roundResult) {
		_pixelsPerUnit = pixelsPerUnit;
		_viewingDistance = viewingDistance;
		_roundResult = roundResult;
	}

	/*! This function exists to serve a per-computer configuration function that is otherwise difficult to provide
	due to the fact that C++ programs are compiled to binaries and cannot be easily edited on the computer on which
	they are running. This function takes the file name of a specially constructed configuration file and reads the
	key-value pairs in that file in order to configure the DegreeToPixelConverter. The format of the file is
	provided in the example code below.

	Sample configuration file:
	\code
	D2PC.pixelsPerUnit = 35
	D2PC.viewingDistance = 50
	D2PC.roundResult = true
	\endcode

	All of the configuration keys are used in this example.
	Note that the "D2PC" prefix allows this configuration to be embedded in a file that also performs other configuration functions.

	See DegreeToPixelConverter::setup() for details about the meanings of the configuration options.

	Because this function uses CX::Util::readKeyValueFile() internally, it has the same arguments.
	\param filename The name of the file containing configuration data.
	\param delimiter The string that separates the key from the value. In the example, it is "=", but can be other values.
	\param trimWhitespace If true, whitespace characters surrounding both the key and value will be removed. This is a good idea to do.
	\param commentString If `commentString` is not the empty string (""), everything on a line
	following the first instance of `commentString` will be ignored.
	\return `true` if there were no problems reading in the file, `false` otherwise.
	*/
	bool DegreeToPixelConverter::configureFromFile(std::string filename, std::string delimiter, bool trimWhitespace, std::string commentString) {
		std::map<std::string, std::string> kv = Util::readKeyValueFile(filename, delimiter, trimWhitespace, commentString);
		bool success = true;

		if (kv.find("D2PC.pixelsPerUnit") != kv.end()) {
			_pixelsPerUnit = ofFromString<float>(kv.at("D2PC.pixelsPerUnit"));
		}

		if (kv.find("D2PC.viewingDistance") != kv.end()) {
			_viewingDistance = ofFromString<float>(kv.at("D2PC.viewingDistance"));
		}

		if (kv.find("D2PC.roundResult") != kv.end()) {
			int result = Private::stringToBooleint(kv["D2PC.roundResult"]);
			if (result != -1) {
				_roundResult = (result == 1);
			} else {
				success = false;
			}
		}
		return success;
	}

	/*! Converts the degrees to pixels based on the settings given during construction.
	\param degrees The number of degrees of visual angle to convert to pixels.
	\return The number of pixels corresponding to the number of degrees of visual angle. */
	float DegreeToPixelConverter::operator() (float degrees) {
		float px = degreesToPixels(degrees, _pixelsPerUnit, _viewingDistance);
		if (_roundResult) {
			px = CX::Util::round(px, 0, CX::Util::Rounding::ToNearest);
		}
		return px;
	}

	/*! Performs the inverse of the operation performed by operator(), i.e. converts pixels to degrees.
	\param pixels The number of pixels to convert to degrees.
	\return The number of degrees of visual angle subtended by the given number of pixels. */
	float DegreeToPixelConverter::inverse(float pixels) {
		float deg = pixelsToDegrees(pixels, _pixelsPerUnit, _viewingDistance);
		//if (_roundResult) {
		//	deg = CX::Util::round(deg, 0, CX::Util::Rounding::ToNearest);
		//}
		return deg;
	}

	///////////////////////////////
	// LengthToPixelConverter //
	///////////////////////////////

	LengthToPixelConverter::LengthToPixelConverter(void) :
		_pixelsPerUnit(0),
		_roundResult(false)
	{}

	/*! Constructs a LengthToPixelConverter with the given configuration. See setup() for the meaning of the parameters. */
	LengthToPixelConverter::LengthToPixelConverter(float pixelsPerUnit, bool roundResult) {
		setup(pixelsPerUnit, roundResult);
	}

	/*! Sets up a LengthToPixelConverter with the given configuration.
	\param pixelsPerUnit The number of pixels per one length unit. This can
	be measured by drawing a ~100-1000 pixel square on the screen and measuring the length of a side and dividing
	the number of pixels by the total length measured.
	\param roundResult If true, the result of conversions will be rounded to the nearest integer (i.e. pixel).
	For drawing certain kinds of stimuli (especially text) it can be helpful to draw on pixel boundaries. */
	void LengthToPixelConverter::setup(float pixelsPerUnit, bool roundResult) {
		_pixelsPerUnit = pixelsPerUnit;
		_roundResult = roundResult;
	}

	/*! This function exists to serve a per-computer configuration function that is otherwise difficult to provide
	due to the fact that C++ programs are compiled to binaries and cannot be easily edited on the computer on which
	they are running. This function takes the file name of a specially constructed configuration file and reads the
	key-value pairs in that file in order to configure the LengthToPixelConverter. The format of the file is
	provided in the example code below.

	Sample configuration file:
	\code
	L2PC.pixelsPerUnit = 35
	L2PC.roundResult = true
	\endcode

	All of the configuration keys are used in this example.
	Note that the "L2PC" prefix allows this configuration to be embedded in a file that also performs other configuration functions.

	See LengthToPixelConverter::setup() for details about the meanings of the configuration options.

	Because this function uses CX::Util::readKeyValueFile() internally, it has the same arguments.
	\param filename The name of the file containing configuration data.
	\param delimiter The string that separates the key from the value. In the example, it is "=", but can be other values.
	\param trimWhitespace If true, whitespace characters surrounding both the key and value will be removed. This is a good idea to do.
	\param commentString If `commentString` is not the empty string (""), everything on a line
	following the first instance of `commentString` will be ignored.
	\return `true` if there were no problems reading in the file, `false` otherwise.
	*/
	bool LengthToPixelConverter::configureFromFile(std::string filename, std::string delimiter, bool trimWhitespace, std::string commentString) {
		std::map<std::string, std::string> kv = Util::readKeyValueFile(filename, delimiter, trimWhitespace, commentString);
		bool success = true;

		if (kv.find("L2PC.pixelsPerUnit") != kv.end()) {
			_pixelsPerUnit = ofFromString<float>(kv.at("L2PC.pixelsPerUnit"));
		}

		if (kv.find("L2PC.roundResult") != kv.end()) {
			int result = Private::stringToBooleint(kv["L2PC.roundResult"]);
			if (result != -1) {
				_roundResult = (result == 1);
			} else {
				success = false;
			}
		}
		return success;
	}

	/*! Converts the length to pixels based on the settings given during construction.
	\param length The length to convert to pixels.
	\return The number of pixels corresponding to the length. */
	float LengthToPixelConverter::operator() (float length) {
		float px = length * _pixelsPerUnit;
		if (_roundResult) {
			px = CX::Util::round(px, 0, CX::Util::Rounding::ToNearest);
		}
		return px;
	}

	/*! Performs to inverse of operator(), i.e. converts pixels to length.
	\param pixels The number of pixels to convert to a length.
	\return The length of the given number of pixels. */
	float LengthToPixelConverter::inverse(float pixels) {
		float length = pixels / _pixelsPerUnit;
		//if (_roundResult) {
		//	length = CX::Util::round(length, 0, CX::Util::Rounding::ToNearest);
		//}
		return length;
	}

	////////////////////////////
	// CoordinateConverter //
	////////////////////////////

	/*! Constructs a CoordinateConverter with the default settings. The settings can be changed later with
	setAxisInversion(), setOrigin(), setMultiplier(), and/or setUnitConverter(). */
	CoordinateConverter::CoordinateConverter(void) :
		_origin(ofPoint(0,0)),
		_multiplier(1.0),
		_conv(nullptr)
	{
		setAxisInversion(false, false, false);
	}

	/*! Constructs a CoordinateConverter with the given settings.
	\param origin The location within the standard coordinate system at which the origin (the point at which the x,
	y, and z values are 0) of the user-defined coordinate system is located.
	If, for example, you want the center of the display to be the origin within your user-defined coordinate system,
	you could use CX_Display::getCenter() as the value for this argument.
	\param invertX Invert the x-axis from the default, which is that x increases to the right.
	\param invertY Invert the y-axis from the default, which is that y increases downward.
	\param invertZ Invert the z-axis from the default, which is that z increases toward the user
	(i.e. pointing out of the front of the screen). The other way of saying this is that smaller
	(increasingly negative) values are farther away. */
	CoordinateConverter::CoordinateConverter(ofPoint origin, bool invertX, bool invertY, bool invertZ) :
		_origin(origin),
		_multiplier(1.0),
		_conv(nullptr)
	{
		setAxisInversion(invertX, invertY, invertZ);
	}

	/*! Sets whether each axis within the user-defined system is inverted from the standard coordinate system.
	\param invertX Invert the x-axis from the default, which is that x increases to the right.
	\param invertY Invert the y-axis from the default, which is that y increases downward.
	\param invertZ Invert the z-axis from the default, which is that z increases toward the viewer
	(i.e. pointing out of the front of the screen).
	*/
	void CoordinateConverter::setAxisInversion(bool invertX, bool invertY, bool invertZ) {
		_inversionCoefficients = ofPoint((invertX ? -1 : 1), (invertY ? -1 : 1), (invertZ ? -1 : 1));
	}

	/*! Sets the location within the standard coordinate system at which the origin
	of the user-defined coordinate system is located.
	\param newOrigin The location within the standard coordinate system at which the origin (the point at which the x,
	y, and z values are 0) of the user-defined coordinate system is located.
	If, for example, you want the center of the display to be the origin within your user-defined coordinate system,
	you could use CX_Display::getCenter() as the value for this argument. */
	void CoordinateConverter::setOrigin(ofPoint newOrigin) {
		_origin = newOrigin;
	}

	/*! This function sets the amount by which user coordinates are multiplied
	before they are converted to standard coordinates. This allows you to easily
	scale stimuli, assuming that the CoordinateConverter is used throughout.
	If it has not been set, the multiplier is 1 by default.
	\param multiplier The amount to multiply user coordinates by.
	*/
	void CoordinateConverter::setMultiplier(float multiplier) {
		_multiplier = multiplier;
	}

	/*! The primary method of conversion between coordinate systems. You supply a point in
	user coordinates and get in return a point in standard coordinates.

	Example use:

	\code{.cpp}
	CoordinateConverter cc(ofPoint(200,200), false, true);
	ofPoint p(-50, 100); //P is in user-defined coordinates, 50 units left and 100 units above the origin.
	ofPoint res = cc(p); //Use operator() to convert from the user system to the standard system.
	//res should contain (150, 100) due to the inverted y axis.
	\endcode

	\param p The point in user coordinates that should be converted to standard coordinates.
	\return The point in standard coordinates.
	*/
	ofPoint CoordinateConverter::operator() (ofPoint p) {
		p *= _multiplier;
		p *= _inversionCoefficients;

		if (_conv != nullptr) {
			p.x = (*_conv)(p.x);
			p.y = (*_conv)(p.y);
			p.z = (*_conv)(p.z);
		}

		p += _origin;
		return p;
	}

	/*! Equivalent to a call to `operator()(ofPoint(x, y, z));`. */
	ofPoint CoordinateConverter::operator() (float x, float y, float z) {
		return this->operator()(ofPoint(x, y, z));
	}

	/*! Performs the inverse of operator(), i.e. converts from standard coordinates to user coordinates.
	\param p A point in standard coordinates.
	\return A point in user coordinates. */
	ofPoint CoordinateConverter::inverse(ofPoint p) {
		p -= _origin;

		if (_conv != nullptr) {
			p.x = _conv->inverse(p.x);
			p.y = _conv->inverse(p.y);
			p.z = _conv->inverse(p.z);
		}

		p /= _inversionCoefficients;
		p /= _multiplier;
		return p;
	}

	/*! Equivalent to `inverse(ofPoint(x,y,z));` */
	ofPoint CoordinateConverter::inverse(float x, float y, float z) {
		return this->inverse(ofPoint(x, y, z));
	}

	/*! Applies the conversion on a whole vector of points at once.	
	\param p The vector of points to convert.
	\return The converted points.
	*/
	std::vector<ofPoint> CoordinateConverter::operator() (const std::vector<ofPoint>& p) {
		std::vector<ofPoint> rval(p.size());
		for (unsigned int i = 0; i < rval.size(); i++) {
			rval[i] = this->operator()(p[i]);
		}
		return rval;
	}

	/*! Applies the inverse conversion on a whole vector of points at once.	
	\param p The vector of points to inverse convert.
	\return The inverse converted points.
	*/
	std::vector<ofPoint> CoordinateConverter::inverse(const std::vector<ofPoint>& p) {
		std::vector<ofPoint> rval(p.size());
		for (unsigned int i = 0; i < rval.size(); i++) {
			rval[i] = this->inverse(p[i]);
		}
		return rval;
	}

	/*! Sets the unit converter that will be used when converting the coordinate system.
	In this way you can convert both the coordinate system in use and the units used by
	the coordinate system in one step. See DegreeToPixelConverter and
	LengthToPixelConverter for examples of the converters that can be used.

	Example use:

	\code{.cpp}
	//At global scope:
	CoordinateConverter conv(ofPoint(0,0), false, true); //The origin will be set to a proper value later.
	DegreeToPixelConverter d2p(35, 70);

	//During setup:
	conv.setOrigin(Disp.getCenter());
	conv.setUnitConverter(&d2p); //Use degrees of visual angle as the units of the user coordinate system.

	//Draw a blue circle 2 degrees of visual angle to the left of the origin and 3 degrees above (inverted y-axis) the origin.
	ofSetColor(0, 0, 255);
	ofCircle(conv(-2, 3), 20);
	\endcode

	\param converter A pointer to an instance of a class that is a BaseUnitConverter
	or which has inherited from that class. See CX_UnitConversion.h/cpp for the implementation
	of LengthToPixelConverter to see an example of how to create you own converter.

	\note The origin of the coordinate converter must be in the units that result from the
	unit conversion. E.g. if you are converting the units from degrees to pixels, the origin
	must be in pixels. See setOrigin().
	\note The unit converter passed to this function must continue to exist throughout the
	lifetime of the coordinate converter. It is not copied.
	*/
	void CoordinateConverter::setUnitConverter(BaseUnitConverter *converter) {
		_conv = converter;
	}

} //namespace Util
} //namespace CX
