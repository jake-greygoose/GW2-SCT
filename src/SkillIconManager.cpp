#include "httpclient.h"
#include "SkillIconManager.h"
#include <string>
#include <stdexcept>
#include <thread>
#include <functional>
#include <unordered_set>
#include <regex>
#include <algorithm>
#include <cctype>
#include "Common.h"
#include "Options.h"
#include "Profiles.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <windows.h>
#include "Texture.h"



nlohmann::json getJSON(std::string url, std::function<void(std::map<std::string, std::string>)> callback = nullptr) {
	std::string fullUrl = "https://api.guildwars2.com" + url;
	std::string response = HTTPClient::GetRequest(HTTPClient::StringToWString(fullUrl));

	if (response.empty()) {
		throw std::runtime_error("Empty response from " + fullUrl);
	}

	if (callback) {
		std::map<std::string, std::string> emptyHeaders;
		callback(emptyHeaders);
	}

	auto it = response.begin();
	while (it != response.end() && std::isspace(static_cast<unsigned char>(*it))) { ++it; }
	if (it == response.end()) {
		throw std::runtime_error("Whitespace-only response from " + fullUrl);
	}

	if (*it == '<') {
		std::string snippet(response.begin(), response.begin() + std::min<size_t>(response.size(), 256));
		std::string lower = snippet;
		for (char& ch : lower) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
		if (lower.find("service unavailable") != std::string::npos || lower.find("503") != std::string::npos) {
			throw std::runtime_error("Service unavailable response from " + fullUrl);
		}
		throw std::runtime_error("Non-JSON response from " + fullUrl);
	}

	try {
		return nlohmann::json::parse(response);
	} catch (const nlohmann::json::parse_error& e) {
		std::string errorMsg = "Invalid JSON response from " + fullUrl + ": " + std::string(e.what());
		LOG("JSON parse error: ", errorMsg);
		throw std::runtime_error(errorMsg);
	} catch (const nlohmann::json::exception& e) {
		std::string errorMsg = "JSON error from " + fullUrl + ": " + std::string(e.what());
		LOG("JSON error: ", errorMsg);
		throw std::runtime_error(errorMsg);
	}
}

bool downloadBinaryFile(const std::string& url, const std::string& outputPath) {
	try {
		std::string response = HTTPClient::GetRequest(HTTPClient::StringToWString(url));
		if (response.empty()) {
			return false;
		}
		
		std::ofstream fileStream(outputPath, std::ofstream::binary);
		if (!fileStream.is_open()) {
			return false;
		}
		
		fileStream.write(response.data(), response.size());
		fileStream.close();
		return true;
	} catch (...) {
		return false;
	}
}

bool RemoveZoneIdentifier(const std::string& filepath) {
	std::string zoneStream = filepath + ":Zone.Identifier";
	return DeleteFileA(zoneStream.c_str()) != 0 || GetLastError() == ERROR_FILE_NOT_FOUND;
}

std::shared_ptr<std::vector<BYTE>> loadBinaryFileData(std::string filename) {
	RemoveZoneIdentifier(filename);
	
	std::ifstream in(filename, std::ios::binary);
	in >> std::noskipws;     
	
	std::shared_ptr<std::vector<BYTE>> ret = std::make_shared<std::vector<BYTE>>(std::istream_iterator<BYTE>(in), std::istream_iterator<BYTE>());
	return ret;
}

extern HMODULE g_hModule;

static std::shared_ptr<std::vector<BYTE>> LoadEmbeddedSkillIcon(uint32_t skillId) {
    if (!g_hModule) return {};
    HRSRC res = FindResourceW(g_hModule, MAKEINTRESOURCEW(skillId), MAKEINTRESOURCEW(10));
    if (!res) return {};
    DWORD size = SizeofResource(g_hModule, res);
    if (!size) return {};
    HGLOBAL hRes = LoadResource(g_hModule, res);
    if (!hRes) return {};
    void* data = LockResource(hRes);
    if (!data) return {};
    auto bytes = std::make_shared<std::vector<BYTE>>();
    bytes->assign(static_cast<const BYTE*>(data), static_cast<const BYTE*>(data) + size);
    return bytes;
}

