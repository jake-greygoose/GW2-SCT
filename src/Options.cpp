#include "Options.h"
#include <sstream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <cstdio>
#include <mutex>
#include <thread>
#include "json.hpp"
#include "Common.h"
#include "imgui.h"
#include "imgui_stdlib.h"
#include "imgui_sct_widgets.h"
#include "TemplateInterpreter.h"
#include "Language.h"
#include "SkillFilterStructures.h"
#include "ScrollArea.h"
#include "Profiles.h"
#include "SkillFilterUI.h"

const char* TextAlignTexts[] = { langStringG(GW2_SCT::LanguageKey::Text_Align_Left), langStringG(GW2_SCT::LanguageKey::Text_Align_Center), langStringG(GW2_SCT::LanguageKey::Text_Align_Right) };
const char* TextCurveTexts[] = { langStringG(GW2_SCT::LanguageKey::Text_Curve_Left), langStringG(GW2_SCT::LanguageKey::Text_Curve_Straight), langStringG(GW2_SCT::LanguageKey::Text_Curve_Right), langStringG(GW2_SCT::LanguageKey::Text_Curve_Static), langStringG(GW2_SCT::LanguageKey::Text_Curve_Angled) };
const char* ScrollDirectionTexts[] = { "Down", "Up" };

GW2_SCT::options_struct GW2_SCT::Options::options;
bool GW2_SCT::Options::windowIsOpen = false;
std::string GW2_SCT::Options::fontSelectionString = "";
std::string GW2_SCT::Options::fontSelectionStringWithMaster = "";
std::string GW2_SCT::Options::fontSizeTypeSelectionString = "";


std::chrono::steady_clock::time_point GW2_SCT::Options::lastSaveRequest = std::chrono::steady_clock::now();
bool GW2_SCT::Options::saveRequested = false;
const std::chrono::milliseconds GW2_SCT::Options::SAVE_DELAY(500);


static bool inScrollAreasTab = false;

bool GW2_SCT::Options::isInScrollAreasTab() {
	return windowIsOpen && inScrollAreasTab;
}

const std::shared_ptr<GW2_SCT::profile_options_struct> GW2_SCT::Options::get() {
	return Profiles::get();
}

std::string GW2_SCT::Options::getCurrentCharacterName() {
	return Profiles::getCurrentCharacterName();
}

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

