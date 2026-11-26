// PVZU: storage for our own command-line options ("-pvzu-*"), so engine parsing needs one hook only.
#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace pvzu {

bool IsPvzuParam(std::string_view name);                   // starts with "-pvzu-"
void SetCmdOption(std::string_view name, std::string_view value);
void ClearCmdOptions();
std::optional<std::string> CmdOption(std::string_view name);
bool CmdFlag(std::string_view name);                       // present at all
int CmdOptionInt(std::string_view name, int fallback);     // fallback if missing or not a number
// True for scripted/automated runs (screenshot, input, quit, level, dump, autoplay, -pvzu-hidden) or when the
// env value (PVZU_HIDDEN) is "1": such runs open an invisible, windowed window on desktop.
bool AutomatedRun(const char* hiddenEnv);

} // namespace pvzu
