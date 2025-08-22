#include "SkillFilterStructures.h"

namespace GW2_SCT {
	void to_json(nlohmann::json& j, const FilterAction& action) {
		j = static_cast<int>(action);
	}

	void from_json(const nlohmann::json& j, FilterAction& action) {
		action = static_cast<FilterAction>(j.get<int>());
	}

	void to_json(nlohmann::json& j, const SkillIdRange& range) {
		j = nlohmann::json{
			{"start", range.start},
			{"end", range.end}
		};
	}

	void from_json(const nlohmann::json& j, SkillIdRange& range) {
		j.at("start").get_to(range.start);
		j.at("end").get_to(range.end);
	}

	void to_json(nlohmann::json& j, const SkillFilter& filter) {
		j = nlohmann::json{
			{"type", static_cast<int>(filter.type)},
			{"action", filter.action}
		};

		switch (filter.type) {
		case FilterType::SKILL_ID:
			j["skillId"] = filter.skillId;
			break;
		case FilterType::SKILL_NAME:
			j["skillName"] = filter.skillName;
			break;
		case FilterType::SKILL_ID_RANGE:
			j["range"] = filter.idRange;
			break;
		}
	}

	void from_json(const nlohmann::json& j, SkillFilter& filter) {
		filter.type = static_cast<FilterType>(j.at("type").get<int>());
		j.at("action").get_to(filter.action);

		switch (filter.type) {
		case FilterType::SKILL_ID:
			if (j.contains("skillId")) {
				j.at("skillId").get_to(filter.skillId);
			}
			break;
		case FilterType::SKILL_NAME:
			if (j.contains("skillName")) {
				j.at("skillName").get_to(filter.skillName);
			}
			break;
		case FilterType::SKILL_ID_RANGE:
			if (j.contains("range")) {
				j.at("range").get_to(filter.idRange);
			}
			break;
		}
	}

	void to_json(nlohmann::json& j, const AdvancedSkillFilterSet& filterSet) {
		j = nlohmann::json{
			{"defaultAction", filterSet.defaultAction},
			{"filters", filterSet.filters}
		};
	}

	void from_json(const nlohmann::json& j, AdvancedSkillFilterSet& filterSet) {
		j.at("defaultAction").get_to(filterSet.defaultAction);
		j.at("filters").get_to(filterSet.filters);
	}

	void to_json(nlohmann::json& j, const NamedSkillFilterSet& filterSet) {
		j = nlohmann::json{
			{"name", filterSet.name},
			{"description", filterSet.description},
			{"filterSet", filterSet.filterSet}
		};
	}

	void from_json(const nlohmann::json& j, NamedSkillFilterSet& filterSet) {
		j.at("name").get_to(filterSet.name);
		if (j.contains("description")) {
			j.at("description").get_to(filterSet.description);
		}
		j.at("filterSet").get_to(filterSet.filterSet);
	}
}
