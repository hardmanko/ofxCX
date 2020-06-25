#pragma once

#include <regex>

#include "ofPoint.h"
#include "ofGraphics.h"

#if OF_VERSION_MAJOR == 0 && OF_VERSION_MINOR < 10
#include "Poco/File.h"
#endif

#include "CX_Clock.h"
#include "CX_Display.h"
#include "CX_InputManager.h"
#include "CX_SoundBufferPlayer.h"


namespace CX {
namespace Util {

ofPoint delimitedStringToPoint(const std::string& locationStr, std::string delim = ",");
ofPoint alignmentStringToPoint(const std::string& align, std::string delim = ",");

ofPoint alignRectangle(ofPoint align, ofPoint p, float w, float h);
ofRectangle alignRectangle(ofPoint align, ofRectangle r);



struct InputInformation {
	std::string filename;
};

struct ParseSettings {

	ParseSettings(void) :
		commentStr("//"),
		kvDlm("="),
		listDlm(","),
		sectionDlmRegex("^#{3,}.*") // Match 3 or more # at start of line plus 0 or more of anything
	{}

	std::string commentStr; //!< Any text including and following `commentStr` will be ignored when parsing files.

	std::string kvDlm; //!< Key-value delimiter. Separates keys from values, e.g. the "=" in "location = 0.95, 1".
	std::string listDlm; //!< List item delimiter. Separates items in a list, e.g. the "," in "location = 0.95, 1".

	std::string sectionDlmRegex; //<! A regular expression for the section delimiter.
};

struct InstructionSetup {
	CX_Display* disp;
	std::shared_ptr<CX_SoundStream> soundStream;
};

/*! Base class for text, image, and sound data. */
struct InstructionData {

	virtual bool setup(const InstructionSetup& is) {
		return true;
	}
	virtual bool parse(std::string section, const ParseSettings& parseSettings, const InputInformation& inputInfo) = 0;
	virtual bool load(void) = 0;
	virtual void draw(CX_Display* disp) = 0;

	virtual std::string getType(void) = 0;

	// Called when the instruction slide starts and stops
	virtual void instructionStarted(void) {
		return;
	};

	virtual void instructionStopped(void) {
		return;
	}

};

struct PositionData {

	PositionData(void) :
		location(0.5, 0.5),
		alignment(0, 0)
	{}

	ofPoint location;
	ofPoint alignment;
};


/*! Defines text that shoud be displayed as part of an instruction screen.
*/
struct TextData : public InstructionData, public PositionData {

	TextData(void) :
		text("NULL"),
		wrapProportion(0.8),
		font(OF_TTF_SANS),
		size(16),
		color(255)
	{}

	std::string text; //!< Text to display.

	float wrapProportion; //!< Text will be wrapped to be no wider than `wrapProportion` of the screen width.

	std::string font; //!< The name of a font to use to display the text.
	int size; //!< Font size.
	ofColor color; //!< Font color.

	bool parse(std::string section, const ParseSettings& parseSettings, const InputInformation& inputInfo) override;
	bool load(void) override;
	void draw(CX_Display* disp) override;

	std::string getType(void) override {
		return "TextData";
	}

private:
	ofTrueTypeFont _ttf;

	static std::string _combineTextLines(const std::vector<std::string>& lines);
};

/*! Defines an image that shoud be displayed as part of an instruction screen.
*/
struct ImageData : public InstructionData, public PositionData {

	ImageData(void) :
		file(""),
		scale(1),
		screenProportion(-1, -1)
	{}

	std::string file; //!< Name of an image file to load.

	float scale; //!< Scale of the image. Defaults to 1, which is unscaled.
	ofPoint screenProportion; //!< Scale of the image in x and y dimensions as a proportion of the screen dimensions. If `screenProportion` has non-negative values, it overrides `scale`.

	bool parse(std::string section, const ParseSettings& parseSettings, const InputInformation& inputInfo) override;
	bool load(void) override;
	void draw(CX_Display* disp) override;

	std::string getType(void) override {
		return "ImageData";
	}

private:

	ofImage _image;
	bool _imageLoaded;

