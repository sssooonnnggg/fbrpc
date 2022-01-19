#pragma once

#include "fbrpc/core/ssUniqueFunction.h"
#include "fbrpc/core/ssFlatBufferHelper.h"

namespace fbrpc
{	
	template <class T, typename = std::enable_if_t<std::is_base_of_v<flatbuffers::Table, T>>>
	class sPromise : public sFlatBufferHelper
	{
	public:
		void resolve(flatbuffers::Offset<T> value = 0)
		{
			// append empty table if no offset available
			if (value.IsNull())
				value = builder().EndTable(builder().StartTable());

			builder().Finish(value);
			triggerBinding();
			builder().Clear();
		}

		template<class F, class ... Args>
		std::enable_if_t<std::is_invocable_v<F, flatbuffers::FlatBufferBuilder&, Args...>>
		resolve(F&& fn, Args&& ... args)
		{
			resolve(fn(builder(), std::forward<Args>(args)...));
		}

		void emit(flatbuffers::Offset<T> value)
		{
			resolve(value);
		}

		template<class F, class ... Args>
		std::enable_if_t<std::is_invocable_v<F, flatbuffers::FlatBufferBuilder&, Args...>>
			emit(F&& fn, Args&& ... args)
		{
			resolve(fn(builder(), std::forward<Args>(args)...));
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

	template <class T>
	using sEventEmitter = sPromise<T>;
}