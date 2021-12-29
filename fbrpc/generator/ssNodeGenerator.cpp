#include "flatbuffers/idl.h"

#include "ssCppPrinter.h"
#include "ssNodeGenerator.h"

namespace fbrpc
{
	bool sNodeGenerator::start(flatbuffers::ServiceDef* service)
	{
		return generateNodeBindingFile(service) && generateTypeScriptFile(service);
	}

	bool sNodeGenerator::finish(std::vector<flatbuffers::ServiceDef*> services)
	{
		return generateFlatBufferBindingFile(services);
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
			std::unique_ptr<sScope> namespaceScope;
			if (!namespaces.empty())
				namespaceScope = printer.addNamespace(namespaces[0]);

			{
				auto classScope = printer.addClass(serviceName);
				printer.addClassAccessSpecifier("public:");
				printer.addContent(
					R"#(static std::size_t serviceHash() { static auto hash = fbrpc::getHash(")#" + serviceName + R"#(Service"); return hash; })#");

				for (auto call : service->calls.vec)
				{
					auto apiName = call->name;

					printer.addContent(R"#(static void )#" + apiName + R"#((fbrpc::sBuffer buffer, std::function<void(fbrpc::sBuffer)> callback)
{
	auto client = FlatbufferClient::get();
	if (!client || !client->isConnected())
		throw std::runtime_error("clinet is not connected");

	static std::size_t apiHash = fbrpc::getHash(")#" + apiName + R"#(");
	client->call(serviceHash(), apiHash, std::move(buffer), [callback](fbrpc::sBufferView view)
		{
			callback(fbrpc::sBuffer::clone(view.data, view.length));
		}
	);
})#");
				}

				printer.addContent("static BindingHelper& bind(BindingHelper& helper)");

				{
					auto scope = printer.addScope();
					printer.addContent("return helper.begin(\"" + serviceName + "\")");
					{
						for (auto call : service->calls.vec)
							printer.addContent(
								"    .addStaticFunction<" + serviceName + "::" + call->name + ">(\"" + call->name + "\")"
							);
						printer.addContent(".end();");
					}
				}
			}
		}

		return writter()(printer.getOutput(), serviceName + "Binding_generated.h");
	}

	bool sNodeGenerator::generateTypeScriptFile(flatbuffers::ServiceDef* service)
	{
		return true;
	}

	bool sNodeGenerator::generateFlatBufferBindingFile(const std::vector<flatbuffers::ServiceDef*>& services)
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

		return writter()(printer.getOutput(), "FlatBufferBinding_generated.h");
	}
}