#include "ssTSUtils.h"
#include "fbrpc/core/ssLogger.h"
#include "ssTSPrinter.h"
#include "ssTSTypesGenerator.h"

namespace fbrpc
{
    bool sTSTypesGenerator::generatePlain(sLanguageGenerator::sWriteFileDelegate writter, const sLanguageGenerator::sContext& context)
    {
        sTSPrinter fromPlainPrinter;
        fromPlainPrinter.addHeader();

        auto types = sTSUtils::getAllSymbolsFromParsers<flatbuffers::StructDef>(context.parsers);
        sTSUtils::importDependentTypes(fromPlainPrinter, context.parsers);
        sTSUtils::importDependentEnums(fromPlainPrinter, context.parsers);

        fromPlainPrinter.nextLine();
        fromPlainPrinter.addContent(R"#(
type InnerType<T> = T extends Array<infer Type> ? Type : never;
type IsArray<T> = T extends any[] ? true : false;
export type Plain<T> = { [P in Exclude<keyof T, 'pack'>]: IsArray<T[P]> extends true ?
	(InnerType<T[P]> extends object ? Plain<InnerType<T[P]>>[] : T[P]) :
	(T[P] extends object | null ? Plain<NonNullable<T[P]>> : T[P]);};)#");
        fromPlainPrinter.nextLine();

        for (auto type : types)
        {
            std::string typeName = type->name + "T";
            fromPlainPrinter.addContent("export function fromPlain$type$(input: Plain<$type$>): $type$ {", {{"type", typeName}});
            fromPlainPrinter.addContent("    let result = new $type$();", {{"type", typeName}});

            auto& fields = type->fields;
            for (const auto& [key, field] : fields.dict)
            {
                if (field->deprecated)
                    continue;

                if (field->value.type.base_type == flatbuffers::BASE_TYPE_STRUCT)
                {
                    fromPlainPrinter.addContent("    if (input.$key$ !== undefined && input.$key$ !== null) result.$key$ = fromPlain$type$(input.$key$!);", {{"key", key}, {"type", field->value.type.struct_def->name + "T"}});
                }
                else if (field->value.type.base_type == flatbuffers::BASE_TYPE_VECTOR && field->value.type.VectorType().base_type == flatbuffers::BASE_TYPE_STRUCT)
                {
                    fromPlainPrinter.addContent("    if (input.$key$ !== undefined && input.$key$ !== null) result.$key$ = [...input.$key$.map(v => fromPlain$type$(v!))];", {{"key", key}, {"type", field->value.type.VectorType().struct_def->name + "T"}});
                }
                else if (field->value.type.base_type == flatbuffers::BASE_TYPE_UNION)
                {
                    auto enumDef = field->value.type.enum_def;
                    std::string enumNameSpace = enumDef->name;
                    auto types = enumDef->Vals();
                    for (auto type : types)
                    {
                        auto unionType = type->union_type.struct_def;
                        if (unionType)
                        {
                            auto typeName = unionType->name + "T";
                            fromPlainPrinter.addContent(
                                "    if (input.$key$_type == $enumNamespace$.$enumName$) result.$key$ = fromPlain$typeName$(input.$key$! as Plain<$typeName$>);",
                                {{"key", key}, {"enumNamespace", enumNameSpace}, {"enumName", unionType->name}, {"typeName", typeName}});
                        }
                    }
                }
                else if (field->value.type.base_type == flatbuffers::BASE_TYPE_VECTOR && field->value.type.VectorType().base_type == flatbuffers::BASE_TYPE_UNION)
                {
                    fromPlainPrinter.addContent(
                        "    if (input.$key$ !== undefined && input.$key$ !== null) result.$key$ = [...input.$key$.map((v, i) => {",
                        {{"key", key}});
                    auto enumDef = field->value.type.enum_def;
                    std::string enumNameSpace = enumDef->name;
                    auto types = enumDef->Vals();
                    fromPlainPrinter.addContent(
                        "            if (input.$key$_type === undefined || input.$key$_type === null) throw new Error(\"$key$_type undefined\");",
                        {{"key", key}});
                    for (auto type : types)
                    {
                        auto unionType = type->union_type.struct_def;
                        if (unionType)
                        {
                            auto typeName = unionType->name + "T";
                            fromPlainPrinter.addContent(
                                "           if (input.$key$_type[i] == $enumNamespace$.$enumName$) return fromPlain$typeName$(v as Plain<$typeName$>);",
                                {{"key", key}, {"enumNamespace", enumNameSpace}, {"enumName", unionType->name}, {"typeName", typeName}});
                        }
                    }
                    fromPlainPrinter.addContent("       throw new Error(\"invalid $key$ type\");", {{"key", key}});
                    fromPlainPrinter.addContent("    })];");
                }
                else
                {
                    fromPlainPrinter.addContent("    if (input.$key$ !== undefined && input.$key$ !== null) result.$key$ = input.$key$;", {{"key", key}});
                }
            }

            fromPlainPrinter.addContent("    return result;");
            fromPlainPrinter.addContent("}");

            fromPlainPrinter.nextLine();
        }

        if (!writter(fromPlainPrinter.getOutput(), "FromPlain.ts"))
        {
            logger().error("write FromPlain.ts failed");
            return false;
        }

        return true;
    }

