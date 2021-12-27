#pragma once

#include "flatbuffers/flatbuffers.h"

namespace fbrpc
{
	class sFlatBufferHelper
	{
	public:
		virtual ~sFlatBufferHelper() = default;

		flatbuffers::FlatBufferBuilder& builder() { return m_builder; }

		template <class T, class ... Args>
		static constexpr auto TypeMatchAny = std::disjunction_v<std::is_same<std::decay_t<T>, Args>...>;

		template <class T>
		flatbuffers::Offset<flatbuffers::String> createString(T&& str)
		{
			static_assert(TypeMatchAny<T, char*, const char*, std::string>, "invalid string!");
			return builder().CreateString(std::forward<T>(str));
		}

	private:
		flatbuffers::FlatBufferBuilder m_builder;
	};
}