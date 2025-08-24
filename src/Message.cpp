#include "Message.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include "Language.h"
#include "Options.h"

// Handler macros now use shared_ptr<const MessageData> views
#define COMBINE_FUNCTION(NAME) \
    std::function<bool(const GW2_SCT::DataVecView&, const GW2_SCT::DataVecView&)> NAME = \
    [](const GW2_SCT::DataVecView& srcData, const GW2_SCT::DataVecView& targetData)

#define PARAMETER_FUNCTION(NAME) \
    std::function<std::string(const GW2_SCT::DataVecView&)> NAME = \
    [](const GW2_SCT::DataVecView& data)

namespace GW2_SCT {

    // -----------------------------
    // Combine predicates
    // -----------------------------

    COMBINE_FUNCTION(combineFunctionSkillId) {
        if (!srcData.empty() && !targetData.empty()) {
            return srcData.front()->skillId == targetData.front()->skillId;
        }
        return false;
    };

    COMBINE_FUNCTION(combineFunctionEntityId) {
        if (!srcData.empty() && !targetData.empty()) {
            return srcData.front()->entityId == targetData.front()->entityId;
        }
        return false;
    };

    COMBINE_FUNCTION(combineFunctionOtherEntityId) {
        if (!srcData.empty() && !targetData.empty()) {
            return srcData.front()->otherEntityId == targetData.front()->otherEntityId;
        }
        return false;
    };

    // -----------------------------
    // Parameter functions
    // -----------------------------

    PARAMETER_FUNCTION(parameterFunctionValue) {
        int32_t value = 0;
        for (auto& i : data) value += i->value;
        return std::to_string(value);
    };

    PARAMETER_FUNCTION(parameterFunctionBuffValue) {
        int32_t value = 0;
        for (auto& i : data) value += i->buffValue;
        return std::to_string(value);
    };

    PARAMETER_FUNCTION(parameterFunctionNegativeValue) {
        int32_t value = 0;
        for (auto& i : data) value -= i->value;
        return std::to_string(value);
    };

    PARAMETER_FUNCTION(parameterFunctionOverstackValue) {
        int32_t value = 0;
        for (auto& i : data) value += i->overstack_value;
        return std::to_string(value);
    };

    PARAMETER_FUNCTION(parameterFunctionNegativeBuffValue) {
        int32_t value = 0;
        for (auto& i : data) value -= i->buffValue;
        return std::to_string(value);
    };

    PARAMETER_FUNCTION(parameterFunctionOverstackBuffValue) {
        int32_t value = 0;
        for (auto& i : data) value += i->overstack_value;
        return std::to_string(value);
    };

    PARAMETER_FUNCTION(parameterFunctionEntityName) {
        if (data.size() > 1) {
            std::string ret = data.front()->entityName ? std::string(data.front()->entityName) : std::string();
            for (auto& temp : data) {
                if (!temp->entityName || std::strcmp(temp->entityName, ret.c_str()) != 0) {
                    ret = std::string(langString(LanguageCategory::Message, LanguageKey::Multiple_Sources));
                    break;
                }
            }
            return ret;
        }
        if (data.size() == 1 && data.front()->entityName != nullptr) {
            return std::string(data.front()->entityName);
        }
        return std::string("");
    };

    PARAMETER_FUNCTION(parameterFunctionOtherEntityName) {
        if (!data.empty() && data.front()->otherEntityName != nullptr) {
            return std::string(data.front()->otherEntityName);
        }
        return std::string("");
    };

    PARAMETER_FUNCTION(parameterFunctionSkillName) {
        if (!data.empty() && data.front()->skillName != nullptr) {
            return std::string(data.front()->skillName);
        }
        return std::string("");
    };

    PARAMETER_FUNCTION(parameterFunctionSkillIcon) {
        if (Options::get()->skillIconsEnabled && !data.empty()) {
            return std::string("[icon=" + std::to_string(data.front()->skillId) + "][/icon]");
        }
        return std::string("");
    };

