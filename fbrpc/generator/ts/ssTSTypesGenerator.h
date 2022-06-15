#pragma once

#include "flatbuffers/flatbuffers.h"
#include "flatbuffers/idl.h"

#include "ssTSGenerator.h"

namespace fbrpc
{
    class sTSTypesGenerator
    {
    public:
        static bool generatePlain(sLanguageGenerator::sWriteFileDelegate writter, const sLanguageGenerator::sContext& context);
        static bool generateComplexTypeMeta(sLanguageGenerator::sWriteFileDelegate writter, const sLanguageGenerator::sContext& context);
    };
}