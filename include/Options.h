#pragma once
#include <vector>
#include <functional>
#include <map>
#include <string>
#include "UtilStructures.h"
#include "OptionsStructures.h"

namespace GW2_SCT {
	class ScrollArea;

	std::map<char, std::string> mapParameterListToLanguage(const char* section, std::vector<char> list);

	class Options {
	public:
		static const std::shared_ptr<profile_options_struct> get();
		// Access to options struct for Profiles class
		static options_struct& getOptionsStruct() { return options; }
		static void paint(const std::vector<std::shared_ptr<ScrollArea>>& scrollAreas);
		static void open();
		static void save();
		static void load();
		static void requestSave();
		static void processPendingSave();
		static std::string getCurrentCharacterName();
		static std::string getFontSelectionString(bool withMaster = true) { return withMaster ? fontSelectionStringWithMaster : fontSelectionString; };
		static std::string getFontSizeTypeSelectionString() { return fontSizeTypeSelectionString; };
		static bool isOptionsWindowOpen();
		static bool isInScrollAreasTab();
		static void paintScrollAreaOverlay(const std::vector<std::shared_ptr<ScrollArea>>& scrollAreas);
	private:
		static void setDefault();
		static void paintGeneral();
		static void paintScrollAreas(const std::vector<std::shared_ptr<ScrollArea>>& scrollAreas);
		static void paintProfessionColors();
		static void paintGlobalThresholds();
		static void paintSkillIcons();
		static options_struct options;
		static bool windowIsOpen;
		static std::string fontSelectionString;
		static std::string fontSizeTypeSelectionString;
		static std::string fontSelectionStringWithMaster;

		static std::chrono::steady_clock::time_point lastSaveRequest;
		static bool saveRequested;
		static const std::chrono::milliseconds SAVE_DELAY; // 500ms delay
	};

	struct receiver_information {
		std::string iniPrefix;
		std::map<char, std::string> options;
		message_receiver_options_struct defaultReceiver;
	};
	extern const std::map<MessageCategory, const std::map<MessageType, receiver_information>> receiverInformationPerCategoryAndType;
	extern const std::map<MessageCategory, std::string> categoryNames;
	extern const std::map<MessageType, std::string> typeNames;
	extern const std::map<SkillIconDisplayType, std::string> skillIconsDisplayTypeNames;
}