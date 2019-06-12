#include "CX_Utilities.h"

#include "ofMain.h"
#include "ofConstants.h"

#include "CX_Logger.h"

namespace CX {
namespace Util {



	/*! Boolean string fuzzy matching. 
	Matches boolean values in common textual (case insensitive) and numeric forms.

	\param s String to evaluate.
	\param log Print log message on errors.
	\return Evaluates to 1 for "true", "t", or "1".
	Evaluates to 0 for "false", "f", or "0".
	Evaluates to -1 otherwise and optionally logs an error.
	 */
	int stringToBooleint(std::string s, bool log) {
		s = ofTrim(s);
		s = ofToLower(s);
		if ((s == "false") || (s == "f") || (s == "0")) {
			return 0;
		}
		else if ((s == "true") || (s == "t") || (s == "1")) {
			return 1;
		}

		if (log) {
			CX::Instances::Log.error("Util") << "stringToBooleint(): Failure converting \"" << s << "\" to booleint. Returning -1";
		}
		return -1;
	}

	/*! Attempts to set the process that CX is running in to high priority.

Note: This function only works on Windows.

\return `false` if a known error is encountered.
*/
	bool setProcessToHighPriority(void) {
#ifdef TARGET_WIN32
		return CX::Util::Windows::setProcessToHighPriority();
#else
		CX::Instances::Log.error() << "setProcessToHighPriority(): CX does not support setting high process priority on your operating system.";
		return false;
#endif
	}

#ifdef TARGET_WIN32
	namespace Windows {
		std::string convertErrorCodeToString(DWORD errorCode) {

			if (errorCode == 0) {
				return "No error.";
			}

			LPSTR messageBuffer = nullptr;
			size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

			std::string message(messageBuffer, size);

			LocalFree(messageBuffer); //Free the buffer.

			return message;
		}

		bool setProcessToHighPriority(void) {
			//See https://msdn.microsoft.com/en-us/library/ms686219%28v=vs.85%29.aspx

			DWORD dwError;
			DWORD dwPriClass;

			HANDLE thisProcess = GetCurrentProcess();

			if (!SetPriorityClass(thisProcess, HIGH_PRIORITY_CLASS)) {
				dwError = GetLastError();
				CX::Instances::Log.error() << "Error setting process priority: " << convertErrorCodeToString(dwError);
				return false;
			}

			dwPriClass = GetPriorityClass(GetCurrentProcess());

			if (dwPriClass != HIGH_PRIORITY_CLASS) {
				CX::Instances::Log.error() << "Failed to set priority to high.";
				return false;
			}
			return true;
		}
	} //namespace Windows
#endif


