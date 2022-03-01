#pragma once

#include <functional>
#include <string>
#include <unordered_set>

#include "ssLanguageGenerator.h"

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

		bool generateFromPlainFunction(sTSPrinter& printer, const sContext& context);

		void importDependentTypes(sTSPrinter& printer, const sContext& context);
		void exportDependentTypes(sTSPrinter& printer, const sContext& context);

		void importDependentEnums(sTSPrinter& printer, const sContext& context);
		void exportDependentEnums(sTSPrinter& printer, const sContext& context);

		bool generateEventCall(sTSPrinter& printer, flatbuffers::ServiceDef* service, flatbuffers::RPCCall* call);
		bool generateSetComponentPropertyCall(sTSPrinter& printer, flatbuffers::ServiceDef* service, flatbuffers::RPCCall* call, std::size_t importIndex);
		bool generateDefaultCall(sTSPrinter& printer, flatbuffers::ServiceDef* service, flatbuffers::RPCCall* call);

		struct sComponentContext
		{
			std::string ns;
			std::map<std::string, std::vector<flatbuffers::FieldDef*>> types;
		};
		sComponentContext getComponentTypes(flatbuffers::RPCCall* call);

		bool generateComponentPropertyHelper(
			flatbuffers::ServiceDef* service, 
			flatbuffers::RPCCall* call, 
			const sComponentContext& context);

		void importComponentTypes(sTSPrinter& printer, const sComponentContext& context);

		std::string genTypeName(const flatbuffers::Type& type);
		std::string genOffsetOrValue(const flatbuffers::Type& type, const std::string& compName, const std::string& propertyName);
	};
}