#include "CX_UnitConversion.h"

namespace CX {
namespace Util {

	/*! Returns the number of pixels needed to subtend deg degrees of visual angle. You might want to round this
	if you want to align to pixel boundaries. However, if you are antialiasing your stimuli you
	might want to use floating point values to get precise subpixel rendering.
	\param deg Number of degrees.
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


	/*! Constructs an instance of a CX_DegreeToPixelConverter using the given settings.
	\param pixelsPerUnit The number of pixels within one length unit (e.g. inches, centimeters). This can
	be measured by drawing a ~100-1000 pixel square on the screen and measuring the length of a side and dividing
	the number of pixels by the total length measured.
	\param viewingDistance The distance from the monitor that the participant will be viewing the screen from.
	\param roundResult If true, the result of conversions will be rounded to the nearest integer (i.e. pixel).
	For drawing certain kinds of stimuli (especially text) it can be helpful to draw on pixel boundaries. */
	CX_DegreeToPixelConverter::CX_DegreeToPixelConverter(float pixelsPerUnit, float viewingDistance, bool roundResult) :
		_pixelsPerUnit(pixelsPerUnit),
		_viewingDistance(viewingDistance),
		_roundResult(roundResult)
	{}

	/*! Converts the degrees to pixels based on the settings given during construction.
	\param degrees The number of degrees of visual angle to convert to pixels.
	\return The number of pixels corresponding to the number of degrees of visual angle.
	*/
	float CX_DegreeToPixelConverter::operator() (float degrees) {
		float px = degreesToPixels(degrees, _pixelsPerUnit, _viewingDistance);
		if (_roundResult) {
			px = CX::Util::round(px, 0, CX::Util::CX_RoundingConfiguration::ROUND_TO_NEAREST);
		}
		return px;
	}

	/*! Constructs a CX_LengthToPixelConverter with the given configuration.
	\param pixelsPerUnit The number of pixels per one length unit. This can
	be measured by drawing a ~100-1000 pixel square on the screen and measuring the length of a side and dividing
	the number of pixels by the total length measured.
	\param roundResult If true, the result of conversions will be rounded to the nearest integer (i.e. pixel).
	For drawing certain kinds of stimuli (especially text) it can be helpful to draw on pixel boundaries. */
	CX_LengthToPixelConverter::CX_LengthToPixelConverter(float pixelsPerUnit, bool roundResult) :
		_pixelsPerUnit(pixelsPerUnit),
		_roundResult(roundResult)
	{}

	/*! Converts the length to pixels based on the settings given during construction.
	\param length The length to convert to pixels.
	\return The number of pixels corresponding to the length. */
	float CX_LengthToPixelConverter::operator() (float length) {
		float px = length * _pixelsPerUnit;
		if (_roundResult) {
			px = CX::Util::round(px, 0, CX::Util::CX_RoundingConfiguration::ROUND_TO_NEAREST);
		}
		return px;
	}

	/*! Constructs a coordinate converter with the given settings.
	\param origin The location within the standard coordinate system at which the origin (the point at which the x,
	y, and z values are 0) of the user-defined coordinate system is located.
	If, for example, you want the center of the display to be the origin within your user-defined coordinate system, 
	you could use CX_Display::getCenterOfDisplay() as the value for this argument.
	\param invertX Invert the x-axis from the default, which is that x increases to the right.
	\param invertY Invert the y-axis from the default, which is that y increases downward.
	\param invertZ Invert the z-axis from the default, which is that z increases toward the user
	(i.e. pointing out of the front of the screen).
	*/
	CX_CoordinateConverter::CX_CoordinateConverter(ofPoint origin, bool invertX, bool invertY, bool invertZ) :
		_origin(origin),
		_invertX(invertX),
		_invertY(invertY),
		_invertZ(invertZ),
		_conv(nullptr),
		_multiplier(1.0)
	{}

	/*! Sets whether each axis within the user-defined system is inverted from the standard coordinate system. 
	\param invertX Invert the x-axis from the default, which is that x increases to the right.
	\param invertY Invert the y-axis from the default, which is that y increases downward.
	\param invertZ Invert the z-axis from the default, which is that z increases toward the viewer
	(i.e. pointing out of the front of the screen).
	*/
	void CX_CoordinateConverter::setAxisInversion(bool invertX, bool invertY, bool invertZ) {
		_invertX = invertX;
		_invertY = invertY;
		_invertZ = invertZ;
		//_orientations = ofPoint(invertX ? -1 : 1, invertY ? -1 : 1, invertZ ? -1 : 1);
	}