void GW2_SCT::Options::paint(const std::vector<std::shared_ptr<ScrollArea>>& scrollAreas) {

	processPendingSave();

	if (windowIsOpen) {
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.60f, 0.60f, 0.60f, 0.30f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
		ImGui::Begin(langString(LanguageCategory::Option_UI, LanguageKey::Title), &windowIsOpen, ImGuiWindowFlags_NoCollapse);

		ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
		if (ImGui::BeginTabBar("##options-tab-bar", tab_bar_flags))
		{
			if (ImGui::BeginTabItem(langString(LanguageCategory::Option_UI, LanguageKey::Menu_Bar_General))) {
				inScrollAreasTab = false;
				paintGeneral();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem(langString(LanguageCategory::Option_UI, LanguageKey::Menu_Bar_Scroll_Areas))) {
				inScrollAreasTab = true;
				paintScrollAreas(scrollAreas);
				ImGui::EndTabItem();
			}
			else {
				if (inScrollAreasTab) {
					inScrollAreasTab = false;
					auto currentProfile = Profiles::get();
					if (currentProfile) {
						for (auto scrollAreaOptions : currentProfile->scrollAreaOptions) {
							if (scrollAreaOptions->outlineState != ScrollAreaOutlineState::NONE) scrollAreaOptions->outlineState = ScrollAreaOutlineState::NONE;
						}
					}
				}
			}
			if (ImGui::BeginTabItem(langString(LanguageCategory::Option_UI, LanguageKey::Menu_Bar_Profession_Colors))) {
				inScrollAreasTab = false;
				paintProfessionColors();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem(langString(LanguageCategory::Option_UI, LanguageKey::Menu_Bar_Filtered_Skills))) {
				inScrollAreasTab = false;
				SkillFilterUI::paintUI();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem(langString(LanguageCategory::Option_UI, LanguageKey::Menu_Bar_Global_Thresholds))) {
				inScrollAreasTab = false;
				paintGlobalThresholds();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem(langString(LanguageCategory::Option_UI, LanguageKey::Menu_Bar_Skill_Icons))) {
				inScrollAreasTab = false;
				paintSkillIcons();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem(langString(LanguageCategory::Option_UI, LanguageKey::Menu_Bar_Profiles))) {
				inScrollAreasTab = false;
				Profiles::paintUI();
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}

		ImGui::End();
		ImGui::PopStyleVar();
		ImGui::PopStyleColor();
	}
	else {
		inScrollAreasTab = false;
		auto currentProfile = Profiles::get();
		if (!currentProfile) return;
		bool stateWasChanged = false;
		for (auto scrollAreaOptions : currentProfile->scrollAreaOptions) {
			if (scrollAreaOptions->outlineState != ScrollAreaOutlineState::NONE) {
				scrollAreaOptions->outlineState = ScrollAreaOutlineState::NONE;
				stateWasChanged = true;
			}
		}
		if (stateWasChanged) {
			save();
		}
	}
}

std::string btos(bool b) {
	return b ? "1" : "0";
}

bool stob(std::string const& s) {
	return s != "0";
}

void GW2_SCT::Options::save() {
#if _DEBUG
	LOG("SAVE TRIGGERED: Saving options with ", options.profiles.size(), " profiles");
#endif
	if (options.profiles.empty()) {
		LOG("ERROR: Attempted to save options with empty profiles map! Save blocked to prevent data loss.");
		LOG("This suggests profiles failed to load or were cleared unexpectedly.");
		return;
	}
	
	std::ofstream out((getSCTPath() + "sct.json").c_str());
	nlohmann::json j = options;
	out << j.dump(2);
	out.close();
#if _DEBUG
	LOG("Save completed");
#endif
}

void GW2_SCT::Options::requestSave() {
#if _DEBUG
	LOG("SAVE REQUESTED: Current profiles count: ", options.profiles.size());
#endif
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
	SkillFilterUI::initialize();

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
			
#if _DEBUG
			LOG("JSON parsing successful. Keys found: ");
			for (auto it = j.begin(); it != j.end(); ++it) {
				LOG("  - ", it.key());
			}
			
			if (j.contains("profiles")) {
				if (j["profiles"].is_object()) {
					LOG("Profiles section found with ", j["profiles"].size(), " entries:");
					for (auto it = j["profiles"].begin(); it != j["profiles"].end(); ++it) {
						LOG("  - Profile: ", it.key());
					}
				} else {
					LOG("Profiles section exists but is not an object, type: ", j["profiles"].type_name());
				}
			} else {
				LOG("No profiles section found in JSON");
			}
#endif
			
			options_struct tempOptions;
			j.get_to(tempOptions);
			
			if (tempOptions.profiles.empty()) {
				LOG("Warning: JSON loaded but profiles is empty, initializing default profile");
				tempOptions.profiles[Profiles::getDefaultProfileName()] = std::make_shared<profile_options_struct>();
				Profiles::initWithDefaults(tempOptions.profiles[Profiles::getDefaultProfileName()]);
			}
#if _DEBUG
			else {
				LOG("Successfully loaded ", tempOptions.profiles.size(), " profiles from JSON");
			}
#endif
			
			options = std::move(tempOptions);
#if _DEBUG
			LOG("After moving tempOptions to options, profiles count: ", options.profiles.size());
#endif
			
			if (options.profiles.find(Profiles::getDefaultProfileName()) == options.profiles.end()) {
#if _DEBUG
				LOG("Default profile not found, creating it");
#endif
				options.profiles[Profiles::getDefaultProfileName()] = std::make_shared<profile_options_struct>();
				Profiles::initWithDefaults(options.profiles[Profiles::getDefaultProfileName()]);
			}

			// Ensure profile is set during initial load
			if (!Profiles::get()) {
				Profiles::requestSwitch(options.profiles[options.pveDefaultProfile]);
			}
			Profiles::loadForCharacter("");
			Profiles::processPendingSwitch();
#if _DEBUG
			LOG("Final profiles count after loading: ", options.profiles.size());
#endif
			auto currentProfile = Profiles::get();
			if (currentProfile) {
				defaultFont = getFontType(currentProfile->masterFont, false);
			}
		}
		catch (std::exception& e) {
			LOG("Error loading options: ", e.what());
			setDefault();
		}
	}
	else {
		setDefault();
	}
}




void GW2_SCT::Options::setDefault() {
	LOG("WARNING: Resetting all options to default - all profiles and settings will be lost");
	auto defaultProfile = std::make_shared<profile_options_struct>();
	Profiles::initWithDefaults(defaultProfile);
	options.profiles[Profiles::getDefaultProfileName()] = defaultProfile;
	Profiles::requestSwitch(defaultProfile);
}

bool GW2_SCT::Options::isOptionsWindowOpen() {
	return windowIsOpen;
}

