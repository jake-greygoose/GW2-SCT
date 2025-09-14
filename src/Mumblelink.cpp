#include "Mumblelink.h"
#include "Common.h"
#include "json.hpp"
#include <codecvt>
#include <locale>
#include <sstream>
#include <set>

namespace GW2_SCT {

static std::wstring GetCommandLineArg(const std::wstring& key) {
    const wchar_t* cmdLine = GetCommandLineW();
    if (!cmdLine) return L"";

    std::wstring cmdString(cmdLine);
    std::wistringstream ss(cmdString);
    std::wstring token;

    while (ss >> token) {
        if (token == key) {
            if (ss >> token) {
                if (token.length() >= 2 && token.front() == L'"' && token.back() == L'"') {
                    return token.substr(1, token.length() - 2);
                }
                return token;
            }
        }
    }
    return L"";
}

MumbleLink& MumbleLink::i() {
    static MumbleLink instance;
    return instance;
}

MumbleLink::MumbleLink() {
    std::wstring customName = GetCommandLineArg(L"-mumble");
    if (!customName.empty()) {
        linkName = customName;
    }
    
    initialize();
}

MumbleLink::~MumbleLink() {
    shutdown();
}

bool MumbleLink::initialize() {
    if (initialized) {
        return true;
    }

    std::string linkNameStr(linkName.begin(), linkName.end());
    
    if (linkNameStr == "MumbleLink") {
        hMapFile = OpenFileMappingW(FILE_MAP_READ, FALSE, linkName.c_str());
        if (hMapFile == nullptr) {
            hMapFile = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, linkName.c_str());
            if (hMapFile == nullptr) {
                return false;
            }
        }
    } else {
        hMapFile = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(MumbleLinkData), linkName.c_str());
        if (hMapFile == nullptr) {
            return false;
        }
    }

    DWORD mapAccess = (linkNameStr == "MumbleLink") ? FILE_MAP_READ : FILE_MAP_ALL_ACCESS;
    pMumbleData = static_cast<MumbleLinkData*>(MapViewOfFile(hMapFile, mapAccess, 0, 0, sizeof(MumbleLinkData)));
    if (pMumbleData == nullptr) {
        CloseHandle(hMapFile);
        hMapFile = nullptr;
        return false;
    }

    initialized = true;
    return true;
}

void MumbleLink::shutdown() {
    if (pMumbleData != nullptr) {
        UnmapViewOfFile(pMumbleData);
        pMumbleData = nullptr;
    }
    
    if (hMapFile != nullptr) {
        CloseHandle(hMapFile);
        hMapFile = nullptr;
    }
    
    initialized = false;
    cachedCharacterName.clear();
    lastTick = 0;
}

void MumbleLink::onUpdate() {
    if (!initialized && !initialize()) {
        return;
    }
    
    if (pMumbleData == nullptr) {
        return;
    }
    
    if (pMumbleData->uiTick != lastTick) {
        lastTick = pMumbleData->uiTick;
        
        std::wstring newCharacterName;
        unsigned currentMapId = 0;
        bool newIsInWvW = false;
        
        if (pMumbleData->identity[0] != L'\0') {
            try {
                std::wstring identityWStr(pMumbleData->identity);
                std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
                std::string identityStr = converter.to_bytes(identityWStr);
                
                auto identityJson = nlohmann::json::parse(identityStr);
                
                if (identityJson.contains("name") && identityJson["name"].is_string()) {
                    std::string characterNameStr = identityJson["name"];
                    newCharacterName = converter.from_bytes(characterNameStr);
                }
                
                if (identityJson.contains("map_id") && identityJson["map_id"].is_number()) {
                    currentMapId = identityJson["map_id"].get<unsigned>();
                    
                    std::set<unsigned> wvwMapIds = {
                        38,   // Eternal Battlegrounds
                        95, 96, 94,  // Borderlands (Red, Blue, Green)
                        968,  // Edge of the Mists
                        899,  // Obsidian Sanctum
                        1315  // Armistice Bastion
                    };
                    
                    newIsInWvW = wvwMapIds.find(currentMapId) != wvwMapIds.end();
                    
                }
            } catch (...) {
            }
        }
        
        if (!newCharacterName.empty() && newCharacterName != cachedCharacterName) {
            cachedCharacterName = newCharacterName;
        } else if (newCharacterName.empty() && !cachedCharacterName.empty()) {
            cachedCharacterName.clear();
        }
        
        cachedIsInWvW = newIsInWvW;
    }
}

std::wstring MumbleLink::characterName() const {
    return cachedCharacterName;
}

bool MumbleLink::isValidData() const {
    return initialized && pMumbleData != nullptr && pMumbleData->uiVersion != 0;
}

bool MumbleLink::isInGame() const {
    return isValidData() && !cachedCharacterName.empty();
}

bool MumbleLink::isInWvW() const {
    return isInGame() && cachedIsInWvW;
}

} // namespace GW2_SCT