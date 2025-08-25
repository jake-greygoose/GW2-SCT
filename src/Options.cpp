#include "Options.h"
#include <sstream>
#include <iomanip>
#include <vector>
#include <regex>
#include "json.hpp"
#include "Common.h"
#include "SimpleIni.h"
#include "imgui.h"
#include "imgui_stdlib.h"
#include "imgui_sct_widgets.h"
#include "TemplateInterpreter.h"
#include "Language.h"
#include "SkillFilterStructures.h"

const char* TextAlignTexts[] = { langStringG(GW2_SCT::LanguageKey::Text_Align_Left), langStringG(GW2_SCT::LanguageKey::Text_Align_Center), langStringG(GW2_SCT::LanguageKey::Text_Align_Right) };
const char* TextCurveTexts[] = { langStringG(GW2_SCT::LanguageKey::Text_Curve_Left), langStringG(GW2_SCT::LanguageKey::Text_Curve_Straight), langStringG(GW2_SCT::LanguageKey::Text_Curve_Right) };

GW2_SCT::options_struct GW2_SCT::Options::options;
ObservableValue<std::shared_ptr<GW2_SCT::profile_options_struct>> GW2_SCT::Options::profile = std::shared_ptr<GW2_SCT::profile_options_struct>();
bool GW2_SCT::Options::windowIsOpen = false;
std::string GW2_SCT::Options::fontSelectionString = "";
std::string GW2_SCT::Options::fontSelectionStringWithMaster = "";
std::string GW2_SCT::Options::fontSizeTypeSelectionString = "";
std::string GW2_SCT::Options::skillFilterTypeSelectionString = "";

const std::string defaultProfileName = "default";

std::string GW2_SCT::Options::currentProfileName = defaultProfileName;
std::string GW2_SCT::Options::currentCharacterName = "";

std::chrono::steady_clock::time_point GW2_SCT::Options::lastSaveRequest = std::chrono::steady_clock::now();
bool GW2_SCT::Options::saveRequested = false;
const std::chrono::milliseconds GW2_SCT::Options::SAVE_DELAY(500);

const std::map<GW2_SCT::MessageCategory, std::string> messageCategorySections = {
	{ GW2_SCT::MessageCategory::PLAYER_OUT, "Messages_Player_Out" },
	{ GW2_SCT::MessageCategory::PLAYER_IN, "Messages_Player_In" },
	{ GW2_SCT::MessageCategory::PET_OUT, "Messages_Pet_Out" },
	{ GW2_SCT::MessageCategory::PET_IN, "Messages_Pet_In" }
};


namespace GW2_SCT {
	std::map<char, std::string> mapParameterListToLanguage(const char* section, std::vector<char> list) {
		std::map<char, std::string> result;
		for (auto p : list) {
			result.insert({ p, std::string(langStringImguiSafe(GetLanguageCategoryValue(section), GetLanguageKeyValue((std::string("Parameter_Description_") + p).c_str()))) });
		}
		return result;
	}
}

