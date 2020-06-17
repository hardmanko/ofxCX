#pragma once

#include "ofPoint.h"

#include "CX_Utilities.h"

namespace CX {
namespace Util {

	ofPoint mapPointBetweenRectangles(const ofPoint& ap, const ofRectangle& a, const ofRectangle& b);

	class RectangleMapper {
	public:
		ofRectangle a;
		ofRectangle b;

		ofPoint a2b(ofPoint ap) const {
			return mapPointBetweenRectangles(ap, a, b);
		}

		ofPoint b2a(ofPoint bp) const {
			return mapPointBetweenRectangles(bp, b, a);
		}

		float wa2b(float aw) const {
			return aw * b.getWidth() / a.getWidth();
		}

		float ha2b(float ah) const {
			return ah * b.getHeight() / a.getHeight();
		}

		float wb2a(float bw) const {
			return bw * a.getWidth() / b.getWidth();
		}

		float hb2a(float bh) const {
			return bh * a.getHeight() / b.getHeight();
		}

	};

	float degreesToPixels(float degrees, float pixelsPerUnit, float viewingDistance);
	float pixelsToDegrees(float pixels, float pixelsPerUnit, float viewingDistance);

	/*! This class should be inherited from by any unit converters. You should override 
	both `operator()` and `inverse()`. `inverse()` should perform the mathematical inverse of 
	the operation performed by `operator()`. */
	class BaseUnitConverter {
	public:
		/*! `operator()` should perform the unit conversion. */
		virtual float operator() (float x) {
			return (5 * x) - 2; //y = 5x - 2
		};

		/*! `inverse()` should perform the inverse operation as `operator()`. */
		virtual float inverse(float y) {
			return (y + 2) / 5; //x = (y + 2)/5
		}

		//No need to override these functions unless your conversion does something really unusual.
		virtual std::vector<float> operator() (const std::vector<float>& vx);
		virtual std::vector<float> inverse(const std::vector<float>& vy);
	};

	/*! This simple utility class is used for converting degrees of visual angle to pixels on a monitor.
	This class uses CX::Util::degreesToPixels() internally. See also CX::Util::CoordinateConverter for
	a way to also convert from one coordinate system to another.

	Example use:

	\code{.cpp}
	Util::DegreeToPixelConverter d2p(34, 60); //34 pixels per unit length (e.g. cm) on the target monitor, user is 60 length units from monitor.
	ofLine( 200, 100, 200 + d2p(1), 100 + d2p(2) ); //Draw a line from (200, 100) (in pixel coordinates) to 1 degree
	//to the right and 2 degrees below that point.
	\endcode		

	\ingroup utility */
	class DegreeToPixelConverter : public BaseUnitConverter {
	public:
		DegreeToPixelConverter(void);
		DegreeToPixelConverter(float pixelsPerUnit, float viewingDistance, bool roundResult = false);

		void setup(float pixelsPerUnit, float viewingDistance, bool roundResult = false);

		float operator() (float degrees) override;
		float inverse(float pixels) override;

		bool configureFromFile(std::string filename, std::string delimiter = "=", bool trimWhitespace = true, std::string commentString = "//");

	private:
		float _pixelsPerUnit;
		float _viewingDistance;
		bool _roundResult;
	};

	/*! This simple utility class is used for converting lengths (perhaps of objects drawn on the monitor) to
	pixels on a monitor. See also CX::Util::CoordinateConverter for a way to also convert from one coordinate 
	system to another. This assumes that pixels are square, which may not be true, especially if you are using
	a resolution that is not the native resolution of the monitor.

	Example use:

	\code{.cpp}
	Util::LengthToPixelConverter l2p(75); //75 pixels per unit length (e.g. inch) on the target monitor.
	ofLine( 200, 100, 200 + l2p(1), 100 + l2p(2) ); //Draw a line from (200, 100) (in pixel coordinates) to 
	//1 distance unit right and 2 units down from that point.
	\endcode

	\ingroup utility */
	class LengthToPixelConverter : public BaseUnitConverter {
	public:
		LengthToPixelConverter(void);
		LengthToPixelConverter(float pixelsPerUnit, bool roundResult = false);

		void setup(float pixelsPerUnit, bool roundResult = false);

		float operator() (float length) override;
		float inverse(float pixels) override;

		bool configureFromFile(std::string filename, std::string delimiter = "=", bool trimWhitespace = true, std::string commentString = "//");

	private:
		float _pixelsPerUnit;
		bool _roundResult;
	};


	/*! This helper class is used for converting from a somewhat user-defined coordinate system into
	the standard computer monitor coordinate system. When user coordinates are input into this class,
	they will be converted into the standard monitor coordinate system. This lets you use coordinates 
	in your own system and convert those coordinates into the standard coordinates that are used by the 
	drawing functions. Note that this does not always play nicely when dealing with angles.

	See CoordinateConverter::setUnitConverter() for a way to do change the units of the coordinate 
	system to, for example, inches or degrees of visual angle.

	Example use:

	\code{.cpp}
	Util::CoordinateConverter conv(Disp.getCenter(), false, true); //Make the center of the display the origin and invert
	//the Y-axis. This makes positive x values go to the right and positive y values go up from the center of the display.
	ofSetColor(255, 0, 0); //Draw a red circle in the center of the display.
	ofCircle(conv(0, 0), 20);
	ofSetColor(0, 255, 0); //Draw a green circle 100 pixels to the right of the center.
	ofCircle(conv(100, 0), 20);
	ofSetColor(0, 0, 255); //Draw a blue circle 100 pixels above the center (inverted y-axis).
	ofCircle(conv(0, 100), 20);
	\endcode

	Another example of the use of this class can be found in the advancedChangeDetection example experiment.

	If you want to invert the y-axis, you may be better off using CX::CX_Display::setYIncreasesUp().

	\ingroup utility */
	class CoordinateConverter {
	public:

		CoordinateConverter(void);
		CoordinateConverter(ofPoint origin, bool invertX, bool invertY, bool invertZ = false);

		void setAxisInversion(bool invertX, bool invertY, bool invertZ = false);
		void setOrigin(ofPoint newOrigin);
		void setMultiplier(float multiplier);
		void setUnitConverter(BaseUnitConverter *converter);

		ofPoint operator() (ofPoint p);
		ofPoint operator() (float x, float y, float z = 0);

		ofPoint inverse(ofPoint p);
		ofPoint inverse(float x, float y, float z = 0);

		std::vector<ofPoint> operator() (const std::vector<ofPoint>& p);
		std::vector<ofPoint> inverse(const std::vector<ofPoint>& p);

	private:
		ofPoint _origin;
		ofPoint _inversionCoefficients;

		float _multiplier;

		BaseUnitConverter *_conv;
	};



} // namespace Util
} // namespace CX