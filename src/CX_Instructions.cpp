#include "CX_Instructions.h"

namespace CX {
namespace Util {

constexpr char* zeroOrMoreSpaces = "\\s*";

std::string stripComments(std::string line, std::string commentStr) {
	size_t off = line.find(commentStr);
	if (off != std::string::npos) {
		line.erase(off);
	}
	return line;
}

std::vector<std::string> splitMultisectionString(std::string str, std::string sectionDlmRegex) {

	std::regex sectionSplitter(sectionDlmRegex);
	std::string dlm = "(#^#] DELIM [#^#)";
	str = std::regex_replace(str, sectionSplitter, dlm);

	return ofSplitString(str, dlm);
}

/*
std::string getSectionType(std::string section, std::string kvDelim) {
	std::vector<std::string> types = { "text", "image" };
	for (std::string type : types) {
		std::regex keyStart("^" + type + "\\s+" + kvDelim + "\\s+");
		if (std::regex_search(section, keyStart)) {
			return type;
		}
	}
	return "unknown";
}
*/

std::pair<std::string, std::string> lineKeyValue(std::string line, const std::vector<std::string>& allKeys, const std::string& kvDelim) {

	std::pair<std::string, std::string> rval;

	for (std::string key : allKeys) {
		std::regex keyStart("^" + key + "\\s*" + kvDelim + "\\s*"); // TODO: Back to + or keep *?
		if (std::regex_search(line, keyStart)) {
			rval.first = ofTrim(key);
			std::string value = std::regex_replace(line, keyStart, "");
			rval.second = ofTrim(value);
		}
	}

	return rval;
}


std::string getFilePathRelativeTo(std::string filename, std::string relPathOrFile) {

	if (ofFilePath::isAbsolute(filename)) {
		return ofFilePath::makeRelative(filename, relPathOrFile);
	}

	std::string enclos = ofFilePath::getEnclosingDirectory(relPathOrFile);

	enclos = ofFilePath::addTrailingSlash(enclos);

	return enclos + filename;
}

/*

*/
ofPoint delimitedStringToPoint(const std::string& locationStr, std::string delim) {

	ofPoint rval;

	std::vector<std::string> alignParts = ofSplitString(locationStr, delim, false, true);

	if (alignParts.size() == 3) {
		rval.x = ofFromString<float>(alignParts[0]);
		rval.y = ofFromString<float>(alignParts[1]);
		rval.z = ofFromString<float>(alignParts[2]);
	} else if (alignParts.size() == 2) {
		rval.x = ofFromString<float>(alignParts[0]);
		rval.y = ofFromString<float>(alignParts[1]);
	} else if (alignParts.size() == 1) {
		rval.x = ofFromString<float>(alignParts[0]);
		rval.y = rval.x;
	}

	return rval;
}

ofPoint alignmentStringToPoint(const std::string& align, std::string delim) {

	std::vector<std::string> allAlignStr = { "center", "left", "right", "top", "bottom", "topleft", "bottomleft", "topright", "bottomright" };

	ofPoint rval(0, 0);

	if (CX::Util::contains(allAlignStr, align)) {

		if (align == "center") {
			rval = ofPoint(0, 0);
		}

		if (align.find("left") != std::string::npos) {
			rval.x = -1;
		} else if (align.find("right") != std::string::npos) {
			rval.x = 1;
		}

		if (align.find("top") != std::string::npos) {
			rval.y = -1;
		} else if (align.find("bottom") != std::string::npos) {
			rval.y = 1;
		}

	} else {
		rval = delimitedStringToPoint(align, delim);
	}
	
	rval.x = Util::clamp<float>(rval.x, -1, 1);
	rval.y = Util::clamp<float>(rval.y, -1, 1);

	return rval;

}

// Both x and y of align are in [-1, 1].
ofPoint alignRectangle(ofPoint align, ofPoint p, float w, float h) {

	float xOff = (1 + align.x) * w / 2;
	float yOff = (1 + align.y) * h / 2;

	p.x -= xOff;
	p.y -= yOff;

	return p;
}

ofRectangle alignRectangle(ofPoint align, ofRectangle r) {

	ofPoint newPos = alignRectangle(align, r.getPosition(), r.width, r.height);
	r.setPosition(newPos);
	return r;
}

// Reads a string containing a char in single quotes, e.g. 'R', and converts it to an int.
// If it is an int, it reads that int.
int readCharStr(std::string chStr) {

	chStr = ofTrim(chStr);

	std::regex singleQuotes("'(.)'");
	std::regex signedDigits("-?\\d+");

	std::smatch match;
	int rval = -1;
	if (std::regex_match(chStr, match, singleQuotes)) {

		std::string matchChar = match.str(1);
		rval = matchChar[0];

	} else if (std::regex_match(chStr, match, signedDigits)) {

		rval = ofFromString<int>(chStr);
	}

	return rval;
}

//////////////////////////
// InstructionPresenter //
//////////////////////////

/*! Sets up the InstructionPresenter with the given configuration settings.
\param config Configuration for the InstructionPresenter.
\return `true` in all cases.
*/
bool InstructionPresenter::setup(const Configuration& config) {
	_config = config;

	_inputSetup();
	_soundSetup();

	return true;
}

/*! Gets a constant reference to the configuration used by the InstructionPresenter.
To modify the configuration, copy the configuration with this function, modify the 
copy, and then call `setup()` with the copy.
\return A reference to the configuration.
*/
const InstructionPresenter::Configuration& InstructionPresenter::getConfiguration(void) {
	return _config;
}

/*! Sets up the InstructionPresenter from a configuration file.
\param configFile The file to read configuration settings from.
\param disp A pointer to a CX_Display to display instruction screens on.
\param parseSet Settings for configuration file and instruction file parsing.
\return `true` if setup from the file was successful, `false` otherwise.
*/
bool InstructionPresenter::setup(std::string configFile, CX_Display* disp, ParseSettings parseSet) {

	_config = Configuration();

	if (disp == nullptr) {
		_config.display.disp = &CX::Instances::Disp;
	} else {
		_config.display.disp = disp;
	}	

	if (!ofFile::doesFileExist(configFile)) {
		CX::Instances::Log.error("InstructionPresenter") << "In setup(): Configuration file \"" << configFile << "\" not found.";
		return false;
	}

	_config.parse = parseSet;

	
	std::map<std::string, std::string> kv = Util::readKeyValueFile(configFile);


	// Misc section
	if (kv.find("display.backgroundColor") != kv.end()) {
		_config.display.backgroundColor = Util::rgbStringToColor<ofColor>(kv.at("display.backgroundColor"), ",");
	} else {
		_config.display.backgroundColor = ofColor(0);
	}
	

	// Reload section
	{
		if (kv.find("reload.key") != kv.end()) {
			_config.reload.key = readCharStr(kv.at("reload.key"));
		}

		if (kv.find("reload.onChange") != kv.end()) {
			_config.reload.onChange = Util::stringToBooleint(kv.at("reload.onChange")) == 1;
		}
	}

	// Input section
	{
		if (kv.find("input.delay") != kv.end()) {
			_config.input.acceptanceDelay = CX_Millis(ofFromString<double>(kv.at("input.delay")));
		}

		// Keyboard
		if (kv.find("keyboard.enabled") != kv.end()) {
			_config.input.keyboard.enabled = Util::stringToBooleint(kv.at("keyboard.enabled")) == 1;
		}

		if (kv.find("keyboard.backKey") != kv.end()) {
			_config.input.keyboard.backKey = readCharStr(kv.at("keyboard.backKey"));
		}

		if (_config.input.keyboard.enabled && kv.find("keyboard.forwardKeys") != kv.end()) {
			_config.input.keyboard.forwardKeys.clear();
			std::vector<std::string> keys = ofSplitString(kv.at("keyboard.forwardKeys"), ",", true, true);
			
			for (std::string key : keys) {
				int ch = readCharStr(key);
				if (ch >= 0) {
					_config.input.keyboard.forwardKeys.push_back(ch);
				}
			}

			// If any of the inputs is "any key", then clear the list, which is interpreted as anything being accepted.
			if (Util::contains(_config.input.keyboard.forwardKeys, -1)) {
				_config.input.keyboard.forwardKeys.clear();
			}
		}

		// Mouse
		if (kv.find("mouse.enabled") != kv.end()) {
			_config.input.mouse.enabled = Util::stringToBooleint(kv.at("mouse.enabled")) == 1;
		}

		if (_config.input.mouse.enabled && kv.find("mouse.buttons") != kv.end()) {
			_config.input.mouse.buttons.clear();
			std::vector<std::string> buttons = ofSplitString(kv.at("mouse.buttons"), ",", true, true);
			std::regex digits("-?\\d+");
			for (std::string button : buttons) {
				if (std::regex_match(button, digits)) {
					_config.input.mouse.buttons.push_back(ofFromString<int>(button));
				}
			}

			// If any of the inputs is "any key", then clear the list, which is interpreted as anything being accepted.
			if (Util::contains(_config.input.mouse.buttons, -1)) {
				_config.input.mouse.buttons.clear();
			}
		}

	}

	// Prompt section
	if (kv.find("prompt.text") != kv.end()) {
		_config.prompt.text = kv.at("prompt.text");
	}
		
	if (kv.find("prompt.location") != kv.end()) {
		_config.prompt.location = delimitedStringToPoint(kv.at("prompt.location"));
	}
		
	if (kv.find("prompt.alignment") != kv.end()) {
		_config.prompt.alignment = alignmentStringToPoint(kv.at("prompt.alignment"));
	}
		
	if (kv.find("prompt.font") != kv.end()) {
		_config.prompt.font = kv.at("prompt.font");
	}
		
	if (kv.find("prompt.size") != kv.end()) {
		_config.prompt.size = ofFromString<int>(kv.at("prompt.size"));
	}
		
	if (kv.find("prompt.color") != kv.end()) {
		_config.prompt.color = Util::rgbStringToColor<ofColor>(kv.at("prompt.color"), ",");
	}
		
	//if (kv.find("prompt.delay") != kv.end()) {
	//	_config.prompt.presentationDelay = CX_Millis(ofFromString<double>(kv.at("prompt.delay")));
	//}

	if (kv.find("prompt.matchInputDelay") != kv.end()) {
		_config.prompt.matchInputDelay = Util::stringToBooleint(kv.at("prompt.matchInputDelay")) == 1;
	}

	// Sound section
	_config.sound.setFromFile(configFile, _config.parse.kvDlm, true, _config.parse.commentStr, "sound.");


	// Set instruction defaults
	std::string defaultsFilename = "";
	if (kv.find("instructionDefaultsFile") != kv.end()) {
		defaultsFilename = kv.at("instructionDefaultsFile");
	}
	if (defaultsFilename != "") {
		defaultsFilename = getFilePathRelativeTo(defaultsFilename, configFile);
		_setInstructionDefaults(defaultsFilename);
	}


	// Final setup of input and sound
	_inputSetup();
	_soundSetup();


	// Present any test instructions
	if (kv.find("testInstructions") != kv.end()) {
		std::string instructions = kv.at("testInstructions");
		std::vector<std::string> iFiles = ofSplitString(instructions, _config.parse.listDlm, true, true);
		this->present(iFiles);
	}

	return true;
}

void InstructionPresenter::_inputSetup(void) {

	ContinueInput& cont = _config.input;

	if (!cont.keyboard.enabled && !cont.mouse.enabled) {
		CX::Instances::Log.notice("InstructionPresenter") << "Neither keyboard nor mouse were enabled to continue past instruction screens. Both have been enabled.";
		cont.keyboard.enabled = true;
		cont.mouse.enabled = true;
	}

	// Only enable if asked for, don't disable if not asked for.
	if (cont.keyboard.enabled) {
		CX::Instances::Input.Keyboard.enable(true);
	}
	if (cont.mouse.enabled) {
		CX::Instances::Input.Mouse.enable(true);
	}

	if (_config.prompt.text == ContinuePrompt().text) {
		std::string deviceString = "";
		if (cont.keyboard.enabled && cont.mouse.enabled) {
			deviceString = "keyboard or mouse";
		} else if (cont.keyboard.enabled) {
			deviceString = "keyboard";
		} else if (cont.mouse.enabled) {
			deviceString = "mouse";
		}
		_config.prompt.text = "Use the " + deviceString + " to continue.";
		if (cont.keyboard.enabled && !cont.mouse.enabled && cont.keyboard.forwardKeys.size() == 0) {
			_config.prompt.text = "Press any key to continue.";
		}
	}
}

void InstructionPresenter::_soundSetup(void) {

	_soundStream = std::make_shared<CX_SoundStream>();

	_soundStream->setup(_config.sound);

	_config.sound = _soundStream->getConfiguration();

}

bool InstructionPresenter::_setInstructionDefaults(std::string inputFile) {

	if (!ofFile::doesFileExist(inputFile)) {
		CX::Instances::Log.error("InstructionPresenter") << "While setting instruction defaults, the file \"" << inputFile << "\" was not found.";
		return false;
	}

	std::vector<std::shared_ptr<InstructionData>> items = _parseInstructionFile(inputFile, true);

	for (std::shared_ptr<InstructionData> item : items) {
		if (item->getType() == _config.defaults.text.getType()) {
			std::shared_ptr<TextData> td = std::dynamic_pointer_cast<TextData, InstructionData>(item);
			td->text = "";
			_config.defaults.text = *td;

		} else if (item->getType() == _config.defaults.image.getType()) {
			std::shared_ptr<ImageData> id = std::dynamic_pointer_cast<ImageData, InstructionData>(item);
			id->file = "";
			_config.defaults.image = *id;

		} else if (item->getType() == _config.defaults.sound.getType()) {
			std::shared_ptr<SoundData> sd = std::dynamic_pointer_cast<SoundData, InstructionData>(item);
			sd->file = "";
			_config.defaults.sound = *sd;
		}
	}

	return true;
}

/*! Present a vector of instruction files.
\param instFiles A vector of instruction file names. Instruction files are treated as relative to the data directory.
If empty, nothing happens.
*/
void InstructionPresenter::present(const std::vector<std::string>& instFiles) {
	if (instFiles.empty()) {
		return;
	}

	bool wasAutoSwapping = _config.display.disp->isAutomaticallySwapping();
	if (wasAutoSwapping) {
		_config.display.disp->setAutomaticSwapping(false);
	}

	_presentSection(instFiles);

	if (wasAutoSwapping) {
		_config.display.disp->setAutomaticSwapping(true);
	}
}

/* Present a vector of instructions as strings containing the contents of an instruction file.

Unlike files, you can't easily edit instruction text while it is running.
*/
//bool InstructionPresenter::presentText(const std::vector<std::string>& instText) {}

//bool InstructionPresenter::presentObjects(const std::vector<std::shared_ptr<InstructionData>>& instObj) {}




/*! Present a single instruction file.
\param instFile An instruction file name. Instruction files are treated as relative to the data directory.
*/
void InstructionPresenter::present(std::string instFile) {
	std::vector<std::string> ifs;
	ifs.push_back(instFile);
	present(ifs);
}

struct PrHistory {

