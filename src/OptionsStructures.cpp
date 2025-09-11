#include "OptionsStructures.h"
#include "Message.h"
#include <memory>
#include <map>
#include <utility>
#include <string>

// ----------------------------------------------
//  Enum <-> int helpers declared in the header
// ----------------------------------------------
namespace GW2_SCT {

    int textAlignToInt(TextAlign type) {
        return static_cast<int>(type);
    }
    TextAlign intToTextAlign(int i) {
        switch (i) {
        case 0: return TextAlign::LEFT;
        case 1: return TextAlign::CENTER;
        case 2: return TextAlign::RIGHT;
        default: return TextAlign::LEFT;
        }
    }

    int textCurveToInt(TextCurve type) {
        return static_cast<int>(type);
    }
    TextCurve intToTextCurve(int i) {
        switch (i) {
        case 0: return TextCurve::LEFT;
        case 1: return TextCurve::STRAIGHT;
        case 2: return TextCurve::RIGHT;
        case 3: return TextCurve::STATIC;
        default: return TextCurve::LEFT;
        }
    }

    int skillIconDisplayTypeToInt(SkillIconDisplayType type) {
        return static_cast<int>(type);
    }
    SkillIconDisplayType intSkillIconDisplayType(int i) {
        switch (i) {
        case 0: return SkillIconDisplayType::NORMAL;
        case 1: return SkillIconDisplayType::BLACK_CULLED;
        default: return SkillIconDisplayType::NORMAL;
        }
    }

    int scrollDirectionToInt(ScrollDirection type) {
        return static_cast<int>(type);
    }
    ScrollDirection intToScrollDirection(int i) {
        switch (i) {
        case 1: return ScrollDirection::UP;
        case 0:
        default: return ScrollDirection::DOWN;
        }
    }

} // namespace GW2_SCT

// ----------------------------------------------
//  JSON adapters
// ----------------------------------------------
namespace nlohmann {

    template <typename T>
    struct adl_serializer<ObservableVector<T>> {
        static void to_json(json& j, const ObservableVector<T>& v) {
            j = json::array();
            for (const auto& el : v) j.push_back(el);
        }
        static void from_json(const json& j, ObservableVector<T>& v) {
            v.clear();
            if (!j.is_array()) return;
            for (const auto& el : j) {
                T item{};
                el.get_to(item);
                v.push_back(std::move(item));
            }
        }
    };

    template <typename K, typename V>
    struct adl_serializer<shared_ptr_map_with_creation<K, V>> {
        static void to_json(json& j, const shared_ptr_map_with_creation<K, V>& m) {
            j = json::object();
            for (const auto& kv : m) {
                if (kv.second) j[kv.first] = *kv.second;
                else           j[kv.first] = nullptr;
            }
        }
        static void from_json(const json& j, shared_ptr_map_with_creation<K, V>& m) {
            m.clear();
            if (!j.is_object()) {
                return;
            }
            for (auto it = j.begin(); it != j.end(); ++it) {
                if (it->is_null()) {
                    m[it.key()] = nullptr;
                }
                else {
                    try {
                        auto ptr = std::make_shared<V>();
                        it.value().get_to(*ptr);
                        m[it.key()] = std::move(ptr);
                    }
                    catch (const std::exception& e) {
                    }
                }
            }
        }
    };

} // namespace nlohmann

// ----------------------------------------------
//  SkillFilterManager JSON
// ----------------------------------------------
namespace GW2_SCT {

    void to_json(nlohmann::json& j, const SkillFilterManager& manager) {
        nlohmann::json filterSetsJson = nlohmann::json::object();
        for (const auto& [name, filterSet] : manager.getAllFilterSets()) {
            if (filterSet) {
                filterSetsJson[name] = *filterSet;
            }
            else {
                filterSetsJson[name] = nullptr;
            }
        }
        j = std::move(filterSetsJson);
    }

    void from_json(const nlohmann::json& j, SkillFilterManager& manager) {
        if (!j.is_object()) return;
        for (auto it = j.begin(); it != j.end(); ++it) {
            try {
                if (it->is_null()) {
                    continue;
                }
                auto filterSet = std::make_shared<NamedSkillFilterSet>();
                it.value().get_to(*filterSet);
                manager.addFilterSet(filterSet);
            }
            catch (...) {
            }
        }
    }

} // namespace GW2_SCT

