#pragma once

#include <cstdio>
#include <memory>
#include <string>
#include <string_view>

namespace fbrpc
{
	class sLoggerSink
	{
	public:
		virtual ~sLoggerSink() = default;

		virtual void info(std::string_view message)
		{
			printf("[fbrpc] [info] %s \r\n", message.data());
		}

		virtual void error(std::string_view message)
		{
			printf("[fbrpc] [error] %s \r\n", message.data());
		}
	};

	class sLogger
	{
	public:
		static sLogger& instance()
		{
			static sLogger instance;
			return instance;
		}

		template <class ... Args>
		void info(Args&& ... args) { m_sink->info((... + (toString(std::forward<Args>(args)) + " "))); }

		template <class ... Args>
		void error(Args&& ... args) { m_sink->error((... + (toString(std::forward<Args>(args)) + " "))); }

		void setSink(std::unique_ptr<sLoggerSink> sink) { m_sink = std::move(sink); }

	private:
		sLogger() : m_sink(std::make_unique<sLoggerSink>()) {}

		std::string toString(const char* msg) { return msg; }
		std::string toString(int msg) { return std::to_string(msg); }
		std::string toString(std::string msg) { return msg; }
		std::string toString(std::string_view msg) { return msg.data(); }
	private:
		std::unique_ptr<sLoggerSink> m_sink;
	};

	static sLogger& logger() { return sLogger::instance(); }
}