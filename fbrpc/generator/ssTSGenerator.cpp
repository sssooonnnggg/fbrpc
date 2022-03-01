#include <array>
#include <algorithm>

#include "flatbuffers/idl.h"
#include "ssTSPrinter.h"
#include "ssTSGenerator.h"
#include "ssUtils.h"
#include "fbrpc/core/ssLogger.h"

namespace fbrpc
{
	using namespace generatorUtils;

	namespace
	{
		constexpr std::string_view BindingTargetName = "fbrpc_binding";
		constexpr std::string_view UnsupportedType = "TODO: Unsupported Type";
		constexpr std::string_view UnsupportedValue = "TODO: Unsupported Value";

		std::string toFullPath(flatbuffers::Definition* def)
		{
			return std::string("./") + toNameSpacePath(def->defined_namespace) + "/" + toDasherizedCase(def->name);
		}

		template <class SymbolType, typename = std::enable_if_t<std::is_base_of_v<flatbuffers::Definition, SymbolType>>>
		std::unordered_set<SymbolType*> getAllSymbolsFromParsers(const sLanguageGenerator::sContext& context)
		{
			std::unordered_set<SymbolType*> exportedTypes;
			std::unordered_set<std::string> typeNames;
			for (auto parser : context.parsers)
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
	}

	bool sTSGenerator::start(flatbuffers::ServiceDef *service)
	{
		return generateTSDeclareFile(service) && generateTSWrapperFile(service);
	}

	bool sTSGenerator::finish(sContext context)
	{
		return finishTSDeclareFile(context.services) && finishTSWrapperFile(context);
	}

	bool sTSGenerator::generateTSDeclareFile(flatbuffers::ServiceDef *service)
	{
		auto serviceName = service->name;
		sTSPrinter printer;
		printer.addHeader();
		{
			auto scope = printer.addExport(std::string("const ") + serviceName + ":");
			for (auto call : service->calls.vec)
				printer.addContent(
					"$apiName$: (req: Uint8Array, callback: (res: Uint8Array) => void) => void", 
					{{"apiName", call->name}});
		}
		return writter()(printer.getOutput(), serviceName + ".d.ts");
	}
	bool sTSGenerator::generateTSWrapperFile(flatbuffers::ServiceDef* service)
	{
		auto serviceName = service->name;
		sTSPrinter printer;
		printer.addHeader();
		printer.addImport("* as flatbuffers", "flatbuffers");
		printer.addImport(std::string("{ ") + serviceName + " }", std::string("./") + BindingTargetName.data());

		importServiceDependentTypes(printer, service);

		auto& calls = service->calls.vec;
		bool hasComponentPropertySetter = (std::find_if(
			calls.begin(),
			calls.end(),
			[](auto&& call) { return hasTag(call->request, sTag::ComponentPropertySetter); }
		) != calls.end());

		if (hasComponentPropertySetter)
			printer.addImport("{ setComponentProperty }", "./ComponentPropertyHelper");

		printer.nextLine();
		auto importIndex = printer.length();

		{
			auto scope = printer.addExport(std::string("class ") + serviceName + "API");
			for (auto call : service->calls.vec)
			{
				if (isEvent(call->response))
					generateEventCall(printer, service, call);
				else if (hasTag(call->request, sTag::ComponentPropertySetter))
					generateSetComponentPropertyCall(printer, service, call, importIndex);
				else
					generateDefaultCall(printer, service, call);
			}
		}

		return writter()(printer.getOutput(), serviceName + "API.ts");
	}

	bool sTSGenerator::generateEventCall(sTSPrinter& printer, flatbuffers::ServiceDef* service, flatbuffers::RPCCall* call)
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

		sPrinter::sVarsMap vars = {
			{"api", api},
			{"requestType", requestType},
			{"responseType", responseType},
			{"serviceName", serviceName},
			{"responseName", responseName},
			{"filter", filter},
			{"packFilter", packFilter}
		};
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