// ----------------------------------------------
//  options_struct JSON
// ----------------------------------------------
namespace GW2_SCT {

    void to_json(nlohmann::json& j, const options_struct& p) {
        j = nlohmann::json{
            {"globalProfile", p.globalProfile},
            {"profiles", p.profiles},
            {"characterProfileMap", p.characterProfileMap}
        };
    }

    void from_json(const nlohmann::json& j, options_struct& p) {
        if (j.contains("globalProfile")) j.at("globalProfile").get_to(p.globalProfile);
        if (j.contains("profiles")) j.at("profiles").get_to(p.profiles);
        if (j.contains("characterProfileMap")) j.at("characterProfileMap").get_to(p.characterProfileMap);
    }

} // namespace GW2_SCT

// ----------------------------------------------
//  profile_options_struct JSON
// ----------------------------------------------
namespace GW2_SCT {

    void to_json(nlohmann::json& j, const profile_options_struct& p) {
        j = nlohmann::json{};
        j["sctEnabled"] = p.sctEnabled;
        j["scrollSpeed"] = p.scrollSpeed;
        j["dropShadow"] = p.dropShadow;
        j["messagesInStack"] = p.messagesInStack;
        j["combineAllMessages"] = p.combineAllMessages;
        j["masterFont"] = p.masterFont;
        j["defaultFontSize"] = p.defaultFontSize;
        j["defaultCritFontSize"] = p.defaultCritFontSize;
        j["selfMessageOnlyIncoming"] = p.selfMessageOnlyIncoming;
        j["outgoingOnlyToTarget"] = p.outgoingOnlyToTarget;
        j["professionColorGuardian"] = p.professionColorGuardian;
        j["professionColorWarrior"] = p.professionColorWarrior;
        j["professionColorEngineer"] = p.professionColorEngineer;
        j["professionColorRanger"] = p.professionColorRanger;
        j["professionColorThief"] = p.professionColorThief;
        j["professionColorElementalist"] = p.professionColorElementalist;
        j["professionColorMesmer"] = p.professionColorMesmer;
        j["professionColorNecromancer"] = p.professionColorNecromancer;
        j["professionColorRevenant"] = p.professionColorRevenant;
        j["professionColorDefault"] = p.professionColorDefault;
        j["scrollAreaOptions"] = p.scrollAreaOptions;
        j["filterManager"] = p.filterManager;
        j["skillIconsEnabled"] = p.skillIconsEnabled;
        j["preloadAllSkillIcons"] = p.preloadAllSkillIcons;
        j["skillIconsDisplayType"] = skillIconDisplayTypeToInt(p.skillIconsDisplayType);
        j["globalThresholdsEnabled"] = p.globalThresholdsEnabled;
        j["globalDamageThreshold"] = p.globalDamageThreshold;
        j["globalHealThreshold"] = p.globalHealThreshold;
        j["globalAbsorbThreshold"] = p.globalAbsorbThreshold;
        j["globalThresholdRespectFilters"] = p.globalThresholdRespectFilters;
    }

