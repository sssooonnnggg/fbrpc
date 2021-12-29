#pragma once

#include "ssLanguageGenerator.h"

namespace fbrpc
{
	class sCppGenerator : public sLanguageGenerator
	{
	public:
		sCppGenerator(sWriteFileDelegate writter) : sLanguageGenerator(std::move(writter)) {}

		bool start(flatbuffers::ServiceDef* service) override;
	private:
		bool generateServiceFile(flatbuffers::ServiceDef* service);
		bool generateStubFile(flatbuffers::ServiceDef* service);
	};
}