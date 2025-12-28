#pragma once

#ifndef SCT_VERSION_STRING
#define SCT_VERSION_STRING "3.0.0.3"
#endif
#define langStringG(KEY) GW2_SCT::Language::get(KEY)
#define langString(SECTION, KEY) GW2_SCT::Language::get(SECTION, KEY)
#define langStringImguiSafe(SECTION, KEY) GW2_SCT::Language::get(SECTION, KEY, true)

#include <Windows.h>
#include <string>
#include <map>
#include <vector>
#include <d3d9.h>
#include <d3d11.h>
#include "FontManager.h"

#include <stdarg.h>
#include <sstream>
#include <fstream>
template<typename T> void log_rec(std::stringstream& strStream, T t) {
	strStream << t;
}
template<typename T, typename... Args> void log_rec(std::stringstream& strStream, T t, Args... args) {
	strStream << t;
	log_rec(strStream, args...);
}

extern std::ofstream logFile;
extern size_t (*arcLogWindowFunc)(char*);
void SetArcDpsLogFunctions(size_t (*logWindowFn)(char*));

#ifdef _DEBUG
#define _CRT_SECURE_NO_WARNINGS
extern HANDLE debug_console_hnd;
#endif

template<typename... Args> void log(Args... args) {
	std::stringstream strStream;
	strStream << "GW2 SCT: ";
	log_rec(strStream, args...);
	strStream << "\n";
	std::string message = strStream.str();
#ifdef _DEBUG
	OutputDebugString(message.c_str());
	DWORD written = 0;
	WriteConsoleA(debug_console_hnd, message.c_str(), (DWORD)message.length(), &written, 0);
#endif
	if (logFile.is_open()) {
		logFile << message;
	}
	if (arcLogWindowFunc) {
		arcLogWindowFunc(const_cast<char*>(message.c_str()));
	}
}
#define LOG(...) log(__VA_ARGS__);

extern float windowHeight;
extern float windowWidth;

extern std::map<int, std::pair<std::string, GW2_SCT::FontType*>> fontMap;
extern GW2_SCT::FontType* defaultFont;
extern char* arcvers;

const float defaultFontSize = 22.f;

extern std::string AbbreviateSkillName(const std::string& skillName);
extern std::string ShortenNumber(double number, int precision = 0);

/* combat event */
struct cbtevent {
	uint64_t time; /* timegettime() at time of event */
	uint64_t src_agent; /* unique identifier */
	uint64_t dst_agent; /* unique identifier */
	int32_t value; /* event-specific */
	int32_t buff_dmg; /* estimated buff damage. zero on application event */
	uint16_t overstack_value; /* estimated overwritten stack duration for buff application */
	uint16_t skillid; /* skill id */
	uint16_t src_instid; /* agent map instance id */
	uint16_t dst_instid; /* agent map instance id */
	uint16_t src_master_instid; /* master source agent map instance id if source is a minion/pet */
	uint8_t iss_offset; /* internal tracking. garbage */
	uint8_t iss_offset_target; /* internal tracking. garbage */
	uint8_t iss_bd_offset; /* internal tracking. garbage */
	uint8_t iss_bd_offset_target; /* internal tracking. garbage */
	uint8_t iss_alt_offset; /* internal tracking. garbage */
	uint8_t iss_alt_offset_target; /* internal tracking. garbage */
	uint8_t skar; /* internal tracking. garbage */
	uint8_t skar_alt; /* internal tracking. garbage */
	uint8_t skar_use_alt; /* internal tracking. garbage */
	uint8_t iff; /* from iff enum */
	uint8_t buff; /* buff application, removal, or damage event */
	uint8_t result; /* from cbtresult enum */
	uint8_t is_activation; /* from cbtactivation enum */
	uint8_t is_buffremove; /* buff removed. src=relevant, dst=caused it (for strips/cleanses). from cbtr enum  */
	uint8_t is_ninety; /* source agent health was over 90% */
	uint8_t is_fifty; /* target agent health was under 50% */
	uint8_t is_moving; /* source agent was moving */
	uint8_t is_statechange; /* from cbtstatechange enum */
	uint8_t is_flanking; /* target agent was not facing source */
	uint8_t is_shields; /* all or partial damage was vs barrier/shield */
	uint8_t is_offcycle; /* zero if buff dmg happened during tick, non-zero otherwise */
	uint8_t pad64; /* internal tracking. garbage */
};

