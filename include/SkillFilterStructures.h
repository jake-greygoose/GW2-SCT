// Updated SkillFilterStructures.h without export functionality

#pragma once
#include <vector>
#include <string>
#include <map>
#include <memory>
#include "json.hpp"

namespace nlohmann {
    template <typename T>
    struct adl_serializer<std::shared_ptr<T>> {
        static void to_json(json& j, const std::shared_ptr<T>& opt) {
            if (opt) {
                j = *opt;
            }
            else {
                j = nullptr;
            }
        }
        static void from_json(const json& j, std::shared_ptr<T>& opt) {
            if (j.is_null()) {
                opt = nullptr;
            }
            else {
                opt = std::make_shared<T>(j.get<T>());
            }
        }
    };
}

namespace GW2_SCT {
    enum class FilterAction {
        ALLOW = 0,
        BLOCK
    };

    enum class FilterType {
        SKILL_ID = 0,
        SKILL_NAME,
        SKILL_ID_RANGE
    };

    struct SkillIdRange {
        uint32_t start;
        uint32_t end;
    };

    struct SkillFilter {
        FilterType type = FilterType::SKILL_ID;
        FilterAction action = FilterAction::BLOCK;
        uint32_t skillId = 0;
        std::string skillName = "";
        SkillIdRange idRange = { 0, 0 };

        int getSpecificityScore() const {
            switch (type) {
            case FilterType::SKILL_ID: return 100;
            case FilterType::SKILL_NAME: return 50;
            case FilterType::SKILL_ID_RANGE: return 10;
            }
            return 0;
        }

        bool matches(uint32_t skillId, const std::string& skillName) const {
            switch (type) {
            case FilterType::SKILL_ID:
                return this->skillId == skillId;
            case FilterType::SKILL_NAME:
                return this->skillName == skillName;
            case FilterType::SKILL_ID_RANGE:
                return skillId >= idRange.start && skillId <= idRange.end;
            }
            return false;
        }
    };

    class AdvancedSkillFilterSet {
    public:
        FilterAction defaultAction = FilterAction::ALLOW;
        std::vector<SkillFilter> filters;

        bool isFiltered(uint32_t skillId, const std::string& skillName) const {
            const SkillFilter* bestMatch = nullptr;
            int highestSpecificity = -1;

            for (const auto& filter : filters) {
                if (filter.matches(skillId, skillName)) {
                    int specificity = filter.getSpecificityScore();
                    if (specificity > highestSpecificity) {
                        highestSpecificity = specificity;
                        bestMatch = &filter;
                    }
                }
            }

            if (bestMatch) {
                return bestMatch->action == FilterAction::BLOCK;
            }

            return defaultAction == FilterAction::BLOCK;
        }
    };

    class NamedSkillFilterSet {
    public:
        std::string name = "";
        std::string description = "";
        AdvancedSkillFilterSet filterSet;
    };

    class SkillFilterManager {
    private:
        std::map<std::string, std::shared_ptr<NamedSkillFilterSet>> filterSets;

    public:
        void addFilterSet(std::shared_ptr<NamedSkillFilterSet> filterSet) {
            if (filterSet) {
                filterSets[filterSet->name] = filterSet;
            }
        }

        std::shared_ptr<NamedSkillFilterSet> createFilterSet(const std::string& name) {
            auto filterSet = std::make_shared<NamedSkillFilterSet>();
            filterSet->name = name;
            filterSets[name] = filterSet;
            return filterSet;
        }

        bool removeFilterSet(const std::string& name) {
            auto it = filterSets.find(name);
            if (it != filterSets.end()) {
                filterSets.erase(it);
                return true;
            }
            return false;
        }

        std::shared_ptr<NamedSkillFilterSet> getFilterSet(const std::string& name) const {
            auto it = filterSets.find(name);
            return (it != filterSets.end()) ? it->second : nullptr;
        }

        const std::map<std::string, std::shared_ptr<NamedSkillFilterSet>>& getAllFilterSets() const {
            return filterSets;
        }
    };

    void to_json(nlohmann::json& j, const FilterAction& action);
    void from_json(const nlohmann::json& j, FilterAction& action);
    void to_json(nlohmann::json& j, const SkillIdRange& range);
    void from_json(const nlohmann::json& j, SkillIdRange& range);
    void to_json(nlohmann::json& j, const SkillFilter& filter);
    void from_json(const nlohmann::json& j, SkillFilter& filter);
    void to_json(nlohmann::json& j, const AdvancedSkillFilterSet& filterSet);
    void from_json(const nlohmann::json& j, AdvancedSkillFilterSet& filterSet);
    void to_json(nlohmann::json& j, const NamedSkillFilterSet& filterSet);
    void from_json(const nlohmann::json& j, NamedSkillFilterSet& filterSet);
}