    PARAMETER_FUNCTION(parameterFunctionEntityProfessionName) {
        if (!data.empty()) {
            std::string professionName;
            switch (data.front()->entityProf)
            {
            case PROFESSION_GUARDIAN:
                professionName = std::string(Language::get(LanguageCategory::Option_UI, LanguageKey::Profession_Colors_Guardian));
                break;
            case PROFESSION_WARRIOR:
                professionName = std::string(Language::get(LanguageCategory::Option_UI, LanguageKey::Profession_Colors_Warrior));
                break;
            case PROFESSION_ENGINEER:
                professionName = std::string(Language::get(LanguageCategory::Option_UI, LanguageKey::Profession_Colors_Engineer));
                break;
            case PROFESSION_RANGER:
                professionName = std::string(Language::get(LanguageCategory::Option_UI, LanguageKey::Profession_Colors_Ranger));
                break;
            case PROFESSION_THIEF:
                professionName = std::string(Language::get(LanguageCategory::Option_UI, LanguageKey::Profession_Colors_Thief));
                break;
            case PROFESSION_ELEMENTALIST:
                professionName = std::string(Language::get(LanguageCategory::Option_UI, LanguageKey::Profession_Colors_Elementalist));
                break;
            case PROFESSION_MESMER:
                professionName = std::string(Language::get(LanguageCategory::Option_UI, LanguageKey::Profession_Colors_Mesmer));
                break;
            case PROFESSION_NECROMANCER:
                professionName = std::string(Language::get(LanguageCategory::Option_UI, LanguageKey::Profession_Colors_Necromancer));
                break;
            case PROFESSION_REVENANT:
                professionName = std::string(Language::get(LanguageCategory::Option_UI, LanguageKey::Profession_Colors_Revenant));
                break;
            default:
                professionName = std::string(Language::get(LanguageCategory::Option_UI, LanguageKey::Profession_Colors_Undetectable));
                break;
            }
            return professionName;
        }
        return std::string("");
    };

    PARAMETER_FUNCTION(parameterFunctionEntityProfessionColor) {
        std::string professionColor;
        if (!data.empty()) {
            switch (data.front()->entityProf) {
            case PROFESSION_GUARDIAN:    professionColor = Options::get()->professionColorGuardian;    break;
            case PROFESSION_WARRIOR:     professionColor = Options::get()->professionColorWarrior;     break;
            case PROFESSION_ENGINEER:    professionColor = Options::get()->professionColorEngineer;    break;
            case PROFESSION_RANGER:      professionColor = Options::get()->professionColorRanger;      break;
            case PROFESSION_THIEF:       professionColor = Options::get()->professionColorThief;       break;
            case PROFESSION_ELEMENTALIST:professionColor = Options::get()->professionColorElementalist; break;
            case PROFESSION_MESMER:      professionColor = Options::get()->professionColorMesmer;      break;
            case PROFESSION_NECROMANCER: professionColor = Options::get()->professionColorNecromancer; break;
            case PROFESSION_REVENANT:    professionColor = Options::get()->professionColorRevenant;    break;
            default:                     professionColor = Options::get()->professionColorDefault;     break;
            }
        }
        return professionColor;
    };

    // -----------------------------
    // Handler table
    // -----------------------------

    static MessageHandler makeOutHandler() {
        return MessageHandler(
            { combineFunctionSkillId },
            {
                { 'v', parameterFunctionNegativeValue },
                { 'n', parameterFunctionEntityName },
                { 's', parameterFunctionSkillName },
                { 'i', parameterFunctionSkillIcon }
            }
        );
    }

    static MessageHandler makeOutBuffHandler(std::function<std::string(const DataVecView&)> valFunc) {
        return MessageHandler(
            { combineFunctionSkillId },
            {
                { 'v', valFunc },
                { 'n', parameterFunctionEntityName },
                { 's', parameterFunctionSkillName },
                { 'i', parameterFunctionSkillIcon }
            }
        );
    }

    static MessageHandler makeInHandler() {
        return MessageHandler(
            { combineFunctionSkillId, combineFunctionEntityId },
            {
                { 'v', parameterFunctionNegativeValue },
                { 'n', parameterFunctionEntityName },
                { 's', parameterFunctionSkillName },
                { 'c', parameterFunctionEntityProfessionColor },
                { 'r', parameterFunctionEntityProfessionName },
                { 'i', parameterFunctionSkillIcon }
            }
        );
    }

