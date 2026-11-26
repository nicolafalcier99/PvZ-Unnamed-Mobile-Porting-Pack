#include "Mod/CmdLine.h"

#include <charconv>
#include <map>

namespace pvzu {

namespace {
std::map<std::string, std::string, std::less<>>& Options() {
	static std::map<std::string, std::string, std::less<>> options;
	return options;
}
}

bool IsPvzuParam(std::string_view name) { return name.starts_with("-pvzu-"); }

void SetCmdOption(std::string_view name, std::string_view value) { Options()[std::string(name)] = std::string(value); }

void ClearCmdOptions() { Options().clear(); }

std::optional<std::string> CmdOption(std::string_view name) {
	auto it = Options().find(name);
	if (it == Options().end())
		return std::nullopt;
	return it->second;
}

bool CmdFlag(std::string_view name) { return Options().find(name) != Options().end(); }

int CmdOptionInt(std::string_view name, int fallback) {
	auto v = CmdOption(name);
	if (!v || v->empty())
		return fallback;
	int out = 0;
	auto [ptr, ec] = std::from_chars(v->data(), v->data() + v->size(), out);
	if (ec != std::errc() || ptr != v->data() + v->size())
		return fallback;
	return out;
}

bool AutomatedRun(const char* hiddenEnv) {
	if (hiddenEnv && std::string_view(hiddenEnv) == "1")
		return true;
	for (const char* f : {"-pvzu-shot", "-pvzu-input", "-pvzu-quit-frame", "-pvzu-level", "-pvzu-quit-after",
	                      "-pvzu-quit-after-waves", "-pvzu-dump-tables", "-pvzu-autoplay", "-pvzu-hidden", "-pvzu-dev-set-level"})
		if (CmdFlag(f))
			return true;
	return false;
}

} // namespace pvzu