#define P99_PROTECT(...) __VA_ARGS__
#define ReceiverInformation(Category, CategoryLanguage, Type, TypeLanguage, IniName, CategoryAndTypeLanguage, PossibileParameters, Enabled, Color, Font, FontSize) \
		{ Type, { \
			IniName, \
			GW2_SCT::mapParameterListToLanguage(#CategoryAndTypeLanguage, PossibileParameters), \
			GW2_SCT::message_receiver_options_struct{ \
				std::string(langString(GW2_SCT::LanguageCategory::Option_UI, GW2_SCT::LanguageKey::CategoryLanguage)) + " - " + std::string(langString(GW2_SCT::LanguageCategory::Option_UI, GW2_SCT::LanguageKey::TypeLanguage)), \
				Category, \
				Type, \
				Enabled, \
				std::string(langString(GW2_SCT::LanguageCategory::CategoryAndTypeLanguage, GW2_SCT::LanguageKey::Default_Value)), \
				std::string(Color), \
				Font, \
				FontSize \
			} \
		} }

namespace GW2_SCT {
	const std::map<MessageCategory, const std::map<MessageType, receiver_information>> receiverInformationPerCategoryAndType = {
		{ MessageCategory::PLAYER_OUT, {
			ReceiverInformation(MessageCategory::PLAYER_OUT, Messages_Category_Player_Out, MessageType::PHYSICAL, Messages_Type_Physical, "Message_Player_Outgoing_Physical", Message_Player_Out_Physical, P99_PROTECT({'v', 'n', 's', 'i', 'd'}), true, "FFFFFF", 0, -1.f),
			ReceiverInformation(MessageCategory::PLAYER_OUT, Messages_Category_Player_Out, MessageType::CRIT, Messages_Type_Crit, "Message_Player_Outgoing_Crit", Message_Player_Out_Crit, P99_PROTECT({ 'v', 'n', 's', 'i', 'd' }), true, "FFFFFF", 0, -2.f),
			ReceiverInformation(MessageCategory::PLAYER_OUT, Messages_Category_Player_Out, MessageType::BLEEDING, Messages_Type_Bleeding, "Message_Player_Outgoing_Bleeding", Message_Player_Out_Bleeding, P99_PROTECT({ 'v', 'n', 's', 'i', 'd' }), true, "E84B30", 0, -1.f),
			ReceiverInformation(MessageCategory::PLAYER_OUT, Messages_Category_Player_Out, MessageType::BURNING, Messages_Type_Burning, "Message_Player_Outgoing_Burning", Message_Player_Out_Burning, P99_PROTECT({ 'v', 'n', 's', 'i', 'd' }), true, "FF9E32", 0, -1.f),
			ReceiverInformation(MessageCategory::PLAYER_OUT, Messages_Category_Player_Out, MessageType::POISON, Messages_Type_Poison, "Message_Player_Outgoing_Poison", Message_Player_Out_Poison, P99_PROTECT({ 'v', 'n', 's', 'i', 'd' }), true, "00C400", 0, -1.f),
			ReceiverInformation(MessageCategory::PLAYER_OUT, Messages_Category_Player_Out, MessageType::CONFUSION, Messages_Type_Confusion, "Message_Player_Outgoing_Confusion", Message_Player_Out_Confusion, P99_PROTECT({ 'v', 'n', 's', 'i', 'd' }), true, "B243FF", 0, -1.f),
			ReceiverInformation(MessageCategory::PLAYER_OUT, Messages_Category_Player_Out, MessageType::RETALIATION, Messages_Type_Retaliation, "Message_Player_Outgoing_Retaliation", Message_Player_Out_Retaliation, P99_PROTECT({ 'v', 'n', 's', 'i', 'd' }), true, "FFED00", 0, -1.f),
			ReceiverInformation(MessageCategory::PLAYER_OUT, Messages_Category_Player_Out, MessageType::TORMENT, Messages_Type_Torment, "Message_Player_Outgoing_Torment", Message_Player_Out_Torment, P99_PROTECT({ 'v', 'n', 's', 'i', 'd' }), true, "24451F", 0, -1.f),
			ReceiverInformation(MessageCategory::PLAYER_OUT, Messages_Category_Player_Out, MessageType::DOT, Messages_Type_Dot, "Message_Player_Outgoing_DoT", Message_Player_Out_DoT, P99_PROTECT({ 'v', 'n', 's', 'i', 'd' }), true, "45CDFF", 0, -1.f),
			ReceiverInformation(MessageCategory::PLAYER_OUT, Messages_Category_Player_Out, MessageType::HEAL, Messages_Type_Heal, "Message_Player_Outgoing_Heal", Message_Player_Out_Heal, P99_PROTECT({ 'v', 'n', 's', 'i', 'd' }), true, "10FB10", 0, -1.f),
			ReceiverInformation(MessageCategory::PLAYER_OUT, Messages_Category_Player_Out, MessageType::HOT, Messages_Type_Hot, "Message_Player_Outgoing_HoT", Message_Player_Out_HoT, P99_PROTECT({ 'v', 'n', 's', 'i', 'd' }), true, "10FB10", 0, -1.f),
			ReceiverInformation(MessageCategory::PLAYER_OUT, Messages_Category_Player_Out, MessageType::SHIELD_RECEIVE, Messages_Type_Shield_Receive, "Message_Player_Outgoing_Shield_Got", Message_Player_Out_Shield_Got, P99_PROTECT({ 'v', 'n', 's', 'i', 'd' }), true, "FFFF00", 0, -1.f),
			ReceiverInformation(MessageCategory::PLAYER_OUT, Messages_Category_Player_Out, MessageType::SHIELD_REMOVE, Messages_Type_Shield_Remove, "Message_Player_Outgoing_Shield", Message_Player_Out_Shield, P99_PROTECT({ 'v', 'n', 's', 'i', 'd' }), true, "FFFF00", 0, -1.f),
			ReceiverInformation(MessageCategory::PLAYER_OUT, Messages_Category_Player_Out, MessageType::BLOCK, Messages_Type_Block, "Message_Player_Outgoing_Block", Message_Player_Out_Block, P99_PROTECT({ 'n', 's', 'i', 'd' }), true, "0000FF", 0, -1.f),
			ReceiverInformation(MessageCategory::PLAYER_OUT, Messages_Category_Player_Out, MessageType::EVADE, Messages_Type_Evade, "Message_Player_Outgoing_Evade", Message_Player_Out_Evade, P99_PROTECT({ 'n', 's', 'i', 'd' }), true, "0000FF", 0, -1.f),
			ReceiverInformation(MessageCategory::PLAYER_OUT, Messages_Category_Player_Out, MessageType::INVULNERABLE, Messages_Type_Invulnerable, "Message_Player_Outgoing_Invulnerable", Message_Player_Out_Invulnerable, P99_PROTECT({ 'n', 's', 'i', 'd' }), true, "0000FF", 0, -1.f),
			ReceiverInformation(MessageCategory::PLAYER_OUT, Messages_Category_Player_Out, MessageType::MISS, Messages_Type_Miss, "Message_Player_Outgoing_Miss", Message_Player_Out_Miss, P99_PROTECT({ 'n', 's', 'i', 'd' }), true, "0000FF", 0, -1.f)
		} },
		{ MessageCategory::PLAYER_IN, {
			ReceiverInformation(MessageCategory::PLAYER_IN, Messages_Category_Player_In, MessageType::PHYSICAL, Messages_Type_Physical, "Message_Player_Incoming_Physical", Message_Player_In_Physical, P99_PROTECT({ 'v', 'n', 's', 'c', 'r', 'i', 'd' }), true, "FF0000", 0, -1.f),
			ReceiverInformation(MessageCategory::PLAYER_IN, Messages_Category_Player_In, MessageType::CRIT, Messages_Type_Crit, "Message_Player_Incoming_Crit", Message_Player_In_Crit, P99_PROTECT({ 'v', 'n', 's', 'c', 'r', 'i', 'd' }), true, "FF0000", 0, -2.f),
			ReceiverInformation(MessageCategory::PLAYER_IN, Messages_Category_Player_In, MessageType::BLEEDING, Messages_Type_Bleeding, "Message_Player_Incoming_Bleeding", Message_Player_In_Bleeding, P99_PROTECT({ 'v', 'n', 's', 'c', 'r', 'i', 'd' }), true, "FF0000", 0, -1.f),
			ReceiverInformation(MessageCategory::PLAYER_IN, Messages_Category_Player_In, MessageType::BURNING, Messages_Type_Burning, "Message_Player_Incoming_Burning", Message_Player_In_Burning, P99_PROTECT({ 'v', 'n', 's', 'c', 'r', 'i', 'd' }), true, "FF0000", 0, -1.f),
			ReceiverInformation(MessageCategory::PLAYER_IN, Messages_Category_Player_In, MessageType::POISON, Messages_Type_Poison, "Message_Player_Incoming_Poison", Message_Player_In_Poison, P99_PROTECT({ 'v', 'n', 's', 'c', 'r', 'i', 'd' }), true, "FF0000", 0, -1.f),
			ReceiverInformation(MessageCategory::PLAYER_IN, Messages_Category_Player_In, MessageType::CONFUSION, Messages_Type_Confusion, "Message_Player_Incoming_Confusion", Message_Player_In_Confusion, P99_PROTECT({ 'v', 'n', 's', 'c', 'r', 'i', 'd' }), true, "FF0000", 0, -1.f),
			ReceiverInformation(MessageCategory::PLAYER_IN, Messages_Category_Player_In, MessageType::RETALIATION, Messages_Type_Retaliation, "Message_Player_Incoming_Retaliation", Message_Player_In_Retaliation, P99_PROTECT({ 'v', 'n', 's', 'c', 'r', 'i', 'd' }), true, "FF0000", 0, -1.f),
			ReceiverInformation(MessageCategory::PLAYER_IN, Messages_Category_Player_In, MessageType::TORMENT, Messages_Type_Torment, "Message_Player_Incoming_Torment", Message_Player_In_Torment, P99_PROTECT({ 'v', 'n', 's', 'c', 'r', 'i', 'd' }), true, "FF0000", 0, -1.f),
			ReceiverInformation(MessageCategory::PLAYER_IN, Messages_Category_Player_In, MessageType::DOT, Messages_Type_Dot, "Message_Player_Incoming_DoT", Message_Player_In_DoT, P99_PROTECT({ 'v', 'n', 's', 'c', 'r', 'i', 'd' }), true, "FF0000", 0, -1.f),
			ReceiverInformation(MessageCategory::PLAYER_IN, Messages_Category_Player_In, MessageType::HEAL, Messages_Type_Heal, "Message_Player_Incoming_Heal", Message_Player_In_Heal, P99_PROTECT({ 'v', 'n', 's', 'c', 'r', 'i', 'd' }), true, "10FB10", 0, -1.f),
			ReceiverInformation(MessageCategory::PLAYER_IN, Messages_Category_Player_In, MessageType::HOT, Messages_Type_Hot, "Message_Player_Incoming_HoT", Message_Player_In_HoT, P99_PROTECT({ 'v', 'n', 's', 'c', 'r', 'i', 'd' }), true, "10FB10", 0, -1.f),
			ReceiverInformation(MessageCategory::PLAYER_IN, Messages_Category_Player_In, MessageType::SHIELD_RECEIVE, Messages_Type_Shield_Receive, "Message_Player_Incoming_Shield_Got", Message_Player_In_Shield_Got, P99_PROTECT({ 'v', 'n', 's', 'c', 'r', 'i', 'd' }), true, "FFFF00", 0, -1.f),
			ReceiverInformation(MessageCategory::PLAYER_IN, Messages_Category_Player_In, MessageType::SHIELD_REMOVE, Messages_Type_Shield_Remove, "Message_Player_Incoming_Shield", Message_Player_In_Shield, P99_PROTECT({ 'v', 'n', 's', 'c', 'r', 'i', 'd' }), true, "FFFF00", 0, -1.f),
			ReceiverInformation(MessageCategory::PLAYER_IN, Messages_Category_Player_In, MessageType::BLOCK, Messages_Type_Block, "Message_Player_Incoming_Block", Message_Player_In_Block, P99_PROTECT({ 'n', 's', 'c', 'r', 'i', 'd' }), true, "0000FF", 0, -1.f),
			ReceiverInformation(MessageCategory::PLAYER_IN, Messages_Category_Player_In, MessageType::EVADE, Messages_Type_Evade, "Message_Player_Incoming_Evade", Message_Player_In_Evade, P99_PROTECT({ 'n', 's', 'c', 'r', 'i', 'd' }), true, "0000FF", 0, -1.f),
			ReceiverInformation(MessageCategory::PLAYER_IN, Messages_Category_Player_In, MessageType::INVULNERABLE, Messages_Type_Invulnerable, "Message_Player_Incoming_Invulnerable", Message_Player_In_Invulnerable, P99_PROTECT({ 'n', 's', 'c', 'r', 'i', 'd' }), true, "0000FF", 0, -1.f),
			ReceiverInformation(MessageCategory::PLAYER_IN, Messages_Category_Player_In, MessageType::MISS, Messages_Type_Miss, "Message_Player_Incoming_Miss", Message_Player_In_Miss, P99_PROTECT({ 'n', 's', 'c', 'r', 'i', 'd' }), true, "0000FF", 0, -1.f)
		} },
		{ MessageCategory::PET_OUT, {
			ReceiverInformation(MessageCategory::PET_OUT, Messages_Category_Pet_Out, MessageType::PHYSICAL, Messages_Type_Physical, "Message_Pet_Outgoing_Physical", Message_Pet_Out_Physical, P99_PROTECT({ 'v', 'n', 's', 'p', 'i', 'd' }), true, "80FB80", 0, -1.f),
			ReceiverInformation(MessageCategory::PET_OUT, Messages_Category_Pet_Out, MessageType::CRIT, Messages_Type_Crit, "Message_Pet_Outgoing_Crit", Message_Pet_Out_Crit, P99_PROTECT({ 'v', 'n', 's', 'p', 'i', 'd' }), true, "80FB80", 0, -2.f),
			ReceiverInformation(MessageCategory::PET_OUT, Messages_Category_Pet_Out, MessageType::BLEEDING, Messages_Type_Bleeding, "Message_Pet_Outgoing_Bleeding", Message_Pet_Out_Bleeding, P99_PROTECT({ 'v', 'n', 's', 'p', 'i', 'd' }), true, "80FB80", 0, -1.f),
			ReceiverInformation(MessageCategory::PET_OUT, Messages_Category_Pet_Out, MessageType::BURNING, Messages_Type_Burning, "Message_Pet_Outgoing_Burning", Message_Pet_Out_Burning, P99_PROTECT({ 'v', 'n', 's', 'p', 'i', 'd' }), true, "80FB80", 0, -1.f),
			ReceiverInformation(MessageCategory::PET_OUT, Messages_Category_Pet_Out, MessageType::POISON, Messages_Type_Poison, "Message_Pet_Outgoing_Poison", Message_Pet_Out_Poison, P99_PROTECT({ 'v', 'n', 's', 'p', 'i', 'd' }), true, "80FB80", 0, -1.f),
			ReceiverInformation(MessageCategory::PET_OUT, Messages_Category_Pet_Out, MessageType::CONFUSION, Messages_Type_Confusion, "Message_Pet_Outgoing_Confusion", Message_Pet_Out_Confusion, P99_PROTECT({ 'v', 'n', 's', 'p', 'i', 'd' }), true, "80FB80", 0, -1.f),
			ReceiverInformation(MessageCategory::PET_OUT, Messages_Category_Pet_Out, MessageType::RETALIATION, Messages_Type_Retaliation, "Message_Pet_Outgoing_Retaliation", Message_Pet_Out_Retaliation, P99_PROTECT({ 'v', 'n', 's', 'p', 'i', 'd' }), true, "80FB80", 0, -1.f),
			ReceiverInformation(MessageCategory::PET_OUT, Messages_Category_Pet_Out, MessageType::TORMENT, Messages_Type_Torment, "Message_Pet_Outgoing_Torment", Message_Pet_Out_Torment, P99_PROTECT({ 'v', 'n', 's', 'p', 'i', 'd' }), true, "80FB80", 0, -1.f),
			ReceiverInformation(MessageCategory::PET_OUT, Messages_Category_Pet_Out, MessageType::DOT, Messages_Type_Dot, "Message_Pet_Outgoing_DoT", Message_Pet_Out_DoT, P99_PROTECT({ 'v', 'n', 's', 'p', 'i', 'd' }), true, "80FB80", 0, -1.f),
			ReceiverInformation(MessageCategory::PET_OUT, Messages_Category_Pet_Out, MessageType::HEAL, Messages_Type_Heal, "Message_Pet_Outgoing_Heal", Message_Pet_Out_Heal, P99_PROTECT({ 'v', 'n', 's', 'p', 'i', 'd' }), true, "80FB80", 0, -1.f),
			ReceiverInformation(MessageCategory::PET_OUT, Messages_Category_Pet_Out, MessageType::HOT, Messages_Type_Hot, "Message_Pet_Outgoing_HoT", Message_Pet_Out_HoT, P99_PROTECT({ 'v', 'n', 's', 'p', 'i', 'd' }), true, "80FB80", 0, -1.f),
			ReceiverInformation(MessageCategory::PET_OUT, Messages_Category_Pet_Out, MessageType::SHIELD_RECEIVE, Messages_Type_Shield_Receive, "Message_Pet_Outgoing_Shield_Got", Message_Pet_Out_Shield_Got, P99_PROTECT({ 'v', 'n', 's', 'p', 'i', 'd' }), true, "FFFF00", 0, -1.f),
			ReceiverInformation(MessageCategory::PET_OUT, Messages_Category_Pet_Out, MessageType::SHIELD_REMOVE, Messages_Type_Shield_Remove, "Message_Pet_Outgoing_Shield", Message_Pet_Out_Shield, P99_PROTECT({ 'v', 'n', 's', 'p', 'i', 'd' }), false, "FFFF00", 0, -1.f),
			ReceiverInformation(MessageCategory::PET_OUT, Messages_Category_Pet_Out, MessageType::BLOCK, Messages_Type_Block, "Message_Pet_Outgoing_Block", Message_Pet_Out_Block, P99_PROTECT({ 'n', 's', 'p', 'i', 'd' }), false, "0000FF", 0, -1.f),
			ReceiverInformation(MessageCategory::PET_OUT, Messages_Category_Pet_Out, MessageType::EVADE, Messages_Type_Evade, "Message_Pet_Outgoing_Evade", Message_Pet_Out_Evade, P99_PROTECT({ 'n', 's', 'p', 'i', 'd' }), false, "0000FF", 0, -1.f),
			ReceiverInformation(MessageCategory::PET_OUT, Messages_Category_Pet_Out, MessageType::INVULNERABLE, Messages_Type_Invulnerable, "Message_Pet_Outgoing_Invulnerable", Message_Pet_Out_Invulnerable, P99_PROTECT({ 'n', 's', 'p', 'i', 'd' }), false, "0000FF", 0, -1.f),
			ReceiverInformation(MessageCategory::PET_OUT, Messages_Category_Pet_Out, MessageType::MISS, Messages_Type_Miss, "Message_Pet_Outgoing_Miss", Message_Pet_Out_Miss, P99_PROTECT({ 'n', 's', 'p', 'i', 'd' }), false, "0000FF", 0, -1.f)
		} },
		{ MessageCategory::PET_IN, {
			ReceiverInformation(MessageCategory::PET_IN, Messages_Category_Pet_In, MessageType::PHYSICAL, Messages_Type_Physical, "Message_Pet_Incoming_Physical", Message_Pet_In_Physical, P99_PROTECT({ 'v', 'n', 's', 'p', 'c', 'r', 'i', 'd' }), false, "80FB80", 0, -1.f),
			ReceiverInformation(MessageCategory::PET_IN, Messages_Category_Pet_In, MessageType::CRIT, Messages_Type_Crit, "Message_Pet_Incoming_Crit", Message_Pet_In_Crit, P99_PROTECT({ 'v', 'n', 's', 'p', 'c', 'r', 'i', 'd' }), false, "80FB80", 0, -2.f),
			ReceiverInformation(MessageCategory::PET_IN, Messages_Category_Pet_In, MessageType::BLEEDING, Messages_Type_Bleeding, "Message_Pet_Incoming_Bleeding", Message_Pet_In_Bleeding, P99_PROTECT({ 'v', 'n', 's', 'p', 'c', 'r', 'i', 'd' }), false, "80FB80", 0, -1.f),
			ReceiverInformation(MessageCategory::PET_IN, Messages_Category_Pet_In, MessageType::BURNING, Messages_Type_Burning, "Message_Pet_Incoming_Burning", Message_Pet_In_Burning, P99_PROTECT({ 'v', 'n', 's', 'p', 'c', 'r', 'i', 'd' }), false, "80FB80", 0, -1.f),
			ReceiverInformation(MessageCategory::PET_IN, Messages_Category_Pet_In, MessageType::POISON, Messages_Type_Poison, "Message_Pet_Incoming_Poison", Message_Pet_In_Poison, P99_PROTECT({ 'v', 'n', 's', 'p', 'c', 'r', 'i', 'd' }), false, "80FB80", 0, -1.f),
			ReceiverInformation(MessageCategory::PET_IN, Messages_Category_Pet_In, MessageType::CONFUSION, Messages_Type_Confusion, "Message_Pet_Incoming_Confusion", Message_Pet_In_Confusion, P99_PROTECT({ 'v', 'n', 's', 'p', 'c', 'r', 'i', 'd' }), false, "80FB80", 0, -1.f),
			ReceiverInformation(MessageCategory::PET_IN, Messages_Category_Pet_In, MessageType::RETALIATION, Messages_Type_Retaliation, "Message_Pet_Incoming_Retaliation", Message_Pet_In_Retaliation, P99_PROTECT({ 'v', 'n', 's', 'p', 'c', 'r', 'i', 'd' }), false, "80FB80", 0, -1.f),
			ReceiverInformation(MessageCategory::PET_IN, Messages_Category_Pet_In, MessageType::TORMENT, Messages_Type_Torment, "Message_Pet_Incoming_Torment", Message_Pet_In_Torment, P99_PROTECT({ 'v', 'n', 's', 'p', 'c', 'r', 'i', 'd' }), false, "80FB80", 0, -1.f),
			ReceiverInformation(MessageCategory::PET_IN, Messages_Category_Pet_In, MessageType::DOT, Messages_Type_Dot, "Message_Pet_Incoming_DoT", Message_Pet_In_DoT, P99_PROTECT({ 'v', 'n', 's', 'p', 'c', 'r', 'i', 'd' }), false, "80FB80", 0, -1.f),
			ReceiverInformation(MessageCategory::PET_IN, Messages_Category_Pet_In, MessageType::HEAL, Messages_Type_Heal, "Message_Pet_Incoming_Heal", Message_Pet_In_Heal, P99_PROTECT({ 'v', 'n', 's', 'p', 'c', 'r', 'i', 'd' }), true, "80FB80", 0, -1.f),
			ReceiverInformation(MessageCategory::PET_IN, Messages_Category_Pet_In, MessageType::HOT, Messages_Type_Hot, "Message_Pet_Incoming_HoT", Message_Pet_In_HoT, P99_PROTECT({ 'v', 'n', 's', 'p', 'c', 'r', 'i', 'd' }), true, "80FB80", 0, -1.f),
			ReceiverInformation(MessageCategory::PET_IN, Messages_Category_Pet_In, MessageType::SHIELD_RECEIVE, Messages_Type_Shield_Receive, "Message_Pet_Incoming_Shield_Got", Message_Pet_In_Shield_Got, P99_PROTECT({ 'v', 'n', 's', 'p', 'c', 'r', 'i', 'd' }), true, "80FB80", 0, -1.f),
			ReceiverInformation(MessageCategory::PET_IN, Messages_Category_Pet_In, MessageType::SHIELD_REMOVE, Messages_Type_Shield_Remove, "Message_Pet_Incoming_Shield", Message_Pet_In_Shield, P99_PROTECT({ 'v', 'n', 's', 'p', 'c', 'r', 'i', 'd' }), false, "80FB80", 0, -1.f),
			ReceiverInformation(MessageCategory::PET_IN, Messages_Category_Pet_In, MessageType::BLOCK, Messages_Type_Block, "Message_Pet_Incoming_Block", Message_Pet_In_Block, P99_PROTECT({ 'n', 's', 'p', 'c', 'r', 'i', 'd' }), false, "80FB80", 0, -1.f),
			ReceiverInformation(MessageCategory::PET_IN, Messages_Category_Pet_In, MessageType::EVADE, Messages_Type_Evade, "Message_Pet_Incoming_Evade", Message_Pet_In_Evade, P99_PROTECT({ 'n', 's', 'p', 'c', 'r', 'i', 'd' }), false, "80FB80", 0, -1.f),
			ReceiverInformation(MessageCategory::PET_IN, Messages_Category_Pet_In, MessageType::INVULNERABLE, Messages_Type_Invulnerable, "Message_Pet_Incoming_Invulnerable", Message_Pet_In_Invulnerable, P99_PROTECT({ 'n', 's', 'p', 'c', 'r', 'i', 'd' }), false, "80FB80", 0, -1.f),
			ReceiverInformation(MessageCategory::PET_IN, Messages_Category_Pet_In, MessageType::MISS, Messages_Type_Miss, "Message_Pet_Incoming_Miss", Message_Pet_In_Miss, P99_PROTECT({ 'n', 's', 'p', 'c', 'r', 'i', 'd' }), false, "80FB80", 0, -1.f)
		} }
	};


	const std::map<MessageCategory, std::string> categoryNames = {
		{ MessageCategory::PLAYER_OUT, std::string(langString(GW2_SCT::LanguageCategory::Option_UI, GW2_SCT::LanguageKey::Messages_Category_Player_Out)) },
		{ MessageCategory::PLAYER_IN, std::string(langString(GW2_SCT::LanguageCategory::Option_UI, GW2_SCT::LanguageKey::Messages_Category_Player_In)) },
		{ MessageCategory::PET_OUT, std::string(langString(GW2_SCT::LanguageCategory::Option_UI, GW2_SCT::LanguageKey::Messages_Category_Pet_Out)) },
		{ MessageCategory::PET_IN, std::string(langString(GW2_SCT::LanguageCategory::Option_UI, GW2_SCT::LanguageKey::Messages_Category_Pet_In)) }
	};

	const std::map<MessageType, std::string> typeNames = {
		{ MessageType::PHYSICAL, std::string(langString(GW2_SCT::LanguageCategory::Option_UI, GW2_SCT::LanguageKey::Messages_Type_Physical)) },
		{ MessageType::CRIT, std::string(langString(GW2_SCT::LanguageCategory::Option_UI, GW2_SCT::LanguageKey::Messages_Type_Crit)) },
		{ MessageType::BLEEDING, std::string(langString(GW2_SCT::LanguageCategory::Option_UI, GW2_SCT::LanguageKey::Messages_Type_Bleeding)) },
		{ MessageType::BURNING, std::string(langString(GW2_SCT::LanguageCategory::Option_UI, GW2_SCT::LanguageKey::Messages_Type_Burning)) },
		{ MessageType::POISON, std::string(langString(GW2_SCT::LanguageCategory::Option_UI, GW2_SCT::LanguageKey::Messages_Type_Poison)) },
		{ MessageType::CONFUSION, std::string(langString(GW2_SCT::LanguageCategory::Option_UI, GW2_SCT::LanguageKey::Messages_Type_Confusion)) },
		{ MessageType::RETALIATION, std::string(langString(GW2_SCT::LanguageCategory::Option_UI, GW2_SCT::LanguageKey::Messages_Type_Retaliation)) },
		{ MessageType::TORMENT, std::string(langString(GW2_SCT::LanguageCategory::Option_UI, GW2_SCT::LanguageKey::Messages_Type_Torment)) },
		{ MessageType::DOT, std::string(langString(GW2_SCT::LanguageCategory::Option_UI, GW2_SCT::LanguageKey::Messages_Type_Dot)) },
		{ MessageType::HEAL, std::string(langString(GW2_SCT::LanguageCategory::Option_UI, GW2_SCT::LanguageKey::Messages_Type_Heal)) },
		{ MessageType::HOT, std::string(langString(GW2_SCT::LanguageCategory::Option_UI, GW2_SCT::LanguageKey::Messages_Type_Hot)) },
		{ MessageType::SHIELD_RECEIVE, std::string(langString(GW2_SCT::LanguageCategory::Option_UI, GW2_SCT::LanguageKey::Messages_Type_Shield_Receive)) },
		{ MessageType::SHIELD_REMOVE, std::string(langString(GW2_SCT::LanguageCategory::Option_UI, GW2_SCT::LanguageKey::Messages_Type_Shield_Remove)) },
		{ MessageType::BLOCK, std::string(langString(GW2_SCT::LanguageCategory::Option_UI, GW2_SCT::LanguageKey::Messages_Type_Block)) },
		{ MessageType::EVADE, std::string(langString(GW2_SCT::LanguageCategory::Option_UI, GW2_SCT::LanguageKey::Messages_Type_Evade)) },
		{ MessageType::INVULNERABLE, std::string(langString(GW2_SCT::LanguageCategory::Option_UI, GW2_SCT::LanguageKey::Messages_Type_Invulnerable)) },
		{ MessageType::MISS, std::string(langString(GW2_SCT::LanguageCategory::Option_UI, GW2_SCT::LanguageKey::Messages_Type_Miss)) }
	};

	const std::map<SkillIconDisplayType, std::string> skillIconsDisplayTypeNames = {
		{ SkillIconDisplayType::NORMAL, std::string(langString(GW2_SCT::LanguageCategory::Skill_Icons_Option_UI, GW2_SCT::LanguageKey::Skill_Icons_Display_Type_Normal)) },
		{ SkillIconDisplayType::BLACK_CULLED, std::string(langString(GW2_SCT::LanguageCategory::Skill_Icons_Option_UI, GW2_SCT::LanguageKey::Skill_Icons_Display_Type_Black_Culled)) },
		{ SkillIconDisplayType::BORDER_BLACK_CULLED, std::string(langString(GW2_SCT::LanguageCategory::Skill_Icons_Option_UI, GW2_SCT::LanguageKey::Skill_Icons_Display_Type_Black_Border_Culled)) },
		{ SkillIconDisplayType::BORDER_TOUCHING_BLACK_CULLED, std::string(langString(GW2_SCT::LanguageCategory::Skill_Icons_Option_UI, GW2_SCT::LanguageKey::Skill_Icons_Display_Type_Black_Border_Touching_Culled)) }
	};
}

void GW2_SCT::Options::open() {
	windowIsOpen = true;
}

void GW2_SCT::Options::paint() {

	processPendingSave();

	if (windowIsOpen) {
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.60f, 0.60f, 0.60f, 0.30f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
		ImGui::Begin(langString(LanguageCategory::Option_UI, LanguageKey::Title), &windowIsOpen, ImGuiWindowFlags_NoCollapse);

		ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
		if (ImGui::BeginTabBar("##options-tab-bar", tab_bar_flags))
		{
			if (ImGui::BeginTabItem(langString(LanguageCategory::Option_UI, LanguageKey::Menu_Bar_General))) {
				paintGeneral();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem(langString(LanguageCategory::Option_UI, LanguageKey::Menu_Bar_Scroll_Areas))) {
				paintScrollAreas();
				if (!windowIsOpen) {
					for (auto scrollAreaOptions : profile->scrollAreaOptions) {
						if (scrollAreaOptions->outlineState != ScrollAreaOutlineState::NONE) scrollAreaOptions->outlineState = ScrollAreaOutlineState::NONE;
					}
				}
				ImGui::EndTabItem();
			}
			else {
				for (auto scrollAreaOptions : profile->scrollAreaOptions) {
					if (scrollAreaOptions->outlineState != ScrollAreaOutlineState::NONE) scrollAreaOptions->outlineState = ScrollAreaOutlineState::NONE;
				}
			}
			if (ImGui::BeginTabItem(langString(LanguageCategory::Option_UI, LanguageKey::Menu_Bar_Profession_Colors))) {
				paintProfessionColors();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem(langString(LanguageCategory::Option_UI, LanguageKey::Menu_Bar_Filtered_Skills))) {
				paintSkillFilters();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem(langString(LanguageCategory::Option_UI, LanguageKey::Menu_Bar_Skill_Icons))) {
				paintSkillIcons();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem(langString(LanguageCategory::Option_UI, LanguageKey::Menu_Bar_Profiles))) {
				paintProfiles();
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}

		ImGui::End();
		ImGui::PopStyleVar();
		ImGui::PopStyleColor();
	}
}

std::string btos(bool b) {
	return b ? "1" : "0";
}

bool stob(std::string const& s) {
	return s != "0";
}

void GW2_SCT::Options::save() {
	std::ofstream out((getSCTPath() + "sct.json").c_str());
	nlohmann::json j = options;
	out << j.dump(2);
	out.close();
}

void GW2_SCT::Options::requestSave() {
	lastSaveRequest = std::chrono::steady_clock::now();
	saveRequested = true;
}

void GW2_SCT::Options::processPendingSave() {
	if (saveRequested) {
		auto now = std::chrono::steady_clock::now();
		if (now - lastSaveRequest >= SAVE_DELAY) {
			save();
			saveRequested = false;
			LOG("Auto-saved configuration");
		}
	}
}

void GW2_SCT::Options::load() {
	fontSelectionString = "";
	for (auto it = fontMap.begin(); it != fontMap.end(); ++it) {
		fontSelectionString += '\0' + it->second.first;
	}
	fontSelectionString += '\0';
	fontSelectionStringWithMaster = std::string("Master Font" + fontSelectionString);
	fontSelectionString.erase(fontSelectionString.begin());

	fontSizeTypeSelectionString = std::string(langStringG(LanguageKey::Default_Font_Size)) + '\0' + std::string(langStringG(LanguageKey::Default_Crit_Font_Size)) + '\0' + std::string(langStringG(LanguageKey::Custom_Font_Size)) + '\0';
	skillFilterTypeSelectionString = std::string(langStringG(LanguageKey::Skill_Filter_Type_ID)) + '\0' + std::string(langStringG(LanguageKey::Skill_Filter_Type_Name)) + '\0';

	std::string jsonFilename = getSCTPath() + "sct.json";
	if (file_exist(jsonFilename)) {
		LOG("Loading sct.json");
		std::string line, text;
		std::ifstream in(jsonFilename);
		while (std::getline(in, line)) {
			text += line + "\n";
		}
		in.close();
		try {
			nlohmann::json j = nlohmann::json::parse(text);
			options = j;
			if (options.profiles.find(defaultProfileName) == options.profiles.end()) {
				options.profiles[defaultProfileName];
			}

			currentProfileName = options.globalProfile;
			profile = options.profiles[currentProfileName];
			defaultFont = getFontType(profile->masterFont, false);
		}
		catch (std::exception e) {
			LOG("Error loading options: ", e.what());
			setDefault();
		}
	}
	else {
		setDefault();
	}
}

void GW2_SCT::Options::loadProfile(std::string characterName) {
	if (currentCharacterName != characterName) {
		currentCharacterName = characterName;
		auto profileMappingIterator = options.characterProfileMap.find(currentCharacterName);
		if (profileMappingIterator == options.characterProfileMap.end()) {
			if (currentProfileName != options.globalProfile) {
				currentProfileName = options.globalProfile;
				profile = options.profiles[currentProfileName];
			}
		}
		else {
			if (currentProfileName != profileMappingIterator->second) {
				currentProfileName = profileMappingIterator->second;
				profile = options.profiles[currentProfileName];
			}
		}
	}
}

namespace GW2_SCT {
	// Helper function to initialize a profile with default settings
	void initProfileWithDefaults(std::shared_ptr<profile_options_struct> p) {
		p->scrollAreaOptions.clear();
		auto incomingStruct = std::make_shared<scroll_area_options_struct>(scroll_area_options_struct{
			std::string(langStringG(LanguageKey::Default_Scroll_Area_Incoming)),
			-249.f,
			-25.f,
			40.f,
			260.f,
			TextAlign::RIGHT,
			TextCurve::LEFT,
			ScrollAreaOutlineState::NONE,
			{}
			});
		for (const auto& messageTypePair : receiverInformationPerCategoryAndType.at(MessageCategory::PLAYER_IN)) {
			incomingStruct->receivers.push_back(std::make_shared<message_receiver_options_struct>(messageTypePair.second.defaultReceiver));
		}
		for (const auto& messageTypePair : receiverInformationPerCategoryAndType.at(MessageCategory::PET_IN)) {
			incomingStruct->receivers.push_back(std::make_shared<message_receiver_options_struct>(messageTypePair.second.defaultReceiver));
		}
		p->scrollAreaOptions.push_back(incomingStruct);
		auto outgoingStruct = std::make_shared<scroll_area_options_struct>(scroll_area_options_struct{
			std::string(langStringG(LanguageKey::Default_Scroll_Area_Outgoing)),
			217.f,
			-25.f,
			40.f,
			260.f,
			TextAlign::LEFT,
			TextCurve::RIGHT,
			ScrollAreaOutlineState::NONE,
			{}
			});
		for (const auto& messageTypePair : receiverInformationPerCategoryAndType.at(MessageCategory::PLAYER_OUT)) {
			outgoingStruct->receivers.push_back(std::make_shared<message_receiver_options_struct>(messageTypePair.second.defaultReceiver));
		}
		for (const auto& messageTypePair : receiverInformationPerCategoryAndType.at(MessageCategory::PET_OUT)) {
			outgoingStruct->receivers.push_back(std::make_shared<message_receiver_options_struct>(messageTypePair.second.defaultReceiver));
		}
		p->scrollAreaOptions.push_back(outgoingStruct);
	}
}

void GW2_SCT::Options::setDefault() {
	currentProfileName = defaultProfileName;
	profile = options.profiles[defaultProfileName] = std::make_shared<profile_options_struct>();
	initProfileWithDefaults(profile);
}

bool GW2_SCT::Options::isOptionsWindowOpen() {
	return windowIsOpen;
}

void GW2_SCT::Options::paintGeneral() {
	if (ImGui::Checkbox(langString(LanguageCategory::Option_UI, LanguageKey::General_Enabled), &profile->sctEnabled)) {
		requestSave();
	}

	if (ImGui::ClampingDragFloat(langString(LanguageCategory::Option_UI, LanguageKey::General_Scrolling_Speed), &profile->scrollSpeed, 1.f, 1.f, 2000.f)) {
		requestSave();
	}
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip(langString(LanguageCategory::Option_UI, LanguageKey::General_Scrolling_Speed_Toolip));

	if (ImGui::Checkbox(langString(LanguageCategory::Option_UI, LanguageKey::General_Drop_Shadow), &profile->dropShadow)) {
		requestSave();
	}

	if (ImGui::ClampingDragInt(langString(LanguageCategory::Option_UI, LanguageKey::General_Max_Messages), &profile->messagesInStack, 1, 1, 8)) {
		requestSave();
	}
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip(langString(LanguageCategory::Option_UI, LanguageKey::General_Max_Messages_Toolip));

	if (ImGui::Checkbox(langString(LanguageCategory::Option_UI, LanguageKey::General_Combine_Messages), &profile->combineAllMessages)) {
		requestSave();
	}
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip(langString(LanguageCategory::Option_UI, LanguageKey::General_Combine_Messages_Toolip));

	if (ImGui::Combo(langStringG(LanguageKey::Font_Master), &profile->masterFont, getFontSelectionString(false).c_str())) {
		defaultFont = getFontType(profile->masterFont, false);
		requestSave();
	}

	if (ImGui::ClampingDragFloat(langStringG(LanguageKey::Default_Font_Size), &profile->defaultFontSize, 1.f, 1.f, 100.f)) {
		requestSave();
	}

	if (ImGui::ClampingDragFloat(langStringG(LanguageKey::Default_Crit_Font_Size), &profile->defaultCritFontSize, 1.f, 1.f, 100.f)) {
		requestSave();
	}

	if (ImGui::Checkbox(langString(LanguageCategory::Option_UI, LanguageKey::General_Self_Only_As_Incoming), &profile->selfMessageOnlyIncoming)) {
		requestSave();
	}
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip(langString(LanguageCategory::Option_UI, LanguageKey::General_Self_Only_As_Incoming_Toolip));

	if (ImGui::Checkbox(langString(LanguageCategory::Option_UI, LanguageKey::General_Out_Only_For_Target), &profile->outgoingOnlyToTarget)) {
		requestSave();
	}
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip(langString(LanguageCategory::Option_UI, LanguageKey::General_Out_Only_For_Target_Toolip));
}

void GW2_SCT::Options::paintScrollAreas() {
	const float square_size = ImGui::GetFontSize();
	ImGuiStyle style = ImGui::GetStyle();

	static int selectedScrollArea = -1;
	{
		ImGui::BeginChild("left pane", ImVec2(ImGui::GetWindowWidth() * 0.25f, 0), true);
		int i = 0;
		auto scrollAreaOptions = std::begin(profile->scrollAreaOptions);
		while (scrollAreaOptions != std::end(profile->scrollAreaOptions)) {
			if (ImGui::Selectable(
				ImGui::BuildLabel("scroll-area-selectable", i).c_str(),
				selectedScrollArea == i,
				ImGuiSelectableFlags_AllowItemOverlap,
				ImVec2(0, square_size + style.FramePadding.y * 2))
				)
				selectedScrollArea = i;
			ImGui::SameLine();
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + style.FramePadding.y);
			ImGui::Text(scrollAreaOptions->get()->name.c_str());
			ImGui::SameLineEnd(square_size + style.FramePadding.y * 2);
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() - style.FramePadding.y);
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.67f, 0.40f, 0.40f, 0.60f));
			std::string modalName = ImGui::BuildLabel(langString(GW2_SCT::LanguageCategory::Scroll_Area_Option_UI, GW2_SCT::LanguageKey::Delete_Confirmation_Title), "scroll-area-delete-modal", i);
			if (ImGui::Button(ImGui::BuildLabel("-", "scroll-area-delete-button", i).c_str(), ImVec2(square_size + style.FramePadding.y * 2, square_size + style.FramePadding.y * 2))) {
				ImGui::OpenPopup(modalName.c_str());
			}
			ImGui::PopStyleColor();
			if (ImGui::BeginPopupModal(modalName.c_str())) {
				ImGui::Text(langString(GW2_SCT::LanguageCategory::Scroll_Area_Option_UI, GW2_SCT::LanguageKey::Delete_Confirmation_Content));
				ImGui::Separator();
				if (ImGui::Button(langString(GW2_SCT::LanguageCategory::Scroll_Area_Option_UI, GW2_SCT::LanguageKey::Delete_Confirmation_Confirmation), ImVec2(120, 0))) {
					scrollAreaOptions = profile->scrollAreaOptions.erase(scrollAreaOptions);
					selectedScrollArea = -1;
					requestSave();
					ImGui::CloseCurrentPopup();
				}
				else {
					scrollAreaOptions++;
					i++;
				}
				ImGui::SameLine();
				if (ImGui::Button(langString(GW2_SCT::LanguageCategory::Scroll_Area_Option_UI, GW2_SCT::LanguageKey::Delete_Confirmation_Cancel), ImVec2(120, 0))) {
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}
			else {
				scrollAreaOptions++;
				i++;
			}
		}
		ImGui::Text("");
		ImGui::SameLineEnd(square_size + style.FramePadding.y * 2);
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.67f, 0.40f, 0.40f, 0.60f));
		if (ImGui::Button("+", ImVec2(square_size + style.FramePadding.y * 2, square_size + style.FramePadding.y * 2))) {
			scroll_area_options_struct newScrollArea{ langString(LanguageCategory::Scroll_Area_Option_UI, LanguageKey::Scroll_Areas_New), 0.f, 0.f, 40.f, 260.f, TextAlign::CENTER, TextCurve::STRAIGHT, ScrollAreaOutlineState::NONE, {} };
			profile->scrollAreaOptions.push_back(std::make_shared<scroll_area_options_struct>(newScrollArea));
			selectedScrollArea = -1;
			requestSave();
		}
		ImGui::PopStyleColor();
		ImGui::EndChild();
	}
	ImGui::SameLine();

	// Right
	{
		ImGui::BeginGroup();
		ImGui::BeginChild("scroll area details", ImVec2(0, -ImGui::GetFrameHeightWithSpacing())); // Leave room for 1 line below us
		if (selectedScrollArea >= 0 && selectedScrollArea < profile->scrollAreaOptions.size()) {
			std::shared_ptr<scroll_area_options_struct> scrollAreaOptions = profile->scrollAreaOptions[selectedScrollArea];

			for (auto otherScrollAreaOptions : profile->scrollAreaOptions) {
				if (scrollAreaOptions == otherScrollAreaOptions) {
					if (otherScrollAreaOptions->outlineState != ScrollAreaOutlineState::FULL) otherScrollAreaOptions->outlineState = ScrollAreaOutlineState::FULL;
				}
				else {
					if (otherScrollAreaOptions->outlineState != ScrollAreaOutlineState::LIGHT) otherScrollAreaOptions->outlineState = ScrollAreaOutlineState::LIGHT;
				}
			}

			if (ImGui::InputText((std::string(langString(GW2_SCT::LanguageCategory::Option_UI, GW2_SCT::LanguageKey::Scroll_Areas_Name)) + "##scroll-area-name").c_str(), &scrollAreaOptions->name)) {
				requestSave();
			}

			ImGui::Separator();

			if (ImGui::ClampingDragFloat(langString(GW2_SCT::LanguageCategory::Scroll_Area_Option_UI, GW2_SCT::LanguageKey::Horizontal_Offset), &scrollAreaOptions->offsetX, 1.f, -windowWidth / 2.f, windowWidth / 2.f)) {
				requestSave();
			}

			if (ImGui::ClampingDragFloat(langString(GW2_SCT::LanguageCategory::Scroll_Area_Option_UI, GW2_SCT::LanguageKey::Vertical_Offset), &scrollAreaOptions->offsetY, 1.f, -windowHeight / 2.f, windowHeight / 2.f)) {
				requestSave();
			}

			if (ImGui::ClampingDragFloat(langString(GW2_SCT::LanguageCategory::Scroll_Area_Option_UI, GW2_SCT::LanguageKey::Width), &scrollAreaOptions->width, 1.f, 1.f, (float)windowWidth)) {
				requestSave();
			}

			if (ImGui::ClampingDragFloat(langString(GW2_SCT::LanguageCategory::Scroll_Area_Option_UI, GW2_SCT::LanguageKey::Height), &scrollAreaOptions->height, 1.f, 1.f, (float)windowHeight)) {
				requestSave();
			}

			if (ImGui::Combo(langString(GW2_SCT::LanguageCategory::Scroll_Area_Option_UI, GW2_SCT::LanguageKey::Text_Align), (int*)&scrollAreaOptions->textAlign, TextAlignTexts, 3)) {
				requestSave();
			}

			if (ImGui::Combo(langString(GW2_SCT::LanguageCategory::Scroll_Area_Option_UI, GW2_SCT::LanguageKey::Text_Flow), (int*)&scrollAreaOptions->textCurve, TextCurveTexts, 3)) {
				requestSave();
			}

			ImGui::Text(langString(GW2_SCT::LanguageCategory::Scroll_Area_Option_UI, GW2_SCT::LanguageKey::All_Receivers));
			int receiverOptionsCounter = 0;
			auto receiverOptionsIterator = std::begin(scrollAreaOptions->receivers);
			while (receiverOptionsIterator != std::end(scrollAreaOptions->receivers)) {
				int receiverReturnFlags = ImGui::ReceiverCollapsible(receiverOptionsCounter, *receiverOptionsIterator);
				if (receiverReturnFlags & ReceiverCollapsible_Remove) {
					receiverOptionsIterator = scrollAreaOptions->receivers.erase(receiverOptionsIterator);
					requestSave();
				}
				else {
					if (receiverReturnFlags != 0) {
						requestSave();
					}
					receiverOptionsIterator++;
					receiverOptionsCounter++;
				}
			}
			if (scrollAreaOptions->receivers.size() == 0) ImGui::Text("    -");
			ImGui::Separator();
			ImGui::Text(langString(GW2_SCT::LanguageCategory::Scroll_Area_Option_UI, GW2_SCT::LanguageKey::New_Receiver));
			static MessageCategory newReceiverCategory = MessageCategory::PLAYER_OUT;
			static MessageType newReceiverType = MessageType::PHYSICAL;
			if (ImGui::NewReceiverLine(&newReceiverCategory, &newReceiverType)) {
				auto& defaultReceiver = receiverInformationPerCategoryAndType.at(newReceiverCategory).at(newReceiverType).defaultReceiver;
				scrollAreaOptions->receivers.push_back(std::make_shared<message_receiver_options_struct>(defaultReceiver));
				requestSave();
			}
		}
		else {
			for (auto scrollAreaOptions : profile->scrollAreaOptions) {
				if (scrollAreaOptions->outlineState != ScrollAreaOutlineState::FULL) scrollAreaOptions->outlineState = ScrollAreaOutlineState::FULL;
			}
		}
		ImGui::EndChild();
		ImGui::EndGroup();
	}
}


