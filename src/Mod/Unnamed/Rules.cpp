#include "Mod/Unnamed/Rules.h"

#include <cstdio>
#include <set>
#include <string>

namespace pvzu::unnamed {

namespace {
nlohmann::json gRules = nlohmann::json::object();
std::set<std::string, std::less<>> gWarned;

const nlohmann::json* Find(std::string_view key) {
	auto it = gRules.find(std::string(key));
	if (it == gRules.end() || !it->is_number()) {
		if (gWarned.insert(std::string(key)).second)
			std::fprintf(stderr, "PVZU: rule '%.*s' missing, using vanilla\n", static_cast<int>(key.size()), key.data());
		return nullptr;
	}
	return &*it;
}
}

void SetRules(const nlohmann::json& rules) {
	gRules = rules.is_object() ? rules : nlohmann::json::object();
	gWarned.clear();
}

int RuleInt(std::string_view key, int vanillaDefault) {
	const auto* v = Find(key);
	return v ? v->get<int>() : vanillaDefault;
}

float RuleFloat(std::string_view key, float vanillaDefault) {
	const auto* v = Find(key);
	return v ? v->get<float>() : vanillaDefault;
}

const nlohmann::json* RuleArray(std::string_view key) {
	auto it = gRules.find(std::string(key));
	if (it == gRules.end() || !it->is_array()) {
		if (gWarned.insert(std::string(key)).second)
			std::fprintf(stderr, "PVZU: rule '%.*s' missing or not a list, using vanilla\n", static_cast<int>(key.size()), key.data());
		return nullptr;
	}
	return &*it;
}

bool HasRule(std::string_view key) { return gRules.contains(std::string(key)); }

} // namespace pvzu::unnamed
