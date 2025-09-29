#include "Updater.h"

#include "Common.h"
#include "httpclient.h"

#include <mutex>
#include <thread>
#include <vector>
#include <filesystem>
#include <fstream>
#include <charconv>
#include <tuple>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <utility>
#include <cctype>
#include <cstdio>
#include <cerrno>
#include <windows.h>
#include <shellapi.h>

#include "json.hpp"
#include "imgui.h"
#include "Options.h"
#include "Language.h"

namespace GW2_SCT {

template<typename... Args>
static std::string FormatLang(LanguageCategory category, LanguageKey key, Args&&... args) {
    const char* fmt = langString(category, key);
    if constexpr (sizeof...(Args) == 0) {
        return std::string(fmt);
    }
    int required = std::snprintf(nullptr, 0, fmt, std::forward<Args>(args)...);
    if (required <= 0) {
        return std::string(fmt);
    }
    std::string buffer(static_cast<size_t>(required) + 1, '\0');
    std::snprintf(buffer.data(), buffer.size(), fmt, std::forward<Args>(args)...);
    buffer.resize(static_cast<size_t>(required));
    return buffer;
}

struct Updater::UpdateState {
    enum class Status {
        Unknown = 0,
        UpdateAvailable,
        UpdateInProgress,
        UpdateSuccessful,
        UpdateError,
        Dismissed
    } status = Status::Unknown;

    std::mutex lock;
    std::vector<std::thread> tasks;

    using Version = std::array<uint32_t,4>;
    std::optional<Version> currentVersion;
    Version newVersion{0,0,0,0};

    std::string installPath;
    std::string downloadUrl;
    std::string releasePageUrl;
    std::string tagName;

    std::optional<std::chrono::system_clock::time_point> lastCheckTime;
    std::string lastCheckMessage;
    bool lastCheckHadError = false;
    bool lastCheckRateLimited = false;
    bool lastCheckFoundUpdate = false;