	enum class ChangeStatus : int {
		NoChange, // the current file did not change
		ForwardNew, // a new file that was moved forward to
		Backward, // an old file that was moved backward to
		ForwardOld, // an old file that was moved forward to
		Reloaded // the current slide has not changed, but it has been reloaded from disk
	};

	PrHistory(const std::vector<std::string>& files) :
		currentFilenameIndex(0),
		filenames(files),
		maxFilenameIndex(0)
	{}

	std::vector<std::string> filenames;

	int currentFilenameIndex;
	int maxFilenameIndex;

	ChangeStatus goBack(void) {
		if (currentFilenameIndex <= 0) {
			return ChangeStatus::NoChange;
		}

		currentFilenameIndex--;
		return ChangeStatus::Backward;
	}

	ChangeStatus goForward(void) {
		if (currentFilenameIndex >= (filenames.size() - 1)) {
			return ChangeStatus::NoChange;
		}

		currentFilenameIndex++;

		// Check if next slide is new before updating maxFilenameIndex
		bool nextSlideNew = currentFilenameIndex > maxFilenameIndex;
		if (currentFilenameIndex > maxFilenameIndex) {
			maxFilenameIndex = currentFilenameIndex;
		}

		return nextSlideNew ? ChangeStatus::ForwardNew : ChangeStatus::ForwardOld;
	}