    bool sTSTypesGenerator::generateComplexTypeMeta(sLanguageGenerator::sWriteFileDelegate writter, const sLanguageGenerator::sContext& context)
    {
        sTSPrinter printer;
        printer.addHeader();

        auto types = sTSUtils::getAllSymbolsFromParsers<flatbuffers::StructDef>(context.parsers);
        sTSUtils::importDependentTypes(printer, context.parsers);
        sTSUtils::importDependentEnums(printer, context.parsers);

        printer.addContent("export type Meta = { key: string; type: string; }");

        for (auto type : types)
        {
            std::string typeName = type->name;
            printer.addContent("function get$type$Meta(): Meta[] {", {{"type", typeName}});
            printer.addContent("    let metas: Meta[] = [];");

            auto& fields = type->fields;
            for (const auto& [key, field] : fields.dict)
            {
                if (field->deprecated)
                    continue;

                if (field->value.type.base_type == flatbuffers::BASE_TYPE_STRUCT)
                {
                    printer.addContent("    metas.push({key: \"$key$\", type: \"$type$\"});", {{"key", key}, {"type", field->value.type.struct_def->name}});
                }
                else if (field->value.type.base_type == flatbuffers::BASE_TYPE_VECTOR && field->value.type.VectorType().base_type == flatbuffers::BASE_TYPE_STRUCT)
                {
                    printer.addContent("    metas.push({key: \"$key$\", type: \"$type$\"});", {{"key", key}, {"type", field->value.type.VectorType().struct_def->name + "[]"}});
                }
                else if (field->value.type.base_type == flatbuffers::BASE_TYPE_UNION)
                {
                    auto enumDef = field->value.type.enum_def;
                    std::string enumNameSpace = enumDef->name;
                    auto types = enumDef->Vals();
                    std::string unityTypeString = "";
                    for (auto it = types.begin(); it != types.end(); ++it)
                    {
                        auto& type = *it;
                        auto unionType = type->union_type.struct_def;
                        if (unionType)
                        {
                            auto typeName = unionType->name;
                            if (unityTypeString.empty())
                                unityTypeString = typeName;
                            else
                                unityTypeString = unityTypeString + " | " + typeName;
                        }
                    }

                    printer.addContent("    metas.push({key: \"$key$\", type: \"$type$\"});", {{"key", key}, {"type", unityTypeString}});
                }
                else
                {
                    printer.addContent("    metas.push({key: \"$key$\", type: \"$type$\"});", {{"key", key}, {"type", sTSUtils::genTypeName(field->value.type)}});
                }
            }

            printer.addContent("    return metas;");
            printer.addContent("}");

            printer.nextLine();
        }

        printer.addContent("export const ComplexTypeMeta = {");

        for (auto type : types)
        {
            std::string typeName = type->name;
            printer.addContent("    $type$: get$type$Meta(),", {{"type", typeName}});
        }

        printer.addContent("}");

        if (!writter(printer.getOutput(), "ComplexTypeMeta.ts"))
        {
            logger().error("write ComplexTypeMeta.ts failed");
            return false;
        }

        return true;
    }
}
