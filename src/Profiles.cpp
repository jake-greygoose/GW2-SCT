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
#include "imgui_stdlib.h"
#include "imgui_sct_widgets.h"
#include "Language.h"
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
		auto& options = Options::getOptionsStruct();
		auto profileMappingIterator = options.characterProfileMap.find(currentCharacterName);
		if (profileMappingIterator == options.characterProfileMap.end()) {
			if (currentProfileName != options.globalProfile) {
				currentProfileName = options.globalProfile;
				requestSwitch(options.profiles[currentProfileName]);
			} else if (!get()) {
				// Ensure profile is set even if names match
				requestSwitch(options.profiles[currentProfileName]);
			}
		}
		else {
			if (currentProfileName != profileMappingIterator->second) {
				currentProfileName = profileMappingIterator->second;
				requestSwitch(options.profiles[currentProfileName]);
			}
		}
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
	ImGui::Spacing();

	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.2f, 0.4f, 0.2f, 0.3f));
	if (ImGui::BeginChild("ActiveProfileStatus", ImVec2(0, 120), true)) {
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 8.0f));
		ImGui::Text(langString(LanguageCategory::Profile_Option_UI, LanguageKey::Active_Profile_Title));
		ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
		ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), "%s", currentProfileName.c_str());
		ImGui::PopFont();
		
		if (currentCharacterName != "") {
			if (doesCharacterMappingExist) {
				ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.4f, 1.0f), langString(LanguageCategory::Profile_Option_UI, LanguageKey::Profile_Source_Character_Override), currentCharacterName.c_str());
			} else {
				ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), langString(LanguageCategory::Profile_Option_UI, LanguageKey::Profile_Source_Default_Profile), currentCharacterName.c_str());
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
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();
	ImGui::Spacing();
	

	ImGui::Text(langString(LanguageCategory::Profile_Option_UI, LanguageKey::Profile_Management_Title));
	ImGui::Spacing();

	if (ImGui::BeginCombo("Default Profile:", options.globalProfile.c_str())) {
		for (auto& nameAndProfile : options.profiles) {
			if (ImGui::Selectable((nameAndProfile.first + "##profile-selectable").c_str())) {
				if (options.globalProfile != nameAndProfile.first) {
					options.globalProfile = nameAndProfile.first;
					if (!doesCharacterMappingExist) {
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
	
	ImGui::Spacing();
	ImGui::Spacing();
	
	if (currentCharacterName != "") {
		if (ImGui::Checkbox("Character-Specific Override", &doesCharacterMappingExist)) {
			if (doesCharacterMappingExist) {
				options.characterProfileMap[currentCharacterName] = currentProfileName;
			}
			else {
				currentProfileName = options.globalProfile;
				requestSwitch(options.profiles[currentProfileName]);
				options.characterProfileMap.erase(currentCharacterName);
			}
			Options::requestSave();
		}
		
		if (!doesCharacterMappingExist) {
			ImGui::BeginDisabled();
		}
		
		ImGui::Indent();
		if (ImGui::BeginCombo("Override Profile:", currentProfileName.c_str())) {
			for (auto& nameAndProfile : options.profiles) {
				if (ImGui::Selectable((nameAndProfile.first + "##character-profile-selectable").c_str())) {
					currentProfileName = nameAndProfile.first;
					requestSwitch(nameAndProfile.second);
					options.characterProfileMap[currentCharacterName] = currentProfileName;
					Options::requestSave();
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
		
		if (!doesCharacterMappingExist) {
			ImGui::EndDisabled();
		}
		
		ImGui::Spacing();
	}
	
	ImGui::Separator();
	ImGui::Spacing();
	ImGui::Spacing();
	
	ImGui::Text("Profile Actions:");
	ImGui::Spacing();
	
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
		auto& options = Options::getOptionsStruct();
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
		if (options.globalProfile == currentProfileName) {
			options.globalProfile = copyName;
		}
		for (auto& characterProfileMapping : options.characterProfileMap) {
			if (characterProfileMapping.second == currentProfileName) {
				characterProfileMapping.second = copyName;
			}
		}
		currentProfileName = copyName;
		requestSwitch(options.profiles[currentProfileName]);
		Options::requestSave();
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
		initWithDefaults(newProfile);

		options.profiles[newProfileName] = newProfile;

		if (doesCharacterMappingExist) {
			options.characterProfileMap[currentCharacterName] = newProfileName;
		}
		else {
			options.globalProfile = newProfileName;
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