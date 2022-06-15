#include "fbrpc/generator/ts/ssTSDocGenerator.h"
#include "ssTSPrinter.h"
#include "ssTSEventGenerator.h"

namespace fbrpc
{
    bool sTSEventGenerator::generate(sTSPrinter& printer, flatbuffers::ServiceDef* service, flatbuffers::RPCCall* call)
    {
        auto serviceName = service->name;
        auto api = call->name;
        auto requestName = call->request->name;
        auto responseName = call->response->name;
        auto requestType = requestName + "T";
        auto responseType = responseName + "T";
        bool isEmptyFilter = call->request->fields.vec.size() == 0;
        std::string filter = isEmptyFilter ? "" : std::string("filter: ") + requestType + ", ";
        std::string packFilter = isEmptyFilter ? std::string("new ") + requestType + "().pack(builder)" : "filter.pack(builder)";

        sTSDocGenerator::generate(printer, service, call);

        sPrinter::sVarsMap vars = {
            {"api", api},
            {"requestType", requestType},
            {"responseType", responseType},
            {"serviceName", serviceName},
            {"responseName", responseName},
            {"filter", filter},
            {"packFilter", packFilter}};
        std::string_view content =
            R"#(static $api$($filter$callback: (event: $responseType$) => void) {
    let builder = new flatbuffers.Builder();
    builder.finish($packFilter$);
    $serviceName$.$api$(builder.asUint8Array(), res => {
        let buffer = new flatbuffers.ByteBuffer(res);
        let response = $responseName$.getRootAs$responseName$(buffer);
        callback(response.unpack());
    });
})#";
        printer.addContent(content, vars);
        return true;
    }
}