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
	skillFilterTypeSelectionString = std::string(langStringG(LanguageKey::Skill_Filter_Type_ID)) + '\0' + std::string(langStringG(LanguageKey::Skill_Filter_Type_Name)) + '\0';
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
		ImGui::Text("Filter Sets");
		ImGui::Separator();

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

		ImGui::Separator();

		static char newFilterSetName[256] = "";
		ImGui::InputText("##new_filter_set_name", newFilterSetName, sizeof(newFilterSetName));
		ImGui::SameLine();
		if (ImGui::Button("Add") && strlen(newFilterSetName) > 0) {
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
						nameExists = (currentProfile->filterManager.getFilterSet(renameBuffer) != nullptr);
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
					if (ImGui::Button("No", ImVec2(120, 0))) {
						ImGui::CloseCurrentPopup();
					}
					ImGui::EndPopup();
				}

				if (ImGui::InputText("Description", &filterSet->description)) {
					Options::requestSave();
				}

				ImGui::Separator();

				ImGui::Text("Default Action:");
				ImGui::SameLine();
				int defaultAction = static_cast<int>(filterSet->filterSet.defaultAction);
				if (ImGui::RadioButton("Allow", &defaultAction, 0)) {
					filterSet->filterSet.defaultAction = FilterAction::ALLOW;
					Options::requestSave();
				}
				ImGui::SameLine();
				if (ImGui::RadioButton("Block", &defaultAction, 1)) {
					filterSet->filterSet.defaultAction = FilterAction::BLOCK;
					Options::requestSave();
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
						Options::requestSave();
					}

					ImGui::SameLine();

					int type = static_cast<int>(it->type);
					ImGui::SetNextItemWidth(120);
					if (ImGui::Combo("##type", &type, "Skill ID\0Skill Name\0ID Range\0")) {
						it->type = static_cast<FilterType>(type);
						Options::requestSave();
					}

					ImGui::SameLine();

					switch (it->type) {
					case FilterType::SKILL_ID: {
						ImGui::SetNextItemWidth(200);
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
						ImGui::SetNextItemWidth(90);
						if (ImGui::InputScalar("##start", ImGuiDataType_U32, &it->idRange.start)) {
							Options::requestSave();
						}
						ImGui::SameLine();
						ImGui::Text("-");
						ImGui::SameLine();
						ImGui::SetNextItemWidth(90);
						if (ImGui::InputScalar("##end", ImGuiDataType_U32, &it->idRange.end)) {
							Options::requestSave();
						}
						break;
					}
					}

					ImGui::EndGroup();

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

				if (ImGui::Button("+ Add Filter")) {
					SkillFilter newFilter;
					newFilter.type = FilterType::SKILL_ID;
					newFilter.action = (filterSet->filterSet.defaultAction == FilterAction::ALLOW) 
						? FilterAction::BLOCK 
						: FilterAction::ALLOW;
					newFilter.skillId = 0;
					filterSet->filterSet.filters.push_back(newFilter);
					Options::requestSave();
				}

				ImGui::Separator();

				ImGui::Text("Used by:");
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