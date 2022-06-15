#pragma once

#include "flatbuffers/flatbuffers.h"
#include "flatbuffers/idl.h"

#include "ssTSGenerator.h"

namespace fbrpc
{
    class sTSEventGenerator
    {
    public:
        static bool generate(sTSPrinter& printer, flatbuffers::ServiceDef* service, flatbuffers::RPCCall* call);
    };
}
