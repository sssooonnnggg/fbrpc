#pragma once

#include "ssLanguageGenerator.h"

namespace fbrpc
{
	class sNodeGenerator : public sLanguageGenerator
	{
	public:
		sNodeGenerator(sWriteFileDelegate writter) : sLanguageGenerator(std::move(writter)) {}
		bool start(flatbuffers::ServiceDef* service) override;
		bool finish(std::vector<flatbuffers::ServiceDef*> services) override;
	private:
		bool generateNodeBindingFile(flatbuffers::ServiceDef* service);
		bool generateTypeScriptFile(flatbuffers::ServiceDef* service);
		bool generateFlatBufferBindingFile(const std::vector<flatbuffers::ServiceDef*>& services);
	};
}