    ~UpdateState() {
        for (auto &t : tasks) { if (t.joinable()) t.join(); }
    }
};

std::unique_ptr<Updater::UpdateState> Updater::s_state;

static std::pair<std::string, std::string> ParseRepoSlug(const std::string& slug) {
    std::string owner = "jake-greygoose";
    std::string repo = "GW2-SCT";
    auto pos = slug.find('/');
    if (pos != std::string::npos && pos + 1 < slug.size()) {
        owner = slug.substr(0, pos);
        repo = slug.substr(pos + 1);
    }
    return {owner, repo};
}

enum class ReleaseSource {
    GitHub = 0,
    Forgejo
};

struct FetchResult {    bool found = false;
    std::array<uint32_t,4> version{0,0,0,0};
    std::string assetUrl;
    std::string htmlUrl;
    std::string tagName;
    std::string error;
    bool rateLimited = false;
    ReleaseSource source = ReleaseSource::GitHub;
};

static std::array<uint32_t,4> ParseVersionAny(std::string_view s) {
    std::array<uint32_t,4> v{0,0,0,0};
    size_t idx = 0, part = 0;
    while (idx < s.size() && part < 4) {
        while (idx < s.size() && (s[idx] < '0' || s[idx] > '9')) idx++;
        if (idx >= s.size()) break;
        uint32_t val = 0;
        size_t start = idx;
        while (idx < s.size() && (s[idx] >= '0' && s[idx] <= '9')) idx++;
        auto token = s.substr(start, idx - start);
        std::from_chars(token.data(), token.data() + token.size(), val);
        v[part++] = val;
        while (idx < s.size() && s[idx] != '\0' && s[idx] != '.' && (s[idx] < '0' || s[idx] > '9')) idx++;
        if (idx < s.size() && s[idx] == '.') idx++;
    }
    return v;
}

static std::string VersionToString(const std::array<uint32_t,4>& v) {
    char buffer[48]{};
    std::snprintf(buffer, sizeof(buffer), "%u.%u.%u.%u", v[0], v[1], v[2], v[3]);
    return std::string(buffer);
}

static std::string FormatTimestamp(const std::chrono::system_clock::time_point& tp) {
    auto timeT = std::chrono::system_clock::to_time_t(tp);
    std::tm local{};
#if defined(_WIN32)
    localtime_s(&local, &timeT);
#else
    localtime_r(&timeT, &local);
#endif
    std::ostringstream oss;
    oss << std::put_time(&local, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

static const char* ReleaseSourceLabel(ReleaseSource src) {
    switch (src) {
    case ReleaseSource::GitHub: return "GitHub";
    case ReleaseSource::Forgejo: return "Forgejo";
    default: return "Unknown";
    }
}

static const char* ReleaseSourceDisplayName(ReleaseSource src) {
    switch (src) {
    case ReleaseSource::GitHub:
        return langString(LanguageCategory::Option_UI, LanguageKey::Update_Source_GitHub);
    case ReleaseSource::Forgejo:
        return langString(LanguageCategory::Option_UI, LanguageKey::Update_Source_ForgejoMirror);
    default:
        return langString(LanguageCategory::Option_UI, LanguageKey::Update_Source_Unknown);
    }
}

static bool IsNewer(const std::array<uint32_t,4>& a, const std::array<uint32_t,4>& b) {
    if (std::tie(a[0],a[1],a[2]) != std::tie(b[0],b[1],b[2]))
        return std::tie(a[0],a[1],a[2]) > std::tie(b[0],b[1],b[2]);
    return a[3] > b[3];
}

static std::optional<std::array<uint32_t,4>> GetCurrentFileVersion(const std::string& path) {
    DWORD dummy = 0;
    DWORD size = GetFileVersionInfoSizeA(path.c_str(), &dummy);
    if (size == 0) return std::nullopt;
    std::vector<BYTE> data(size);
    if (!GetFileVersionInfoA(path.c_str(), 0, size, data.data())) return std::nullopt;
    VS_FIXEDFILEINFO* ffi = nullptr; UINT len = 0;
    if (!VerQueryValueA(data.data(), "\\", reinterpret_cast<void**>(&ffi), &len)) return std::nullopt;
    std::array<uint32_t,4> v{
        HIWORD(ffi->dwProductVersionMS),
        LOWORD(ffi->dwProductVersionMS),
        HIWORD(ffi->dwProductVersionLS),
        LOWORD(ffi->dwProductVersionLS)
    };
    return v;
}

static std::optional<std::string> HttpGet(const std::string& url) {
    auto w = HTTPClient::StringToWString(url);
    std::string resp = HTTPClient::GetRequest(w);
    if (resp.empty()) return std::nullopt;
    return resp;
}



static BOOL CALLBACK EnumWindowsForProcessProc(HWND hwnd, LPARAM lParam) {
    DWORD windowPid = 0;
    GetWindowThreadProcessId(hwnd, &windowPid);
    if (windowPid == GetCurrentProcessId()) {
        auto windows = reinterpret_cast<std::vector<HWND>*>(lParam);
        windows->push_back(hwnd);
    }
    return TRUE;
}

static std::vector<HWND> GetProcessTopLevelWindows() {
    std::vector<HWND> windows;
    EnumWindows(EnumWindowsForProcessProc, reinterpret_cast<LPARAM>(&windows));
    return windows;
}

static bool HttpDownloadToFile(const std::string& url, const std::filesystem::path& out) {
    auto w = HTTPClient::StringToWString(url);
    std::string resp = HTTPClient::GetRequest(w);
    if (resp.empty()) return false;
    std::ofstream f(out, std::ios::binary);
    if (!f) return false;
    f.write(resp.data(), static_cast<std::streamsize>(resp.size()));
    return f.good();
}

static FetchResult FetchLatestRelease(const std::string& owner, const std::string& repo, bool includePrerelease, ReleaseSource source) {
    FetchResult result;
    result.source = source;

    std::string apiUrl;
    switch (source) {
    case ReleaseSource::GitHub:
        apiUrl = std::string("https://api.github.com/repos/") + owner + "/" + repo + "/releases";
        break;
    case ReleaseSource::Forgejo:
        apiUrl = std::string("https://git.gay/api/v1/repos/") + owner + "/" + repo + "/releases";
        break;
    default:
        apiUrl = std::string("https://api.github.com/repos/") + owner + "/" + repo + "/releases";
        break;
    }

    auto bodyOpt = HttpGet(apiUrl);
    if (!bodyOpt) {
        LOG("Updater: HTTP GET failed for ", apiUrl);
        result.error = FormatLang(LanguageCategory::Option_UI, LanguageKey::Update_Message_FetchFailed, ReleaseSourceDisplayName(source));
        return result;
    }
    try {
        auto j = nlohmann::json::parse(*bodyOpt);
        if (!j.is_array()) {
            std::string snippet = bodyOpt->substr(0, 200);
            LOG("Updater: Releases API returned non-array (", ReleaseSourceLabel(source), "). Body snippet: ", snippet);
            if (j.is_object()) {
                std::string message = j.value("message", std::string{});
                if (!message.empty()) {
                    if (source == ReleaseSource::GitHub && message.find("API rate limit exceeded") != std::string::npos) {
                        result.rateLimited = true;
                        result.error = FormatLang(LanguageCategory::Option_UI, LanguageKey::Update_Message_RateLimited);
                    } else {
                        result.error = message;
                    }
                }
            }
            if (result.error.empty()) {
                result.error = FormatLang(LanguageCategory::Option_UI, LanguageKey::Update_Message_UnexpectedResponse, ReleaseSourceDisplayName(source));
            }
            return result;
        }
        size_t count = j.size();
        if (count == 0) {
            LOG("Updater: Releases API returned empty array for ", owner, "/", repo, " (", ReleaseSourceLabel(source), ")");
            result.error = FormatLang(LanguageCategory::Option_UI, LanguageKey::Update_Message_NoReleases);
            return result;
        }
    FetchResult bestCandidate;
    bool haveCandidate = false;

    for (const auto& rel : j) {
        if (rel.value("draft", false)) continue;
        if (!includePrerelease && rel.value("prerelease", false)) continue;

        std::string tag = rel.value("tag_name", std::string{});
        auto version = ParseVersionAny(tag);
        std::string htmlUrl = rel.value("html_url", std::string{});
        std::string chosenAssetUrl;
        if (rel.contains("assets") && rel["assets"].is_array()) {
            const char* preferredNames[] = { "gw2-sct.dll", "d3d9_arcdps_sct.dll" };
            for (const char* pref : preferredNames) {
                std::string lowerPreferred = pref;
                for (auto& c : lowerPreferred) c = (char)tolower(c);
                for (const auto& asset : rel["assets"]) {
                    auto name = asset.value("name", std::string{});
                    std::string nameLower = name; for (auto& c : nameLower) c = (char)tolower(c);
                    if (nameLower == lowerPreferred) {
                        chosenAssetUrl = asset.value("browser_download_url", std::string{});
                        break;
                    }
                }
                if (!chosenAssetUrl.empty()) break;
            }
            if (chosenAssetUrl.empty()) {
                for (const auto& asset : rel["assets"]) {
                    auto name = asset.value("name", std::string{});
                    std::string nameLower = name; for (auto& c : nameLower) c = (char)tolower(c);
                    if (nameLower.size() >= 4 && nameLower.rfind(".dll") == nameLower.size() - 4) {
                        chosenAssetUrl = asset.value("browser_download_url", std::string{});
                        break;
                    }
                }
            }
            if (chosenAssetUrl.empty() && !rel["assets"].empty()) {
                chosenAssetUrl = rel["assets"][0].value("browser_download_url", std::string{});
            }
        }

        LOG("Updater: Found release ", tag, " via ", ReleaseSourceLabel(source), ", asset=", chosenAssetUrl.empty() ? "<none>" : chosenAssetUrl.c_str());

        if (!haveCandidate || IsNewer(version, bestCandidate.version)) {
            bestCandidate.found = true;
            bestCandidate.version = version;
            bestCandidate.assetUrl = chosenAssetUrl;
            bestCandidate.htmlUrl = htmlUrl;
            bestCandidate.tagName = tag;
            bestCandidate.error.clear();
            bestCandidate.rateLimited = result.rateLimited;
            bestCandidate.source = source;
            haveCandidate = true;
        }
    }

    if (haveCandidate) {
        return bestCandidate;
    }

    result.error = FormatLang(LanguageCategory::Option_UI, LanguageKey::Update_Message_NoReleaseMatchingFilters);
    } catch (std::exception& e) {
        LOG("Updater: JSON parse error from ", ReleaseSourceLabel(source), ": ", e.what());
        result.error = FormatLang(LanguageCategory::Option_UI, LanguageKey::Update_Message_ParseError, ReleaseSourceDisplayName(source), e.what());
        if (source == ReleaseSource::GitHub && std::string(e.what()).find("API rate limit exceeded") != std::string::npos) {
            result.rateLimited = true;
        }
    }
    return result;
}

static void CloseGameNow() {
    LOG("Updater: Close Game requested by user");
    auto windows = GetProcessTopLevelWindows();
    if (windows.empty()) {
        LOG("Updater: No top-level windows found; issuing ExitProcess fallback.");
        ExitProcess(0);
        return;
    }
    for (HWND hwnd : windows) {
        PostMessage(hwnd, WM_CLOSE, 0, 0);
        PostMessage(hwnd, WM_DESTROY, 0, 0);
    }
    LOG("Updater: Posted WM_CLOSE/WM_DESTROY to", static_cast<int>(windows.size()), "window(s)");
}

static void RestartGameNow() {
    LOG("Updater: Restart Game requested by user");
    wchar_t exePath[MAX_PATH]{};
    if (GetModuleFileNameW(nullptr, exePath, MAX_PATH) == 0) {
        LOG("Updater: GetModuleFileNameW failed, gle=", GetLastError());
        return;
    }
    LPCWSTR currentCmd = GetCommandLineW();
    std::wstring cmdLineCopy = currentCmd ? std::wstring(currentCmd) : std::wstring();
    LPWSTR mutableCmd = cmdLineCopy.empty() ? nullptr : cmdLineCopy.data();
    wchar_t workDir[MAX_PATH]{};
    lstrcpynW(workDir, exePath, MAX_PATH);
    for (int i = (int)wcslen(workDir) - 1; i >= 0; --i) {
        if (workDir[i] == L'\\' || workDir[i] == L'/') { workDir[i] = L'\0'; break; }
    }

    STARTUPINFOW si{}; si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    BOOL ok = CreateProcessW(
        exePath,
        mutableCmd,
        nullptr,
        nullptr,
        FALSE,
        0,
        nullptr,
        workDir[0] ? workDir : nullptr,
        &si,
        &pi);
    if (!ok) {
        LOG("Updater: CreateProcessW failed, gle=", GetLastError());
        return;
    } else {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
    ExitProcess(0);
}

void Updater::Init() {
    LOG("Updater: Init");
    s_state = std::make_unique<UpdateState>();
    ClearLeftoverFiles();

    auto pathOpt = GetSelfPath();
    if (!pathOpt) { LOG("Updater: GetSelfPath failed; skipping auto-check"); return; }
    s_state->installPath = *pathOpt;
    LOG("Updater: Install path: ", s_state->installPath);

    auto fileV = GetCurrentFileVersion(*pathOpt);
    if (fileV) {
        s_state->currentVersion = *fileV;
        auto v = *fileV; LOG("Updater: Current file version: ", v[0], ".", v[1], ".", v[2], ".", v[3]);
    } else {
        s_state->currentVersion = ParseVersionAny(SCT_VERSION_STRING);
        auto v = *s_state->currentVersion; LOG("Updater: Current display version: ", v[0], ".", v[1], ".", v[2], ".", v[3]);
    }

    auto& opts = Options::getOptionsStruct();
    bool enabled = true;
    bool allowPre = false;
    enabled = opts.updatesEnabled;
    allowPre = opts.includePrerelease;

    LOG("Updater: Auto-check ", (enabled ? "enabled" : "disabled"), ", includePrerelease=", (allowPre ? "true" : "false"), ", repo=", opts.updateRepo);
    if (enabled) {
        LOG("Updater: Queueing initial auto-check");
        CheckNow();
    }
}

void Updater::Shutdown() {
    if (s_state) {
        std::lock_guard<std::mutex> g(s_state->lock);
        for (auto &t : s_state->tasks) { if (t.joinable()) t.join(); }
        s_state.reset();
    }
}

void Updater::CheckNow(bool useBackupOnly) {
    if (!s_state) return;
    std::lock_guard<std::mutex> g(s_state->lock);
    LOG("Updater: CheckNow queued");
    s_state->tasks.emplace_back([useBackupOnly]() {
        auto& st = *s_state;
        auto& opts = Options::getOptionsStruct();
        bool allowPre = opts.includePrerelease;

        const std::string defaultSlug = "jake-greygoose/GW2-SCT";
        std::string primarySlug = opts.updateRepo.empty() ? defaultSlug : opts.updateRepo;
        std::string backupSlug = opts.backupUpdateRepo.empty() ? primarySlug : opts.backupUpdateRepo;

        ReleaseSource primarySource = useBackupOnly ? ReleaseSource::Forgejo : ReleaseSource::GitHub;
        std::string owner;
        std::string repo;
        if (useBackupOnly) {
            auto parsed = ParseRepoSlug(backupSlug);
            owner = parsed.first;
            repo = parsed.second;
            LOG("Updater: Fetching latest release (includePrerelease=", (allowPre ? "true" : "false"), ") from ", owner, "/", repo, " via Forgejo only");
        } else {
            auto parsed = ParseRepoSlug(primarySlug);
            owner = parsed.first;
            repo = parsed.second;
            LOG("Updater: Fetching latest release (includePrerelease=", (allowPre ? "true" : "false"), ") from ", owner, "/", repo);
        }

        auto fetchPrimary = FetchLatestRelease(owner, repo, allowPre, primarySource);
        auto now = std::chrono::system_clock::now();

        bool triedBackup = false;
        bool usedBackup = false;
        auto fetch = fetchPrimary;
        bool rateLimited = fetchPrimary.rateLimited;

        if (!useBackupOnly && !fetchPrimary.found) {
            triedBackup = true;
            auto parsedBackup = ParseRepoSlug(backupSlug);
            const auto& backupOwner = parsedBackup.first;
            const auto& backupRepo = parsedBackup.second;
            LOG("Updater: GitHub fetch failed (", (fetchPrimary.error.empty() ? "no details" : fetchPrimary.error.c_str()), ") -- attempting Forgejo mirror ", backupOwner, "/", backupRepo);
            auto fetchForgejo = FetchLatestRelease(backupOwner, backupRepo, allowPre, ReleaseSource::Forgejo);
            if (fetchForgejo.found) {
                fetch = fetchForgejo;
                usedBackup = true;
            } else {
                std::string combined = fetchPrimary.error;
                if (!fetchForgejo.error.empty()) {
                    if (!combined.empty()) combined += " | ";
                    combined += FormatLang(LanguageCategory::Option_UI, LanguageKey::Update_Message_SourceError,
                        ReleaseSourceDisplayName(fetchForgejo.source), fetchForgejo.error.c_str());
                }
                if (combined.empty()) {
                    combined = FormatLang(LanguageCategory::Option_UI, LanguageKey::Update_Message_NoSuitable);
                }
                fetch = fetchPrimary;
                fetch.error = combined;
                fetch.rateLimited = fetchPrimary.rateLimited || fetchForgejo.rateLimited;
            }
            rateLimited = fetchPrimary.rateLimited || fetchForgejo.rateLimited;
            if (usedBackup && fetchPrimary.rateLimited) {
                LOG("Updater: GitHub rate limited; Forgejo mirror succeeded.");
            }
            if (triedBackup && !usedBackup) {
                LOG("Updater: Forgejo mirror did not provide a newer release.");
            }
        }

        if (useBackupOnly) {
            rateLimited = fetch.rateLimited;
            if (!fetch.found && fetch.error.empty()) {
                fetch.error = langString(LanguageCategory::Option_UI, LanguageKey::Update_Message_NoSuitable);
            }
        }

        bool hadError = false;
        bool updateAvailable = false;
        std::string message;
        UpdateState::Version newVersion{0,0,0,0};
        std::string downloadUrl;
        std::string htmlUrl;
        std::string tag;

        if (!fetch.found) {
            hadError = true;
            if (fetch.error.empty()) {
                message = FormatLang(LanguageCategory::Option_UI, LanguageKey::Update_Message_NoSuitable);
            } else {
                message = fetch.error;
            }
            LOG("Updater: No suitable release found or fetch failed: ", message);
        } else if (!st.currentVersion.has_value()) {
            hadError = true;
            message = FormatLang(LanguageCategory::Option_UI, LanguageKey::Update_Message_CurrentUnknown);
            LOG("Updater: Current version not available; cannot compare to latest.");
        } else if (!IsNewer(fetch.version, *st.currentVersion)) {
            auto cv = *st.currentVersion;
            LOG("Updater: Latest not newer. current=", cv[0], ".", cv[1], ".", cv[2], ".", cv[3],
                " latest=", fetch.version[0], ".", fetch.version[1], ".", fetch.version[2], ".", fetch.version[3]);
            std::string label = fetch.tagName.empty() ? VersionToString(fetch.version) : fetch.tagName;
            std::string sourceNote = FormatLang(LanguageCategory::Option_UI, LanguageKey::Update_Message_SourceSuffix,
                ReleaseSourceDisplayName(fetch.source));
            std::string messageBody = FormatLang(LanguageCategory::Option_UI, LanguageKey::Update_Message_UpToDate, label.c_str());
            message = messageBody + sourceNote;
        } else {
            updateAvailable = true;
            newVersion = fetch.version;
            downloadUrl = fetch.assetUrl;
            htmlUrl = fetch.htmlUrl;
            tag = fetch.tagName;
            std::string label = tag.empty() ? VersionToString(newVersion) : tag;
            std::string sourceNote = FormatLang(LanguageCategory::Option_UI, LanguageKey::Update_Message_SourceSuffix,
                ReleaseSourceDisplayName(fetch.source));
            std::string messageBody = FormatLang(LanguageCategory::Option_UI, LanguageKey::Update_Message_Available, label.c_str());
            message = messageBody + sourceNote;
            LOG("Updater: Update available via ", ReleaseSourceLabel(fetch.source), ": ", label, ", asset=", downloadUrl.empty() ? "<none>" : downloadUrl.c_str());
        }

        {
            std::lock_guard<std::mutex> gg(st.lock);
            st.lastCheckTime = now;
            st.lastCheckMessage = std::move(message);
            st.lastCheckHadError = hadError;
            st.lastCheckRateLimited = rateLimited;
            st.lastCheckFoundUpdate = updateAvailable;
            if (updateAvailable) {
                st.newVersion = newVersion;
                st.downloadUrl = std::move(downloadUrl);
                st.releasePageUrl = std::move(htmlUrl);
                st.tagName = std::move(tag);
                st.status = UpdateState::Status::UpdateAvailable;
            } else if (!hadError && st.status != UpdateState::Status::UpdateInProgress && st.status != UpdateState::Status::UpdateSuccessful) {
                st.status = UpdateState::Status::Unknown;
                st.downloadUrl.clear();
                st.releasePageUrl.clear();
                st.tagName.clear();
            }
        }
    });
}

void Updater::DrawPopup() {
    if (!s_state) return;
    std::lock_guard<std::mutex> g(s_state->lock);
    auto st = s_state.get();
    if (st->status == UpdateState::Status::Unknown || st->status == UpdateState::Status::Dismissed)
        return;

    bool open = true;
    std::string windowLabel = std::string(langString(LanguageCategory::Option_UI, LanguageKey::Update_Popup_WindowTitle)) + "###GW2_SCT_UPDATE";
    if (ImGui::Begin(windowLabel.c_str(), &open, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize)) {
        auto cur = st->currentVersion.value_or(UpdateState::Version{0,0,0,0});
        auto nv = st->newVersion;
        ImGui::TextColored(ImVec4(1,0,0,1), "%s", langString(LanguageCategory::Option_UI, LanguageKey::Update_Popup_Header));
        ImGui::TextColored(ImVec4(1,0,0,1), langString(LanguageCategory::Option_UI, LanguageKey::Update_Popup_Current), cur[0], cur[1], cur[2], cur[3]);
        ImGui::TextColored(ImVec4(0,1,0,1), langString(LanguageCategory::Option_UI, LanguageKey::Update_Popup_Latest), nv[0], nv[1], nv[2], nv[3]);

        if (!st->releasePageUrl.empty()) {
            if (ImGui::Button(langString(LanguageCategory::Option_UI, LanguageKey::Update_Popup_Button_OpenRelease))) {
                std::string url = st->releasePageUrl;
                std::thread([url]() { ShellExecuteA(nullptr, nullptr, url.c_str(), nullptr, nullptr, SW_SHOW); }).detach();
            }
        }

        switch (st->status) {
        case UpdateState::Status::UpdateAvailable: {
            bool canAuto = false;
            if (!st->downloadUrl.empty()) {
                std::string lower = st->downloadUrl; for (auto &c : lower) c = (char)tolower(c);
                if (lower.size() >= 4 && lower.rfind(".dll") == lower.size() - 4) canAuto = true;
            }
            if (canAuto) {
                ImGui::SameLine();
                if (ImGui::Button(langString(LanguageCategory::Option_UI, LanguageKey::Update_Popup_Button_UpdateAutomatically))) {
                    auto installPath = st->installPath;
                    auto url = st->downloadUrl;
                    st->status = UpdateState::Status::UpdateInProgress;
                    s_state->tasks.emplace_back([installPath, url]() {
                        std::filesystem::path tmp = installPath + ".tmp";
                        std::filesystem::path old = installPath + ".old";
                        if (!HttpDownloadToFile(url, tmp)) {
                            std::lock_guard<std::mutex> g2(s_state->lock);
                            s_state->status = UpdateState::Status::UpdateError;
                            LOG("Updater: Download failed");
                            return;
                        }
                        if (rename(installPath.c_str(), old.string().c_str()) != 0) {
                            LOG("Updater: Rename to .old failed: errno=", errno, " gle=", GetLastError());
                            std::lock_guard<std::mutex> g2(s_state->lock);
                            s_state->status = UpdateState::Status::UpdateError;
                            return;
                        }
                        if (rename(tmp.string().c_str(), installPath.c_str()) != 0) {
                            LOG("Updater: Rename tmp->current failed: errno=", errno, " gle=", GetLastError());
                            std::lock_guard<std::mutex> g2(s_state->lock);
                            s_state->status = UpdateState::Status::UpdateError;
                            return;
                        }
                        LOG("Updater: Update installed. Restart GW2 for changes to take effect.");
                        std::lock_guard<std::mutex> g2(s_state->lock);
                        s_state->status = UpdateState::Status::UpdateSuccessful;
                    });
                }
            }
            break;
        }
        case UpdateState::Status::UpdateInProgress:
            ImGui::TextUnformatted(langString(LanguageCategory::Option_UI, LanguageKey::Update_Popup_Status_Downloading));
            break;
        case UpdateState::Status::UpdateSuccessful:
            ImGui::TextColored(ImVec4(0,1,0,1), "%s", langString(LanguageCategory::Option_UI, LanguageKey::Update_Popup_Status_Complete));
            if (ImGui::Button(langString(LanguageCategory::Option_UI, LanguageKey::Update_Popup_Button_Restart))) {
                RestartGameNow();
            }
            ImGui::SameLine();
            if (ImGui::Button(langString(LanguageCategory::Option_UI, LanguageKey::Update_Popup_Button_Close))) {
                CloseGameNow();
            }
            break;
        case UpdateState::Status::UpdateError:
            ImGui::TextColored(ImVec4(1,0,0,1), "%s", langString(LanguageCategory::Option_UI, LanguageKey::Update_Popup_Status_Error));
            break;
        default: break;
        }
        ImGui::End();
    }
    if (!open) {
        s_state->status = UpdateState::Status::Dismissed;
    }
}

void Updater::DrawSettingsInline() {
    auto& opts = Options::getOptionsStruct();
    bool changed = false;

    int currentMode = (!opts.updatesEnabled) ? 0 : (opts.includePrerelease ? 2 : 1);
    int newMode = currentMode;

    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::TextUnformatted(langString(LanguageCategory::Option_UI, LanguageKey::Update_Menu_Header));
    if (ImGui::RadioButton(langString(LanguageCategory::Option_UI, LanguageKey::Update_Mode_Off), newMode == 0)) newMode = 0;
    ImGui::SameLine();
    if (ImGui::RadioButton(langString(LanguageCategory::Option_UI, LanguageKey::Update_Mode_OnStable), newMode == 1)) newMode = 1;
    ImGui::SameLine();
    if (ImGui::RadioButton(langString(LanguageCategory::Option_UI, LanguageKey::Update_Mode_OnPrerelease), newMode == 2)) newMode = 2;

    if (newMode != currentMode) {
        switch (newMode) {
        case 0:
            opts.updatesEnabled = false;
            opts.includePrerelease = false;
            break;
        case 1:
            opts.updatesEnabled = true;
            opts.includePrerelease = false;
            break;
        case 2:
        default:
            opts.updatesEnabled = true;
            opts.includePrerelease = true;
            break;
        }
        changed = true;
    }

    if (ImGui::Button(langString(LanguageCategory::Option_UI, LanguageKey::Update_Button_CheckNow))) {
        if (!s_state) Init();
        CheckNow();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(langString(LanguageCategory::Option_UI, LanguageKey::Update_Check_Tooltip));
    }

    if (changed) Options::requestSave();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    UpdateState::Version curVersion{0,0,0,0};
    UpdateState::Version latestVersion{0,0,0,0};
    bool hasCurrent = false;
    bool hasLatest = false;
    std::optional<std::chrono::system_clock::time_point> lastCheckTime;
    std::string lastCheckMessage;
    bool lastCheckHadError = false;
    bool lastCheckRateLimited = false;
    bool lastCheckFoundUpdate = false;

    if (s_state) {
        std::lock_guard<std::mutex> guard(s_state->lock);
        if (s_state->currentVersion.has_value()) {
            curVersion = *s_state->currentVersion;
            hasCurrent = true;
        }
        if (s_state->status == UpdateState::Status::UpdateAvailable) {
            latestVersion = s_state->newVersion;
            hasLatest = true;
        }
        if (s_state->lastCheckTime.has_value()) {
            lastCheckTime = s_state->lastCheckTime;
            lastCheckMessage = s_state->lastCheckMessage;
            lastCheckHadError = s_state->lastCheckHadError;
            lastCheckRateLimited = s_state->lastCheckRateLimited;
            lastCheckFoundUpdate = s_state->lastCheckFoundUpdate;
        }
    }

    if (hasCurrent) {
        ImGui::Text(langString(LanguageCategory::Option_UI, LanguageKey::Update_Current_Version), curVersion[0], curVersion[1], curVersion[2], curVersion[3]);
        if (hasLatest) {
            ImGui::Text(langString(LanguageCategory::Option_UI, LanguageKey::Update_Latest_Version), latestVersion[0], latestVersion[1], latestVersion[2], latestVersion[3]);
        }
    }

    if (lastCheckTime.has_value()) {
        ImGui::Spacing();
        auto formatted = FormatTimestamp(*lastCheckTime);
        ImGui::Text(langString(LanguageCategory::Option_UI, LanguageKey::Update_Last_Check), formatted.c_str());
        if (!lastCheckMessage.empty()) {
            ImVec4 color(0.7f, 0.7f, 0.7f, 1.0f);
            if (lastCheckHadError) {
                color = lastCheckRateLimited ? ImVec4(1.0f, 0.7f, 0.0f, 1.0f) : ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
            } else if (lastCheckFoundUpdate) {
                color = ImVec4(0.3f, 0.85f, 0.3f, 1.0f);
            }
            ImGui::TextColored(color, "%s", lastCheckMessage.c_str());
        }
    }
}

void Updater::ClearLeftoverFiles() {
    auto pathOpt = GetSelfPath();
    if (!pathOpt) return;
    std::error_code ec;
    auto tmp = *pathOpt + ".tmp";
    auto old = *pathOpt + ".old";
    if (std::filesystem::exists(tmp, ec)) {
        std::filesystem::remove(tmp, ec);
        LOG("Updater: Cleared ", tmp, ", ec=", ec.value());
    }
    ec.clear();
    if (std::filesystem::exists(old, ec)) {
        std::filesystem::remove(old, ec);
        LOG("Updater: Cleared ", old, ", ec=", ec.value());
    }
}

std::optional<std::string> Updater::GetSelfPath() {
    char path[MAX_PATH]{};
    HMODULE hm = nullptr;
    if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCSTR>(&GetSelfPath), &hm)) {
        return std::nullopt;
    }
    if (GetModuleFileNameA(hm, path, MAX_PATH) == 0) return std::nullopt;
    return std::string(path);
}

} // namespace GW2_SCT