	bool back(void);
	bool forward(void);
	std::string currentFilename(void);

};

bool PrHistory::back(void) {
	if (currentFilenameIndex <= 0) {
		return false;
	}

	//status = PresStatus::Old_Back;

	currentFilenameIndex--;
	return true;
}

bool PrHistory::forward(void) {
	if (currentFilenameIndex >= (filenames.size() - 1)) {
		return false;
	}

	bool nextSlideRevisited = currentFilenameIndex < maxFilenameIndex;

	currentFilenameIndex++;

	bool nextSlideNew = currentFilenameIndex > maxFilenameIndex;
	

	if (currentFilenameIndex > maxFilenameIndex) {
		maxFilenameIndex = currentFilenameIndex;
		//status = PresStatus::New;
	//} else {
		//status = PresStatus::Old_Forward;
	}
	return true;
}

std::string PrHistory::currentFilename(void) {
	if (currentFilenameIndex < 0 || currentFilenameIndex >= filenames.size()) {
		return "";
	}
	return filenames[currentFilenameIndex];
}


/*
void InstructionPresenter::_presentSection(const std::vector<std::string>& instFiles) {

	// Copy and modify later
	CX_Millis inputAcceptanceDelay = _config.input.acceptanceDelay;

	PrHistory history(instFiles);

	ActiveInstruction inst;
	inst.setup(this, history.currentFilename());
	inst.start(false);

	bool presentingInstructions = true;
	while (presentingInstructions) {

		//PrHistory::ChangeStatus changeStat = PrHistory::ChangeStatus::NoChange;

		bool instructionIsNew = true;
		bool instructionChanged = false;
		bool restartInstruction = false; // should the presentation be treated as a start or a restart?

		AwaitResult ar = _getAwaitResult();

		if (ar == AwaitResult::ReloadInstruction || inst.fileHasChanged(true)) {
			instructionIsNew = false;
			instructionChanged = true;
			restartInstruction = true;

			inputAcceptanceDelay = CX_Millis(0); //ignore input delay
		}

		if (ar == AwaitResult::PreviousInstruction) {
			instructionChanged = history.back();

			//changeStat = history.goBack();

			instructionIsNew = false;
			restartInstruction = false;

			inputAcceptanceDelay = CX_Millis(0); //ignore intput delay
		}

		if (ar == AwaitResult::NextInstruction) {
			CX_Millis elapsedTime = CX::Instances::Clock.now() - inst.startTime;

			if (elapsedTime > inputAcceptanceDelay) {

				instructionChanged = history.forward();

				if (instructionChanged) {
					inputAcceptanceDelay = _config.input.acceptanceDelay;
				} else {
					// If the instruction did not change, then the current file is the last file and you are done
					presentingInstructions = false;
				}
				
			}
		}

		if (ar == AwaitResult::SkipInstructions) {
			presentingInstructions = false;
		}

		if (ar == AwaitResult::DoNothing) {
			//do nothing
		}

		
		if (instructionChanged) {

			//bool newInstruction = history.maxFilenameIndex == history.currentFilenameIndex;

			inst.setup(this, history.currentFilename());
			inst.start(restartInstruction);
		}
		

		inst.update();

		CX::Instances::Clock.sleep(this->_config.sleepDuration);

	}
}
*/

void InstructionPresenter::_presentSection(const std::vector<std::string>& instFiles) {

	// Copy and modify later
	//CX_Millis inputAcceptanceDelay = _config.input.acceptanceDelay;
	CX_Millis promptDrawDelay = _config.prompt.matchInputDelay ? _config.input.acceptanceDelay : CX_Millis(0);

	PrHistory history(instFiles);

	ActiveInstruction inst;
	inst.setup(this);

	// Set the first instruction and start it
	inst.setInstruction(history.currentFilename());
	inst.startInstruction(true);

	bool presentingInstructions = true;
	while (presentingInstructions) {


		AwaitResult ar = _getAwaitResult();

		if (ar == AwaitResult::SkipInstructions) {
			presentingInstructions = false;
		} else if (ar == AwaitResult::DoNothing) {
			//do nothing
		}


		CX_Millis timeSinceStart = CX::Instances::Clock.now() - inst.startTime;

		bool instructionChangeAllowed = timeSinceStart > _config.input.acceptanceDelay;

		// unless the instruction changes, this is correct.
		bool shouldDrawPrompt = timeSinceStart >= promptDrawDelay;

		if (ar == AwaitResult::ReloadInstruction || inst.fileHasChanged(true)) {

			inst.startInstruction(history.currentFilename(), false);
		}

		if (instructionChangeAllowed) {

			if (ar == AwaitResult::PreviousInstruction) {

				PrHistory::ChangeStatus cs = history.goBack();

				if (cs == PrHistory::ChangeStatus::Backward) {
					shouldDrawPrompt = true;
					inst.startInstruction(history.currentFilename(), false);
				} // else you stay on the first instruction

			}

			if (ar == AwaitResult::NextInstruction) {

				PrHistory::ChangeStatus cs = history.goForward();
				
				if (cs == PrHistory::ChangeStatus::NoChange) {
					// If you tried to go forward and could not, you are done with instruction screens.
					presentingInstructions = false;
				} else if (cs == PrHistory::ChangeStatus::ForwardNew) {
					shouldDrawPrompt = promptDrawDelay == CX_Millis(0);
					inst.startInstruction(history.currentFilename(), true);
				} else if (cs == PrHistory::ChangeStatus::ForwardOld) {
					shouldDrawPrompt = true;
					inst.startInstruction(history.currentFilename(), false);
				}

			}
		}


		if (presentingInstructions) {

			this->_drawInstructionDisplay(inst.instructions, shouldDrawPrompt);

			CX::Instances::Log.flush();
			CX::Instances::Clock.sleep(this->_config.sleepDuration);
		}

	}

	inst.stopInstruction();
}

/////////////////////////////////////////////
// InstructionPresenter::ActiveInstruction //
/////////////////////////////////////////////

bool InstructionPresenter::ActiveInstruction::setup(InstructionPresenter* ip, const std::string& filename_) {

	if (!ip) {
		return false;
	}

	this->_ip = ip;

	if (filename_ != "") {
		return this->setInstruction(filename_);
	}

	return true;
}


bool InstructionPresenter::ActiveInstruction::setInstruction(const std::string& filename_) {

	bool success = true;

	this->filename = filename_;
	this->dataPathFilename = ofToDataPath(filename_);
	this->instructions = _ip->_loadInstructionFile(this->dataPathFilename);

	// If there is nothing in the file, provide a message.
	if (this->instructions.size() == 0) {
		std::shared_ptr<TextData> ti = std::make_shared<TextData>(_ip->getConfiguration().defaults.text);
		ti->text = "Failed to load instruction file \"" + this->filename + "\".";
		ti->location = ofPoint(0.05, 0.05);
		ti->alignment = ofPoint(-1, -1);
		instructions.push_back(ti);

		CX::Instances::Log.error("InstructionPresenter") << ti->text;

		success = false;
	}

	//this->startTime = CX_Millis(0); // why???

	// Set up file tracking
#if OF_VERSION_MAJOR == 0 && OF_VERSION_MINOR < 10
	_pocoFile = Poco::File(this->dataPathFilename);
	_fileLastModified = _pocoFile.getLastModified();
#else
	_fileLastModified = std::filesystem::last_write_time(this->dataPathFilename);
	//_fileLastModified = boost::filesystem::last_write_time(filename);
#endif

	return success;
}



bool InstructionPresenter::ActiveInstruction::startInstruction(bool firstStart) {

	if (firstStart) {
		this->startTime = CX::Instances::Clock.now();
	}

	/*
	const InstructionPresenter::Configuration& cfg = _ip->getConfiguration();
	bool promptHasDelay = cfg.prompt.matchInputDelay && cfg.input.acceptanceDelay > CX_Millis(0);
	bool drawPrompt = !firstStart || !promptHasDelay;
	_ip->_drawInstructionDisplay(instructions, drawPrompt);
	*/

	for (size_t i = 0; i < instructions.size(); i++) {
		instructions[i]->instructionStarted();
	}

	//hasStarted = true;
	return true;
}

bool InstructionPresenter::ActiveInstruction::startInstruction(const std::string& filename, bool firstStart) {

	stopInstruction();

	bool setupSuccess = setInstruction(filename);
	// continue even if error
	return startInstruction(firstStart) && setupSuccess;
}

void InstructionPresenter::ActiveInstruction::stopInstruction(void) {
	for (size_t i = 0; i < instructions.size(); i++) {
		instructions[i]->instructionStopped();
	}
}

bool InstructionPresenter::ActiveInstruction::fileHasChanged(bool update) {
#if OF_VERSION_MAJOR == 0 && OF_VERSION_MINOR < 10
	Poco::Timestamp mod = _pocoFile.getLastModified();
#else
	time_t mod = std::filesystem::last_write_time(this->dataPathFilename);
#endif

	if (mod > _fileLastModified) {
		if (update) {
			_fileLastModified = mod;
		}
		return true;
	}
	return false;
}


/*
bool InstructionPresenter::ActiveInstruction::start(bool restart, bool drawContinuePromptImmediately) {

	if (!restart) {
		this->startTime = CX::Instances::Clock.now();
	}

	const InstructionPresenter::Configuration& cfg = _ip->getConfiguration();

	bool promptHasDelay = cfg.prompt.matchInputDelay && cfg.input.acceptanceDelay > CX_Millis(0);

	//bool drawPrompt = restart || drawContinuePromptImmediately; // || !promptHasDelay;
	bool drawPrompt = restart || !promptHasDelay;

	//_promptDrawn = restart || (_ip->getConfiguration().prompt.presentationDelay == CX_Millis(0));
	
	_ip->_drawInstructionDisplay(instructions, drawPrompt);
	_promptDrawn = drawPrompt;
	
	for (size_t i = 0; i < instructions.size(); i++) {
		instructions[i]->instructionStarted();
	}

	//hasStarted = true;
}


void InstructionPresenter::ActiveInstruction::update(void) {

	CX_Millis elapsedTime = CX::Instances::Clock.now() - startTime;

	const InstructionPresenter::Configuration& cfg = _ip->getConfiguration();
	CX_Millis promptDelay = cfg.prompt.matchInputDelay ? cfg.input.acceptanceDelay : CX_Millis(0);
	bool drawPrompt = elapsedTime >= promptDelay && !_promptDrawn;

	if (drawPrompt) {
		_ip->_drawInstructionDisplay(this->instructions, true);
		_promptDrawn = true;
	}

}
*/




void InstructionPresenter::_drawInstructionDisplay(const std::vector<std::shared_ptr<InstructionData>>& instructions, bool drawContinue) {
	_config.display.disp->beginDrawingToBackBuffer();
	ofBackground(_config.display.backgroundColor);

	for (unsigned int i = 0; i < instructions.size(); i++) {
		instructions[i]->draw(_config.display.disp);
	}

	if (drawContinue) {
		_drawContinueText();
	}

	_config.display.disp->endDrawingToBackBuffer();
	_config.display.disp->swapBuffers();
}


// Private struct
struct InstructionPresenter::ParsedSection {
	ParsedSection(void) :
		sectionType("unknown"),
		parseSuccess(false)
	{}

