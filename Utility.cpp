#include "Utility.h"

std::string GetWindowTextString(HWND hWnd)
{
	std::string ret;
	auto len = GetWindowTextLength(hWnd);
	ret.resize(len + 1);
	GetWindowText(hWnd, &ret[0], len + 1);
	ret.resize(len);
	return ret;
}
bool StringRangeOverlaps(size_t pos1, size_t pos2, size_t length1, size_t length2)
{
	if (pos1 == std::string::npos || pos2 == std::string::npos)
		return false;

	if (pos2 >= pos1 && pos2 < pos1 + length1)
		return true;
	if (pos1 >= pos2 && pos1 < pos2 + length2)
		return true;

	return false;
}
std::string ReplaceGuarded(std::string input, const std::string &replacee, const std::string &replacer)
{
	size_t pos = 0;
	while (pos < input.size())
	{
		auto p1 = input.find(replacee, pos);
		auto p2 = input.find(replacer, pos);

		if (p1 == std::string::npos)
			break;

		if (!StringRangeOverlaps(p1, p2, replacee.length(), replacer.length()))
		{
			std::string::iterator where = input.begin() + p1;
			input.replace(where, where + replacee.length(), replacer.begin(), replacer.end());
			pos = p1 + replacer.length();
		}
		else
		{
			pos = p1 + 1;
		}
	}
	return input;
}
//struct tester
//{
//	tester()
//	{
//		auto test1 = ReplaceGuarded("melonenkonvi", "konvi", "nuss");
//		auto test2 = ReplaceGuarded("melonen\nkonvi", "\n", "\r\n");
//		auto test3 = ReplaceGuarded("melonen\nkonvi", "o", "oooo");
//		__asm int 3
//	}
//} tester;
void AddLogMessage(HWND hWnd, const std::string &str, unsigned max_char)
{
	std::string new_str;

	std::string windows_newline = "\r\n";

	std::string adjusted_string = ReplaceGuarded(str, "\n", windows_newline);

	unsigned current_length = GetWindowTextLength(hWnd);
	unsigned new_length = adjusted_string.length();
	if (current_length + 2 + new_length <= max_char) // everything fits
	{
		auto conditional_newline = current_length ? windows_newline : std::string();
		new_str = GetWindowTextString(hWnd) + conditional_newline + adjusted_string;
	}
	else if (new_length + 2 < max_char) // somethings fits
	{
		unsigned remaining_length = max_char - new_length - 2;
		auto conditional_newline = current_length != remaining_length ? windows_newline : std::string();
		new_str = GetWindowTextString(hWnd).substr(current_length - remaining_length) + conditional_newline + adjusted_string;
	}
	else if (new_length <= max_char) // fits without newline
	{
		new_str = adjusted_string;
	}
	else // only part of the new fits
	{
		new_str = adjusted_string.substr(new_length - max_char);
	}
	// trimm first
	while (StartsWith(new_str, windows_newline))
		new_str.replace(new_str.begin(), new_str.begin() + windows_newline.length(), {});

	SetWindowText(hWnd, new_str.c_str());
}