    static MessageHandler makeInBuffHandler(std::function<std::string(const DataVecView&)> valFunc) {
        return MessageHandler(
            { combineFunctionSkillId, combineFunctionEntityId },
            {
                { 'v', valFunc },
                { 'n', parameterFunctionEntityName },
                { 's', parameterFunctionSkillName },
                { 'c', parameterFunctionEntityProfessionColor },
                { 'r', parameterFunctionEntityProfessionName },
                { 'i', parameterFunctionSkillIcon }
            }
        );
    }

    std::map<MessageCategory, std::map<MessageType, MessageHandler>> EventMessage::messageHandlers = {
        // --- PLAYER_OUT ---
        { MessageCategory::PLAYER_OUT, {
            { MessageType::PHYSICAL,       makeOutHandler() },
            { MessageType::CRIT,           makeOutHandler() },
            { MessageType::BLEEDING,       makeOutBuffHandler(parameterFunctionNegativeBuffValue) },
            { MessageType::BURNING,        makeOutBuffHandler(parameterFunctionNegativeBuffValue) },
            { MessageType::POISON,         makeOutBuffHandler(parameterFunctionNegativeBuffValue) },
            { MessageType::CONFUSION,      makeOutBuffHandler(parameterFunctionNegativeBuffValue) },
            { MessageType::RETALIATION,    makeOutBuffHandler(parameterFunctionNegativeBuffValue) },
            { MessageType::TORMENT,        makeOutBuffHandler(parameterFunctionNegativeBuffValue) },
            { MessageType::DOT,            makeOutBuffHandler(parameterFunctionNegativeBuffValue) },
            { MessageType::HEAL,           makeOutBuffHandler(parameterFunctionValue) },
            { MessageType::HOT,            makeOutBuffHandler(parameterFunctionBuffValue) },
            { MessageType::SHIELD_RECEIVE, makeOutBuffHandler(parameterFunctionOverstackBuffValue) },
            { MessageType::SHIELD_REMOVE,  makeOutBuffHandler(parameterFunctionOverstackValue) },
            { MessageType::BLOCK,          makeOutHandler() },
            { MessageType::EVADE,          makeOutHandler() },
            { MessageType::INVULNERABLE,   makeOutHandler() },
            { MessageType::MISS,           makeOutHandler() }
        }},

        // --- PLAYER_IN ---
        { MessageCategory::PLAYER_IN, {
            { MessageType::PHYSICAL,       makeInHandler() },
            { MessageType::CRIT,           makeInHandler() },
            { MessageType::BLEEDING,       makeInBuffHandler(parameterFunctionNegativeBuffValue) },
            { MessageType::BURNING,        makeInBuffHandler(parameterFunctionNegativeBuffValue) },
            { MessageType::POISON,         makeInBuffHandler(parameterFunctionNegativeBuffValue) },
            { MessageType::CONFUSION,      makeInBuffHandler(parameterFunctionNegativeBuffValue) },
            { MessageType::RETALIATION,    makeInBuffHandler(parameterFunctionNegativeBuffValue) },
            { MessageType::TORMENT,        makeInBuffHandler(parameterFunctionNegativeBuffValue) },
            { MessageType::DOT,            makeInBuffHandler(parameterFunctionNegativeBuffValue) },
            { MessageType::HEAL,           makeInBuffHandler(parameterFunctionValue) },
            { MessageType::HOT,            makeInBuffHandler(parameterFunctionBuffValue) },
            { MessageType::SHIELD_RECEIVE, makeInBuffHandler(parameterFunctionOverstackBuffValue) },
            { MessageType::SHIELD_REMOVE,  makeInBuffHandler(parameterFunctionOverstackValue) },
            { MessageType::BLOCK,          makeInHandler() },
            { MessageType::EVADE,          makeInHandler() },
            { MessageType::INVULNERABLE,   makeInHandler() },
            { MessageType::MISS,           makeInHandler() }
        }},

        // --- PET_OUT mirrors PLAYER_OUT ---
        { MessageCategory::PET_OUT, {
            { MessageType::PHYSICAL,       makeOutHandler() },
            { MessageType::CRIT,           makeOutHandler() },
            { MessageType::BLEEDING,       makeOutBuffHandler(parameterFunctionNegativeBuffValue) },
            { MessageType::BURNING,        makeOutBuffHandler(parameterFunctionNegativeBuffValue) },
            { MessageType::POISON,         makeOutBuffHandler(parameterFunctionNegativeBuffValue) },
            { MessageType::CONFUSION,      makeOutBuffHandler(parameterFunctionNegativeBuffValue) },
            { MessageType::RETALIATION,    makeOutBuffHandler(parameterFunctionNegativeBuffValue) },
            { MessageType::TORMENT,        makeOutBuffHandler(parameterFunctionNegativeBuffValue) },
            { MessageType::DOT,            makeOutBuffHandler(parameterFunctionNegativeBuffValue) },
            { MessageType::HEAL,           makeOutBuffHandler(parameterFunctionValue) },
            { MessageType::HOT,            makeOutBuffHandler(parameterFunctionBuffValue) },
            { MessageType::SHIELD_RECEIVE, makeOutBuffHandler(parameterFunctionOverstackBuffValue) },
            { MessageType::SHIELD_REMOVE,  makeOutBuffHandler(parameterFunctionOverstackValue) },
            { MessageType::BLOCK,          makeOutHandler() },
            { MessageType::EVADE,          makeOutHandler() },
            { MessageType::INVULNERABLE,   makeOutHandler() },
            { MessageType::MISS,           makeOutHandler() }
        }},

        // --- PET_IN mirrors PLAYER_IN ---
        { MessageCategory::PET_IN, {
            { MessageType::PHYSICAL,       makeInHandler() },
            { MessageType::CRIT,           makeInHandler() },
            { MessageType::BLEEDING,       makeInBuffHandler(parameterFunctionNegativeBuffValue) },
            { MessageType::BURNING,        makeInBuffHandler(parameterFunctionNegativeBuffValue) },
            { MessageType::POISON,         makeInBuffHandler(parameterFunctionNegativeBuffValue) },
            { MessageType::CONFUSION,      makeInBuffHandler(parameterFunctionNegativeBuffValue) },
            { MessageType::RETALIATION,    makeInBuffHandler(parameterFunctionNegativeBuffValue) },
            { MessageType::TORMENT,        makeInBuffHandler(parameterFunctionNegativeBuffValue) },
            { MessageType::DOT,            makeInBuffHandler(parameterFunctionNegativeBuffValue) },
            { MessageType::HEAL,           makeInBuffHandler(parameterFunctionValue) },
            { MessageType::HOT,            makeInBuffHandler(parameterFunctionBuffValue) },
            { MessageType::SHIELD_RECEIVE, makeInBuffHandler(parameterFunctionOverstackBuffValue) },
            { MessageType::SHIELD_REMOVE,  makeInBuffHandler(parameterFunctionOverstackValue) },
            { MessageType::BLOCK,          makeInHandler() },
            { MessageType::EVADE,          makeInHandler() },
            { MessageType::INVULNERABLE,   makeInHandler() },
            { MessageType::MISS,           makeInHandler() }
        }}
    };

