#include "ofPoint.h"

#include "CX_Utilities.h"

namespace CX {
	namespace Util {

		float degreesToPixels(float degrees, float pixelsPerUnit, float viewingDistance);
		float pixelsToDegrees(float pixels, float pixelsPerUnit, float viewingDistance);

		class CX_BaseUnitConverter {
		public:
			virtual float operator() (float inputUnits) {
				return inputUnits;
			};
		};

		/*! This simple utility class is used for converting degrees of visual angle to pixels on a monitor.
		This class uses CX::Util::degreesToPixels() internally. See also CX::Util::CX_CoordinateConverter for
		a way to also convert from one coordinate system to another.

		Example use:

		\code{.cpp}
		CX_DegreeToPixelConverter d2p(34, 60); //34 pixels per unit length (e.g. cm) on the target monitor, user is 60 length units from monitor.
		ofLine( 200, 100, 200 + d2p(1), 100 + d2p(2) ); //Draw a line from (200, 100) (in pixel coordinates) to 1 degree
		//to the right and 2 degrees below that point.
		\endcode

		\ingroup utility */
		class CX_DegreeToPixelConverter : public CX_BaseUnitConverter {
		public:
			CX_DegreeToPixelConverter(float pixelsPerUnit, float viewingDistance, bool roundResult = false);
			float operator() (float degrees);

		private:
			float _pixelsPerUnit;
			float _viewingDistance;
			bool _roundResult;
		};

		/*! This simple utility class is used for converting lengths (perhaps of objects drawn on the monitor) to
		pixels on a monitor. See also CX::Util::CX_CoordinateConverter for a way to also convert from one coordinate 
		system to another.

		Example use:

		\code{.cpp}
		CX_LengthToPixelConverter l2p(75); //75 pixels per unit length (e.g. inch) on the target monitor.
		ofLine( 200, 100, 200 + l2p(1), 100 + l2p(2) ); //Draw a line from (200, 100) (in pixel coordinates) to 1 unit
		//horizontally and 2 units vertically from that point.
		\endcode

		\ingroup utility */
		class CX_LengthToPixelConverter : public CX_BaseUnitConverter {
		public:
			CX_LengthToPixelConverter(float pixelsPerUnit, bool roundResult = false);
			float operator() (float length);

		private:
			float _pixelsPerUnit;
			bool _roundResult;
		};


		/*! This helper class is used for converting from a somewhat user-defined coordinate system into
		the standard computer monitor coordinate system. When user coordinates are input into this class,
		they will be converted into the standard monitor coordinate system. This lets you use this class
		to allow you to use coordinates in your own system and convert those coordinates into the standard
		coordinates that are used by the drawing functions of openFrameworks.

		See setUnitConverter() for a way to do change the units of the coordinate system to, for example, 
		inches or degrees of visual angle.

		Example use:

		\code{.cpp}
		CX_CoordinateConverter conv(Display.getCenterOfDisplay(), false, true); //Make the center of the display the origin and invert
		//the Y-axis. This makes positive x values go to the right and positive y values go up from the center of the display.
		ofSetColor(255, 0, 0); //Draw a red circle in the center of the display.
		ofCircle(conv(0, 0), 20);
		ofSetColor(0, 255, 0); //Draw a green circle 100 pixels to the right of the center.
		ofCircle(conv(100, 0), 20);
		ofSetColor(0, 0, 255); //Draw a blue circle 100 pixels above the center (inverted y-axis).
		ofCircle(conv(0, 100), 20);
		\endcode

		\ingroup utility */
		class CX_CoordinateConverter {
		public:

			CX_CoordinateConverter(ofPoint origin, bool invertX, bool invertY, bool invertZ = false);

			ofPoint operator() (ofPoint p);
			ofPoint operator() (float x, float y, float z = 0);

			void setAxisInversion(bool invertX, bool invertY, bool invertZ = false);
			void setOrigin(ofPoint newOrigin);

			void setUnitConverter(CX_BaseUnitConverter *converter);

		private:
			ofPoint _origin;
			bool _invertX;
			bool _invertY;
			bool _invertZ;

			//ofPoint _orientations;

			CX_BaseUnitConverter *_conv;
		};
	}
}