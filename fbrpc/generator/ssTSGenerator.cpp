#include "flatbuffers/idl.h"

#include "ssTSPrinter.h"
#include "ssTSGenerator.h"

namespace fbrpc
{
	namespace
	{
		std::string toDasherizedCase(std::string camel)
		{
			std::string result;
			for (auto c : camel)
			{
				if (std::isupper(c))
				{
					if (!result.empty())
						result += "-";
					result += std::tolower(c);
				}
				else
				{
					result += c;
				}
			}
			return result;
		};

		bool isEvent(const std::string& responseName)
		{
			return responseName.find("Event") != std::string::npos;
		}
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
		printer.addImport(std::string("{ ") + serviceName + " }", "./flatbuffer_binding");

		importDependentTypes(printer, service);
		printer.nextLine();

		{
			auto scope = printer.addExport(std::string("class ") + serviceName + "API");
			for (auto call : service->calls.vec)
			{
				auto api = call->name;
				auto requestName = call->request->name;
				auto responseName = call->response->name;

				if (isEvent(responseName))
				{
					printer.addContent("static " + api + "(filter: " + requestName + "T, callback: (event: " + responseName + R"#(T) => void) {
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
					printer.addContent("static " + api + "(req: " + requestName + "T): Promise<" + responseName + R"#(T> {
    return new Promise(resolve => {
        let builder = new flatbuffers.Builder();
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

		printer.addContent("export function connect(option: { address: string, port: number }): number");

		for (auto service : services)
			printer.addContent(std::string("export { ") + service->name + " } from './" + service->name + "'");

		return writter()(printer.getOutput(), "flatbuffer_binding.d.ts");
	}

	bool sTSGenerator::finishTSWrapperFile(const std::vector<flatbuffers::ServiceDef*>& services)
	{
		sTSPrinter printer;
		printer.addHeader();

		printer.addContent("export { connect } from './flatbuffer_binding'");

		for (auto service : services)
			exportDependentTypes(printer, service);

		printer.nextLine();

		for (auto service : services)
			printer.addContent(std::string("export { ") + service->name + "API } from './" + service->name + "API'");

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

		auto nameSpaceDir = toDasherizedCase(ns);

		for (const auto& type : dependentTypes)
			processor(std::string("{ ") + type + ", " + type + "T }", std::string("./") + nameSpaceDir + "/" + toDasherizedCase(type));
	}
}