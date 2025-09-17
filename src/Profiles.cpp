#include "Profiles.h"
#include <sstream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <mutex>
#include <thread>
#include "json.hpp"
#include "Common.h"
#include "imgui.h"
#include "Language.h"
#include "Mumblelink.h"
#include "imgui_stdlib.h"
#include "imgui_sct_widgets.h"
#include "SkillFilterStructures.h"
#include "Options.h"

// Forward declarations
namespace GW2_SCT {
	extern const std::map<MessageCategory, const std::map<MessageType, receiver_information>> receiverInformationPerCategoryAndType;
}


ObservableValue<std::shared_ptr<GW2_SCT::profile_options_struct>> GW2_SCT::Profiles::profile = std::shared_ptr<GW2_SCT::profile_options_struct>();

const std::string GW2_SCT::Profiles::defaultProfileName = "default";
std::string GW2_SCT::Profiles::currentProfileName = defaultProfileName;
std::string GW2_SCT::Profiles::currentCharacterName = "";

std::mutex GW2_SCT::Profiles::profileSwitchMutex;
std::shared_ptr<GW2_SCT::profile_options_struct> GW2_SCT::Profiles::pendingProfile;

void GW2_SCT::Profiles::requestSwitch(std::shared_ptr<profile_options_struct> newProfile) {
	std::lock_guard<std::mutex> lock(profileSwitchMutex);
	pendingProfile = newProfile;
}

void GW2_SCT::Profiles::processPendingSwitch() {
	std::lock_guard<std::mutex> lock(profileSwitchMutex);
	if (pendingProfile) {
		profile = pendingProfile;
		pendingProfile.reset();
	}
}

void GW2_SCT::Profiles::loadForCharacter(std::string characterName) {
	if (currentCharacterName != characterName) {
		currentCharacterName = characterName;
	}
	
	auto& options = Options::getOptionsStruct();
	std::string newProfileName;
	bool isInWvW = MumbleLink::i().isInWvW();
	
	if (isInWvW) {
		auto it = options.characterWvwProfileMap.find(currentCharacterName);
		if (it != options.characterWvwProfileMap.end()) {
			newProfileName = it->second;
		} else {
			newProfileName = options.wvwDefaultProfile;
		}
	} else {
		auto it = options.characterPveProfileMap.find(currentCharacterName);
		if (it != options.characterPveProfileMap.end()) {
			newProfileName = it->second;
		} else {
			newProfileName = options.pveDefaultProfile;
		}
	}
	
	if (currentProfileName != newProfileName) {
		currentProfileName = newProfileName;
		requestSwitch(options.profiles[currentProfileName]);
	} else if (!get()) {
		requestSwitch(options.profiles[currentProfileName]);
	}
}

