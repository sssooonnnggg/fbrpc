#pragma once

#include <string_view>

namespace fbrpc
{
	inline std::size_t getHash(std::string_view name)
	{
		return std::hash<std::string_view>{}(name);
	}
}