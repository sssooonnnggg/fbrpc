#pragma once

#include "fbrpc/generator/ts/ssTSPrinter.h"
#include "flatbuffers/flatbuffers.h"
#include "flatbuffers/idl.h"

#include "ssTSGenerator.h"

namespace fbrpc
{
    class sTSComponentGenerator
    {
    public:
        static void importTypes(sTSPrinter& printer);
        static bool generate(
            sLanguageGenerator::sWriteFileDelegate writter, 
            sTSPrinter& printer, 
            flatbuffers::ServiceDef* service, 
            flatbuffers::RPCCall* call);

    private:
        struct sComponentContext
        {
            std::string ns;
            std::map<std::string, std::vector<flatbuffers::FieldDef*>> types;
        };
        static sComponentContext getComponentContext(flatbuffers::RPCCall* call);

        static bool generateComponentPropertyHelper(
            sLanguageGenerator::sWriteFileDelegate writter,
            flatbuffers::ServiceDef* service,
            flatbuffers::RPCCall* call,
            const sComponentContext& context);

        static bool generateComponentTypes(
            sLanguageGenerator::sWriteFileDelegate writter,
            flatbuffers::ServiceDef* service,
            flatbuffers::RPCCall* call,
            const sComponentContext& context);

        static void importComponentTypes(sTSPrinter& printer, const sComponentContext& context);
    };
}