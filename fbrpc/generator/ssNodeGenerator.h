#pragma once

#include "ssLanguageGenerator.h"

namespace fbrpc
{
	class sTSPrinter;

	class sNodeGenerator : public sLanguageGenerator
	{
	public:
		sNodeGenerator(sWriteFileDelegate writter) : sLanguageGenerator(std::move(writter)) {}
		bool start(flatbuffers::ServiceDef* service) override;
		bool finish(sContext context) override;
	private:
		bool generateNodeBindingFile(flatbuffers::ServiceDef* service);
		bool finishNodeBindingFile(const std::vector<flatbuffers::ServiceDef*>& services);
	};
}