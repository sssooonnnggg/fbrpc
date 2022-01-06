#pragma once

#include <string>
#include "flatbuffers/idl.h"

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

	inline bool isEvent(flatbuffers::StructDef* table)
	{
		auto&& attributes = table->attributes.dict;
		return attributes.find("event") != attributes.end();
	}
}