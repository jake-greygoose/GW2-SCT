#pragma once
#include <windows.h>
#include <string>

namespace GW2_SCT {

struct Vector2
{
    float X;
    float Y;
};

struct Vector3
{
    float X;
    float Y;
    float Z;
};

enum class EMapType : unsigned char
{
    AutoRedirect,
    CharacterCreation,
    PvP,
    GvG,
    Instance,
    Public,
    Tournament,
    Tutorial,
    UserTournament,
    WvW_EternalBattlegrounds,
    WvW_BlueBorderlands,
    WvW_GreenBorderlands,
    WvW_RedBorderlands,
    WvW_FortunesVale,
    WvW_ObsidianSanctum,
    WvW_EdgeOfTheMists,
    Public_Mini,
    BigBattle,
    WvW_Lounge
};

enum class EMountIndex : unsigned char
{
    None,
    Jackal,
    Griffon,
    Springer,
    Skimmer,
    Raptor,
    RollerBeetle,
    Warclaw,
    Skyscale,
    Skiff,
    SiegeTurtle
};

enum class EProfession : unsigned char
{
    None,
    Guardian,
    Warrior,
    Engineer,
    Ranger,
    Thief,
    Elementalist,
    Mesmer,
    Necromancer,
    Revenant
};

enum class ERace : unsigned char
{
    Asura,
    Charr,
    Human,
    Norn,
    Sylvari
};

enum class EUIScale : unsigned char
{
    Small,
    Normal,
    Large,
    Larger
};

struct Identity
{
    char            Name[20];
    EProfession     Profession;
    unsigned        Specialization;
    ERace           Race;
    unsigned        MapID;
    unsigned        WorldID;
    unsigned        TeamColorID;
    bool            IsCommander;
    float           FOV;
    EUIScale        UISize;
};

struct Compass
{
    unsigned short  Width;
    unsigned short  Height;
    float           Rotation;
    Vector2         PlayerPosition;
    Vector2         Center;
    float           Scale;
};

struct Context
{
    unsigned char   ServerAddress[28];
    unsigned        MapID;
    EMapType        MapType;
    unsigned        ShardID;
    unsigned        InstanceID;
    unsigned        BuildID;
    unsigned        IsMapOpen            : 1;
    unsigned        IsCompassTopRight    : 1;
    unsigned        IsCompassRotating    : 1;
    unsigned        IsGameFocused        : 1;
    unsigned        IsCompetitive        : 1;
    unsigned        IsTextboxFocused     : 1;
    unsigned        IsInCombat           : 1;
    Compass         Compass;
    unsigned        ProcessID;
    EMountIndex     MountIndex;
};

struct MumbleLinkData {
    unsigned int uiVersion;
    unsigned int uiTick;
    Vector3 fAvatarPosition;
    Vector3 fAvatarFront;
    Vector3 fAvatarTop;
    wchar_t name[256];
    Vector3 fCameraPosition;
    Vector3 fCameraFront;
    Vector3 fCameraTop;
    wchar_t identity[256];
    unsigned int context_len;
    Context context;
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
    bool isInWvW() const;
    
private:
    MumbleLink();
    ~MumbleLink();
    
    HANDLE hMapFile = nullptr;
    MumbleLinkData* pMumbleData = nullptr;
    
    std::wstring cachedCharacterName;
    std::wstring linkName = L"MumbleLink";
    unsigned int lastTick = 0;
    bool initialized = false;
    bool cachedIsInWvW = false;
};

} // namespace GW2_SCT