	bool sTSGenerator::generateSetComponentPropertyCall(
		sTSPrinter& printer, flatbuffers::ServiceDef* service, flatbuffers::RPCCall* call, std::size_t importIndex)
	{
		auto componentContext = getComponentTypes(call);
		if (componentContext.types.empty())
			return false;

		if (!generateComponentPropertyHelper(service, call, componentContext))
			return false;

		printer.moveTo(importIndex);
		importComponentTypes(printer, componentContext);
		printer.moveBack();

		std::string overloadMethods;
		for (const auto& [type, _] : componentContext.types)
			printer.addContent(
				"static setComponentProperty(objectIds: string[], type: fbComponent.$type$, args: Partial<Plain<$type$T>>): Promise<SetComponentPropertyResponseT>;",
				{ {"type", type} }
			);

		printer.addContent(R"#(
static setComponentProperty(...args: Parameters<typeof setComponentProperty>): Promise<SetComponentPropertyResponseT> {
	return setComponentProperty(...args);
})#");

		return true;
	}

	sTSGenerator::sComponentContext sTSGenerator::getComponentTypes(flatbuffers::RPCCall* call)
	{
		std::map<std::string, std::vector<flatbuffers::FieldDef*>> dependentTypes;
		auto addDependentType = [&dependentTypes](const flatbuffers::EnumVal& enumValue)
		{
			if (dependentTypes.find(enumValue.name) == dependentTypes.end())
			{
				std::vector<flatbuffers::FieldDef*> propertyFields;
				if (enumValue.union_type.struct_def)
					for (const auto& propertyValue : enumValue.union_type.struct_def->fields.vec)
						propertyFields.push_back(propertyValue);

				dependentTypes[enumValue.name] = propertyFields;
			}
		};

		auto& requestFields = call->request->fields.vec;
		auto componentField = std::find_if(
			requestFields.begin(),
			requestFields.end(),
			[](auto&& field) { return field->value.type.base_type == flatbuffers::BASE_TYPE_STRUCT; }
		);

		if (componentField == requestFields.end())
		{
			logger().error("generate component setter failed, no component field");
			return {};
		}

		auto componentStructDef = (*componentField)->value.type.struct_def;
		std::string ns = toNameSpacePath(componentStructDef->defined_namespace);

		for (const auto& structDefField : componentStructDef->fields.vec)
			if (structDefField->value.type.base_type == flatbuffers::BASE_TYPE_UNION && structDefField->value.type.enum_def)
				for (const auto& enumValue : structDefField->value.type.enum_def->Vals())
					if (enumValue->union_type.base_type != flatbuffers::BASE_TYPE_NONE)
						addDependentType(*enumValue);

		return { ns, dependentTypes };
	}

