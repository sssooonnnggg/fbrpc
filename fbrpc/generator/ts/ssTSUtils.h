#pragma once

#include <string>
#include <unordered_set>

#include "flatbuffers/flatbuffers.h"
#include "flatbuffers/idl.h"

#include "fbrpc/generator/ssUtils.h"
#include "ssTSPrinter.h"

namespace fbrpc
{
    class sTSUtils
    {
    public:

        static constexpr std::string_view BindingTargetName = "fbrpc_binding";

        static std::string toFullPath(flatbuffers::Definition* def);

        template <class SymbolType, typename = std::enable_if_t<std::is_base_of_v<flatbuffers::Definition, SymbolType>>>
        static std::unordered_set<SymbolType*> getAllSymbolsFromParsers(const std::vector<flatbuffers::Parser*>& parsers)
        {
            std::unordered_set<SymbolType*> exportedTypes;
            std::unordered_set<std::string> typeNames;
            for (auto parser : parsers)
            {
                std::vector<SymbolType*> symbols;
                if constexpr (std::is_same_v<SymbolType, flatbuffers::StructDef>)
                    symbols = parser->structs_.vec;
                else if constexpr (std::is_same_v<SymbolType, flatbuffers::EnumDef>)
                    symbols = parser->enums_.vec;

                for (auto exportSymbol : symbols)
                {
                    if (typeNames.find(exportSymbol->name) == typeNames.end())
                    {
                        exportedTypes.insert(exportSymbol);
                        typeNames.insert(exportSymbol->name);
                    }
                }
            }
            return exportedTypes;
        }

        static void importDependentTypes(sTSPrinter& printer, const std::vector<flatbuffers::Parser*>& parsers);
        static void exportDependentTypes(sTSPrinter& printer, const std::vector<flatbuffers::Parser*>& parsers);

        static void importDependentEnums(sTSPrinter& printer, const std::vector<flatbuffers::Parser*>& parsers);
        static void exportDependentEnums(sTSPrinter& printer, const std::vector<flatbuffers::Parser*>& parsers);

        static std::string genTypeName(const flatbuffers::Type& type);
        static std::string genOffsetOrValue(const flatbuffers::Type& type, const std::string& compName, const std::string& propertyName);
    };
}