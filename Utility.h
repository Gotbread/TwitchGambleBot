#pragma once

#include <string>
#include <algorithm>
#include <windows.h>

enum class ComparePolicy { CaseSensitive, CaseInsensitive };

template<ComparePolicy policy = ComparePolicy::CaseSensitive, class Container, typename std::enable_if<policy == ComparePolicy::CaseSensitive, bool>::type = true>
bool StartsWith(const Container &input, const Container &compare)
{
	return std::mismatch(input.begin(), input.end(), compare.begin(), compare.end()).second == compare.end();
}
template<ComparePolicy policy, class Container, typename std::enable_if<policy == ComparePolicy::CaseInsensitive, bool>::type = true>
bool StartsWith(const Container &input, const Container &compare)
{
	auto pred = [](auto &a, auto &b) { return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));  };
	return std::mismatch(input.begin(), input.end(), compare.begin(), compare.end(), pred).second == compare.end();
}

std::string GetWindowTextString(HWND hWnd);
bool StringRangeOverlaps(size_t pos1, size_t pos2, size_t length1, size_t length2);
std::string ReplaceGuarded(std::string input, const std::string &replacee, const std::string &replacer);
void AddLogMessage(HWND hWnd, const std::string &str, unsigned max_char);