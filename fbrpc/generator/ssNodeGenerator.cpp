#include "flatbuffers/idl.h"

#include "ssCppPrinter.h"
#include "ssNodeGenerator.h"

#include "ssUtils.h"

namespace fbrpc
{
	bool sNodeGenerator::start(flatbuffers::ServiceDef* service)
	{
		return generateNodeBindingFile(service);
	}

	bool sNodeGenerator::finish(sContext context)
	{
		return finishNodeBindingFile(context.services);
	}

	bool sNodeGenerator::generateNodeBindingFile(flatbuffers::ServiceDef* service)
	{
		std::string serviceName = service->name;
		auto namespaces = service->defined_namespace->components;

		sCppPrinter printer;
		printer.addHeader();
		printer.addInclude("FlatBufferBinding.h");
		printer.addInclude("fbrpc/ssFlatBufferRpc.h");
		printer.addInclude(serviceName + "_generated.h");
		printer.nextLine();

		{
			std::unique_ptr<sPrinter::sScope> namespaceScope;
			if (!namespaces.empty())
				namespaceScope = printer.addNamespace(namespaces[0]);

			{
				auto classScope = printer.addClass(serviceName);
				printer.addClassAccessSpecifier("public:");
				printer.addContent(
					R"#(static std::size_t serviceHash() { static auto hash = fbrpc::getHash(")#" + serviceName + R"#(Service"); return hash; })#");

				for (auto call : service->calls.vec)
				{
					sPrinter::sVarsMap vars = {
						{"apiName", call->name},
						{"repeat", generatorUtils::isEvent(call->response) ? "true" : "false"}
					};

					printer.addContent(
R"#(static void $apiName$(fbrpc::sBuffer buffer, std::function<void(fbrpc::sBuffer)> callback)
{
	static std::size_t apiHash = fbrpc::getHash("$apiName$");
	auto client = FlatbufferClient::get();
	client->call(serviceHash(), apiHash, std::move(buffer), [callback](fbrpc::sBufferView view)
		{
			callback(fbrpc::sBuffer::clone(view.data, view.length));
		}
	, $repeat$);
})#", vars);
				}

				printer.addContent("static BindingHelper& bind(BindingHelper& helper)");

				{
					sPrinter::sVarsMap vars = {
						{"serviceName", serviceName}
					};
					auto scope = printer.addScope();
					printer.addContent("return helper.begin(\"$serviceName$\")", vars);
					{
						for (auto call : service->calls.vec)
						{
							vars["api"] = call->name;
							printer.addContent(
								"    .addStaticFunction<$serviceName$::$api$>(\"$api$\")", vars
							);
						}
						printer.addContent(".end();");
					}
				}
			}
		}

		return writter()(printer.getOutput(), serviceName + "Binding_generated.h");
	}

	bool sNodeGenerator::finishNodeBindingFile(const std::vector<flatbuffers::ServiceDef*>& services)
	{
		sCppPrinter printer;
		printer.addHeader();

		for (auto service : services)
		{
			auto serviceName = service->name;
			printer.addInclude(serviceName + "Binding_generated.h");
		}

		printer.nextLine();

		{
			auto scope = printer.addClass("FlatBufferBinding");
			printer.addClassAccessSpecifier("public:");
			printer.addContent("static BindingHelper& bind(BindingHelper& helper)");
			{
				auto scope = printer.addScope();

				for (auto service : services)
				{
					auto namespaces = service->defined_namespace->components;
					auto serviceName = service->name;
					auto qualifiedName = namespaces.size() > 0 ? (namespaces[0] + "::" + serviceName) : serviceName;
					printer.addContent(qualifiedName + "::bind(helper);");
				}

				printer.addContent("return helper;");
			}
		}

		return writter()(printer.getOutput(), "FlatBufferRpcBinding_generated.h");
	}
}