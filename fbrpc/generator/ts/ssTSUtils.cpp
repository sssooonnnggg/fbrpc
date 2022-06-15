#include "ssTSUtils.h"
#include "ssTSPrinter.h"

namespace
{
    constexpr std::string_view UnsupportedType = "TODO: Unsupported Type";
    constexpr std::string_view UnsupportedValue = "TODO: Unsupported Value";
}

namespace fbrpc
{
    std::string sTSUtils::toFullPath(flatbuffers::Definition* def)
    {
        return std::string("./") + generatorUtils::toNameSpacePath(def->defined_namespace) + "/" + generatorUtils::toDasherizedCase(def->name);
    }

    std::string sTSUtils::genTypeName(const flatbuffers::Type& type)
    {
        switch (type.base_type)
        {
        case flatbuffers::BASE_TYPE_BOOL:
            return "boolean";
        case flatbuffers::BASE_TYPE_STRING:
            return "string";
        case flatbuffers::BASE_TYPE_VECTOR:
            return genTypeName(type.VectorType()) + "[]";
        case flatbuffers::BASE_TYPE_LONG:
        case flatbuffers::BASE_TYPE_ULONG:
            return "bigint";
        case flatbuffers::BASE_TYPE_STRUCT:
            return type.struct_def->name + "T";
        default:
            if (IsScalar(type.base_type))
            {
                if (type.enum_def)
                    return type.enum_def->name;
                return "number";
            }
            return std::string(UnsupportedType);
        }
    }

    std::string sTSUtils::genOffsetOrValue(const flatbuffers::Type& type, const std::string& compName, const std::string& propertyName)
    {
        switch (type.base_type)
        {
        case flatbuffers::BASE_TYPE_STRING:
            return "(value !== null ? this.builder.createString(value) : 0)";
        case flatbuffers::BASE_TYPE_STRUCT:
            if (flatbuffers::IsStruct(type))
                return "value";
            else
                return "value.pack(this.builder)";
        case flatbuffers::BASE_TYPE_VECTOR: {
            if (type.VectorType().base_type == flatbuffers::BASE_TYPE_STRING)
                return compName + ".create" + propertyName + "Vector(this.builder, this.builder.createObjectOffsetList(value))";
            else if (flatbuffers::IsStruct(type.VectorType()))
                return "this.builder.createStructOffsetList(value, " + compName + ".start" + propertyName + "Vector)";
            else if (flatbuffers::IsScalar(type.VectorType().base_type))
                return compName + ".create" + propertyName + "Vector(this.builder, value)";
            else if (type.VectorType().base_type == flatbuffers::BASE_TYPE_STRUCT)
                return compName + ".create" + propertyName + "Vector(this.builder, this.builder.createObjectOffsetList(value))";
            else
                return std::string(UnsupportedValue);
        }
        default:
            if (flatbuffers::IsScalar(type.base_type))
                return "value";
            else
                return std::string(UnsupportedValue);
        }
    }

    void sTSUtils::importDependentTypes(sTSPrinter& printer, const std::vector<flatbuffers::Parser*>& parsers)
	{
		auto types = sTSUtils::getAllSymbolsFromParsers<flatbuffers::StructDef>(parsers);
		for (auto type : types)
			printer.addContent(std::string("import { ") + type->name + "T } from '" + sTSUtils::toFullPath(type) + "'");
	}

	void sTSUtils::exportDependentTypes(sTSPrinter& printer, const std::vector<flatbuffers::Parser*>& parsers)
	{
		auto types = sTSUtils::getAllSymbolsFromParsers<flatbuffers::StructDef>(parsers);
		for (auto type : types)
			printer.addContent(std::string("export { ") + type->name + "T } from '" + sTSUtils::toFullPath(type) + "'");
	}

	void sTSUtils::importDependentEnums(sTSPrinter& printer, const std::vector<flatbuffers::Parser*>& parsers)
	{
		auto types = sTSUtils::getAllSymbolsFromParsers<flatbuffers::EnumDef>(parsers);
		for (auto type : types)
			printer.addContent(std::string("import { ") + type->name + " } from '" + sTSUtils::toFullPath(type) + "'");
	}

	void sTSUtils::exportDependentEnums(sTSPrinter& printer, const std::vector<flatbuffers::Parser*>& parsers)
	{
		auto types = sTSUtils::getAllSymbolsFromParsers<flatbuffers::EnumDef>(parsers);
		for (auto type : types)
			printer.addContent(std::string("export { ") + type->name + " } from '" + sTSUtils::toFullPath(type) + "'");
	}
}