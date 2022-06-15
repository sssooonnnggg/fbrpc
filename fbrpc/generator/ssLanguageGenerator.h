#pragma once

#include <functional>

namespace flatbuffers
{
	class Parser;
	struct ServiceDef;
}

namespace fbrpc
{
	class sLanguageGenerator
	{
	public:
		using sWriteFileDelegate = std::function<bool(std::string, std::string)>;
		sLanguageGenerator(sWriteFileDelegate writter)
			: m_writter(std::move(writter))
		{}
		virtual ~sLanguageGenerator() = default;
		sWriteFileDelegate writter() { return m_writter; }

		virtual bool generateDummyFile(std::string fbsFileName) { return true; };
		virtual bool start(flatbuffers::ServiceDef* service) = 0;

		struct sContext
		{
			std::vector<flatbuffers::Parser*> parsers;
			std::vector<flatbuffers::ServiceDef*> services;
		};
		virtual bool finish(sContext context) { return true; }
	private:
		sWriteFileDelegate m_writter;
	};
}