    // -----------------------------
    // MessageHandler ctor
    // -----------------------------
    MessageHandler::MessageHandler(
        std::vector<std::function<bool(const DataVecView&, const DataVecView&)>> tryToCombineWithFunctions,
        std::map<char, std::function<std::string(const DataVecView&)>> parameterToStringFunctions
    ) : tryToCombineWithFunctions(std::move(tryToCombineWithFunctions)),
        parameterToStringFunctions(std::move(parameterToStringFunctions)) {
    }

    // -----------------------------
    // EventMessage
    // -----------------------------

    EventMessage::EventMessage(MessageCategory category, MessageType type, cbtevent* ev, ag* src, ag* dst, char* skillname)
        : category(category), type(type), timepoint(std::chrono::system_clock::now()) {
        switch (category) {
        case MessageCategory::PLAYER_OUT:
        case MessageCategory::PET_OUT:
            messageDatas.push_back(std::make_shared<MessageData>(ev, dst, src, skillname));
            break;
        case MessageCategory::PLAYER_IN:
        case MessageCategory::PET_IN:
            messageDatas.push_back(std::make_shared<MessageData>(ev, src, dst, skillname));
            break;
        default:
            LOG("GW2_SCT: Received message for unknown category.");
            break;
        }
    }

    EventMessage::EventMessage(MessageCategory category, MessageType type, cbtevent1* ev, ag* src, ag* dst, char* skillname)
        : category(category), type(type), timepoint(std::chrono::system_clock::now()) {
        switch (category) {
        case MessageCategory::PLAYER_OUT:
        case MessageCategory::PET_OUT:
            messageDatas.push_back(std::make_shared<MessageData>(ev, dst, src, skillname));
            break;
        case MessageCategory::PLAYER_IN:
        case MessageCategory::PET_IN:
            messageDatas.push_back(std::make_shared<MessageData>(ev, src, dst, skillname));
            break;
        default:
            LOG("GW2_SCT: Received message for unknown category.");
            break;
        }
    }

