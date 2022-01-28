#pragma once

#include <type_traits>

#include "fbrpc/core/ssUniqueFunction.h"
#include "fbrpc/core/ssFlatBufferHelper.h"

namespace fbrpc
{	
	template <class T, class Enable = void>
	struct CppToFlatBuffer
	{
		static auto convert(flatbuffers::FlatBufferBuilder& fbb, T obj)
		{
			return obj;
		}
	};

	template <class T>
	using CppToFlatBufferResult =
		std::invoke_result_t<decltype(CppToFlatBuffer<std::decay_t<T>>::convert), flatbuffers::FlatBufferBuilder&, std::decay_t<T>>;

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
		std::enable_if_t<std::is_invocable_v<F, flatbuffers::FlatBufferBuilder&, CppToFlatBufferResult<Args>...>>
		resolve(F&& fn, Args&& ... args)
		{
			resolve(
				fn(builder(), CppToFlatBuffer<std::decay_t<Args>>::convert(builder(), std::forward<Args>(args))...));
		}

		void emit(flatbuffers::Offset<T> value = 0)
		{
			resolve(value);
		}

		template<class F, class ... Args>
		std::enable_if_t<std::is_invocable_v<F, flatbuffers::FlatBufferBuilder&, CppToFlatBufferResult<Args>...>>
			emit(F&& fn, Args&& ... args)
		{
			resolve(std::forward<F>(fn), std::forward<Args>(args)...);
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