void GW2_SCT::Options::paintGeneral() {
    auto currentProfile = Profiles::get();
    if (!currentProfile) return;
    if (ImGui::Checkbox(langString(LanguageCategory::Option_UI, LanguageKey::General_Enabled), &currentProfile->sctEnabled)) {
        requestSave();
    }

	if (ImGui::ClampingDragFloat(langString(LanguageCategory::Option_UI, LanguageKey::General_Scrolling_Speed), &currentProfile->scrollSpeed, 1.f, 1.f, 2000.f)) {
		requestSave();
	}
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip(langString(LanguageCategory::Option_UI, LanguageKey::General_Scrolling_Speed_Toolip));

    if (ImGui::Checkbox(langString(LanguageCategory::Option_UI, LanguageKey::General_Drop_Shadow), &currentProfile->dropShadow)) {
        requestSave();
    }

    {
        if (ImGui::SliderFloat(langString(LanguageCategory::Option_UI, LanguageKey::General_Global_Opacity), &currentProfile->globalOpacity, 0.0f, 1.0f, "%.2f")) {
            requestSave();
        }
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
        ImGui::Text("(%.0f%%)", currentProfile->globalOpacity * 100.0f);
        ImGui::PopStyleColor();
    }

	if (ImGui::ClampingDragInt(langString(LanguageCategory::Option_UI, LanguageKey::General_Max_Messages), &currentProfile->messagesInStack, 1, 1, 8)) {
		requestSave();
	}
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip(langString(LanguageCategory::Option_UI, LanguageKey::General_Max_Messages_Toolip));

	if (ImGui::Checkbox(langString(LanguageCategory::Option_UI, LanguageKey::General_Combine_Messages), &currentProfile->combineAllMessages)) {
		requestSave();
	}
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip(langString(LanguageCategory::Option_UI, LanguageKey::General_Combine_Messages_Toolip));

	if (ImGui::Combo(langStringG(LanguageKey::Font_Master), &currentProfile->masterFont, getFontSelectionString(false).c_str())) {
		defaultFont = getFontType(currentProfile->masterFont, false);
		requestSave();
	}

	if (ImGui::ClampingDragFloat(langStringG(LanguageKey::Default_Font_Size), &currentProfile->defaultFontSize, 1.f, 1.f, 100.f)) {
		requestSave();
	}

	if (ImGui::ClampingDragFloat(langStringG(LanguageKey::Default_Crit_Font_Size), &currentProfile->defaultCritFontSize, 1.f, 1.f, 100.f)) {
		requestSave();
	}

	if (ImGui::Checkbox(langString(LanguageCategory::Option_UI, LanguageKey::General_Self_Only_As_Incoming), &currentProfile->selfMessageOnlyIncoming)) {
		requestSave();
	}
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip(langString(LanguageCategory::Option_UI, LanguageKey::General_Self_Only_As_Incoming_Toolip));

	if (ImGui::Checkbox(langString(LanguageCategory::Option_UI, LanguageKey::General_Out_Only_For_Target), &currentProfile->outgoingOnlyToTarget)) {
		requestSave();
	}
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip(langString(LanguageCategory::Option_UI, LanguageKey::General_Out_Only_For_Target_Toolip));
}