	bool sTSGenerator::generateComponentPropertyHelper(
		flatbuffers::ServiceDef* service, 
		flatbuffers::RPCCall* call,
		const sComponentContext& context)
	{
		auto serviceName = service->name;

		sTSPrinter printer;
		printer.addHeader();

		printer.addImport("* as flatbuffers", "flatbuffers");
		printer.addImport(std::string("{ ") + serviceName + " }", std::string("./") + BindingTargetName.data());
		printer.addImport(
			std::string("{ ") + call->request->name + " }",
			toFullPath(call->request));
		printer.addImport(
			std::string("{ ") + call->response->name + ", " + call->response->name + "T }", 
			toFullPath(call->response));

		importComponentTypes(printer, context);

		printer.addImport("{ Plain }", "./FromPlain");
		for (const auto& [type, _] : context.types)
			printer.addImport(std::string("{ fromPlain" + type + "T }"), "./FromPlain");

		printer.nextLine();

		std::string createSetterCodeSnippet("switch (type) {");
		for (const auto& [type, _] : context.types)
		{
			createSetterCodeSnippet += printer.format(
				R"#(
        case fbComponent.$type$: {
            input = fromPlain$type$T(args as Plain<$type$T>);
            setter = new $type$Setter(objectIds) as any; 
            break;
        })#", { {"type", type} });
		}
		createSetterCodeSnippet += "    default: break;\n    }\n";

		sPrinter::sVarsMap vars = {
			{"createSetterCodeSnippet", createSetterCodeSnippet}
		};

		printer.addContent(
			R"#(
export async function setComponentProperty(objectIds: string[], type: any, args: any): Promise<SetComponentPropertyResponseT> {
    var setter: any;
    let input: any;
    $createSetterCodeSnippet$
    for(let key of Object.keys(args))
        setter[key] = input[key];
    return await setter.finish();
})#", vars);
		printer.addContent(
			R"#(async function sendSetComponentPropertyRequest(builder: flatbuffers.Builder, ids: string[], componentOffset: flatbuffers.Offset): Promise<SetComponentPropertyResponseT>{
	return new Promise(resolve => {
		const idsOffset = SetComponentPropertyRequest.createIdsVector(builder, builder.createObjectOffsetList(ids));
		SetComponentPropertyRequest.startSetComponentPropertyRequest(builder);
		SetComponentPropertyRequest.addIds(builder, idsOffset);
		SetComponentPropertyRequest.addComponent(builder, componentOffset);
		builder.finish(SetComponentPropertyRequest.endSetComponentPropertyRequest(builder));
		Sandbox.setComponentProperty(builder.asUint8Array(), res => {
			let buffer = new flatbuffers.ByteBuffer(res);
			let response = SetComponentPropertyResponse.getRootAsSetComponentPropertyResponse(buffer);
			resolve(response.unpack());
		});
	});
})#");
		printer.addContent(
			R"#(class BaseSetter {
	ids: string[] = [];
	builder: flatbuffers.Builder = new flatbuffers.Builder();
	valueOrOffsetMap = new Map();
	constructor(ids: string[]) {
		this.ids = ids;
		this.builder.forceDefaults(true);
				}
	addSettedProperties() {
		for (let key of this.valueOrOffsetMap.keys())
			(this as any)["add_" + key](this.valueOrOffsetMap.get(key));
	}
})#"
);
		for (const auto& depComp : context.types)
		{
			std::string propertyCodeSnippet;
			std::string propertySetterCodeSnippet;
			for (const flatbuffers::FieldDef* propertyDef : depComp.second)
			{
				auto baseType = propertyDef->value.type;
				std::string propertyName = propertyDef->name;
				auto typeName = genTypeName(propertyDef->value.type);

				std::string compName = depComp.first;
				std::string propertyNameCamel = toCamel(propertyDef->name, true);
				std::string genOffsetOrValue = this->genOffsetOrValue(propertyDef->value.type, compName, propertyNameCamel);

				auto propertyCodeSnippetTemplate = R"#(
    set $propertyName$(value: $typeName$) {
        if (value !== null)
            this.valueOrOffsetMap.set("$propertyName$", $genOffsetOrValue$);
			}
)#";
				propertyCodeSnippet += printer.format(
					propertyCodeSnippetTemplate,
					{{"propertyName", propertyName}, {"typeName", typeName}, {"genOffsetOrValue", genOffsetOrValue}}
				);

				bool needOffset = !flatbuffers::IsScalar(baseType.base_type) && !flatbuffers::IsStruct(baseType);

				std::string offsetOrValueParam = needOffset ? "offset: number" : "value: " + genTypeName(propertyDef->value.type);
				std::string offsetOrValue = needOffset ? "offset" : (flatbuffers::IsStruct(propertyDef->value.type) ? "value.pack(this.builder)" : "value");

				auto propertySetterCodeSnippetTemplate = R"#(
    add_$propertyName$($offsetOrValueParam$) {
        $compName$.add$propertyNameCamel$(this.builder, $offsetOrValue$);
    }
)#";
				propertySetterCodeSnippet += printer.format(
					propertySetterCodeSnippetTemplate, 
					{ 
						{"propertyName", propertyName}, 
						{"offsetOrValueParam", offsetOrValueParam}, 
						{"compName", compName}, 
						{"propertyNameCamel", propertyNameCamel}, 
						{"offsetOrValue", offsetOrValue}
					}
				);
			}

			sPrinter::sVarsMap vars = {
				{"componentType", depComp.first},
				{"propertyCodeSnippet", propertyCodeSnippet},
				{"propertySetterCodeSnippet", propertySetterCodeSnippet} 
			};

			printer.addContent(
				R"#(class $componentType$Setter extends BaseSetter {
    constructor(ids: string[]) {
        super(ids);
    }
    async finish(): Promise<SetComponentPropertyResponseT> {
        $componentType$.start$componentType$(this.builder);
		super.addSettedProperties();
        const componentUnionOffset = $componentType$.end$componentType$(this.builder);
        const componentOffset = Component.createComponent(
            this.builder,
            fbComponent.$componentType$,
            componentUnionOffset
        );
        return sendSetComponentPropertyRequest(this.builder, this.ids, componentOffset);
    }
$propertyCodeSnippet$
$propertySetterCodeSnippet$
})#", vars);
		}

		printer.nextLine();

		printer.addContent("export type ComponentTypeMap = {");
		for (const auto& [type, _] : context.types)
			printer.addContent("    [fbComponent.$type$]: Plain<$type$T>,", {{"type", type}});

		printer.addContent("}");

		return writter()(printer.getOutput(), "ComponentPropertyHelper.ts");
	}

	void sTSGenerator::importComponentTypes(sTSPrinter& printer, const sComponentContext& context)
	{
		auto nameSpaceDir = toDasherizedCase(context.ns);
		printer.addContent("import { Component } from './" + nameSpaceDir + "/component';");
		printer.addContent("import { fbComponent } from './" + nameSpaceDir + "/fb-component';");

		std::set<std::string> importedClass;
		for (const auto& type : context.types)
		{
			auto target = std::string("{ ") + type.first + ", " + type.first + "T" + " }";
			auto from = std::string("./") + nameSpaceDir + "/" + toDasherizedCase(type.first);
			printer.addImport(target, from);

			for (const flatbuffers::FieldDef* propertyDef : type.second)
			{
				auto& type = propertyDef->value.type;
				if (type.enum_def && importedClass.find(type.enum_def->name) == importedClass.end())
	{
					importedClass.insert(type.enum_def->name);
					std::string enumTarget = std::string("{ ") + type.enum_def->name + " }";
					std::string enumFrom = toFullPath(type.enum_def);
					printer.addImport(enumTarget, enumFrom);
				}
				if (type.struct_def && importedClass.find(type.struct_def->name) == importedClass.end())
				{
					importedClass.insert(type.struct_def->name);
					std::string structTarget = std::string("{ ") + type.struct_def->name + 'T' + " }";
					std::string structFrom = toFullPath(type.struct_def);
					printer.addImport(structTarget, structFrom);
				}
			}
		}
	}

	bool sTSGenerator::generateDefaultCall(sTSPrinter& printer, flatbuffers::ServiceDef* service, flatbuffers::RPCCall* call)
	{
		auto serviceName = service->name;
		auto api = call->name;
		auto requestName = call->request->name;
		auto responseName = call->response->name;

		auto requestType = requestName + "T";
		auto responseType = responseName + "T";
		bool isEmptyRequest = call->request->fields.vec.empty();

		sPrinter::sVarsMap vars = {
			{"api", api},
			{"requestType", requestType},
			{"responseType", responseType},
			{"serviceName", serviceName},
			{"responseName", responseName}
		};

		if (isEmptyRequest)
			printer.addContent("static $api$() : Promise<$responseType$>;", vars);
		else
			printer.addContent("static $api$(req: Partial<Plain<$requestType$>>) : Promise<$responseType$>;", vars);

		printer.addContent(
			R"#(static $api$(req: $requestType$): Promise<$responseType$>;
static $api$(req?: any): Promise<$responseType$> {
    return new Promise(resolve => {
        let builder = new flatbuffers.Builder();
        let request = req instanceof $requestType$ ? req : (req === undefined ? new $requestType$() : fromPlain$requestType$(req));
        builder.finish(request.pack(builder));
        $serviceName$.$api$(builder.asUint8Array(), res => {
            let buffer = new flatbuffers.ByteBuffer(res);
            let response = $responseName$.getRootAs$responseName$(buffer);
            resolve(response.unpack());
        });
    })
})#", vars);
		return true;
	}
	bool sTSGenerator::finishTSDeclareFile(const std::vector<flatbuffers::ServiceDef *> &services)
	{
		sTSPrinter printer;
		printer.addHeader();
		for (auto service : services)
			printer.addContent(std::string("export { ") + service->name + " } from './" + service->name + "'");
		return writter()(printer.getOutput(), std::string(BindingTargetName.data()) + ".d.ts");
	}

	bool sTSGenerator::finishTSWrapperFile(const sContext& context)
	{
		sTSPrinter printer;
		printer.addHeader();

		generateFromPlainFunction(printer, context);
		printer.nextLine();

		exportDependentTypes(printer, context);

		printer.nextLine();
		exportDependentEnums(printer, context);

		printer.nextLine();
		for (auto service : context.services)
			printer.addContent(std::string("export { ") + service->name + "API } from './" + service->name + "API'");

		printer.nextLine();
		printer.addContent("export * from './internal'");
		printer.addContent("export { Plain } from './FromPlain'");
		printer.addContent("export { ComponentTypeMap } from './ComponentPropertyHelper'");

		return writter()(printer.getOutput(), "FlatBufferAPI.ts");
	}

	void sTSGenerator::importServiceDependentTypes(sTSPrinter& printer, flatbuffers::ServiceDef* service)
	{
		auto nameSpace = service->defined_namespace->components;
		std::string ns = nameSpace.size() > 0 ? nameSpace[0] : "";

		struct sDependentType
		{
			std::string name;
			bool isRequest;
		};

		std::vector<sDependentType> dependentTypes;
		std::set<std::string> dependentTypesSet;

		auto addDependentType = [&dependentTypes, &dependentTypesSet](std::string type, bool isRequest)
		{
			if (dependentTypesSet.find(type) == dependentTypesSet.end())
			{
				dependentTypesSet.insert(type);
				dependentTypes.push_back({ type, isRequest });
			}
		};

		for (auto call : service->calls.vec)
		{
			addDependentType(call->request->name, true);
			addDependentType(call->response->name, false);
		}

		auto nameSpaceDir = toDasherizedCase(ns);

		for (const auto& type : dependentTypes)
		{
			printer.addImport(
				std::string("{ ") + type.name + "," + type.name + "T }",
				std::string("./") + nameSpaceDir + "/" + toDasherizedCase(type.name));

			if (type.isRequest)
				printer.addImport(std::string("{ fromPlain") + type.name + "T }", "./FromPlain");
		}

		printer.addImport("{ Plain }", "./FromPlain");
	}

	bool sTSGenerator::generateFromPlainFunction(sTSPrinter& printer, const sContext& context)
	{
		sTSPrinter fromPlainPrinter;
		fromPlainPrinter.addHeader();

		auto types = getAllSymbolsFromParsers<flatbuffers::StructDef>(context);
		importDependentTypes(fromPlainPrinter, context);
		importDependentEnums(fromPlainPrinter, context);

		fromPlainPrinter.nextLine();
		fromPlainPrinter.addContent(R"#(
type InnerType<T> = T extends Array<infer Type> ? Type : never;
type IsArray<T> = T extends any[] ? true : false;
export type Plain<T> = { [P in Exclude<keyof T, 'pack'>]: IsArray<T[P]> extends true ?
	(InnerType<T[P]> extends object ? Plain<InnerType<T[P]>>[] : T[P]) :
	(T[P] extends object | null ? Plain<NonNullable<T[P]>> : T[P]);};)#"
		);
		fromPlainPrinter.nextLine();

		std::vector<std::string> fromPlainFunctions;

		for (auto type : types)
		{
			std::string typeName = type->name + "T";
			fromPlainPrinter.addContent("export function fromPlain$type$(input: Plain<$type$>): $type$ {", {{"type", typeName}});
			fromPlainPrinter.addContent("    let result = new $type$()", { {"type", typeName} });

			fromPlainFunctions.push_back(std::string("{ fromPlain") + typeName + " }");

			auto& fields = type->fields;
			for (const auto& [key, field] : fields.dict)
			{
				if (field->value.type.base_type == flatbuffers::BASE_TYPE_STRUCT)
				{
					fromPlainPrinter.addContent("    if (input.$key$ !== undefined && input.$key$ !== null) result.$key$ = fromPlain$type$(input.$key$!);",
						{ {"key", key}, {"type", field->value.type.struct_def->name + "T"} });
				}
				else if (field->value.type.base_type == flatbuffers::BASE_TYPE_VECTOR
					&& field->value.type.VectorType().base_type == flatbuffers::BASE_TYPE_STRUCT)
				{
					fromPlainPrinter.addContent("    if (input.$key$ !== undefined && input.$key$ !== null) result.$key$ = [...input.$key$.map(v => fromPlain$type$(v!))];",
						{ {"key", key}, {"type", field->value.type.VectorType().struct_def->name + "T"} });
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
								"    if (result.$key$_type == $enumNamespace$.$enumName$) result.$key$ = fromPlain$typeName$(input.$key$! as Plain<$typeName$>);",
								{ {"key", key}, {"enumNamespace", enumNameSpace}, {"enumName", unionType->name}, {"typeName", typeName}}
							);
						}
					}
				}
				else
				{
					fromPlainPrinter.addContent("    result.$key$ = input.$key$;", { {"key", key} });
				}	
			}
				

			fromPlainPrinter.addContent("    return result;");
			fromPlainPrinter.addContent("}");

			fromPlainPrinter.nextLine();
		}

		if (!writter()(fromPlainPrinter.getOutput(), "FromPlain.ts"))
		{
			logger().error("write FromPlain.ts failed");
			return false;
		}

		return true;
	}

	void sTSGenerator::importDependentTypes(sTSPrinter& printer, const sContext& context)
	{
		auto types = getAllSymbolsFromParsers<flatbuffers::StructDef>(context);
		for (auto type : types)
			printer.addContent(std::string("import { ") + type->name + "T } from '" + toFullPath(type) + "'");
	}

	void sTSGenerator::exportDependentTypes(sTSPrinter& printer, const sContext& context)
	{
		auto types = getAllSymbolsFromParsers<flatbuffers::StructDef>(context);
		for (auto type : types)
			printer.addContent(std::string("export { ") + type->name + "T } from '" + toFullPath(type) + "'");
	}

	void sTSGenerator::importDependentEnums(sTSPrinter& printer, const sContext& context)
	{
		auto types = getAllSymbolsFromParsers<flatbuffers::EnumDef>(context);
		for (auto type : types)
			printer.addContent(std::string("import { ") + type->name + " } from '" + toFullPath(type) + "'");
	}

	void sTSGenerator::exportDependentEnums(sTSPrinter& printer, const sContext& context)
	{
		auto types = getAllSymbolsFromParsers<flatbuffers::EnumDef>(context);
		for (auto type : types)
			printer.addContent(std::string("export { ") + type->name + " } from '" + toFullPath(type) + "'");
	}

	std::string sTSGenerator::genTypeName(const flatbuffers::Type& type)
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

	std::string sTSGenerator::genOffsetOrValue(const flatbuffers::Type& type, const std::string& compName, const std::string& propertyName)
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
		case flatbuffers::BASE_TYPE_VECTOR:
		{
			if (type.VectorType().base_type == flatbuffers::BASE_TYPE_STRING)
				return "this.builder.createObjectOffsetList(value)";
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
}