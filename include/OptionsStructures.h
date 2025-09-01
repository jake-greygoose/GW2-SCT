#pragma once
#include <vector>
#include <string>
#include "json.hpp"
#include "Common.h"
#include "UtilStructures.h"
#include "SkillFilterStructures.h"


namespace GW2_SCT {
	enum class TextAlign {
		LEFT = 0,
		CENTER,
		RIGHT
	};
	extern int textAlignToInt(TextAlign type);
	extern TextAlign intToTextAlign(int i);

	enum class TextCurve {
		LEFT = 0,
		STRAIGHT,
		RIGHT,
		STATIC
	};
	extern int textCurveToInt(TextCurve type);
	extern TextCurve intToTextCurve(int i);

	enum class SkillIconDisplayType {
		NORMAL = 0,
		BLACK_CULLED,
		BORDER_BLACK_CULLED,
		BORDER_TOUCHING_BLACK_CULLED
	};
	extern int skillIconDisplayTypeToInt(SkillIconDisplayType type);
	extern SkillIconDisplayType intSkillIconDisplayType(int i);

	class profile_options_struct;
	class scroll_area_options_struct;
	class message_receiver_options_struct;

	class options_struct {
	public:
		std::string globalProfile = "default";
		shared_ptr_map_with_creation<std::string, profile_options_struct> profiles;
		std::map<std::string, std::string> characterProfileMap;
	};
	void to_json(nlohmann::json& j, const options_struct& p);
	void from_json(const nlohmann::json& j, options_struct& p);

	class profile_options_struct {
	public:
		bool sctEnabled = true;
		float scrollSpeed = 90.f;
		bool dropShadow = true;
		int messagesInStack = 3;
		bool combineAllMessages = true;
		FontId masterFont = 0;
		float defaultFontSize = 22.f;
		float defaultCritFontSize = 30.f;
		bool selfMessageOnlyIncoming = false;
		bool outgoingOnlyToTarget = false;
		std::string professionColorGuardian = "72C1D9";
		std::string professionColorWarrior = "FFD166";
		std::string professionColorEngineer = "D09C59";
		std::string professionColorRanger = "8EDF44";
		std::string professionColorThief = "C08F95";
		std::string professionColorElementalist = "F68A87";
		std::string professionColorMesmer = "B679D5";
		std::string professionColorNecromancer = "52A76F";
		std::string professionColorRevenant = "D16E5A";
		std::string professionColorDefault = "FF0000";
		ObservableVector<std::shared_ptr<scroll_area_options_struct>> scrollAreaOptions = {};
		SkillFilterManager filterManager;
		ObservableValue<bool> skillIconsEnabled = false;
		ObservableValue<bool> preloadAllSkillIcons = false;
		SkillIconDisplayType skillIconsDisplayType = SkillIconDisplayType::NORMAL;
	};
	void to_json(nlohmann::json& j, const profile_options_struct& p);
	void from_json(const nlohmann::json& j, profile_options_struct& p);

	enum class ScrollAreaOutlineState {
		NONE = 0,
		LIGHT,
		FULL
	};

	enum class ScrollDirection {
		DOWN = 0,
		UP = 1
	};
	extern int scrollDirectionToInt(ScrollDirection type);
	extern ScrollDirection intToScrollDirection(int i);

	class scroll_area_options_struct {
	public:
		bool enabled = true;
		std::string name = "";
		float offsetX = 0;
		float offsetY = 0;
		float width = 0;
		float height = 0;
		TextAlign textAlign = TextAlign::LEFT;
		TextCurve textCurve = TextCurve::LEFT;
		ScrollDirection scrollDirection = ScrollDirection::DOWN;
		bool showCombinedHitCount = true;
		ScrollAreaOutlineState outlineState = ScrollAreaOutlineState::NONE;
		ObservableVector<std::shared_ptr<message_receiver_options_struct>> receivers = {};
		bool abbreviateSkillNames = false;
		int  shortenNumbersPrecision = -1;
		bool disableCombining = false;
		float customScrollSpeed = -1.0f;
	};
	void to_json(nlohmann::json& j, const scroll_area_options_struct& p);
	void from_json(const nlohmann::json& j, scroll_area_options_struct& p);

	class message_receiver_options_struct {
	public:
		std::string name = "";
		MessageCategory category = MessageCategory::PLAYER_OUT;
		MessageType type = MessageType::PHYSICAL;
		bool enabled = true;
		ObservableValue<std::string> outputTemplate = std::string("");
		ObservableValue<std::string> color = std::string("#FFFFFF");
		ObservableValue<FontId> font = 0;
		ObservableValue<float> fontSize = 22.f;
		std::vector<std::string> assignedFilterSets = {};
		bool filtersEnabled = true;
		bool transient_showCombinedHitCount = true;
		bool transient_abbreviateSkillNames = false;
		int  transient_numberShortPrecision = -1;

		// TODO: relocate this
		bool isSkillFiltered(uint32_t skillId, const std::string& skillName, const SkillFilterManager& filterManager) const {
			if (!filtersEnabled || assignedFilterSets.empty()) {
				return false;
			}

			std::vector<std::pair<FilterAction, int>> matchingFilters; // action, specificity
			FilterAction combinedDefaultAction = FilterAction::ALLOW;

			for (const std::string& filterSetName : assignedFilterSets) {
				auto filterSet = filterManager.getFilterSet(filterSetName);
				if (!filterSet) continue;

				if (filterSet->filterSet.defaultAction == FilterAction::BLOCK) {
					combinedDefaultAction = FilterAction::BLOCK;
				}

				for (const auto& filter : filterSet->filterSet.filters) {
					if (filter.matches(skillId, skillName)) {
						matchingFilters.push_back({ filter.action, filter.getSpecificityScore() });
					}
				}
			}

			if (matchingFilters.empty()) {
				return combinedDefaultAction == FilterAction::BLOCK;
			}

			FilterAction finalAction = combinedDefaultAction;
			int highestSpecificity = -1;
			bool hasAllow = false;
			bool hasBlock = false;

			for (const auto& filterPair : matchingFilters) {
				if (filterPair.second > highestSpecificity) {
					highestSpecificity = filterPair.second;
				}
			}

			for (const auto& filterPair : matchingFilters) {
				if (filterPair.second == highestSpecificity) {
					if (filterPair.first == FilterAction::ALLOW) hasAllow = true;
					if (filterPair.first == FilterAction::BLOCK) hasBlock = true;
				}
			}

			if (hasAllow && hasBlock) {
				finalAction = FilterAction::ALLOW;  // ALLOW wins conflicts
			}
			else if (hasAllow) {
				finalAction = FilterAction::ALLOW;
			}
			else if (hasBlock) {
				finalAction = FilterAction::BLOCK;
			}

			return finalAction == FilterAction::BLOCK;
		}
	};
	void to_json(nlohmann::json& j, const message_receiver_options_struct& p);
	void from_json(const nlohmann::json& j, message_receiver_options_struct& p);
}