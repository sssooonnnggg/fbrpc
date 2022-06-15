#include "ssTSPrinter.h"
#include "ssTSDocGenerator.h"
#include "ssTSUtils.h"

namespace fbrpc
{
    bool sTSDocGenerator::generate(sTSPrinter& printer, flatbuffers::ServiceDef* service, flatbuffers::RPCCall* call)
    {
        if (!call->doc_comment.empty())
        {
            printer.addContent("/**");

            for (auto&& comment : call->doc_comment)
                printer.addContent(std::string("*") + comment);

            auto* request = call->request;
            auto* response = call->response;

            auto& requestFields = request->fields.vec;
            if (!requestFields.empty())
            {
                for (auto* field : requestFields)
                {
                    auto& fieldDoc = field->doc_comment;
                    printer.addContent(
                        "* @param **$name$: $type$** $doc$",
                        {{"type", sTSUtils::genTypeName(field->value.type)},
                         {"name", field->name},
                         {"doc", fieldDoc.empty() ? "" : fieldDoc[0]}});
                }
                printer.nextLine();
            }

            auto& responseFields = response->fields.vec;
            if (!responseFields.empty())
            {
                for (auto* field : responseFields)
                {
                    auto& fieldDoc = field->doc_comment;
                    printer.addContent(
                        "* @returns **$name$: $type$** $doc$",
                        {{"type", sTSUtils::genTypeName(field->value.type)},
                         {"name", field->name},
                         {"doc", fieldDoc.empty() ? "" : fieldDoc[0]}});
                }
                printer.nextLine();
            }

            printer.addContent("*/");
        }

        return true;
    }
}