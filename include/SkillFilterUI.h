#pragma once
#include <string>

namespace GW2_SCT {
	
	class SkillFilterUI {
	public:
		static void paintUI();
		static std::string getFilterTypeSelectionString() { return skillFilterTypeSelectionString; }
		static void initialize();

	private:
		static std::string skillFilterTypeSelectionString;
	};
}