#include "CX_Utilities.h"

#include "ofMain.h"
#include "ofConstants.h"

using namespace std;
using namespace CX;

static unsigned int multisamplingSampleCount = 4; //This is set during setup

//This should not be used in user code to set the MSAA sample count because if this is set
//after the window is opened, it only affects FBOs and not the primary buffers (e.g. GL_BACK, 
//GL_FRONT). Use CX::relaunchWindow() to set the MSAA sample count.
void CX::Private::setMsaaSampleCount(unsigned int count) {
	multisamplingSampleCount = count;
}

namespace CX {
namespace Util {

	/*! This function retrieves the MSAA (http://en.wikipedia.org/wiki/Multisample_anti-aliasing)
	sample count. The sample count can be set by calling CX::relaunchWindow() with the desired sample
	count set in the argument to relaunchWindow(). */
	unsigned int getMsaaSampleCount(void) {
		return multisamplingSampleCount;
	};

	/*! Checks that the version of oF that is used during compilation matches the requested version. If the desired version
	was 0.8.1, simply input (0, 8, 1) for `versionMajor`, `versionMinor`, and `versionPatch`, respectively.
	\param log If `true`, a version mismatch will result in a warning being logged.
	\return `true` if the versions match, `false` otherwise. */
	bool checkOFVersion(int versionMajor, int versionMinor, int versionPatch, bool log) {
		if (versionMajor == OF_VERSION_MAJOR && versionMinor == OF_VERSION_MINOR && versionPatch == OF_VERSION_PATCH) {
			return true;
		}
		if (log) {
			CX::Instances::Log.warning("CX::Util::checkOFVersion") << 
				"openFrameworks version does not match target version. Current oF version: " << ofGetVersionInfo();
		}
		return false;
	}

	/*! Writes data to a file, either appending the data to an existing file or creating a new file, overwriting
	any existing file with the given filename.
	\param filename Name of the file to write to. If it is a relative file name, it will be placed relative
	the the data directory.
	\param data The data to write
	\param append If true, data will be appended to an existing file, if it exists. If append is false, any
	existing file will be overwritten and a warning will be logged. If no file exists, a new one will be created.
	\return True if an error was encountered while writing the file, true otherwise. If there was an error,
	an error message will be logged.
	*/
	bool writeToFile(std::string filename, std::string data, bool append) {
		filename = ofToDataPath(filename);
		ofFile out(filename, ofFile::Reference);
		if (out.exists() && !append) {
			CX::Instances::Log.warning("CX::Util::writeToFile") << "File \"" << filename << "\" already exists. It will be overwritten.";
		}
		out.close();
		out.open(filename, (append ? ofFile::Append : ofFile::WriteOnly), false);
		if (out.is_open()) {
			out << data;
			out.close();
			return true;
		} else {
			CX::Instances::Log.error("CX::Util::writeToFile") << "File \"" << filename << "\" could not be opened.";
		}
		return false;
	}

	/*!
	Rounds the given double to the given power of 10.
	\param d The number to be rounded.
	\param roundingPower The power of 10 to round `d` to. For the value 34.56, the results with different rounding powers 
	(and `c = ROUND_TO_NEAREST`) are as follows: `RP = 0 -> 35; RP = 1 -> 30; RP = -1 -> 34.6`.
	\param c The type of rounding to do, from the CX::Util::CX_RoundingConfiguration enum. You can round up, down, to nearest (default), and toward zero.
	\return The rounded value.
	*/
	double round(double d, int roundingPower, CX::Util::CX_RoundingConfiguration c) {
		//When <cfenv> becomes availble, use that instead of this stuff.
		double loc = std::pow(10, roundingPower);
		double modulo = abs(fmod(d, loc));

		if (d >= 0) {
			d -= modulo;
		} else {
			d -= (loc - modulo);
		}

		switch (c) {
		case CX_RoundingConfiguration::ROUND_TO_NEAREST:
			d += (modulo >= (.5 * loc)) ? (loc) : 0;
			break;
		case CX_RoundingConfiguration::ROUND_UP:
			d += (modulo != 0) ? loc : 0;
			break;
		case CX_RoundingConfiguration::ROUND_DOWN:
			break;
		case CX_RoundingConfiguration::ROUND_TOWARD_ZERO:
			if (d < 0) {
				d += (modulo != 0) ? loc : 0;
			}
			break;
		}

		return d;
	}


