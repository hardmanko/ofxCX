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
	was 0.7.1, simply input (0, 7, 1) as the arguments. A warning will be logged if the versions don't match.
	\return True if the versions match, false otherwise. */
	bool checkOFVersion(int versionMajor, int versionMinor, int versionPatch) {
		if (versionMajor == OF_VERSION_MAJOR && versionMinor == OF_VERSION_MINOR && versionPatch == OF_VERSION_PATCH) {
			return true;
		}
		CX::Instances::Log.warning("CX::Util::checkOFVersion") << "openFrameworks version does not match target version. Current oF version: " << ofGetVersionInfo();
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
	\param roundingPower The power of 10 to round d to. For example, if roundingPower is 0, d is rounded to the one's place (10^0 == 1).
	If roundingPower is -3, d is rounded to the thousandth's place (10^-3 = .001). If roundingPower is 1, d is rounded to the ten's place.
	\param c The type of rounding to do, from the CX::Util::CX_RoundingConfiguration enum. You can round up, down, to nearest, and toward zero.
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

	/*! Saves the contents of an ofFbo to a file. The file type is hinted by the file extension you provide
	as part of the file name.
	\param fbo The framebuffer to save.
	\param filename The path of the file to save. The file extension determines the type of file that is saved.
	Many standard file types are supported: png, bmp, jpg, gif, etc. However, if the fbo has an alpha channel,
	only png works properly (at least of those I have tested).
	*/
	void saveFboToFile(ofFbo& fbo, std::string filename) {
		ofPixels pix;
		fbo.readToPixels(pix);
		ofSaveImage(pix, filename, OF_IMAGE_QUALITY_BEST);
	}


	/*! This function reads in a file containing information stored as key-value pairs. A file of this kind could look like:
	\code
	unleash_penguins=true
	Key=Value
	blue=0000FF
	\endcode
	This type of file is often used for configuration of a program. This function simply provides a simple way to read in such data.
	\param filename The name of the file containing key-value data.
	\param delimiter The string that separates the key from the value. In the example, it is "=".
	\param trimWhitespace If true, whitespace characters surrounding both the key and value will be removed.
	\param commentStr If commentStr is not the empty string (i.e. ""), everything on a line following the first instance of commentStr will be ignored.
	\return A map<string, string>, where the keys are the keys to the map.
	*/
	std::map<std::string, std::string> readKeyValueFile(std::string filename, std::string delimiter, bool trimWhitespace, std::string commentStr) {
		std::map<std::string, std::string> rval;

		if (!ofFile::doesFileExist(filename, true)) {
			CX::Instances::Log.error() << "File \"" << filename << "\" not found when attempting to read with CX::Util::readKeyValueFile().";
			return rval;
		}

		ofBuffer buf = ofBufferFromFile(filename, false);
		while (!buf.isLastLine()) {
			std::string line = buf.getNextLine();

			//Strip comments. Only // is supported for comments, not /* */.
			if (commentStr != "") {
				std::string::size_type commentStart = line.find(commentStr, 0);
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

}
}
