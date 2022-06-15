#include <array>
#include <algorithm>

#include "fbrpc/generator/ts/ssTSDocGenerator.h"
#include "fbrpc/generator/ts/ssTSTypesGenerator.h"
#include "flatbuffers/idl.h"

#include "fbrpc/generator/ssUtils.h"
#include "fbrpc/core/ssLogger.h"

#include "ssTSPrinter.h"
#include "ssTSGenerator.h"
#include "ssTSUtils.h"
#include "ssTSEventGenerator.h"
#include "ssTSComponentGenerator.h"

namespace fbrpc
{
	using namespace generatorUtils;

	bool sTSGenerator::start(flatbuffers::ServiceDef *service)
	{
		return generateTSDeclareFile(service) && generateTSWrapperFile(service);
	}

	bool sTSGenerator::finish(sContext context)
	{
		return finishTSDeclareFile(context.services) && finishTSWrapperFile(context);
	}

	bool sTSGenerator::generateTSDeclareFile(flatbuffers::ServiceDef *service)
	{
		auto serviceName = service->name;
		sTSPrinter printer;
		printer.addHeader();
		{
			auto scope = printer.addExport(std::string("const ") + serviceName + ":");
			for (auto call : service->calls.vec)
				printer.addContent(
					"$apiName$: (req: Uint8Array, callback: (res: Uint8Array) => void) => void", 
					{{"apiName", call->name}});
		}
		return writter()(printer.getOutput(), serviceName + ".d.ts");
	}
	
	bool sTSGenerator::generateTSWrapperFile(flatbuffers::ServiceDef* service)
	{
		auto serviceName = service->name;
		sTSPrinter printer;
		printer.addHeader();
		printer.addImport("* as flatbuffers", "flatbuffers");
		printer.addImport("{ PowerPartial }", "./internal");
		printer.addImport(std::string("{ ") + serviceName + " }", std::string("./") + sTSUtils::BindingTargetName.data());

		importServiceDependentTypes(printer, service);

		auto& calls = service->calls.vec;
		bool hasComponentPropertySetter = (std::find_if(
			calls.begin(),
			calls.end(),
			[](auto&& call) { return hasTag(call->request, sTag::ComponentPropertySetter); }
		) != calls.end());

		if (hasComponentPropertySetter)
			sTSComponentGenerator::importTypes(printer);

		printer.nextLine();
		auto importIndex = printer.length();

		{
			auto scope = printer.addExport(std::string("class ") + serviceName + "API");
			bool componentGenerated = false;
			for (auto call : service->calls.vec)
			{
				if (isEvent(call->response))
				{
					sTSEventGenerator::generate(printer, service, call);
				}
				else if (hasTag(call->request, sTag::ComponentPropertySetter))
				{
					if (!componentGenerated)
					{
						sTSComponentGenerator::generate(writter(), printer, service, call);
						componentGenerated = true;
					}
				}
				else
				{
					generateDefaultCall(printer, service, call);
				}
			}
		}

		return writter()(printer.getOutput(), serviceName + "API.ts");
	}

	bool sTSGenerator::generateDefaultCall(sTSPrinter& printer, flatbuffers::ServiceDef* service, flatbuffers::RPCCall* call)
	{
		auto serviceName = service->name;
		auto api = call->name;
		auto requestName = call->request->name;
		auto responseName = call->response->name;

		auto requestType = requestName + "T";
		auto responseType = responseName + "T";
		bool isEmptyRequest = call->request->fields.vec.empty();

		sPrinter::sVarsMap vars = {
			{"api", api},
			{"requestType", requestType},
			{"responseType", responseType},
			{"serviceName", serviceName},
			{"responseName", responseName}
		};

		sTSDocGenerator::generate(printer, service, call);

		if (isEmptyRequest)
			printer.addContent("static $api$() : Promise<$responseType$>;", vars);
		else
			printer.addContent("static $api$(req: PowerPartial<Plain<$requestType$>>) : Promise<$responseType$>;", vars);

		printer.addContent(
			R"#(static $api$(req: $requestType$): Promise<$responseType$>;
static $api$(req?: any): Promise<$responseType$> {
    return new Promise(resolve => {
        let builder = new flatbuffers.Builder();
        let request = req instanceof $requestType$ ? req : (req === undefined ? new $requestType$() : fromPlain$requestType$(req));
        builder.finish(request.pack(builder));
        $serviceName$.$api$(builder.asUint8Array(), res => {
            let buffer = new flatbuffers.ByteBuffer(res);
            let response = $responseName$.getRootAs$responseName$(buffer);
            resolve(response.unpack());
        });
    })
})#", vars);
		return true;
	}

