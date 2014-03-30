#include "CX_Utilities.h"

#include "ofMain.h"
#include "ofConstants.h"

using namespace std;
using namespace CX;

static unsigned int multisamplingSampleCount = 8;

/*! This function retrieves the MSAA (http://en.wikipedia.org/wiki/Multisample_anti-aliasing)
sample count. The sample count can be set by calling CX::relaunchWindow() with the desired sample
count set in the argument to relaunchWindow(). */
unsigned int CX::Util::getSampleCount(void) {
	return multisamplingSampleCount;
};

//This should not be used in user code to set the MSAA sample count because if this is set
//after the window is opened, it only affects FBOs and not the primary buffers (e.g. GL_BACK, 
//GL_FRONT). Use CX::relaunchWindow() to set the MSAA sample count.
void CX::Private::setSampleCount(unsigned int count) {
	multisamplingSampleCount = count;
}

/*! Checks that the version of oF that is used during compilation matches the requested version. If the desired version
was 0.7.1, simply input (0, 7, 1) as the arguments. A warning will be logged if the versions don't match.
\return True if the versions match, false otherwise. */
bool CX::Util::checkOFVersion(int versionMajor, int versionMinor, int versionPatch) {
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
bool CX::Util::writeToFile(std::string filename, std::string data, bool append) {
	ofFile out( ofToDataPath(filename), ofFile::Reference );
	if (out.exists() && !append) {
		CX::Instances::Log.warning("CX::Util::writeToFile") << "File " << filename << " already exists. It will be overwritten.";
	}
	out.close();
	out.open( ofToDataPath(filename), (append ? ofFile::Append : ofFile::WriteOnly), false );
	if (out.is_open()) {
		out << data;
		out.close();
		return true;
	} else {
		CX::Instances::Log.error("CX::Util::writeToFile") << "File " << filename << " could not be opened.";
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
double CX::Util::round(double d, int roundingPower, CX::Util::CX_RoundingConfiguration c) {
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
void CX::Util::saveFboToFile(ofFbo& fbo, std::string filename) {
	ofPixels pix;
	fbo.readToPixels(pix);
	ofSaveImage(pix, filename, OF_IMAGE_QUALITY_BEST);
}



/*! This class is used very much like ofLog. You use the constructor without assigning the
result of construction to a variable and then stream data into the temporary. For example:

writeToFile("fileName.txt") << "A string. Some numbers: " << 50 << 25 << 12.5 << endl << "The next line";

The data that is entered is printed verbatim without any padding, assumptions about endlines, etc.

This class borrows a subset of the functionality of ofLog using some of the same ideas.
class writeToFile {
public:
writeToFile(std::string filename, bool append = true) :
_filename(filename),
_append(append)
{}


writeToFile(std::string filename, std::string data, bool append = true) :
_filename(filename),
_append(append)
{
_message << data;
}


~writeToFile(void) {
//CX::Util::writeToFile(_filename, _message.str(), true);

ofFile out(ofToDataPath(_filename), ofFile::Reference);
if (out.exists() && !_append) {
Instances::Log.warning("CX::writeToFile") << "File " << _filename << " already exists. I will be overwritten.";
}
out.close();
out.open(ofToDataPath(_filename), (_append ? ofFile::Append : ofFile::WriteOnly), false);
if (out.is_open()) {
out << _message;
out.close();
//return true;
} else {
Instances::Log.error("CX::writeToFile") << "File " << _filename << " could not be opened.";
}
//return false;


}

template <typename T>
writeToFile& operator<< (const T& data) {
_message << data;
return *this;
}

writeToFile& operator<< (std::ostream& (*func)(std::ostream&)) {
func(_message);
return *this;
}

private:
std::string _filename;
std::ostringstream _message;
bool _append;
};
*/