bool drawColorSelector(const char* name, std::string& color) {
	int num = std::stoi(color, 0, 16);
	int red = num / 0x10000;
	int green = (num / 0x100) % 0x100;
	int blue = num % 0x100;
	float col[3] = { red / 255.f, green / 255.f, blue / 255.f };
	if (ImGui::ColorEdit3(name, col, ImGuiColorEditFlags_NoAlpha)) {
		std::stringstream stm;
		stm << std::uppercase << std::setfill('0') << std::setw(2) << std::hex << static_cast<int>(col[0] * 255.f);
		stm << std::uppercase << std::setfill('0') << std::setw(2) << std::hex << static_cast<int>(col[1] * 255.f);
		stm << std::uppercase << std::setfill('0') << std::setw(2) << std::hex << static_cast<int>(col[2] * 255.f);
		color = stm.str();
		GW2_SCT::Options::requestSave();
		return true;
	}
	return false;
}

void GW2_SCT::Options::paintProfessionColors() {
	drawColorSelector(langString(LanguageCategory::Option_UI, LanguageKey::Profession_Colors_Guardian), profile->professionColorGuardian);
	drawColorSelector(langString(LanguageCategory::Option_UI, LanguageKey::Profession_Colors_Warrior), profile->professionColorWarrior);
	drawColorSelector(langString(LanguageCategory::Option_UI, LanguageKey::Profession_Colors_Engineer), profile->professionColorEngineer);
	drawColorSelector(langString(LanguageCategory::Option_UI, LanguageKey::Profession_Colors_Ranger), profile->professionColorRanger);
	drawColorSelector(langString(LanguageCategory::Option_UI, LanguageKey::Profession_Colors_Thief), profile->professionColorThief);
	drawColorSelector(langString(LanguageCategory::Option_UI, LanguageKey::Profession_Colors_Elementalist), profile->professionColorElementalist);
	drawColorSelector(langString(LanguageCategory::Option_UI, LanguageKey::Profession_Colors_Mesmer), profile->professionColorMesmer);
	drawColorSelector(langString(LanguageCategory::Option_UI, LanguageKey::Profession_Colors_Necromancer), profile->professionColorNecromancer);
	drawColorSelector(langString(LanguageCategory::Option_UI, LanguageKey::Profession_Colors_Revenant), profile->professionColorRevenant);
	drawColorSelector(langString(LanguageCategory::Option_UI, LanguageKey::Profession_Colors_Undetectable), profile->professionColorDefault);
}

