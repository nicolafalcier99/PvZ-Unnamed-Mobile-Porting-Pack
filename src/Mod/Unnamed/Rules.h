// PVZU: Unnamed rule constants (chances, HP, speeds) from the pack's data/rules.json.
// Every lookup names the vanilla value as fallback, so a missing key keeps vanilla behaviour.
#pragma once

#include "nlohmann/json.hpp"

#include <string_view>

namespace pvzu::unnamed {

void SetRules(const nlohmann::json& rules);  // object of "key": number
int RuleInt(std::string_view key, int vanillaDefault);
float RuleFloat(std::string_view key, float vanillaDefault);
// A non-number rule (e.g. a list); nullptr (and one warning) when missing or not an array.
const nlohmann::json* RuleArray(std::string_view key);
// True when the key is present (any type); never warns. For rules that exist only for some modes/levels.
bool HasRule(std::string_view key);

} // namespace pvzu::unnamed