void GW2_SCT::Options::paintScrollAreas(const std::vector<std::shared_ptr<ScrollArea>>& scrollAreas) {
	auto currentProfile = Profiles::get();
	if (!currentProfile) return;
	
	const float square_size = ImGui::GetFontSize();
	ImGuiStyle style = ImGui::GetStyle();

	static int selectedScrollArea = -1;
	{
		ImGui::BeginChild("left pane", ImVec2(ImGui::GetWindowWidth() * 0.25f, 0), true);
		int i = 0;
		auto scrollAreaOptions = std::begin(currentProfile->scrollAreaOptions);
		while (scrollAreaOptions != std::end(currentProfile->scrollAreaOptions)) {
			if (ImGui::Selectable(
				ImGui::BuildLabel("scroll-area-selectable", i).c_str(),
				selectedScrollArea == i,
				ImGuiSelectableFlags_AllowItemOverlap,
				ImVec2(0, square_size + style.FramePadding.y * 2))) {
				selectedScrollArea = i;
			}
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
					scrollAreaOptions = currentProfile->scrollAreaOptions.erase(scrollAreaOptions);
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
			scroll_area_options_struct newArea;
			newArea.name = langString(LanguageCategory::Scroll_Area_Option_UI, LanguageKey::Scroll_Areas_New);
			newArea.offsetX = 0.f;
			newArea.offsetY = 0.f;
			newArea.width = 40.f;
			newArea.height = 260.f;
			newArea.textAlign = TextAlign::CENTER;
			newArea.textCurve = TextCurve::STRAIGHT;
			newArea.scrollDirection = ScrollDirection::DOWN;
			newArea.showCombinedHitCount = true;
			newArea.outlineState = ScrollAreaOutlineState::NONE;
			newArea.receivers = {};
			newArea.enabled = true;
			newArea.abbreviateSkillNames = false;
			newArea.shortenNumbersPrecision = -1;
			newArea.disableCombining = false;
			newArea.customScrollSpeed = -1.0f;  // use global

			currentProfile->scrollAreaOptions.push_back(std::make_shared<scroll_area_options_struct>(newArea));
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
		ImGui::BeginChild("scroll area details", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()));
		if (selectedScrollArea >= 0 && selectedScrollArea < currentProfile->scrollAreaOptions.size()) {
			std::shared_ptr<scroll_area_options_struct> scrollAreaOptions = currentProfile->scrollAreaOptions[selectedScrollArea];

			for (auto otherScrollAreaOptions : currentProfile->scrollAreaOptions) {
				if (scrollAreaOptions == otherScrollAreaOptions) {
					if (otherScrollAreaOptions->outlineState != ScrollAreaOutlineState::FULL) otherScrollAreaOptions->outlineState = ScrollAreaOutlineState::FULL;
				}
				else {
					if (otherScrollAreaOptions->outlineState != ScrollAreaOutlineState::LIGHT) otherScrollAreaOptions->outlineState = ScrollAreaOutlineState::LIGHT;
				}
			}

			if (ImGui::Checkbox(scrollAreaOptions->enabled ? "Scroll area enabled" : "Scroll area disabled", &scrollAreaOptions->enabled)) {
				requestSave();
			}
			
			ImGui::Text("Name:");
			ImGui::SameLine();
			if (ImGui::InputText("##scroll-area-name", &scrollAreaOptions->name)) {
				requestSave();
			}

			ImGui::Separator();

			ImGui::Columns(2, "position_columns", false);
			ImGui::SetColumnWidth(0, 150);
			
			ImGui::Text("Horizontal Offset");
			ImGui::NextColumn();
			if (ImGui::InputFloat("##horizontal_offset", &scrollAreaOptions->offsetX, 1.0f, 10.0f)) {
				scrollAreaOptions->offsetX = std::clamp(scrollAreaOptions->offsetX, -windowWidth / 2.f, windowWidth / 2.f);
				requestSave();
			}
			ImGui::NextColumn();
			
			ImGui::Text("Vertical Offset");
			ImGui::NextColumn();
			if (ImGui::InputFloat("##vertical_offset", &scrollAreaOptions->offsetY, 1.0f, 10.0f)) {
				scrollAreaOptions->offsetY = std::clamp(scrollAreaOptions->offsetY, -windowHeight / 2.f, windowHeight / 2.f);
				requestSave();
			}
			ImGui::NextColumn();
			
			ImGui::Text("Width");
			ImGui::NextColumn();
			if (ImGui::InputFloat("##width", &scrollAreaOptions->width, 1.0f, 10.0f)) {
				scrollAreaOptions->width = std::clamp(scrollAreaOptions->width, 1.f, (float)windowWidth);
				requestSave();
			}
			ImGui::NextColumn();
			
			ImGui::Text("Height");
			ImGui::NextColumn();
			if (ImGui::InputFloat("##height", &scrollAreaOptions->height, 1.0f, 10.0f)) {
				scrollAreaOptions->height = std::clamp(scrollAreaOptions->height, 1.f, (float)windowHeight);
				requestSave();
			}
			ImGui::NextColumn();
			
			ImGui::Text("Text Align");
			ImGui::NextColumn();
			if (ImGui::Combo("##text_align", (int*)&scrollAreaOptions->textAlign, TextAlignTexts, 3)) {
				requestSave();
			}
			ImGui::NextColumn();
			
			ImGui::Text("Text Flow");
			ImGui::NextColumn();
			if (ImGui::Combo("##text_flow", (int*)&scrollAreaOptions->textCurve, TextCurveTexts, 5)) {
				requestSave();
			}
			ImGui::NextColumn();
			
			ImGui::Text(langString(GW2_SCT::LanguageCategory::Scroll_Area_Option_UI, GW2_SCT::LanguageKey::Scroll_Direction));
			ImGui::NextColumn();
			if (ImGui::Combo("##scroll_direction", (int*)&scrollAreaOptions->scrollDirection, ScrollDirectionTexts, 2)) {
				requestSave();
			}
			
			ImGui::Columns(1);

			// Advanced Options - collapsible section
			if (ImGui::CollapsingHeader(langString(GW2_SCT::LanguageCategory::Scroll_Area_Option_UI, GW2_SCT::LanguageKey::Advanced_Options))) {
				if (ImGui::Checkbox(langString(GW2_SCT::LanguageCategory::Scroll_Area_Option_UI, GW2_SCT::LanguageKey::Abbreviate_Skill_Names), &scrollAreaOptions->abbreviateSkillNames)) {
					requestSave();
				}
				{
					const char* precisionOptions[] = { "Off", "0 digits", "1 digit", "2 digits", "3 digits" };
					int currentSelection = scrollAreaOptions->shortenNumbersPrecision + 1;
					if (currentSelection < 0) currentSelection = 0;
					if (currentSelection > 4) currentSelection = 4;
					
					ImGui::SetNextItemWidth(120);
					if (ImGui::Combo(langString(GW2_SCT::LanguageCategory::Scroll_Area_Option_UI, GW2_SCT::LanguageKey::Shorten_Numbers), &currentSelection, precisionOptions, IM_ARRAYSIZE(precisionOptions))) {
						scrollAreaOptions->shortenNumbersPrecision = currentSelection - 1;
						requestSave();
					}
					
					ImGui::SameLine();
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
					if (currentSelection == 0) {
						ImGui::Text("(off)");
					} else if (currentSelection == 1) {
						ImGui::Text("(1234 -> 1k)");
					} else if (currentSelection == 2) {
						ImGui::Text("(1234 -> 1.2k)");
					} else if (currentSelection == 3) {
						ImGui::Text("(1234 -> 1.23k)");
					} else if (currentSelection == 4) {
						ImGui::Text("(1234 -> 1.234k)");
					}
					ImGui::PopStyleColor();
				}
                if (ImGui::Checkbox(langString(GW2_SCT::LanguageCategory::Scroll_Area_Option_UI, GW2_SCT::LanguageKey::Disable_Message_Combining), &scrollAreaOptions->disableCombining)) {
                    requestSave();
                }
                if (ImGui::Checkbox(langString(GW2_SCT::LanguageCategory::Scroll_Area_Option_UI, GW2_SCT::LanguageKey::Show_Combined_Hit_Count), &scrollAreaOptions->showCombinedHitCount)) {
                    requestSave();
                }
                if (ImGui::Checkbox(langString(GW2_SCT::LanguageCategory::Scroll_Area_Option_UI, GW2_SCT::LanguageKey::Merge_Crit_With_Hit), &scrollAreaOptions->mergeCritWithHit)) {
                    requestSave();
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("%s", langString(GW2_SCT::LanguageCategory::Scroll_Area_Option_UI, GW2_SCT::LanguageKey::Merge_Crit_With_Hit_Tooltip));
                }
            {
                bool useCustomSpeed = scrollAreaOptions->customScrollSpeed > 0.0f;
                if (ImGui::Checkbox(langString(GW2_SCT::LanguageCategory::Scroll_Area_Option_UI, GW2_SCT::LanguageKey::Custom_Scroll_Speed), &useCustomSpeed)) {
                    scrollAreaOptions->customScrollSpeed = useCustomSpeed ? 90.0f : -1.0f;
                    requestSave();
                }
                ImGui::SameLine();
                if (!useCustomSpeed) ImGui::BeginDisabled();
                ImGui::SetNextItemWidth(120);
                float speed = (scrollAreaOptions->customScrollSpeed > 0.0f ? scrollAreaOptions->customScrollSpeed : 90.0f);
                if (ImGui::ClampingDragFloat("##customspeed", &speed, 1.0f, 1.0f, 2000.0f)) {
                    scrollAreaOptions->customScrollSpeed = speed;
                    requestSave();
                }
                if (!useCustomSpeed) ImGui::EndDisabled();
                
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
                if (!useCustomSpeed) {
                    ImGui::Text("(uses global: %.0f)", currentProfile->scrollSpeed);
                } else {
                    ImGui::Text("(%.0f px/s)", scrollAreaOptions->customScrollSpeed);
                }
                ImGui::PopStyleColor();
            }

            {
                bool useCustomOpacity = scrollAreaOptions->opacityOverrideEnabled;
                if (ImGui::Checkbox(langString(GW2_SCT::LanguageCategory::Scroll_Area_Option_UI, GW2_SCT::LanguageKey::Custom_Opacity), &useCustomOpacity)) {
                    scrollAreaOptions->opacityOverrideEnabled = useCustomOpacity;
                    requestSave();
                }
                ImGui::SameLine();
                if (!useCustomOpacity) ImGui::BeginDisabled();
                ImGui::SetNextItemWidth(120);
                if (ImGui::SliderFloat("##area_opacity", &scrollAreaOptions->opacity, 0.0f, 1.0f, "%.2f")) {
                    requestSave();
                }
                if (!useCustomOpacity) ImGui::EndDisabled();
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
                if (useCustomOpacity) {
                    ImGui::Text("(override %.0f%%)", scrollAreaOptions->opacity * 100.0f);
                } else {
                    ImGui::Text("(inherits global: %.0f%%)", currentProfile->globalOpacity * 100.0f);
                }
                ImGui::PopStyleColor();
            }
				
				ImGui::Separator();
				ImGui::Text("Animation & Spacing");
				
				{
					ImGui::SetNextItemWidth(120);
					if (ImGui::DragFloat("Min Line Spacing (px)", &scrollAreaOptions->minLineSpacingPx, 0.1f, 0.0f, 50.0f)) {
						requestSave();
					}
					ImGui::SameLine();
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
					ImGui::Text("(%.1fpx)", scrollAreaOptions->minLineSpacingPx);
					ImGui::PopStyleColor();
				}
				
				{
					ImGui::SetNextItemWidth(120);
					if (ImGui::SliderFloat("Queue Speedup Factor", &scrollAreaOptions->queueSpeedupFactor, 0.0f, 2.0f, "%.2f")) {
						requestSave();
					}
					if (ImGui::IsItemHovered()) {
						ImGui::SetTooltip("How much to speed up when messages queue. 0.5 = up to 1.5x speed, 1.0 = up to 2.0x speed.");
					}
					ImGui::SameLine();
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
					ImGui::Text("(up to %.1fx speed)", 1.0f + scrollAreaOptions->queueSpeedupFactor);
					ImGui::PopStyleColor();
				}
				
				{
					ImGui::SetNextItemWidth(120);
					if (ImGui::SliderFloat("Speedup Smoothing", &scrollAreaOptions->queueSpeedupSmoothingTau, 0.05f, 1.0f, "%.2fs")) {
						requestSave();
					}
					if (ImGui::IsItemHovered()) {
						ImGui::SetTooltip("How quickly speed changes. Lower = more responsive, Higher = smoother.");
					}
					ImGui::SameLine();
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
					ImGui::Text("(%.2fs)", scrollAreaOptions->queueSpeedupSmoothingTau);
					ImGui::PopStyleColor();
				}
				
				if (scrollAreaOptions->textCurve == TextCurve::STATIC) {
					ImGui::SetNextItemWidth(120);
					if (ImGui::DragFloat("Static Display Time (ms)", &scrollAreaOptions->staticDisplayTimeMs, 10.0f, 100.0f, 10000.0f)) {
						requestSave();
					}
					ImGui::SameLine();
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
					ImGui::Text("(%.0fms)", scrollAreaOptions->staticDisplayTimeMs);
					ImGui::PopStyleColor();
				}
				
				if (scrollAreaOptions->textCurve == TextCurve::ANGLED) {
					ImGui::SetNextItemWidth(120);
					if (ImGui::SliderFloat(langString(GW2_SCT::LanguageCategory::Scroll_Area_Option_UI, GW2_SCT::LanguageKey::Angled_Angle_Degrees), &scrollAreaOptions->angleDegrees, 0.0f, 45.0f, "%.1f°")) {
						requestSave();
					}
					
					ImGui::SetNextItemWidth(120);
					if (ImGui::SliderFloat(langString(GW2_SCT::LanguageCategory::Scroll_Area_Option_UI, GW2_SCT::LanguageKey::Angled_Angle_Jitter), &scrollAreaOptions->angleJitterDegrees, 0.0f, 15.0f, "%.1f°")) {
						requestSave();
					}
					
					const char* directionTexts[] = { 
						langStringG(GW2_SCT::LanguageKey::Angled_Direction_Alternating), 
						langStringG(GW2_SCT::LanguageKey::Angled_Direction_Always_Left), 
						langStringG(GW2_SCT::LanguageKey::Angled_Direction_Always_Right) 
					};
					int directionIndex = (scrollAreaOptions->angledDirection == 0) ? 0 : (scrollAreaOptions->angledDirection > 0 ? 2 : 1);
					ImGui::SetNextItemWidth(120);
					if (ImGui::Combo(langString(GW2_SCT::LanguageCategory::Scroll_Area_Option_UI, GW2_SCT::LanguageKey::Angled_Direction), &directionIndex, directionTexts, 3)) {
						scrollAreaOptions->angledDirection = (directionIndex == 0) ? 0 : (directionIndex == 2 ? 1 : -1);
						requestSave();
					}
				}
			}

            ImGui::Text(langString(GW2_SCT::LanguageCategory::Scroll_Area_Option_UI, GW2_SCT::LanguageKey::All_Receivers));
            int receiverOptionsCounter = 0;
            auto receiverOptionsIterator = std::begin(scrollAreaOptions->receivers);
            while (receiverOptionsIterator != std::end(scrollAreaOptions->receivers)) {
                // Hide crit receivers if merging crit with hit
                if (scrollAreaOptions->mergeCritWithHit && (*receiverOptionsIterator)->type == MessageType::CRIT) {
                    receiverOptionsIterator++;
                    continue;
                }

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
            // Hide CRIT option when merging crit into hit
            ImGui::SetNewReceiverHideCrit(scrollAreaOptions->mergeCritWithHit);
            if (ImGui::NewReceiverLine(&newReceiverCategory, &newReceiverType)) {
                auto& defaultReceiver = receiverInformationPerCategoryAndType.at(newReceiverCategory).at(newReceiverType).defaultReceiver;
                scrollAreaOptions->receivers.push_back(std::make_shared<message_receiver_options_struct>(defaultReceiver));
                requestSave();
            }
            ImGui::SetNewReceiverHideCrit(false);
		}
		else {
			for (auto scrollAreaOptions : currentProfile->scrollAreaOptions) {
				if (scrollAreaOptions->outlineState != ScrollAreaOutlineState::FULL) scrollAreaOptions->outlineState = ScrollAreaOutlineState::FULL;
			}
		}
		ImGui::EndChild();
		ImGui::EndGroup();
	}
}

void GW2_SCT::Options::paintScrollAreaOverlay(const std::vector<std::shared_ptr<ScrollArea>>& scrollAreas) {
	if (!windowIsOpen || !inScrollAreasTab) return;

	ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
	ImGui::SetNextWindowSize(ImVec2((float)windowWidth, (float)windowHeight));
	ImGuiWindowFlags drawFlags =
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoBackground |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_NoInputs;

	ImGui::Begin("##ScrollAreaOverlay_Draw", nullptr, drawFlags);

	struct Rect {
		std::shared_ptr<scroll_area_options_struct> opt;
		ImVec2 pos;
		ImVec2 size;
	};

	std::vector<Rect> rects;
	rects.reserve(scrollAreas.size());

	ImDrawList* dl = ImGui::GetWindowDrawList();

	for (auto& sa : scrollAreas) {
		auto o = sa->getOptions();
		if (!o) continue;
		if (o->outlineState == ScrollAreaOutlineState::NONE) continue;

		const float x = windowWidth * 0.5f + o->offsetX;
		const float y = windowHeight * 0.5f + o->offsetY;
		const float w = o->width;
		const float h = o->height;

		rects.push_back(Rect{ o, ImVec2(x, y), ImVec2(w, h) });

		const bool full = (o->outlineState == ScrollAreaOutlineState::FULL);
		const ImU32 fillCol = ImGui::GetColorU32(full ? ImVec4(0.15f, 0.15f, 0.15f, 0.66f)
			: ImVec4(0.15f, 0.15f, 0.15f, 0.33f));
		const ImU32 rectCol = ImGui::GetColorU32(full ? ImVec4(1.00f, 1.00f, 1.00f, 0.66f)
			: ImVec4(1.00f, 1.00f, 1.00f, 0.33f));
		const ImU32 textCol = ImGui::GetColorU32(full ? ImVec4(1.00f, 1.00f, 1.00f, 0.90f)
			: ImVec4(1.00f, 1.00f, 1.00f, 0.50f));

		dl->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h), fillCol);
		dl->AddRect(ImVec2(x, y), ImVec2(x + w, y + h), rectCol);

		if (!o->name.empty()) {
			ImVec2 name_sz = ImGui::CalcTextSize(o->name.c_str());
			dl->AddText(ImVec2(x + (w - name_sz.x) * 0.5f, y + (h - name_sz.y) * 0.5f), textCol, o->name.c_str());
		}

		char buf[64];
		std::snprintf(buf, sizeof(buf), "X: %.0f, Y: %.0f", o->offsetX, o->offsetY);
		ImVec2 off_sz = ImGui::CalcTextSize(buf);
		dl->AddText(ImVec2(x + (w - off_sz.x) * 0.5f, y + 5.0f), textCol, buf);
	}

	ImGui::End();

	static std::shared_ptr<scroll_area_options_struct> dragged = nullptr;
	static ImVec2 dragStartMouse(0.0f, 0.0f);
	static ImVec2 dragStartOffset(0.0f, 0.0f);

	for (const Rect& r : rects) {
		if (!r.opt || r.opt->outlineState != ScrollAreaOutlineState::FULL)
			continue;

		ImGui::SetNextWindowPos(r.pos);
		ImGui::SetNextWindowSize(r.size);
		ImGuiWindowFlags hitFlags =
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoBringToFrontOnFocus |
			ImGuiWindowFlags_NoBackground |
			ImGuiWindowFlags_NoFocusOnAppearing;

		char winName[64];
		std::snprintf(winName, sizeof(winName), "##ScrollAreaHit_%p", (void*)r.opt.get());

		ImGui::Begin(winName, nullptr, hitFlags);

		const bool hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_None);

		ImGuiIO& io = ImGui::GetIO();

		if (!dragged && hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
			dragged = r.opt;
			dragStartMouse = io.MousePos;
			dragStartOffset = ImVec2(r.opt->offsetX, r.opt->offsetY);
		}

		if (dragged == r.opt) {
			const float dx = io.MousePos.x - dragStartMouse.x;
			const float dy = io.MousePos.y - dragStartMouse.y;

			r.opt->offsetX = dragStartOffset.x + dx;
			r.opt->offsetY = dragStartOffset.y + dy;

			const float minX = -windowWidth * 0.5f;
			const float maxX = windowWidth * 0.5f - r.size.x;
			const float minY = -windowHeight * 0.5f;
			const float maxY = windowHeight * 0.5f - r.size.y;

			r.opt->offsetX = std::max(minX, std::min(r.opt->offsetX, maxX));
			r.opt->offsetY = std::max(minY, std::min(r.opt->offsetY, maxY));

			requestSave();

			if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
				dragged = nullptr;
			}
		}

		if (hovered || dragged == r.opt)
			ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);

		ImGui::End();
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
	auto currentProfile = Profiles::get();
	if (!currentProfile) return;
	
	drawColorSelector(langString(LanguageCategory::Option_UI, LanguageKey::Profession_Colors_Guardian), currentProfile->professionColorGuardian);
	drawColorSelector(langString(LanguageCategory::Option_UI, LanguageKey::Profession_Colors_Warrior), currentProfile->professionColorWarrior);
	drawColorSelector(langString(LanguageCategory::Option_UI, LanguageKey::Profession_Colors_Engineer), currentProfile->professionColorEngineer);
	drawColorSelector(langString(LanguageCategory::Option_UI, LanguageKey::Profession_Colors_Ranger), currentProfile->professionColorRanger);
	drawColorSelector(langString(LanguageCategory::Option_UI, LanguageKey::Profession_Colors_Thief), currentProfile->professionColorThief);
	drawColorSelector(langString(LanguageCategory::Option_UI, LanguageKey::Profession_Colors_Elementalist), currentProfile->professionColorElementalist);
	drawColorSelector(langString(LanguageCategory::Option_UI, LanguageKey::Profession_Colors_Mesmer), currentProfile->professionColorMesmer);
	drawColorSelector(langString(LanguageCategory::Option_UI, LanguageKey::Profession_Colors_Necromancer), currentProfile->professionColorNecromancer);
	drawColorSelector(langString(LanguageCategory::Option_UI, LanguageKey::Profession_Colors_Revenant), currentProfile->professionColorRevenant);
	drawColorSelector(langString(LanguageCategory::Option_UI, LanguageKey::Profession_Colors_Undetectable), currentProfile->professionColorDefault);
}


