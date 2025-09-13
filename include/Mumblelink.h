#pragma once
#include <windows.h>
#include <string>

namespace GW2_SCT {

struct MumbleLinkData {
    unsigned int uiVersion;
    unsigned int uiTick;
    float fAvatarPosition[3];
    float fAvatarFront[3];
    float fAvatarTop[3];
    wchar_t name[256];
    float fCameraPosition[3];
    float fCameraFront[3];
    float fCameraTop[3];
    wchar_t identity[256];
    unsigned int context_len;
    unsigned char context[256];
    wchar_t description[2048];
};

class MumbleLink {
public:
    static MumbleLink& i();
    
    bool initialize();
    void shutdown();
    void onUpdate();
    
    std::wstring characterName() const;
    bool isValidData() const;
    bool isInGame() const;
    
private:
    MumbleLink();
    ~MumbleLink();
    
    HANDLE hMapFile = nullptr;
    MumbleLinkData* pMumbleData = nullptr;
    
    std::wstring cachedCharacterName;
    std::wstring linkName = L"MumbleLink";
    unsigned int lastTick = 0;
    bool initialized = false;
};

} // namespace GW2_SCT