/* combat event - for logging (revision 1, when byte16 == 1) */
struct cbtevent1 {
	uint64_t time; /* timegettime() at time of event */
	uint64_t src_agent; /* unique identifier */
	uint64_t dst_agent; /* unique identifier */
	int32_t value; /* event-specific */
	int32_t buff_dmg; /* estimated buff damage. zero on application event */
	uint32_t overstack_value; /* estimated overwritten stack duration for buff application */
	uint32_t skillid; /* skill id */
	uint16_t src_instid; /* agent map instance id */
	uint16_t dst_instid; /* agent map instance id */
	uint16_t src_master_instid; /* master source agent map instance id if source is a minion/pet */
	uint16_t dst_master_instid; /* master destination agent map instance id if destination is a minion/pet */
	uint8_t iff; /* from iff enum */
	uint8_t buff; /* buff application, removal, or damage event */
	uint8_t result; /* from cbtresult enum */
	uint8_t is_activation; /* from cbtactivation enum */
	uint8_t is_buffremove; /* buff removed. src=relevant, dst=caused it (for strips/cleanses). from cbtr enum  */
	uint8_t is_ninety; /* source agent health was over 90% */
	uint8_t is_fifty; /* target agent health was under 50% */
	uint8_t is_moving; /* source agent was moving */
	uint8_t is_statechange; /* from cbtstatechange enum */
	uint8_t is_flanking; /* target agent was not facing source */
	uint8_t is_shields; /* all or partial damage was vs barrier/shield */
	uint8_t is_offcycle;
	uint8_t pad61;
	uint8_t pad62;
	uint8_t pad63;
	uint8_t pad64;
};

/* combat result (physical) */
enum cbtresult {
	CBTR_NORMAL, // good physical hit
	CBTR_CRIT, // physical hit was crit
	CBTR_GLANCE, // physical hit was glance
	CBTR_BLOCK, // physical hit was blocked eg. mesmer shield 4
	CBTR_EVADE, // physical hit was evaded, eg. dodge or mesmer sword 2
	CBTR_INTERRUPT, // physical hit interrupted something
	CBTR_ABSORB, // physical hit was "invlun" or absorbed eg. guardian elite
	CBTR_BLIND, // physical hit missed
	CBTR_KILLINGBLOW, // hit was killing hit
	CBTR_DOWNED, // hit was downing hit
};

/* agent short */
typedef struct ag {
	const char* name; /* agent name. may be null. valid only at time of event. utf8 */
	uintptr_t id; /* agent unique identifier */
	uint32_t prof; /* profession at time of event. refer to evtc notes for identification */
	uint32_t elite; /* elite spec at time of event. refer to evtc notes for identification */
	uint32_t self; /* 1 if self, 0 if not */
	uint16_t team; /* sep21+ */
} ag;

/* iff */
enum iff {
	IFF_FRIEND, // green vs green, red vs red
	IFF_FOE, // green vs red
	IFF_UNKNOWN // something very wrong happened
};

enum profession {
	PROFESSION_UNDEFINED = 0,
	PROFESSION_GUARDIAN,
	PROFESSION_WARRIOR,
	PROFESSION_ENGINEER,
	PROFESSION_RANGER,
	PROFESSION_THIEF,
	PROFESSION_ELEMENTALIST,
	PROFESSION_MESMER,
	PROFESSION_NECROMANCER,
	PROFESSION_REVENANT
};

namespace GW2_SCT {
	extern uint32_t d3dversion;
	extern ID3D11Device* d3Device11;
	extern ID3D11DeviceContext* d3D11Context;
	extern IDXGISwapChain* d3d11SwapChain;

	enum class MessageCategory {
		PLAYER_OUT = 0,
		PLAYER_IN,
		PET_OUT,
		PET_IN
	};
#define NUM_CATEGORIES 4
	int messageCategoryToInt(MessageCategory category);
	MessageCategory intToMessageCategory(int i);

	enum class MessageType {
		NONE = 0,
		PHYSICAL,
		CRIT,
		BLEEDING,
		BURNING,
		POISON,
        CONFUSION,
        TORMENT,
		DOT,
		HEAL,
		HOT,
		SHIELD_RECEIVE,
		SHIELD_REMOVE,
		BLOCK,
		EVADE,
		INVULNERABLE,
		MISS
	};
#define NUM_MESSAGE_TYPES 17
	int messageTypeToInt(MessageType type);
	MessageType intToMessageType(int i);
}

std::string getExePath();
std::string getSCTPath();
bool file_exist(const std::string& name);
bool getFilesInDirectory(std::string path, std::vector<std::string>& files);
ImU32 stoc(std::string);

typedef int FontId;
GW2_SCT::FontType* getFontType(int, bool withMaster = true);

bool floatEqual(float a, float b);

std::string fotos(FontId i, bool withMaster = true);
FontId stofo(std::string const& s, bool withMaster = true);