void GW2_SCT::Options::paintGlobalThresholds() {
	auto currentProfile = Profiles::get();
	if (!currentProfile) return;

	ImGui::Text(langString(LanguageCategory::Option_UI, LanguageKey::Menu_Bar_Global_Thresholds));
	ImGui::Separator();
	
	ImGui::PushTextWrapPos(ImGui::GetContentRegionAvail().x);
	ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), langString(LanguageCategory::Option_UI, LanguageKey::Global_Thresholds_Description));
	ImGui::PopTextWrapPos();
	
	ImGui::Spacing();
	
    {
        std::string label = std::string(langString(LanguageCategory::Option_UI, LanguageKey::Receiver_Thresholds_Enable)) + "##global-thresholds-enabled";
        if (ImGui::Checkbox(label.c_str(), &currentProfile->globalThresholdsEnabled)) {
            requestSave();
        }
    }
	
	if (!currentProfile->globalThresholdsEnabled) {
		ImGui::BeginDisabled();
	}
	
    {
        std::string label = std::string(langString(LanguageCategory::Option_UI, LanguageKey::Receiver_Threshold_Respect_Filters)) + "##global-threshold-respect-filters";
        if (ImGui::Checkbox(label.c_str(), &currentProfile->globalThresholdRespectFilters)) {
            requestSave();
        }
    }
	if (ImGui::IsItemHovered()) {
		ImGui::BeginTooltip();
		ImGui::PushTextWrapPos(300.0f);
		ImGui::Text(langString(LanguageCategory::Option_UI, LanguageKey::Global_Threshold_Respect_Filters_Tooltip));
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
	
	ImGui::Spacing();
	ImGui::Text("Threshold Values:");
	
    {
        std::string label = std::string(langString(LanguageCategory::Option_UI, LanguageKey::Receiver_Damage_Threshold)) + "##global-damage-threshold";
        if (ImGui::ClampingDragInt(label.c_str(), &currentProfile->globalDamageThreshold, 1.0f, 0, 999999, "%d")) {
            requestSave();
        }
    }
	ImGui::SameLine();
	ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "(Physical, Crit, DoT, etc.)");
	
    {
        std::string label = std::string(langString(LanguageCategory::Option_UI, LanguageKey::Receiver_Heal_Threshold)) + "##global-heal-threshold";
        if (ImGui::ClampingDragInt(label.c_str(), &currentProfile->globalHealThreshold, 1.0f, 0, 999999, "%d")) {
            requestSave();
        }
    }
	ImGui::SameLine();
	ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "(Heal, HoT)");
	
    {
        std::string label = std::string(langString(LanguageCategory::Option_UI, LanguageKey::Receiver_Barrier_Threshold)) + "##global-absorb-threshold";
        if (ImGui::ClampingDragInt(label.c_str(), &currentProfile->globalAbsorbThreshold, 1.0f, 0, 999999, "%d")) {
            requestSave();
        }
    }
	ImGui::SameLine();
	ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "(Shield, Barrier)");
	
	if (!currentProfile->globalThresholdsEnabled) {
		ImGui::EndDisabled();
	}
}