	std::string sectionType;
	bool parseSuccess;

	std::shared_ptr<InstructionData> instruction;
};

// Parses, then loads, instructions
std::vector<std::shared_ptr<InstructionData>> InstructionPresenter::_loadInstructionFile(std::string filename, bool keepOnlyParsed, bool keepOnlyLoaded) const {
	std::vector<std::shared_ptr<InstructionData>> instructions = _parseInstructionFile(filename, keepOnlyParsed);
	std::vector<std::shared_ptr<InstructionData>> rval;
	for (std::shared_ptr<InstructionData> inst : instructions) {
		bool loaded = inst->load();
		if (loaded || !keepOnlyLoaded) {
			rval.push_back(inst);
		}
	}
	return rval;
}

// This function does not load() the instructions. See _loadInstructionFile
std::vector<std::shared_ptr<InstructionData>> InstructionPresenter::_parseInstructionFile(std::string filename, bool keepOnlyParsed) const {

	if (!ofFile::doesFileExist(filename)) {
		return {};
	}

	InputInformation inputInfo;
	inputInfo.filename = filename;

	ofBuffer buf = ofBufferFromFile(filename, false);
	
	return _parseInstructionText(buf.getText(), inputInfo, keepOnlyParsed);
}

std::vector<std::shared_ptr<InstructionData>> InstructionPresenter::_parseInstructionText(const std::string& instructionText, const InputInformation& inputInfo, bool keepOnlyParsed) const {

	std::vector<std::string> sections = splitMultisectionString(instructionText, _config.parse.sectionDlmRegex);

	std::vector<std::shared_ptr<InstructionData>> rval;

	for (std::string section : sections) {
		ParsedSection ps = _parseInstructionSection(section, inputInfo);

		// Ignore on nullptr, unknown section type, or on parse failure, but only if keepOnlyParsed
		bool ignore = (ps.instruction == nullptr) ||
			(ps.sectionType == "unknown") ||
			(keepOnlyParsed && !ps.parseSuccess);

		if (!ignore) {
			rval.push_back(ps.instruction);
		}
	}

	return rval;

}

InstructionPresenter::ParsedSection InstructionPresenter::_parseInstructionSection(const std::string& sectionText, const InputInformation& inputInfo) const {

	ParsedSection ps;

	ps.sectionType = "unknown";
	std::vector<std::string> allowedTypes = { "text", "image", "sound" };
	for (std::string type : allowedTypes) {
		std::regex keyStart("^" + type + "\\s*" + _config.parse.kvDlm + "\\s*"); // TODO: + is 1 or more, * is 0 or more
		if (std::regex_search(sectionText, keyStart)) {
			ps.sectionType = type;
		}
	}

	if (ps.sectionType == "unknown") {
		ps.parseSuccess = false;
	} else {
		if (ps.sectionType == "text") {
			ps.instruction = std::make_shared<TextData>(_config.defaults.text);
		} else if (ps.sectionType == "image") {
			ps.instruction = std::make_shared<ImageData>(_config.defaults.image);
		} else if (ps.sectionType == "sound") {
			ps.instruction = std::make_shared<SoundData>(_config.defaults.sound);
		}

		InstructionSetup is;
		is.disp = this->_config.display.disp;
		is.soundStream = this->_soundStream;
		ps.instruction->setup(is);

		ps.parseSuccess = ps.instruction->parse(sectionText, _config.parse, inputInfo);
	}

	return ps;
}



InstructionPresenter::AwaitResult InstructionPresenter::_getAwaitResult(void) {

	if (!CX::Instances::Input.pollEvents()) {
		return AwaitResult::DoNothing;
	}

	auto enabledAndMatch = [](int k, int eventKey) -> bool {
		return (k >= 0) && (k == eventKey);
	};

	if (_config.input.keyboard.enabled) {
		while (CX::Instances::Input.Keyboard.availableEvents()) {
			CX_Keyboard::Event kev = CX::Instances::Input.Keyboard.getNextEvent();

			if (kev.type != CX_Keyboard::EventType::Pressed) {
				continue;
			}

			// Any key if empty
			bool isForwardKey = _config.input.keyboard.forwardKeys.empty() ||
				Util::contains(_config.input.keyboard.forwardKeys, kev.key);

			bool isReloadKey = enabledAndMatch(_config.reload.key, kev.key);

			bool isBackKey = enabledAndMatch(_config.input.keyboard.backKey, kev.key);

			bool isSkipKey = enabledAndMatch(_config.input.keyboard.skipKey, kev.key);
			
			if (isReloadKey) {
				return AwaitResult::ReloadInstruction;
			} else if (isBackKey) {
				return AwaitResult::PreviousInstruction;
			} else if (isForwardKey) {
				return AwaitResult::NextInstruction;
			} else if (isSkipKey) {
				return AwaitResult::SkipInstructions;
			}

		}
	}

	if (_config.input.mouse.enabled) {
		while (CX::Instances::Input.Mouse.availableEvents()) {
			CX_Mouse::Event mev = CX::Instances::Input.Mouse.getNextEvent();

			if (mev.type != CX_Mouse::EventType::Pressed) {
				continue;
			}

			bool isContinueButton = _config.input.mouse.buttons.empty() ||
				Util::contains(_config.input.mouse.buttons, mev.button);

			if (isContinueButton) {
				return AwaitResult::NextInstruction;
			}
		}
	}

	return AwaitResult::DoNothing;
}

void InstructionPresenter::_drawContinueText(void) {

	TextData td;
	td.text = _config.prompt.text;

	td.location = _config.prompt.location;
	td.alignment = _config.prompt.alignment;

	td.font = _config.prompt.font;
	td.size = _config.prompt.size;
	td.color = _config.prompt.color;

	if (td.load()) {
		td.draw(_config.display.disp);
	}
}



//////////////
// TextData //
//////////////

bool TextData::parse(std::string section, const ParseSettings& parseSettings, const InputInformation& inputInfo) {

	std::vector<std::string> allKeys = { "text", "wrap", "font", "size", "color", "location", "alignment", "align" };

	std::vector<std::string> lines = ofSplitString(section, "\n", false, false);

	// Find text subsection lines and read in other values
	bool textKeyFound = false;
	std::string lastKey = "";
	std::vector<std::string> textLines;

	for (unsigned int i = 0; i < lines.size(); i++) {
		std::string noComments = stripComments(lines[i], parseSettings.commentStr);

		std::pair<std::string, std::string> kv = lineKeyValue(noComments, allKeys, parseSettings.kvDlm);

		if (kv.first == "") {
			if (lastKey == "text") {
				textLines.push_back(noComments);
			}
		} else {
			if (kv.first == "text") {
				std::regex textStart("^text\\s*=\\s?"); // TODO: \\s+ or *
				std::string textStripped = std::regex_replace(noComments, textStart, "");
				textLines.push_back(textStripped);
				textKeyFound = true;

			} else if (kv.first == "wrap") {
				this->wrapProportion = Util::clamp<float>(ofFromString<float>(kv.second), 0, 1);

			} else if (kv.first == "font") {
				this->font = kv.second;

			} else if (kv.first == "size") {
				this->size = std::max<int>(ofFromString<int>(kv.second), 1);

			} else if (kv.first == "color") {
				this->color = Util::rgbStringToColor<ofColor>(kv.second, ",");

			} else if (kv.first == "location") {
				this->location = delimitedStringToPoint(kv.second, ",");

			} else if (kv.first == "alignment" || kv.first == "align") {
				this->alignment = alignmentStringToPoint(kv.second, ",");
			}

			lastKey = kv.first;
		}
	}

	//Convert text lines into a single string
	this->text = _combineTextLines(textLines);

	return textKeyFound;
}

bool TextData::load(void) {
	return _ttf.load(this->font, this->size);
}

std::string TextData::_combineTextLines(const std::vector<std::string>& lines) {

	// Find the last line that is not empty
	unsigned int lastNonEmpty = 0;
	std::regex whitespace("\\s*");
	for (unsigned int i = 0; i < lines.size(); i++) {
		bool lineEmpty = std::regex_match(lines[i], whitespace); // Match only if all is whitespace
		if (!lineEmpty) {
			lastNonEmpty = i;
		}
	}

	unsigned int newSize = lastNonEmpty + 1;

	std::string instText = "";
	for (unsigned int i = 0; i < newSize; i++) {
		instText += lines[i];
		if (i < lines.size() - 1) {
			instText += "\n";
		}
	}

	return instText;
}

void TextData::draw(CX_Display* disp) {

	if (!_ttf.isLoaded()) {
		_ttf.load(this->font, this->size);
	}

	ofRectangle res = disp->getResolution();

	std::string wrappedText = Util::wordWrap(this->text, res.width * this->wrapProportion, _ttf);

	ofRectangle bb = _ttf.getStringBoundingBox(wrappedText, 0, 0);

	ofPoint locationInPixels(location.x * res.width, location.y * res.height);

	ofPoint textLoc = alignRectangle(this->alignment, locationInPixels, bb.width, bb.height);

	// The y location is the bottom of the first line of _contPrompt, so move the _contPrompt down one line
	ofRectangle aRect = _ttf.getStringBoundingBox("A", 0, 0);
	textLoc.y += aRect.height;

	ofSetColor(this->color);
	_ttf.drawString(wrappedText, textLoc.x, textLoc.y);
}

bool ImageData::parse(std::string section, const ParseSettings& parseSettings, const InputInformation& inputInfo) {

	std::vector<std::string> allKeys = { "image", "scale", "propWidth", "propHeight", "location", "alignment", "align" };

	std::vector<std::string> lines = ofSplitString(section, "\n", false, false);

	// Find text subsection lines and read in other values
	std::string lastKey = "";
	bool imageKeyFound = false;
	std::set<std::string> foundKeys;
	for (unsigned int i = 0; i < lines.size(); i++) {
		std::string noComments = stripComments(lines[i], parseSettings.commentStr);

		std::pair<std::string, std::string> kv = lineKeyValue(noComments, allKeys, parseSettings.kvDlm);

		if (kv.first != "") {

			foundKeys.insert(kv.first);

			if (kv.first == "image") {
				// Get the image file path relative to the instruction file
				this->file = getFilePathRelativeTo(kv.second, inputInfo.filename);
				imageKeyFound = true;

			} else if (kv.first == "scale") {
				this->scale = std::max<float>(ofFromString<float>(kv.second), 0); // Enforce >= 0

			} else if (kv.first == "propWidth") {
				this->screenProportion.x = Util::clamp<float>(ofFromString<float>(kv.second), 0, 1); // Enforce 0 <= x <= 1

			} else if (kv.first == "propHeight") {
				this->screenProportion.y = Util::clamp<float>(ofFromString<float>(kv.second), 0, 1); // Enforce 0 <= x <= 1

			} else if (kv.first == "location") {
				this->location = Util::delimitedStringToPoint(kv.second, ",");

			} else if (kv.first == "alignment" || kv.first == "align") {
				this->alignment = Util::alignmentStringToPoint(kv.second, ",");
			}
		}

	}

	if (screenProportion.x > 0 || screenProportion.y > 0) {
		scale = 1.0;
	}

	return imageKeyFound;
}

bool ImageData::load(void) {

	_imageLoaded = _image.load(this->file);

	if (_imageLoaded && this->scale != 1.0) {
		_image.resize(_image.getWidth() * this->scale, _image.getHeight() * this->scale);
	}

	return _imageLoaded;

}

void ImageData::draw(CX_Display* disp) {
	if (!_imageLoaded) {
		return;
	}

	ofRectangle res = disp->getResolution();

	ofImage imgCopy = _image;

	if (screenProportion.x > 0 && screenProportion.y > 0) {
		imgCopy.resize(res.x * screenProportion.x, res.y * screenProportion.y);

	} else if (screenProportion.x > 0 && screenProportion.y <= 0) {
		float newX = res.x * screenProportion.x;
		float xRatio = newX / imgCopy.getWidth();
		imgCopy.resize(newX, imgCopy.getHeight() * xRatio);

	} else if (screenProportion.y > 0 && screenProportion.x <= 0) {
		float newY = res.y * screenProportion.y;
		float yRatio = newY / imgCopy.getHeight();
		imgCopy.resize(imgCopy.getWidth() * yRatio, newY);
	}

	ofPoint locationInPixels(location.x * res.width, location.y * res.height);

	ofPoint imgLoc = alignRectangle(alignment, locationInPixels, imgCopy.getWidth(), imgCopy.getHeight());
	ofSetColor(255);
	imgCopy.draw(imgLoc);
}

///////////////
// SoundData //
///////////////

bool SoundData::setup(const InstructionSetup& is) {
	_soundStream = is.soundStream;
	_soundPlayer.setup(_soundStream.get());
	return true;
}

bool SoundData::parse(std::string section, const ParseSettings& parseSettings, const InputInformation& inputInfo) {

	std::vector<std::string> allKeys = { "sound", "gain", "startTime" };

	std::vector<std::string> lines = ofSplitString(section, "\n", false, false);

	// Find text subsection lines and read in other values
	std::string lastKey = "";
	bool soundKeyFound = false;
	std::set<std::string> foundKeys;
	for (unsigned int i = 0; i < lines.size(); i++) {
		std::string noComments = stripComments(lines[i], parseSettings.commentStr);

		std::pair<std::string, std::string> kv = lineKeyValue(noComments, allKeys, parseSettings.kvDlm);

		if (kv.first != "") {

			foundKeys.insert(kv.first);

			if (kv.first == "sound") {
				// Get the image file path relative to the instruction file
				this->file = getFilePathRelativeTo(kv.second, inputInfo.filename);
				soundKeyFound = true;

			} else if (kv.first == "gain") {
				this->gain = ofFromString<float>(kv.second);

			} else if (kv.first == "startTime") {
				
				std::vector<std::string> startStr = ofSplitString(kv.second, parseSettings.listDlm, true, true);
				for (std::string& start : startStr) {
					double millis = ofFromString<double>(start);
					startTimes.push_back(CX_Millis(millis));
				}

			}
		}

	}

	if (startTimes.size() == 0) {
		startTimes.push_back(CX_Millis(0));
	}

	return soundKeyFound;
}

bool SoundData::load(void) {
	CX_SoundBuffer tempBuf;

	if (!tempBuf.loadFile(file)) {
		return false;
	}

	tempBuf.applyGain(gain);

	_soundBuffer.clear();
	for (CX_Millis& start : startTimes) {
		_soundBuffer.addSound(tempBuf, start);
	}

	return _soundPlayer.setSoundBuffer(&_soundBuffer);
}

void SoundData::draw(CX_Display* disp) {
	// Nothing to draw
	return;
}

void SoundData::instructionStarted(void) {
	_soundPlayer.play();
}

void SoundData::instructionStopped(void) {
	_soundPlayer.stop();
}


} // namespace Instruct
} // namespace CX