void GW2_SCT::Profiles::initWithDefaults(std::shared_ptr<profile_options_struct> p) {
	p->scrollAreaOptions.clear();

	// --- Incoming area ---
	auto incomingStruct = std::make_shared<scroll_area_options_struct>();
	incomingStruct->name = std::string(langStringG(LanguageKey::Default_Scroll_Area_Incoming));
	incomingStruct->offsetX = -249.f;
	incomingStruct->offsetY = -25.f;
	incomingStruct->width = 40.f;
	incomingStruct->height = 260.f;
	incomingStruct->textAlign = TextAlign::RIGHT;
	incomingStruct->textCurve = TextCurve::LEFT;
	incomingStruct->scrollDirection = ScrollDirection::DOWN;

	// persisted toggles
	incomingStruct->showCombinedHitCount = true;
	incomingStruct->enabled = true;
	incomingStruct->abbreviateSkillNames = false;
	incomingStruct->shortenNumbersPrecision = -1;  // off
	incomingStruct->disableCombining = false;
	incomingStruct->customScrollSpeed = -1.0f;  // use global

	// runtime UI outline
	incomingStruct->outlineState = ScrollAreaOutlineState::NONE;

	// receivers
	incomingStruct->receivers.clear();
	for (const auto& kv : receiverInformationPerCategoryAndType.at(MessageCategory::PLAYER_IN)) {
		incomingStruct->receivers.push_back(
			std::make_shared<message_receiver_options_struct>(kv.second.defaultReceiver));
	}
	for (const auto& kv : receiverInformationPerCategoryAndType.at(MessageCategory::PET_IN)) {
		incomingStruct->receivers.push_back(
			std::make_shared<message_receiver_options_struct>(kv.second.defaultReceiver));
	}
	p->scrollAreaOptions.push_back(incomingStruct);

	// --- Outgoing area ---
	auto outgoingStruct = std::make_shared<scroll_area_options_struct>();
	outgoingStruct->name = std::string(langStringG(LanguageKey::Default_Scroll_Area_Outgoing));
	outgoingStruct->offsetX = 217.f;
	outgoingStruct->offsetY = -25.f;
	outgoingStruct->width = 40.f;
	outgoingStruct->height = 260.f;
	outgoingStruct->textAlign = TextAlign::LEFT;
	outgoingStruct->textCurve = TextCurve::RIGHT;
	outgoingStruct->scrollDirection = ScrollDirection::DOWN;

	// persisted toggles
	outgoingStruct->showCombinedHitCount = true;
	outgoingStruct->enabled = true;
	outgoingStruct->abbreviateSkillNames = false;
	outgoingStruct->shortenNumbersPrecision = -1;  // off
	outgoingStruct->disableCombining = false;
	outgoingStruct->customScrollSpeed = -1.0f;  // use global

	// runtime UI outline
	outgoingStruct->outlineState = ScrollAreaOutlineState::NONE;

	// receivers
	outgoingStruct->receivers.clear();
	for (const auto& kv : receiverInformationPerCategoryAndType.at(MessageCategory::PLAYER_OUT)) {
		outgoingStruct->receivers.push_back(
			std::make_shared<message_receiver_options_struct>(kv.second.defaultReceiver));
	}
	for (const auto& kv : receiverInformationPerCategoryAndType.at(MessageCategory::PET_OUT)) {
		outgoingStruct->receivers.push_back(
			std::make_shared<message_receiver_options_struct>(kv.second.defaultReceiver));
	}
	p->scrollAreaOptions.push_back(outgoingStruct);
}