	bool sTSGenerator::finishTSDeclareFile(const std::vector<flatbuffers::ServiceDef *> &services)
	{
		sTSPrinter printer;
		printer.addHeader();
		for (auto service : services)
			printer.addContent(std::string("export { ") + service->name + " } from './" + service->name + "'");
		return writter()(printer.getOutput(), std::string(sTSUtils::BindingTargetName.data()) + ".d.ts");
	}

	bool sTSGenerator::finishTSWrapperFile(const sContext& context)
	{
		sTSPrinter typesPrinter;
		typesPrinter.addHeader();

		if (!sTSTypesGenerator::generatePlain(writter(), context))
			return false;

		if (!sTSTypesGenerator::generateComplexTypeMeta(writter(), context))
			return false;

		typesPrinter.nextLine();

		sTSUtils::exportDependentTypes(typesPrinter, context.parsers);

		typesPrinter.nextLine();
		sTSUtils::exportDependentEnums(typesPrinter, context.parsers);

		typesPrinter.addContent("export { Plain } from './FromPlain'");
		typesPrinter.addContent("export { ComplexTypeMeta } from './ComplexTypeMeta'");
		typesPrinter.addContent("export { ComponentTypeMap } from './ComponentTypes'");

		if (!writter()(typesPrinter.getOutput(), "types.ts"))
		{
			logger().error("write tyeps.ts failed");
			return false;
		}

		sTSPrinter printer;
		printer.addHeader();

		printer.nextLine();
		for (auto service : context.services)
			printer.addContent(std::string("export { ") + service->name + "API } from './" + service->name + "API'");

		printer.nextLine();
		printer.addContent("export * from './internal'");
		printer.addContent("export * from './types'");

		return writter()(printer.getOutput(), "FlatBufferAPI.ts");
	}

	void sTSGenerator::importServiceDependentTypes(sTSPrinter& printer, flatbuffers::ServiceDef* service)
	{
		auto nameSpace = service->defined_namespace->components;
		std::string ns = nameSpace.size() > 0 ? nameSpace[0] : "";

		struct sDependentType
		{
			std::string name;
			bool isRequest;
		};

		std::vector<sDependentType> dependentTypes;
		std::set<std::string> dependentTypesSet;

		auto addDependentType = [&dependentTypes, &dependentTypesSet](std::string type, bool isRequest)
		{
			if (dependentTypesSet.find(type) == dependentTypesSet.end())
			{
				dependentTypesSet.insert(type);
				dependentTypes.push_back({ type, isRequest });
			}
		};

		for (auto call : service->calls.vec)
		{
			addDependentType(call->request->name, true);
			addDependentType(call->response->name, false);
		}

		auto nameSpaceDir = toDasherizedCase(ns);

		for (const auto& type : dependentTypes)
		{
			printer.addImport(
				std::string("{ ") + type.name + "," + type.name + "T }",
				std::string("./") + nameSpaceDir + "/" + toDasherizedCase(type.name));

			if (type.isRequest)
				printer.addImport(std::string("{ fromPlain") + type.name + "T }", "./FromPlain");
		}

		printer.addImport("{ Plain }", "./FromPlain");
	}
}