	/*! Writes data to a file, either appending the data to an existing file or creating a new file, overwriting
	any existing file with the given filename.
	\param filename Name of the file to write to. If it is a relative file name, it will be placed relative
	the the data directory.
	\param data The data to write
	\param append If `true`, data will be appended to an existing file, if it exists. If `false`, any
	existing file will be overwritten and a warning will be logged (if `overwriteWarning` is `true`). 
	If no file exists, a new one will be created.
	\param overwriteWarning If `true`, a warning will be logged if a file will be overwritten.
	\return `true` if an error was encountered while writing the file, `false` otherwise. If there was an error,
	an error message will be logged.
	*/
	bool writeToFile(std::string filename, std::string data, bool append, bool overwriteWarning) {
		filename = ofToDataPath(filename);
		ofFile out(filename, ofFile::Reference);
		if (overwriteWarning && out.exists() && !append) {
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
	(and `rounding = ToNearest`) are as follows: `RP = 0 -> 35; RP = 1 -> 30; RP = -1 -> 34.6`.
	\param rounding The type of rounding to do, from the CX::Util::Rounding enum. You can round up, down, to nearest (default), and toward zero.
	\return The rounded value.
	*/
	double round(double d, int roundingPower, CX::Util::Rounding rounding) {
		//When <cfenv> becomes availble, use that instead of this stuff.
		double loc = std::pow(10, roundingPower);
		double modulo = std::abs(std::fmod(d, loc));

		if (d >= 0) {
			d -= modulo;
		} else {
			d -= (loc - modulo);
		}

		switch (rounding) {
		case Rounding::ToNearest:
			d += (modulo >= (.5 * loc)) ? (loc) : 0;
			break;
		case Rounding::Up:
			d += (modulo != 0) ? loc : 0;
			break;
		case Rounding::Down:
			break;
		case Rounding::TowardZero:
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
#if OF_VERSION_MAJOR == 0 && OF_VERSION_MINOR >= 9 && OF_VERSION_PATCH >= 0
		ofBuffer::Lines lines = buf.getLines();
		for (auto it = lines.begin(); it != lines.end(); it++) {
			std::string line = *it;
#else
		while (!buf.isLastLine()) {
			std::string line = buf.getNextLine();
#endif

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

	/*! Write key-value pairs stored in a map<string, string> to a file. 
	A delimiter is inserted between each key-value pair.
	Each key-value pair in on its own line.

	\param kv A map<string, string> with the keys and values.
	\param filename The file to write to.
	\param delimiter A string to separate keys from values. Defaults to "=".

	If the contents of `kv` are `{ "pigs" : "happy", "cowsMilked" : "no" }` and `delimiter` is "=", the output could be
	\code
	pigs=happy
	cowsMilked=no
	\endcode
	*/
	bool writeKeyValueFile(const std::map<std::string, std::string>& kv, std::string filename, std::string delimiter) {

		std::ostringstream ss;
		for (const std::pair<std::string, std::string>& p : kv) {
			ss << p.first + delimiter + p.second << std::endl;
		}

		return Util::writeToFile(filename, ss.str(), false);
	}

	/*! Performs a word wrapping procedure, splitting `s` into multiple lines so
	that each line is no more than `width` wide. The algorithm attempts to end lines
	at whitespace, so as to avoid splitting up words. However, if there is no whitespace
	on a line, the line will be broken just before it would exceed the width
	and a hyphen is inserted. If the width is absurdly narrow (less than 2 characters),
	the algorithm will break.
	\param s The string to wrap.
	\param width The maxmimum width of each line of `s`, in pixels.
	\param font A configured ofTrueTypeFont.
	\return A string with newlines inserted to keep lines to be less than `width` wide.
	*/
	std::string wordWrap(std::string s, float width, ofTrueTypeFont& font) {
		unsigned int lineStart = 0;
		unsigned int lastWS = 0;
		std::vector<std::string> lines;
		for (unsigned int i = 0; i < s.size(); i++) {

			if (s[i] == ' ' || s[i] == '\t' || s[i] == '\n' || s[i] == '\r') {
				lastWS = i;
			}

			std::string sub = s.substr(lineStart, i - lineStart);
			float currentW = font.getStringBoundingBox(sub, 0, 0).width;

			if (currentW >= width) {

				if (lastWS > lineStart) {
					// Whitespace can be found on this line, so split at the whitespace.
					sub = s.substr(lineStart, lastWS - lineStart + 1);
					lineStart = lastWS + 1; // Skip the WS

				} else {
					//If no whitespace on this line, do the gross thing and split mid-word

					int poppedChars = 0;
					if (sub.length() >= 2) {
						sub.pop_back(); //pop off two letters to 1) make the string shorter and 
						sub.pop_back(); //2) make room for the hyphen.
						poppedChars = 2;
					}

					sub += '-'; // stick in a lame hyphen
					lineStart = i - poppedChars;
				}

				i = lineStart;
				lines.push_back(sub);

			} else if (i == s.size() - 1) {
				// At the end of s, just accept the last line.
				sub = s.substr(lineStart);
				lines.push_back(sub);
			}
		}

		std::string rval = "";
		if (lines.size() > 0) {
			rval = lines.front();
			for (unsigned int i = 1; i < lines.size(); i++) {
				rval += "\n" + lines[i];
			}
		}

		return rval;
	}



	/*! Returns the angle in degrees "between" p1 and p2. If you take the difference between p2 and p1,
	you get a resulting vector, V, that gives the displacement from p1 to p2. Imagine that you create a 
	vector T = [1, 0]. Now if you "rotate" T in the positive direction, like the hand of a clock, until 
	you reach V, the angle rotated through is the value returned by this function.

	This is useful if you want to know, e.g., the angle between the mouse cursor and the center of the screen.

	\param p1 The start point of the vector V.
	\param p2 The end point of V. If p1 and p2 are reversed, the angle will be off by 180 degrees.
	\return The angle in degrees between p1 and p2. The values are in the range [0, 360). */
	float getAngleBetweenPoints(ofPoint p1, ofPoint p2) {
		if (p1 == p2) {
			CX::Instances::Log.error("Util") << "getAngleBetweenPoints(): Points are equal.";
			return std::numeric_limits<float>::infinity();
		}

		ofPoint p = p2 - p1;
		float angle = atan2(p.y, p.x);
		angle = ofRadToDeg(angle);

		return fmod(360 + angle, 360);
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

} //namespace Util
} //namespace CX