	/*! This function reads in a file containing information stored as key-value pairs. A file of this kind could look like:
	\code
	Key=Value
	blue = 0000FF
	unleash_penguins=true
	\endcode
	This type of file is often used for configuration of a program. This function simply provides a simple way to read in such data.
	\param filename The name of the file containing key-value data.
	\param delimiter The string that separates the key from the value. In the example, it is "=".
	\param trimWhitespace If `true`, whitespace characters surrounding both the key and value will be removed. If this is `false`, in the example, one of the
	key-value pairs would be ("blue ", " 0000FF"). Generally, you would want to trim.
	\param commentString If commentString is not the empty string (i.e. ""), everything on a line following the first instance of commentString will be ignored.
	\return A map<string, string>, where each key string accesses a value string.
	*/
	std::map<std::string, std::string> readKeyValueFile(std::string filename, std::string delimiter, bool trimWhitespace, std::string commentString) {
		std::map<std::string, std::string> rval;

		if (!ofFile::doesFileExist(filename, true)) {
			CX::Instances::Log.error() << "File \"" << filename << "\" not found when attempting to read with CX::Util::readKeyValueFile().";
			return rval;
		}

		ofBuffer buf = ofBufferFromFile(filename, false);
		while (!buf.isLastLine()) {
			std::string line = buf.getNextLine();

			//Strip comments. Only // is supported for comments, not /* */.
			if (commentString != "") {
				std::string::size_type commentStart = line.find(commentString, 0);
				if (commentStart != std::string::npos) {
					line = line.erase(commentStart);
				}
			}

			std::vector<std::string> parts = ofSplitString(line, delimiter, false, trimWhitespace);
			if (parts.size() >= 2) {
				rval[parts[0]] = parts[1];
			}
		}

		return rval;
	}


	/*! Returns the angle in degrees "between" p1 and p2. If you take the difference between p2 and p1,
	you get a resulting vector, V, that gives the displacement from p1 to p2. Imagine that you begin at
	(0, 0) and move to (abs(V.x), 0), creating a line segment. Now if you "rotate" this line segment clockwise,
	like the hand of a clock, until you reach V, the angle rotated through is the value returned by this function.

	This is useful if you want to know, e.g., the angle between the mouse cursor and the center of the screen.

	\param p1 The start point of the vector V.
	\param p2 The end point of V. If p1 and p2 are reversed, the angle will be off by 180 degrees.
	\return The angle in degrees between p1 and p2. */
	float getAngleBetweenPoints(ofPoint p1, ofPoint p2) {
		if (p1 == p2) {
			CX::Instances::Log.error("Util") << "getAngleBetweenPoints: Points are equal.";
			return std::numeric_limits<float>::infinity();
		}

		ofPoint p = p2 - p1;

		float angle = 0;
		if (p.x == 0) {
			if (p.y >= 0) {
				angle = PI / 2;
			} else {
				angle = -PI / 2;
			}
		} else {
			angle = atan(p.y / p.x);
		}

		if (p.x < 0) {
			angle += PI;
		}

		return fmod(360 + angle * 180 / PI, 360);
	}

	/*! This function begins at point `start` and travels `distance` from that point along `angle`, returning the resulting point.

	This is useful for, e.g., drawing an object at a position relative to the center of the screen.

	\param start The starting point.
	\param distance The distance to travel.
	\param angle The angle to travel on, in degrees. */
	ofPoint getRelativePointFromDistanceAndAngle(ofPoint start, float distance, float angle) {
		angle = ofDegToRad(angle);
		start.x += distance * cos(angle);
		start.y += distance * sin(angle);
		return start;
	}

}
}