    void from_json(const nlohmann::json& j, profile_options_struct& p) {
        if (j.contains("sctEnabled")) j.at("sctEnabled").get_to(p.sctEnabled);
        if (j.contains("scrollSpeed")) j.at("scrollSpeed").get_to(p.scrollSpeed);
        if (j.contains("dropShadow")) j.at("dropShadow").get_to(p.dropShadow);
        if (j.contains("messagesInStack")) j.at("messagesInStack").get_to(p.messagesInStack);
        if (j.contains("combineAllMessages")) j.at("combineAllMessages").get_to(p.combineAllMessages);
        if (j.contains("masterFont")) j.at("masterFont").get_to(p.masterFont);
        if (j.contains("defaultFontSize")) j.at("defaultFontSize").get_to(p.defaultFontSize);
        if (j.contains("defaultCritFontSize")) j.at("defaultCritFontSize").get_to(p.defaultCritFontSize);
        if (j.contains("selfMessageOnlyIncoming")) j.at("selfMessageOnlyIncoming").get_to(p.selfMessageOnlyIncoming);
        if (j.contains("outgoingOnlyToTarget")) j.at("outgoingOnlyToTarget").get_to(p.outgoingOnlyToTarget);
        if (j.contains("professionColorGuardian")) j.at("professionColorGuardian").get_to(p.professionColorGuardian);
        if (j.contains("professionColorWarrior")) j.at("professionColorWarrior").get_to(p.professionColorWarrior);
        if (j.contains("professionColorEngineer")) j.at("professionColorEngineer").get_to(p.professionColorEngineer);
        if (j.contains("professionColorRanger")) j.at("professionColorRanger").get_to(p.professionColorRanger);
        if (j.contains("professionColorThief")) j.at("professionColorThief").get_to(p.professionColorThief);
        if (j.contains("professionColorElementalist")) j.at("professionColorElementalist").get_to(p.professionColorElementalist);
        if (j.contains("professionColorMesmer")) j.at("professionColorMesmer").get_to(p.professionColorMesmer);
        if (j.contains("professionColorNecromancer")) j.at("professionColorNecromancer").get_to(p.professionColorNecromancer);
        if (j.contains("professionColorRevenant")) j.at("professionColorRevenant").get_to(p.professionColorRevenant);
        if (j.contains("professionColorDefault")) j.at("professionColorDefault").get_to(p.professionColorDefault);
        if (j.contains("scrollAreaOptions")) j.at("scrollAreaOptions").get_to(p.scrollAreaOptions);
        if (j.contains("filterManager")) j.at("filterManager").get_to(p.filterManager);
        if (j.contains("skillIconsEnabled")) j.at("skillIconsEnabled").get_to(p.skillIconsEnabled);
        if (j.contains("preloadAllSkillIcons")) j.at("preloadAllSkillIcons").get_to(p.preloadAllSkillIcons);
        if (j.contains("skillIconsDisplayType")) {
            int v{}; j.at("skillIconsDisplayType").get_to(v);
            p.skillIconsDisplayType = intSkillIconDisplayType(v);
        }
        if (j.contains("globalThresholdsEnabled")) j.at("globalThresholdsEnabled").get_to(p.globalThresholdsEnabled);
        if (j.contains("globalDamageThreshold")) j.at("globalDamageThreshold").get_to(p.globalDamageThreshold);
        if (j.contains("globalHealThreshold")) j.at("globalHealThreshold").get_to(p.globalHealThreshold);
        if (j.contains("globalAbsorbThreshold")) j.at("globalAbsorbThreshold").get_to(p.globalAbsorbThreshold);
        if (j.contains("globalThresholdRespectFilters")) j.at("globalThresholdRespectFilters").get_to(p.globalThresholdRespectFilters);
    }

} // namespace GW2_SCT

// ----------------------------------------------
//  scroll_area_options_struct JSON
// ----------------------------------------------
namespace GW2_SCT {

    static int outlineStateToInt(ScrollAreaOutlineState s) {
        return static_cast<int>(s);
    }
    static ScrollAreaOutlineState intToOutlineState(int i) {
        return static_cast<ScrollAreaOutlineState>(i);
    }

    void to_json(nlohmann::json& j, const scroll_area_options_struct& p) {
        j = nlohmann::json{};
        j["name"] = p.name;
        j["enabled"] = p.enabled;
        j["offsetX"] = p.offsetX;
        j["offsetY"] = p.offsetY;
        j["width"] = p.width;
        j["height"] = p.height;
        j["textAlign"] = textAlignToInt(p.textAlign);
        j["textCurve"] = textCurveToInt(p.textCurve);
        j["scrollDirection"] = scrollDirectionToInt(p.scrollDirection);
        j["abbreviateSkillNames"] = p.abbreviateSkillNames;
        j["shortenNumbersPrecision"] = p.shortenNumbersPrecision;
        j["disableCombining"] = p.disableCombining;
        j["showCombinedHitCount"] = p.showCombinedHitCount;
        j["customScrollSpeed"] = p.customScrollSpeed;
        j["minLineSpacingPx"] = p.minLineSpacingPx;
        j["maxNudgeMsPerSecond"] = p.maxNudgeMsPerSecond;
        j["staticDisplayTimeMs"] = p.staticDisplayTimeMs;
        j["overflowMaxFactor"] = p.overflowMaxFactor;
        j["occupancyStart"] = p.occupancyStart;
        j["occupancyEnd"] = p.occupancyEnd;
        j["overflowSmoothingTau"] = p.overflowSmoothingTau;
        j["receivers"] = p.receivers;
    }

