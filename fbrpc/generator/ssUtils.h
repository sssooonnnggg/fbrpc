#pragma once

#include <string>
#include <string_view>
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

	inline std::string toNameSpacePath(flatbuffers::Namespace *ns)
	{
		std::string result;
		if (ns)
		{
			const auto &depc_comps = ns->components;
			for (auto it = depc_comps.begin(); it != depc_comps.end(); it++)
			{
				result += result.empty() ? toDasherizedCase(*it) : '/' + toDasherizedCase(*it);
			}
		}
		return result;
	}
	// Convert an underscore_based_identifier in to camelCase.
	// Also uppercases the first character if first is true.
	inline std::string toCamel(const std::string &in, bool first)
	{
		std::string s;
		for (size_t i = 0; i < in.length(); i++)
		{
			if (!i && first)
				s += std::toupper(in[0]);
			else if (in[i] == '_' && i + 1 < in.length())
				s += std::toupper(in[++i]);
			else
				s += in[i];
		}
		return s;
	}

	struct sTag
	{
		static constexpr std::string_view Event = "event";
		static constexpr std::string_view ComponentPropertySetter = "component_property_setter";
	};

	inline bool hasTag(flatbuffers::Definition* def, std::string_view tag)
	{
		return def->attributes.dict.find(std::string(tag)) != def->attributes.dict.end();
	}

	inline bool isEvent(flatbuffers::Definition* def)
	{
		return hasTag(def, sTag::Event);
	}
}