    EventMessage::EventMessage(MessageCategory category, MessageType type, std::shared_ptr<MessageData> data)
        : category(category), type(type), timepoint(std::chrono::system_clock::now()) {
        // Deep copy to avoid aliasing across EventMessage instances
        messageDatas.push_back(std::make_shared<MessageData>(*data));
    }

    std::string EventMessage::getStringForOptions(std::shared_ptr<message_receiver_options_struct> opt) {
        if (!opt) return "";

        if (messageDatas.empty()) {
            LOG("WARN: empty message");
            return "";
        }

        // Remove any accidental nulls (defensive)
        messageDatas.erase(
            std::remove_if(messageDatas.begin(), messageDatas.end(),
                [](const std::shared_ptr<MessageData>& p) { return !p; }),
            messageDatas.end()
        );

        // Build const view for handlers
        DataVecView view;
        view.reserve(messageDatas.size());
        for (auto& sp : messageDatas) view.push_back(sp);

        std::string outputTemplate = opt->outputTemplate;
        std::stringstream stm;
        stm << "[col=" << opt->color << "]";
        for (auto it = outputTemplate.begin(); it != outputTemplate.end(); ++it) {
            switch (*it) {
            case '%':
                ++it;
                if (it != outputTemplate.end() && *it == '%') {
                    stm << *it;
                }
                else if (it != outputTemplate.end()) {
                    auto cat = messageHandlers.find(category);
                    if (cat != messageHandlers.end()) {
                        auto typ = cat->second.find(type);
                        if (typ != cat->second.end()) {
                            auto pf = typ->second.parameterToStringFunctions.find(*it);
                            if (pf != typ->second.parameterToStringFunctions.end()) {
                                stm << pf->second(view);
                            }
                        }
                    }
                }
                break;
            default:
                stm << *it;
                break;
            }
        }
        if (messageDatas.size() > 1) {
            stm << " [[" << messageDatas.size() << " "
                << langString(LanguageCategory::Message, LanguageKey::Number_Of_Hits) << "]]";
        }
        stm << "[/col]";
        return stm.str();
    }

    std::shared_ptr<MessageData> EventMessage::getCopyOfFirstData() {
        if (!messageDatas.empty() && messageDatas.front()) {
            return std::make_shared<MessageData>(*messageDatas.front());
        }
        return {};
    }

    MessageCategory EventMessage::getCategory() { return category; }
    MessageType     EventMessage::getType() { return type; }
    bool            EventMessage::hasToBeFiltered() { return false; }
    std::chrono::system_clock::time_point EventMessage::getTimepoint() { return timepoint; }

