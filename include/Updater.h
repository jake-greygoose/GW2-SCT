#pragma once

#include <string>
#include <optional>
#include <array>
#include <memory>

namespace GW2_SCT {

class Updater {
public:
    static void Init();
    static void Shutdown();
    static void DrawPopup();
    static void DrawSettingsInline();
    static void CheckNow(bool useBackupOnly = false);

private:
    struct UpdateState;
    static std::unique_ptr<UpdateState> s_state;
    static void ClearLeftoverFiles();
    static std::optional<std::string> GetSelfPath();
};

} // namespace GW2_SCT