void GW2_SCT::Options::paintSkillFilters() {
	const float square_size = ImGui::GetFontSize();
	ImGuiStyle style = ImGui::GetStyle();

	static std::string selectedFilterSet = "";

	// Left pane - Filter Set List
	{
		ImGui::BeginChild("filter_sets_list", ImVec2(ImGui::GetWindowWidth() * 0.3f, 0), true);
		ImGui::Text("Filter Sets");
		ImGui::Separator();

		for (auto& [name, filterSet] : profile->filterManager.getAllFilterSets()) {
			bool isSelected = (selectedFilterSet == name);
			if (ImGui::Selectable(ImGui::BuildLabel(name.c_str(), "filter-set-selectable", name).c_str(), isSelected)) {
				selectedFilterSet = name;
			}

			if (ImGui::IsItemHovered() && !filterSet->description.empty()) {
				ImGui::BeginTooltip();
				ImGui::Text("%s", filterSet->description.c_str());
				ImGui::EndTooltip();
			}
		}

		ImGui::Separator();

		static char newFilterSetName[256] = "";
		ImGui::InputText("##new_filter_set_name", newFilterSetName, sizeof(newFilterSetName));
		ImGui::SameLine();
		if (ImGui::Button("Add") && strlen(newFilterSetName) > 0) {
			if (profile->filterManager.getFilterSet(newFilterSetName) == nullptr) {
				profile->filterManager.createFilterSet(newFilterSetName);
				selectedFilterSet = newFilterSetName;
				memset(newFilterSetName, 0, sizeof(newFilterSetName));
				requestSave();
			}
		}

		ImGui::EndChild();
	}

	ImGui::SameLine();

	// Right pane - Filter Set Details
	{
		ImGui::BeginChild("filter_set_details", ImVec2(0, 0), true);

		if (!selectedFilterSet.empty()) {
			auto filterSet = profile->filterManager.getFilterSet(selectedFilterSet);
			if (filterSet) {
				ImGui::Text("Filter Set: %s", filterSet->name.c_str());

				static char renameBuffer[256] = "";
				if (ImGui::BeginPopupModal("Rename Filter Set")) {
					if (ImGui::IsWindowAppearing()) {
						strncpy(renameBuffer, filterSet->name.c_str(), sizeof(renameBuffer) - 1);
						renameBuffer[sizeof(renameBuffer) - 1] = '\0';
					}

					ImGui::Text("Enter new name:");
					ImGui::InputText("##rename_filter_set", renameBuffer, sizeof(renameBuffer));

					bool nameExists = false;
					if (strlen(renameBuffer) > 0 && strcmp(renameBuffer, filterSet->name.c_str()) != 0) {
						nameExists = (profile->filterManager.getFilterSet(renameBuffer) != nullptr);
						if (nameExists) {
							ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "A filter set with this name already exists!");
						}
					}

					ImGui::Separator();

					if (strlen(renameBuffer) == 0 || nameExists) {
						ImGui::BeginDisabled();
					}
					if (ImGui::Button("OK", ImVec2(120, 0))) {
						if (strlen(renameBuffer) > 0 && strcmp(renameBuffer, filterSet->name.c_str()) != 0) {
							std::string oldName = filterSet->name;
							std::string newName = renameBuffer;

							filterSet->name = newName;

							profile->filterManager.removeFilterSet(oldName);
							profile->filterManager.addFilterSet(filterSet);

							for (auto& scrollArea : profile->scrollAreaOptions) {
								for (auto& receiver : scrollArea->receivers) {
									for (auto& assignedName : receiver->assignedFilterSets) {
										if (assignedName == oldName) {
											assignedName = newName;
										}
									}
								}
							}

							selectedFilterSet = newName;

							requestSave();
							ImGui::CloseCurrentPopup();
						}
					}
					if (strlen(renameBuffer) == 0 || nameExists) {
						ImGui::EndDisabled();
					}

					ImGui::SameLine();
					if (ImGui::Button("Cancel", ImVec2(120, 0))) {
						ImGui::CloseCurrentPopup();
					}
					ImGui::EndPopup();
				}

				ImGui::Separator();

				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.67f, 0.40f, 0.40f, 0.60f));
				if (ImGui::Button("Delete Filter Set")) {
					ImGui::OpenPopup("Delete Filter Set");
				}
				ImGui::PopStyleColor();

				ImGui::SameLine();
				if (ImGui::Button("Rename")) {
					ImGui::OpenPopup("Rename Filter Set");
				}

				if (ImGui::BeginPopupModal("Delete Filter Set")) {
					ImGui::Text("Delete filter set '%s'?", filterSet->name.c_str());
					if (ImGui::Button("Yes", ImVec2(120, 0))) {
						for (auto& scrollArea : profile->scrollAreaOptions) {
							for (auto& receiver : scrollArea->receivers) {
								auto it = std::find(receiver->assignedFilterSets.begin(),
									receiver->assignedFilterSets.end(),
									filterSet->name);
								if (it != receiver->assignedFilterSets.end()) {
									receiver->assignedFilterSets.erase(it);
								}
							}
						}
						profile->filterManager.removeFilterSet(filterSet->name);
						selectedFilterSet = "";
						requestSave();
						ImGui::CloseCurrentPopup();
					}
					ImGui::SameLine();
					if (ImGui::Button("No", ImVec2(120, 0))) {
						ImGui::CloseCurrentPopup();
					}
					ImGui::EndPopup();
				}

				if (ImGui::InputText("Description", &filterSet->description)) {
					requestSave();
				}

				ImGui::Separator();

				ImGui::Text("Default Action:");
				ImGui::SameLine();
				int defaultAction = static_cast<int>(filterSet->filterSet.defaultAction);
				if (ImGui::RadioButton("Allow", &defaultAction, 0)) {
					filterSet->filterSet.defaultAction = FilterAction::ALLOW;
					requestSave();
				}
				ImGui::SameLine();
				if (ImGui::RadioButton("Block", &defaultAction, 1)) {
					filterSet->filterSet.defaultAction = FilterAction::BLOCK;
					requestSave();
				}

				ImGui::Separator();
				ImGui::Text("Filters:");

				int filterIndex = 0;
				auto it = filterSet->filterSet.filters.begin();
				while (it != filterSet->filterSet.filters.end()) {
					ImGui::PushID(filterIndex);

					bool shouldDelete = false;

					ImGui::BeginGroup();

					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.67f, 0.40f, 0.40f, 0.60f));
					if (ImGui::Button("X")) {
						shouldDelete = true;
					}
					ImGui::PopStyleColor();

					ImGui::SameLine();

					int action = static_cast<int>(it->action);
					ImGui::SetNextItemWidth(80);
					if (ImGui::Combo("##action", &action, "Allow\0Block\0")) {
						it->action = static_cast<FilterAction>(action);
						requestSave();
					}

					ImGui::SameLine();

					int type = static_cast<int>(it->type);
					ImGui::SetNextItemWidth(120);
					if (ImGui::Combo("##type", &type, "Skill ID\0Skill Name\0ID Range\0")) {
						it->type = static_cast<FilterType>(type);
						requestSave();
					}

					ImGui::SameLine();

					switch (it->type) {
					case FilterType::SKILL_ID: {
						ImGui::SetNextItemWidth(200);
						if (ImGui::InputScalar("##value", ImGuiDataType_U32, &it->skillId)) {
							requestSave();
						}
						break;
					}
					case FilterType::SKILL_NAME: {
						ImGui::SetNextItemWidth(200);
						if (ImGui::InputText("##value", &it->skillName)) {
							requestSave();
						}
						break;
					}
					case FilterType::SKILL_ID_RANGE: {
						ImGui::SetNextItemWidth(90);
						if (ImGui::InputScalar("##start", ImGuiDataType_U32, &it->idRange.start)) {
							requestSave();
						}
						ImGui::SameLine();
						ImGui::Text("-");
						ImGui::SameLine();
						ImGui::SetNextItemWidth(90);
						if (ImGui::InputScalar("##end", ImGuiDataType_U32, &it->idRange.end)) {
							requestSave();
						}
						break;
					}
					}

					ImGui::EndGroup();

					ImGui::PopID();

					if (shouldDelete) {
						it = filterSet->filterSet.filters.erase(it);
						requestSave();
					}
					else {
						++it;
						++filterIndex;
					}
				}

				if (ImGui::Button("+ Add Filter")) {
					SkillFilter newFilter;
					newFilter.type = FilterType::SKILL_ID;
					newFilter.action = FilterAction::BLOCK;
					newFilter.skillId = 0;
					filterSet->filterSet.filters.push_back(newFilter);
					requestSave();
				}

				ImGui::Separator();

				ImGui::Text("Used by:");
				int usageCount = 0;
				for (auto& scrollArea : profile->scrollAreaOptions) {
					for (auto& receiver : scrollArea->receivers) {
						auto it = std::find(receiver->assignedFilterSets.begin(),
							receiver->assignedFilterSets.end(),
							filterSet->name);
						if (it != receiver->assignedFilterSets.end()) {
							ImGui::BulletText("%s - %s",
								scrollArea->name.c_str(),
								receiver->name.c_str());
							usageCount++;
						}
					}
				}
				if (usageCount == 0) {
					ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "  Not assigned to any receivers");
				}
			}
		}
		else {
			ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Select or create a filter set");
		}

		ImGui::EndChild();
	}
}