	/*! Sets the location within the standard coordinate system at which the origin
	of the user-defined coordinate system is located.
	\param newOrigin The location within the standard coordinate system at which the origin (the point at which the x,
	y, and z values are 0) of the user-defined coordinate system is located.
	If, for example, you want the center of the display to be the origin within your user-defined coordinate system, 
	you could use CX_Display::getCenterOfDisplay() as the value for this argument. */
	void CX_CoordinateConverter::setOrigin(ofPoint newOrigin) {
		_origin = newOrigin;
	}

	/*! This function sets the amount by which user coordinates are multiplied
	before they are converted to standard coordinates. This allows you to easily
	scale stimuli. The multiplier is 1 by default.
	\param multiplier The amount to multiply user coordinates by.
	*/
	void CX_CoordinateConverter::setMultiplier(float multiplier) {
		_multiplier = multiplier;
	}

	/*! The primary method of conversion between coordinate systems. You supply a point in
	user coordinates and get in return a point in standard coordinates.
		
	Example use:

	\code{.cpp}
	CX_CoordinateConverter cc(ofPoint(200,200), false, true);
	ofPoint p(-50, 100); //P is in user-defined coordinates, 50 units left and 100 units above the origin.
	ofPoint res = cc(p); //Use operator() to convert from the user system to the standard system.
	//res should contain (150, 100) due to the inverted y axis.
	\endcode

	\param p The point in user coordinates that should be converted to standard coordinates.
	\return The point in standard coordinates.
	*/
	ofPoint CX_CoordinateConverter::operator() (ofPoint p) {

		p *= _multiplier;

		if (_conv != nullptr) {
			p.x = _origin.x + (*_conv)((_invertX ? -1 : 1) * p.x);
			p.y = _origin.y + (*_conv)((_invertY ? -1 : 1) * p.y);
			p.z = _origin.z + (*_conv)((_invertZ ? -1 : 1) * p.z);
		} else {
			p.x = _origin.x + ((_invertX ? -1 : 1) * p.x);
			p.y = _origin.y + ((_invertY ? -1 : 1) * p.y);
			p.z = _origin.z + ((_invertZ ? -1 : 1) * p.z);
		}

		return p;
	}

	/*! Equivalent to a call to operator()(ofPoint(x, y, z)). */
	ofPoint CX_CoordinateConverter::operator() (float x, float y, float z) {
		return this->operator()(ofPoint(x, y, z));
	}

	/*! Sets the unit converter that will be used when converting the coordinate system.
	In this way you can convert both the coordinate system in use and the units used by
	the coordinate system in one step. See CX_DegreeToPixelConverter and 
	CX_LengthToPixelConverter for examples of the converters that can be used.

	Example use:

	\code{.cpp}
	//At global scope:
	CX_CoordinateConverter conv(ofPoint(0,0), false, true); //The origin will be set to a proper value later.
	CX_DegreeToPixelConverter d2p(35, 70);

	//During setup:
	conv.setOrigin(Display.getCenterOfDisplay());
	conv.setUnitConverter(&d2p); //Use degrees of visual angle as the units of the user coordinate system.

	//Draw a blue circle 2 degrees of visual angle to the left of the origin and 3 degrees above (inverted y-axis) the origin.
	ofSetColor(0, 0, 255); 
	ofCircle(conv(-2, 3), 20);
	\endcode

	\param converter A pointer to an instance of a class that is a CX_BaseUnitConverter
	or which has inherited from that class. See CX_UnitConversion.h/cpp for the implementation
	of CX_LengthToPixelConverter to see an example of how to create you own converter.

	\note The origin of the coordinate converter must be in the units that result from the
	unit conversion. E.g. if you are converting the units from degrees to pixels, the origin
	must be in pixels. See setOrigin().
	\note The unit converter passed to this function must continue to exist throughout the
	lifetime of the coordinate converter. It is not copied.
	*/
	void CX_CoordinateConverter::setUnitConverter(CX_BaseUnitConverter *converter) {
		_conv = converter;
	}

} //namespace Util
} //namespace CX
