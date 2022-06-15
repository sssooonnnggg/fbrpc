#pragma once

#include <functional>
#include <string>
#include <unordered_set>

#include "flatbuffers/idl.h"
#include "fbrpc/generator/ssLanguageGenerator.h"

namespace fbrpc
{
	class sTSPrinter;

	class sTSGenerator : public sLanguageGenerator
	{
	public:
		sTSGenerator(sWriteFileDelegate writter) : sLanguageGenerator(std::move(writter)) {}
		bool start(flatbuffers::ServiceDef* service) override;
		bool finish(sContext context) override;

	private:
		bool generateTSDeclareFile(flatbuffers::ServiceDef* service);
		bool generateTSWrapperFile(flatbuffers::ServiceDef* service);

		void importServiceDependentTypes(sTSPrinter& printer, flatbuffers::ServiceDef* service);
		
		bool finishTSDeclareFile(const std::vector<flatbuffers::ServiceDef*>& services);
		bool finishTSWrapperFile(const sContext& context);

		bool generateDefaultCall(sTSPrinter& printer, flatbuffers::ServiceDef* service, flatbuffers::RPCCall* call);
	};
}