sf::contfree_safe_ptr<std::unordered_map<uint32_t, bool>> GW2_SCT::SkillIconManager::checkedIDs;
sf::contfree_safe_ptr<std::unordered_map<uint32_t, GW2_SCT::SkillIcon>> GW2_SCT::SkillIconManager::loadedIcons;
sf::contfree_safe_ptr<std::unordered_set<uint32_t>> GW2_SCT::SkillIconManager::embeddedIconIds;
sf::contfree_safe_ptr<std::list<uint32_t>> GW2_SCT::SkillIconManager::requestedIDs;
std::thread GW2_SCT::SkillIconManager::loadThread;
int GW2_SCT::SkillIconManager::requestsPerMinute = 200;
std::atomic<bool> GW2_SCT::SkillIconManager::keepLoadThreadRunning;

long GW2_SCT::SkillIconManager::skillIconsEnabledCallbackId = -1;
long GW2_SCT::SkillIconManager::preloadAllSkillIconsId = -1;

std::unordered_map<uint32_t, std::pair<std::string, std::string>> GW2_SCT::SkillIconManager::staticFiles = {
	{ 736, { "79FF0046A5F9ADA3B4C4EC19ADB4CB124D5F0021", "102848" } }, //Bleeding
	{ 737, { "B47BF5803FED2718D7474EAF9617629AD068EE10", "102849" } }, //Burning
	{ 723, { "559B0AF9FB5E1243D2649FAAE660CCB338AACC19", "102840" } }, //Poison
	{ 861, { "289AA0A4644F0E044DED3D3F39CED958E1DDFF53", "102880" } }, //Confusion
	{ 19426, { "10BABF2708CA3575730AC662A2E72EC292565B08", "598887" } }, //Torment
	{ 718, { "F69996772B9E18FD18AD0AABAB25D7E3FC42F261", "102835" } }, //Regeneration
	{ 17495, { "F69996772B9E18FD18AD0AABAB25D7E3FC42F261", "102835" } }, //Regeneration
	{ 17674, { "F69996772B9E18FD18AD0AABAB25D7E3FC42F261", "102835" } }, //Regeneration
    { 13814, { "0DF523B154B8FD9F2E2B95F72A534E7C80352D2A", "1012544" } }, //Vampiric Strikes
	{ 30285, { "22B370D754AC1D69A8FE66DCCB36BE940455E5EA", "1012539" } }, //Vampiric Aura
    { 43260, { "2459E053ADCC6A6064DD505CC03FEA767D12DBB6", "1769970" } }, //Desert Empowerment
	{ 43759, { "076FD4FE99097163B3A861CDDEEE15C460E80FA8", "1770537" } }, //Nefarious Favor
	{ 54958, { "A4779F3D1924A75A58C0C100E82A0923A46C5CBD", "1769968" } }, //Herald of Sorrow Explosion
	{ 13017, { "630D6100268010ED04B2ABE529BD4AE9110BF65F", "2503655" } }, //Panaku's Ambition"
	{ 68070, { "3B7C573FBF124EC43F4102D530DE5E333F31014F", "1012750" } }, //Shielding Restoration"
	{ 99965, { "2F7AE267BA29B35DEC7F2C0FCE5C30D806E31E0D", "3122350" } }, //Flock Relic
    { 100633, { "2F7AE267BA29B35DEC7F2C0FCE5C30D806E31E0D", "3122350" } }, //Flock Relic
	{ 71356, { "DD034A0B53355503350F07CCFFE5CC06A90F41D9", "3187629" } }, //Karakosa Relic
	{ 551, { "32BAB20860259FF3E8214E784E6BE7521213089C", "1012412" } }, //Selfless Daring
	{ 549, { "B8FD6EB1B2C6CF7CEFE4715994B6FBCC201FF24D", "1012406" } } //Pure of Heart
	
};