void GW2_SCT::Options::paintSkillIcons() {
	std::string language_warning(langString(LanguageCategory::Skill_Icons_Option_UI, LanguageKey::Skill_Icons_Warning));
	ImGui::TextWrapped((language_warning + " '" + getSCTPath() + "icons\\" + "'").c_str());
	if (ImGui::Checkbox(langString(LanguageCategory::Skill_Icons_Option_UI, LanguageKey::Skill_Icons_Enable), &profile->skillIconsEnabled)) {
		requestSave();
	}
	ImGui::TextWrapped(langString(LanguageCategory::Skill_Icons_Option_UI, LanguageKey::Skill_Icons_Preload_Description));
	if (ImGui::Checkbox(langString(LanguageCategory::Skill_Icons_Option_UI, LanguageKey::Skill_Icons_Preload), &profile->preloadAllSkillIcons)) {
		requestSave();
	}
	ImGui::Text("");
	if (ImGui::BeginCombo(
		ImGui::BuildVisibleLabel(langString(LanguageCategory::Skill_Icons_Option_UI, LanguageKey::Skill_Icons_Display_Type), "skill-icons-display-combo").c_str(),
		skillIconsDisplayTypeNames.at(profile->skillIconsDisplayType).c_str())
		) {
		int i = 0;
		for (auto& skillIconDisplayTypeAndName : skillIconsDisplayTypeNames) {
			if (ImGui::Selectable(ImGui::BuildLabel(skillIconDisplayTypeAndName.second, "skill-icons-display-selectable", i).c_str())) {
				if (profile->skillIconsDisplayType != skillIconDisplayTypeAndName.first) {
					profile->skillIconsDisplayType = skillIconDisplayTypeAndName.first;
					requestSave();
				}
			}
			i++;
		}
		ImGui::EndCombo();
	}
}

