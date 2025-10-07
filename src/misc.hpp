#pragma once

#include <string>
#include <type_traits>
#include <charconv>
#include <cstring>
#include <system_error>

using std::string;


template<typename T>
bool ConvertStrToData(const string &src, T &dest)
{
	if constexpr (std::is_same_v<T, bool>)
	{
		// Handle boolean conversion
		if (src == "true" || src == "1")
		{
			dest = true;
			return true;
		}
		else if (src == "false" || src == "0")
		{
			dest = false;
			return true;
		}
		return false;
	}
	else
	{
		auto result = std::from_chars(src.data(), src.data() + src.size(), dest);
		return result.ec == std::errc() && result.ptr == src.data() + src.size();
	}
}

template<typename T>
bool ConvertStrToData(const char *src, T &dest)
{
	if (src == nullptr)
		return false;

	if constexpr (std::is_same_v<T, bool>)
	{
		// Handle boolean conversion
		if (strcmp(src, "true") == 0 || strcmp(src, "1") == 0)
		{
			dest = true;
			return true;
		}
		else if (strcmp(src, "false") == 0 || strcmp(src, "0") == 0)
		{
			dest = false;
			return true;
		}
		return false;
	}
	else
	{
		auto result = std::from_chars(src, src + strlen(src), dest);
		return result.ec == std::errc() && result.ptr == src + strlen(src);
	}
}


template<typename T, unsigned int B = 10U>
bool ConvertDataToStr(T src, string &dest)
{
	if constexpr (std::is_same_v<T, bool>)
	{
		dest += src ? "true" : "false";
		return true;
	}
	else
	{
		char buffer[64];
		auto result = std::to_chars(buffer, buffer + sizeof(buffer), src, B);
		if (result.ec == std::errc())
		{
			dest.append(buffer, result.ptr - buffer);
			return true;
		}
		return false;
	}
}