void GW2_SCT::SkillIconManager::init() {
	Profiles::profile.onAssign([=](std::shared_ptr<profile_options_struct> oldProfile, std::shared_ptr<profile_options_struct> newProfile) {
		if (skillIconsEnabledCallbackId >= 0) {
			oldProfile->skillIconsEnabled.removeOnAssign(skillIconsEnabledCallbackId);
		}
		if (preloadAllSkillIconsId >= 0) {
			oldProfile->preloadAllSkillIcons.removeOnAssign(preloadAllSkillIconsId);
		}
		if (newProfile->skillIconsEnabled) {
			std::thread t(SkillIconManager::internalInit);
			t.detach();
		}
		skillIconsEnabledCallbackId = newProfile->skillIconsEnabled.onAssign([=](const bool& wasEnabled, const bool& isNowEnabled) {
			if (wasEnabled == isNowEnabled) return;
			if (isNowEnabled) {
				std::thread t(SkillIconManager::internalInit);
				t.detach();
			}
			else {
				if (loadThread.joinable()) {
					keepLoadThreadRunning = false;
					loadThread.detach();
				}
			}
		});
		preloadAllSkillIconsId = newProfile->preloadAllSkillIcons.onAssign([=](const bool& wasEnabled, const bool& isNowEnabled) {
			if (wasEnabled == isNowEnabled) return;
			if (isNowEnabled) {
				std::thread t([]() {
					requestedIDs->clear();
					if (Options::get()->preloadAllSkillIcons) {
						auto s_checkedIDs = sf::slock_safe_ptr(checkedIDs);
						for (
							auto checkableIDIterator = s_checkedIDs->begin();
							checkableIDIterator != s_checkedIDs->end();
							checkableIDIterator++
						) {
							if (!checkableIDIterator->second) {
								requestedIDs->push_back(checkableIDIterator->first);
							}
						}
					}
				});
				t.detach();
			}
		});
	});
}

void GW2_SCT::SkillIconManager::cleanup() {
	if (loadThread.joinable()) {
		keepLoadThreadRunning = false;
		loadThread.join();
	}
}

void GW2_SCT::SkillIconManager::internalInit() {
    try {
        if (Options::get()->skillIconsEnabled) {
			std::regex matcher("X-Rate-Limit-Limit: ([0-9]+)");
			std::vector<int> skillIdList;
		try {
			nlohmann::json skillListJson = getJSON("/v2/skills", [](std::map<std::string, std::string> headers) {
				auto foundHeader = headers.find("X-Rate-Limit-Limit");
				if (foundHeader != headers.end()) {
					requestsPerMinute = std::min(std::stoi(foundHeader->second), requestsPerMinute);
				}
			});
			if (skillListJson.is_array()) {
				skillIdList.reserve(skillListJson.size());
				for (const auto& value : skillListJson) {
					if (value.is_number_integer()) {
						skillIdList.push_back(value.get<int>());
					}
				}
			}
		}
		catch (const std::exception& e) {
			static bool s_couldNotFetchSkillList = false;
			if (!s_couldNotFetchSkillList) {
				LOG("Skill icon API unavailable, continuing with cached data: ", e.what());
				s_couldNotFetchSkillList = true;
			}
		}
		std::string skillJsonFilename = getSCTPath() + "skill.json";
		std::map<uint32_t, std::string> skillJsonValues;
		if (file_exist(skillJsonFilename)) {
			LOG("Loading skill.json");
			std::string line, textFile;
			std::ifstream in(skillJsonFilename);
			while (std::getline(in, line)) {
				textFile += line + "\n";
			}
			in.close();
			try {
				skillJsonValues = nlohmann::json::parse(textFile);
			} catch (std::exception&) {
				LOG("Error parsing skill.json");
			}
		} else {
			LOG("Warning: could not find a skill.json");
		}
		if (skillIdList.empty() && !skillJsonValues.empty()) {
			for (const auto& entry : skillJsonValues) {
				skillIdList.push_back(static_cast<int>(entry.first));
			}
		}
		bool preload = Options::get()->preloadAllSkillIcons;
		checkedIDs->clear();
		embeddedIconIds->clear();
		for (const auto& skillId : skillIdList) {
			checkedIDs->insert({ skillId, false });
			if (preload) {
				requestedIDs->push_back(skillId);
			}
		}
		for (auto it : staticFiles) {
			checkedIDs->insert({ it.first, false });
			auto embedded = LoadEmbeddedSkillIcon((uint32_t)it.first);
			if (embedded && !embedded->empty()) {
				(*checkedIDs)[it.first] = true;
				embeddedIconIds->insert(it.first);
				// Replace any existing icon
				auto existing = loadedIcons->find(it.first);
				if (existing != loadedIcons->end()) {
					existing->second = SkillIcon(embedded, (uint32_t)it.first);
				} else {
					loadedIcons->insert({ it.first, SkillIcon(embedded, (uint32_t)it.first) });
				}
			}
            }

            spawnLoadThread();
        }
	} catch (std::exception& e) {
		LOG("Skill icon thread error: ", e.what());
	}
}

