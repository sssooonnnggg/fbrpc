#pragma once

#include <functional>
#include <memory>

namespace fbrpc
{
	class sTimer
	{
	public:
		static std::unique_ptr<sTimer> create();
		virtual ~sTimer() = default;
		virtual void once(int timeoutMs, std::function<void()> onTimeout) = 0;
		virtual void repeat(int timeoutMs, std::function<void()> onTimeout) = 0;
		virtual void stop() = 0;
	protected:
		sTimer() = default;
	};
}