void GW2_SCT::Profiles::paintUI() {
	auto& options = Options::getOptionsStruct();
	bool doesCharacterMappingExist = currentCharacterName != "" && options.characterProfileMap.find(currentCharacterName) != options.characterProfileMap.end();

	ImGui::Spacing();

	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.2f, 0.4f, 0.2f, 0.3f));
	if (ImGui::BeginChild("ActiveProfileStatus", ImVec2(0, 120), true)) {
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 8.0f));
		ImGui::Text(langString(LanguageCategory::Profile_Option_UI, LanguageKey::Active_Profile_Title));
		ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
		ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), "%s", currentProfileName.c_str());
		ImGui::PopFont();
		
		if (currentCharacterName != "") {
			bool isInWvW = MumbleLink::i().isInWvW();
			bool hasPveOverride = options.characterPveProfileMap.find(currentCharacterName) != options.characterPveProfileMap.end();
			bool hasWvwOverride = options.characterWvwProfileMap.find(currentCharacterName) != options.characterWvwProfileMap.end();
			bool hasOverride = isInWvW ? hasWvwOverride : hasPveOverride;
			
			if (isInWvW) {
				ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.7f, 1.0f), "WvW Mode - Character: %s", currentCharacterName.c_str());
			} else {
				ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.9f, 1.0f), "PvE Mode - Character: %s", currentCharacterName.c_str());
			}
			
			if (hasOverride) {
				std::string overrideType = isInWvW ? "WvW" : "PvE";
				ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.4f, 1.0f), "Source: %s Character Override", overrideType.c_str());
			} else {
				std::string defaultType = isInWvW ? "WvW" : "PvE";
				ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Source: %s Default Profile", defaultType.c_str());
			}
		} else {
			ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), langString(LanguageCategory::Profile_Option_UI, LanguageKey::Profile_Source_No_Character));
		}
		
		auto currentProfile = profile.operator std::shared_ptr<profile_options_struct>();
		if (currentProfile && !currentProfile->scrollAreaOptions.empty()) {
			ImGui::Spacing();
			ImGui::Text(langString(LanguageCategory::Profile_Option_UI, LanguageKey::Scroll_Areas_Count), (int)currentProfile->scrollAreaOptions.size());
			ImGui::Indent();
			for (auto& area : currentProfile->scrollAreaOptions) {
				if (area->enabled) {
					ImGui::TextColored(ImVec4(0.7f, 0.9f, 0.7f, 1.0f), "• %s", area->name.c_str());
				} else {
					ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "• %s (disabled)", area->name.c_str());
				}
			}
			ImGui::Unindent();
		} else {
			ImGui::Spacing();
			ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), langString(LanguageCategory::Profile_Option_UI, LanguageKey::No_Scroll_Areas));
		}
		ImGui::PopStyleVar();
	}
	ImGui::EndChild();
	ImGui::PopStyleColor();
	
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();
	

	ImGui::Text(langString(LanguageCategory::Profile_Option_UI, LanguageKey::Profile_Management_Title));
	ImGui::Spacing();

	ImGui::Text("%s", Language::get(LanguageCategory::Profile_Option_UI, LanguageKey::Default_Profiles_Title));
	ImGui::Indent();
	
	ImGui::Text("%s", Language::get(LanguageCategory::Profile_Option_UI, LanguageKey::PvE_Default_Profile));
	ImGui::SameLine(100);
	ImGui::SetNextItemWidth(200);
	if (ImGui::BeginCombo("##PvEDefault", options.pveDefaultProfile.c_str())) {
		for (auto& nameAndProfile : options.profiles) {
			if (ImGui::Selectable((nameAndProfile.first + "##pve-default-selectable").c_str())) {
				if (options.pveDefaultProfile != nameAndProfile.first) {
					options.pveDefaultProfile = nameAndProfile.first;
					if (!MumbleLink::i().isInWvW() && !doesCharacterMappingExist) {
						currentProfileName = nameAndProfile.first;
						requestSwitch(nameAndProfile.second);
					}
					Options::requestSave();
				}
			}
			if (ImGui::IsItemHovered()) {
				ImGui::BeginTooltip();
				ImGui::Text("Profile: %s", nameAndProfile.first.c_str());
				ImGui::Text("Scroll Areas: %d", (int)nameAndProfile.second->scrollAreaOptions.size());
				if (!nameAndProfile.second->scrollAreaOptions.empty()) {
					ImGui::Separator();
					for (auto& area : nameAndProfile.second->scrollAreaOptions) {
						if (area->enabled) {
							ImGui::Text("• %s", area->name.c_str());
						} else {
							ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "• %s", area->name.c_str());
						}
					}
				}
				ImGui::EndTooltip();
			}
		}
		ImGui::EndCombo();
	}

	ImGui::Text("%s", Language::get(LanguageCategory::Profile_Option_UI, LanguageKey::WvW_Default_Profile));
	ImGui::SameLine(100);
	ImGui::SetNextItemWidth(200);
	if (ImGui::BeginCombo("##WvWDefault", options.wvwDefaultProfile.c_str())) {
		for (auto& nameAndProfile : options.profiles) {
			if (ImGui::Selectable((nameAndProfile.first + "##wvw-default-selectable").c_str())) {
				if (options.wvwDefaultProfile != nameAndProfile.first) {
					options.wvwDefaultProfile = nameAndProfile.first;
					if (MumbleLink::i().isInWvW() && options.characterWvwProfileMap.find(currentCharacterName) == options.characterWvwProfileMap.end()) {
						currentProfileName = nameAndProfile.first;
						requestSwitch(nameAndProfile.second);
					}
					Options::requestSave();
				}
			}
			if (ImGui::IsItemHovered()) {
				ImGui::BeginTooltip();
				ImGui::Text("Profile: %s", nameAndProfile.first.c_str());
				ImGui::Text("Scroll Areas: %d", (int)nameAndProfile.second->scrollAreaOptions.size());
				if (!nameAndProfile.second->scrollAreaOptions.empty()) {
					ImGui::Separator();
					for (auto& area : nameAndProfile.second->scrollAreaOptions) {
						if (area->enabled) {
							ImGui::Text("• %s", area->name.c_str());
						} else {
							ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "• %s", area->name.c_str());
						}
					}
				}
				ImGui::EndTooltip();
			}
		}
		ImGui::EndCombo();
	}
	
	ImGui::Unindent();
	
	ImGui::Spacing();
	
	if (currentCharacterName != "") {
		ImGui::Text("%s", Language::get(LanguageCategory::Profile_Option_UI, LanguageKey::Character_Specific_Overrides_Title));
		ImGui::TextColored(ImVec4(0.7f, 0.9f, 0.7f, 1.0f), "Character: %s", currentCharacterName.c_str());
		if (MumbleLink::i().isInWvW()) {
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.7f, 1.0f), "(in WvW)");
		} else {
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.9f, 1.0f), "(in PvE)");
		}
		
		ImGui::Indent();
		
		bool hasPveOverride = options.characterPveProfileMap.find(currentCharacterName) != options.characterPveProfileMap.end();
		if (ImGui::Checkbox(Language::get(LanguageCategory::Profile_Option_UI, LanguageKey::PvE_Override_Checkbox), &hasPveOverride)) {
			if (hasPveOverride) {
				options.characterPveProfileMap[currentCharacterName] = currentProfileName;
			} else {
				options.characterPveProfileMap.erase(currentCharacterName);
				if (!MumbleLink::i().isInWvW()) {
					currentProfileName = options.pveDefaultProfile;
					requestSwitch(options.profiles[currentProfileName]);
				}
			}
			Options::requestSave();
		}
		
		if (hasPveOverride) {
			ImGui::Indent();
			std::string currentPveProfile = options.characterPveProfileMap[currentCharacterName];
			ImGui::Text("%s", Language::get(LanguageCategory::Profile_Option_UI, LanguageKey::Profile_Label));
			ImGui::SameLine(120);
			ImGui::SetNextItemWidth(180);
			if (ImGui::BeginCombo("##CharacterPvEProfile", currentPveProfile.c_str())) {
				for (auto& nameAndProfile : options.profiles) {
					if (ImGui::Selectable((nameAndProfile.first + "##character-pve-profile-selectable").c_str())) {
						options.characterPveProfileMap[currentCharacterName] = nameAndProfile.first;
						if (!MumbleLink::i().isInWvW()) {
							currentProfileName = nameAndProfile.first;
							requestSwitch(nameAndProfile.second);
						}
						Options::requestSave();
					}
					if (ImGui::IsItemHovered()) {
						ImGui::BeginTooltip();
						ImGui::Text("Profile: %s", nameAndProfile.first.c_str());
						ImGui::Text("Scroll Areas: %d", (int)nameAndProfile.second->scrollAreaOptions.size());
						ImGui::EndTooltip();
					}
				}
				ImGui::EndCombo();
			}
			ImGui::Unindent();
		}
		
		bool hasWvwOverride = options.characterWvwProfileMap.find(currentCharacterName) != options.characterWvwProfileMap.end();
		if (ImGui::Checkbox(Language::get(LanguageCategory::Profile_Option_UI, LanguageKey::WvW_Override_Checkbox), &hasWvwOverride)) {
			if (hasWvwOverride) {
				options.characterWvwProfileMap[currentCharacterName] = currentProfileName;
			} else {
				options.characterWvwProfileMap.erase(currentCharacterName);
				if (MumbleLink::i().isInWvW()) {
					currentProfileName = options.wvwDefaultProfile;
					requestSwitch(options.profiles[currentProfileName]);
				}
			}
			Options::requestSave();
		}
		
		if (hasWvwOverride) {
			ImGui::Indent();
			std::string currentWvwProfile = options.characterWvwProfileMap[currentCharacterName];
			ImGui::Text("%s", Language::get(LanguageCategory::Profile_Option_UI, LanguageKey::Profile_Label));
			ImGui::SameLine(120);
			ImGui::SetNextItemWidth(180);
			if (ImGui::BeginCombo("##CharacterWvWProfile", currentWvwProfile.c_str())) {
				for (auto& nameAndProfile : options.profiles) {
					if (ImGui::Selectable((nameAndProfile.first + "##character-wvw-profile-selectable").c_str())) {
						options.characterWvwProfileMap[currentCharacterName] = nameAndProfile.first;
						if (MumbleLink::i().isInWvW()) {
							currentProfileName = nameAndProfile.first;
							requestSwitch(nameAndProfile.second);
						}
						Options::requestSave();
					}
					if (ImGui::IsItemHovered()) {
						ImGui::BeginTooltip();
						ImGui::Text("Profile: %s", nameAndProfile.first.c_str());
						ImGui::Text("Scroll Areas: %d", (int)nameAndProfile.second->scrollAreaOptions.size());
						ImGui::EndTooltip();
					}
				}
				ImGui::EndCombo();
			}
			ImGui::Unindent();
		}
		
		ImGui::Unindent();
		ImGui::Spacing();
	}
	
	ImGui::Separator();
	ImGui::Spacing();
	
	ImGui::Text("%s", Language::get(LanguageCategory::Profile_Option_UI, LanguageKey::Profile_Actions_Title));
	ImGui::Spacing();
	
	ImGui::Text(langString(LanguageCategory::Profile_Option_UI, LanguageKey::Current_Profile_Heading));
	bool currentProfileIsDefault = currentProfileName == defaultProfileName;
	if (currentProfileIsDefault) {
		ImGui::BeginDisabled();
	}
	std::string nameCopy = currentProfileName;
	bool changedBG = false;
	ImGui::Text(langString(LanguageCategory::Profile_Option_UI, LanguageKey::Current_Profile_Name));
	ImGui::SameLine(100);
	ImGui::SetNextItemWidth(200);
	if (ImGui::InputText("##ProfileName", &nameCopy, ImGuiInputTextFlags_CallbackAlways, [](ImGuiInputTextCallbackData* data) {
		bool* d = static_cast<bool*>(data->UserData);
		std::string text(data->Buf);
		auto& options = Options::getOptionsStruct();
		if (text != currentProfileName && (text == defaultProfileName || options.profiles.find(text) != options.profiles.end())) {
			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(1.f, 0.f, 0.f, .6f));
			*d = true;
		}
		return 0;
		}, &changedBG)) {
		if (!changedBG && nameCopy != currentProfileName) {
			options.profiles[nameCopy] = profile;
			options.profiles.erase(currentProfileName);
			for (auto& characterProfileMapping : options.characterProfileMap) {
				if (characterProfileMapping.second == currentProfileName) {
					characterProfileMapping.second = nameCopy;
				}
			}
			for (auto& characterPveProfileMapping : options.characterPveProfileMap) {
				if (characterPveProfileMapping.second == currentProfileName) {
					characterPveProfileMapping.second = nameCopy;
				}
			}
			for (auto& characterWvwProfileMapping : options.characterWvwProfileMap) {
				if (characterWvwProfileMapping.second == currentProfileName) {
					characterWvwProfileMapping.second = nameCopy;
				}
			}
			if (options.pveDefaultProfile == currentProfileName) {
				options.pveDefaultProfile = nameCopy;
			}
			if (options.wvwDefaultProfile == currentProfileName) {
				options.wvwDefaultProfile = nameCopy;
			}
			currentProfileName = nameCopy;
			Options::requestSave();
		}
	}
	if (changedBG) {
		ImGui::PopStyleColor();
	}
	if (currentProfileIsDefault) {
		ImGui::EndDisabled();
	}
	
	ImGui::Spacing();
	
	ImGui::Text(langString(LanguageCategory::Profile_Option_UI, LanguageKey::Manage_Profiles_Title));
	ImGui::Spacing();
	if (ImGui::Button(ImGui::BuildVisibleLabel(langString(LanguageCategory::Profile_Option_UI, LanguageKey::Create_Profile_Copy), "profile-copy-button").c_str())) {
		std::string copyName = currentProfileName + " " + std::string(langString(LanguageCategory::Profile_Option_UI, LanguageKey::Profile_Copy_Suffix));
		if (options.profiles.find(copyName) != options.profiles.end()) {
			int i = 1;
			while (options.profiles.find(copyName + " " + std::to_string(i)) != options.profiles.end()) { i++; }
			copyName = copyName + " " + std::to_string(i);
		}
		nlohmann::json j = *profile.operator std::shared_ptr<profile_options_struct>().get();
		options.profiles[copyName] = std::make_shared<profile_options_struct>(j);
		for (auto& characterProfileMapping : options.characterProfileMap) {
			if (characterProfileMapping.second == currentProfileName) {
				characterProfileMapping.second = copyName;
			}
		}
		for (auto& characterPveProfileMapping : options.characterPveProfileMap) {
			if (characterPveProfileMapping.second == currentProfileName) {
				characterPveProfileMapping.second = copyName;
			}
		}
		for (auto& characterWvwProfileMapping : options.characterWvwProfileMap) {
			if (characterWvwProfileMapping.second == currentProfileName) {
				characterWvwProfileMapping.second = copyName;
			}
		}
		if (options.pveDefaultProfile == currentProfileName) {
			options.pveDefaultProfile = copyName;
		}
		if (options.wvwDefaultProfile == currentProfileName) {
			options.wvwDefaultProfile = copyName;
		}
		currentProfileName = copyName;
		requestSwitch(options.profiles[currentProfileName]);
		Options::requestSave();
	}
	ImGui::SameLine();
	if (ImGui::Button(ImGui::BuildVisibleLabel(langString(LanguageCategory::Profile_Option_UI, LanguageKey::Create_Profile_New), "profile-new-button").c_str())) {
		std::string newProfileName = Language::get(LanguageCategory::Profile_Option_UI, LanguageKey::New_Profile_Name);
		if (options.profiles.find(newProfileName) != options.profiles.end()) {
			int i = 1;
			while (options.profiles.find(newProfileName + " " + std::to_string(i)) != options.profiles.end()) { i++; }
			newProfileName = newProfileName + " " + std::to_string(i);
		}

		auto newProfile = std::make_shared<profile_options_struct>();
		initWithDefaults(newProfile);

		options.profiles[newProfileName] = newProfile;

		if (doesCharacterMappingExist) {
			options.characterProfileMap[currentCharacterName] = newProfileName;
		}
		else {
			if (MumbleLink::i().isInWvW()) {
				options.wvwDefaultProfile = newProfileName;
			} else {
				options.pveDefaultProfile = newProfileName;
			}
		}
		currentProfileName = newProfileName;
		requestSwitch(newProfile);
		Options::requestSave();
	}
	
	ImGui::SameLine();
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
			if (options.pveDefaultProfile == currentProfileName) {
				options.pveDefaultProfile = defaultProfileName;
			}
			if (options.wvwDefaultProfile == currentProfileName) {
				options.wvwDefaultProfile = defaultProfileName;
			}
			for (auto& characterProfileMapping : options.characterProfileMap) {
				if (characterProfileMapping.second == currentProfileName) {
					characterProfileMapping.second = defaultProfileName;
				}
			}
			for (auto& characterPveProfileMapping : options.characterPveProfileMap) {
				if (characterPveProfileMapping.second == currentProfileName) {
					characterPveProfileMapping.second = defaultProfileName;
				}
			}
			for (auto& characterWvwProfileMapping : options.characterWvwProfileMap) {
				if (characterWvwProfileMapping.second == currentProfileName) {
					characterWvwProfileMapping.second = defaultProfileName;
				}
			}
			options.profiles.erase(currentProfileName);
			currentProfileName = defaultProfileName;
			requestSwitch(options.profiles[currentProfileName]);
			Options::requestSave();
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button(ImGui::BuildVisibleLabel(langString(GW2_SCT::LanguageCategory::Profile_Option_UI, GW2_SCT::LanguageKey::Delete_Confirmation_Cancel), "delete-profile-modal-cancel").c_str(), ImVec2(120, 0))) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}