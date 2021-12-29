#pragma once

#include <functional>

namespace flatbuffers
{
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

		virtual bool start(flatbuffers::ServiceDef* service) = 0;
		virtual bool finish(std::vector<flatbuffers::ServiceDef*> services) { return true; }
	private:
		sWriteFileDelegate m_writter;
	};
}