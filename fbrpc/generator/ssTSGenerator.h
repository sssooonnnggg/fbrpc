#pragma once

#include <functional>
#include <string>

#include "ssLanguageGenerator.h"

namespace fbrpc
{
	class sTSPrinter;

	class sTSGenerator : public sLanguageGenerator
	{
	public:
		sTSGenerator(sWriteFileDelegate writter) : sLanguageGenerator(std::move(writter)) {}
		bool start(flatbuffers::ServiceDef* service) override;
		bool finish(std::vector<flatbuffers::ServiceDef*> services) override;
	private:
		bool generateTSDeclareFile(flatbuffers::ServiceDef* service);
		bool generateTSWrapperFile(flatbuffers::ServiceDef* service);
		bool finishTSDeclareFile(const std::vector<flatbuffers::ServiceDef*>& services);
		bool finishTSWrapperFile(const std::vector<flatbuffers::ServiceDef*>& services);

		void importDependentTypes(sTSPrinter& printer, flatbuffers::ServiceDef* service);
		void exportDependentTypes(sTSPrinter& printer, flatbuffers::ServiceDef* service);

		void foreachDependentType(
			sTSPrinter& printer,
			flatbuffers::ServiceDef* service,
			std::function<void(const std::string types, const std::string& from)> processor
		);
	};
}