	std::string _instructionFilePath;
};

/*! Defines a sound that shoud be played as part of an instruction screen.
*/
struct SoundData : public InstructionData {

	SoundData(void) :
		file(""),
		gain(0)
	{}

	SoundData(const SoundData& sd) {
		this->operator=(sd);
	}

	SoundData& operator=(const SoundData& rhs) {
		this->file = rhs.file;
		this->gain = rhs.gain;
		this->startTimes = rhs.startTimes;

		this->_soundStream = rhs._soundStream;

		return *this;
	}

	std::string file; //!< Name of a sound file to load.
	float gain; //!< Relative gain of the sound in decibels. Defaults to 0, which is does not change the volume.
	std::vector<CX_Millis> startTimes; //!< Time(s) relative to the start of the instruction screen on which this sound will be played. Deaults to playing once immediately at start of the instruction.

	bool setup(const InstructionSetup& is) override;
	bool parse(std::string section, const ParseSettings& parseSettings, const InputInformation& inputInfo) override;
	bool load(void) override;
	void draw(CX_Display* disp) override;

	void instructionStarted(void) override;
	void instructionStopped(void) override;

	std::string getType(void) override {
		return "SoundData";
	}

	//void setSoundStream(std::shared_ptr<CX_SoundStream> stream);

private:

	std::shared_ptr<CX_SoundStream> _soundStream;

	CX_SoundBufferPlayer _soundPlayer;
	CX_SoundBuffer _soundBuffer;

};


/*! This class helps to present screens of instructions to participants.

The instruction screens are defined in text files that can be easily edited 
even once the experiment program has been compiled.

This class is best documented alongside examples of the configuration files and 
instruction screen files that it is used with. See examples/instructionPresenter for more information.
*/
class InstructionPresenter {
public:

	/*! Contains settings related to the display used to present instructions on. */
	struct DisplayConfig {

		DisplayConfig(void) :
			disp(&CX::Instances::Disp),
			backgroundColor(0)
		{}

		CX_Display* disp; //!< Pointer to the CX_Display to use. Defaults to &CX::Instances::Disp.
		ofColor backgroundColor; //!< Background color of the instruction screens.
	};

	/*! Contains settings related to the text that prompts users to continue past instruction screens. */
	struct ContinuePrompt {

		ContinuePrompt(void) :
			text("CONTINUE_PROMPT_REPLACE_ME"),
			location(0.98, 0.98), // bottom-right corner of screen
			alignment(1, 1), // bottom-right of corner of text
			font(OF_TTF_SANS),
			size(14),
			color(255),
			matchInputDelay(true)
		{}

		std::string text; //!< The text to display to prompt users to continue.
		ofPoint location; //!< The location, in proportion of screen size, of the continue text.
		ofPoint alignment; //!< The alignment of the text.

		std::string font; //!< The name of the font to use for the continue text.
		float size; //!< The size of the continue text font.
		ofColor color; //!< The color of the contiue text.

		bool matchInputDelay; //!< If `matchInputDelay && ContinueInput::acceptanceDelay > 0`, the continue prompt will be delayed by that amount of time.
	};

	/*! Contains settings related to the input device(s) to use to continue past instruction screens. */
	struct ContinueInput {

		ContinueInput(void) :
			acceptanceDelay(CX_Millis(0))
		{
			keyboard.enabled = true;
			keyboard.backKey = CX::Keycode::BACKSPACE;
			keyboard.skipKey = -1; // Disabled by default. CX::Keycode::ESCAPE;
			//keyboard.forwardKeys = { CX::Keycode::ENTER };

			mouse.enabled = false;
		}

		struct {
			bool enabled; //!< Whether the keyboard is used to continue.
			std::vector<int> forwardKeys; //!< The keys that, when pressed, continue past the instruction screen.
			int backKey; //!< The key that returns to the previous instruction screen in the current block of instruction screens.               
			int skipKey; //!< For testing. The key that skips all instruction screens in the current block. Disabled by default. Should be disabled in finished experiments. Ignores `acceptanceDelay`.
		} keyboard;

		struct {
			bool enabled; //!< Whether the mouse is used to continue.
			std::vector<int> buttons; //!< The buttons that, when clicked, continue past the instruction screen.
		} mouse;