    bool EventMessage::tryToCombineWith(std::shared_ptr<EventMessage> m) {
        if (!m) return false;
        if (m->category != category || m->type != type) return false;

        auto cat = messageHandlers.find(category);
        if (cat == messageHandlers.end()) return false;
        auto typ = cat->second.find(type);
        if (typ == cat->second.end()) return false;

        // Views
        DataVecView a, b;
        a.reserve(messageDatas.size());
        for (auto& sp : messageDatas) a.push_back(sp);
        b.reserve(m->messageDatas.size());
        for (auto& sp : m->messageDatas) b.push_back(sp);

        for (auto& fn : typ->second.tryToCombineWithFunctions) {
            if (!fn(a, b)) return false;
        }

        // Append deep copies (no aliasing)
        for (auto& src : m->messageDatas) {
            if (src) messageDatas.push_back(std::make_shared<MessageData>(*src));
        }
        return true;
    }

    // -----------------------------
    // MessageData impl (deep copy + cleanup)
    // -----------------------------

    namespace {
        inline char* dup_cstr(const char* s) {
            if (!s) return nullptr;
            size_t n = std::strlen(s) + 1;
            char* p = static_cast<char*>(std::malloc(n));
            if (p) std::memcpy(p, s, n);
            return p;
        }
    }

    MessageData::MessageData(cbtevent* ev, ag* entity, ag* otherEntity, const char* skillname) {
        skillName = dup_cstr(skillname);
        value = ev->value;
        buffValue = ev->buff_dmg;
        skillId = ev->skillid;

        if (entity) {
            entityId = entity->id;
            entityProf = entity->prof;
            if (entity->name) entityName = dup_cstr(entity->name);
        }
        if (otherEntity) {
            otherEntityId = otherEntity->id;
            otherEntityProf = otherEntity->prof;
            if (otherEntity->name) otherEntityName = dup_cstr(otherEntity->name);
        }
        hasToBeFiltered = false;
    }

    MessageData::MessageData(cbtevent1* ev, ag* entity, ag* otherEntity, const char* skillname) {
        skillName = dup_cstr(skillname);
        value = ev->value;
        overstack_value = ev->overstack_value;
        buffValue = ev->buff_dmg;
        skillId = ev->skillid;

        if (entity) {
            entityId = entity->id;
            entityProf = entity->prof;
            if (entity->name) entityName = dup_cstr(entity->name);
        }
        if (otherEntity) {
            otherEntityId = otherEntity->id;
            otherEntityProf = otherEntity->prof;
            if (otherEntity->name) otherEntityName = dup_cstr(otherEntity->name);
        }
        hasToBeFiltered = false;
    }

#ifdef _DEBUG
    MessageData::MessageData(int32_t value, int32_t buffValue, uint32_t overstack_value, uint32_t skillId,
        ag* entity, ag* otherEntity, const char* skillname)
        : value(value), overstack_value(overstack_value), buffValue(buffValue), skillId(skillId) {
        skillName = dup_cstr(skillname);
        if (entity) {
            entityId = entity->id; entityProf = entity->prof;
            if (entity->name) entityName = dup_cstr(entity->name);
        }
        if (otherEntity) {
            otherEntityId = otherEntity->id; otherEntityProf = otherEntity->prof;
            if (otherEntity->name) otherEntityName = dup_cstr(otherEntity->name);
        }
        hasToBeFiltered = false;
    }
#endif

    MessageData::MessageData(const MessageData& o) {
        skillName = dup_cstr(o.skillName);
        entityName = dup_cstr(o.entityName);
        otherEntityName = dup_cstr(o.otherEntityName);
        value = o.value;
        overstack_value = o.overstack_value;
        buffValue = o.buffValue;
        skillId = o.skillId;
        entityId = o.entityId;
        entityProf = o.entityProf;
        otherEntityId = o.otherEntityId;
        otherEntityProf = o.otherEntityProf;
        hasToBeFiltered = o.hasToBeFiltered;
    }

    MessageData::~MessageData() {
        if (skillName)       std::free(skillName);
        if (entityName)      std::free(entityName);
        if (otherEntityName) std::free(otherEntityName);
        skillName = entityName = otherEntityName = nullptr;
    }

} // namespace GW2_SCT
