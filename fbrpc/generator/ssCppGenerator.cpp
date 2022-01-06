#include "flatbuffers/idl.h"

#include "ssCppPrinter.h"
#include "ssCppGenerator.h"

#include "ssUtils.h"

namespace fbrpc
{
	bool sCppGenerator::start(flatbuffers::ServiceDef* service)
	{
		return generateServiceFile(service) && generateStubFile(service);
	}

	bool sCppGenerator::generateServiceFile(flatbuffers::ServiceDef* service)
	{
		std::string serviceName = service->name;
		auto namespaces = service->defined_namespace->components;

		sCppPrinter printer;
		printer.addHeader();
		printer.addInclude("fbrpc/core/ssPromise.h");
		printer.addInclude("fbrpc/interface/ssService.h");
		printer.addInclude(serviceName + "_generated.h");
		printer.nextLine();

		{
			printer.addContent("using namespace fbrpc;");
			printer.nextLine();

			std::unique_ptr<sPrinter::sScope> namespaceScope;
			if (!namespaces.empty())
				namespaceScope = printer.addNamespace(namespaces[0]);

			{
				auto className = serviceName + "Service";
				auto classScope = printer.addClass(className, "sService");
				printer.addClassAccessSpecifier("public:");
				printer.addContent(
					std::string("std::string name() const override { return \"") + className + "\"; }");

				for (auto call : service->calls.vec)
				{
					auto apiName = call->name;
					auto requestName = call->request->name;
					auto responseName = call->response->name;

					if (generatorUtils::isEvent(call->response))
						printer.addContent("virtual void " + apiName + "(const " + requestName + "* filter, std::unique_ptr<sEventEmitter<"
							+ responseName + ">> emitter) = 0; ");
					else
						printer.addContent("virtual void " + apiName + "(const " + requestName + "* request, std::unique_ptr<sPromise<"
							+ responseName + ">> response) = 0; ");
				}

				printer.nextLine();
				printer.addContent("void init() override");
				{
					auto fucntion = printer.addScope();
					for (auto call : service->calls.vec)
					{
						auto apiName = call->name;
						auto requestName = call->request->name;
						auto responseName = call->response->name;

						printer.addContent(R"#(
addApiWrapper(getHash(")#" + apiName + R"#("), [this](sBufferView buffer, sResponder responder)
	{
		auto* request = flatbuffers::GetRoot<)#" + requestName + R"#(>(reinterpret_cast<const void*>(buffer.data));
		auto promise = createPromise<)#" + responseName + R"#(>();
		sUniqueFunction<void()> sendResponse = [capturedPromise = promise.get(), capturedResponder = std::move(responder)]() mutable
		{
			capturedResponder(sBuffer::clone(capturedPromise->builder()));
		};
		promise->bind(std::move(sendResponse));
		)#" + apiName + R"#((request, std::move(promise));
	}
);)#");
					}
				}
			}
		}

		return writter()(printer.getOutput(), serviceName + "Service_generated.h");
	}

	bool sCppGenerator::generateStubFile(flatbuffers::ServiceDef* service)
	{
		std::string serviceName = service->name;
		auto namespaces = service->defined_namespace->components;

		sCppPrinter printer;
		printer.addHeader();
		printer.addInclude("fbrpc/interface/ssStub.h");
		printer.addInclude("fbrpc/core/ssBuffer.h");
		printer.addInclude(serviceName + "_generated.h");
		printer.nextLine();

		{
			printer.addContent("using namespace fbrpc;");
			printer.nextLine();

			std::unique_ptr<sPrinter::sScope> namespaceScope;
			if (!namespaces.empty())
				namespaceScope = printer.addNamespace(namespaces[0]);

			{
				auto className = serviceName + "Stub";
				auto classScope = printer.addClass(className, "sStub");
				printer.addClassAccessSpecifier("public:");
				printer.addContent(className + "(sFlatBufferRpcClient* client): sStub(client) {}");
				printer.addContent(
					R"#(static std::size_t serviceHash() { static auto hash = getHash(")#" + serviceName + R"#(Service"); return hash; })#");
				printer.addContent(
					R"#(static std::size_t typeHash() { static auto hash = getHash(")#" + className + R"#("); return hash; })#");
				printer.addContent("std::size_t hash() const override { return typeHash(); }");

				for (auto call : service->calls.vec)
				{
					auto apiName = call->name;
					auto requestName = call->request->name;
					auto responseName = call->response->name;

					printer.nextLine();
					printer.addContent(
						std::string("using ") + responseName + "Handler = std::function<void(const " + responseName + "*)>;");
					printer.addContent(
						std::string("void ") + apiName + "(flatbuffers::Offset<" + requestName + "> request, "
						+ responseName + "Handler handler)"
					);

					auto functionScope = printer.addScope();
					printer.addContent(R"#(static auto apiHash = getHash(")#" + apiName + R"#(");
builder().Finish(request);
call(serviceHash(), apiHash, sBuffer::clone(builder()), [handler](sBufferView buffer)
	{
		auto* response = flatbuffers::GetRoot<)#" + responseName + R"#(>(buffer.data);
		handler(response);
	}
);
builder().Clear();)#");
				}
			}
		}

		return writter()(printer.getOutput(), serviceName + "Stub_generated.h");
	}
}