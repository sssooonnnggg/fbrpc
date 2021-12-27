#pragma once

#include "core/ssUniqueFunction.h"
#include "ssFlatBufferHelper.h"

namespace fbrpc
{	
	template <class T, typename = std::enable_if_t<std::is_base_of_v<flatbuffers::Table, T>>>
	class sPromise : public sFlatBufferHelper
	{
	public:
		void finish(flatbuffers::Offset<T> value)
		{
			builder().Finish(value);
			triggerBinding();
			builder().Clear();
		}

		using sBinding = sUniqueFunction<void()>;
		void bind(sBinding&& binding) { m_binding = std::move(binding); }

	private:
		friend class sService;
		sPromise() = default;

		void triggerBinding() { if (m_binding) m_binding(); }

		flatbuffers::Offset<T> m_offset;
		sBinding m_binding;
	};
}