    void from_json(const nlohmann::json& j, scroll_area_options_struct& p) {
        if (j.contains("name")) j.at("name").get_to(p.name);
        if (j.contains("enabled")) j.at("enabled").get_to(p.enabled);
        if (j.contains("offsetX")) j.at("offsetX").get_to(p.offsetX);
        if (j.contains("offsetY")) j.at("offsetY").get_to(p.offsetY);
        if (j.contains("width")) j.at("width").get_to(p.width);
        if (j.contains("height")) j.at("height").get_to(p.height);
        if (j.contains("textAlign")) { int v{}; j.at("textAlign").get_to(v); p.textAlign = intToTextAlign(v); }
        if (j.contains("textCurve")) { int v{}; j.at("textCurve").get_to(v); p.textCurve = intToTextCurve(v); }
        if (j.contains("scrollDirection")) { int v{}; j.at("scrollDirection").get_to(v); p.scrollDirection = intToScrollDirection(v); }
        if (j.contains("abbreviateSkillNames")) j.at("abbreviateSkillNames").get_to(p.abbreviateSkillNames);
        if (j.contains("shortenNumbersPrecision")) j.at("shortenNumbersPrecision").get_to(p.shortenNumbersPrecision);
        if (j.contains("disableCombining")) j.at("disableCombining").get_to(p.disableCombining);
        if (j.contains("showCombinedHitCount")) j.at("showCombinedHitCount").get_to(p.showCombinedHitCount);
        if (j.contains("customScrollSpeed")) j.at("customScrollSpeed").get_to(p.customScrollSpeed);
        if (j.contains("minLineSpacingPx")) j.at("minLineSpacingPx").get_to(p.minLineSpacingPx);
        if (j.contains("maxNudgeMsPerSecond")) {
            j.at("maxNudgeMsPerSecond").get_to(p.maxNudgeMsPerSecond);
        }        
        if (j.contains("staticDisplayTimeMs")) j.at("staticDisplayTimeMs").get_to(p.staticDisplayTimeMs);
        if (j.contains("overflowMaxFactor")) j.at("overflowMaxFactor").get_to(p.overflowMaxFactor);
        if (j.contains("occupancyStart")) j.at("occupancyStart").get_to(p.occupancyStart);
        if (j.contains("occupancyEnd")) j.at("occupancyEnd").get_to(p.occupancyEnd);
        if (j.contains("overflowSmoothingTau")) j.at("overflowSmoothingTau").get_to(p.overflowSmoothingTau);
        if (j.contains("receivers")) j.at("receivers").get_to(p.receivers);
    }

} // namespace GW2_SCT

// ----------------------------------------------
//  message_receiver_options_struct JSON
// ----------------------------------------------
namespace GW2_SCT {

    void to_json(nlohmann::json& j, const message_receiver_options_struct& p) {
        j = nlohmann::json{};
        j["name"] = p.name;
        j["category"] = static_cast<int>(p.category);
        j["type"] = static_cast<int>(p.type);
        j["enabled"] = p.enabled;
        j["outputTemplate"] = p.outputTemplate;
        j["color"] = p.color;
        j["font"] = p.font;
        j["fontSize"] = p.fontSize;
        j["assignedFilterSets"] = p.assignedFilterSets;
        j["filtersEnabled"] = p.filtersEnabled;
        j["thresholdsEnabled"] = p.thresholdsEnabled;
        j["damageThreshold"] = p.damageThreshold;
        j["healThreshold"] = p.healThreshold;
        j["absorbThreshold"] = p.absorbThreshold;
        j["thresholdRespectFilters"] = p.thresholdRespectFilters;
    }

