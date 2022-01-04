#pragma once

#include <string>

namespace fbrpc::generatorUtils
{
	inline std::string toDasherizedCase(std::string camel)
	{
		std::string result;
		for (auto c : camel)
		{
			if (std::isupper(c))
			{
				if (!result.empty())
					result += "-";
				result += std::tolower(c);
			}
			else
			{
				result += c;
			}
		}
		return result;
	};

	inline bool isEvent(const std::string& responseName)
	{
		return responseName.find("Event") != std::string::npos;
	}
}