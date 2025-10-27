#include "Common.h"
#include "Language.h"
#include <fstream>

#ifdef _DEBUG
HANDLE debug_console_hnd;
#endif

uint32_t GW2_SCT::d3dversion;
ID3D11Device* GW2_SCT::d3Device11;
ID3D11DeviceContext* GW2_SCT::d3D11Context;
IDXGISwapChain* GW2_SCT::d3d11SwapChain;
std::ofstream logFile;
size_t (*arcLogWindowFunc)(char*) = nullptr;
char* arcvers;
bool ensuredSCTDirectoryExists = false;

void SetArcDpsLogFunctions(size_t (*logWindowFn)(char*)) {
	arcLogWindowFunc = logWindowFn;
}

std::map<int, std::pair<std::string, GW2_SCT::FontType*>> fontMap;
GW2_SCT::FontType* defaultFont;

std::string getExePath() {
	char buffer[MAX_PATH];
	GetModuleFileName(NULL, buffer, MAX_PATH);
	std::string::size_type pos = std::string(buffer).find_last_of("\\/");
	return std::string(buffer).substr(0, pos);
}

std::string getSCTPath() {
	std::string addonPath = getExePath() + "\\addons\\sct\\";
	if (!ensuredSCTDirectoryExists) {
		std::string addonPath = getExePath() + "\\addons\\";
		CreateDirectory(addonPath.c_str(), NULL);
		addonPath += "sct\\";
		CreateDirectory(addonPath.c_str(), NULL);
		ensuredSCTDirectoryExists = true;
		LOG("Created SCT addon dir.");
	}
	return addonPath;
}

bool file_exist(const std::string& name) {
	struct stat buffer;
	return (stat(name.c_str(), &buffer) == 0);
}

bool getFilesInDirectory(std::string path, std::vector<std::string>& files) {
	HANDLE hFind = INVALID_HANDLE_VALUE;
	WIN32_FIND_DATA ffd;
	std::string spec;

	spec = path + "*";

	hFind = FindFirstFile(spec.c_str(), &ffd);
	if (hFind == INVALID_HANDLE_VALUE) {
		return false;
	}

	do {
		if (strcmp(ffd.cFileName, ".") != 0 &&
			strcmp(ffd.cFileName, "..") != 0) {
			files.push_back(ffd.cFileName);
		}
	} while (FindNextFile(hFind, &ffd) != 0);

	if (GetLastError() != ERROR_NO_MORE_FILES) {
		FindClose(hFind);
		return false;
	}

	FindClose(hFind);
	hFind = INVALID_HANDLE_VALUE;

	return true;
}

ImU32 stoc(std::string s) {
	int num = stoi(s, 0, 16);
	int r = num / 0x10000;
	int g = (num / 0x100) % 0x100;
	int b = num % 0x100;
	return ImGui::GetColorU32(ImVec4(r / 255.f, g / 255.f, b / 255.f, 1.f));
}

GW2_SCT::FontType* getFontType(int fontId, bool withMaster) {
	GW2_SCT::FontType* font = defaultFont;
	// -1 because of ID 0 being master font
	if (fontMap.find(fontId - (withMaster ? 1 : 0)) != fontMap.end()) {
		font = fontMap[fontId - (withMaster ? 1 : 0)].second;
	}
	return font;
}

std::string fotos(FontId i, bool withMaster) {
	if (fontMap.find(i - (withMaster ? 1 : 0)) != fontMap.end()) {
		return fontMap[i - (withMaster ? 1 : 0)].first;
	}
	return withMaster ? langStringG(GW2_SCT::LanguageKey::Font_Master) : langStringG(GW2_SCT::LanguageKey::Font_Default);
}

FontId stofo(std::string const& s, bool withMaster) {
	for (std::map<int, std::pair<std::string, GW2_SCT::FontType*>>::iterator it = fontMap.begin(); it != fontMap.end(); ++it) {
		if (it->second.first.compare(s) == 0) {
			return it->first + (withMaster ? 1 : 0);
		}
	}
	return 0;
}

int GW2_SCT::messageCategoryToInt(MessageCategory category) {
	return (int)category;
}

GW2_SCT::MessageCategory GW2_SCT::intToMessageCategory(int i) {
	if (i < 0 || i > NUM_CATEGORIES) return (MessageCategory)0;
	return (MessageCategory)i;
}

int GW2_SCT::messageTypeToInt(MessageType type) {
	return (int)type;
}

GW2_SCT::MessageType GW2_SCT::intToMessageType(int i) {
	if (i < 0 || i > NUM_MESSAGE_TYPES) return (MessageType)0;
	return (MessageType)i;
}

bool floatEqual(float a, float b) {
	float diff = a - b;
	return (diff < std::numeric_limits<float>::epsilon()) && (-diff < std::numeric_limits<float>::epsilon());
}

std::string AbbreviateSkillName(const std::string& skillName) {
	if (skillName.find_first_of(" -") == std::string::npos) {
		return skillName;
	}

	std::string result;
	bool nextIsFirst = true;

	for (char c : skillName) {
		if (std::isalpha(c)) {
			if (nextIsFirst) {
				result += c;
				nextIsFirst = false;
			}
		}
		else if (c == ' ' || c == '-') {
			nextIsFirst = true;
		}
	}

	return result;
}

std::string ShortenNumber(double number, int precision) {
	if (std::isnan(number) || std::isinf(number) || number < 0) {
		return "0";
	}
	if (precision == -1) {
		return std::to_string(static_cast<long long>(std::round(number)));
	}

	precision = std::max(0, std::min(3, precision));

	auto cleanDecimal = [](std::string str) -> std::string {
		if (str.find('.') != std::string::npos) {
			str.erase(str.find_last_not_of('0') + 1);
			if (str.back() == '.') {
				str.pop_back();
			}
		}
		return str;
		};

	auto formatValue = [&cleanDecimal](double value, const std::string& suffix, int prec) -> std::string {
		std::ostringstream oss;
		oss << std::fixed << std::setprecision(prec) << value;
		return cleanDecimal(oss.str()) + suffix;
		};

	if (number >= 1e12) {
		return formatValue(number / 1e12, "T", precision);
	}
	else if (number >= 1e9) {
		return formatValue(number / 1e9, "B", precision);
	}
	else if (number >= 1e6) {
		return formatValue(number / 1e6, "M", precision);
	}
	else if (number >= 1e3) {
		return formatValue(number / 1e3, "k", precision);
	}
	else {
		if (precision == 0) {
			return std::to_string(static_cast<int>(std::round(number)));
		}
		else {
			std::ostringstream oss;
			oss << std::fixed << std::setprecision(precision) << number;
			return cleanDecimal(oss.str());
		}
	}
}