void GW2_SCT::Options::paintProfiles() {
	bool doesCharacterMappingExist = currentCharacterName != "" && options.characterProfileMap.find(currentCharacterName) != options.characterProfileMap.end();

	ImGui::TextWrapped(langString(LanguageCategory::Profile_Option_UI, LanguageKey::Profile_Description));

	if (ImGui::BeginCombo(ImGui::BuildVisibleLabel(langString(LanguageCategory::Profile_Option_UI, LanguageKey::Master_Profile), "profile-combo").c_str(), options.globalProfile.c_str())) {
		for (auto& nameAndProfile : options.profiles) {
			if (ImGui::Selectable((nameAndProfile.first + "##profile-selectable").c_str())) {
				if (options.globalProfile != nameAndProfile.first) {
					options.globalProfile = nameAndProfile.first;
					if (!doesCharacterMappingExist) {
						currentProfileName = nameAndProfile.first;
						profile = nameAndProfile.second;
					}
					requestSave();
				}
			}
		}
		ImGui::EndCombo();
	}
	if (currentCharacterName != "") {
		ImGui::Text(langString(LanguageCategory::Profile_Option_UI, LanguageKey::Character_Specific_Profile_Heading));
		if (ImGui::Checkbox((std::string(langString(LanguageCategory::Profile_Option_UI, LanguageKey::Character_Specific_Profile_Enabled)) + currentCharacterName).c_str(), &doesCharacterMappingExist)) {
			if (doesCharacterMappingExist) {
				options.characterProfileMap[currentCharacterName] = currentProfileName;
			}
			else {
				currentProfileName = options.globalProfile;
				profile = options.profiles[currentProfileName];
				options.characterProfileMap.erase(currentCharacterName);
			}
			requestSave();
		}
		if (!doesCharacterMappingExist) {
			ImGui::BeginDisabled();
		}
		if (ImGui::BeginCombo("##character-profile-combo", currentProfileName.c_str())) {
			for (auto& nameAndProfile : options.profiles) {
				if (ImGui::Selectable((nameAndProfile.first + "##character-profile-selectable").c_str())) {
					currentProfileName = nameAndProfile.first;
					profile = nameAndProfile.second;
					options.characterProfileMap[currentCharacterName] = currentProfileName;
					requestSave();
				}
			}
			ImGui::EndCombo();
		}
		if (!doesCharacterMappingExist) {
			ImGui::EndDisabled();
		}
	}
	ImGui::Text(langString(LanguageCategory::Profile_Option_UI, LanguageKey::Current_Profile_Heading));
	bool currentProfileIsDefault = currentProfileName == defaultProfileName;
	if (currentProfileIsDefault) {
		ImGui::BeginDisabled();
	}
	std::string nameCopy = currentProfileName;
	bool changedBG = false;
	if (ImGui::InputText(ImGui::BuildVisibleLabel(langString(LanguageCategory::Profile_Option_UI, LanguageKey::Current_Profile_Name), "profile-name").c_str(), &nameCopy, ImGuiInputTextFlags_CallbackAlways, [](ImGuiInputTextCallbackData* data) {
		bool* d = static_cast<bool*>(data->UserData);
		std::string text(data->Buf);
		if (text != currentProfileName && (text == defaultProfileName || options.profiles.find(text) != options.profiles.end())) {
			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(1.f, 0.f, 0.f, .6f));
			*d = true;
		}
		return 0;
		}, &changedBG)) {
		if (!changedBG && nameCopy != currentProfileName) {
			if (options.globalProfile == currentProfileName) {
				options.globalProfile = nameCopy;
			}
			options.profiles[nameCopy] = profile;
			options.profiles.erase(currentProfileName);
			for (auto& characterProfileMapping : options.characterProfileMap) {
				if (characterProfileMapping.second == currentProfileName) {
					characterProfileMapping.second = nameCopy;
				}
			}
			currentProfileName = nameCopy;
			requestSave();
		}
	}
	if (changedBG) {
		ImGui::PopStyleColor();
	}
	if (currentProfileIsDefault) {
		ImGui::EndDisabled();
	}
	if (ImGui::Button(ImGui::BuildVisibleLabel(langString(LanguageCategory::Profile_Option_UI, LanguageKey::Create_Profile_Copy), "profile-copy-button").c_str())) {
		std::string copyName = currentProfileName + " " + std::string(langString(LanguageCategory::Profile_Option_UI, LanguageKey::Profile_Copy_Suffix));
		if (options.profiles.find(copyName) != options.profiles.end()) {
			int i = 1;
			while (options.profiles.find(copyName + " " + std::to_string(i)) != options.profiles.end()) { i++; }
			copyName = copyName + " " + std::to_string(i);
		}
		nlohmann::json j = *profile.operator std::shared_ptr<profile_options_struct>().get();
		options.profiles[copyName] = std::make_shared<profile_options_struct>(j);
		if (options.globalProfile == currentProfileName) {
			options.globalProfile = copyName;
		}
		for (auto& characterProfileMapping : options.characterProfileMap) {
			if (characterProfileMapping.second == currentProfileName) {
				characterProfileMapping.second = copyName;
			}
		}
		currentProfileName = copyName;
		profile = options.profiles[currentProfileName];
		requestSave();
	}
	ImGui::SameLine();
	if (ImGui::Button(ImGui::BuildVisibleLabel(langString(LanguageCategory::Profile_Option_UI, LanguageKey::Create_Profile_New), "profile-new-button").c_str())) {
		std::string newProfileName = "New Profile";
		if (options.profiles.find(newProfileName) != options.profiles.end()) {
			int i = 1;
			while (options.profiles.find(newProfileName + " " + std::to_string(i)) != options.profiles.end()) { i++; }
			newProfileName = newProfileName + " " + std::to_string(i);
		}

		auto newProfile = std::make_shared<profile_options_struct>();
		initProfileWithDefaults(newProfile);

		options.profiles[newProfileName] = newProfile;

		if (doesCharacterMappingExist) {
			options.characterProfileMap[currentCharacterName] = newProfileName;
		}
		else {
			options.globalProfile = newProfileName;
		}
		currentProfileName = newProfileName;
		profile = newProfile;
		requestSave();
	}
	if (currentProfileIsDefault) {
		ImGui::BeginDisabled();
	}
	std::string deleteProfileLabel = ImGui::BuildVisibleLabel(langString(LanguageCategory::Profile_Option_UI, LanguageKey::Delete_Confirmation_Title), "##delete-profile-modal");
	if (ImGui::Button(ImGui::BuildVisibleLabel(langString(LanguageCategory::Profile_Option_UI, LanguageKey::Delete_Profile), "profile-delete-button").c_str())) {
		ImGui::OpenPopup(deleteProfileLabel.c_str());
	}
	if (currentProfileIsDefault) {
		ImGui::EndDisabled();
	}
	if (ImGui::BeginPopupModal(deleteProfileLabel.c_str())) {
		ImGui::Text(langString(GW2_SCT::LanguageCategory::Profile_Option_UI, GW2_SCT::LanguageKey::Delete_Confirmation_Content));
		ImGui::Separator();
		if (ImGui::Button(ImGui::BuildVisibleLabel(langString(GW2_SCT::LanguageCategory::Profile_Option_UI, GW2_SCT::LanguageKey::Delete_Confirmation_Confirmation), "delete-profile-modal-confirm").c_str(), ImVec2(120, 0))) {
			if (options.globalProfile == currentProfileName) {
				options.globalProfile = defaultProfileName;
			}
			for (auto& characterProfileMapping : options.characterProfileMap) {
				if (characterProfileMapping.second == currentProfileName) {
					characterProfileMapping.second = defaultProfileName;
				}
			}
			options.profiles.erase(currentProfileName);
			currentProfileName = defaultProfileName;
			profile = options.profiles[currentProfileName];
			requestSave();
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button(ImGui::BuildVisibleLabel(langString(GW2_SCT::LanguageCategory::Profile_Option_UI, GW2_SCT::LanguageKey::Delete_Confirmation_Cancel), "delete-profile-modal-cancel").c_str(), ImVec2(120, 0))) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}