    void from_json(const nlohmann::json& j, message_receiver_options_struct& p) {
        if (j.contains("name")) j.at("name").get_to(p.name);
        if (j.contains("category")) { int v{}; j.at("category").get_to(v); p.category = static_cast<MessageCategory>(v); }
        if (j.contains("type")) { int v{}; j.at("type").get_to(v); p.type = static_cast<MessageType>(v); }
        if (j.contains("enabled")) j.at("enabled").get_to(p.enabled);
        if (j.contains("outputTemplate")) j.at("outputTemplate").get_to(p.outputTemplate);
        if (j.contains("color")) j.at("color").get_to(p.color);
        if (j.contains("font")) j.at("font").get_to(p.font);
        if (j.contains("fontSize")) j.at("fontSize").get_to(p.fontSize);
        if (j.contains("assignedFilterSets")) j.at("assignedFilterSets").get_to(p.assignedFilterSets);
        if (j.contains("filtersEnabled")) j.at("filtersEnabled").get_to(p.filtersEnabled);
        if (j.contains("thresholdsEnabled")) j.at("thresholdsEnabled").get_to(p.thresholdsEnabled);
        if (j.contains("damageThreshold")) j.at("damageThreshold").get_to(p.damageThreshold);
        if (j.contains("healThreshold")) j.at("healThreshold").get_to(p.healThreshold);
        if (j.contains("absorbThreshold")) j.at("absorbThreshold").get_to(p.absorbThreshold);
        if (j.contains("thresholdRespectFilters")) j.at("thresholdRespectFilters").get_to(p.thresholdRespectFilters);
    }

    bool message_receiver_options_struct::isThresholdExceeded(std::shared_ptr<EventMessage> message, uint32_t skillId, const std::string& skillName, const SkillFilterManager& filterManager) const {
        auto globalOptions = Options::get();
        
        bool thresholdsActive = thresholdsEnabled || (globalOptions->globalThresholdsEnabled && !thresholdsEnabled);
        if (!thresholdsActive && !globalOptions->globalThresholdsEnabled) {
            return false;
        }

        bool respectFilters = thresholdsEnabled ? thresholdRespectFilters : globalOptions->globalThresholdRespectFilters;
        
        if (respectFilters && filtersEnabled && !assignedFilterSets.empty() && !isSkillFiltered(skillId, skillName, filterManager)) {
            return false;
        }

        ThresholdCategory category = getMessageThresholdCategory(message->getType());
        int32_t totalValue = message->getCombinedValue();

        int activeDamageThreshold = thresholdsEnabled ? damageThreshold : globalOptions->globalDamageThreshold;
        int activeHealThreshold = thresholdsEnabled ? healThreshold : globalOptions->globalHealThreshold;
        int activeAbsorbThreshold = thresholdsEnabled ? absorbThreshold : globalOptions->globalAbsorbThreshold;

        switch (category) {
            case ThresholdCategory::DAMAGE:
                return totalValue < activeDamageThreshold;
            case ThresholdCategory::HEAL:
                return totalValue < activeHealThreshold;
            case ThresholdCategory::ABSORB:
                return totalValue < activeAbsorbThreshold;
            case ThresholdCategory::OTHER:
            default:
                return totalValue < activeDamageThreshold;
        }
    }

    message_receiver_options_struct::ThresholdCategory message_receiver_options_struct::getMessageThresholdCategory(MessageType type) const {
        switch (type) {
            case MessageType::PHYSICAL:
            case MessageType::CRIT:
            case MessageType::BLEEDING:
            case MessageType::BURNING:
            case MessageType::POISON:
            case MessageType::CONFUSION:
            case MessageType::RETALIATION:
            case MessageType::TORMENT:
            case MessageType::DOT:
                return ThresholdCategory::DAMAGE;
                
            case MessageType::HEAL:
            case MessageType::HOT:
                return ThresholdCategory::HEAL;
                
            case MessageType::SHIELD_RECEIVE:
            case MessageType::SHIELD_REMOVE:
                return ThresholdCategory::ABSORB;
                
            default:
                return ThresholdCategory::OTHER;
        }
    }

} // namespace GW2_SCT
