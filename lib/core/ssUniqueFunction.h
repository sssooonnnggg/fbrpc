#pragma once

#include <type_traits>
#include <memory>

namespace fbrpc
{

	namespace detail
	{
		template <class Ret, class ... Args>
		class sCallable
		{
		public:
			virtual ~sCallable() = default;
			virtual Ret invoke(Args&& ... args) = 0;
		};

		template <class F, class Ret, class ... Args>
		class sCallbaleImpl : public sCallable<Ret, Args...>
		{
		public:
			sCallbaleImpl(F&& f)
				: m_function(std::forward<F>(f))
			{}

			Ret invoke(Args&& ... args) override
			{
				return m_function(std::forward<Args>(args)...);
			}

		private:
			F m_function;
		};
	}

	template <class T>
	class sUniqueFunction;

	template <class Ret, class ... Args>
	class sUniqueFunction<Ret(Args...)>
	{
	public:
		template <class F, typename = std::enable_if_t<std::is_same_v<Ret, std::invoke_result_t<F, Args...>>>>
		sUniqueFunction(F&& f)
		{
			m_callable = std::make_unique<detail::sCallbaleImpl<std::decay_t<F>, Ret, Args...>>(std::forward<F>(f));
		}

		sUniqueFunction() = default;

		Ret operator()(Args&& ... args)
		{
			return m_callable->invoke(std::forward<Args>(args)...);
		}

		operator bool() const
		{
 			return m_callable.operator bool();
		}

	private:
		std::unique_ptr<detail::sCallable<Ret, Args...>> m_callable;
	};
}