void GW2_SCT::Options::paintSkillIcons() {
	auto currentProfile = Profiles::get();
	if (!currentProfile) return;
	
	std::string language_warning(langString(LanguageCategory::Skill_Icons_Option_UI, LanguageKey::Skill_Icons_Warning));
	ImGui::TextWrapped((language_warning + " '" + getSCTPath() + "icons\\" + "'").c_str());
	if (ImGui::Checkbox(langString(LanguageCategory::Skill_Icons_Option_UI, LanguageKey::Skill_Icons_Enable), &currentProfile->skillIconsEnabled)) {
		requestSave();
	}
	ImGui::TextWrapped(langString(LanguageCategory::Skill_Icons_Option_UI, LanguageKey::Skill_Icons_Preload_Description));
	if (ImGui::Checkbox(langString(LanguageCategory::Skill_Icons_Option_UI, LanguageKey::Skill_Icons_Preload), &currentProfile->preloadAllSkillIcons)) {
		requestSave();
	}
	ImGui::Text("");
	if (ImGui::BeginCombo(
		ImGui::BuildVisibleLabel(langString(LanguageCategory::Skill_Icons_Option_UI, LanguageKey::Skill_Icons_Display_Type), "skill-icons-display-combo").c_str(),
		skillIconsDisplayTypeNames.at(currentProfile->skillIconsDisplayType).c_str())
		) {
		int i = 0;
		for (auto& skillIconDisplayTypeAndName : skillIconsDisplayTypeNames) {
			if (ImGui::Selectable(ImGui::BuildLabel(skillIconDisplayTypeAndName.second, "skill-icons-display-selectable", i).c_str())) {
				if (currentProfile->skillIconsDisplayType != skillIconDisplayTypeAndName.first) {
					currentProfile->skillIconsDisplayType = skillIconDisplayTypeAndName.first;
					requestSave();
				}
			}
			i++;
		}
		ImGui::EndCombo();
	}
}

