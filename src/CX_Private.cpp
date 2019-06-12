#include "CX_Private.h"

#include "CX_Logger.h"
//#include "CX_EntryPoint.h"

namespace CX {
namespace Private {


/* Returns 0 if the string evaluates to false, 1 if the string evaluates to true.
If the string evaluates to neither, this returns -1 and logs an error. */
int stringToBooleint(std::string s) {
	s = ofTrim(s);
	s = ofToLower(s);
	if ((s == "false") || (s.front() == '0')) {
		return 0;
	} else if ((s == "true") || (s.front() == '1')) {
		return 1;
	}

	CX::Instances::Log.error("Private") << "stringToBooleint: Failure attempting to convert "
		"string to boolean: invalid boolean value given: \"" << s << "\". Use \"0\", \"1\", \"true\", or \"false\" (capitalization is ignored).";
	return -1;
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

} //namespace Private
} //namespace CX
