#pragma once
#include <vector>
#include <functional>
#include <map>
#include <string>
#include <mutex>
#include "UtilStructures.h"
#include "OptionsStructures.h"

namespace GW2_SCT {
	
	class Profiles {
	public:
		static const std::shared_ptr<profile_options_struct> get() { return profile.operator std::shared_ptr<profile_options_struct>(); }
		static ObservableValue<std::shared_ptr<profile_options_struct>> profile;
		
		static void requestSwitch(std::shared_ptr<profile_options_struct> newProfile);
		static void processPendingSwitch();
		static void loadForCharacter(std::string characterName);
		static void paintUI();
		static void initWithDefaults(std::shared_ptr<profile_options_struct> p);
		
		// Access to current profile name and character name
		static const std::string& getCurrentProfileName() { return currentProfileName; }
		static const std::string& getCurrentCharacterName() { return currentCharacterName; }
		static const std::string& getDefaultProfileName() { return defaultProfileName; }
		static void setCurrentCharacterName(const std::string& name) { currentCharacterName = name; }

	private:
		static const std::string defaultProfileName;
		static std::string currentProfileName;
		static std::string currentCharacterName;
		
		static std::mutex profileSwitchMutex;
		static std::shared_ptr<profile_options_struct> pendingProfile;
	};
}