void GW2_SCT::SkillIconManager::spawnLoadThread() {
	if (loadThread.joinable()) {
		keepLoadThreadRunning = false;
		loadThread.join();
	}
	keepLoadThreadRunning = true;
	loadThread = std::thread(GW2_SCT::SkillIconManager::loadThreadCycle);
}


template <typename Range, typename Value = typename Range::value_type>
std::string join(Range const& elements, const char* const delimiter) {
	std::ostringstream os;
	auto b = begin(elements), e = end(elements);

	if (b != e) {
		std::copy(b, prev(e), std::ostream_iterator<Value>(os, delimiter));
		b = prev(e);
	}
	if (b != e) {
		os << *b;
	}

	return os.str();
}

void GW2_SCT::SkillIconManager::loadThreadCycle() {
#if _DEBUG
	LOG("Skillicon load thread started");
#endif
	try {
		std::vector<std::string> files;
		std::string iconPath = getSCTPath() + "icons\\";
		CreateDirectory(iconPath.c_str(), NULL);
		if (getFilesInDirectory(iconPath, files)) {
			for (std::string iconFile : files) {
				size_t itDot = iconFile.find_last_of(".");
				std::string extension = iconFile.substr(itDot + 1);
				if (extension == "jpg" || extension == "png") {
					std::string fileName = iconFile.substr(0, itDot);
					uint32_t skillID = std::strtoul(fileName.c_str(), NULL, 10);
					if (skillID != 0) {
						loadedIcons->insert({ skillID, SkillIcon(loadBinaryFileData(iconPath + iconFile), skillID) });
						(*checkedIDs)[skillID] = true;
						continue;
					}
				}
			}
		}

	std::string skillJsonFilename = getSCTPath() + "skill.json";
	std::map<uint32_t, std::string> skillJsonValues;
	if (file_exist(skillJsonFilename)) {
		LOG("Loading skill.json");
		std::string line, text;
		std::ifstream in(skillJsonFilename);
		while (std::getline(in, line)) {
			text += line + "\n";
		}
		in.close();
		try {
			skillJsonValues = nlohmann::json::parse(text);
		} catch (std::exception&) {
			LOG("Error parsing skill.json");
		}
	} else {
		LOG("Warning: could not find a skill.json");
	}
		std::regex renderAPIURLMatcher("/file/([A-Z0-9]+)/([0-9]+)\\.(png|jpg)");
		while (keepLoadThreadRunning) {
		if (requestedIDs->size() > 0) {
			std::list<std::tuple<uint32_t, std::string, std::string>> loadableIconURLs;
			std::vector<uint32_t> idListToRequestFromApi = {};
			std::list<std::pair<uint32_t, std::shared_ptr<std::vector<BYTE>>>> embeddedLoadedIcons;
			while (requestedIDs->size() > 0 && idListToRequestFromApi.size() <= 10) {
				int frontRequestedSkillId = requestedIDs->front();
				requestedIDs->pop_front();

                auto embedded = LoadEmbeddedSkillIcon((uint32_t)frontRequestedSkillId);
                if (embedded && !embedded->empty()) {
                    embeddedLoadedIcons.push_back({ (uint32_t)frontRequestedSkillId, embedded });
                    embeddedIconIds->insert((uint32_t)frontRequestedSkillId);
                    continue;
                }

				auto iteratorToFoundStaticFileInformation = staticFiles.find(frontRequestedSkillId);
				if (iteratorToFoundStaticFileInformation != staticFiles.end()) {
					loadableIconURLs.push_back({ iteratorToFoundStaticFileInformation->first, iteratorToFoundStaticFileInformation->second.first, iteratorToFoundStaticFileInformation->second.second });
				} else {
					idListToRequestFromApi.push_back(frontRequestedSkillId);
				}
			}
			if (idListToRequestFromApi.size() > 0) {
				std::string idStringToRequestFromApi = join(idListToRequestFromApi, ",");
				try {
					std::this_thread::sleep_for(std::chrono::milliseconds(60000 / requestsPerMinute));
					nlohmann::json possibleSkillInformationList;
					try {
						possibleSkillInformationList = getJSON("/v2/skills?ids=" + idStringToRequestFromApi);
					} catch (const std::exception& e) {
						static bool s_detailFetchWarned = false;
						if (!s_detailFetchWarned) {
							LOG("Skill icon detail fetch failed: ", e.what());
							s_detailFetchWarned = true;
						}
						for (auto& unresolvedSkillId : idListToRequestFromApi) {
							requestedIDs->push_back(unresolvedSkillId);
						}
						std::this_thread::sleep_for(std::chrono::seconds(5));
						continue;
					}
					if (possibleSkillInformationList.is_array()) {
						std::vector<nlohmann::json> skillInformationList = possibleSkillInformationList;
						for (auto& skillInformation : skillInformationList) {
							if (!skillInformation["icon"].is_null()) {
								std::string renderingApiURL = skillInformation["icon"];
								std::string signature = std::regex_replace(renderingApiURL, renderAPIURLMatcher, "$1", std::regex_constants::format_no_copy);
								std::string fileId = std::regex_replace(renderingApiURL, renderAPIURLMatcher, "$2", std::regex_constants::format_no_copy);
								loadableIconURLs.push_back(std::make_tuple(skillInformation["id"], signature, fileId));
							}
						}
					}
				}
				catch (std::exception& e) {
					LOG("Could not receive skill information for IDs: ", idStringToRequestFromApi, "(", e.what(), ")");
					for (auto& unresolvedSkillId : idListToRequestFromApi) {
						requestedIDs->push_back(unresolvedSkillId);
					}
				}
			}

			std::list<std::pair<uint32_t, std::shared_ptr<std::vector<BYTE>>>> binaryLoadedIcons;
			for (auto& p : embeddedLoadedIcons) binaryLoadedIcons.push_back(p);
			for (auto it = loadableIconURLs.begin(); it != loadableIconURLs.end() && keepLoadThreadRunning; ++it) {
				uint32_t curSkillId = std::get<0>(*it);
				try {
					std::string desc = std::get<1>(*it) + "/" + std::get<2>(*it);
					auto it = skillJsonValues.find(curSkillId);
					std::string iniVal = it == skillJsonValues.end() ? "" : it->second;
					std::string curImagePath = iconPath + std::to_string(curSkillId) + ".jpg";
					std::string curImagePathPng = iconPath + std::to_string(curSkillId) + ".png";
					if (iniVal.compare(desc) != 0 || (!std::filesystem::exists(curImagePath.c_str()) && !std::filesystem::exists(curImagePathPng.c_str()))) {
						std::this_thread::sleep_for(std::chrono::milliseconds(60000 / requestsPerMinute));
#if _DEBUG
						LOG("Downloading skill icon: ", curSkillId);
#endif
						
						std::string downloadUrl = "https://render.guildwars2.com/file/" + desc;
						if (downloadBinaryFile(downloadUrl, curImagePath)) {
#if _DEBUG
							LOG("Finished downloading skill icon.");
#endif
							binaryLoadedIcons.push_back(std::pair<uint32_t, std::shared_ptr<std::vector<BYTE>>>(curSkillId, loadBinaryFileData(curImagePath)));
							skillJsonValues[curSkillId] = desc;
						} else {
							LOG("Failed to download skill icon: ", curSkillId);
							throw std::exception(("Failed to download skill icon: " + std::to_string(curSkillId)).c_str());
						}
					}
				}
				catch (std::exception& e) {
					LOG("Error during downloading skill icon image for skill ", std::to_string(curSkillId), ": ", e.what());
					requestedIDs->push_back(curSkillId);
				}
			}
			while (binaryLoadedIcons.size() > 0 && keepLoadThreadRunning) {
				(*checkedIDs)[binaryLoadedIcons.front().first] = true;
				loadedIcons->insert(std::pair<uint32_t, SkillIcon>(binaryLoadedIcons.front().first, SkillIcon(binaryLoadedIcons.front().second, binaryLoadedIcons.front().first)));
				binaryLoadedIcons.pop_front();
			}
		} else {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}

	std::ofstream out(skillJsonFilename);
	nlohmann::json outJson = skillJsonValues;
#if _DEBUG
	out << outJson.dump(2);
#else
	out << outJson.dump();
#endif
		out.close();
#if _DEBUG
		LOG("Skillicon load thread stopping");
#endif
	} catch (const std::exception& e) {
		LOG("Critical error in skill icon load thread: ", e.what());
		LOG("Skill icon loading disabled to prevent crashes");
	} catch (...) {
		LOG("Unknown critical error in skill icon load thread");
		LOG("Skill icon loading disabled to prevent crashes");
	}
}

GW2_SCT::SkillIcon* GW2_SCT::SkillIconManager::getIcon(uint32_t skillID) {
    if (!Options::get()->skillIconsEnabled) {
        return nullptr;
    }

    bool preferEmbedded = Options::get()->preferEmbeddedIcons;
    auto itLoaded = loadedIcons->find(skillID);

    if (preferEmbedded) {
        if (embeddedIconIds->find(skillID) != embeddedIconIds->end()) {
            if (itLoaded != loadedIcons->end()) {
                return &itLoaded->second;
            }
        } else {
            auto embedded = LoadEmbeddedSkillIcon(skillID);
            if (embedded && !embedded->empty()) {
                (*checkedIDs)[skillID] = true;
                embeddedIconIds->insert(skillID);
                if (itLoaded != loadedIcons->end()) {
                    itLoaded->second = SkillIcon(embedded, skillID);
                    return &itLoaded->second;
                }
                auto ins = loadedIcons->insert({ skillID, SkillIcon(embedded, skillID) });
                return &ins.first->second;
            }
        }
    } else {
        if (itLoaded != loadedIcons->end()) {
            return &itLoaded->second;
        }

        auto embedded = LoadEmbeddedSkillIcon(skillID);
        if (embedded && !embedded->empty()) {
            (*checkedIDs)[skillID] = true;
            embeddedIconIds->insert(skillID);
            auto ins = loadedIcons->insert({ skillID, SkillIcon(embedded, skillID) });
            return &ins.first->second;
        }
    }

    if (itLoaded != loadedIcons->end()) {
        return &itLoaded->second;
    }

    if (!Options::get()->preloadAllSkillIcons) {
        if (!(*checkedIDs)[skillID]) {
            requestedIDs->push_back(skillID);
            return nullptr;
        }
    }

    return nullptr;
}

ImVec2 GW2_SCT::SkillIcon::draw(ImVec2 pos, ImVec2 size, ImU32 color) {
    SkillIconDisplayType requestedDisplayType = Options::get()->skillIconsDisplayType;
    if (!texturesCreated[requestedDisplayType]) {
        requestTextureCreation(requestedDisplayType);
    }
    if (textures[requestedDisplayType] == nullptr) {
        return ImVec2(0, 0);
    }

    ImVec2 end(pos.x + size.x, pos.y + size.y);
    if (textures[requestedDisplayType] != nullptr) {
        textures[requestedDisplayType]->draw(pos, size, color);
    }
    else {
        return ImVec2(0, 0);
    }
    return size;
}
GW2_SCT::SkillIcon::SkillIcon(std::shared_ptr<std::vector<BYTE>> fileData, uint32_t skillID)
	: fileData(fileData), skillID(skillID) {}

GW2_SCT::SkillIcon::~SkillIcon() {
	for (auto& texture: textures)
		if (texture.second != nullptr)ImmutableTexture::Release(texture.second);
}

struct HexCharStruct {
	unsigned char c;
	HexCharStruct(unsigned char _c) : c(_c) { }
};

inline std::ostream& operator<<(std::ostream& o, const HexCharStruct& hs) {
	return (o << std::hex << (int)hs.c);
}

inline HexCharStruct hex(unsigned char _c) {
	return HexCharStruct(_c);
}

void logImageData(std::shared_ptr<std::vector<BYTE>> data) {
	std::stringstream str;
	int i = 0;
	for (auto it = data->begin(); it != data->end(); it++) {
		str << " 0x" << std::setfill('0') << std::setw(2) << hex(*it);
		i++;
		if (i >= 16) {
			std::string out = str.str();
			out.erase(out.begin());
			LOG(out);
			str = std::stringstream();
			i = 0;
		}
	}
}

void logImageData(unsigned char* data, size_t size) {
	std::stringstream str;
	int i = 0;
	while (i < size) {
		str << " 0x" << std::setfill('0') << std::setw(2) << hex(*(data + i));
		i++;
		if (i % 16 == 0) {
			std::string out = str.str();
			out.erase(out.begin());
			LOG(out);
			str = std::stringstream();
		}
	}
}

struct ImageDataHelper {
	unsigned char* data;
	int row_size;
private:
	struct ImageDataHelperRow {
		unsigned char* row_start;
		unsigned char* operator[](int x) {
			return { row_start + x * 4 };
		}
		operator unsigned char* () { return row_start; };
	};
public:
	ImageDataHelperRow operator[](int y) {
		return { data + y * 4 * row_size };
	}
};

void setScaledTransparency(unsigned char* cur) {
	int r = cur[0];
	int g = cur[1];
	int b = cur[2];
	cur[3] = std::min(0xfe, (r + r + r + b + g + g + g + g) >> 3 * 0xff / 10);
}

bool pixelIsBlack(unsigned char* cur) {
	int r = cur[0];
	int g = cur[1];
	int b = cur[2];
	return (r + r + r + b + g + g + g + g) >> 3 < 10;
}

bool borderNIsBlack(ImageDataHelper data, int n, int image_width, int image_height) {
	for (int x = n; x < image_width - n; x++) if (!pixelIsBlack(data[n][x])) return false;
	for (int x = n; x < image_width - n; x++) if (!pixelIsBlack(data[image_height - n - 1][x])) return false;
	for (int y = n + 1; y < image_height - n - 1; y++) if (!pixelIsBlack(data[y][n])) return false;
	for (int y = n + 1; y < image_height - n - 1; y++) if (!pixelIsBlack(data[y][image_width - n - 1])) return false;
	return true;
}

void convertRGBAToARGBAndCull(unsigned char* image_data, int image_width, int image_height, GW2_SCT::SkillIconDisplayType displayType) {
	unsigned char* cur = image_data;
	for (int y = 0; y < image_height; y++) {
		for (int x = 0; x < image_width; x++) {
			char red = cur[0];
			cur[0] = cur[2];
			cur[2] = red;
			if (displayType == GW2_SCT::SkillIconDisplayType::BLACK_CULLED) {
				if (pixelIsBlack(cur)) {
					setScaledTransparency(cur);
				}
			}
			cur += 4;
		}
	}
	if (displayType == GW2_SCT::SkillIconDisplayType::BORDER_TOUCHING_BLACK_CULLED) {
		ImageDataHelper image{ image_data, image_width };
		std::queue<std::pair<int, int>> pixelsToResolve;
		for (int x = 0; x < image_width; x++) pixelsToResolve.push(std::pair<int, int>(x, 0));
		for (int x = 0; x < image_width; x++) pixelsToResolve.push(std::pair<int, int>(x, image_height - 1));
		for (int y = 1; y < image_height - 1; y++) pixelsToResolve.push(std::pair<int, int>(0, y));
		for (int y = 1; y < image_height - 1; y++) pixelsToResolve.push(std::pair<int, int>(image_width - 1, y));
		while (pixelsToResolve.size() > 0) {
			auto pixelIndex = pixelsToResolve.front();
			pixelsToResolve.pop();
			cur = image[pixelIndex.second][pixelIndex.first];
			if (cur[3] == 0xff && pixelIsBlack(cur)) {
				setScaledTransparency(cur);
				if (pixelIndex.first - 1 > 0) pixelsToResolve.push(std::pair<int, int>(pixelIndex.first - 1, pixelIndex.second));
				if (pixelIndex.second - 1 > 0) pixelsToResolve.push(std::pair<int, int>(pixelIndex.first, pixelIndex.second - 1));
				if (pixelIndex.first + 1 < image_width) pixelsToResolve.push(std::pair<int, int>(pixelIndex.first + 1, pixelIndex.second));
				if (pixelIndex.first + 1 < image_height) pixelsToResolve.push(std::pair<int, int>(pixelIndex.first, pixelIndex.second + 1));
			}
		}
	}
	if (displayType == GW2_SCT::SkillIconDisplayType::BORDER_BLACK_CULLED) {
		ImageDataHelper image{ image_data, image_width };
		int n = 0;
		while (borderNIsBlack(image, n, image_width, image_height)) {
			for (int x = n; x < image_width - n; x++) setScaledTransparency(image[n][x]);
			for (int x = n; x < image_width - n; x++) setScaledTransparency(image[image_height - n - 1][x]);
			for (int y = n + 1; y < image_height - n - 1; y++) setScaledTransparency(image[y][n]);
			for (int y = n + 1; y < image_height - n - 1; y++) setScaledTransparency(image[y][image_width - n - 1]);
			n++;
		}
	}
}

void GW2_SCT::SkillIcon::createTextureNow(GW2_SCT::SkillIconDisplayType displayType) {
	texturesCreated[displayType] = true;
	textureCreationRequested[displayType] = false;

	if (fileData->size() < 100) {
		LOG("Icon: ", std::to_string(skillID));
		LOG("WARNING - loaded file for texture too small: ", fileData->size());
		logImageData(fileData);
	}

	// Load from file
	int image_width = 0;
	int image_height = 0;
	int nBpp = 4; // Bytes per pixel
	unsigned char* image_data = stbi_load_from_memory(fileData->data(), (int)fileData->size(), &image_width, &image_height, NULL, nBpp);
	if (image_data == NULL) {
		textures[displayType] = nullptr;
		LOG("stbi_load_from_memory image_data == NULL");
		logImageData(fileData);
		return;
	}

	convertRGBAToARGBAndCull(image_data, image_width, image_height, displayType);

	if (textures[displayType] != nullptr) ImmutableTexture::Release(textures[displayType]);
	textures[displayType] = ImmutableTexture::Create(image_width, image_height, image_data);
	if (textures[displayType] == nullptr) {
		LOG("Could not upload skill icon data to graphics system.");
	}

	stbi_image_free(image_data);
}


struct PendingTextureData {
	GW2_SCT::SkillIconDisplayType displayType;
	GW2_SCT::SkillIcon* icon;
};

namespace GW2_SCT {
	static std::queue<std::shared_ptr<PendingTextureData>> pendingIconTextures;
	static std::mutex pendingIconTexturesMutex;
}

void GW2_SCT::SkillIcon::requestTextureCreation(GW2_SCT::SkillIconDisplayType displayType) {
	if (texturesCreated[displayType] || textureCreationRequested[displayType]) {
		return;
	}

	textureCreationRequested[displayType] = true;

	auto pendingData = std::make_shared<PendingTextureData>();
	pendingData->displayType = displayType;
	pendingData->icon = this;

	std::lock_guard<std::mutex> lock(pendingIconTexturesMutex);
	pendingIconTextures.push(pendingData);
}

void GW2_SCT::SkillIcon::ProcessPendingIconTextures() {
	std::lock_guard<std::mutex> lock(pendingIconTexturesMutex);
	const int maxTexturesPerFrame = 3;
	int processed = 0;

	while (!pendingIconTextures.empty() && processed < maxTexturesPerFrame) {
		auto pendingData = pendingIconTextures.front();
		pendingIconTextures.pop();

		if (pendingData && pendingData->icon) {
			pendingData->icon->createTextureNow(pendingData->displayType);
		}
		processed++;
	}
}