		CX_Millis acceptanceDelay; //!< The first time an instruction screen is displayed, the user must wait `acceptanceDelay` before continuing to the next slide.
	};

	/*! Contains settings to when instruction screens are reloaded. */
	struct ReloadConfiguration {
		ReloadConfiguration(void) :
			onChange(true),
			key(CX::Keycode::F5)
		{}
		bool onChange; //!< Whether instruction screens should automatically be reloaded from file if a file modification is detected (based on file modified timestamps).
		int key; //!< A keyboard key to use to manually reload the instruction screen.
	};

	/*! Contains all of the settings for the InstructionPresenter. */
	struct Configuration {

		//Configuration(void) : {}
		
		DisplayConfig display;
		CX_SoundStream::Configuration sound;

		ContinueInput input;
		ContinuePrompt prompt;
		//std::vector<ContinuePrompt> prompts; ?

		ReloadConfiguration reload;

		ParseSettings parse;

		struct {
			TextData text;
			ImageData image;
			SoundData sound;
		} defaults;

		CX_Millis sleepDuration = 5; //!< To save CPU cycles, sleeps for this many milliseconds each updating loop when checking for user input.
	};

	bool setup(const Configuration& config);
	bool setup(std::string configFile, CX_Display* disp = &CX::Instances::Disp, ParseSettings parseSet = ParseSettings());

	const Configuration& getConfiguration(void);

	void present(std::string instFile);
	void present(const std::vector<std::string>& instFiles);

	//bool presentText(const std::vector<std::string>& instText);
	//bool presentObjects(const std::vector<std::shared_ptr<InstructionData>>& instObj);

private:

	Configuration _config;

	bool _setInstructionDefaults(std::string inputFile);

	void _inputSetup(void);
	void _soundSetup(void);

	void _drawContinueText(void);

	enum class AwaitResult : int {
		DoNothing,
		NextInstruction,
		PreviousInstruction,
		ReloadInstruction,
		SkipInstructions
	};

	AwaitResult _getAwaitResult(void);

	struct ActiveInstruction {
		// data members should be treated as read-only
		std::string filename;
		std::string dataPathFilename;

		std::vector<std::shared_ptr<InstructionData>> instructions;

		CX_Millis startTime;

		bool setup(InstructionPresenter* ip, const std::string& filename = "");

		bool setInstruction(const std::string& filename);
		bool startInstruction(bool firstStart);
		bool startInstruction(const std::string& filename, bool firstStart);

		void stopInstruction(void);

		//void drawCurrentInstruction(bool drawContinuePromptImmediately);

		
		//bool restartInstruction(void);

		//void start(bool restart = false, bool drawContinuePromptImmediately = false);
		//void update(void);

		bool fileHasChanged(bool update = true);

	private:
		InstructionPresenter* _ip;

		//bool _promptDrawn;
#if OF_VERSION_MAJOR == 0 && OF_VERSION_MINOR < 10
		Poco::File _pocoFile;
		Poco::Timestamp _fileLastModified;
#else
		time_t _fileLastModified;
#endif

	};

	void _presentSection(const std::vector<std::string>& instFiles);
	//void _presentSingleFile(const std::string& filename);
	void _drawInstructionDisplay(const std::vector<std::shared_ptr<InstructionData>>& instructions, bool drawContinue);

	struct ParsedSection;

	std::vector<std::shared_ptr<InstructionData>> _loadInstructionFile(std::string filename, bool keepOnlyParsed = true, bool keepOnlyLoaded = true) const;
	std::vector<std::shared_ptr<InstructionData>> _parseInstructionFile(std::string filename, bool keepOnlyParsed = true) const;
	std::vector<std::shared_ptr<InstructionData>> _parseInstructionText(const std::string& instructionText, const InputInformation& inputInfo, bool keepOnlyParsed = true) const;
	ParsedSection _parseInstructionSection(const std::string& sectionText, const InputInformation& inputInfo) const;
	

	// Sound
	std::shared_ptr<CX_SoundStream> _soundStream;
	
};


} // namespace Util
} // namespace CX

