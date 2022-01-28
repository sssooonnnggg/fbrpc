#include <array>

#include "flatbuffers/idl.h"

#include "ssTSPrinter.h"
#include "ssTSGenerator.h"
#include "ssUtils.h"

namespace fbrpc
{
	namespace
	{
		constexpr std::string_view BindingTargetName = "fbrpc_binding";

		struct sGlobalFunction
		{
			std::string_view name;
			std::string_view decl;
		};
	}

	bool sTSGenerator::start(flatbuffers::ServiceDef* service)
	{
		return generateTSDeclareFile(service) && generateTSWrapperFile(service);
	}

	bool sTSGenerator::finish(std::vector<flatbuffers::ServiceDef*> services)
	{
		return finishTSDeclareFile(services) && finishTSWrapperFile(services);
	}

	bool sTSGenerator::generateTSDeclareFile(flatbuffers::ServiceDef* service)
	{
		auto serviceName = service->name;

		sTSPrinter printer;
		printer.addHeader();

		{
			auto scope = printer.addExport(std::string("const ") + serviceName + ":");
			for (auto call : service->calls.vec)
				printer.addContent(call->name + ": (req: Uint8Array, callback: (res: Uint8Array) => void) => void");
		}

		return writter()(printer.getOutput(), serviceName + ".d.ts");
	}

	bool sTSGenerator::generateTSWrapperFile(flatbuffers::ServiceDef* service)
	{
		auto serviceName = service->name;

		sTSPrinter printer;
		printer.addHeader();

		printer.addImport("* as flatbuffers", "flatbuffers");
		printer.addImport(std::string("{ ") + serviceName + " }", std::string("./") + BindingTargetName.data());

		importDependentTypes(printer, service);
		printer.nextLine();

		{
			auto scope = printer.addExport(std::string("class ") + serviceName + "API");
			for (auto call : service->calls.vec)
			{
				auto api = call->name;
				auto requestName = call->request->name;
				auto responseName = call->response->name;

				auto requestType = requestName + "T";
				auto requestParams = "ConstructorParameters<typeof " + requestName + "T>";
				bool isEmptyRequest = call->request->fields.vec.empty();

				if (generatorUtils::isEvent(call->response))
				{
					printer.addContent(
"static " + api + "(filter: " + requestName + "T, callback: (event: " + responseName + R"#(T) => void) {
    let builder = new flatbuffers.Builder();
    builder.finish(filter.pack(builder));
    )#" + serviceName + "." + api + R"#((builder.asUint8Array(), res => {
        let buffer = new flatbuffers.ByteBuffer(res);
        let response = )#" + responseName + ".getRootAs" + responseName + R"#((buffer);
        callback(response.unpack());
    });
})#");
				}
				else
				{
					std::string directCall = "static " + api + "(req: " + requestType + "): Promise<" + responseName + "T>;";
					std::string flatCall = "static " + api + "(...args: " + requestParams + "): Promise<" + responseName + "T>;";

					// generate overload functions
					printer.addContent(directCall);
					printer.addContent(flatCall);

					printer.addContent(
"static " + api + "(...args: any[]): Promise<" + responseName + R"#(T> {
    return new Promise(resolve => {
        let builder = new flatbuffers.Builder();
        let req = args[0] instanceof )#" + requestType + " ? args[0] : new " + requestType + (isEmptyRequest ? "()" : "(...args)") + R"#(;
        builder.finish(req.pack(builder));
        )#" + serviceName + "." + api + R"#((builder.asUint8Array(), res => {
            let buffer = new flatbuffers.ByteBuffer(res);
            let response = )#" + responseName + ".getRootAs" + responseName + R"#((buffer);
            resolve(response.unpack());
        });
    })
})#");
				}
			}
		}

		return writter()(printer.getOutput(), serviceName + "API.ts");
	}

	bool sTSGenerator::finishTSDeclareFile(const std::vector<flatbuffers::ServiceDef*>& services)
	{
		sTSPrinter printer;
		printer.addHeader();

		for (auto service : services)
			printer.addContent(std::string("export { ") + service->name + " } from './" + service->name + "'");

		return writter()(printer.getOutput(), std::string(BindingTargetName.data()) + ".d.ts");
	}

	bool sTSGenerator::finishTSWrapperFile(const std::vector<flatbuffers::ServiceDef*>& services)
	{
		sTSPrinter printer;
		printer.addHeader();

		for (auto service : services)
			exportDependentTypes(printer, service);

		printer.nextLine();

		for (auto service : services)
			printer.addContent(std::string("export { ") + service->name + "API } from './" + service->name + "API'");

		printer.addContent("export * from './internal'");

		return writter()(printer.getOutput(), "FlatBufferAPI.ts");
	}

	void sTSGenerator::importDependentTypes(sTSPrinter& printer, flatbuffers::ServiceDef* service)
	{
		foreachDependentType(printer, service, [&printer](auto&& targets, auto&& from)
			{
				printer.addImport(targets, from);
			}
		);
	}

	void sTSGenerator::exportDependentTypes(sTSPrinter& printer, flatbuffers::ServiceDef* service)
	{
		foreachDependentType(printer, service, [&printer](auto&& targets, auto&& from)
			{
				printer.addContent(std::string("export ") + targets + " from '" + from + "'");
			}
		);
	}

	void sTSGenerator::foreachDependentType(
		sTSPrinter& printer,
		flatbuffers::ServiceDef* service,
		std::function<void(const std::string types, const std::string& from)> processor
	)
	{
		auto nameSpace = service->defined_namespace->components;
		std::string ns = nameSpace.size() > 0 ? nameSpace[0] : "";

		std::vector<std::string> dependentTypes;
		std::set<std::string> dependentTypesSet;

		auto addDependentType = [&dependentTypes, &dependentTypesSet](std::string type)
		{
			if (dependentTypesSet.find(type) == dependentTypesSet.end())
			{
				dependentTypesSet.insert(type);
				dependentTypes.push_back(type);
			}
		};

		for (auto call : service->calls.vec)
		{
			addDependentType(call->request->name);
			addDependentType(call->response->name);
		}

		auto nameSpaceDir = generatorUtils::toDasherizedCase(ns);

		for (const auto& type : dependentTypes)
			processor(
				std::string("{ ") + type + ", " + type + "T }", 
				std::string("./") + nameSpaceDir + "/" + generatorUtils::toDasherizedCase(type));
	}
}