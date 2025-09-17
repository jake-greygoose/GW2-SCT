#include "SkillFilterUI.h"
#include <cstring>
#include <algorithm>
#include "imgui.h"
#include "imgui_stdlib.h"
#include "imgui_sct_widgets.h"
#include "Language.h"
#include "SkillFilterStructures.h"
#include "Profiles.h"
#include "Options.h"

std::string GW2_SCT::SkillFilterUI::skillFilterTypeSelectionString = "";

void GW2_SCT::SkillFilterUI::initialize() {
	skillFilterTypeSelectionString =
		std::string(langStringG(LanguageKey::Skill_Filter_Type_ID)) + '\0' +
		std::string(langStringG(LanguageKey::Skill_Filter_Type_Name)) + '\0' +
		std::string(langStringG(LanguageKey::Skill_Filter_Type_ID_Range)) + '\0';
}

void GW2_SCT::SkillFilterUI::paintUI() {
	auto currentProfile = Profiles::get();
	if (!currentProfile) return;
	
	const float square_size = ImGui::GetFontSize();
	ImGuiStyle style = ImGui::GetStyle();

	static std::string selectedFilterSet = "";

	// Left pane - Filter Set List
	{
		ImGui::BeginChild("filter_sets_list", ImVec2(ImGui::GetWindowWidth() * 0.3f, 0), true);
		ImGui::Text(langString(LanguageCategory::Skill_Filter_Option_UI, LanguageKey::Filter_Sets_List_Title));
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		for (auto& [name, filterSet] : currentProfile->filterManager.getAllFilterSets()) {
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

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		static char newFilterSetName[256] = "";
		ImGui::SetNextItemWidth(-80);
		ImGui::InputText("##new_filter_set_name", newFilterSetName, sizeof(newFilterSetName));
		ImGui::SameLine();
		if (ImGui::Button(ImGui::BuildVisibleLabel(langString(LanguageCategory::Skill_Filter_Option_UI, LanguageKey::New_Filter_Set_Add), "add-filter-set").c_str(), ImVec2(70, 0)) && strlen(newFilterSetName) > 0) {
			if (currentProfile->filterManager.getFilterSet(newFilterSetName) == nullptr) {
				currentProfile->filterManager.createFilterSet(newFilterSetName);
				selectedFilterSet = newFilterSetName;
				memset(newFilterSetName, 0, sizeof(newFilterSetName));
				Options::requestSave();
			}
		}

		ImGui::EndChild();
	}

	ImGui::SameLine();

	// Right pane - Filter Set Details
	{
		ImGui::BeginChild("filter_set_details", ImVec2(0, 0), true);

		if (!selectedFilterSet.empty()) {
			auto filterSet = currentProfile->filterManager.getFilterSet(selectedFilterSet);
			if (filterSet) {
				ImGui::Text("%s: %s", langString(LanguageCategory::Skill_Filter_Option_UI, LanguageKey::Filter_Set_Label), filterSet->name.c_str());
				ImGui::Spacing();

				static char renameBuffer[256] = "";
				if (ImGui::BeginPopupModal(langString(LanguageCategory::Skill_Filter_Option_UI, LanguageKey::Rename_Filter_Set_Title))) {
					if (ImGui::IsWindowAppearing()) {
						strncpy(renameBuffer, filterSet->name.c_str(), sizeof(renameBuffer) - 1);
						renameBuffer[sizeof(renameBuffer) - 1] = '\0';
					}

					ImGui::Text(langString(LanguageCategory::Skill_Filter_Option_UI, LanguageKey::Rename_Filter_Set_Prompt));
					ImGui::InputText("##rename_filter_set", renameBuffer, sizeof(renameBuffer));

					bool nameExists = false;
					if (strlen(renameBuffer) > 0 && strcmp(renameBuffer, filterSet->name.c_str()) != 0) {
						nameExists = (currentProfile->filterManager.getFilterSet(renameBuffer) != nullptr);
						if (nameExists) {
							ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), langString(LanguageCategory::Skill_Filter_Option_UI, LanguageKey::Rename_Filter_Set_Name_Exists));
						}
					}

					ImGui::Separator();

					if (strlen(renameBuffer) == 0 || nameExists) {
						ImGui::BeginDisabled();
					}
					if (ImGui::Button(ImGui::BuildVisibleLabel(langString(LanguageCategory::Skill_Filter_Option_UI, LanguageKey::Delete_Confirmation_Confirmation), "rename-filter-set-ok").c_str(), ImVec2(120, 0))) {
						if (strlen(renameBuffer) > 0 && strcmp(renameBuffer, filterSet->name.c_str()) != 0) {
							std::string oldName = filterSet->name;
							std::string newName = renameBuffer;

							filterSet->name = newName;

							currentProfile->filterManager.removeFilterSet(oldName);
							currentProfile->filterManager.addFilterSet(filterSet);

							for (auto& scrollArea : currentProfile->scrollAreaOptions) {
								for (auto& receiver : scrollArea->receivers) {
									for (auto& assignedName : receiver->assignedFilterSets) {
										if (assignedName == oldName) {
											assignedName = newName;
										}
									}
								}
							}

							selectedFilterSet = newName;

							Options::requestSave();
							ImGui::CloseCurrentPopup();
						}
					}
					if (strlen(renameBuffer) == 0 || nameExists) {
						ImGui::EndDisabled();
					}

					ImGui::SameLine();
					if (ImGui::Button(ImGui::BuildVisibleLabel(langString(LanguageCategory::Skill_Filter_Option_UI, LanguageKey::Delete_Confirmation_Cancel), "rename-filter-set-cancel").c_str(), ImVec2(120, 0))) {
						ImGui::CloseCurrentPopup();
					}
					ImGui::EndPopup();
				}

				ImGui::Separator();
				ImGui::Spacing();

				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.67f, 0.40f, 0.40f, 0.60f));
				if (ImGui::Button(ImGui::BuildVisibleLabel(langString(LanguageCategory::Skill_Filter_Option_UI, LanguageKey::Filter_Set_Delete_Button_Text), "delete-filter-set-button").c_str(), ImVec2(120, 0))) {
					ImGui::OpenPopup(langString(LanguageCategory::Skill_Filter_Option_UI, LanguageKey::Delete_Confirmation_Title));
				}
				ImGui::PopStyleColor();

				ImGui::SameLine();
				if (ImGui::Button(ImGui::BuildVisibleLabel(langString(LanguageCategory::Skill_Filter_Option_UI, LanguageKey::Filter_Set_Rename_Button_Text), "rename-filter-set-button").c_str(), ImVec2(120, 0))) {
					ImGui::OpenPopup(langString(LanguageCategory::Skill_Filter_Option_UI, LanguageKey::Rename_Filter_Set_Title));
				}

				if (ImGui::BeginPopupModal(langString(LanguageCategory::Skill_Filter_Option_UI, LanguageKey::Delete_Confirmation_Title))) {
					ImGui::Text(langString(LanguageCategory::Skill_Filter_Option_UI, LanguageKey::Delete_Filter_Set_Content), filterSet->name.c_str());
					if (ImGui::Button(ImGui::BuildVisibleLabel(langString(LanguageCategory::Skill_Filter_Option_UI, LanguageKey::Delete_Confirmation_Confirmation), "delete-filter-set-confirm").c_str(), ImVec2(120, 0))) {
						for (auto& scrollArea : currentProfile->scrollAreaOptions) {
							for (auto& receiver : scrollArea->receivers) {
								auto it = std::find(receiver->assignedFilterSets.begin(),
									receiver->assignedFilterSets.end(),
									filterSet->name);
								if (it != receiver->assignedFilterSets.end()) {
									receiver->assignedFilterSets.erase(it);
								}
							}
						}
						currentProfile->filterManager.removeFilterSet(filterSet->name);
						selectedFilterSet = "";
						Options::requestSave();
						ImGui::CloseCurrentPopup();
					}
					ImGui::SameLine();
					if (ImGui::Button(ImGui::BuildVisibleLabel(langString(LanguageCategory::Skill_Filter_Option_UI, LanguageKey::Delete_Confirmation_Cancel), "delete-filter-set-cancel").c_str(), ImVec2(120, 0))) {
						ImGui::CloseCurrentPopup();
					}
					ImGui::EndPopup();
				}

				ImGui::Spacing();
				ImGui::Text(langString(LanguageCategory::Skill_Filter_Option_UI, LanguageKey::Description_Label));
				ImGui::SetNextItemWidth(-1);
				if (ImGui::InputText("##description", &filterSet->description)) {
					Options::requestSave();
				}

				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();

				ImGui::Text(langString(LanguageCategory::Skill_Filter_Option_UI, LanguageKey::Default_Action_Label));
				ImGui::SameLine();
				int defaultAction = static_cast<int>(filterSet->filterSet.defaultAction);
				if (ImGui::RadioButton(langString(LanguageCategory::Skill_Filter_Option_UI, LanguageKey::Default_Action_Allow), &defaultAction, 0)) {
					filterSet->filterSet.defaultAction = FilterAction::ALLOW;
					Options::requestSave();
				}
				ImGui::SameLine();
				if (ImGui::RadioButton(langString(LanguageCategory::Skill_Filter_Option_UI, LanguageKey::Default_Action_Block), &defaultAction, 1)) {
					filterSet->filterSet.defaultAction = FilterAction::BLOCK;
					Options::requestSave();
				}

				ImGui::Separator();
				ImGui::Spacing();
				ImGui::Text(langString(LanguageCategory::Skill_Filter_Option_UI, LanguageKey::Filters_Title));
				ImGui::Spacing();

				int filterIndex = 0;
				auto it = filterSet->filterSet.filters.begin();
				while (it != filterSet->filterSet.filters.end()) {
					ImGui::PushID(filterIndex);

					bool shouldDelete = false;

					ImGui::BeginGroup();

					// Delete button with confirmation
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.67f, 0.40f, 0.40f, 0.60f));
					if (ImGui::Button("×", ImVec2(25, 0))) {
						shouldDelete = true;
					}
					ImGui::PopStyleColor();
					if (ImGui::IsItemHovered()) {
						ImGui::SetTooltip("%s", langString(LanguageCategory::Skill_Filter_Option_UI, LanguageKey::Delete_Filter_Tooltip));
					}

					ImGui::SameLine();

					// Action dropdown
					int action = static_cast<int>(it->action);
					ImGui::SetNextItemWidth(90);
					{
						std::string actionCombo = std::string(langString(LanguageCategory::Skill_Filter_Option_UI, LanguageKey::Default_Action_Allow)) + '\0' + std::string(langString(LanguageCategory::Skill_Filter_Option_UI, LanguageKey::Default_Action_Block)) + '\0';
						if (ImGui::Combo("##action", &action, actionCombo.c_str())) {
						it->action = static_cast<FilterAction>(action);
						Options::requestSave();
						}
					}

					ImGui::SameLine();

					// Filter type dropdown
					int type = static_cast<int>(it->type);
					ImGui::SetNextItemWidth(130);
					{
						std::string typeCombo = GW2_SCT::SkillFilterUI::getFilterTypeSelectionString();
						if (ImGui::Combo("##type", &type, typeCombo.c_str())) {
						it->type = static_cast<FilterType>(type);
						Options::requestSave();
						}
					}

					ImGui::SameLine();

					// Value input based on type
					switch (it->type) {
					case FilterType::SKILL_ID: {
						ImGui::SetNextItemWidth(150);
						if (ImGui::InputScalar("##value", ImGuiDataType_U32, &it->skillId)) {
							Options::requestSave();
						}
						break;
					}
					case FilterType::SKILL_NAME: {
						ImGui::SetNextItemWidth(200);
						if (ImGui::InputText("##value", &it->skillName)) {
							Options::requestSave();
						}
						break;
					}
					case FilterType::SKILL_ID_RANGE: {
						ImGui::SetNextItemWidth(100);
						if (ImGui::InputScalar("##start", ImGuiDataType_U32, &it->idRange.start)) {
							Options::requestSave();
						}
						ImGui::SameLine();
						ImGui::Text("%s", langString(LanguageCategory::Skill_Filter_Option_UI, LanguageKey::Range_To_Word));
						ImGui::SameLine();
						ImGui::SetNextItemWidth(100);
						if (ImGui::InputScalar("##end", ImGuiDataType_U32, &it->idRange.end)) {
							Options::requestSave();
						}
						break;
					}
					}

					ImGui::EndGroup();
					ImGui::Spacing();

					ImGui::PopID();

					if (shouldDelete) {
						it = filterSet->filterSet.filters.erase(it);
						Options::requestSave();
					}
					else {
						++it;
						++filterIndex;
					}
				}

				if (filterSet->filterSet.filters.empty()) {
					ImGui::Spacing();
					ImGui::TextDisabled(langString(LanguageCategory::Skill_Filter_Option_UI, LanguageKey::No_Filters_Defined_Info));
					ImGui::Spacing();
				}

				{
					std::string addFilterLabel = std::string("+ ") + std::string(langString(LanguageCategory::Skill_Filter_Option_UI, LanguageKey::Add_Filter_Button_Text));
					if (ImGui::Button(ImGui::BuildVisibleLabel(addFilterLabel.c_str(), "add-filter-button").c_str(), ImVec2(120, 0))) {
					SkillFilter newFilter;
					newFilter.type = FilterType::SKILL_ID;
					newFilter.action = (filterSet->filterSet.defaultAction == FilterAction::ALLOW)
						? FilterAction::BLOCK
						: FilterAction::ALLOW;
					newFilter.skillId = 0;
					filterSet->filterSet.filters.push_back(newFilter);
					Options::requestSave();
					}
				}

				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();

				ImGui::Text(langString(LanguageCategory::Skill_Filter_Option_UI, LanguageKey::Used_By_Label));
				int usageCount = 0;
				for (auto& scrollArea : currentProfile->scrollAreaOptions) {
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
					ImGui::TextDisabled(langString(LanguageCategory::Skill_Filter_Option_UI, LanguageKey::Not_Assigned_Info));
				}
			}
		}
		else {
			ImGui::Spacing();
			ImGui::TextDisabled(langString(LanguageCategory::Skill_Filter_Option_UI, LanguageKey::Select_Filter_Set_Hint_Line1));
			ImGui::Spacing();
			ImGui::TextDisabled(langString(LanguageCategory::Skill_Filter_Option_UI, LanguageKey::Select_Filter_Set_Hint_Line2));
		}

		ImGui::EndChild();
	}
}
